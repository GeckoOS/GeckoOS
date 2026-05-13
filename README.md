# Gecko OS

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

- [x] Dynamic Memory Management
- [x] Shell with basic command system
- [x] Storage management/Filesystem
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
- General feedback: [Google Forms](https://docs.google.com/forms/d/e/1FAIpQLSe-euunJ8venpIXOD9_Luy_ZyDaszK5GU0efVQ_QOV7YDge9w/viewform?usp=publish-editor)

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
sudo apt install build-essential binutils nasm clang qemu-system-x86
```

**Arch:**
```bash
sudo pacman -S base-devel binutils nasm clang qemu-system-x86 ccache
```

### Build Commands

- `make all` - Produces `os.img`
- `make run` - Builds and runs in QEMU
  - When prompted for boot drive, press "1"

## Contact

- Email: elroylilly@gmail.com
- [Mastodon](https://mastodon.social/@GeckoOS)
- Join the community:
  - [Slack](https://join.slack.com/t/communtiyos/shared_invite/zt-3t3iltj77-5JTo~dkpCm44tZcyA0Or_w) (OLD)
  - [Matrix](https://matrix.to/#/#geckoos-official:matrix.org) <-- Join this

## Screenshots

![2026-03-22 20-42-07](https://github.com/user-attachments/assets/4af57872-342f-45e0-9eb0-31b1033c9d54)

## Contributors

- **Ember2819** (Founder & Manager)
- Sifi11 (Founder)
- Crim (OG)
- CheeseFunnel23
- **bonk enjoyer/dorito girl** (Bootloader Creator)
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
- Pumpkicks or flixytss
- DarkThemeGeek
- nfoxers
