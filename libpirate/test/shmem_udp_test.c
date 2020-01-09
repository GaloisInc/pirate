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

#define _POSIX_C_SOURCE 200809L

#include "greatest.h"
#include "primitives.h"
#include "shmem.h"

#define HIGH_TEST_CH 0
#define HIGH_TO_LOW_CH 1
#define LOW_TO_HIGH_CH 2
#define UDP_PACKET_SIZE 64

static int test_buffer_fill(char *buffer, int len, int value) {
  int i;
  for (i = 0; i < len; i++) {
    if (buffer[i] != value) {
      return i + 1;
    }
  }
  return 0;
}

TEST test_high_to_low_comm_shmem_udp() {
  int rv;
  char *data = malloc(UDP_PACKET_SIZE);

  rv = pirate_open(HIGH_TO_LOW_CH, O_WRONLY);
  if (rv < 0) {
    perror("unable to open HIGH_TO_LOW_CH for writing");
  }
  ASSERT_EQ_FMT(HIGH_TO_LOW_CH, rv, "%d");

  rv = pirate_open(LOW_TO_HIGH_CH, O_RDONLY);
  if (rv < 0) {
    perror("unable to open LOW_TO_HIGH_CH for reading");
  }
  ASSERT_EQ_FMT(LOW_TO_HIGH_CH, rv, "%d");

  memset(data, '0', UDP_PACKET_SIZE);
  rv = pirate_write(HIGH_TO_LOW_CH, data, UDP_PACKET_SIZE);
  ASSERT_EQ_FMT(UDP_PACKET_SIZE, rv, "%d");

  rv = pirate_read(LOW_TO_HIGH_CH, data, UDP_PACKET_SIZE);
  ASSERT_EQ_FMT(UDP_PACKET_SIZE, rv, "%d");
  ASSERT_EQ_FMT(0, test_buffer_fill(data, UDP_PACKET_SIZE, '1'), "%d");

  rv = pirate_close(HIGH_TO_LOW_CH, O_WRONLY);
  ASSERT_EQ_FMT(0, rv, "%d");

  rv = pirate_close(LOW_TO_HIGH_CH, O_RDONLY);
  ASSERT_EQ_FMT(0, rv, "%d");

  free(data);

  PASS();
}

TEST test_low_to_high_comm_shmem_udp() {
  int rv;
  char *data = malloc(UDP_PACKET_SIZE);

  rv = pirate_open(HIGH_TO_LOW_CH, O_RDONLY);
  if (rv < 0) {
    perror("unable to open HIGH_TO_LOW_CH for reading");
  }
  ASSERT_EQ_FMT(HIGH_TO_LOW_CH, rv, "%d");

  rv = pirate_open(LOW_TO_HIGH_CH, O_WRONLY);
  if (rv < 0) {
    perror("unable to open LOW_TO_HIGH_CH for writing");
  }
  ASSERT_EQ_FMT(LOW_TO_HIGH_CH, rv, "%d");

  rv = pirate_read(HIGH_TO_LOW_CH, data, UDP_PACKET_SIZE);
  ASSERT_EQ_FMT(UDP_PACKET_SIZE, rv, "%d");
  ASSERT_EQ_FMT(0, test_buffer_fill(data, UDP_PACKET_SIZE, '0'), "%d");

  memset(data, '1', UDP_PACKET_SIZE);
  rv = pirate_write(LOW_TO_HIGH_CH, data, UDP_PACKET_SIZE);
  ASSERT_EQ_FMT(UDP_PACKET_SIZE, rv, "%d");

  rv = pirate_close(HIGH_TO_LOW_CH, O_RDONLY);
  ASSERT_EQ_FMT(0, rv, "%d");

  rv = pirate_close(LOW_TO_HIGH_CH, O_WRONLY);
  ASSERT_EQ_FMT(0, rv, "%d");

  free(data);

  PASS();
}

void *low_to_high_func_shmem_udp(__attribute__((unused)) void *unused) {
  return (void *)test_low_to_high_comm_shmem_udp();
}

void *high_to_low_func_shmem_udp(__attribute__((unused)) void *unused) {
  return (void *)test_high_to_low_comm_shmem_udp();
}

enum greatest_test_res test_communication_pthread_shmem_udp() {
  pthread_t low_to_high_id, high_to_low_id;
  int rv;
  void *status1, *status2;
  channel_t prev1, prev2;

  prev1 = pirate_get_channel_type(HIGH_TO_LOW_CH);
  prev2 = pirate_get_channel_type(LOW_TO_HIGH_CH);
  pirate_set_channel_type(HIGH_TO_LOW_CH, SHMEM_UDP);
  pirate_set_channel_type(LOW_TO_HIGH_CH, SHMEM_UDP);

  rv = pthread_create(&low_to_high_id, NULL, low_to_high_func_shmem_udp, NULL);
  if (rv != 0) {
    FAILm(strerror(rv));
  }

  rv = pthread_create(&high_to_low_id, NULL, high_to_low_func_shmem_udp, NULL);
  if (rv != 0) {
    FAILm(strerror(rv));
  }

  rv = pthread_join(low_to_high_id, &status1);
  if (rv != 0) {
    FAILm(strerror(rv));
  }

  rv = pthread_join(high_to_low_id, &status2);
  if (rv != 0) {
    FAILm(strerror(rv));
  }

  pirate_set_channel_type(HIGH_TO_LOW_CH, prev1);
  pirate_set_channel_type(LOW_TO_HIGH_CH, prev2);

  if (((greatest_test_res)status1) == GREATEST_TEST_RES_FAIL) {
    if (GREATEST_ABORT_ON_FAIL()) {
      abort();
    }
    return GREATEST_TEST_RES_FAIL;
  }

  if (((greatest_test_res)status2) == GREATEST_TEST_RES_FAIL) {
    if (GREATEST_ABORT_ON_FAIL()) {
      abort();
    }
    return GREATEST_TEST_RES_FAIL;
  }

  PASS();
}
