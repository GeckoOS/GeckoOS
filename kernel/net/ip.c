#include <net/ip.h>
#include <net/net.h>
#include <net/arp.h>
#include <net/ethernet.h>
#include <net/icmp.h>
#include <mem.h>

uint16_t ip_checksum(void *data, uint16_t len) {
    uint32_t sum = 0;
    uint16_t *buf = (uint16_t *)data;

    while (len > 1) {
        sum += *buf++;
        len -= 2;
    }
    if (len == 1)
        sum += *(uint8_t *)buf;

    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    return ~sum;
}

void ip_send(uint32_t dst_ip, uint8_t protocol, const void *payload, uint16_t payload_len) {
    static uint8_t tx_buf[1514];
    uint16_t id = 0;

    ip_header_t *ip = (ip_header_t *)tx_buf;
    ip->version         = 4;
    ip->ihl             = 5;
    ip->dscp            = 0;
    ip->ecn             = 0;
    ip->total_length    = __builtin_bswap16(sizeof(ip_header_t) + payload_len);
    ip->identification  = __builtin_bswap16(id++);
    ip->flags_fragment  = __builtin_bswap16(IP_DF);
    ip->ttl             = 64;
    ip->protocol        = protocol;
    ip->checksum        = 0;
    ip->src_ip          = __builtin_bswap32(net_ip);
    ip->dst_ip          = __builtin_bswap32(dst_ip);

    memcpy(ip->payload, payload, payload_len);
    ip->checksum = ip_checksum(ip, sizeof(ip_header_t));

    uint8_t dst_mac[6];
    if (arp_resolve(dst_ip, dst_mac)) {
        net_send_frame(dst_mac, ETHER_TYPE_IPV4, tx_buf, sizeof(ip_header_t) + payload_len);
    } else {
        arp_request(dst_ip);
    }
}

void ip_handle(uint8_t *data, uint16_t len) {
    (void)len;
    ip_header_t *ip = (ip_header_t *)data;

    if (ip->version != 4) return;

    uint32_t dst_ip = __builtin_bswap32(ip->dst_ip);
    uint32_t src_ip = __builtin_bswap32(ip->src_ip);

    if (dst_ip != net_ip) return;

    uint16_t total_len = __builtin_bswap16(ip->total_length);
    uint16_t hdr_len   = ip->ihl * 4;
    uint16_t payload_len = total_len - hdr_len;

    switch (ip->protocol) {
    case IP_PROTO_ICMP:
        icmp_handle(src_ip, ip->payload, payload_len);
        break;
    }
}
