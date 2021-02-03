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
 * Copyright 2021 Two Six Labs, LLC.  All rights reserved.
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
            if ((void*)udp + sizeof(*udp) <= data_end) {
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
#if SRC_PORT
   if (ntohs(udp_header->source) != SRC_PORT) {
      return XDP_PASS;
   }
#endif
#if DST_PORT
   if (ntohs(udp_header->dest) != DST_PORT) {
      return XDP_PASS;
   }
#endif
   #pragma clang loop unroll(full)
   for (size_t i = 0; i < 8; i++) {
      const void* mpeg_packet = (void*)udp_header + sizeof(struct udphdr) + i * MPEG_TS_PACKET_SIZE;
      const void* mpeg_packet_next = mpeg_packet + MPEG_TS_PACKET_SIZE;
      uint8_t* packet_bytes = (uint8_t*) mpeg_packet;
      uint16_t mpeg_header_second_third_bytes; // TEI, PUSI, transport priority, and PID
      uint16_t pid;

      // MPEG-TS network packets are exactly MPEG_TS_PACKET_SIZE bytes in length.
      // If the current network packet does not fit
      // inside the bounds check then end.
      if (mpeg_packet_next > data_end) {
         break;
      }

      // if the MPEG-TS sync byte is invalid then skip this packet
      if (packet_bytes[0] != MPEG_TS_SYNC_BYTE) {
         continue;
      }
      mpeg_header_second_third_bytes = ((packet_bytes[1] << 8 ) | packet_bytes[2]);
      pid = mpeg_header_second_third_bytes & 0x1fff;
      // KLV data is stored in stream with METADATA_PID 
      if (pid != METADATA_PID) {
         continue;
      }
      packet_bytes[1] = 0x1F;
      packet_bytes[2] = 0xFF;
      packet_bytes[3] = 0x10;
      memset(packet_bytes + 4, 0, MPEG_TS_PACKET_SIZE - 4);
   }
   return XDP_PASS;
}
