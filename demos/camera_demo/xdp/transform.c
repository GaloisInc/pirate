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
#include <linux/limits.h>
#include <linux/stddef.h>
#include <linux/types.h>
#include <linux/udp.h>

#define MPEG_TS_PACKET_SIZE 188
#define MPEG_TS_SYNC_BYTE   0x47
#define METADATA_PID        0x101

// 4 bytes for the MPEG TS header
// 9 bytes for the required PES packet header information
// 5 bytes of additional PES header
// 16 bytes of mystery KLV data that is skipped
#define MPEG_TS_KLV_OFFSET  34

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

static uint16_t get_klv_length(uint8_t** mpeg_packet_position) {
   uint8_t* position = *mpeg_packet_position;
   uint8_t head = *position;
   uint16_t length = 0;

   if (head < 0x80) {
      length = head;
   } else if (head == 0x81) {
      position++;
      length += *position;
   } else if (head == 0x82) {
      position++;
      length += *position;
      length <<= 8;
      position++;
      length += *position;
   } else if (head == 0x83) {
      position++;
      length += *position;
      length <<= 8;
      position++;
      length += *position;
      length <<= 8;
      position++;
      length += *position;
   } else {
      bpf_trace_printk("unexpected KLV length prefix %d\n", head);
      return USHRT_MAX;
   }
   position++;
   *mpeg_packet_position = position;
   return length;
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
   for (size_t i = 0; i < 1; i++) {
      const void* mpeg_packet = (void*)udp_header + sizeof(struct udphdr) + i * MPEG_TS_PACKET_SIZE;
      const void* mpeg_packet_next = mpeg_packet + MPEG_TS_PACKET_SIZE;
      uint8_t* mpeg_packet_position = (uint8_t*) mpeg_packet;
      uint16_t mpeg_header_second_third_bytes; // TEI, PUSI, transport priority, and PID
      uint16_t pid;
      uint8_t  adaptation_field_control;

      uint16_t klv_total_length = 0;

      // MPEG-TS network packets are exactly MPEG_TS_PACKET_SIZE bytes in length.
      // If the current network packet does not fit
      // inside the bounds check then end.
      if (mpeg_packet_next > data_end) {
         break;
      }

      // if the MPEG-TS sync byte is invalid then skip this packet
      if (mpeg_packet_position[0] != MPEG_TS_SYNC_BYTE) {
         continue;
      }
      mpeg_header_second_third_bytes = ((mpeg_packet_position[1] << 8 ) | mpeg_packet_position[2]);
      pid = mpeg_header_second_third_bytes & 0x1fff;
      adaptation_field_control = ( mpeg_packet_position[3] >> 4 ) & 0x3;
      // KLV data is stored in stream with METADATA_PID 
      if (pid != METADATA_PID) {
         continue;
      }
      if (adaptation_field_control != 1) {
         continue;
      }
      mpeg_packet_position += MPEG_TS_KLV_OFFSET;
      klv_total_length = get_klv_length(&mpeg_packet_position);
      if (klv_total_length == USHRT_MAX) {
         return XDP_DROP;
      }
      bpf_trace_printk("KLV total length %d\n", klv_total_length);
      #pragma clang loop unroll(full)
      for (size_t j = 0; j < 16; j++) {
         uint16_t klv_value_length;
         if (((void*) mpeg_packet_position + 8) > data_end) {
            return XDP_PASS;
         }
         bpf_trace_printk("KLV key %d\n", *mpeg_packet_position);
         mpeg_packet_position++;
         klv_value_length = get_klv_length(&mpeg_packet_position);
         bpf_trace_printk("KLV value length %d\n", klv_value_length);
         if (klv_value_length > 64) {
            return XDP_PASS;
         }
         mpeg_packet_position += klv_value_length;
      }
   }
   return XDP_PASS;
}
