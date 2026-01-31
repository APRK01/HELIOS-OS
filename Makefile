# Helios OS Makefile for AArch64

# Toolchain
CC = aarch64-elf-gcc
LD = aarch64-elf-ld
OBJCOPY = aarch64-elf-objcopy

# Paths
BUILD_DIR = build
ISO_ROOT = $(BUILD_DIR)/iso_root
LIMINE_DIR = limine

# Target
KERNEL_ELF = $(BUILD_DIR)/kernel.elf
ISO = helios.iso

# Compiler flags
CFLAGS = \
	-ffreestanding \
	-fno-stack-protector \
	-fno-stack-check \
	-fno-PIC \
	-mno-outline-atomics \
	-mcmodel=large \
	-nostdlib \
	-O2 \
	-Wall \
	-Wextra \
	-I. \
	-I$(LIMINE_DIR)

# Linker flags
LDFLAGS = \
	-T linker.ld \
	-nostdlib \
	-m aarch64elf

# Source files
SRCS_C = kernel.c uart.c console.c shell.c virtio.c keyboard.c pmm.c heap.c vfs.c string.c tmpfs.c process.c gic.c timer.c irq.c editor.c donut.c
SRCS_S = entry.S vectors.S

OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS_C)) $(patsubst %.S,$(BUILD_DIR)/%.o,$(SRCS_S))

# Default target
.PHONY: all
all: $(ISO)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compile kernel source
# Compile kernel source
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile assembly source
$(BUILD_DIR)/%.o: %.S | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Link kernel
$(KERNEL_ELF): $(OBJS) linker.ld
	$(LD) $(LDFLAGS) $(OBJS) -o $@

# Create ISO
$(ISO): $(KERNEL_ELF) limine.conf $(LIMINE_DIR)/BOOTAA64.EFI
	rm -rf $(ISO_ROOT)
	mkdir -p $(ISO_ROOT)/boot
	mkdir -p $(ISO_ROOT)/EFI/BOOT
	
	# Copy files to ISO root (for Non-EFI systems or secondary access)
	cp $(KERNEL_ELF) $(ISO_ROOT)/boot/kernel.elf
	cp limine.conf $(ISO_ROOT)/boot/limine.conf
	cp $(LIMINE_DIR)/BOOTAA64.EFI $(ISO_ROOT)/EFI/BOOT/BOOTAA64.EFI
	cp limine.conf $(ISO_ROOT)/EFI/BOOT/limine.conf
	
	# Create FAT image for UEFI
	dd if=/dev/zero of=$(BUILD_DIR)/fat.img bs=1M count=64
	mformat -i $(BUILD_DIR)/fat.img -F ::
	mmd -i $(BUILD_DIR)/fat.img ::/EFI
	mmd -i $(BUILD_DIR)/fat.img ::/EFI/BOOT
	mmd -i $(BUILD_DIR)/fat.img ::/boot
	
	# Copy files into FAT image
	mcopy -i $(BUILD_DIR)/fat.img $(LIMINE_DIR)/BOOTAA64.EFI ::/EFI/BOOT/BOOTAA64.EFI
	mcopy -i $(BUILD_DIR)/fat.img limine.conf ::/EFI/BOOT/limine.conf
	mcopy -i $(BUILD_DIR)/fat.img $(KERNEL_ELF) ::/boot/kernel.elf
	mcopy -i $(BUILD_DIR)/fat.img limine.conf ::/boot/limine.conf
	mcopy -i $(BUILD_DIR)/fat.img limine.conf ::/limine.conf
	
	# Create the ISO
	xorriso -as mkisofs \
		-R -J \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		--efi-boot fat.img \
		-o $(ISO) \
		-graft-points \
		/fat.img=$(BUILD_DIR)/fat.img \
		/boot/kernel.elf=$(KERNEL_ELF) \
		/boot/limine.conf=limine.conf \
		/EFI/BOOT/BOOTAA64.EFI=$(LIMINE_DIR)/BOOTAA64.EFI \
		/EFI/BOOT/limine.conf=limine.conf

# Clean build artifacts
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	rm -f $(ISO)

# Run in QEMU
.PHONY: run
run: $(ISO)
	qemu-system-aarch64 \
		-M virt \
		-cpu cortex-a72 \
		-m 512M \
		-bios /opt/homebrew/share/qemu/edk2-aarch64-code.fd \
		-drive format=raw,file=$(ISO) \
		-device ramfb \
		-device qemu-xhci \
		-device virtio-keyboard-device \
		-serial mon:stdio

# Help
.PHONY: help
help:
	@echo "Helios OS Build System (AArch64)"
