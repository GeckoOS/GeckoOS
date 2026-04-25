# Gecko OS

<!--toc:start-->
- [Gecko OS](#gecko-os)
  - [Vision](#vision)
  - [Features](#features)
  - [Roadmap](#roadmap)
  - [How to Contribute](#how-to-contribute)
  - [Building](#building)
    - [Prerequisites (Linux)](#prerequisites-linux)
    - [Build Commands](#build-commands)
  - [Contact](#contact)
  - [Screenshots](#screenshots)
  - [Contributors](#contributors)
  - [TODO](#todo)
<!--toc:end-->

A community-built bare-metal operating system for x86 machines, written entirely from scratch.

## Vision

A collaborative OS project where contributors build a custom operating system from the ground up. Any contributions or feedback are welcome!

## Features

- **Bootloader** - Custom x86 bootloader
- **Kernel** - Full kernel implementation with:
  - VGA text mode driver
  - AT keyboard driver with multiple layouts
  - ATA disk driver
  - Programmable timer
  - Interrupt handling (IDT, ISR, IRQ)
- **Memory Management**
  - Physical memory management
  - Virtual memory with paging
- **File System** - FAT16 support
- **Partition Support** - MBR partitioning
- **Shell** - Interactive command-line shell
- **Text Editor** - Built-in terminal editor
- **Custom Language** - GK interpreted language

## Roadmap

- [x] Dynamic Memory Management (Not fully yet)
- [x] Shell with basic command system
- [x] Storage management/Filesystem (Not fully yet)
- [ ] Internet Access
- [ ] Package Manager
- [ ] First Official Release

## TODO

- [ ] Add more shell commands
- [ ] Implement additional file operations
- [ ] Add user account system improvements
- [ ] Expand GK language features

## How to Contribute

Submit ideas or feedback:
- General feedback: [Google Forms](https://forms.gle/3aL4FJqvc8MZaJJv5)

Ways to contribute:
- Fix typos or improve wording
- Add new features
- Improve or optimize code
- Clean up code
- Tell others about the project

Apply contributions via:
- Fork the repo, make changes, and submit a PR

Add your name to the contributors list when you contribute!

## Building

### Prerequisites (Linux)

**Debian/Ubuntu:**
```bash
sudo apt install build-essential binutils nasm clang qemu-system-x86 grub
```

**Arch:**
```bash
sudo pacman -S base-devel binutils nasm clang qemu-system-x86 grub
```

### Build Commands

- `make all` - Produces `boot.iso` and run it
- `make run` - Builds and runs in QEMU

## Contact

- Email: elroylilly@gmail.com
- [Mastodon](https://mastodon.social/@GeckoOS)
- Join the community:
  - [Slack](https://join.slack.com/t/communtiyos/shared_invite/zt-3t3iltj77-5JTo~dkpCm44tZcyA0Or_w)
  - [Matrix](https://matrix.to/#/#geckoos-official:matrix.org)
  - [Stoat](https://stt.gg/Cv2wabFH)
  - [Fluxer](https://fluxer.gg/6K7A79x1)

## Screenshots

**This video is an old version of the actual OS!**
![2026-03-22 20-42-07](https://github.com/user-attachments/assets/4af57872-342f-45e0-9eb0-31b1033c9d54)
**This is the actual new version (unless there is a new version in the sources...)**
![2026-04-24 23-20-44](https://github.com/flixytss/Random-People-Coding-Stuff/blob/main/screenshots/1.png)

## Contributors

- **Ember2819** (Founder & Manager)
- Sifi11 (Founder)
- Crim (OG)
- CheeseFunnel23
- **bonk enjoyer/dorito girl** (Bootloader Creator (of the main branch))
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
- TheOtterMonarch
- codecrafter01001
- Pumpkicks
- DarkThemeGeek
- nfoxers
