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
CC_FLAGS = -m32 -ffreestanding -nostdlib -fno-builtin -fno-stack-protector -g -c  $(addprefix -I,$(include_folder))
AS_FLAGS = -f bin
LD_FLAGS = -m elf_i386
KERNEL_OBJECTS = kernel/kernel.o kernel/mem/mem.o kernel/mem/physical_mem/physical_mem.o kernel/mem/virtual_mem/paging.o kernel/process/process.o

DRIVER_OBJECTS = kernel/drivers/vga.o kernel/drivers/keyboard.o kernel/drivers/tables/idt/idt_c.o kernel/drivers/tables/idt/idt_s.o \
	kernel/drivers/tables/isr/isr_c.o kernel/drivers/tables/isr/isr_s.o kernel/drivers/tables/irq/irq_c.o kernel/drivers/tables/irq/irq_s.o kernel/drivers/tables/timer/timer.o 

MISC_OBJECTS = kernel/colors.o kernel/terminal/terminal.o kernel/commands.o kernel/layouts/kb_layouts.o \
               kernel/gk/gk_lexer.o kernel/gk/gk_parser.o kernel/gk/gk_interp.o \
               kernel/editor/editor.o \
               kernel/users/users.o #ember2819 user/permission system
# Ember2819
FS_OBJECTS = kernel/drivers/drives.o kernel/drivers/ata.o \
	kernel/fs/fs.o kernel/fs/fat16.o

# Builds the final disk image
all: os.img
	
# If no clang detected, use gcc
%.o: %.c
	$(CC) $(CC_FLAGS) $< -o $@
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

# IDT IRQ IRQ
kernel/drivers/tables/idt/idt_c.o: kernel/drivers/tables/idt/idt.c
	$(CC) $(CC_FLAGS) $< -o $@
kernel/drivers/tables/isr/isr_c.o: kernel/drivers/tables/isr/isr.c
	$(CC) $(CC_FLAGS) $< -o $@
kernel/drivers/tables/irq/irq_c.o: kernel/drivers/tables/irq/irq.c
	$(CC) $(CC_FLAGS) $< -o $@
kernel/drivers/tables/idt/idt_s.o: kernel/drivers/tables/idt/idt.s
	$(AS) -felf32 $< -o $@
kernel/drivers/tables/irq/irq_s.o: kernel/drivers/tables/irq/irq.s
	$(AS) -felf32 $< -o $@
kernel/drivers/tables/isr/isr_s.o: kernel/drivers/tables/isr/isr.s
	$(AS) -felf32 $< -o $@
kernel/drivers/tables/paging/paging_s.o:kernel/drivers/tables/paging/paging.s
	$(AS) -felf32 $< -o $@
# Link all kernel objects 
kernel.elf: $(KERNEL_OBJECTS) $(DRIVER_OBJECTS) $(MISC_OBJECTS) $(FS_OBJECTS)
	$(LD) $(LD_FLAGS) -T linker.ld $(KERNEL_OBJECTS) $(DRIVER_OBJECTS) $(MISC_OBJECTS) $(FS_OBJECTS) -o kernel.elf
kernel.bin: kernel.elf
	$(OBJCOPY) $(OBJCOPY_ARGS) kernel.elf kernel.bin
	$(TRUNCATE) -s $(TRUNC_AMNT) kernel.bin
os.img: bootloader/boot.bin kernel.bin
	cat bootloader/boot.bin kernel.bin > os.img
# Launch the image in QEMU

run: os.img
	qemu-system-i386 -s -drive format=raw,file=os.img -usb


fat16.img:
	dd if=/dev/zero of=fat16.img bs=1M count=16
	mkfs.fat -F 16 -n "GECKOOS" fat16.img
	@echo "fat16.img created. Copy files onto it with:"
	@echo "  mcopy -i fat16.img yourfile.txt ::yourfile.txt"

run-fat16: os.img fat16.img
	qemu-system-i386 -s \
	  -drive format=raw,file=os.img \
	  -drive format=raw,file=fat16.img \
	  -usb \
	  -d int
clean:
	rm -f $(KERNEL_OBJECTS) $(DRIVER_OBJECTS) $(MISC_OBJECTS) $(FS_OBJECTS) kernel.elf kernel.bin bootloader/bootc.elf bootloader/bootc.bin bootloader/boot.bin
	rm -f fat16.img
.PHONY: all run run-fat16 clean
