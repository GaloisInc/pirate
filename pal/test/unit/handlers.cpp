#include <fcntl.h>
#include <cstring>

#include "gtest/gtest.h"

extern "C" {
#include "handlers.h"
}
#include "pal/envelope.h"

#define alen(_arr) (sizeof (_arr) / sizeof *(_arr))

static void run_handler(pal_env_t *env, const struct resource *rsc)
{
    resource_handler_t *handle;

    ASSERT_NE(handle = lookup_handler(rsc->r_type), nullptr);
    ASSERT_EQ(handle(env, rsc), PAL_ERR_SUCCESS);
}

static char *get_string_iter_data(pal_env_iterator_t iter)
{
    size_t size = pal_env_iterator_size(iter);
    const char *data = (char *)pal_env_iterator_data(iter);
    char *res = nullptr;
    static char buf[1024];

    if(data) {
        memcpy(buf, pal_env_iterator_data(iter), sizeof buf);
        buf[size] = '\0';
        res = buf;
    }

    return res;
}

TEST(handlers, cstring_resource_handler)
{
    char value[] = "blargh";
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        struct resource rsc = {0};
        rsc.r_name = (char *)"my_string";
        rsc.r_type = (char *)"string";
        rsc.r_ids = nullptr;
        rsc.r_ids_count = 0;
        rsc.r_contents.cc_string_value = value;

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);

        ASSERT_NE(start, end);
        EXPECT_EQ(pal_env_iterator_size(start), sizeof(value) - 1);
        EXPECT_STREQ(get_string_iter_data(start), value);
        ASSERT_EQ(pal_env_iterator_next(start), end);
    }

    pal_free_env(&env);
}

TEST(handlers, int64_resource_handler)
{
    int64_t value = 9001;
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        struct resource rsc = {0};
        rsc.r_name = (char *)"my_integer";
        rsc.r_type = (char *)"integer";
        rsc.r_ids = nullptr;
        rsc.r_ids_count = 0;
        rsc.r_contents.cc_integer_value = &value;

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);

        ASSERT_NE(start, end);
        EXPECT_EQ(pal_env_iterator_size(start), sizeof value);
        EXPECT_EQ(*(int64_t *)pal_env_iterator_data(start), value);
        ASSERT_EQ(pal_env_iterator_next(start), end);
    }

    pal_free_env(&env);
}

TEST(handlers, bool_resource_handler)
{
    bool value = true;
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        struct resource rsc = {0};
        rsc.r_name = (char *)"my_boolean";
        rsc.r_type = (char *)"boolean";
        rsc.r_ids = nullptr;
        rsc.r_ids_count = 0;
        rsc.r_contents.cc_boolean_value = &value;

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);

        ASSERT_NE(start, end);
        EXPECT_EQ(pal_env_iterator_size(start), sizeof value);
        EXPECT_EQ(*(int64_t *)pal_env_iterator_data(start), value);
        ASSERT_EQ(pal_env_iterator_next(start), end);
    }

    pal_free_env(&env);
}

TEST(handlers, file_resource_handler_required_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        struct resource rsc = {0};
        rsc.r_name = (char *)"my_file";
        rsc.r_type = (char *)"file";
        rsc.r_ids = nullptr;
        rsc.r_ids_count = 0;
        rsc.r_contents.cc_file_path = (char *)"/dev/null";
        rsc.r_contents.cc_file_flags = nullptr;

        run_handler(&env, &rsc);
    }

    ASSERT_EQ(env.fds_count, 1);
    EXPECT_NE(fcntl(env.fds[0], F_GETFL), -1);

    pal_free_env(&env);
}

TEST(handlers, file_resource_handler_optional_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        int flags = O_RDONLY;
        struct resource rsc = {0};
        rsc.r_name = (char *)"my_file";
        rsc.r_type = (char *)"file";
        rsc.r_ids = nullptr;
        rsc.r_ids_count = 0;
        rsc.r_contents.cc_file_path = (char *)"/dev/null";
        rsc.r_contents.cc_file_flags = &flags;

        run_handler(&env, &rsc);
    }

    ASSERT_EQ(env.fds_count, 1);
    EXPECT_NE(fcntl(env.fds[0], F_GETFL), -1);

    pal_free_env(&env);
}

