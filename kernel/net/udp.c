#include <net/udp.h>
#include <net/ip.h>
#include <net/net.h>
#include <mem.h>

#define UDP_MAX_HANDLERS 16

typedef struct {
    uint16_t port;
    udp_receive_handler_t handler;
} udp_binding_t;

static udp_binding_t bindings[UDP_MAX_HANDLERS];

// Reply-capture state used by udp_request() and its internal handler. Only
// one outstanding request is supported at a time; udp_request() installs the
// capture handler on src_port for the duration of the call and clears it on
// return.
static volatile int      s_reply_ready = 0;
static volatile uint32_t s_reply_src_ip = 0;
static volatile uint16_t s_reply_src_port = 0;
static volatile uint16_t s_reply_len = 0;
static uint8_t           s_reply_buf[UDP_REPLY_PAYLOAD_MAX];
static volatile uint16_t s_reply_listen_port = 0;

static uint16_t ntohs(uint16_t value) {
    return __builtin_bswap16(value);
}

static uint16_t htons(uint16_t value) {
    return __builtin_bswap16(value);
}

/* Adds network-order bytes using Internet one's-complement arithmetic. */
static uint32_t checksum_add(uint32_t sum, const uint8_t *data, uint16_t len) {
    while (len >= 2) {
        sum += ((uint16_t)data[0] << 8) | data[1];
        data += 2;
        len -= 2;
    }

    if (len != 0)
        sum += (uint16_t)data[0] << 8;

    return sum;
}

/*
* With checksum set to zero: returns the checksum to transmit.
* With a received checksum included: returns zero when valid.
*/
static uint16_t udp_checksum(uint32_t src_ip, uint32_t dst_ip,
                            const uint8_t *udp, uint16_t udp_len) {
    uint8_t pseudo_header[12] = {
        (uint8_t)(src_ip >> 24),
        (uint8_t)(src_ip >> 16),
        (uint8_t)(src_ip >> 8),
        (uint8_t)src_ip,
        (uint8_t)(dst_ip >> 24),
        (uint8_t)(dst_ip >> 16),
        (uint8_t)(dst_ip >> 8),
        (uint8_t)dst_ip,
        0,
        IP_PROTO_UDP,
        (uint8_t)(udp_len >> 8),
        (uint8_t)udp_len
    };

    uint32_t sum = 0;
    sum = checksum_add(sum, pseudo_header, sizeof(pseudo_header));
    sum = checksum_add(sum, udp, udp_len);

    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    return (uint16_t)~sum;
}

int udp_bind(uint16_t port, udp_receive_handler_t handler) {
    for (int i = 0; i < UDP_MAX_HANDLERS; i++) {
        if (bindings[i].handler != 0 && bindings[i].port == port) {
            bindings[i].handler = handler;
            return 0;
        }
    }

    for (int i = 0; i < UDP_MAX_HANDLERS; i++) {
        if (bindings[i].handler == 0) {
            bindings[i].port = port;
            bindings[i].handler = handler;
            return 0;
        }
    }

    return -1;
}

void udp_handle(uint32_t src_ip, uint32_t dst_ip,
                const uint8_t *packet, uint16_t packet_len) {
    if (packet_len < sizeof(udp_header_t))
        return;

    const udp_header_t *udp = (const udp_header_t *)packet;
    uint16_t udp_len = ntohs(udp->length);

    /* Reject a sender-claimed size that cannot exist in our receive buffer. */
    if (udp_len < sizeof(udp_header_t) || udp_len > packet_len)
        return;

    /*
    * IPv4 allows UDP checksum zero to mean "not provided".
    * A nonzero transmitted checksum must validate.
    */
    if (udp->checksum != 0 && udp_checksum(src_ip, dst_ip, packet, udp_len) != 0)
        return;

    uint16_t dst_port = ntohs(udp->dst_port);
    uint16_t src_port = ntohs(udp->src_port);
    const uint8_t *payload = packet + sizeof(udp_header_t);
    uint16_t payload_len = udp_len - sizeof(udp_header_t);

    for (int i = 0; i < UDP_MAX_HANDLERS; i++) {
        if (bindings[i].handler != 0 && bindings[i].port == dst_port) {
            bindings[i].handler(src_ip, src_port, payload, payload_len);
            return;
        }
    }
}

int udp_send(uint32_t dst_ip, uint16_t src_port, uint16_t dst_port,
            const void *payload, uint16_t payload_len) {
    static uint8_t packet[sizeof(udp_header_t) + UDP_MAX_PAYLOAD];

    if (payload_len > UDP_MAX_PAYLOAD)
        return -1;

    udp_header_t *udp = (udp_header_t *)packet;
    uint16_t udp_len = sizeof(udp_header_t) + payload_len;

    udp->src_port = htons(src_port);
    udp->dst_port = htons(dst_port);
    udp->length = htons(udp_len);
    udp->checksum = 0;

    memcpy(packet + sizeof(udp_header_t), payload, payload_len);

    uint16_t checksum = udp_checksum(net_ip, dst_ip, packet, udp_len);

    /* A computed zero is encoded as all ones for UDP over IPv4. */
    if (checksum == 0)
        checksum = 0xFFFF;

    udp->checksum = htons(checksum);

    ip_send(dst_ip, IP_PROTO_UDP, packet, udp_len);
    return 0;
}

void udp_unbind(uint16_t port) {
    for (int i = 0; i < UDP_MAX_HANDLERS; i++) {
        if (bindings[i].port == port) {
            bindings[i].port = 0;
            bindings[i].handler = 0;
            return;
        }
    }
}

// Internal handler installed by udp_request() on its src_port. Copies the
// first matching datagram into the static reply buffer and sets the ready
// flag so udp_request() can return it.
static void udp_capture_reply(uint32_t src_ip, uint16_t src_port,
                              const uint8_t *payload, uint16_t payload_len) {
    s_reply_src_ip = src_ip;
    s_reply_src_port = src_port;
    uint16_t n = payload_len;
    if (n > sizeof(s_reply_buf))
        n = sizeof(s_reply_buf);
    for (uint16_t k = 0; k < n; k++)
        s_reply_buf[k] = payload[k];
    s_reply_len = n;
    s_reply_ready = 1;
}

int udp_request_start(uint32_t dst_ip, uint16_t src_port, uint16_t dst_port,
                      const void *payload, uint16_t payload_len) {
    s_reply_ready = 0;
    s_reply_len = 0;
    s_reply_listen_port = src_port;

    if (udp_bind(src_port, udp_capture_reply) != 0) {
        s_reply_listen_port = 0;
        return -1;
    }

    int rc = udp_send(dst_ip, src_port, dst_port, payload, payload_len);
    if (rc != 0) {
        udp_unbind(src_port);
        s_reply_listen_port = 0;
        return -1;
    }
    return 0;
}

int udp_request_try_reply(udp_reply_t *out) {
    if (s_reply_listen_port == 0)
        return -2;
    if (!s_reply_ready)
        return -1;

    if (out) {
        out->received = 1;
        out->src_ip = s_reply_src_ip;
        out->src_port = s_reply_src_port;
        out->payload_len = s_reply_len;
        for (uint16_t k = 0; k < s_reply_len; k++)
            out->payload[k] = s_reply_buf[k];
    }
    return 0;
}

void udp_request_finish(void) {
    if (s_reply_listen_port != 0) {
        udp_unbind(s_reply_listen_port);
        s_reply_listen_port = 0;
    }
    s_reply_ready = 0;
    s_reply_len = 0;
}