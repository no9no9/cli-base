#define _POSIX_C_SOURCE 200809L
#include "register_io.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/* Change these two values to match the target register aperture. */
#ifndef REG_BASE
#define REG_BASE UINT64_C(0x40000000)
#endif
#ifndef REG_SIZE
#define REG_SIZE UINT64_C(0x1000)
#endif

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
    off_t offset = (off_t)(address - REG_BASE);
    int fd = open(MOCK_FILE, O_RDWR | O_CREAT, 0600);
    if (fd < 0) return -1;
    ssize_t n = write_access ? pwrite(fd, value, sizeof *value, offset)
                             : pread(fd, value, sizeof *value, offset);
    int saved = errno;
    if (!write_access && n == 0) { *value = 0; n = sizeof *value; }
    if (close(fd) != 0 && n == sizeof *value) return -1;
    if (n != sizeof *value) { errno = n < 0 ? saved : EIO; return -1; }
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
