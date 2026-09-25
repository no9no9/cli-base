#include "cli.h"

#include <stdio.h>
#include <string.h>

#define CLI_LINE_SIZE 512
#define CLI_MAX_ARGS 32

static void discard_line(void)
{
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) {}
}

int cli_run(const cli_command *commands, size_t count)
{
    char line[CLI_LINE_SIZE];
    char *argv[CLI_MAX_ARGS];

    for (;;) {
        fputs("cli> ", stdout);
        fflush(stdout);
        if (!fgets(line, sizeof line, stdin)) {
            putchar('\n');
            return ferror(stdin) ? 1 : 0;
        }
        if (!strchr(line, '\n') && !feof(stdin)) {
            discard_line();
            fputs("Input line too long\n", stderr);
            continue;
        }

        int argc = 0;
        char *token = strtok(line, " \t\r\n");
        while (token && argc < CLI_MAX_ARGS) {
            argv[argc++] = token;
            token = strtok(NULL, " \t\r\n");
        }
        if (token) {
            fputs("Too many arguments\n", stderr);
            continue;
        }
        if (!argc) continue;
        if (!strcmp(argv[0], "exit")) return 0;
        if (!strcmp(argv[0], "help")) {
            puts("help                 Show commands\nexit                 Quit");
            for (size_t i = 0; i < count; ++i)
                printf("%-20s %s\n", commands[i].usage, commands[i].description);
            continue;
        }

        size_t i;
        for (i = 0; i < count; ++i) {
            if (!strcmp(argv[0], commands[i].name)) {
                if (commands[i].handler(argc, argv) != 0)
                    fprintf(stderr, "Usage: %s\n", commands[i].usage);
                break;
            }
        }
        if (i == count) fprintf(stderr, "Unknown command: %s\n", argv[0]);
    }
}
