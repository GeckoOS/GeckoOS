#ifndef PCI_H
#define PCI_H

// nfoxers - PCI

#include <stdint.h>

/* total abstraction */

struct pci_bus {
  struct pci_bus *parent;   // parent bus
  struct pci_bus *children; // chain of bridges in this bus
  struct pci_bus *next;     // chain of all p2p buses

  struct pci_dev *self;
  struct pci_dev *devices; // devices in this bridge

  void *sysdata; // hook for system specific extension

  uint8_t bus;         // bus number
  uint8_t primary;     // primary bridge number
  uint8_t secondary;   // secondary bridge number
  uint8_t subordinate; // max subordinates
};

struct pci_dev {
  struct pci_bus *bus;     // bus this device is on
  struct pci_dev *sibling; // next device on this bus
  struct pci_dev *next;    // chain of all devices

  void *sysdata;

  uint32_t devfn;  // encoded device & function index
  uint16_t vendor; // vendor id
  uint16_t device; // device id
  uint32_t class;  // 3 bytes: base, sub, prog-if
  uint32_t master : 1;

  uint8_t irq;
};

/* technical detail */

// todo: rename and document nicer for better judgment

struct pci_common_hdr {
  uint16_t vendid;
  uint16_t devid;
  uint16_t cmd;
  uint16_t status;
  uint8_t revid;
  uint8_t prog_if;
  uint8_t subclass;
  uint8_t clas;
  uint8_t clz;
  uint8_t lt;
  uint8_t htype;
  uint8_t bist;
} __attribute__((packed));

// regular device
struct pci_hdr_dev {
  uint32_t bar[6];
  uint32_t cardptr;
  uint16_t subvendid;
  uint16_t subid;
  uint32_t erombs;
  uint8_t capptr;
  uint8_t res[7];
  uint8_t iline;
  uint8_t ipin;
  uint8_t mingrant;
  uint8_t maxlat;
} __attribute__((packed));

// bridge device
struct pci_hdr_brd {
  uint32_t bar[2];
  uint8_t p_busnum;
  uint8_t s_busnum;
  uint8_t ss_busnum;
  uint8_t s_lt;
  uint8_t iobase;
  uint8_t iolim;
  uint16_t s_stat;
  uint16_t membase;
  uint16_t memlim;
  uint16_t pmembase;
  uint16_t pmemlim;
  uint32_t pupperbase;
  uint32_t pupperlim;
  uint16_t ioupperbase;
  uint16_t ioupperlim;
  uint8_t capptr;
  uint8_t res[3];
  uint32_t erombs;
  uint8_t iline;
  uint8_t ipin;
  uint16_t bctrl;
} __attribute__((packed));

// cardbus bridge
struct pci_hdr_crd {
  uint32_t cardbus_addr;
  uint8_t cl_offset;
  uint8_t res;
  uint16_t s_stat;
  uint8_t pcibusnum;
  uint8_t cbusnum;
  uint8_t ss_busnum;
  uint8_t cbus_lt;
  uint32_t membase0;
  uint32_t memlim0;
  uint32_t membase1;
  uint32_t memlim1;
  uint32_t iobase0;
  uint32_t iolim0;
  uint32_t iobase1;
  uint32_t iolim1;
  uint8_t iline;
  uint8_t ipin;
  uint16_t bctrl;
} __attribute__((packed));

struct pci_hdr {
  struct pci_common_hdr common;
  union {
    struct pci_hdr_dev dev;
    struct pci_hdr_brd bridge;
    struct pci_hdr_crd card_bridge;
  }; // anonymous union trick
} __attribute__((packed));

/* functions */

uint8_t enumerate_pcidev(struct pci_bus *parent, uint8_t dev, uint8_t func);
void enumerate_pcibus(struct pci_bus *bus);
void enumerate_pci();

void pci_lspci();

#endif