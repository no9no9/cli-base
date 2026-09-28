#include "cli.h"
#include <stdio.h>
#include <string.h>
#define LINE_SIZE 1024
#define MAX_ARGS 32
#define MAX_DEPTH 16

static void help(const cli_menu *menu)
{
    puts("help                 Show commands\nback                 Parent menu\nexit                 Quit application");
    for (size_t i = 0; i < menu->count; ++i)
        printf("%-28s %s\n", menu->commands[i].usage, menu->commands[i].description);
}

int cli_run_menu(const cli_menu *root)
{
    const cli_menu *stack[MAX_DEPTH] = {root};
    size_t depth = 0;
    char line[LINE_SIZE];
    for (;;) {
        printf("%s> ", stack[depth]->prompt);
        fflush(stdout);
        if (!fgets(line, sizeof line, stdin)) return ferror(stdin) ? CLI_ERROR : CLI_OK;
        if (!strchr(line, '\n') && !feof(stdin)) {
            int ch = getchar();
            if (ch != '\n' && ch != EOF) {
                while ((ch = getchar()) != '\n' && ch != EOF) {}
                fputs("Input line too long\n", stderr);
                continue;
            }
        }
        char *argv[MAX_ARGS + 1];
        int argc = 0;
        char *token = strtok(line, " \t\r\n");
        while (token && argc < MAX_ARGS) {
            argv[argc++] = token;
            token = strtok(NULL, " \t\r\n");
        }
        argv[argc] = NULL;
        if (token) { fputs("Too many arguments\n", stderr); continue; }
        if (!argc) continue;
        const cli_menu *menu = stack[depth];
        int offset = 0;
        for (;;) {
            const char *name = argv[offset];
            if (!strcmp(name, "exit") || !strcmp(name, "back") || !strcmp(name, "help")) {
                if (argc - offset != 1) { fprintf(stderr, "Usage: %s\n", name); break; }
                if (!strcmp(name, "exit")) return CLI_OK;
                if (!strcmp(name, "help")) help(menu);
                else if (depth) --depth;
                break;
            }
            const cli_command *cmd = NULL;
            for (size_t i = 0; i < menu->count; ++i)
                if (!strcmp(name, menu->commands[i].name)) { cmd = &menu->commands[i]; break; }
            if (!cmd) { fprintf(stderr, "Unknown command: %s\n", name); break; }
            if (cmd->submenu) {
                menu = cmd->submenu;
                if (++offset < argc) continue;
                if (depth + 1 == MAX_DEPTH) fputs("Menu nesting limit reached\n", stderr);
                else stack[++depth] = menu;
                break;
            }
            int rc = cmd->handler(argc - offset, argv + offset);
            if (rc == CLI_USAGE) fprintf(stderr, "Usage: %s\n", cmd->usage);
            else if (rc != CLI_OK) fprintf(stderr, "Command failed: %s (%d)\n", cmd->name, rc);
            break;
        }
    }
}

int cli_run(const cli_command *commands, size_t count)
{
    const cli_menu root = {"cli", commands, count};
    return cli_run_menu(&root);
}
