CC ?= x86_64-elf-gcc
AR ?= ar
BUILD ?= build
INCLUDES = -I include
CFLAGS ?= -ffreestanding -O2 -Wall -Wextra -std=c99 -m64 -nostdlib -fno-stack-protector -fno-pie -fno-pic -mno-red-zone

LIBC_SRCS = libc/syscalls.c libc/string.c libc/stdlib.c libc/stdio.c
LIBC_OBJS = $(patsubst %.c,$(BUILD)/%.o,$(LIBC_SRCS))

all: $(BUILD)/libc.a $(BUILD)/crt0.o

$(BUILD):
	mkdir -p $(BUILD)/libc $(BUILD)/crt

$(BUILD)/%.o: %.c | $(BUILD)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD)/crt0.o: crt/crt0.S | $(BUILD)
	$(CC) -m64 -ffreestanding -nostdlib -c $< -o $@

$(BUILD)/libc.a: $(LIBC_OBJS)
	$(AR) rcs $@ $^

clean:
	rm -rf $(BUILD)

.PHONY: all clean
