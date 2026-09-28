CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic
MODE ?= mock
ifeq ($(filter $(MODE),mock real),)
$(error MODE must be mock or real)
endif
SOURCES := src/main.c $(wildcard src/core/*.c) $(wildcard src/commands/*.c)
BUILD_DIR := build/$(MODE)
OBJECTS := $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SOURCES))
TARGET := $(BUILD_DIR)/debug_cli
MODE_FLAGS := $(if $(filter real,$(MODE)),-DREAL_MMIO,)
.PHONY: all real clean test
all: $(TARGET)
real:
	$(MAKE) MODE=real all
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@
$(BUILD_DIR)/%.o: src/%.c Makefile
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -Iinclude $(MODE_FLAGS) $(CFLAGS) -MMD -MP -c $< -o $@
-include $(OBJECTS:.o=.d)
test:
	$(MAKE) MODE=mock all
	python3 tests/test_cli.py build/mock/debug_cli
clean:
	rm -rf build