TEST(handlers, pirate_channel_resource_handler_device_required_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        struct resource rsc = {0};
        rsc.r_name = (char *)"my_channel";
        rsc.r_type = (char *)"pirate_channel";
        rsc.r_ids = nullptr;
        rsc.r_ids_count = 0;
        rsc.r_contents.cc_channel_type = DEVICE;
        rsc.r_contents.cc_path = (char *)"/foo/bar";

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);

        ASSERT_NE(start, end);
        EXPECT_STREQ(get_string_iter_data(start), "device,/foo/bar");
        ASSERT_EQ(pal_env_iterator_next(start), end);
    }

    pal_free_env(&env);
}

TEST(handlers, pirate_channel_resource_handler_device_optional_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        struct resource rsc = {0};
        rsc.r_name = (char *)"my_channel";
        rsc.r_type = (char *)"pirate_channel";
        rsc.r_ids = nullptr;
        rsc.r_ids_count = 0;
        rsc.r_contents.cc_channel_type = DEVICE;
        rsc.r_contents.cc_path = (char *)"/foo/bar";
        rsc.r_contents.cc_min_tx_size = 9001;
        rsc.r_contents.cc_mtu = 9002;

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);

        ASSERT_NE(start, end);
        EXPECT_STREQ(get_string_iter_data(start),
                "device,/foo/bar,min_tx_size=9001,mtu=9002");
        ASSERT_EQ(pal_env_iterator_next(start), end);
    }

    pal_free_env(&env);
}

TEST(handlers, pirate_channel_resource_handler_pipe_required_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        struct resource rsc = {0};
        rsc.r_name = (char *)"my_channel";
        rsc.r_type = (char *)"pirate_channel";
        rsc.r_ids = nullptr;
        rsc.r_ids_count = 0;
        rsc.r_contents.cc_channel_type = PIPE;
        rsc.r_contents.cc_path = (char *)"/foo/bar";

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);

        ASSERT_NE(start, end);
        EXPECT_STREQ(get_string_iter_data(start), "pipe,/foo/bar");
        ASSERT_EQ(pal_env_iterator_next(start), end);
    }

    pal_free_env(&env);
}

TEST(handlers, pirate_channel_resource_handler_pipe_optional_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        struct resource rsc = {0};
        rsc.r_name = (char *)"my_channel";
        rsc.r_type = (char *)"pirate_channel";
        rsc.r_ids = nullptr;
        rsc.r_ids_count = 0;
        rsc.r_contents.cc_channel_type = PIPE;
        rsc.r_contents.cc_path = (char *)"/foo/bar";
        rsc.r_contents.cc_min_tx_size = 9001;
        rsc.r_contents.cc_mtu = 9002;

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);

        ASSERT_NE(start, end);
        EXPECT_STREQ(get_string_iter_data(start),
                "pipe,/foo/bar,min_tx_size=9001,mtu=9002");
        ASSERT_EQ(pal_env_iterator_next(start), end);
    }

    pal_free_env(&env);
}

TEST(handlers, pirate_channel_resource_handler_unix_socket_required_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        struct resource rsc = {0};
        rsc.r_name = (char *)"my_channel";
        rsc.r_type = (char *)"pirate_channel";
        rsc.r_ids = nullptr;
        rsc.r_ids_count = 0;
        rsc.r_contents.cc_channel_type = UNIX_SOCKET;
        rsc.r_contents.cc_path = (char *)"/foo/bar";

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);

        ASSERT_NE(start, end);
        ASSERT_STREQ(get_string_iter_data(start), "unix_socket,/foo/bar");
        ASSERT_EQ(pal_env_iterator_next(start), end);
    }

    pal_free_env(&env);
}

TEST(handlers, pirate_channel_resource_handler_unix_socket_optional_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        struct resource rsc = {0};
        rsc.r_name = (char *)"my_channel";
        rsc.r_type = (char *)"pirate_channel";
        rsc.r_ids = nullptr;
        rsc.r_ids_count = 0;
        rsc.r_contents.cc_channel_type = UNIX_SOCKET;
        rsc.r_contents.cc_path = (char *)"/foo/bar";
        rsc.r_contents.cc_min_tx_size = 9001;
        rsc.r_contents.cc_mtu = 9002;
        rsc.r_contents.cc_buffer_size = 9003;

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);

        ASSERT_NE(start, end);
        EXPECT_STREQ(get_string_iter_data(start), "unix_socket,/foo/bar,"
                "buffer_size=9003,min_tx_size=9001,mtu=9002");
        ASSERT_EQ(pal_env_iterator_next(start), end);
    }

    pal_free_env(&env);
}

