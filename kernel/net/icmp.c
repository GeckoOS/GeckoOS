#include <net/icmp.h>
#include <net/ip.h>
#include <net/net.h>
#include <mem.h>
#include "terminal/printf.h"

static volatile int reply_received = 0;

int icmp_got_reply(void) {
    return reply_received;
}

void icmp_clear_reply(void) {
    reply_received = 0;
}

static uint16_t icmp_checksum(void *data, uint16_t len) {
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

void icmp_handle(uint32_t src_ip, uint8_t *data, uint16_t len) {
    icmp_header_t *icmp = (icmp_header_t *)data;

    if (icmp->type == ICMP_ECHO_REQUEST) {
        uint16_t payload_len = len - sizeof(icmp_header_t);
        static uint8_t reply_buf[1024];

        icmp_header_t *reply = (icmp_header_t *)reply_buf;
        reply->type       = ICMP_ECHO_REPLY;
        reply->code       = 0;
        reply->checksum   = 0;
        reply->identifier = icmp->identifier;
        reply->sequence   = icmp->sequence;
        memcpy(reply->payload, icmp->payload, payload_len);
        reply->checksum = icmp_checksum(reply, sizeof(icmp_header_t) + payload_len);

        ip_send(src_ip, IP_PROTO_ICMP, reply, sizeof(icmp_header_t) + payload_len);
    } else if (icmp->type == ICMP_ECHO_REPLY) {
        reply_received = 1;
    }
}

void icmp_send_echo_request(uint32_t dst_ip, uint16_t id, uint16_t seq) {
    static uint8_t buf[128];
    const char *msg = "GeckoOS ping";
    uint16_t payload_len = 12;

    icmp_header_t *icmp = (icmp_header_t *)buf;
    icmp->type       = ICMP_ECHO_REQUEST;
    icmp->code       = 0;
    icmp->checksum   = 0;
    icmp->identifier = __builtin_bswap16(id);
    icmp->sequence   = __builtin_bswap16(seq);
    memcpy(icmp->payload, msg, payload_len);
    icmp->checksum = icmp_checksum(icmp, sizeof(icmp_header_t) + payload_len);

    ip_send(dst_ip, IP_PROTO_ICMP, icmp, sizeof(icmp_header_t) + payload_len);
}
