# Makefile to make and run with QEMU. //ember2819
CC      = clang
AS      = nasm
LD      = ld
ifeq ($(shell scripts/checkarch), aarch64)
	CCLD = $(shell command -v i386-elf-ld 2> /dev/null) # is there a cross compiled ld in PATH //R1shy
	ifdef CCLD
		LD = i386-elf-ld
	else
		$(error you are on an arm system and do not have a cross compiled ld, please install it to link the kernel)
	endif
endif
OBJCOPY = objcopy

include_folder = include
CC_FLAGS = -target i386-elf -march=i686 -m32 -MMD -MP \
           -ffreestanding -nostdlib -fno-builtin -fno-stack-protector \
           -g -c $(addprefix -I,$(include_folder))
LD_FLAGS = -m elf_i386

SOURCES := $(shell find ./kernel -name "*.c" -o -name "*.s")
OBJECTS := $(patsubst ./kernel/%.c,./build/%.o,   $(SOURCES))
OBJECTS := $(patsubst ./kernel/%.s,./build/%_s.o, $(OBJECTS))
DEPS    := $(OBJECTS:.o=.d)

all: grub-iso

build/%.o: kernel/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CC_FLAGS) $< -o $@

build/%_s.o: kernel/%.s
	@mkdir -p $(dir $@)
	$(AS) -felf32 $< -o $@

kernel.elf: $(OBJECTS)
	$(LD) $(LD_FLAGS) -T linker.ld $^ -o kernel.elf

-include $(DEPS)

ISODIR = isodir

grub-modules/i386-pc/modinfo.sh:
	@echo "Downloading i386-pc GRUB modules..."
	@mkdir -p grub-modules
	@curl -sL "https://archive.archlinux.org/packages/g/grub/grub-2%3A2.14-1-x86_64.pkg.tar.zst" -o /tmp/grub-x86_64.pkg.tar.zst
	@cd /tmp && tar --zstd -xf grub-x86_64.pkg.tar.zst -C /tmp/grub-extract 2>/dev/null || true
	@mkdir -p grub-modules
	@cp -r /tmp/grub-extract/usr/lib/grub/i386-pc grub-modules/
	@rm -rf /tmp/grub-extract /tmp/grub-x86_64.pkg.tar.zst

grub-iso: kernel.elf grub-modules/i386-pc/modinfo.sh
	@mkdir -p $(ISODIR)/boot/grub
	cp kernel.elf         $(ISODIR)/boot/kernel.elf
	cp boot/grub/grub.cfg $(ISODIR)/boot/grub/grub.cfg
	grub-mkrescue --directory=grub-modules/i386-pc -o gecko.iso $(ISODIR) --locale-directory=/usr/share/locale
	@echo ""
	@echo "  gecko.iso built. Boot with:  make run-grub"

run-grub: gecko.iso
	qemu-system-i386 -cdrom gecko.iso -boot order=d

fat32.img:
	dd if=/dev/zero of=fat32.img bs=1M count=64
	mkfs.fat -F 32 -n "GECKOOS" fat32.img
	@echo "fat32.img created."

run-fat32: gecko.iso fat32.img
	qemu-system-i386 \
	  -cdrom gecko.iso \
	  -drive format=raw,file=fat32.img \
	  -boot order=d

clean:
	rm -f $(OBJECTS) $(DEPS)
	rm -f kernel.elf gecko.iso fat32.img
	rm -rf $(ISODIR)

.PHONY: all grub-iso run-grub fat32.img run-fat32 clean
