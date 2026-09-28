#include "cli.h"
#include "commands/register_cmd.h"

int main(void)
{
    static const cli_command *const commands[] = {
        &register_read_command,
        &register_write_command,
        &register_menu_command,
    };

    return cli_run(commands, CLI_COUNT(commands));
}
