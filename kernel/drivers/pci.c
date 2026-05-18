#include "terminal/printf.h"
#include <drivers/pci.h>
#include <mem.h>
#include <ports.h>
#include <stddef.h>

// nfoxers - PCI subsystem

#define PCI_CFG_ADDR 0xCF8
#define PCI_CFG_DATA 0xCFC

#define PCI_DEV(devfn) ((devfn) >> 3)
#define PCI_FN(devfn) ((devfn) & 7)

struct pci_bus pci_root_bus;

/* APIs for I/O access stuff */

uint32_t pci_readl(uint32_t bus, uint32_t slot, uint32_t func, uint8_t off) {
  uint32_t addr = (bus << 16) | (slot << 11) | (func << 8) | (off & 0xFC) |
                  ((uint32_t)0x80000000);
  outl(PCI_CFG_ADDR, addr);
  return inl(PCI_CFG_DATA);
}

uint16_t pci_readw(uint32_t bus, uint32_t slot, uint32_t func, uint8_t off) {
  uint32_t addr = (bus << 16) | (slot << 11) | (func << 8) | (off & 0xFC) |
                  ((uint32_t)0x80000000);
  outl(PCI_CFG_ADDR, addr);
  uint32_t tmp = (uint16_t)((inl(PCI_CFG_DATA) >> ((off & 2) * 8)) & 0xffff);
  return tmp;
}

uint8_t pci_readb(uint32_t bus, uint32_t slot, uint32_t func, uint8_t off) {
  uint8_t align = off & ~3;
  uint32_t v = pci_readl(bus, slot, func, align);

  return (v >> ((off & 3) * 8)) & 0xff;
}

void pci_writel(uint32_t bus, uint32_t slot, uint32_t func, uint32_t off,
                uint32_t data) {
  uint32_t addr = (bus << 16) | (slot << 11) | (func << 8) | (off & 0xFC) |
                  ((uint32_t)0x80000000);
  outl(PCI_CFG_ADDR, addr);
  outl(PCI_CFG_DATA, data);
}

void pci_writew(uint32_t bus, uint32_t slot, uint32_t func, uint32_t off,
                uint16_t data) {
  uint32_t align = off & ~3;
  uint32_t o = pci_readl(bus, slot, func, align);
  uint8_t shift = (off & 2) * 8;
  uint32_t mask = 0xffff << shift;
  uint32_t new = (o & mask) | ((uint32_t)data << shift);

  pci_writel(bus, slot, func, align, new);
}

void pci_writeb(uint32_t bus, uint32_t slot, uint32_t func, uint32_t off,
                uint8_t data) {
  uint32_t align = off & ~3;
  uint32_t o = pci_readl(bus, slot, func, align);
  uint8_t shift = (off & 3) * 8;
  uint32_t mask = 0xff << shift;
  uint32_t new = (o & mask) | ((uint32_t)data << shift);

  pci_writel(bus, slot, func, align, new);
}

/* high level section */

const char *class2str(uint8_t class) {
  switch (class) {
  case 0x00:
    return "Unclassified";
  case 0x01:
    return "Mass Storage Controller";
  case 0x02:
    return "Network Controller";
  case 0x03:
    return "Display Controller";
  case 0x04:
    return "Multimedia Controller";
  case 0x05:
    return "Memory Controller";
  case 0x06:
    return "Bridge";
  case 0x07:
    return "Simple Communicatin Controller";
  case 0x08:
    return "Base System Peripheral";
  default:
    return "Unknown";
  }
}

void alloc_new_bus(struct pci_bus *parent, struct pci_dev *self) {
  struct pci_bus *child = kmalloc(sizeof(*child));

  child->next = NULL;
  child->parent = parent;
  child->self = self;
  child->devices = NULL;
  child->children = NULL;

  child->secondary =
      pci_readb(parent->bus, PCI_DEV(self->devfn), PCI_FN(self->devfn),
                offsetof(struct pci_hdr, bridge.ss_busnum));
  child->bus = parent->bus;
  child->primary = child->secondary;

  if (child->secondary == parent->bus)
    parent->subordinate++;

  if (parent->children == NULL) {
    parent->children = child;
  }

  struct pci_bus *end = parent->children;
  while (end->next) {
    end = end->next;
  }
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

  pci_dev->bus = parent;
  pci_dev->devfn = dev << 3 | (func & 7);
  pci_dev->class = pci_readb(parent->primary, dev, func,
                             offsetof(struct pci_hdr, common.clas));
  pci_dev->device = tmp >> 16;
  pci_dev->vendor = tmp & 0xffff;
  pci_dev->sibling = NULL;
  pci_dev->next = NULL;

  if ((htype & 0x7f) == 1)
    alloc_new_bus(parent, pci_dev);

  if (parent->devices == NULL) {
    parent->devices = pci_dev;
    return 0;
  }

  struct pci_dev *endpoint = parent->devices;
  while (endpoint->sibling) {
    endpoint = endpoint->sibling;
  }
  endpoint->sibling = pci_dev;
  return 0;
}

void enumerate_pcibus(struct pci_bus *bus) {
  uint8_t device = 0;
  uint8_t fn = 0;

  for (device = 0; device < 32; device++) {
    for (fn = 0; fn < 8; fn++) {
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
  pci_root_bus.bus = 0;
  pci_root_bus.primary = 0;
  pci_root_bus.children = NULL;
  pci_root_bus.devices = NULL;
  pci_root_bus.parent = NULL;
  pci_root_bus.self = NULL;
  pci_root_bus.subordinate = 0;

  enumerate_pcibus(&pci_root_bus);
}

/* userspace stuff */

void pci_lspci() { // ras syndrome ahh name
  struct pci_bus *cur = &pci_root_bus;

  while (cur) {
    struct pci_dev *dev = cur->devices;
    while (dev) {
      printf("%02x:%02x.%d %s\n", dev->bus->primary, PCI_DEV(dev->devfn),
              PCI_FN(dev->devfn), class2str(dev->class & 0xff));

      dev = dev->sibling;
    }
    if(cur->children) {cur = cur->children; continue;}
    // ! this must be placed to prevent bus loops. why did it happen in the first place?
    // todo: fix `child->next = child`
    if(cur->next == cur) return;
    
    cur = cur->next;
  }
}