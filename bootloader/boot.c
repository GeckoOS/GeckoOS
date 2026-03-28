
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void outw(uint16_t port, uint16_t val) {
    asm volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret; 
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

struct mbr_partition_t
{
	uint8_t flags;
	uint32_t chr_start: 24;
	uint8_t type;
	uint32_t chr_end: 24;
	uint32_t lba;
	uint32_t size;
};
typedef struct mbr_partition_t mbr_t[4];

void _entry()
{
	uint16_t *mem = (void*)0x100000;
	void (*jmp)(void) = (void*)0x100000;
	// read kernel
	while (inb(0x1F7) & 0x80);
	outb(0x1F6, 0xE0);
	outb(0x1F2, 0); //256 sectors
	outb(0x1F3, 16); //8kb is bootloader
	outb(0x1F4, 0);
	outb(0x1F5, 0);
	outb(0x1F7, 0x20);
	while (inb(0x1F7) & 0x80);
	for (int s = 0; s < 256; s++)
	{
		while (!(inb(0x1F7) & 0x08));
		for (int i = 0; i < 256; i++)
		{
			mem[s*256+i] = inw(0x1F0);
		}
	}
	jmp();
}
