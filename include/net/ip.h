#pragma once

#include <stdint.h>

#define IP_PROTO_ICMP 1
#define IP_PROTO_TCP  6
#define IP_PROTO_UDP  17

#define IP_DF 0x4000

typedef struct {
    uint8_t  ihl:4;
    uint8_t  version:4;
    uint8_t  dscp:6;
    uint8_t  ecn:2;
    uint16_t total_length;
    uint16_t identification;
    uint16_t flags_fragment;
    uint8_t  ttl;
    uint8_t  protocol;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dst_ip;
    uint8_t  payload[];
} __attribute__((packed)) ip_header_t;

uint16_t ip_checksum(void *data, uint16_t len);
void ip_send(uint32_t dst_ip, uint8_t protocol, const void *payload, uint16_t payload_len);
void ip_handle(uint8_t *frame, uint16_t len);
