#pragma once

#include <stdint.h>

#define IP(a,b,c,d) ((uint32_t)(((a)&0xFF)<<24|((b)&0xFF)<<16|((c)&0xFF)<<8|((d)&0xFF)))

extern uint32_t net_ip;
extern uint32_t net_gateway;
extern uint32_t net_subnet;
extern uint8_t  net_mac[6];

void net_init(void);
void net_handle_packet(uint8_t *data, uint16_t len);
void net_send_frame(uint8_t dst_mac[6], uint16_t ether_type, const void *payload, uint16_t len);
