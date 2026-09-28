#include "register_cmd.h"
#include "register_io.h"
#include "parse.h"
#include <inttypes.h>
#include <stdio.h>
int read_handler(int argc, char **argv)
{
    uint64_t address;
    uint32_t value;
    if (argc != 2 || parse_u64(argv[1], &address)) return CLI_USAGE;
    if (reg_read32(address, &value)) { perror("read"); return CLI_ERROR; }
    printf("0x%08" PRIx32 "\n", value);
    return CLI_OK;
}
int write_handler(int argc, char **argv)
{
    uint64_t address;
    uint32_t value;
    if (argc != 3 || parse_u64(argv[1], &address) || parse_u32(argv[2], &value)) return CLI_USAGE;
    if (reg_write32(address, value)) { perror("write"); return CLI_ERROR; }
    return CLI_OK;
}
static int field_handler(int argc, char **argv)
{
    uint64_t address;
    uint32_t mask, shift, value;
    if ((argc != 4 && argc != 5) || parse_u64(argv[1], &address) ||
        parse_u32(argv[2], &mask) || parse_u32(argv[3], &shift) ||
        (argc == 5 && parse_u32(argv[4], &value))) return CLI_USAGE;
    int rc = argc == 5 ? reg_write_field32(address, mask, shift, value)
                       : reg_read_field32(address, mask, shift, &value);
    if (rc) { perror("field"); return CLI_ERROR; }
    if (argc == 4) printf("0x%08" PRIx32 "\n", value);
    return CLI_OK;
}
static const cli_command commands[] = {
    {"read", read_handler, "read <address>", "Read a 32-bit register", NULL},
    {"write", write_handler, "write <address> <value>", "Write a 32-bit register", NULL},
    {"field", field_handler, "field <addr> <mask> <shift> [value]", "Read/write a bit field", NULL},
};
const cli_menu register_menu = {"reg", commands, CLI_COUNT(commands)};
