CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic
CPPFLAGS += -Iinclude
SOURCES := src/main.c $(wildcard src/core/*.c) $(wildcard src/commands/*.c)
HEADERS := $(wildcard include/*.h) $(wildcard src/commands/*.h)

.PHONY: all clean real
all: debug_cli

debug_cli: $(SOURCES) $(HEADERS) Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) $(LDFLAGS) $(LDLIBS) -o $@

real:
	$(CC) $(CPPFLAGS) $(CFLAGS) -DREAL_MMIO $(SOURCES) $(LDFLAGS) $(LDLIBS) -o debug_cli_real

clean:
	rm -f debug_cli debug_cli_real mock_phys_mem.bin
