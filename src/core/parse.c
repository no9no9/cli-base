#include "parse.h"
#include <errno.h>
#include <stdlib.h>
int parse_u64(const char *text, uint64_t *out)
{
    if (!text || !out || *text < '0' || *text > '9') return -1;
    char *end;
    int base = text[0] == '0' && (text[1] == 'x' || text[1] == 'X') ? 16 : 10;
    errno = 0;
    unsigned long long value = strtoull(text, &end, base);
    if (errno || end == text || *end || value > UINT64_MAX) return -1;
    *out = (uint64_t)value;
    return 0;
}
int parse_u32(const char *text, uint32_t *out)
{
    uint64_t value;
    if (!out || parse_u64(text, &value) || value > UINT32_MAX) return -1;
    *out = (uint32_t)value;
    return 0;
}
