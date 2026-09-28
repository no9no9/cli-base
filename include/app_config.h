#ifndef APP_CONFIG_H
#define APP_CONFIG_H
#include <stdint.h>
/* Project-specific settings. Byte addresses, one contiguous aperture. */
#ifndef REG_BASE
#define REG_BASE UINT64_C(0x40000000)
#endif
#ifndef REG_SIZE
#define REG_SIZE UINT64_C(0x1000)
#endif
#ifndef MOCK_FILE
#define MOCK_FILE "mock_phys_mem.bin"
#endif
#endif
