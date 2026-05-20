#include "terminal/printf.h"
#include <drivers/pci.h>
#include <drivers/e1000.h>
#include <mem.h>
#include <ports.h>
#include <stddef.h>

#define PCI_CFG_ADDR 0xCF8
#define PCI_CFG_DATA 0xCFC

#define PCI_DEV(devfn) ((devfn) >> 3)
#define PCI_FN(devfn)  ((devfn) & 7)

#define PCI_CLASS_NETWORK 0x02

struct pci_bus pci_root_bus;

uint32_t pci_readl(uint32_t bus, uint32_t slot, uint32_t func, uint8_t off) {
    uint32_t addr = (bus << 16) | (slot << 11) | (func << 8) | (off & 0xFC) | 0x80000000;
    outl(PCI_CFG_ADDR, addr);
    return inl(PCI_CFG_DATA);
}

uint16_t pci_readw(uint32_t bus, uint32_t slot, uint32_t func, uint8_t off) {
    uint32_t addr = (bus << 16) | (slot << 11) | (func << 8) | (off & 0xFC) | 0x80000000;
    outl(PCI_CFG_ADDR, addr);
    return (uint16_t)((inl(PCI_CFG_DATA) >> ((off & 2) * 8)) & 0xffff);
}

uint8_t pci_readb(uint32_t bus, uint32_t slot, uint32_t func, uint8_t off) {
    uint32_t v = pci_readl(bus, slot, func, off & ~3);
    return (v >> ((off & 3) * 8)) & 0xff;
}

void pci_writel(uint32_t bus, uint32_t slot, uint32_t func, uint32_t off, uint32_t data) {
    uint32_t addr = (bus << 16) | (slot << 11) | (func << 8) | (off & 0xFC) | 0x80000000;
    outl(PCI_CFG_ADDR, addr);
    outl(PCI_CFG_DATA, data);
}

void pci_writew(uint32_t bus, uint32_t slot, uint32_t func, uint32_t off, uint16_t data) {
    uint32_t align = off & ~3;
    uint32_t o     = pci_readl(bus, slot, func, align);
    uint8_t  shift = (off & 2) * 8;
    pci_writel(bus, slot, func, align, (o & ~(0xffff << shift)) | ((uint32_t)data << shift));
}

void pci_writeb(uint32_t bus, uint32_t slot, uint32_t func, uint32_t off, uint8_t data) {
    uint32_t align = off & ~3;
    uint32_t o     = pci_readl(bus, slot, func, align);
    uint8_t  shift = (off & 3) * 8;
    pci_writel(bus, slot, func, align, (o & ~(0xff << shift)) | ((uint32_t)data << shift));
}

static const char *class2str(uint8_t class) {
    switch (class) {
    case 0x00: return "Unclassified";
    case 0x01: return "Mass Storage Controller";
    case 0x02: return "Network Controller";
    case 0x03: return "Display Controller";
    case 0x04: return "Multimedia Controller";
    case 0x05: return "Memory Controller";
    case 0x06: return "Bridge";
    case 0x07: return "Simple Communication Controller";
    case 0x08: return "Base System Peripheral";
    default:   return "Unknown";
    }
}

static const char *nic_vendor_str(uint16_t vendor) {
    switch (vendor) {
    case 0x8086: return "Intel";
    case 0x10EC: return "Realtek";
    case 0x14E4: return "Broadcom";
    case 0x1022: return "AMD";
    case 0x15AD: return "VMware";
    default:     return NULL;
    }
}

static const char *nic_model_str(uint16_t vendor, uint16_t device) {
    if (vendor == 0x8086) {
        switch (device) {
        case 0x100E: return "82540EM";
        case 0x100F: return "82545EM";
        case 0x10D3: return "82574L";
        case 0x1533: return "I210";
        case 0x15B8: return "I219-V";
        }
    }
    if (vendor == 0x10EC) {
        switch (device) {
        case 0x8139: return "RTL8139";
        case 0x8168: return "RTL8111/8168B";
        case 0x8125: return "RTL8125";
        }
    }
    if (vendor == 0x15AD) {
        switch (device) {
        case 0x0720: return "VMXNET3";
        case 0x0740: return "PCnet32";
        }
    }
    return NULL;
}

static void alloc_new_bus(struct pci_bus *parent, struct pci_dev *self) {
    struct pci_bus *child = kmalloc(sizeof(*child));

    child->next     = NULL;
    child->parent   = parent;
    child->self     = self;
    child->devices  = NULL;
    child->children = NULL;

    child->secondary = pci_readb(
        parent->bus, PCI_DEV(self->devfn), PCI_FN(self->devfn),
        offsetof(struct pci_hdr, bridge.ss_busnum));

    child->bus     = child->secondary;
    child->primary = child->secondary;

    if (child->secondary == parent->bus)
        parent->subordinate++;

    if (parent->children == NULL) {
        parent->children = child;
        enumerate_pcibus(child);
        return;
    }

    struct pci_bus *end = parent->children;
    while (end->next && end->next != end)
        end = end->next;

    end->next = child;
    enumerate_pcibus(child);
}

