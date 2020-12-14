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

static uint8_t* klv_get_length(uint8_t *pData, uint64_t *pLength, void* data_end) {
   if (((void*) pData) > data_end) {
      return pData;
   }
   /*
   if (*pData & 0x80) {
      int bytes = *pData & 0x7F;
      pData++;
      *pLength = 0;
      for (int i = 0; (bytes > 0) && (i < 4) && (pLength <= data_end); i++, bytes--) {
         *pLength |= ((uint64_t)*pData) << (bytes * 8);
         pData++;
      }
      */
   *pLength = *pData;
   return pData;
}

int mpegts_filter(struct xdp_md *ctx) {
   const void *data_end = (void*)(long)ctx->data_end;
   const struct udphdr* udp_header = locate_udpheader(ctx);
   if (udp_header == NULL) {
      return XDP_PASS;
   }
   #pragma clang loop unroll(full)
   for (size_t i = 0; i < 7; i++) {
      const void* packet_body = (void*)udp_header + sizeof(struct udphdr) + i * MPEG_TS_PACKET_SIZE;
      const void* packet_end = packet_body + MPEG_TS_PACKET_SIZE;
      uint16_t up_to_pid; /* First 16 bits after sync_byte up to and including PID. */
      uint8_t  after_pid;
      const uint8_t* packet_bytes = (uint8_t*) packet_body;

      uint16_t pid;
      uint8_t  adaptation_field_control;
      uint16_t total_length = 0;
      uint8_t  offset = 34;

      // if we do not have a complete MPEG-TS packet then stop
      // bounds check for BPF virtual machine
      if ((packet_end - 1) > data_end) {
         break;
      }

      // if the MPEG-TS sync byte is invalid then skip this packet
      if (packet_bytes[0] != MPEG_TS_SYNC_BYTE) {
         continue;
      }
      up_to_pid = ((packet_bytes[1] << 8 ) | packet_bytes[2]);
      after_pid = packet_bytes[3];
      pid = up_to_pid & 0x1fff;
      adaptation_field_control = ( after_pid >> 4 ) & 0x3;
      // KLV data is stored in stream with METADATA_PID 
      if (pid != METADATA_PID) {
         continue;
      }
      if (adaptation_field_control != 1) {
         continue;
      }
      // 4 bytes for the MPEG TS header
      // 9 bytes for the required PES packet header information
      // 5 bytes of additional PES header
      // 16 bytes of mystery KLV data that is skipped
      if (((void*) (packet_bytes + offset)) >= packet_end) {
         return XDP_DROP;
      }
      if (packet_bytes[offset] < 0x80) {
         total_length = packet_bytes[offset];
         offset++;
      } else if (packet_bytes[offset] == 0x81) {
         if (((void*) (packet_bytes + offset + 1)) < packet_end) {
            total_length = packet_bytes[offset + 1];
         }
         offset += 2;
      } else if (packet_bytes[offset] == 0x82) {
         if (((void*) (packet_bytes + offset + 2)) < packet_end) {
            total_length = packet_bytes[offset + 1];
            total_length <<= 8;
            total_length += packet_bytes[offset + 2];
         }
         offset += 3;
      } else if (packet_bytes[offset] == 0x83) {
         if (((void*) (packet_bytes + offset + 3)) < packet_end) {
            total_length = packet_bytes[offset + 1];
            total_length <<= 8;
            total_length += packet_bytes[offset + 2];
            total_length <<= 8;
            total_length += packet_bytes[offset + 3];
         }
         offset += 4;
      } else {
         bpf_trace_printk("unexpected KLV length prefix %d\n", packet_bytes[offset]);
         return XDP_DROP;
      }
      if (((void*) (packet_bytes + offset)) >= packet_end) {
         return XDP_PASS;
      }
      bpf_trace_printk("KLV total length %d\n", total_length);
      bpf_trace_printk("KLV key %d\n", packet_bytes[offset]);
      // This is an assumption that the latitude and longitude
      // are present in the first metadata MPEG-TS packet that is
      // inside the UDP packet.
      return XDP_PASS;
   }
   return XDP_PASS;
}
