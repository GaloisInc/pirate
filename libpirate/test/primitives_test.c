#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "greatest.h"
#include "primitives.h"

int gd;
char pathname[PIRATE_LEN_NAME];

static void setup_cb(__attribute__((unused)) void *data) {
    remove(pathname);
}

static void teardown_cb(__attribute__((unused)) void *data) {
    remove(pathname);
}

TEST test_pirate_open_invalid(void) {
    int rv;
    
    rv = pirate_open(-1, O_WRONLY);
    ASSERT_EQ_FMT(-1, rv, "%d");
    ASSERT_EQ_FMT(EBADF, errno, "%d");

    rv = pirate_open(gd, O_RDWR);
    ASSERT_EQ_FMT(-1, rv, "%d");
    ASSERT_EQ_FMT(EINVAL, errno, "%d");

    PASS();
}

TEST test_pirate_open_write(void) {
    int rv;

    ASSERT(access(pathname, F_OK) < 0);
    rv = pirate_open(gd, O_WRONLY);
    ASSERT_EQ_FMT(gd, rv, "%d");
    ASSERT(access(pathname, F_OK) == 0);
    rv = pirate_open(gd, O_WRONLY);
    ASSERT_EQ_FMT(gd, rv, "%d");
    rv = pirate_close(gd);
    ASSERT_EQ_FMT(0, rv, "%d");
    ASSERT(access(pathname, F_OK) == 0);
    rv = pirate_close(gd);
    ASSERT_EQ_FMT(-1, rv, "%d");

    PASS();
}

TEST test_pirate_open_write_and_read(void) {
    int rv;

    ASSERT(access(pathname, F_OK) < 0);
    rv = pirate_open(gd, O_WRONLY);
    ASSERT_EQ_FMT(gd, rv, "%d");
    ASSERT(access(pathname, F_OK) == 0);
    rv = pirate_open(gd, O_RDONLY);
    ASSERT(access(pathname, F_OK) == 0);
    rv = pirate_close(gd);
    ASSERT_EQ_FMT(0, rv, "%d");
    ASSERT(access(pathname, F_OK) == 0);
    rv = pirate_close(gd);
    ASSERT_EQ_FMT(0, rv, "%d");
    ASSERT(access(pathname, F_OK) < 0);
    rv = pirate_close(gd);
    ASSERT_EQ_FMT(-1, rv, "%d");

    PASS();
}

TEST test_pirate_read_write() {
    int rv;
    ssize_t num;
    int input = 12345;
    int output = 0;

    rv = pirate_open(gd, O_WRONLY);
    ASSERT_EQ_FMT(gd, rv, "%d");
    rv = pirate_open(gd, O_RDONLY);
    ASSERT_EQ_FMT(gd, rv, "%d");

    num = pirate_write(gd, &input, sizeof(int));
    ASSERT_EQ_FMT(sizeof(int), num, "%zd");

    num = pirate_read(gd, &output, sizeof(int));
    ASSERT_EQ_FMT(sizeof(int), num, "%zd");

    ASSERT_EQ_FMT(input, output, "%d");

    rv = pirate_close(gd);
    ASSERT_EQ_FMT(0, rv, "%d");

    rv = pirate_close(gd);
    ASSERT_EQ_FMT(0, rv, "%d");

    PASS();
}

SUITE(pirate_primitives) {
    SET_SETUP(setup_cb, NULL);
    SET_TEARDOWN(teardown_cb, NULL);
    RUN_TEST(test_pirate_open_invalid);
    RUN_TEST(test_pirate_open_write);
    RUN_TEST(test_pirate_open_write_and_read);
    RUN_TEST(test_pirate_read_write);
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();

    srand(time(NULL));
    gd = rand();
    sprintf(pathname, PIRATE_FILENAME, gd);

    RUN_SUITE(pirate_primitives);

    GREATEST_MAIN_END();
}