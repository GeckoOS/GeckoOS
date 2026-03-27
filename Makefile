# By Ember2819, google, and random people on the internet.... What are we doing????
# C compiler
CC = ccache clang
# Secondary C compiler
CC2 = gcc
# Assembler (for boot.s)
AS = nasm
# Linker
LD = ld
# Truncate (to make the kernel.bin divisible by 512)
TRUNCATE = truncate
TRUNC_AMNT = 360448
# Objcopy (to translate elf to bin)
OBJCOPY = objcopy
OBJCOPY_ARGS = -O binary
CC_FLAGS = -m32 -ffreestanding -nostdlib -fno-builtin -fno-stack-protector -g -c
AS_FLAGS = -f bin
LD_FLAGS = -m elf_i386 -T linker.ld
KERNEL_OBJECTS = kernel/kernel.o kernel/ports.o kernel/mem.o
DRIVER_OBJECTS = kernel/drivers/vga.o kernel/drivers/keyboard.o kernel/drivers/tables/idt/idt_c.o kernel/drivers/tables/idt/idt_s.o \
	kernel/drivers/tables/isr/isr_c.o kernel/drivers/tables/isr/isr_s.o kernel/drivers/tables/irq/irq_c.o kernel/drivers/tables/irq/irq_s.o \
	kernel/drivers/tables/timer/timer.o kernel/drivers/panic.o
MISC_OBJECTS = kernel/colors.o kernel/terminal/terminal.o kernel/commands.o kernel/layouts/kb_layouts.o \
               kernel/comos/comos_lexer.o kernel/comos/comos_parser.o kernel/comos/comos_interp.o kernel/paging/paging.o # ADDED
# Ember2819
FS_OBJECTS = kernel/drivers/ata.o kernel/fs/fat16.o

# Builds the final disk image
all: os.img
	
# If no clang detected, use gcc
%.o: %.c
	$(CC) $(CC_FLAGS) $< -o $@ || $(CC2) $(CC_FLAGS) $< -o $@
# Assemble the bootloader
bootloader/boot.bin: bootloader/boot.s
	$(AS) $(AS_FLAGS) $< -o $@
# Added by MorganPG1
kernel/colors.o: kernel/colors.c
	$(CC) $(CC_FLAGS) $< -o $@ || $(CC2) $(CC_FLAGS) $< -o $@
kernel/layouts/kb_layouts.o: kernel/layouts/kb_layouts.c
	$(CC) $(CC_FLAGS) $< -o $@ || $(CC2) $(CC_FLAGS) $< -o $@

# IDT IRQ IRQ
kernel/drivers/tables/idt/idt_c.o: kernel/drivers/tables/idt/idt.c
	$(CC) $(CC_FLAGS) $< -o $@ || $(CC2) $(CC_FLAGS) $< -o $@
kernel/drivers/tables/isr/isr_c.o: kernel/drivers/tables/isr/isr.c
	$(CC) $(CC_FLAGS) $< -o $@ || $(CC2) $(CC_FLAGS) $< -o $@
kernel/drivers/tables/irq/irq_c.o: kernel/drivers/tables/irq/irq.c
	$(CC) $(CC_FLAGS) $< -o $@ || $(CC2) $(CC_FLAGS) $< -o $@
kernel/drivers/tables/timer/timer.o: kernel/drivers/tables/timer/timer.c
	$(CC) $(CC_FLAGS) $< -o $@ || $(CC2) $(CC_FLAGS) $< -o $@
kernel/drivers/tables/irq/irq_s.o: kernel/drivers/tables/irq/irq.s
	$(AS) -felf32 $< -o $@
kernel/drivers/tables/idt/idt_s.o: kernel/drivers/tables/idt/idt.s
	$(AS) -felf32 $< -o $@
kernel/drivers/tables/isr/isr_s.o: kernel/drivers/tables/isr/isr.s
	$(AS) -felf32 $< -o $@
# Paging & Panic
kernel/paging/paging.o: kernel/paging/paging.c
	$(CC) $(CC_FLAGS) $< -o $@ || $(CC2) $(CC_FLAGS) $< -o $@
