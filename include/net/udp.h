#pragma once

#include <stdint.h>

#define IP_PROTO_UDP 17
#define UDP_MAX_PAYLOAD 1472
#define UDP_REPLY_PAYLOAD_MAX 1472

// Transport definitions

typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum;
} __attribute__((packed)) udp_header_t;

typedef void (*udp_receive_handler_t)(
    uint32_t src_ip, uint16_t src_port,
    const uint8_t *payload, uint16_t payload_len
);

// A captured reply delivered to udp_request(). All fields are populated only
// when `received` is non-zero.
typedef struct {
    int      received;                       // 1 if a reply arrived before timeout
    uint32_t src_ip;                         // sender of the reply
    uint16_t src_port;                       // sender port of the reply
    uint16_t payload_len;                    // bytes copied into `payload`
    uint8_t  payload[UDP_REPLY_PAYLOAD_MAX]; // reply payload (truncated if larger)
} udp_reply_t;

// UDP Functions

int udp_bind(uint16_t port, udp_receive_handler_t handler);

// Removes the binding installed on `port` (if any). Safe to call when no
// handler is registered on the port.
void udp_unbind(uint16_t port);

void udp_handle(uint32_t src_ip, uint32_t dst_ip,
                const uint8_t *packet, uint16_t packet_len);

int udp_send(uint32_t dst_ip, uint16_t src_port, uint16_t dst_port,
            const void *payload, uint16_t payload_len);

// Sends a UDP datagram and registers a one-shot reply capture on src_port.
// After this returns 0, the caller should pump inbound packets (e.g. via
// e1000_receive()) and periodically poll udp_request_try_reply() until it
// returns 0 (got a reply) or the desired timeout elapses. The capture is
// cleared by udp_request_finish().
int udp_request_start(uint32_t dst_ip, uint16_t src_port, uint16_t dst_port,
                      const void *payload, uint16_t payload_len);

// Polls for a reply matching the outstanding udp_request_start(). If a reply
// is available, populates `out` (if non-NULL) and returns 0. Returns -1 if
// no reply has arrived yet. Returns -2 if there is no outstanding request.
int udp_request_try_reply(udp_reply_t *out);

// Clears any outstanding request state and unbinds the capture handler.
void udp_request_finish(void);