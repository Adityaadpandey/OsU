AS = nasm
QEMU = qemu-system-i386

CROSS ?= i686-elf-
KCC ?= $(CROSS)gcc
KLD ?= $(CROSS)ld
KOBJCOPY ?= $(CROSS)objcopy

HAVE_CROSS := $(shell command -v $(KCC) >/dev/null 2>&1 && command -v $(KLD) >/dev/null 2>&1 && command -v $(KOBJCOPY) >/dev/null 2>&1 && echo 1 || echo 0)

CFLAGS = -m32 -ffreestanding -fno-stack-protector -fno-builtin -fno-pic -nostdlib -Wall -Wextra -O2 -std=c11 -Ikernel
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

BUILD_DIR = build
BOOT_DIR = boot
KERNEL_DIR = kernel

STAGE2_SECTORS = 16
KERNEL_SECTORS = 256
IMG_SECTORS = 32768

KERNEL_ASM = $(KERNEL_DIR)/entry.asm $(KERNEL_DIR)/isr.asm $(KERNEL_DIR)/context.asm
KERNEL_C = $(wildcard $(KERNEL_DIR)/*.c)

KERNEL_ASM_OBJ = $(patsubst $(KERNEL_DIR)/%.asm,$(BUILD_DIR)/%.o,$(KERNEL_ASM))
KERNEL_C_OBJ = $(patsubst $(KERNEL_DIR)/%.c,$(BUILD_DIR)/%.o,$(KERNEL_C))

.PHONY: all run clean check-toolchain

all: check-toolchain $(BUILD_DIR)/os.img

check-toolchain:
	@if [ "$(HAVE_CROSS)" != "1" ]; then \
		echo "Missing cross toolchain. Install i686-elf-gcc/i686-elf-ld/i686-elf-objcopy or run make with CROSS=<prefix>"; \
		exit 1; \
	fi

$(BUILD_DIR)/os.img: $(BUILD_DIR)/mbr.bin $(BUILD_DIR)/stage2.bin $(BUILD_DIR)/kernel.bin
	@mkdir -p $(BUILD_DIR)
	dd if=/dev/zero of=$@ bs=512 count=$(IMG_SECTORS) status=none
	dd if=$(BUILD_DIR)/mbr.bin of=$@ bs=512 count=1 conv=notrunc status=none
	dd if=$(BUILD_DIR)/stage2.bin of=$@ bs=512 seek=1 count=$(STAGE2_SECTORS) conv=notrunc status=none
	dd if=$(BUILD_DIR)/kernel.bin of=$@ bs=512 seek=$$(expr 1 + $(STAGE2_SECTORS)) conv=notrunc status=none
	@echo "Built $@"

$(BUILD_DIR)/mbr.bin: $(BOOT_DIR)/mbr.asm
	@mkdir -p $(BUILD_DIR)
	$(AS) -f bin $< -o $@

$(BUILD_DIR)/stage2.bin: $(BOOT_DIR)/stage2.asm
	@mkdir -p $(BUILD_DIR)
	$(AS) -f bin $< -o $@

$(BUILD_DIR)/kernel.elf: $(KERNEL_ASM_OBJ) $(KERNEL_C_OBJ) linker.ld
	$(KLD) $(LDFLAGS) -o $@ $(KERNEL_ASM_OBJ) $(KERNEL_C_OBJ)

$(BUILD_DIR)/kernel.bin: $(BUILD_DIR)/kernel.elf
	$(KOBJCOPY) -O binary $< $@
	@size=$$(wc -c < $@); \
	if [ $$size -gt $$((512 * $(KERNEL_SECTORS))) ]; then \
		echo "kernel.bin too large: $$size bytes (limit $$((512 * $(KERNEL_SECTORS))))"; \
		exit 1; \
	fi

$(BUILD_DIR)/%.o: $(KERNEL_DIR)/%.asm
	@mkdir -p $(BUILD_DIR)
	$(AS) -f elf32 $< -o $@

$(BUILD_DIR)/%.o: $(KERNEL_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(KCC) $(CFLAGS) -c $< -o $@

run: check-toolchain $(BUILD_DIR)/os.img
	$(QEMU) -drive format=raw,file=$(BUILD_DIR)/os.img,if=ide,index=0,media=disk -boot c

clean:
	rm -rf $(BUILD_DIR)
