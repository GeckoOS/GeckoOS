# GeckoOS Roadmap

A document detailing the planned future of GeckoOS

---

## Networking Foundation (Short Term)

UDP and DHCP are the important. They allow real network functionality and are prerequisites for almost everything networking-related going forward.

- [ ] **UDP stack** — implement UDP send/receive in the IP layer; required for DHCP and DNS
- [ ] **DHCP client** — replace the hardcoded `10.0.2.15` / `10.0.2.2` IP and gateway with a proper DHCP handshake; should work on any network without recompiling
- [ ] **TAP networking in QEMU** — switch from `-netdev user` to a TAP interface so real internet traffic can be tested; `-netdev user` silently drops raw ICMP and won't forward properly
- [ ] **DNS resolver** — basic UDP-based DNS so hostnames can be resolved; unlocks useful internet use

---

## Hardware Compatibility

PS/2 is legacy on any hardware made in the last decade. USB HID is the most important step toward running on real machines. Similarly, the ATA driver should be replaced with a SATA SSD driver.

- [ ] **USB HID driver** — implement UHCI/EHCI and USB HID so keyboard and mouse work on hardware without PS/2 ports
- [ ] **AHCI driver** — replace the ATA/IDE driver with proper AHCI so real SATA SSDs and modern drives are supported
- [ ] **Real hardware test** — boot on a Dell OptiPlex 780 or 9010/9020; I just found these and they have the right network card and are cheap on Ebay. [Help Wanted]

---

## Display and shell

VGA text mode is old and doesn't work on modern hardware. A framebuffer allows for real graphics.

- [ ] **GOP framebuffer** — replace the VGA text mode driver with a UEFI GOP pixel framebuffer
- [ ] **Improved shell** — add proper argument parsing, and tab completion
- [ ] **TCP stack** — implement TCP on top of the existing IP layer; opens up HTTP, SSH, and real internet protocols
- [ ] **HTTP client** — basic HTTP GET over TCP; VERY DIFFICULT
---

## IMPORTANT FEATURES

I didn't know what to label them but they are needed.

- [ ] **ELF loader** — load and execute ELF binaries from disk
- [ ] **Syscall interface** — implement a proper user/kernel boundary with syscalls
- [ ] **User/kernel memory separation** — enforce privilege levels so user programs can't read or write kernel memory
- [ ] **Basic C runtime** — provide enough of a C standard library that simple programs compiled with a cross-compiler can run on GeckoOS
- [ ] **Multiple processes** — expand the current process system to support running more than one program at a time

---

## Long Term QOL

- [ ] **GRUB UEFI boot** — build and install GRUB as a `.efi` binary so GeckoOS can boot on UEFI machines without legacy BIOS fallback
- [ ] **Installer** — write a basic installer that can copy GeckoOS onto a real disk partition
- [ ] **Dual boot support** — install alongside an existing OS without destroying the EFI partition or bootloader
- [ ] **NVMe driver** — required for machines with NVMe SSDs; Mostly useful for wanting to boot on modern hardware
- [ ] **I219 NIC driver** — Just another modern network card

---

## Hardware Targets
(In chronological order)

- Dell OptiPlex 780 (Just real hardware, basically what QEMU emulates)
- Dell OptiPlex 9010/9020 (UEFI boot)
- Thinkpad T480 or similar (NVMe, USB, I219)
- Other Thinkpads and Towers

---

## Not Planned

- ARM / Raspberry Pi port
- Custom Bootloader