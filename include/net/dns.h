#pragma once

#include <stdint.h>

#pragma pack(push, 1)
typedef struct {
    uint16_t id;
    uint16_t flags;
    uint16_t qdCount;
    uint16_t anCount;
    uint16_t nsCount;
    uint16_t arCount;
} DnsHeader;
#pragma pack(pop)

#define DNS_FLAG_RECURSION_DESIRED 0x0100
#define DNS_TYPE_A  0x0001
#define DNS_CLASS_IN 0x0001
#define DNS_PORT 53

uint16_t hostToNetShort(uint16_t val);
void send_dns(const char *name);
int dns_parse_reply(const uint8_t *reply, uint16_t len, uint32_t *out_ip);
int dns_reply_rcode(const uint8_t *reply, uint16_t len);