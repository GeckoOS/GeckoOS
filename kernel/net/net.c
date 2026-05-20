#include <net/net.h>
#include <net/ethernet.h>
#include <net/arp.h>
#include <net/ip.h>
#include <drivers/e1000.h>
#include <mem.h>

uint32_t net_ip;
uint32_t net_gateway;
uint32_t net_subnet;
uint8_t  net_mac[6];

static uint8_t tx_buf[2048];

void net_init(void) {
    net_ip      = IP(10, 0, 2, 15);
    net_gateway = IP(10, 0, 2, 2);
    net_subnet  = IP(255, 255, 255, 0);
    e1000_get_mac(net_mac);
}

void net_send_frame(uint8_t dst_mac[6], uint16_t ether_type, const void *payload, uint16_t len) {
    eth_frame_t *frame = (eth_frame_t *)tx_buf;
    for (int i = 0; i < 6; i++) {
        frame->dst[i] = dst_mac[i];
        frame->src[i] = net_mac[i];
    }
    frame->ether_type = __builtin_bswap16(ether_type);
    memcpy(frame->payload, payload, len);
    e1000_send(frame, sizeof(eth_frame_t) + len);
}

void net_handle_packet(uint8_t *data, uint16_t len) {
    if (len < sizeof(eth_frame_t)) return;

    eth_frame_t *frame = (eth_frame_t *)data;
    uint16_t type = __builtin_bswap16(frame->ether_type);

    int for_us = 1;
    for (int i = 0; i < 6; i++) {
        if (frame->dst[i] != net_mac[i] && frame->dst[i] != 0xFF) {
            for_us = 0;
            break;
        }
    }
    if (!for_us) return;

    switch (type) {
    case ETHER_TYPE_ARP:
        arp_handle(frame->payload, len - sizeof(eth_frame_t));
        break;
    case ETHER_TYPE_IPV4:
        ip_handle(frame->payload, len - sizeof(eth_frame_t));
        break;
    }
}