#include "register_cmd.h"
#include "register_io.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

static int parse_number(const char *text, uint64_t *result)
{
    char *end;
    if (!text || !*text || *text == '-') return -1;
    errno = 0;
    unsigned long long parsed = strtoull(text, &end, 0);
    if (errno || *end) return -1;
    *result = (uint64_t)parsed;
    return 0;
}

/* Project-specific handlers live here; they call the reusable I/O layer. */
int read_handler(int argc, char **argv)
{
    uint64_t address;
    uint32_t value;
    if (argc != 2 || parse_number(argv[1], &address)) return -1;
    if (reg_read32(address, &value)) { perror("read"); return -1; }
    printf("0x%08" PRIx32 "\n", value);
    return 0;
}

int write_handler(int argc, char **argv)
{
    uint64_t address, parsed;
    if (argc != 3 || parse_number(argv[1], &address) ||
        parse_number(argv[2], &parsed) || parsed > UINT32_MAX) return -1;
    if (reg_write32(address, (uint32_t)parsed)) { perror("write"); return -1; }
    return 0;
}

