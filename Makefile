# Makefile to make and run with QEMU. //ember2819
CC      = clang
AS      = nasm
# For MacOS compatability, it must use ld.lld, not just ld.
LD := $(shell command -v ld.lld 2>/dev/null || command -v ld 2>/dev/null)
OBJCOPY := $(shell command -v llvm-objcopy 2>/dev/null || command -v objcopy 2>/dev/null) # Similar methodology for objcopy
GRUB_MKRESCUE := $(shell command -v x86_64-elf-grub-mkrescue 2>/dev/null || command -v grub-mkrescue 2>/dev/null)

ifeq ($(LD),)
$(error ld.lld not found. Install LLVM/LLD and put ld.lld in PATH.)
endif

ifeq ($(OBJCOPY),)
$(error llvm-objcopy/objcopy not found.)
endif

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
	$(GRUB_MKRESCUE) --directory=grub-modules/i386-pc -o gecko.iso $(ISODIR) --locale-directory=/usr/share/locale
	@echo "gecko.iso built. Boot with:  make run-grub"

run-grub: gecko.iso
	qemu-system-x86_64 -cdrom gecko.iso -boot order=d \
	  -netdev user,id=net0 \
	  -device e1000,netdev=net0

run-grub-drive: gecko.iso fat32.img
	qemu-system-x86_64 -cdrom gecko.iso -boot order=d \
	  -netdev user,id=net0 \
	  -device e1000,netdev=net0 \
	  -drive file=fat32.img,format=raw,if=ide,index=1

fat32.img:
	dd if=/dev/zero of=fat32.img bs=1M count=64
	mkfs.fat -F 32 -n "GECKOOS" fat32.img
	@echo "fat32.img created."

Elffile:
	$(MAKE) -C Assets/Elf\ for\ testing
	mv Assets/Elf\ for\ testing/Elf .

run-fat32: gecko.iso fat32.img # I dont want to make a new .img
	qemu-system-x86_64 \
	  -cdrom gecko.iso \
	  -drive format=raw,file=fat32.img \
	  -boot order=d \
	  -netdev user,id=net0 \
	  -device e1000,netdev=net0 \
	  -monitor stdio # -d int,pcall
clean:
	rm -f $(OBJECTS) $(DEPS)
	rm -f kernel.elf gecko.iso
	rm -rf $(ISODIR)

.PHONY: all grub-iso run-grub fat32.img run-fat32 clean
