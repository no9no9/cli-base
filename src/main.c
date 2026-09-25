#include "cli.h"
#include "commands/register_cmd.h"

int main(void)
{
    static const cli_command commands[] = {
        { "read", read_handler, "read <address>", "Read a 32-bit register" },
        { "write", write_handler, "write <address> <value>", "Write a 32-bit register" },
    };
    return cli_run(commands, sizeof commands / sizeof commands[0]);
}
