#ifndef CLI_H
#define CLI_H

#include <stddef.h>

typedef int (*cli_handler)(int argc, char **argv);

typedef struct {
    const char *name;
    cli_handler handler;
    const char *usage;
    const char *description;
} cli_command;

/* A handler receives argv[0] as its command name. Return 0 for success. */
int cli_run(const cli_command *commands, size_t count);

#endif