TEST(handlers, pirate_channel_resource_handler_tcp_socket_required_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        struct resource rsc = {0};
        rsc.r_name = (char *)"my_channel";
        rsc.r_type = (char *)"pirate_channel";
        rsc.r_ids = nullptr;
        rsc.r_ids_count = 0;
        rsc.r_contents.cc_channel_type = TCP_SOCKET;
        rsc.r_contents.cc_host = (char *)"10.0.0.1";
        rsc.r_contents.cc_port = 9004;

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);

        ASSERT_NE(start, end);
        EXPECT_STREQ(get_string_iter_data(start), "tcp_socket,10.0.0.1,9004");
        ASSERT_EQ(pal_env_iterator_next(start), end);
    }

    pal_free_env(&env);
}

TEST(handlers, pirate_channel_resource_handler_tcp_socket_optional_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        struct resource rsc = {0};
        rsc.r_name = (char *)"my_channel";
        rsc.r_type = (char *)"pirate_channel";
        rsc.r_ids = nullptr;
        rsc.r_ids_count = 0;
        rsc.r_contents.cc_channel_type = TCP_SOCKET;
        rsc.r_contents.cc_min_tx_size = 9001;
        rsc.r_contents.cc_mtu = 9002;
        rsc.r_contents.cc_buffer_size = 9003;
        rsc.r_contents.cc_host = (char *)"10.0.0.1";
        rsc.r_contents.cc_port = 9004;

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);

        ASSERT_NE(start, end);
        EXPECT_STREQ(get_string_iter_data(start), "tcp_socket,10.0.0.1,9004,"
                "buffer_size=9003,min_tx_size=9001,mtu=9002");
        ASSERT_EQ(pal_env_iterator_next(start), end);
    }

    pal_free_env(&env);
}

TEST(handlers, pirate_channel_resource_handler_udp_socket_required_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        struct resource rsc = {0};
        rsc.r_name = (char *)"my_channel";
        rsc.r_type = (char *)"pirate_channel";
        rsc.r_ids = nullptr;
        rsc.r_ids_count = 0;
        rsc.r_contents.cc_channel_type = UDP_SOCKET;
        rsc.r_contents.cc_host = (char *)"10.0.0.1";
        rsc.r_contents.cc_port = 9004;

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);

        ASSERT_NE(start, end);
        EXPECT_STREQ(get_string_iter_data(start), "udp_socket,10.0.0.1,9004");
        ASSERT_EQ(pal_env_iterator_next(start), end);
    }

    pal_free_env(&env);
}

TEST(handlers, pirate_channel_resource_handler_udp_socket_optional_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        struct resource rsc = {0};
        rsc.r_name = (char *)"my_channel";
        rsc.r_type = (char *)"pirate_channel";
        rsc.r_ids = nullptr;
        rsc.r_ids_count = 0;
        rsc.r_contents.cc_channel_type = UDP_SOCKET;
        rsc.r_contents.cc_mtu = 9002;
        rsc.r_contents.cc_buffer_size = 9003;
        rsc.r_contents.cc_host = (char *)"10.0.0.1";
        rsc.r_contents.cc_port = 9004;

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);

        ASSERT_NE(start, end);
        EXPECT_STREQ(get_string_iter_data(start), "udp_socket,10.0.0.1,"
                "9004,buffer_size=9003,mtu=9002");
        ASSERT_EQ(pal_env_iterator_next(start), end);
    }

    pal_free_env(&env);
}

// Libpirate is compiled with shmem buffers disabled by default
TEST(handlers, DISABLED_pirate_channel_resource_handler_shmem_required_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        struct resource rsc = {0};
        rsc.r_name = (char *)"my_channel";
        rsc.r_type = (char *)"pirate_channel";
        rsc.r_ids = nullptr;
        rsc.r_ids_count = 0;
        rsc.r_contents.cc_channel_type = SHMEM;
        rsc.r_contents.cc_path = (char *)"/foo/bar";

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);

        ASSERT_NE(start, end);
        EXPECT_STREQ(get_string_iter_data(start), "shmem,/foo/bar");
        ASSERT_EQ(pal_env_iterator_next(start), end);
    }

    pal_free_env(&env);
}

// Libpirate is compiled with shmem buffers disabled by default
TEST(handlers, DISABLED_pirate_channel_resource_handler_shmem_optional_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        struct resource rsc = {0};
        rsc.r_name = (char *)"my_channel";
        rsc.r_type = (char *)"pirate_channel";
        rsc.r_ids = nullptr;
        rsc.r_ids_count = 0;
        rsc.r_contents.cc_channel_type = SHMEM;
        rsc.r_contents.cc_path = (char *)"/foo/bar";
        rsc.r_contents.cc_mtu = 9002;
        rsc.r_contents.cc_buffer_size = 9003;
        rsc.r_contents.cc_max_tx_size = 9001;

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);

        ASSERT_NE(start, end);
        EXPECT_STREQ(get_string_iter_data(start), "shmem,/foo/bar,"
                "buffer_size=9003,max_tx_size=9001,mtu=9002");
        ASSERT_EQ(pal_env_iterator_next(start), end);
    }

    pal_free_env(&env);
}