kernel/drivers/panic.o: kernel/drivers/panic.c
	$(CC) $(CC_FLAGS) $< -o $@ || $(CC2) $(CC_FLAGS) $< -o $@

# Compile drivers
kernel/drivers/vga.o: kernel/drivers/vga.c
	$(CC) $(CC_FLAGS) $< -o $@ || $(CC2) $(CC_FLAGS) $< -o $@
kernel/drivers/keyboard.o:  kernel/drivers/keyboard.c
	$(CC) $(CC_FLAGS) $< -o $@ || $(CC2) $(CC_FLAGS) $< -o $@
kernel/ports.o: kernel/ports.c
	$(CC) $(CC_FLAGS) $< -o $@ || $(CC2) $(CC_FLAGS) $< -o $@
kernel/mem.o: kernel/mem.c
	$(CC) $(CC_FLAGS) $< -o $@ || $(CC2) $(CC_FLAGS) $< -o $@
kernel/terminal/terminal.o: kernel/terminal/terminal.c
	$(CC) $(CC_FLAGS) $< -o $@ || $(CC2) $(CC_FLAGS) $< -o $@
# Added by Ember2819
kernel/commands.o: kernel/commands.c
	$(CC) $(CC_FLAGS) $< -o $@ || $(CC2) $(CC_FLAGS) $< -o $@
# End added by Ember2819

kernel/comos/comos_lexer.o: kernel/comos/comos_lexer.c
	$(CC) $(CC_FLAGS) $< -o $@ || $(CC2) $(CC_FLAGS) $< -o $@
kernel/comos/comos_parser.o: kernel/comos/comos_parser.c
	$(CC) $(CC_FLAGS) $< -o $@ || $(CC2) $(CC_FLAGS) $< -o $@
kernel/comos/comos_interp.o: kernel/comos/comos_interp.c
	$(CC) $(CC_FLAGS) $< -o $@ || $(CC2) $(CC_FLAGS) $< -o $@

# [Ember2819: BEGIN - FAT16 + ATA compile rules]
kernel/drivers/ata.o: kernel/drivers/ata.c
	$(CC) $(CC_FLAGS) $< -o $@ || $(CC2) $(CC_FLAGS) $< -o $@
kernel/fs/fat16.o: kernel/fs/fat16.c
	$(CC) $(CC_FLAGS) $< -o $@ || $(CC2) $(CC_FLAGS) $< -o $@
# [Ember2819: END]

# Link all kernel objects 
kernel.elf: $(KERNEL_OBJECTS) $(DRIVER_OBJECTS) $(MISC_OBJECTS) $(FS_OBJECTS)
	$(LD) $(LD_FLAGS) $(KERNEL_OBJECTS) $(DRIVER_OBJECTS) $(MISC_OBJECTS) $(FS_OBJECTS) -o kernel.elf
kernel.bin: kernel.elf
	$(OBJCOPY) $(OBJCOPY_ARGS) kernel.elf kernel.bin
	$(TRUNCATE) -s $(TRUNC_AMNT) kernel.bin
os.img: bootloader/boot.bin kernel.bin
	cat bootloader/boot.bin kernel.bin > os.img
# Launch the image in QEMU
run: os.img
	qemu-system-i386 -s -drive format=raw,file=os.img -usb

fat16.img:
	dd if=/dev/zero of=fat16.img bs=512 count=8192
	mkfs.fat -F 16 -n "GECKOOS" fat16.img
	@echo "fat16.img created. Copy files onto it with:"
	@echo "  mcopy -i fat16.img yourfile.txt ::yourfile.txt"

run-fat16: os.img fat16.img
	qemu-system-i386 -s \
	  -drive format=raw,file=os.img \
	  -drive format=raw,file=fat16.img \
	  -usb
clean:
	rm -f $(KERNEL_OBJECTS) $(DRIVER_OBJECTS) $(MISC_OBJECTS) $(FS_OBJECTS) kernel.elf kernel.bin bootloader/boot.bin
	rm -f fat16.img
.PHONY: all run run-fat16 clean
