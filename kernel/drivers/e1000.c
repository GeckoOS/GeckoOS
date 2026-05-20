#include <drivers/e1000.h>
#include <drivers/pci.h>
#include <drivers/tables/irq.h>
#include <mem/paging.h>
#include <mem/physical_mem.h>
#include <mem.h>
#include "terminal/printf.h"

static volatile uint32_t *mmio_base = NULL;
static uint8_t mac[6];

static e1000_rx_desc_t *rx_descs_phys = NULL;
static e1000_tx_desc_t *tx_descs_phys = NULL;
static uint8_t         *rx_bufs_phys  = NULL;
static uint8_t         *tx_buf_phys   = NULL;

static e1000_rx_desc_t *rx_descs = NULL;
static e1000_tx_desc_t *tx_descs = NULL;
static uint8_t         *rx_bufs  = NULL;
static uint8_t         *tx_buf   = NULL;

static uint32_t rx_tail = 0;
static uint32_t tx_tail = 0;

static uint32_t e1000_read(uint32_t reg) {
    return mmio_base[reg / 4];
}

static void e1000_write(uint32_t reg, uint32_t val) {
    mmio_base[reg / 4] = val;
}

static uint32_t eeprom_read(uint8_t addr) {
    e1000_write(E1000_EERD, (1u) | ((uint32_t)addr << 8));
    uint32_t v;
    do { v = e1000_read(E1000_EERD); } while (!(v & (1u << 4)));
    return (v >> 16) & 0xFFFF;
}

static void read_mac(void) {
    uint32_t lo = eeprom_read(0);
    uint32_t mid = eeprom_read(1);
    uint32_t hi = eeprom_read(2);
    mac[0] = lo & 0xFF;
    mac[1] = (lo >> 8) & 0xFF;
    mac[2] = mid & 0xFF;
    mac[3] = (mid >> 8) & 0xFF;
    mac[4] = hi & 0xFF;
    mac[5] = (hi >> 8) & 0xFF;
}

static void rx_init(void) {
    for (int i = 0; i < E1000_NUM_RX_DESC; i++) {
        uint64_t buf_phys = (uint64_t)(uintptr_t)(rx_bufs_phys + i * E1000_RX_BUF_SIZE);
        rx_descs[i].addr   = buf_phys;
        rx_descs[i].status = 0;
    }

    uint64_t base = (uint64_t)(uintptr_t)rx_descs_phys;
    e1000_write(E1000_RDBAL, (uint32_t)(base & 0xFFFFFFFF));
    e1000_write(E1000_RDBAH, (uint32_t)(base >> 32));
    e1000_write(E1000_RDLEN, E1000_NUM_RX_DESC * sizeof(e1000_rx_desc_t));
    e1000_write(E1000_RDH, 0);
    e1000_write(E1000_RDT, E1000_NUM_RX_DESC - 1);
    rx_tail = 0;

    e1000_write(E1000_RCTL,
        E1000_RCTL_EN  |
        E1000_RCTL_BAM |
        E1000_RCTL_BSIZE_2048 |
        E1000_RCTL_SECRC);
}

static void tx_init(void) {
    for (int i = 0; i < E1000_NUM_TX_DESC; i++) {
        tx_descs[i].addr   = 0;
        tx_descs[i].status = E1000_TXD_STAT_DD;
    }

    uint64_t base = (uint64_t)(uintptr_t)tx_descs_phys;
    e1000_write(E1000_TDBAL, (uint32_t)(base & 0xFFFFFFFF));
    e1000_write(E1000_TDBAH, (uint32_t)(base >> 32));
    e1000_write(E1000_TDLEN, E1000_NUM_TX_DESC * sizeof(e1000_tx_desc_t));
    e1000_write(E1000_TDH, 0);
    e1000_write(E1000_TDT, 0);
    tx_tail = 0;

    e1000_write(E1000_TCTL,
        E1000_TCTL_EN  |
        E1000_TCTL_PSP |
        (15u << E1000_TCTL_CT_SHIFT)   |
        (63u << E1000_TCTL_COLD_SHIFT));

    e1000_write(E1000_TIPG, 0x0060200A);
}

static volatile int rx_pending = 0;

static void e1000_irq_handler(registers_t *regs) {
    (void)regs;
    uint32_t icr = e1000_read(E1000_ICR);

    if (icr & E1000_ICR_LSC) {
        uint32_t status = e1000_read(E1000_STATUS);
        printf("e1000: link %s\n", (status & 2) ? "up" : "down");
    }

    if (icr & (E1000_ICR_RXT0 | E1000_ICR_RXO)) {
        rx_pending = 1;
    }
}