// Libpirate is compiled with udp shmem disabled by default
TEST(handlers, DISABLED_pirate_channel_resource_handler_udp_shmem_required_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        struct resource rsc = {0};
        rsc.r_name = (char *)"my_channel";
        rsc.r_type = (char *)"pirate_channel";
        rsc.r_ids = nullptr;
        rsc.r_ids_count = 0;
        rsc.r_contents.cc_channel_type = UDP_SHMEM;
        rsc.r_contents.cc_path = (char *)"/foo/bar";

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);

        ASSERT_NE(start, end);
        EXPECT_STREQ(get_string_iter_data(start), "udp_shmem,/foo/bar");
        ASSERT_EQ(pal_env_iterator_next(start), end);
    }

    pal_free_env(&env);
}

// Libpirate is compiled with udp shmem disabled by default
TEST(handlers, DISABLED_pirate_channel_resource_handler_udp_shmem_optional_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        struct resource rsc = {0};
        rsc.r_name = (char *)"my_channel";
        rsc.r_type = (char *)"pirate_channel";
        rsc.r_ids = nullptr;
        rsc.r_ids_count = 0;
        rsc.r_contents.cc_channel_type = UDP_SHMEM;
        rsc.r_contents.cc_path = (char *)"/foo/bar";
        rsc.r_contents.cc_mtu = 9002;
        rsc.r_contents.cc_buffer_size = 9003;
        rsc.r_contents.cc_packet_size = 9004;
        rsc.r_contents.cc_packet_count = 9005;

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);

        ASSERT_NE(start, end);
        EXPECT_STREQ(get_string_iter_data(start), "udp_shmem,/foo/bar,"
                "buffer_size=9003,packet_size=9004,packet_count=9005,"
                "mtu=9002");
        ASSERT_EQ(pal_env_iterator_next(start), end);
    }

    pal_free_env(&env);
}

// Libpirate is compiled with uio disabled by default
TEST(handlers, DISABLED_pirate_channel_resource_handler_uio_required_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        struct resource rsc = {0};
        rsc.r_name = (char *)"my_channel";
        rsc.r_type = (char *)"pirate_channel";
        rsc.r_ids = nullptr;
        rsc.r_ids_count = 0;
        rsc.r_contents.cc_channel_type = UIO_DEVICE;

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);

        ASSERT_NE(start, end);
        EXPECT_STREQ(get_string_iter_data(start), "uio");
        ASSERT_EQ(pal_env_iterator_next(start), end);
    }

    pal_free_env(&env);
}

// Libpirate is compiled with uio disabled by default
TEST(handlers, DISABLED_pirate_channel_resource_handler_uio_optional_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        struct resource rsc = {0};
        rsc.r_name = (char *)"my_channel";
        rsc.r_type = (char *)"pirate_channel";
        rsc.r_ids = nullptr;
        rsc.r_ids_count = 0;
        rsc.r_contents.cc_channel_type = UIO_DEVICE;
        rsc.r_contents.cc_path = (char *)"/foo/bar";
        rsc.r_contents.cc_mtu = 9002;
        rsc.r_contents.cc_max_tx_size = 9003;

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);

        ASSERT_NE(start, end);
        EXPECT_STREQ(get_string_iter_data(start), "uio,path=/foo/bar,"
                "max_tx_size=9003,mtu=9002");
        ASSERT_EQ(pal_env_iterator_next(start), end);
    }

    pal_free_env(&env);
}

TEST(handlers, pirate_channel_resource_handler_serial_optional_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        struct resource rsc = {0};
        rsc.r_name = (char *)"my_channel";
        rsc.r_type = (char *)"pirate_channel";
        rsc.r_ids = nullptr;
        rsc.r_ids_count = 0;
        rsc.r_contents.cc_channel_type = SERIAL;
        rsc.r_contents.cc_path = (char *)"/foo/bar";
        rsc.r_contents.cc_baud = B9600;

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);

        ASSERT_NE(start, end);
        EXPECT_STREQ(get_string_iter_data(start), "serial,/foo/bar,baud=9600");
        ASSERT_EQ(pal_env_iterator_next(start), end);
    }

    pal_free_env(&env);
}

