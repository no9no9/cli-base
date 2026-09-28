#ifndef REGISTER_IO_H
#define REGISTER_IO_H

#include <stdint.h>

/* Address is a byte address. Returns 0 on success, -1 with errno set. */
int reg_read32(uint64_t address, uint32_t *value);
int reg_write32(uint64_t address, uint32_t value);

#endif
