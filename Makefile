# By Ember2819, google, and random people on the internet.... What are we doing????
# C compiler
# Is ccache really required?
CC = clang
# Assembler (for boot.s)
AS = nasm
# Linker
LD = ld
# Truncate (to make the kernel.bin divisible by 512)
TRUNCATE = truncate
TRUNC_ALIGN = 4096 

# Objcopy (to translate elf to bin)
OBJCOPY = objcopy
OBJCOPY_ARGS = -O binary
CC_FLAGS = -m32 -ffreestanding -nostdlib -fno-builtin -fno-stack-protector -g -c -I headers -I kernel
AS_FLAGS = -f bin
LD_FLAGS = -m elf_i386 -T linker.ld
KERNEL_OBJECTS = kernel/kernel.o kernel/mem.o kernel/drive.o
MISC_OBJECTS = kernel/colors.o kernel/terminal/terminal.o kernel/layouts/kb_layouts.o \
        kernel/comos/comos_lexer.o kernel/comos/comos_parser.o kernel/comos/comos_interp.o
DRIVER_OBJECTS = kernel/drivers/vga.o kernel/drivers/keyboard.o kernel/drivers/tables/idt/idt_c.o kernel/drivers/tables/idt/idt_s.o \
	kernel/drivers/tables/isr/isr_c.o kernel/drivers/tables/isr/isr_s.o kernel/drivers/tables/irq/irq_c.o kernel/drivers/tables/irq/irq_s.o kernel/drivers/tables/timer/timer.o \
	kernel/drivers/drives/ata.o

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
kernel/drivers/tables/irq/irq_s.o: kernel/drivers/tables/irq/irq.s
	$(AS) -felf32 $< -o $@
kernel/drivers/tables/isr/isr_s.o: kernel/drivers/tables/isr/isr.s
	$(AS) -felf32 $< -o $@

# Builds the final disk image
all: os.img
	
# If no clang detected, use gcc
%.o: %.c
	$(CC) $(CC_FLAGS) $< -o $@
# Assemble the bootloader
bootloader/boot.bin: bootloader/boot.s
	$(AS) $(AS_FLAGS) $< -o $@

# Link all kernel objects 
kernel.elf: $(KERNEL_OBJECTS) $(DRIVER_OBJECTS) $(MISC_OBJECTS)
	$(LD) $(LD_FLAGS) $(KERNEL_OBJECTS) $(DRIVER_OBJECTS) $(MISC_OBJECTS) -o $@
kernel.bin: kernel.elf
	$(OBJCOPY) $(OBJCOPY_ARGS) $< $@
os.img: bootloader/boot.bin kernel.bin
	cat bootloader/boot.bin kernel.bin README.md > os.img
	$(TRUNCATE) -s 1M $@
	$(TRUNCATE) -s $$(( ( $$(stat -c%s $@) + $(TRUNC_ALIGN) - 1 ) / $(TRUNC_ALIGN) * $(TRUNC_ALIGN) )) $@
# Launch the image in QEMU
run: os.img
	qemu-system-i386 -s -drive format=raw,file=os.img -usb
clean:
	rm -f $(KERNEL_OBJECTS) $(DRIVER_OBJECTS) $(MISC_OBJECTS) kernel.elf kernel.bin bootloader/boot.bin
.PHONY: all run clean
