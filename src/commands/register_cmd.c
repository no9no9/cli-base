#include "register_cmd.h"
#include "register_io.h"
#include "parse.h"
#include "app_log.h"

#include <inttypes.h>
#include <stdio.h>

static int read_handler(int argc, char **argv)
{
    uint64_t address;
    uint32_t value;

    if (argc != 2 || parse_u64(argv[1], &address)) {
        return CLI_USAGE;
    }
    if (reg_read32(address, &value)) {
        app_log_errno("read");
        return CLI_ERROR;
    }
    app_logf("0x%08" PRIx32, value);
    return CLI_OK;
}

static int write_handler(int argc, char **argv)
{
    uint64_t address;
    uint32_t value;

    if (argc != 3 || parse_u64(argv[1], &address) || parse_u32(argv[2], &value)) {
        return CLI_USAGE;
    }
    if (reg_write32(address, value)) {
        app_log_errno("write");
        return CLI_ERROR;
    }
    return CLI_OK;
}

const cli_command register_read_command = {
    .name = "read",
    .handler = read_handler,
    .usage = "read <address>",
    .description = "Read a 32-bit register",
};

const cli_command register_write_command = {
    .name = "write",
    .handler = write_handler,
    .usage = "write <address> <value>",
    .description = "Write a 32-bit register",
};

static const cli_command *const commands[] = {
    &register_read_command,
    &register_write_command,
};

static const cli_menu register_menu = {
    .prompt = "reg",
    .commands = commands,
    .count = CLI_COUNT(commands),
};

const cli_command register_menu_command = {
    .name = "reg",
    .usage = "reg [command ...]",
    .description = "Register commands (or enter submenu)",
    .submenu = &register_menu,
};
