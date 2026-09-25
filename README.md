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
`REG_BASE` and `REG_SIZE` in `register_io.c` for each target, or pass compile
definitions in `CFLAGS`. Only addresses in this window are accepted.

`make real` builds `debug_cli_real` with `/dev/mem` access. **Check the board's
register map and access permissions before using it on hardware.** The real
build uses volatile 32-bit loads and stores and assumes the target mapping
allows those accesses and has the expected byte order.

To start a project, edit the command array and handlers in `main.c`. The
handlers parse project-specific arguments and call `reg_read32` or
`reg_write32`; `cli.c` owns the input loop, help, and dispatch. Inputs are
space-separated tokens (no quoting). `argv` points into a temporary input
line and is valid only during the handler call. `exit` and `help` are reserved.
