#include <sys/socket.h>
#include <sys/types.h>

#include "gtest/gtest.h"

#include "pal/pal.h"
#include "pal/envelope.h"

int mk_sockets(int fds[2])
{
    return socketpair(AF_UNIX, SOCK_DGRAM, 0, fds);
}

TEST(envelope, add_get_integer)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);
    const int data = 9001;

    ASSERT_EQ(pal_add_to_env(&env, &data, sizeof data), PAL_ERR_SUCCESS);

    pal_env_iterator_t start = pal_env_iterator_start(&env);
    pal_env_iterator_t end = pal_env_iterator_end(&env);
    int *pdata;

    EXPECT_EQ(pal_env_iterator_size(start), sizeof data);
    ASSERT_NE(pdata = (int *)pal_env_iterator_data(start), nullptr);
    EXPECT_EQ(*pdata, data);
    ASSERT_EQ(pal_env_iterator_next(start), end);

    pal_free_env(&env);
}

TEST(envelope, add_get_multiple)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);
    const int data1 = 9001;
    const char data2[] = "blargh";
    const bool data3 = true;

    ASSERT_EQ(pal_add_to_env(&env, &data1, sizeof data1), PAL_ERR_SUCCESS);
    ASSERT_EQ(pal_add_to_env(&env, &data2, sizeof data2), PAL_ERR_SUCCESS);
    ASSERT_EQ(pal_add_to_env(&env, &data3, sizeof data3), PAL_ERR_SUCCESS);

    pal_env_iterator_t start = pal_env_iterator_start(&env);
    pal_env_iterator_t end = pal_env_iterator_end(&env);

    {
        int *pdata;

        EXPECT_EQ(pal_env_iterator_size(start), sizeof data1);
        ASSERT_NE(pdata = (int *)pal_env_iterator_data(start), nullptr);
        EXPECT_EQ(*pdata, data1);
        ASSERT_NE(start = pal_env_iterator_next(start), end);
    }

    {
        char *pdata;

        EXPECT_EQ(pal_env_iterator_size(start), sizeof data2);
        ASSERT_NE(pdata = (char *)pal_env_iterator_data(start), nullptr);
        EXPECT_STREQ(pdata, data2);
        ASSERT_NE(start = pal_env_iterator_next(start), end);
    }

    {
        bool *pdata;

        EXPECT_EQ(pal_env_iterator_size(start), sizeof data3);
        ASSERT_NE(pdata = (bool *)pal_env_iterator_data(start), nullptr);
        EXPECT_EQ(*pdata, data3);
        ASSERT_EQ(pal_env_iterator_next(start), end);
    }

    pal_free_env(&env);
}

TEST(envelope, send_recv_integer)
{
    const int data = 9001;
    int fds[2];

    ASSERT_EQ(mk_sockets(fds), 0);

    {
        pal_env_t s_env = EMPTY_PAL_ENV(PAL_RESOURCE);

        ASSERT_EQ(pal_add_to_env(&s_env, &data, sizeof data), PAL_ERR_SUCCESS);
        ASSERT_EQ(pal_send_env(fds[0], &s_env, 0), PAL_ERR_SUCCESS);

        pal_free_env(&s_env);
    }

    {
        pal_env_t r_env;

        ASSERT_EQ(pal_recv_env(fds[1], &r_env, 0), 0);

        pal_env_iterator_t start = pal_env_iterator_start(&r_env);
        pal_env_iterator_t end = pal_env_iterator_end(&r_env);
        int *pdata;

        EXPECT_EQ(pal_env_iterator_size(start), sizeof data);
        ASSERT_NE(pdata = (int *)pal_env_iterator_data(start), nullptr);
        EXPECT_EQ(*pdata, data);
        ASSERT_EQ(pal_env_iterator_next(start), end);

        pal_free_env(&r_env);
    }

    close(fds[0]);
    close(fds[1]);
}

TEST(envelope, send_recv_request)
{
    const char s_type[] = "bar", s_name[] = "foo";
    int fds[2];

    ASSERT_EQ(mk_sockets(fds), 0);

    {
        ASSERT_EQ(pal_send_resource_request(fds[0], s_type, s_name, 0),
                PAL_ERR_SUCCESS);
    }

    {
        char *r_type, *r_name;

        ASSERT_EQ(pal_recv_resource_request(fds[1], &r_type, &r_name, 0),
                PAL_ERR_SUCCESS);
        ASSERT_STREQ(r_type, s_type);
        ASSERT_STREQ(r_name, s_name);

        free(r_type);
        free(r_name);
    }
}
