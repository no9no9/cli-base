#ifndef TERMINAL_UI_H
#define TERMINAL_UI_H
#include <stddef.h>

/* One CLI session at a time. start returns 1 for live UI, 0 for plain mode,
 * -1 on error. read_line returns 1, 0 (EOF), or -1 (rejected input). */
int terminal_start(void);
void terminal_stop(void);
int terminal_read_line(const char *prompt, char *line, size_t capacity);
void terminal_set_busy(const char *command);
int terminal_interrupted(void);
#endif
