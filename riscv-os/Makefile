# RISC-V OS Makefile

# Cross-compiler toolchain
CROSS_COMPILE ?= riscv64-unknown-elf-
CC = $(CROSS_COMPILE)gcc
AS = $(CROSS_COMPILE)as
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump

# Compiler flags
CFLAGS = -march=rv64imac_zicsr -mabi=lp64 -mcmodel=medany -nostdlib -ffreestanding -O2 -Wall -Wextra
ASFLAGS = -march=rv64imac_zicsr -mabi=lp64
LDFLAGS = -T linker.ld -nostdlib

# Source files
ASM_SOURCES = boot.S
C_SOURCES = kernel.c uart.c memory.c process.c scheduler.c syscall.c filesystem.c string.c

# Object files
ASM_OBJECTS = $(ASM_SOURCES:.S=.o)
C_OBJECTS = $(C_SOURCES:.c=.o)
OBJECTS = $(ASM_OBJECTS) $(C_OBJECTS)

# Output
TARGET = kernel.elf
BINARY = kernel.bin

# Build rules
all: $(TARGET) $(BINARY)

$(TARGET): $(OBJECTS) linker.ld
	$(LD) $(LDFLAGS) $(OBJECTS) -o $@
	@echo "Kernel ELF created: $(TARGET)"
	$(OBJDUMP) -d $(TARGET) > kernel.dis
	@echo "Disassembly created: kernel.dis"

$(BINARY): $(TARGET)
	$(OBJCOPY) -O binary $(TARGET) $@
	@echo "Kernel binary created: $(BINARY)"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(AS) $(ASFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET) $(BINARY) kernel.dis

run: all
	qemu-system-riscv64 -machine virt -bios none -kernel $(TARGET) -m 128M -nographic

debug: all
	qemu-system-riscv64 -machine virt -bios none -kernel $(TARGET) -m 128M -nographic -s -S

# Show build info
info:
	@echo "Cross Compiler: $(CC)"
	@echo "CFLAGS: $(CFLAGS)"
	@echo "Sources: $(C_SOURCES) $(ASM_SOURCES)"
	@echo "Objects: $(OBJECTS)"

.PHONY: all clean run debug info