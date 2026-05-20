#include <net/arp.h>
#include <net/net.h>
#include <net/ethernet.h>
#include <mem.h>

#define ARP_CACHE_SIZE 16

typedef struct {
    uint32_t ip;
    uint8_t  mac[6];
    int      valid;
} arp_entry_t;

static arp_entry_t arp_cache[ARP_CACHE_SIZE];

void arp_init(void) {
    for (int i = 0; i < ARP_CACHE_SIZE; i++)
        arp_cache[i].valid = 0;
}

void arp_handle(uint8_t *data, uint16_t len) {
    (void)len;
    arp_packet_t *arp = (arp_packet_t *)data;

    uint16_t op = __builtin_bswap16(arp->op);
    uint32_t target_ip = __builtin_bswap32(arp->target_ip);
    uint32_t sender_ip = __builtin_bswap32(arp->sender_ip);

    if (op == ARP_OP_REQUEST && target_ip == net_ip) {
        arp_packet_t reply;
        reply.hw_type    = __builtin_bswap16(ARP_HWTYPE_ETHER);
        reply.proto_type = __builtin_bswap16(ARP_PROTYPE_IPV4);
        reply.hw_size    = 6;
        reply.proto_size = 4;
        reply.op         = __builtin_bswap16(ARP_OP_REPLY);

        for (int i = 0; i < 6; i++) {
            reply.sender_mac[i] = net_mac[i];
            reply.target_mac[i] = arp->sender_mac[i];
        }
        reply.sender_ip = __builtin_bswap32(net_ip);
        reply.target_ip = __builtin_bswap32(arp->sender_ip);

        net_send_frame(arp->sender_mac, ETHER_TYPE_ARP, &reply, sizeof(arp_packet_t));
    }

    if (op == ARP_OP_REQUEST || op == ARP_OP_REPLY) {
        for (int i = 0; i < ARP_CACHE_SIZE; i++) {
            if (arp_cache[i].valid && arp_cache[i].ip == sender_ip) {
                memcpy(arp_cache[i].mac, arp->sender_mac, 6);
                return;
            }
        }
        for (int i = 0; i < ARP_CACHE_SIZE; i++) {
            if (!arp_cache[i].valid) {
                arp_cache[i].ip = sender_ip;
                memcpy(arp_cache[i].mac, arp->sender_mac, 6);
                arp_cache[i].valid = 1;
                return;
            }
        }
    }
}

void arp_request(uint32_t target_ip) {
    arp_packet_t req;
    req.hw_type    = __builtin_bswap16(ARP_HWTYPE_ETHER);
    req.proto_type = __builtin_bswap16(ARP_PROTYPE_IPV4);
    req.hw_size    = 6;
    req.proto_size = 4;
    req.op         = __builtin_bswap16(ARP_OP_REQUEST);

    for (int i = 0; i < 6; i++) {
        req.sender_mac[i] = net_mac[i];
        req.target_mac[i] = 0;
    }
    req.sender_ip = __builtin_bswap32(net_ip);
    req.target_ip = __builtin_bswap32(target_ip);

    net_send_frame(MAC_BROADCAST, ETHER_TYPE_ARP, &req, sizeof(arp_packet_t));
}

int arp_resolve(uint32_t ip, uint8_t out_mac[6]) {
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].valid && arp_cache[i].ip == ip) {
            memcpy(out_mac, arp_cache[i].mac, 6);
            return 1;
        }
    }
    return 0;
}
