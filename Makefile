# Makefile to make and run with QEMU. //ember2819
CC      = clang
AS      = nasm
LD      = ld.lld # *should* support all arch on any arch (x86 on arm, x86_64 on arm, etc)
OBJCOPY = objcopy

include_folder = include
CC_FLAGS = -target x86_64-elf -march=x86-64 -m64 -MMD -MP \
           -ffreestanding -nostdlib -fno-builtin -fno-stack-protector \
           -mno-red-zone -mcmodel=kernel \
           -mno-sse -mno-sse2 -mno-avx \
           -g -c $(addprefix -I,$(include_folder))
LD_FLAGS = -m elf_x86_64

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
	$(AS) -felf64 $< -o $@

kernel.elf: $(OBJECTS)
	$(LD) $(LD_FLAGS) -T linker.ld $^ -o kernel.elf

-include $(DEPS)

ISODIR = isodir

grub-modules/i386-pc/modinfo.sh:
	@echo "Downloading i386-pc GRUB modules..."
	@mkdir -p grub-modules
	@curl -sL "https://archive.archlinux.org/packages/g/grub/grub-2%3A2.14-1-x86_64.pkg.tar.zst" \
		-o /tmp/grub-x86_64.pkg.tar.zst
	@tar --zstd -xf /tmp/grub-x86_64.pkg.tar.zst -C /tmp 2>/dev/null || true
	@cp -r /tmp/usr/lib/grub/i386-pc grub-modules/
	@rm -rf /tmp/usr /tmp/grub-x86_64.pkg.tar.zst

grub-iso: kernel.elf grub-modules/i386-pc/modinfo.sh
	@mkdir -p $(ISODIR)/boot/grub
	cp kernel.elf         $(ISODIR)/boot/kernel.elf
	cp boot/grub/grub.cfg $(ISODIR)/boot/grub/grub.cfg
	grub-mkrescue --directory=grub-modules/i386-pc -o gecko.iso $(ISODIR) --locale-directory=/usr/share/locale
	@echo "gecko.iso built. Boot with:  make run-grub"

run-grub: gecko.iso
	qemu-system-x86_64 -cdrom gecko.iso -boot order=d

fat32.img:
	dd if=/dev/zero of=fat32.img bs=1M count=64
	mkfs.fat -F 32 -n "GECKOOS" fat32.img
	@echo "fat32.img created."

run-fat32: gecko.iso fat32.img
	qemu-system-x86_64 \
	  -cdrom gecko.iso \
	  -drive format=raw,file=fat32.img \
	  -boot order=d

clean:
	rm -f $(OBJECTS) $(DEPS)
	rm -f kernel.elf gecko.iso fat32.img
	rm -rf $(ISODIR)

.PHONY: all grub-iso run-grub fat32.img run-fat32 clean
