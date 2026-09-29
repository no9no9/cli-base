#include "register_watch.h"
#include "register_io.h"
#include "app_config.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>

void register_watch_format(char *line, size_t capacity)
{
    if (!WATCH_ENABLED) {
        if (capacity) line[0] = '\0';
        return;
    }
    uint32_t value;
    uint64_t address = WATCH_ADDRESS;
    if (reg_read32(address, &value)) {
        int error = errno;
        snprintf(line, capacity, "REG [0x%08" PRIx64 "] = ERROR (errno %d)", address, error);
    } else {
        snprintf(line, capacity, "REG [0x%08" PRIx64 "] = 0x%08" PRIx32, address, value);
    }
}