uint8_t enumerate_pcidev(struct pci_bus *parent, uint8_t dev, uint8_t func) {
    uint32_t tmp = pci_readl(parent->primary, dev, func,
                             offsetof(struct pci_common_hdr, vendid));

    if (tmp == 0xffffffff)
        return 0;

    struct pci_dev *pci_dev = kmalloc(sizeof(*pci_dev));

    uint8_t htype = pci_readb(parent->primary, dev, func,
                              offsetof(struct pci_hdr, common.htype));

    pci_dev->bus     = parent;
    pci_dev->devfn   = (dev << 3) | (func & 7);
    pci_dev->class   = pci_readb(parent->primary, dev, func,
                                 offsetof(struct pci_hdr, common.clas));
    pci_dev->device  = tmp >> 16;
    pci_dev->vendor  = tmp & 0xffff;
    pci_dev->sibling = NULL;
    pci_dev->next    = NULL;

    if ((htype & 0x7f) == 1)
        alloc_new_bus(parent, pci_dev);

    if (parent->devices == NULL) {
        parent->devices = pci_dev;
        return 0;
    }

    struct pci_dev *endpoint = parent->devices;
    while (endpoint->sibling)
        endpoint = endpoint->sibling;

    endpoint->sibling = pci_dev;
    return 0;
}

void enumerate_pcibus(struct pci_bus *bus) {
    for (uint8_t device = 0; device < 32; device++) {
        for (uint8_t fn = 0; fn < 8; fn++) {
            uint8_t htype = pci_readb(bus->primary, device, fn,
                                      offsetof(struct pci_hdr, common.htype));
            if (htype == 0xff)
                break;

            enumerate_pcidev(bus, device, fn);

            if (!(htype & 0x80))
                break;
        }
    }
}

void enumerate_pci() {
    pci_root_bus.bus         = 0;
    pci_root_bus.primary     = 0;
    pci_root_bus.children    = NULL;
    pci_root_bus.devices     = NULL;
    pci_root_bus.parent      = NULL;
    pci_root_bus.self        = NULL;
    pci_root_bus.subordinate = 0;

    enumerate_pcibus(&pci_root_bus);
}

static void walk_buses(struct pci_bus *start, void (*cb)(struct pci_dev *)) {
    struct pci_bus *cur = start;
    struct pci_bus *visited[64];
    int nvisited = 0;

    while (cur) {
        int already_seen = 0;
        for (int i = 0; i < nvisited; i++) {
            if (visited[i] == cur) { already_seen = 1; break; }
        }
        if (already_seen) break;
        if (nvisited < 64) visited[nvisited++] = cur;

        for (struct pci_dev *dev = cur->devices; dev; dev = dev->sibling)
            cb(dev);

        if (cur->children) { cur = cur->children; continue; }
        cur = cur->next;
    }
}

static void lspci_cb(struct pci_dev *dev) {
    printf("%02x:%02x.%d %s\n",
           dev->bus->primary,
           PCI_DEV(dev->devfn),
           PCI_FN(dev->devfn),
           class2str(dev->class & 0xff));
}

void pci_lspci() {
    walk_buses(&pci_root_bus, lspci_cb);
}

static void nic_detect_cb(struct pci_dev *dev) {
    if ((dev->class & 0xff) != PCI_CLASS_NETWORK)
        return;

    uint8_t bus  = dev->bus->primary;
    uint8_t slot = PCI_DEV(dev->devfn);
    uint8_t fn   = PCI_FN(dev->devfn);

    uint8_t subclass = pci_readb(bus, slot, fn,
                                 offsetof(struct pci_hdr, common.subclass));

    const char *vendor = nic_vendor_str(dev->vendor);
    const char *model  = nic_model_str(dev->vendor, dev->device);

    if (vendor && model)
        printf("  [%02x:%02x.%d] %s %s\n", bus, slot, fn, vendor, model);
    else if (vendor)
        printf("  [%02x:%02x.%d] %s (device %04x, subclass %02x)\n",
               bus, slot, fn, vendor, dev->device, subclass);
    else
        printf("  [%02x:%02x.%d] Unknown NIC (%04x:%04x, subclass %02x)\n",
               bus, slot, fn, dev->vendor, dev->device, subclass);

    if (dev->vendor == 0x8086 && dev->device == 0x100E)
        e1000_init(bus, slot, fn);
}

void pci_detect_nics() {
    printf("Network controllers:\n");
    walk_buses(&pci_root_bus, nic_detect_cb);
}