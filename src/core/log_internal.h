#ifndef LOG_INTERNAL_H
#define LOG_INTERNAL_H

#include <stddef.h>
#define LOG_LINES 128
#define LOG_WIDTH 512

typedef struct {
    size_t count;
    char lines[LOG_LINES][LOG_WIDTH];
} log_snapshot;

void log_set_live(int enabled);
void log_get_snapshot(log_snapshot *snapshot);

#endif
