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

static struct udphdr* locate_udpheader(struct xdp_md *ctx) {
   void *data = (void *)(long)ctx->data;
   const void *data_end = (void *)(long)ctx->data_end;
   struct ethhdr *eth = data;
   // bounds check for BPF virtual machine
   if ((void*)eth + sizeof(*eth) <= data_end) {
      struct iphdr *ip = data + sizeof(*eth);
      // bounds check for BPF virtual machine
      if ((void*)ip + sizeof(*ip) <= data_end) {
         if (ip->protocol == IPPROTO_UDP) {
            struct udphdr *udp = (void*)ip + sizeof(*ip);
            // bounds check for BPF virtual machine 
            if ((void*)udp + sizeof(*udp) < data_end) {
               return udp;
            }
         }
      }
   }
   return NULL;
}

int mpegts_filter(struct xdp_md *ctx) {
   const void *data_end = (void*)(long)ctx->data_end;
   const struct udphdr* udp_header = locate_udpheader(ctx);
   if (udp_header == NULL) {
      return XDP_PASS;
   }
   #pragma clang loop unroll(full)
   for (size_t i = 0; i < 8; i++) {
      const void* packet_body = (void*)udp_header + sizeof(struct udphdr) + i * MPEG_TS_PACKET_SIZE;
      const void* packet_end = packet_body + MPEG_TS_PACKET_SIZE;
      uint16_t up_to_pid; /* First 16 bits after sync_byte up to and including PID. */
      uint8_t* packet_bytes = (uint8_t*) packet_body;
      uint16_t pid;

      // if we do not have a complete MPEG-TS packet then stop
      // bounds check for BPF virtual machine
      if (data_end < packet_end) {
         break;
      }

      // if the MPEG-TS sync byte is invalid then skip this packet
      if (packet_bytes[0] != MPEG_TS_SYNC_BYTE) {
         continue;
      }
      up_to_pid = ((packet_bytes[1] << 8 ) | packet_bytes[2]);
      pid = up_to_pid & 0x1fff;
      // KLV data is stored in stream with METADATA_PID 
      if (pid != METADATA_PID) {
         continue;
      }
      packet_bytes[1] = 0x1F;
      packet_bytes[2] = 0xFF;
      packet_bytes[3] = 0x10;
      memset(packet_bytes + 4, 0xFF, MPEG_TS_PACKET_SIZE - 4);
   }
   return XDP_PASS;
}
