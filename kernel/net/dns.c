#include <net/dns.h>
#include <net/ip.h>
#include <net/udp.h>

// Minimal memcpy implementation
#include <stdint.h>
static void *memcpy(void *dst, const void *src, uint32_t n) {
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    while (n--) *d++ = *s++;
    return dst;
}

// Forward declarations for functions/macros that may not be visible here.
static inline uint32_t IP(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    return ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)c << 8) | (uint32_t)d;
}

uint16_t hostToNetShort(uint16_t val) {
    return (uint16_t)((val << 8) | (val >> 8));
}

void encode_dns_name(const char *name, uint8_t *out, uint16_t *out_len) {
      uint16_t pos = 0;
      const char *label = name;

      while (*label != '\0') {
          uint16_t label_len = 0;
          while (label[label_len] != '.' && label[label_len] != '\0')
              label_len++;

          if (label_len == 0 || label_len > 63)
              return;  // invalid name

          out[pos++] = (uint8_t)label_len;

          for (uint16_t i = 0; i < label_len; i++)
              out[pos++] = (uint8_t)label[i];

          label += label_len;
          if (*label == '.')
              label++;
      }

      out[pos++] = 0;  // root-label terminator
      *out_len = pos;
  }


void send_dns(const char *name) {
    uint8_t query[512];
    uint16_t pos = 0;

    DnsHeader header = {0};
    header.id      = hostToNetShort(0x1234);
    header.flags   = hostToNetShort(DNS_FLAG_RECURSION_DESIRED);
    header.qdCount = hostToNetShort(1);

    memcpy(query + pos, &header, sizeof(header));
    pos += sizeof(header);

    uint16_t nameLen;
    encode_dns_name(name, query + pos, &nameLen);
    pos += nameLen;

    uint16_t qtype = hostToNetShort(DNS_TYPE_A);
    uint16_t qclass = hostToNetShort(DNS_CLASS_IN);
    memcpy(query + pos, &qtype, sizeof(qtype));
    pos += sizeof(qtype);
    memcpy(query + pos, &qclass, sizeof(qclass));
    pos += sizeof(qclass);

    udp_request_start(IP(8, 8, 4, 4), 49152, 53, query, pos);
}

static uint16_t dns_read16(const uint8_t *p) {
    return (uint16_t)((p[0] << 8) | p[1]);
}

/*
* Skips one name inside a DNS message, handling length-prefixed labels and
* compression pointers (RFC 1035 4.1.4). Advances *pos past the name.
*/
static int dns_skip_name(const uint8_t *msg, uint16_t len, uint16_t *pos) {
    uint16_t p = *pos;

    for (;;) {
        if (p >= len)
            return -1;
        uint8_t label = msg[p];

        if (label == 0) {
            p++;
            break;
        }
        if ((label & 0xC0) == 0xC0) {
            p += 2;
            break;
        }
        if ((label & 0xC0) != 0)
            return -1;

        p += 1 + label;
        if (p > len)
            return -1;
    }

    *pos = p;
    return 0;
}

/*
* Returns the response code (rcode) from a DNS reply, or -1 if the message
* is too short to contain a header.
*/
int dns_reply_rcode(const uint8_t *reply, uint16_t len) {
    if (len < sizeof(DnsHeader))
        return -1;
    return dns_read16(reply + 2) & 0x000F;
}

/*
* Parses a DNS reply message and returns the first A record address in host
* byte order. Returns 0 on success, -1 if the message is malformed or
* contains no usable A record.
*/
int dns_parse_reply(const uint8_t *reply, uint16_t len, uint32_t *out_ip) {
    if (len < sizeof(DnsHeader))
        return -1;

    uint16_t flags   = dns_read16(reply + 2);
    uint16_t qdCount = dns_read16(reply + 4);
    uint16_t anCount = dns_read16(reply + 6);

    if ((flags & 0x8000) == 0)      // must be a response
        return -1;
    if ((flags & 0x000F) != 0)      // rcode must be NOERROR
        return -1;

    uint16_t pos = sizeof(DnsHeader);

    // Skip the question section (the name, plus qtype/qclass).
    for (uint16_t q = 0; q < qdCount; q++) {
        if (dns_skip_name(reply, len, &pos) != 0)
            return -1;
        pos += 4;
        if (pos > len)
            return -1;
    }

    // Walk the answer records; names here can also be compressed.
    for (uint16_t a = 0; a < anCount; a++) {
        if (dns_skip_name(reply, len, &pos) != 0)
            return -1;
        if (pos + 10 > len)
            return -1;

        uint16_t type     = dns_read16(reply + pos);
        uint16_t rdlength = dns_read16(reply + pos + 8);
        pos += 10;

        if (type == DNS_TYPE_A && rdlength == 4) {
            if (pos + 4 > len)
                return -1;
            *out_ip = ((uint32_t)reply[pos] << 24) |
                      ((uint32_t)reply[pos + 1] << 16) |
                      ((uint32_t)reply[pos + 2] << 8) |
                      reply[pos + 3];
            return 0;
        }

        pos += rdlength;
        if (pos > len)
            return -1;
    }

    return -1;
}