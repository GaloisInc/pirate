#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "greatest.h"
#include "primitives.h"

#define HIGH_TEST_CH    0
#define HIGH_TO_LOW_CH  1
#define LOW_TO_HIGH_CH  2
#define TEST_DATA       0xC0DEFACE

GREATEST_MAIN_DEFS();

TEST test_pirate_open_invalid(void) {
    int rv;

    rv = pirate_open(-1, O_WRONLY);
    ASSERT_EQ_FMT(-1, rv, "%d");
    ASSERT_EQ_FMT(EBADF, errno, "%d");

    rv = pirate_open(PIRATE_NUM_CHANNELS, O_WRONLY);
    ASSERT_EQ_FMT(-1, rv, "%d");
    ASSERT_EQ_FMT(EBADF, errno, "%d");

    rv = pirate_open(HIGH_TEST_CH, O_RDWR);
    ASSERT_EQ_FMT(-1, rv, "%d");
    ASSERT_EQ_FMT(EINVAL, errno, "%d");

    PASS();
}

TEST test_pirate_unopened(void) {
    int rv;

    rv = pirate_read(HIGH_TEST_CH, NULL, 0);
    ASSERT_EQ_FMT(-1, rv, "%d");
    ASSERT_EQ_FMT(EBADF, errno, "%d");

    rv = pirate_write(HIGH_TEST_CH, NULL, 0);
    ASSERT_EQ_FMT(-1, rv, "%d");
    ASSERT_EQ_FMT(EBADF, errno, "%d");

    rv = pirate_close(HIGH_TEST_CH, O_RDONLY);
    ASSERT_EQ_FMT(-1, rv, "%d");
    ASSERT_EQ_FMT(ENODEV, errno, "%d");

    PASS();
}

TEST test_high_to_low_comm() {
    int rv;
    uint32_t data = TEST_DATA;

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

    // test double-open
    rv = pirate_open(LOW_TO_HIGH_CH, O_RDONLY);
    ASSERT_EQ_FMT(LOW_TO_HIGH_CH, rv, "%d");

    rv = pirate_write(HIGH_TO_LOW_CH, &data, sizeof(data));
    ASSERT_EQ_FMT((int)sizeof(data), rv, "%d");

    rv = pirate_read(LOW_TO_HIGH_CH, &data, sizeof(data));
    ASSERT_EQ_FMT((int)sizeof(data), rv, "%d");
    ASSERT_EQ_FMT(~TEST_DATA, data, "%u");

    rv = pirate_close(HIGH_TO_LOW_CH, O_WRONLY);
    ASSERT_EQ_FMT(0, rv, "%d");

    rv = pirate_close(LOW_TO_HIGH_CH, O_RDONLY);
    ASSERT_EQ_FMT(0, rv, "%d");

    PASS();
}

TEST test_low_to_high_comm() {
    int rv;
    uint32_t data;

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

    rv = pirate_read(HIGH_TO_LOW_CH, &data, sizeof(data));
    ASSERT_EQ_FMT((int)sizeof(data), rv, "%d");
    ASSERT_EQ_FMT(TEST_DATA, data, "%u");

    data = ~data;
    rv = pirate_write(LOW_TO_HIGH_CH, &data, sizeof(data));
    ASSERT_EQ_FMT((int)sizeof(data), rv, "%d");

    rv = pirate_close(HIGH_TO_LOW_CH, O_RDONLY);
    ASSERT_EQ_FMT(0, rv, "%d");

    rv = pirate_close(LOW_TO_HIGH_CH, O_WRONLY);
    ASSERT_EQ_FMT(0, rv, "%d");

    PASS();
}

void *low_to_high_func(__attribute__((unused)) void* unused) {
    return (void*) test_low_to_high_comm();
}

void *high_to_low_func(__attribute__((unused)) void* unused) {
    return (void*) test_high_to_low_comm();
}

TEST test_communication_pthread() {
    pthread_t low_to_high_id, high_to_low_id;
    int rv;
    void *status1, *status2;

    rv = pthread_create(&low_to_high_id, NULL, low_to_high_func, NULL);
    if (rv != 0) {
        FAILm(strerror(rv));
    }

    rv = pthread_create(&high_to_low_id, NULL, high_to_low_func, NULL);
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

    if (((greatest_test_res) status1) == GREATEST_TEST_RES_FAIL) {
        if (GREATEST_ABORT_ON_FAIL()) { abort(); }
        return GREATEST_TEST_RES_FAIL;
    }

    if (((greatest_test_res) status2) == GREATEST_TEST_RES_FAIL) {
        if (GREATEST_ABORT_ON_FAIL()) { abort(); }
        return GREATEST_TEST_RES_FAIL;
    }

    PASS();
}

SUITE(pirate_one_process) {
    RUN_TEST(test_pirate_open_invalid);
    RUN_TEST(test_pirate_unopened);
}

SUITE(pirate_low) {
    RUN_TEST(test_low_to_high_comm);
}

SUITE(pirate_high) {
    RUN_TEST(test_high_to_low_comm);
}

int main(int argc, char **argv) {
    int i;
    GREATEST_MAIN_BEGIN();

    for (i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {
            pirate_set_channel_type(HIGH_TO_LOW_CH, DEVICE);
            pirate_set_pathname(HIGH_TO_LOW_CH, argv[i]);
            break;
        }
    }

    RUN_SUITE(pirate_one_process);

    RUN_TEST(test_communication_pthread);

    pid_t ch_pid = fork();
    switch (ch_pid) {
    case -1:        // Error
        perror("fork failed");
        return -1;
    case 0:         // Child
        RUN_SUITE(pirate_low);
        break;
    default:        // Parent
        RUN_SUITE(pirate_high);
        wait(NULL);
    }

    GREATEST_MAIN_END();
}
