# cli-base

Embedded Linux user-space command-line tool template written in C11.

```sh
make
./debug_cli
```

Example session:

```text
cli> help
cli> write 0x40000000 0x12345678
cli> read 0x40000000
0x12345678
cli> exit
```

The default build maps the address window starting at `REG_BASE` into a local
`mock_phys_mem.bin` file. The file persists between runs. The default aperture
is `0x40000000` through `0x40000fff`; 32-bit accesses must be aligned. Set
`REG_BASE` and `REG_SIZE` in `src/core/register_io.c` for each target, or pass compile
definitions in `CFLAGS`. Only addresses in this window are accepted.

`make real` builds `debug_cli_real` with `/dev/mem` access. **Check the board's
register map and access permissions before using it on hardware.** The real
build uses volatile 32-bit loads and stores and assumes the target mapping
allows those accesses and has the expected byte order.

## Source layout

| Path | Responsibility |
| --- | --- |
| `include/` | Shared CLI and register I/O interfaces |
| `src/main.c` | Command table and application startup |
| `src/core/` | Reusable input, dispatch, and register I/O implementations |
| `src/commands/` | Project-specific command handlers and their headers |

## Adding commands for a project

1. Add a handler implementation to `src/commands/<feature>_cmd.c` and its
   declaration to `src/commands/<feature>_cmd.h`.
2. Include that header from `src/main.c` and add an entry to its command table.
3. Run `make`. C files directly under `src/commands/` are included automatically.

Use `register_cmd.c` as an example: each handler receives `argc` and `argv`,
parses arguments, and calls the shared I/O functions. Return 0 on success and
nonzero on error. A handler should print the error detail; the CLI then prints
its usage. Keep project-specific behavior in the handlers and reusable
operations in `src/core/` with interfaces in `include/`.

Inputs are space-separated tokens (no quoting). `argv[0]` is the command name.
`argv` points into a temporary input line and is valid only during the handler
call. `exit` and `help` are reserved. Command names must be unique.
