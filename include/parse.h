#ifndef PARSE_H
#define PARSE_H
#include <stdint.h>
/* Decimal or 0x-prefixed hexadecimal. Reject signs, trailing text and overflow. */
int parse_u64(const char *text, uint64_t *out);
int parse_u32(const char *text, uint32_t *out);
#endif
