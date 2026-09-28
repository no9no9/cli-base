#include "cli.h"

#include <stdio.h>
#include <string.h>

#define LINE_SIZE 1024
#define MAX_ARGS 32
#define MAX_DEPTH 16

typedef struct {
    const cli_menu *path[MAX_DEPTH];
    size_t depth;
} menu_state;

static void print_help(const cli_menu *menu)
{
    puts("help                 Show commands\n"
         "back                 Parent menu\n"
         "exit                 Quit application");

    for (size_t i = 0; i < menu->count; ++i) {
        const cli_command *command = menu->commands[i];
        printf("%-28s %s\n", command->usage, command->description);
    }
}

/* Return 1 for a complete line, 0 for EOF, and -1 for a rejected line. */
static int read_line(char *line, size_t capacity)
{
    if (!fgets(line, capacity, stdin)) {
        return 0;
    }
    if (strchr(line, '\n') || feof(stdin)) {
        return 1;
    }

    int ch = getchar();
    if (ch == '\n' || ch == EOF) {
        return 1;
    }
    while ((ch = getchar()) != '\n' && ch != EOF) {
    }
    fputs("Input line too long\n", stderr);
    return -1;
}

static int split_arguments(char *line, char **argv)
{
    int argc = 0;
    char *token = strtok(line, " \t\r\n");

    while (token) {
        if (argc == MAX_ARGS) {
            fputs("Too many arguments\n", stderr);
            return -1;
        }
        argv[argc++] = token;
        token = strtok(NULL, " \t\r\n");
    }
    argv[argc] = NULL;
    return argc;
}

static const cli_command *find_command(const cli_menu *menu, const char *name)
{
    for (size_t i = 0; i < menu->count; ++i) {
        if (strcmp(name, menu->commands[i]->name) == 0) {
            return menu->commands[i];
        }
    }
    return NULL;
}

static void run_handler(const cli_command *command, int argc, char **argv)
{
    if (!command->handler) {
        fprintf(stderr, "Missing handler: %s\n", command->name);
        return;
    }

    int result = command->handler(argc, argv);
    if (result == CLI_USAGE) {
        fprintf(stderr, "Usage: %s\n", command->usage);
    } else if (result != CLI_OK) {
        fprintf(stderr, "Command failed: %s (%d)\n", command->name, result);
    }
}

/* Resolve against a copy: a one-line command must not change the active menu.
 * Commit the full path only when entering a menu or explicitly going back. */
static int dispatch(menu_state *state, int argc, char **argv)
{
    menu_state next = *state;

    for (int offset = 0; offset < argc; ++offset) {
        const cli_menu *menu = next.path[next.depth];
        const char *name = argv[offset];
        int is_help = strcmp(name, "help") == 0;
        int is_back = strcmp(name, "back") == 0;
        int is_exit = strcmp(name, "exit") == 0;

        if (is_help || is_back || is_exit) {
            if (argc - offset != 1) {
                fprintf(stderr, "Usage: %s\n", name);
                return 0;
            }
            if (is_exit) {
                return 1;
            }
            if (is_help) {
                print_help(menu);
            } else {
                if (next.depth > 0) {
                    --next.depth;
                }
                *state = next;
            }
            return 0;
        }

        const cli_command *command = find_command(menu, name);
        if (!command) {
            fprintf(stderr, "Unknown command: %s\n", name);
            return 0;
        }
        if (!command->submenu) {
            run_handler(command, argc - offset, argv + offset);
            return 0;
        }
        if (next.depth + 1 == MAX_DEPTH) {
            fputs("Menu nesting limit reached\n", stderr);
            return 0;
        }
        next.path[++next.depth] = command->submenu;
    }

    *state = next;
    return 0;
}

int cli_run_menu(const cli_menu *root)
{
    menu_state state = { .path = {root}, .depth = 0 };
    char line[LINE_SIZE];
    char *argv[MAX_ARGS + 1];

    for (;;) {
        printf("%s> ", state.path[state.depth]->prompt);
        fflush(stdout);

        int result = read_line(line, sizeof line);
        if (result == 0) {
            return ferror(stdin) ? CLI_ERROR : CLI_OK;
        }
        if (result < 0) {
            continue;
        }
        int argc = split_arguments(line, argv);
        if (argc > 0 && dispatch(&state, argc, argv)) {
            return CLI_OK;
        }
    }
}

int cli_run(const cli_command *const *commands, size_t count)
{
    const cli_menu root = { .prompt = "cli", .commands = commands, .count = count };
    return cli_run_menu(&root);
}
