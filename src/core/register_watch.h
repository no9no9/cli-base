#ifndef REGISTER_WATCH_H
#define REGISTER_WATCH_H
#include <stddef.h>
/* Refresh a single watch entry through the shared register I/O layer. */
void register_watch_format(char *line, size_t capacity);
#endif
