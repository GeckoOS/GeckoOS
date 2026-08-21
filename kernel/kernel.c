#include "boot/multiboot2.h"
#include "drivers/acpi.h"
#include "drivers/apic/lapic.h"
#include "drivers/mouse.h"
#include "drivers/tables/isr.h"
#include "drivers/uhci.h"
#include "drivers/usb.h"
#include "ports.h"
#include "terminal/printf.h"
#include <colors.h>
#include <commands.h>
#include <drivers/apic/ioapic.h>
#include <drivers/drives.h>
#include <drivers/keyboard.h>
#include <drivers/pci.h>
#include <drivers/serial.h>
#include <drivers/tables/idt.h>
#include <drivers/tables/irq.h>
#include <drivers/vga.h>
#include <layouts/kb_layouts.h>
#include <mem/paging.h>
#include <mem/physical_mem.h>
#include <net/arp.h>
#include <net/net.h>
#include <stdbool.h>
#include <stdint.h>
#include <terminal/terminal.h>
#include <fs/fs.h>

#define GECKO_VERSION "2.2"

void process_input(unsigned char *buffer) {
    run_command(buffer, TERM_COLOR);
}

void kmain();

#ifdef DEBUG
    extern struct multiboot2_tag_bootloader_name* bootloader_info;
#endif

// uint64_t global_table;

extern struct pci_bus pci_root_bus;

__attribute__((section(".text.entry")))
void _entry(uint64_t mbi) {
    initialize_memory_manager_from_mbi(mbi);
    kalloc_init();

    if (!vmm_init()) {
        printc("Vmm_init failed -- halting\n", VGA_COLOR_RED);
        for (;;)
            asm volatile("hlt");
    }
    outb(0x22, 0x70);
    outb(0x23, 0x01);

    //=====================Setting up interrupts===================//
    int ret = acpi_init();
    if (ret != 0) {
        set_printf_color(VGA_COLOR_RED);
            printf("initializing apic failed: %d \n", ret);
        set_printf_color(VGA_COLOR_WHITE);
    }

    if (cpu_has_apic()) {
        asm volatile("cli"); // cutting interrupts while we set em up
        printc("Mapping IDT... \n", VGA_COLOR_LIGHT_GREY);
        init_idt();
        printc("Installing IRQ... \n", VGA_COLOR_LIGHT_GREY);
        irq_install();
        printc("Setting up LAPIC... \n", VGA_COLOR_LIGHT_GREY);
        ret = lapic_init();
        if (ret) {
            printf("Setting up LAPIC failed err %d", ret);
        }
        printc("Preparing IOAPIC... \n", VGA_COLOR_LIGHT_GREY);
        ioapic_init();
        asm volatile("sti"); // repoening interrupts
    } else {
        // idk should have apic not my problem
    }
    //================= hardware init===================//
    printc("Enabling Timer...\n", VGA_COLOR_LIGHT_GREY);
    lapic_timer_start();
    printc("Enabling hardware devices...\n", VGA_COLOR_LIGHT_GREY);
    // basic stuff
    keyboard_install();
    set_layout(LAYOUTS[0]);
    mouse_init();
    terminal_init();
    register_interrupt_handler(0x6, ud_exception_handler);
    // pci init
    enumerate_pci();
    pci_detect_controllers();

    // network init//
    lapic_start_cores();
    net_init();
    arp_init();
    drives_init();

    #ifndef DEBUG
        terminal_clear(TERM_COLOR);
    #endif

    printf("GeckoOS Version %s\n", GECKO_VERSION);
    #ifdef DEBUG
        printf("Booted via %s/Multiboot2.\n", bootloader_info->string);
        // pci_lspci();
    #else
        printc("Booted via GRUB/Multiboot2.\n", TERM_COLOR);
    #endif

    kmain();
}

void kmain() {
    for (int i = 1; i < 5; i++) {
        printf("Trying drive %d", i);
        if (fsmount(i)) break;
    } if (!fs)
        printc("The drives 1 - 4 don't have any disk attached (Or it failed when mounting the FAT32 filesystem)\n\n", VGA_COLOR_RED);

    GetUHCIDescriptor((struct UHCIDevice*)USBDevices[0].data);

    while (1) {
        printc("gecko> ", PROMPT_COLOR);
        unsigned char buff[512];
        input(buff, 512, TERM_COLOR);
        process_input(buff);
    }
}