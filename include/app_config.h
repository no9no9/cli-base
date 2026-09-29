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
#define MOCK_FILE "./dev/memory.bin"
#endif
/* Periodic 32-bit display. Set to a register safe for repeated reads. */
#ifndef WATCH_ADDRESS
#define WATCH_ADDRESS REG_BASE
#endif
#ifndef WATCH_ENABLED
#define WATCH_ENABLED 1
#endif
#endif
