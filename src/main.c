#include "cli.h"
#include "commands/register_cmd.h"
int main(void)
{
    static const cli_command commands[] = {
        {"read", read_handler, "read <address>", "Read a 32-bit register", NULL},
        {"write", write_handler, "write <address> <value>", "Write a 32-bit register", NULL},
        {"reg", NULL, "reg [command ...]", "Register commands (or enter submenu)", &register_menu},
    };
    return cli_run(commands, CLI_COUNT(commands));
}