TEST(handlers, pirate_channel_resource_handler_serial_required_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        struct resource rsc = {0};
        rsc.r_name = (char *)"my_channel";
        rsc.r_type = (char *)"pirate_channel";
        rsc.r_ids = nullptr;
        rsc.r_ids_count = 0;
        rsc.r_contents.cc_channel_type = SERIAL;
        rsc.r_contents.cc_path = (char *)"/foo/bar";
        rsc.r_contents.cc_mtu = 9002;
        rsc.r_contents.cc_max_tx_size = 9003;
        rsc.r_contents.cc_baud = B9600;

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);

        ASSERT_NE(start, end);
        EXPECT_STREQ(get_string_iter_data(start),
                "serial,/foo/bar,baud=9600,mtu=9002,max_tx_size=9003");
        ASSERT_EQ(pal_env_iterator_next(start), end);
    }

    pal_free_env(&env);
}

TEST(handlers, pirate_channel_resource_handler_mercury_required_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        struct session sess = {0};
        sess.sess_level = 1;
        sess.sess_src_id = 2;
        sess.sess_dst_id = 3;
        sess.sess_messages = nullptr;
        sess.sess_messages_count = 0;

        struct resource rsc = {0};
        rsc.r_name = (char *)"my_channel";
        rsc.r_type = (char *)"pirate_channel";
        rsc.r_ids = nullptr;
        rsc.r_ids_count = 0;
        rsc.r_contents.cc_channel_type = MERCURY;
        rsc.r_contents.cc_session = &sess;

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);

        ASSERT_NE(start, end);
        EXPECT_STREQ(get_string_iter_data(start), "mercury,1,2,3");
        ASSERT_EQ(pal_env_iterator_next(start), end);
    }

    pal_free_env(&env);
}

// Broken due to issues #108 and #109
TEST(handlers, DISABLED_pirate_channel_resource_handler_mercury_optional_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        uint32_t msg_ids[] = {4, 5, 6};

        struct session sess = {0};
        sess.sess_level = 1;
        sess.sess_src_id = 2;
        sess.sess_dst_id = 3;
        sess.sess_messages = msg_ids;
        sess.sess_messages_count = alen(msg_ids);
        sess.sess_id = 7;

        struct resource rsc = {0};
        rsc.r_name = (char *)"my_channel";
        rsc.r_type = (char *)"pirate_channel";
        rsc.r_ids = nullptr;
        rsc.r_ids_count = 0;
        rsc.r_contents.cc_channel_type = MERCURY;
        rsc.r_contents.cc_mtu = 9002;
        rsc.r_contents.cc_session = &sess;

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);

        ASSERT_NE(start, end);
        EXPECT_STREQ(get_string_iter_data(start),
                "mercury,1,2,3,4,5,6,mtu=9002");
        ASSERT_EQ(pal_env_iterator_next(start), end);
    }

    pal_free_env(&env);
}

TEST(handlers, pirate_channel_resource_handler_ge_eth_required_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        struct resource rsc = {0};
        rsc.r_name = (char *)"my_channel";
        rsc.r_type = (char *)"pirate_channel";
        rsc.r_ids = nullptr;
        rsc.r_ids_count = 0;
        rsc.r_contents.cc_channel_type = GE_ETH;
        rsc.r_contents.cc_host = (char *)"10.0.0.1";
        rsc.r_contents.cc_port = 9003;
        rsc.r_contents.cc_message_id = 9004;

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);

        ASSERT_NE(start, end);
        EXPECT_STREQ(get_string_iter_data(start), "ge_eth,10.0.0.1,9003,9004");
        ASSERT_EQ(pal_env_iterator_next(start), end);
    }

    pal_free_env(&env);
}

TEST(handlers, pirate_channel_resource_handler_ge_eth_optional_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        struct resource rsc = {0};
        rsc.r_name = (char *)"my_channel";
        rsc.r_type = (char *)"pirate_channel";
        rsc.r_ids = nullptr;
        rsc.r_ids_count = 0;
        rsc.r_contents.cc_channel_type = GE_ETH;
        rsc.r_contents.cc_mtu = 9002;
        rsc.r_contents.cc_host = (char *)"10.0.0.1";
        rsc.r_contents.cc_port = 9003;
        rsc.r_contents.cc_message_id = 9004;

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);

        ASSERT_NE(start, end);
        EXPECT_STREQ(get_string_iter_data(start),
                "ge_eth,10.0.0.1,9003,9004,mtu=9002");
        ASSERT_EQ(pal_env_iterator_next(start), end);
    }

    pal_free_env(&env);
}
