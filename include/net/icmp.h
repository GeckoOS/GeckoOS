#pragma once

#include <stdint.h>

#define ICMP_ECHO_REQUEST 8
#define ICMP_ECHO_REPLY   0

typedef struct {
    uint8_t  type;
    uint8_t  code;
    uint16_t checksum;
    uint16_t identifier;
    uint16_t sequence;
    uint8_t  payload[];
} __attribute__((packed)) icmp_header_t;

void icmp_handle(uint32_t src_ip, uint8_t *data, uint16_t len);
void icmp_send_echo_request(uint32_t dst_ip, uint16_t id, uint16_t seq);
int  icmp_got_reply(void);
void icmp_clear_reply(void);
