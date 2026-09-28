#ifndef REGISTER_IO_H
#define REGISTER_IO_H

#include <stdint.h>

/* Address is a byte address. Returns 0 on success, -1 with errno set. */
int reg_read32(uint64_t address, uint32_t *value);
int reg_write32(uint64_t address, uint32_t value);

/* mask is positioned in the word; shift is the bit position.
 * Field write performs non-atomic read-modify-write. Use only ordinary R/W
 * registers, not W1C, read-clear, or concurrently modified registers. */
int reg_read_field32(uint64_t address, uint32_t mask, unsigned shift, uint32_t *value);
int reg_write_field32(uint64_t address, uint32_t mask, unsigned shift, uint32_t value);

#endif
