#define _POSIX_C_SOURCE 200809L
#define _FILE_OFFSET_BITS 64
#include "register_io.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "app_config.h"

static int check_address(uint64_t address)
{
    if ((address & 3) || address < REG_BASE || REG_SIZE < 4 ||
        address - REG_BASE > REG_SIZE - 4) {
        errno = EINVAL;
        return -1;
    }
    return 0;
}

#ifdef REAL_MMIO
static int access32(uint64_t address, uint32_t *value, int write_access)
{
    if (check_address(address)) return -1;
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0) { errno = EINVAL; return -1; }
    uint64_t page = address - address % (uint64_t)page_size;
    if (page > INT64_MAX) { errno = EOVERFLOW; return -1; }
    size_t offset = (size_t)(address - page);
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) return -1;
    void *mapped = mmap(NULL, (size_t)page_size, PROT_READ | PROT_WRITE,
                        MAP_SHARED, fd, (off_t)page);
    int saved = errno;
    close(fd);
    if (mapped == MAP_FAILED) { errno = saved; return -1; }
    volatile uint32_t *reg = (volatile uint32_t *)((char *)mapped + offset);
    if (write_access) *reg = *value;
    else *value = *reg;
    int rc = munmap(mapped, (size_t)page_size);
    return rc;
}
#else
#ifndef MOCK_FILE
#define MOCK_FILE "mock_phys_mem.bin"
#endif
static int access32(uint64_t address, uint32_t *value, int write_access)
{
    if (check_address(address)) return -1;
    if (address - REG_BASE > INT64_MAX - 4) { errno = EOVERFLOW; return -1; }
    off_t offset = (off_t)(address - REG_BASE);
    const char *path = getenv("DEBUG_MEMORY_FILE");
    if (!path || !*path) path = MOCK_FILE;
    int fd = open(path, O_RDWR | O_CREAT, 0600);
    if (fd < 0) return -1;
    size_t done = 0;
    if (!write_access) *value = 0;
    while (done < sizeof *value) {
        ssize_t n = write_access ? pwrite(fd, (char *)value + done, sizeof *value - done, offset + (off_t)done)
                                 : pread(fd, (char *)value + done, sizeof *value - done, offset + (off_t)done);
        if (n < 0 && errno == EINTR) continue;
        if (n < 0 || (n == 0 && write_access)) {
            int saved = n < 0 ? errno : EIO;
            close(fd); errno = saved; return -1;
        }
        if (n == 0) break;
        done += (size_t)n;
    }
    if (close(fd)) return -1;
    return 0;
}
#endif

int reg_read32(uint64_t address, uint32_t *value)
{
    if (!value) { errno = EINVAL; return -1; }
    return access32(address, value, 0);
}

int reg_write32(uint64_t address, uint32_t value)
{
    return access32(address, &value, 1);
}

static int check_field(uint32_t mask, unsigned shift)
{
    if (shift >= 32 || !mask || (mask & ((UINT32_C(1) << shift) - 1))) {
        errno = EINVAL; return -1;
    }
    return 0;
}
int reg_read_field32(uint64_t address, uint32_t mask, unsigned shift, uint32_t *value)
{
    uint32_t raw;
    if (!value) { errno = EINVAL; return -1; }
    if (check_field(mask, shift) || reg_read32(address, &raw)) return -1;
    *value = (raw & mask) >> shift;
    return 0;
}
int reg_write_field32(uint64_t address, uint32_t mask, unsigned shift, uint32_t value)
{
    uint32_t raw;
    if (check_field(mask, shift)) return -1;
    if (value & ~(mask >> shift)) { errno = ERANGE; return -1; }
    if (reg_read32(address, &raw)) return -1;
    return reg_write32(address, (raw & ~mask) | (value << shift));
}
