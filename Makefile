# Makefile to make and run with QEMU. //ember2819
CC      = clang
CPP     = clang++
AS      = nasm
LD      = ld
OBJCOPY = objcopy

include_folder = include
CC_FLAGS = -target x86_64-elf -march=x86-64 -m64 -MMD -MP \
           -ffreestanding -nostdlib -fno-builtin -fno-stack-protector \
           -mno-red-zone -mcmodel=kernel \
           -mno-sse -mno-sse2 -mno-avx \
           -g -c $(addprefix -I,$(include_folder)) -DDEBUG
LD_FLAGS = -m elf_x86_64

SOURCES := $(shell find ./kernel -name "*.c" -o -name "*.cpp" -o -name "*.s")
OBJECTS := $(patsubst ./kernel/%.c,./build/%.o,   $(SOURCES))
OBJECTS := $(patsubst ./kernel/%.cpp,./build/%_cpp.o, $(OBJECTS))
OBJECTS := $(patsubst ./kernel/%.s,./build/%_s.o, $(OBJECTS))
DEPS    := $(OBJECTS:.o=.d)

all: grub-iso

build/%.o: kernel/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CC_FLAGS) $< -o $@
build/%_cpp.o: kernel/%.cpp
	@mkdir -p $(dir $@)
	$(CPP) $(CC_FLAGS) $< -o $@
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

grub-iso: kernel.elf
	@mkdir -p $(ISODIR)/boot/grub
	cp kernel.elf         $(ISODIR)/boot/kernel.elf
	cp boot/grub/grub.cfg $(ISODIR)/boot/grub/grub.cfg
	grub-mkrescue -o gecko.iso $(ISODIR) --locale-directory=/usr/share/locale
	@echo "gecko.iso built. Boot with:  make run-grub"

run-grub: gecko.iso
	qemu-system-x86_64 -cdrom gecko.iso -boot order=d \
	  -netdev user,id=net0 \
	  -device e1000,netdev=net0

fat32.img:
	dd if=/dev/zero of=fat32.img bs=1M count=64
	mkfs.fat -F 32 -n "GECKOOS" fat32.img
	@echo "fat32.img created."

Elffile:
	$(MAKE) -C Assets/Elf\ for\ testing
	mv Assets/Elf\ for\ testing/Elf .

run-fat32: gecko.iso fat32.img # I dont want to make a new .img
	qemu-system-x86_64 \
	  -cdrom gecko.iso -m 1G \
	  -drive format=raw,file=fat32.img \
	  -boot order=d \
	  -netdev user,id=net0 \
	  -device e1000,netdev=net0 \
	  -monitor stdio

run-uhci: gecko.iso fat32.img
	qemu-system-x86_64 \
	  -cdrom gecko.iso -m 512M \
	  -drive format=raw,file=fat32.img \
	  -boot order=d \
	  -netdev user,id=net0 \
	  -device e1000,netdev=net0 \
	  -monitor stdio \
	  -device piix3-usb-uhci,id=uhci \
	  -device usb-mouse,bus=uhci.0,id=mouse \
	  -device usb-tablet,bus=uhci.0,id=keyboard

run-ohci: gecko.iso fat32.img
	qemu-system-x86_64 \
	  -cdrom gecko.iso -m 512M \
	  -drive format=raw,file=fat32.img \
	  -boot order=d \
	  -netdev user,id=net0 \
	  -device e1000,netdev=net0 \
	  -monitor stdio \
	  -device pci-ohci,id=ohci -device usb-mouse,bus=ohci.0

VBOXCreateMachine:
	VBoxManage createvm --name "GECKOOS" --ostype "Other_64" --register
	VBoxManage storagectl "GECKOOS" --name "IDE Controller" --add ide
run-virtualbox: # You have to create the machine first, then you can run geckos in virtualbox
	VBoxManage storageattach "GECKOOS" --storagectl "IDE Controller" --port 0 --device 0 --type dvddrive --medium gecko.iso
	VBoxManage startvm "GECKOOS"

clean:
	rm -f $(OBJECTS) $(DEPS)
	rm -f kernel.elf gecko.iso
	rm -rf $(ISODIR)

.PHONY: all grub-iso run-grub fat32.img run-fat32 clean
