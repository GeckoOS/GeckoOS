#ifndef PCI_H
#define PCI_H

#include <stdint.h>

struct pci_bus {
    struct pci_bus *parent;
    struct pci_bus *children;
    struct pci_bus *next;

    struct pci_dev *self;
    struct pci_dev *devices;

    void *sysdata;

    uint8_t bus;
    uint8_t primary;
    uint8_t secondary;
    uint8_t subordinate;
};

struct pci_dev {
    struct pci_bus *bus;
    struct pci_dev *sibling;
    struct pci_dev *next;

    void *sysdata;

    uint32_t devfn;
    uint16_t vendor;
    uint16_t device;
    uint32_t class;
    uint32_t master : 1;

    uint8_t irq;
};

struct pci_common_hdr {
    uint16_t vendid;
    uint16_t devid;
    uint16_t cmd;
    uint16_t status;
    uint8_t  revid;
    uint8_t  prog_if;
    uint8_t  subclass;
    uint8_t  clas;
    uint8_t  clz;
    uint8_t  lt;
    uint8_t  htype;
    uint8_t  bist;
} __attribute__((packed));

struct pci_hdr_dev {
    uint32_t bar[6];
    uint32_t cardptr;
    uint16_t subvendid;
    uint16_t subid;
    uint32_t erombs;
    uint8_t  capptr;
    uint8_t  res[7];
    uint8_t  iline;
    uint8_t  ipin;
    uint8_t  mingrant;
    uint8_t  maxlat;
} __attribute__((packed));

struct pci_hdr_brd {
    uint32_t bar[2];
    uint8_t  p_busnum;
    uint8_t  s_busnum;
    uint8_t  ss_busnum;
    uint8_t  s_lt;
    uint8_t  iobase;
    uint8_t  iolim;
    uint16_t s_stat;
    uint16_t membase;
    uint16_t memlim;
    uint16_t pmembase;
    uint16_t pmemlim;
    uint32_t pupperbase;
    uint32_t pupperlim;
    uint16_t ioupperbase;
    uint16_t ioupperlim;
    uint8_t  capptr;
    uint8_t  res[3];
    uint32_t erombs;
    uint8_t  iline;
    uint8_t  ipin;
    uint16_t bctrl;
} __attribute__((packed));

struct pci_hdr_crd {
    uint32_t cardbus_addr;
    uint8_t  cl_offset;
    uint8_t  res;
    uint16_t s_stat;
    uint8_t  pcibusnum;
    uint8_t  cbusnum;
    uint8_t  ss_busnum;
    uint8_t  cbus_lt;
    uint32_t membase0;
    uint32_t memlim0;
    uint32_t membase1;
    uint32_t memlim1;
    uint32_t iobase0;
    uint32_t iolim0;
    uint32_t iobase1;
    uint32_t iolim1;
    uint8_t  iline;
    uint8_t  ipin;
    uint16_t bctrl;
} __attribute__((packed));

struct pci_hdr {
    struct pci_common_hdr common;
    union {
        struct pci_hdr_dev dev;
        struct pci_hdr_brd bridge;
        struct pci_hdr_crd card_bridge;
    };
} __attribute__((packed));

uint32_t pci_readl(uint32_t bus, uint32_t slot, uint32_t func, uint8_t off);
uint16_t pci_readw(uint32_t bus, uint32_t slot, uint32_t func, uint8_t off);
uint8_t  pci_readb(uint32_t bus, uint32_t slot, uint32_t func, uint8_t off);
void     pci_writel(uint32_t bus, uint32_t slot, uint32_t func, uint32_t off, uint32_t data);
void     pci_writew(uint32_t bus, uint32_t slot, uint32_t func, uint32_t off, uint16_t data);
void     pci_writeb(uint32_t bus, uint32_t slot, uint32_t func, uint32_t off, uint8_t data);

uint8_t enumerate_pcidev(struct pci_bus *parent, uint8_t dev, uint8_t func);
void    enumerate_pcibus(struct pci_bus *bus);
void    enumerate_pci();

void pci_lspci();
void pci_detect_nics();

#endif