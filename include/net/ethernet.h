#pragma once

#include <stdint.h>

#define ETHER_TYPE_ARP  0x0806
#define ETHER_TYPE_IPV4 0x0800

#define MAC_BROADCAST   ((uint8_t[6]){0xFF,0xFF,0xFF,0xFF,0xFF,0xFF})

typedef struct {
    uint8_t  dst[6];
    uint8_t  src[6];
    uint16_t ether_type;
    uint8_t  payload[];
} __attribute__((packed)) eth_frame_t;
