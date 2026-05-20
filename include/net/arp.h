#pragma once

#include <stdint.h>

#define ARP_HWTYPE_ETHER 1
#define ARP_PROTYPE_IPV4 0x0800
#define ARP_OP_REQUEST   1
#define ARP_OP_REPLY     2

typedef struct {
    uint16_t hw_type;
    uint16_t proto_type;
    uint8_t  hw_size;
    uint8_t  proto_size;
    uint16_t op;
    uint8_t  sender_mac[6];
    uint32_t sender_ip;
    uint8_t  target_mac[6];
    uint32_t target_ip;
} __attribute__((packed)) arp_packet_t;

void arp_init(void);
void arp_handle(uint8_t *frame, uint16_t len);
void arp_request(uint32_t target_ip);
int arp_resolve(uint32_t ip, uint8_t out_mac[6]);
