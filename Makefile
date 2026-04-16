# By Ember2819
# C compiler
CC = clang
# Assembler (for boot.s)
AS = nasm
# Linker
LD = ld
# Truncate (to make the kernel.bin divisible by 512)
TRUNCATE = truncate
TRUNC_AMNT = 131072
# Objcopy (to translate elf to bin)
OBJCOPY = objcopy
OBJCOPY_ARGS = -O binary
include_folder = kernel/include kernel/drivers kernel
CC_FLAGS = -target i386-elf -march=i686 -m32 -ffreestanding -nostdlib -fno-builtin -fno-stack-protector -g -c $(addprefix -I,$(include_folder))
AS_FLAGS = -f bin
LD_FLAGS = -m elf_i386

# nfoxers
SOURCES := $(shell find ./kernel -name "*.c" -o -name "*.s")
# change all the .c's
OBJECTS := $(patsubst ./kernel/%.c,./build/%.o, $(SOURCES))
# then all the .s's, name change to avoid conflict with .c sources w the same name
OBJECTS := $(patsubst ./kernel/%.s,./build/%_s.o, $(OBJECTS))

# Builds the final disk image
all: os.img

# If no clang detected, use gcc
build/%.o: kernel/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CC_FLAGS) $< -o $@

build/%_s.o: kernel/%.s
	@mkdir -p $(dir $@)
	$(AS) -felf32 $< -o $@

# Assemble the bootloader
bootloader/boot.bin: bootloader/boot.s bootloader/bootc.bin
	$(AS) $(AS_FLAGS) $< -o $@
	cat bootloader/bootc.bin >> $@
	$(TRUNCATE) -s 8192 bootloader/boot.bin
bootloader/bootc.o: bootloader/boot.c
	$(CC) $(CC_FLAGS) $< -o $@

bootloader/bootc.elf: bootloader/bootc.o
	$(LD) $(LD_FLAGS) -T bootloader/linker.ld $< -o $@ 
bootloader/bootc.bin: bootloader/bootc.elf
	$(OBJCOPY) $(OBJCOPY_ARGS) $< $@

# Link all kernel objects 
kernel.elf: $(OBJECTS)
	$(LD) $(LD_FLAGS) -T linker.ld $^ -o kernel.elf
kernel.bin: kernel.elf
	$(OBJCOPY) $(OBJCOPY_ARGS) kernel.elf kernel.bin
	$(TRUNCATE) -s $(TRUNC_AMNT) kernel.bin
os.img: bootloader/boot.bin kernel.bin
	cat bootloader/boot.bin kernel.bin > os.img

# Launch the image in QEMU
run: os.img
	qemu-system-i386 -s -drive format=raw,file=os.img -usb -monitor stdio

fat16.img:
	dd if=/dev/zero of=fat16.img bs=1M count=16
	mkfs.fat -F 16 -n "GECKOOS" fat16.img
	@echo "fat16.img created. Copy files onto it with:"
	@echo "  mcopy -i fat16.img yourfile.txt ::yourfile.txt"

run-fat16: os.img fat16.img
	qemu-system-i386 -s \
	  -drive format=raw,file=os.img \
	  -drive format=raw,file=fat16.img \
	  -usb
		
clean:
	rm -f $(OBJECTS) kernel.elf kernel.bin bootloader/bootc.elf bootloader/bootc.bin bootloader/boot.bin
	rm -f fat16.img
.PHONY: all run run-fat16 clean
