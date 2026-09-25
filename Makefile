CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic

.PHONY: all clean real
all: debug_cli

debug_cli: main.c cli.c register_io.c cli.h register_io.h
	$(CC) $(CFLAGS) main.c cli.c register_io.c -o $@

real:
	$(CC) $(CFLAGS) -DREAL_MMIO main.c cli.c register_io.c -o debug_cli_real

clean:
	rm -f debug_cli debug_cli_real mock_phys_mem.bin
