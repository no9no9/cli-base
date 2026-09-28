#ifndef REGISTER_CMD_H
#define REGISTER_CMD_H
#include "cli.h"
extern const cli_menu register_menu;
int read_handler(int argc, char **argv);
int write_handler(int argc, char **argv);
#endif
