/*
 * This work was authored by Two Six Labs, LLC and is sponsored by a subcontract
 * agreement with Galois, Inc.  This material is based upon work supported by
 * the Defense Advanced Research Projects Agency (DARPA) under Contract No.
 * HR0011-19-C-0103.
 *
 * The Government has unlimited rights to use, modify, reproduce, release,
 * perform, display, or disclose computer software or computer software
 * documentation marked with this legend. Any reproduction of technical data,
 * computer software, or portions thereof marked with this legend must also
 * reproduce this marking.
 *
 * Copyright 2020 Two Six Labs, LLC.  All rights reserved.
 */

/*
 * ASSUMPTIONS
 * 
 * The metadata stream has MPEG-TS PID field of 0x101.
 * 
 * Only the first 8 MPEG-TS packets inside a UDP packet
 * are filtered.
 * 
 */

#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <linux/stddef.h>
#include <linux/types.h>
#include <linux/udp.h>

#define MPEG_TS_PACKET_SIZE 188
#define MPEG_TS_SYNC_BYTE   0x47
#define METADATA_PID        0x101

static inline unsigned short ip_checksum(unsigned short *buf, int bufsz) {
    unsigned long sum = 0;

    while (bufsz > 1) {
        sum += *buf;
        buf++;
        bufsz -= 2;
    }

    if (bufsz == 1) {
        sum += *(unsigned char *)buf;
    }

    sum = (sum & 0xffff) + (sum >> 16);
    sum = (sum & 0xffff) + (sum >> 16);

    return ~sum;
}

int mpegts_filter(struct xdp_md *ctx) {
   const void *data_end = (void*)(long)ctx->data_end;
   struct ethhdr *eth_header;
   struct iphdr* ip_header;
   struct udphdr* udp_header;
   int success = 0;

   eth_header = (void*)(long)ctx->data;
   // bounds check for BPF virtual machine
   if ((void*)eth_header + sizeof(*eth_header) <= data_end) {
      ip_header = (void*)eth_header + sizeof(*eth_header);
      // bounds check for BPF virtual machine
      if ((void*)ip_header + sizeof(*ip_header) <= data_end) {
         if (ip_header->protocol == IPPROTO_UDP) {
         udp_header = (void*)ip_header + sizeof(*ip_header);
            // bounds check for BPF virtual machine 
            if ((void*)udp_header + sizeof(*udp_header) <= data_end) {
               success = 1;
            }
         }
      }
   }
   if (!success) {
      return XDP_PASS;
   }

   if (udp_header->dest == htons(15004)) {
      __be32 tmp_ip;
      unsigned char tmp_mac;

      for (int i = 0; i < ETH_ALEN; i++) {
         tmp_mac = eth_header->h_source[i];
         eth_header->h_source[i] = eth_header->h_dest[i];
         eth_header->h_dest[i] = tmp_mac;
      }

      tmp_ip = ip_header->saddr;
      ip_header->saddr = ip_header->daddr;
      ip_header->daddr = tmp_ip;
      ip_header->check = 0;
      ip_header->check = ip_checksum((unsigned short*)ip_header, sizeof(struct iphdr));

      udp_header->dest = htons(15005);
      udp_header->check = 0;

      return XDP_TX; 
   }
   return XDP_PASS;
}