bool e1000_init(uint8_t bus, uint8_t slot, uint8_t func) {
    uint32_t bar0 = pci_readl(bus, slot, func, 0x10) & ~0xFu;
    if (!bar0) {
        printf("e1000: BAR0 is zero\n");
        return false;
    }

    uint64_t virt = mmio_map((uint64_t)bar0, 0x20000);
    if (!virt) {
        printf("e1000: mmio_map failed\n");
        return false;
    }
    mmio_base = (volatile uint32_t *)virt;

    uint16_t cmd = pci_readw(bus, slot, func, 0x04);
    pci_writew(bus, slot, func, 0x04, cmd | (1u << 1) | (1u << 2));

    /* reset */
    e1000_write(E1000_CTRL, e1000_read(E1000_CTRL) | E1000_CTRL_RST);
    for (volatile int i = 0; i < 100000; i++);
    if (e1000_read(E1000_CTRL) & E1000_CTRL_RST) {
        printf("e1000: reset timed out\n");
        return false;
    }

    e1000_write(E1000_CTRL, e1000_read(E1000_CTRL) | E1000_CTRL_SLU | E1000_CTRL_ASDE);

    /* clear multicast table */
    for (int i = 0; i < 128; i++)
        e1000_write(E1000_MTA + i * 4, 0);

    rx_descs_phys = (e1000_rx_desc_t *)allocate_blocks(1);
    tx_descs_phys = (e1000_tx_desc_t *)allocate_blocks(1);
    rx_bufs_phys  = (uint8_t *)allocate_blocks(E1000_NUM_RX_DESC);
    tx_buf_phys   = (uint8_t *)allocate_blocks(1);

    if (!rx_descs_phys || !tx_descs_phys || !rx_bufs_phys || !tx_buf_phys) {
        printf("e1000: failed to allocate DMA buffers\n");
        return false;
    }

    /* First 16 MB is identity-mapped, so virt == phys */
    rx_descs = rx_descs_phys;
    tx_descs = tx_descs_phys;
    rx_bufs  = rx_bufs_phys;
    tx_buf   = tx_buf_phys;

    read_mac();
    printf("e1000: MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    /* write MAC into receive address register 0 */
    e1000_write(E1000_RAL,
        (uint32_t)mac[0] |
        (uint32_t)mac[1] << 8  |
        (uint32_t)mac[2] << 16 |
        (uint32_t)mac[3] << 24);
    e1000_write(E1000_RAH,
        (uint32_t)mac[4] |
        (uint32_t)mac[5] << 8  |
        E1000_RAH_AV);

    rx_init();
    tx_init();

    /* unmask TX done, RX timer, link change, RX overrun */
    e1000_write(E1000_IMS,
        E1000_ICR_TXDW |
        E1000_ICR_RXT0 |
        E1000_ICR_LSC  |
        E1000_ICR_RXO);

    uint8_t irq_line = pci_readb(bus, slot, func, 0x3C);
    irq_install_handler(irq_line, e1000_irq_handler);
    printf("e1000: init done, IRQ %d\n", irq_line);

    return true;
}

int e1000_send(const void *data, uint16_t len) {
    if (!mmio_base) return -1;
    if (len > E1000_TX_BUF_SIZE) return -1;

    /* wait for descriptor to be free */
    int timeout = 100000;
    while (!(tx_descs[tx_tail].status & E1000_TXD_STAT_DD)) {
        if (--timeout == 0) return -1;
    }

    /* Copy data into physical TX buffer */
    memcpy(tx_buf, data, len);

    uint64_t buf_phys = (uint64_t)(uintptr_t)tx_buf;
    tx_descs[tx_tail].addr   = buf_phys;
    tx_descs[tx_tail].length = len;
    tx_descs[tx_tail].cmd    = E1000_TXD_CMD_EOP | E1000_TXD_CMD_IFCS | E1000_TXD_CMD_RS;
    tx_descs[tx_tail].status = 0;

    tx_tail = (tx_tail + 1) % E1000_NUM_TX_DESC;
    e1000_write(E1000_TDT, tx_tail);

    return 0;
}

void e1000_get_mac(uint8_t out[6]) {
    for (int i = 0; i < 6; i++)
        out[i] = mac[i];
}

int e1000_receive(uint8_t *buf, uint16_t *len) {
    if (!mmio_base) return -1;

    if (rx_descs[rx_tail].status & E1000_RXD_STAT_DD) {
        *len = rx_descs[rx_tail].length;
        uint8_t *src = rx_bufs + rx_tail * E1000_RX_BUF_SIZE;
        for (uint16_t i = 0; i < *len; i++)
            buf[i] = src[i];

        rx_descs[rx_tail].status = 0;
        e1000_write(E1000_RDT, rx_tail);
        rx_tail = (rx_tail + 1) % E1000_NUM_RX_DESC;
        return 0;
    }
    return -1;
}