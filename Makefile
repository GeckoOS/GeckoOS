# By Ember2819
# C compiler
CC = clang
CXX = clang++
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
CC_FLAGS = -target i386-linux-gnu -march=i686 -m32 -ffreestanding -nostdlib -fno-builtin -fno-stack-protector -g -c $(addprefix -I,$(include_folder)) -fno-asynchronous-unwind-tables \
	    	-Wshadow -Wno-pointer-sign -Oz -fno-ident -fno-unwind-tables -fno-pie -falign-functions=1 -falign-loops=1 -fno-plt
CXX_FLAGS = -march=i686 -m32 -ffreestanding -O2 -Wall -Wextra -fno-exceptions -fno-rtti -c $(addprefix -I,$(include_folder))
LD_FLAGS = -m elf_i386

# nfoxers
SOURCES := $(shell find ./ -name "*.c" -o -name "*.s" -o -name "*.cpp")
# change all the .c's
OBJECTS := $(patsubst ./kernel/%.c,./build/%.o, $(SOURCES))
# then all the .s's, name change to avoid conflict with .c sources w the same name
OBJECTS := $(patsubst %.s,./build/%_s.o, $(OBJECTS))
OBJECTS := $(patsubst ./kernel/%.cpp,./build/%.o, $(OBJECTS))

all: boot.iso run

# If no clang detected, use gcc
build/%.o: kernel/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CC_FLAGS) $< -o $@
build/%.o: kernel/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXX_FLAGS) $< -o $@
build/%_s.o: ./%.s
	@mkdir -p $(dir $@)
	$(AS) -felf32 $< -o $@

# Link all kernel objects
kernel.elf: $(OBJECTS)
	$(LD) $(LD_FLAGS) -T linker.ld $^ -o kernel.elf
kernel.bin: kernel.elf
	$(OBJCOPY) $(OBJCOPY_ARGS) kernel.elf kernel.bin
boot.iso: kernel.elf
	mv kernel.elf iso/boot
	grub-mkrescue -o boot.iso iso

# Launch the image in QEMU
run: boot.iso
	qemu-system-i386 -cdrom boot.iso -serial stdio

fat16.img:
	dd if=/dev/zero of=fat16.img bs=1M count=16
	mkfs.fat -F 16 -n "GECKOOS" fat16.img
	@echo "fat16.img created. Copy files onto it with:"
	@echo "  mcopy -i fat16.img yourfile.txt ::yourfile.txt"

run-fat16: boot.iso fat16.img
	qemu-system-i386 -s \
	  -drive format=raw,file=boot.iso \
	  -drive format=raw,file=fat16.img \

clean:
	rm -f $(OBJECTS) boot.iso
	rm -f fat16.img
.PHONY: all run run-fat16 clean
