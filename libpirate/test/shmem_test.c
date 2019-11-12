#define _POSIX_C_SOURCE 200809L

#include "greatest.h"
#include "primitives.h"
#include "shmem.h"

#define HIGH_TEST_CH 0
#define HIGH_TO_LOW_CH 1
#define LOW_TO_HIGH_CH 2

int test_buffer_fill(char *buffer, int len, int value) {
  int i;
  for (i = 0; i < len; i++) {
    if (buffer[i] != value) {
      return i + 1;
    }
  }
  return 0;
}

TEST test_high_to_low_comm_shmem() {
  int rv;
  char *data = malloc(DEFAULT_SHMEM_BUFFER);

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

  memset(data, '0', DEFAULT_SHMEM_BUFFER);
  rv = pirate_write(HIGH_TO_LOW_CH, data, DEFAULT_SHMEM_BUFFER);
  ASSERT_EQ_FMT(DEFAULT_SHMEM_BUFFER, rv, "%d");

  rv = pirate_read(LOW_TO_HIGH_CH, data, DEFAULT_SHMEM_BUFFER);
  ASSERT_EQ_FMT(DEFAULT_SHMEM_BUFFER, rv, "%d");
  ASSERT_EQ_FMT(0, test_buffer_fill(data, DEFAULT_SHMEM_BUFFER, '1'), "%d");

  rv = pirate_close(HIGH_TO_LOW_CH, O_WRONLY);
  ASSERT_EQ_FMT(0, rv, "%d");

  rv = pirate_close(LOW_TO_HIGH_CH, O_RDONLY);
  ASSERT_EQ_FMT(0, rv, "%d");

  free(data);

  PASS();
}

TEST test_low_to_high_comm_shmem() {
  int rv;
  char *data = malloc(DEFAULT_SHMEM_BUFFER);

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

  rv = pirate_read(HIGH_TO_LOW_CH, data, DEFAULT_SHMEM_BUFFER);
  ASSERT_EQ_FMT(DEFAULT_SHMEM_BUFFER, rv, "%d");
  ASSERT_EQ_FMT(0, test_buffer_fill(data, DEFAULT_SHMEM_BUFFER, '0'), "%d");

  memset(data, '1', DEFAULT_SHMEM_BUFFER);
  rv = pirate_write(LOW_TO_HIGH_CH, data, DEFAULT_SHMEM_BUFFER);
  ASSERT_EQ_FMT(DEFAULT_SHMEM_BUFFER, rv, "%d");

  rv = pirate_close(HIGH_TO_LOW_CH, O_RDONLY);
  ASSERT_EQ_FMT(0, rv, "%d");

  rv = pirate_close(LOW_TO_HIGH_CH, O_WRONLY);
  ASSERT_EQ_FMT(0, rv, "%d");

  free(data);

  PASS();
}

void *low_to_high_func_shmem(__attribute__((unused)) void *unused) {
  return (void *)test_low_to_high_comm_shmem();
}

void *high_to_low_func_shmem(__attribute__((unused)) void *unused) {
  return (void *)test_high_to_low_comm_shmem();
}

enum greatest_test_res test_communication_pthread_shmem() {
  pthread_t low_to_high_id, high_to_low_id;
  int rv;
  void *status1, *status2;
  channel_t prev1, prev2;

  prev1 = pirate_get_channel_type(HIGH_TO_LOW_CH);
  prev2 = pirate_get_channel_type(LOW_TO_HIGH_CH);
  pirate_set_channel_type(HIGH_TO_LOW_CH, SHMEM);
  pirate_set_channel_type(LOW_TO_HIGH_CH, SHMEM);

  rv = pthread_create(&low_to_high_id, NULL, low_to_high_func_shmem, NULL);
  if (rv != 0) {
    FAILm(strerror(rv));
  }

  rv = pthread_create(&high_to_low_id, NULL, high_to_low_func_shmem, NULL);
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
