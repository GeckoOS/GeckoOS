# GeckoOS

A community-built bare-metal operating system for x86 machines, written entirely from scratch.

## Vision

A collaborative OS project where contributors build a custom operating system from the ground up. Any contributions or feedback are welcome!

## Features

- **Bootloader** — Custom x86 bootloader (legacy, in `bootloader/legacy/`); active boot path is GRUB2 + Multiboot2
- **Kernel** — Full kernel implementation with:
  - VGA text mode driver
  - AT keyboard driver with multiple layouts
  - ATA disk driver (primary IDE, master + slave)
  - Programmable timer
  - Interrupt handling (IDT, ISR, IRQ)
- **Memory Management**
  - Physical memory management
  - Virtual memory with paging
- **File System** — FAT32 support
- **Partition Support** — MBR partitioning; raw (non-MBR) disks also supported
- **Shell** — Interactive command-line shell with file system commands
- **Text Editor** — Built-in terminal editor
- **Custom Language** — GK interpreted scripting language

---

## Building

### Prerequisites

**Fedora:**
```bash
sudo dnf install clang nasm binutils grub2-tools-extra xorriso mtools qemu-system-x86 -y
```

**Arch Linux:**
```bash
sudo pacman -S clang nasm binutils grub xorriso qemu-system-x86 mtools
```

**Debian / Ubuntu / any APT-based distro:**
```bash
sudo apt install clang nasm binutils grub-common xorriso qemu-system-x86 mtools grub2 -y
```

---

### Build and run

Note: The DEBUG mode is activated by default

**Step 1 — Create the FAT32 data disk:**
```bash
make fat32.img
```

**Step 2 — Build the kernel and ISO:**
```bash
make
```

**Step 3 — Run in QEMU:**
```bash
qemu-system-x86_64 -cdrom gecko.iso -drive file=fat32.img,format=raw,if=ide,index=1 -boot order=d
```

Once booted, run `fsmount` in the GeckoOS shell to mount the FAT32 data disk, then use `help` to see available filesystem commands.

**All three steps at once:**
```bash
make fat32.img && make && qemu-system-x86_64 -cdrom gecko.iso -drive file=fat32.img,format=raw,if=ide,index=1 -boot order=d
```

**Clean build artifacts:**
```bash
make clean
```

---

## Shell Commands

You need to run these commands with no arguments!

| Command | Description |
|---|---|
| `help` | List all commands |
| `fsmount` | Mount the FAT32 filesystem (Deprecated, The filesystem is mounted since boot) |
| `fsinfo` | Print FAT32 volume info |
| `ls` | List files on the mounted filesystem |
| `touch` | Create a new file |
| `rm` | Delete a file |
| `cp` | Copy a file |
| `mv` | Move/rename a file |
| `mkdir` | Create a directory entry |
| `write` | Append text to a file |
| `cat` | Print file contents |
| `edit` | Open the built-in text editor |

## How to Contribute

Submit ideas or feedback:
- General feedback: [Google Forms](https://docs.google.com/forms/d/e/1FAIpQLSe-euunJ8venpIXOD9_Luy_ZyDaszK5FUefVQ_QOV7YDge9w/viewform?usp=publish-editor)

Ways to contribute:
- Fix typos or improve wording
- Add new features
- Improve or optimize code
- Clean up code
- Tell others about the project

Apply contributions via:
- Fork the repo, make changes, and submit a PR

Add your name to the contributors list when you contribute!

### Contributing Guidelines

- [AI Usage Policy](Docs/AI_USAGE.md) — Rules and expectations for using AI in GeckoOS development
- [Help Wanted](Docs/HELP.md) — Areas where the project needs help (hardware, software, marketing, general)
- [Roadmap](Docs/ROADMAP.md) — Planned features and future direction of GeckoOS
- [Donate](Docs/DONATE.md) — How to support the project

---

## Contact

- Email: elroylilly@gmail.com
- [Mastodon](https://mastodon.social/@GeckoOS)
- Join the community:
  - [Matrix](https://matrix.to/#/#geckoos-official:matrix.org) ← Join this

---


## Contributors

- **Ember2819** (Founder & Manager)
- DarkThemeGeek
- TheOtterMonarch
- Just Pumpkicks
- nfoxers
- Sifi11 (Founder)
- Crim (OG)
- CheeseFunnel23
- bonk enjoyer/dorito girl (Bootloader Creator)
- KaleidoscopeOld5841
- billythemoon (V1 Website creator)
- TheGirl790 (OG)
- kotofyt
- xtn59
- c-bass
- u/EastConsequence3792
- MorganPG1
- Zorx555
- mckaylap2304 (V2 Website creator)
- codecrafter01001