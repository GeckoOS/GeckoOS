#include "boot/multiboot2.h"
#include "drivers/acpi.h"
#include "drivers/apic/lapic.h"
#include "drivers/mouse.h"
#include "drivers/tables/isr.h"
#include <drivers/pit.h>
#include "drivers/uhci.h"
#include "mem.h"
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

// extern struct pci_bus pci_root_bus;
extern struct multiboot2_mmap_entry max_mem_used;
extern struct madt_iso pit_timer_iso;

bool has_apic;

__attribute__((section(".text.entry")))
void _entry(uint64_t mbi) {
    initialize_memory_manager_from_mbi(mbi);
    kalloc_init(max_mem_used.base_addr + 0x100000, max_mem_used.length);
    // mmio_map((uint64_t)max_mem_used.base_addr, max_mem_used.length);

    if (!vmm_init()) {
        printc("Vmm_init failed -- halting\n", VGA_COLOR_RED);
        for (;;)
            asm volatile("hlt");
    }
    outb(0x22, 0x70);
    outb(0x23, 0x01);

    //=====================Setting up interrupts===================//

    has_apic = cpu_has_apic();

    int ret = acpi_init(); // This activates the PIT timer if the is a interrupt source override with the irq of the PIT timer
    if (ret != 0) {
        set_printf_color(VGA_COLOR_RED);
            printf("initializing apic failed: %d \n", ret);
        set_printf_color(VGA_COLOR_WHITE);
    }

    printc("Mapping IDT... \n", VGA_COLOR_LIGHT_GREY);
    init_idt();
    printc("Installing IRQ... \n", VGA_COLOR_LIGHT_GREY);
    irq_install();

    // pit_timer_wait(100);

    if (has_apic) {
        asm volatile("cli"); // cutting interrupts while we set em up
        printc("Setting up LAPIC... \n", VGA_COLOR_LIGHT_GREY);
        ret = lapic_init();
        if (ret) {
            printf("Setting up LAPIC failed err %d", ret);
        }
        printc("Preparing IOAPIC... \n", VGA_COLOR_LIGHT_GREY);
        ioapic_init();
        asm volatile("sti"); // repoening interrupts

        printc("Enabling Timer...\n", VGA_COLOR_LIGHT_GREY);
        start_pit_timer(PITHZ);
        lapic_timer_start();
        lapic_start_cores();
    } else {
        // Use PIT as the timer
        // printc("Using PIT timer (50Hz)... \n", VGA_COLOR_LIGHT_GREY);
        // start_pit_timer(50);
    }
    //================= hardware init===================//
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

    // network init
    net_init();
    arp_init();
    drives_init();

    #ifndef DEBUG
        terminal_clear(TERM_COLOR);
    #else
        printf("\n");
    #endif

    printf("GeckoOS Version %s\n", GECKO_VERSION);
    #ifdef DEBUG
        printf("Booted via %s/Multiboot2.\n", bootloader_info->string);
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

    while (1) {
        printc("gecko> ", PROMPT_COLOR);
        unsigned char buff[512];
        input(buff, 512, TERM_COLOR);
        process_input(buff);
    }
}