#ifndef CLI_H
#define CLI_H
#include <stddef.h>
#define CLI_OK 0
#define CLI_USAGE 2
#define CLI_ERROR 1
#define CLI_COUNT(a) (sizeof(a) / sizeof((a)[0]))
typedef int (*cli_handler)(int argc, char **argv);
typedef struct cli_menu cli_menu;
typedef struct {
    const char *name;
    cli_handler handler;
    const char *usage;
    const char *description;
    const cli_menu *submenu; /* Set either handler or submenu. */
} cli_command;
struct cli_menu {
    const char *prompt;
    const cli_command *commands;
    size_t count;
};
/* argv[0] is the local command name; argv[argc] is NULL.
 * Arguments remain valid only during a synchronous handler call.
 * CLI_USAGE prints usage; other nonzero results report execution failure. */
int cli_run_menu(const cli_menu *root);
int cli_run(const cli_command *commands, size_t count);
#endif
