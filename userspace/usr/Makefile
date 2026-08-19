CC ?= x86_64-elf-gcc
AR ?= ar
BUILD ?= build
INCLUDES = -I ../include
CFLAGS ?= -ffreestanding -O2 -Wall -Wextra -std=c99 -m64 -nostdlib -fno-stack-protector -fno-pie -fno-pic -mno-red-zone

LIBC_SRCS = core/lib/syscalls.c core/lib/string.c core/lib/stdlib.c core/lib/stdio.c
LIBC_OBJS = $(patsubst %.c,$(BUILD)/%.o,$(LIBC_SRCS))
TEST_OBJ = $(BUILD)/tests/hello.o
POSIX_TEST_OBJ = $(BUILD)/tests/posix.o

all: $(BUILD)/libc.a $(BUILD)/crt0.o

test-hello: $(BUILD)/hello.elf

test-posix: $(BUILD)/posix.elf

$(BUILD)/hello.elf: $(TEST_OBJ) $(BUILD)/crt0.o $(BUILD)/libc.a tests/hello.ld
	$(CC) $(CFLAGS) -nostdlib -static -no-pie -Wl,-T,tests/hello.ld -o $@ $(BUILD)/crt0.o $(TEST_OBJ) $(BUILD)/libc.a

$(BUILD)/posix.elf: $(POSIX_TEST_OBJ) $(BUILD)/crt0.o $(BUILD)/libc.a tests/hello.ld
	$(CC) $(CFLAGS) -nostdlib -static -no-pie -Wl,-T,tests/hello.ld -o $@ $(BUILD)/crt0.o $(POSIX_TEST_OBJ) $(BUILD)/libc.a

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

.PHONY: all test-hello test-posix clean
