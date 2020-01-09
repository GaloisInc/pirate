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
 * Copyright 2019 Two Six Labs, LLC.  All rights reserved.
 */

#ifndef __SHMEM_H
#define __SHMEM_H

#include "primitives.h"
#include "shmem_buffer.h"

#define DEFAULT_PACKET_COUNT (1000)

#define DEFAULT_PACKET_SIZE (1024)

struct ip_hdr {
  uint8_t version : 4;
  uint8_t ihl : 4;
  uint8_t tos;
  uint16_t len;
  uint16_t id;
  uint16_t flags : 3;
  uint16_t frag_offset : 13;
  uint8_t ttl;
  uint8_t proto;
  uint16_t csum;
  uint32_t srcaddr;
  uint32_t dstaddr;
} __attribute__((packed));

struct udp_hdr {
  uint16_t srcport;
  uint16_t dstport;
  uint16_t len;
  uint16_t csum;
} __attribute__((packed));

struct pseudo_ip_hdr {
  uint32_t srcaddr;
  uint32_t dstaddr;
  uint8_t zeros;
  uint8_t proto;
  uint16_t udp_len;
  uint16_t srcport;
  uint16_t dstport;
  uint16_t len;
  uint16_t csum;
} __attribute__((packed));

#define UDP_HEADER_SIZE (sizeof(struct ip_hdr) + sizeof(struct udp_hdr))

int shmem_udp_buffer_open(int gd, int flags, char *name,
                          pirate_channel_t *channel);

ssize_t shmem_udp_buffer_read(shmem_buffer_t *shmem_buffer, void *buf,
                              size_t count);

ssize_t shmem_udp_buffer_write(shmem_buffer_t *shmem_buffer, const void *buf,
                               size_t size);

int shmem_udp_buffer_close(int flags, shmem_buffer_t *shmem_buffer);

#endif
