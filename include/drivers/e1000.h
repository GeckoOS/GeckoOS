#pragma once

#include <stdint.h>
#include <stdbool.h>

/*
 * e1000 register offsets
 */
#define E1000_CTRL      0x0000
#define E1000_STATUS    0x0008
#define E1000_EECD      0x0010
#define E1000_EERD      0x0014
#define E1000_ICR       0x00C0
#define E1000_IMS       0x00D0
#define E1000_IMC       0x00D8
#define E1000_RCTL      0x0100
#define E1000_TCTL      0x0400
#define E1000_TIPG      0x0410
#define E1000_RDBAL     0x2800
#define E1000_RDBAH     0x2804
#define E1000_RDLEN     0x2808
#define E1000_RDH       0x2810
#define E1000_RDT       0x2818
#define E1000_TDBAL     0x3800
#define E1000_TDBAH     0x3804
#define E1000_TDLEN     0x3808
#define E1000_TDH       0x3810
#define E1000_TDT       0x3818
#define E1000_MTA       0x5200
#define E1000_RAL       0x5400
#define E1000_RAH       0x5404

/* CTRL bits */
#define E1000_CTRL_RST      (1u << 26)
#define E1000_CTRL_SLU      (1u << 6)
#define E1000_CTRL_ASDE     (1u << 5)

/* RCTL bits */
#define E1000_RCTL_EN       (1u << 1)
#define E1000_RCTL_SBP      (1u << 2)
#define E1000_RCTL_UPE      (1u << 3)
#define E1000_RCTL_MPE      (1u << 4)
#define E1000_RCTL_LPE      (1u << 5)
#define E1000_RCTL_BAM      (1u << 15)
#define E1000_RCTL_BSIZE_2048 0
#define E1000_RCTL_SECRC    (1u << 26)

/* TCTL bits */
#define E1000_TCTL_EN       (1u << 1)
#define E1000_TCTL_PSP      (1u << 3)
#define E1000_TCTL_CT_SHIFT 4
#define E1000_TCTL_COLD_SHIFT 12

/* ICR/IMS bits */
#define E1000_ICR_TXDW      (1u << 0)
#define E1000_ICR_TXQE      (1u << 1)
#define E1000_ICR_LSC       (1u << 2)
#define E1000_ICR_RXO       (1u << 6)
#define E1000_ICR_RXT0      (1u << 7)

/* RAH valid bit */
#define E1000_RAH_AV        (1u << 31)

/* TX descriptor command bits */
#define E1000_TXD_CMD_EOP   (1u << 0)
#define E1000_TXD_CMD_IFCS  (1u << 1)
#define E1000_TXD_CMD_RS    (1u << 3)

/* TX descriptor status bits */
#define E1000_TXD_STAT_DD   (1u << 0)

/* RX descriptor status bits */
#define E1000_RXD_STAT_DD   (1u << 0)
#define E1000_RXD_STAT_EOP  (1u << 1)

#define E1000_NUM_RX_DESC   32
#define E1000_NUM_TX_DESC   8
#define E1000_RX_BUF_SIZE   2048
#define E1000_TX_BUF_SIZE   2048

typedef struct {
    uint64_t addr;
    uint16_t length;
    uint16_t checksum;
    uint8_t  status;
    uint8_t  errors;
    uint16_t special;
} __attribute__((packed)) e1000_rx_desc_t;

typedef struct {
    uint64_t addr;
    uint16_t length;
    uint8_t  cso;
    uint8_t  cmd;
    uint8_t  status;
    uint8_t  css;
    uint16_t special;
} __attribute__((packed)) e1000_tx_desc_t;

bool e1000_init(uint8_t bus, uint8_t slot, uint8_t func);
int  e1000_send(const void *data, uint16_t len);
void e1000_get_mac(uint8_t mac[6]);
int  e1000_receive(uint8_t *buf, uint16_t *len);