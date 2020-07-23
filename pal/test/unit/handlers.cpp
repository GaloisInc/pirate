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

    if(data) {
        res = (char *)calloc(size + 1, 1);

        if(res)
            memcpy(res, pal_env_iterator_data(iter), size);
    }

    return res;
}

TEST(handlers, cstring_resource_handler)
{
    char value[] = "blargh";
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        const struct resource rsc = {
            .r_name = (char *)"my_string",
            .r_type = (char *)"string",
            .r_ids = nullptr,
            .r_ids_count = 0,
            .r_contents = { .cc_string_value = value },
        };

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);
        char *data = nullptr;

        ASSERT_NE(start, end);
        ASSERT_EQ(pal_env_iterator_size(start), sizeof(value) - 1);
        ASSERT_STREQ(data = get_string_iter_data(start), value);
        ASSERT_EQ(pal_env_iterator_next(start), end);

        free(data);
    }

    pal_free_env(&env);
}

TEST(handlers, int64_resource_handler)
{
    int64_t value = 9001;
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        const struct resource rsc = {
            .r_name = (char *)"my_integer",
            .r_type = (char *)"integer",
            .r_ids = nullptr,
            .r_ids_count = 0,
            .r_contents = { .cc_integer_value = &value },
        };

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);

        ASSERT_NE(start, end);
        ASSERT_EQ(pal_env_iterator_size(start), sizeof value);
        ASSERT_EQ(*(int64_t *)pal_env_iterator_data(start), value);
        ASSERT_EQ(pal_env_iterator_next(start), end);
    }

    pal_free_env(&env);
}

TEST(handlers, bool_resource_handler)
{
    bool value = true;
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        const struct resource rsc = {
            .r_name = (char *)"my_boolean",
            .r_type = (char *)"boolean",
            .r_ids = nullptr,
            .r_ids_count = 0,
            .r_contents = { .cc_boolean_value = &value },
        };

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);

        ASSERT_NE(start, end);
        ASSERT_EQ(pal_env_iterator_size(start), sizeof value);
        ASSERT_EQ(*(int64_t *)pal_env_iterator_data(start), value);
        ASSERT_EQ(pal_env_iterator_next(start), end);
    }

    pal_free_env(&env);
}

TEST(handlers, file_resource_handler_required_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        const struct resource rsc = {
            .r_name = (char *)"my_file",
            .r_type = (char *)"file",
            .r_ids = nullptr,
            .r_ids_count = 0,
            .r_contents = {
                .cc_file_path = (char *)"/dev/null",
                .cc_file_flags = nullptr,
            },
        };

        run_handler(&env, &rsc);
    }

    ASSERT_EQ(env.fds_count, 1);
    ASSERT_NE(fcntl(env.fds[0], F_GETFL), -1);

    pal_free_env(&env);
}

TEST(handlers, file_resource_handler_optional_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        int flags = O_RDONLY;
        const struct resource rsc = {
            .r_name = (char *)"my_file",
            .r_type = (char *)"file",
            .r_ids = nullptr,
            .r_ids_count = 0,
            .r_contents = {
                .cc_file_path = (char *)"/dev/null",
                .cc_file_flags = &flags,
            },
        };

        run_handler(&env, &rsc);
    }

    ASSERT_EQ(env.fds_count, 1);
    ASSERT_NE(fcntl(env.fds[0], F_GETFL), -1);

    pal_free_env(&env);
}

TEST(handlers, pirate_channel_resource_handler_device_required_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        const struct resource rsc = {
            .r_name = (char *)"my_channel",
            .r_type = (char *)"pirate_channel",
            .r_ids = nullptr,
            .r_ids_count = 0,
            .r_contents = {
                .cc_channel_type = DEVICE,
                .cc_path = (char *)"/foo/bar",
            },
        };

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);
        char *data = nullptr;

        ASSERT_NE(start, end);
        ASSERT_STREQ(data = get_string_iter_data(start), "device,/foo/bar");
        ASSERT_EQ(pal_env_iterator_next(start), end);

        free(data);
    }

    pal_free_env(&env);
}

TEST(handlers, pirate_channel_resource_handler_device_optional_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        const struct resource rsc = {
            .r_name = (char *)"my_channel",
            .r_type = (char *)"pirate_channel",
            .r_ids = nullptr,
            .r_ids_count = 0,
            .r_contents = {
                .cc_channel_type = DEVICE,
                .cc_path = (char *)"/foo/bar",
                .cc_min_tx_size = 9001,
                .cc_mtu = 9002,
            },
        };

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);
        char *data = nullptr;

        ASSERT_NE(start, end);
        ASSERT_STREQ(data = get_string_iter_data(start),
                "device,/foo/bar,min_tx_size=9001,mtu=9002");
        ASSERT_EQ(pal_env_iterator_next(start), end);

        free(data);
    }

    pal_free_env(&env);
}

TEST(handlers, pirate_channel_resource_handler_pipe_required_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        const struct resource rsc = {
            .r_name = (char *)"my_channel",
            .r_type = (char *)"pirate_channel",
            .r_ids = nullptr,
            .r_ids_count = 0,
            .r_contents = {
                .cc_channel_type = PIPE,
                .cc_path = (char *)"/foo/bar",
            },
        };

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);
        char *data = nullptr;

        ASSERT_NE(start, end);
        ASSERT_STREQ(data = get_string_iter_data(start), "pipe,/foo/bar");
        ASSERT_EQ(pal_env_iterator_next(start), end);

        free(data);
    }

    pal_free_env(&env);
}

TEST(handlers, pirate_channel_resource_handler_pipe_optional_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        const struct resource rsc = {
            .r_name = (char *)"my_channel",
            .r_type = (char *)"pirate_channel",
            .r_ids = nullptr,
            .r_ids_count = 0,
            .r_contents = {
                .cc_channel_type = PIPE,
                .cc_path = (char *)"/foo/bar",
                .cc_min_tx_size = 9001,
                .cc_mtu = 9002,
            },
        };

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);
        char *data = nullptr;

        ASSERT_NE(start, end);
        ASSERT_STREQ(data = get_string_iter_data(start),
                "pipe,/foo/bar,min_tx_size=9001,mtu=9002");
        ASSERT_EQ(pal_env_iterator_next(start), end);

        free(data);
    }

    pal_free_env(&env);
}

TEST(handlers, pirate_channel_resource_handler_unix_socket_required_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        const struct resource rsc = {
            .r_name = (char *)"my_channel",
            .r_type = (char *)"pirate_channel",
            .r_ids = nullptr,
            .r_ids_count = 0,
            .r_contents = {
                .cc_channel_type = UNIX_SOCKET,
                .cc_path = (char *)"/foo/bar",
            },
        };

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);
        char *data = nullptr;

        ASSERT_NE(start, end);
        ASSERT_STREQ(data = get_string_iter_data(start),
                "unix_socket,/foo/bar");
        ASSERT_EQ(pal_env_iterator_next(start), end);

        free(data);
    }

    pal_free_env(&env);
}

TEST(handlers, pirate_channel_resource_handler_unix_socket_optional_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        const struct resource rsc = {
            .r_name = (char *)"my_channel",
            .r_type = (char *)"pirate_channel",
            .r_ids = nullptr,
            .r_ids_count = 0,
            .r_contents = {
                .cc_channel_type = UNIX_SOCKET,
                .cc_path = (char *)"/foo/bar",
                .cc_min_tx_size = 9001,
                .cc_mtu = 9002,
                .cc_buffer_size = 9003,
            },
        };

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);
        char *data = nullptr;

        ASSERT_NE(start, end);
        ASSERT_STREQ(data = get_string_iter_data(start),
                "unix_socket,/foo/bar,buffer_size=9003,min_tx_size=9001,"
                "mtu=9002");
        ASSERT_EQ(pal_env_iterator_next(start), end);

        free(data);
    }

    pal_free_env(&env);
}

TEST(handlers, pirate_channel_resource_handler_tcp_socket_required_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        const struct resource rsc = {
            .r_name = (char *)"my_channel",
            .r_type = (char *)"pirate_channel",
            .r_ids = nullptr,
            .r_ids_count = 0,
            .r_contents = {
                .cc_channel_type = TCP_SOCKET,
                .cc_host = (char *)"10.0.0.1",
                .cc_port = 9004,
            },
        };

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);
        char *data = nullptr;

        ASSERT_NE(start, end);
        ASSERT_STREQ(data = get_string_iter_data(start),
                "tcp_socket,10.0.0.1,9004");
        ASSERT_EQ(pal_env_iterator_next(start), end);

        free(data);
    }

    pal_free_env(&env);
}

TEST(handlers, pirate_channel_resource_handler_tcp_socket_optional_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        const struct resource rsc = {
            .r_name = (char *)"my_channel",
            .r_type = (char *)"pirate_channel",
            .r_ids = nullptr,
            .r_ids_count = 0,
            .r_contents = {
                .cc_channel_type = TCP_SOCKET,
                .cc_min_tx_size = 9001,
                .cc_mtu = 9002,
                .cc_buffer_size = 9003,
                .cc_host = (char *)"10.0.0.1",
                .cc_port = 9004,
            },
        };

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);
        char *data = nullptr;

        ASSERT_NE(start, end);
        ASSERT_STREQ(data = get_string_iter_data(start),
                "tcp_socket,10.0.0.1,9004,buffer_size=9003,min_tx_size=9001,"
                "mtu=9002");
        ASSERT_EQ(pal_env_iterator_next(start), end);

        free(data);
    }

    pal_free_env(&env);
}

TEST(handlers, pirate_channel_resource_handler_udp_socket_required_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        const struct resource rsc = {
            .r_name = (char *)"my_channel",
            .r_type = (char *)"pirate_channel",
            .r_ids = nullptr,
            .r_ids_count = 0,
            .r_contents = {
                .cc_channel_type = UDP_SOCKET,
                .cc_host = (char *)"10.0.0.1",
                .cc_port = 9004,
            },
        };

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);
        char *data = nullptr;

        ASSERT_NE(start, end);
        ASSERT_STREQ(data = get_string_iter_data(start),
                "udp_socket,10.0.0.1,9004");
        ASSERT_EQ(pal_env_iterator_next(start), end);

        free(data);
    }

    pal_free_env(&env);
}

TEST(handlers, pirate_channel_resource_handler_udp_socket_optional_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        const struct resource rsc = {
            .r_name = (char *)"my_channel",
            .r_type = (char *)"pirate_channel",
            .r_ids = nullptr,
            .r_ids_count = 0,
            .r_contents = {
                .cc_channel_type = UDP_SOCKET,
                .cc_mtu = 9002,
                .cc_buffer_size = 9003,
                .cc_host = (char *)"10.0.0.1",
                .cc_port = 9004,
            },
        };

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);
        char *data = nullptr;

        ASSERT_NE(start, end);
        ASSERT_STREQ(data = get_string_iter_data(start),
                "udp_socket,10.0.0.1,9004,buffer_size=9003,mtu=9002");
        ASSERT_EQ(pal_env_iterator_next(start), end);

        free(data);
    }

    pal_free_env(&env);
}

// Libpirate is compiled with shmem buffers disabled by default
TEST(handlers, DISABLED_pirate_channel_resource_handler_shmem_required_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        const struct resource rsc = {
            .r_name = (char *)"my_channel",
            .r_type = (char *)"pirate_channel",
            .r_ids = nullptr,
            .r_ids_count = 0,
            .r_contents = {
                .cc_channel_type = SHMEM,
                .cc_path = (char *)"/foo/bar",
            },
        };

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);
        char *data = nullptr;

        ASSERT_NE(start, end);
        ASSERT_STREQ(data = get_string_iter_data(start), "shmem,/foo/bar");
        ASSERT_EQ(pal_env_iterator_next(start), end);

        free(data);
    }

    pal_free_env(&env);
}

// Libpirate is compiled with shmem buffers disabled by default
TEST(handlers, DISABLED_pirate_channel_resource_handler_shmem_optional_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        const struct resource rsc = {
            .r_name = (char *)"my_channel",
            .r_type = (char *)"pirate_channel",
            .r_ids = nullptr,
            .r_ids_count = 0,
            .r_contents = {
                .cc_channel_type = SHMEM,
                .cc_path = (char *)"/foo/bar",
                .cc_mtu = 9002,
                .cc_buffer_size = 9003,
                .cc_max_tx_size = 9001,
            },
        };

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);
        char *data = nullptr;

        ASSERT_NE(start, end);
        ASSERT_STREQ(data = get_string_iter_data(start),
                "shmem,/foo/bar,buffer_size=9003,max_tx_size=9001,mtu=9002");
        ASSERT_EQ(pal_env_iterator_next(start), end);

        free(data);
    }

    pal_free_env(&env);
}

// Libpirate is compiled with udp shmem disabled by default
TEST(handlers, DISABLED_pirate_channel_resource_handler_udp_shmem_required_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        const struct resource rsc = {
            .r_name = (char *)"my_channel",
            .r_type = (char *)"pirate_channel",
            .r_ids = nullptr,
            .r_ids_count = 0,
            .r_contents = {
                .cc_channel_type = UDP_SHMEM,
                .cc_path = (char *)"/foo/bar",
            },
        };

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);
        char *data = nullptr;

        ASSERT_NE(start, end);
        ASSERT_STREQ(data = get_string_iter_data(start), "udp_shmem,/foo/bar");
        ASSERT_EQ(pal_env_iterator_next(start), end);

        free(data);
    }

    pal_free_env(&env);
}

// Libpirate is compiled with udp shmem disabled by default
TEST(handlers, DISABLED_pirate_channel_resource_handler_udp_shmem_optional_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        const struct resource rsc = {
            .r_name = (char *)"my_channel",
            .r_type = (char *)"pirate_channel",
            .r_ids = nullptr,
            .r_ids_count = 0,
            .r_contents = {
                .cc_channel_type = UDP_SHMEM,
                .cc_path = (char *)"/foo/bar",
                .cc_mtu = 9002,
                .cc_buffer_size = 9003,
                .cc_packet_size = 9004,
                .cc_packet_count = 9005,
            },
        };

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);
        char *data = nullptr;

        ASSERT_NE(start, end);
        ASSERT_STREQ(data = get_string_iter_data(start),
                "udp_shmem,/foo/bar,buffer_size=9003,packet_size=9004,"
                "packet_count=9005,mtu=9002");
        ASSERT_EQ(pal_env_iterator_next(start), end);

        free(data);
    }

    pal_free_env(&env);
}

// Libpirate is compiled with uio disabled by default
TEST(handlers, DISABLED_pirate_channel_resource_handler_uio_required_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        const struct resource rsc = {
            .r_name = (char *)"my_channel",
            .r_type = (char *)"pirate_channel",
            .r_ids = nullptr,
            .r_ids_count = 0,
            .r_contents = {
                .cc_channel_type = UIO_DEVICE,
            },
        };

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);
        char *data = nullptr;

        ASSERT_NE(start, end);
        ASSERT_STREQ(data = get_string_iter_data(start), "uio");
        ASSERT_EQ(pal_env_iterator_next(start), end);

        free(data);
    }

    pal_free_env(&env);
}

// Libpirate is compiled with uio disabled by default
TEST(handlers, DISABLED_pirate_channel_resource_handler_uio_optional_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        const struct resource rsc = {
            .r_name = (char *)"my_channel",
            .r_type = (char *)"pirate_channel",
            .r_ids = nullptr,
            .r_ids_count = 0,
            .r_contents = {
                .cc_channel_type = UIO_DEVICE,
                .cc_path = (char *)"/foo/bar",
                .cc_mtu = 9002,
                .cc_max_tx_size = 9003,
            },
        };

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);
        char *data = nullptr;

        ASSERT_NE(start, end);
        ASSERT_STREQ(data = get_string_iter_data(start),
                "uio,path=/foo/bar,max_tx_size=9003,mtu=9002");
        ASSERT_EQ(pal_env_iterator_next(start), end);

        free(data);
    }

    pal_free_env(&env);
}

TEST(handlers, pirate_channel_resource_handler_serial_optional_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        const struct resource rsc = {
            .r_name = (char *)"my_channel",
            .r_type = (char *)"pirate_channel",
            .r_ids = nullptr,
            .r_ids_count = 0,
            .r_contents = {
                .cc_channel_type = SERIAL,
                .cc_path = (char *)"/foo/bar",
                .cc_baud = B9600,
            },
        };

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);
        char *data = nullptr;

        ASSERT_NE(start, end);
        ASSERT_STREQ(data = get_string_iter_data(start),
                "serial,/foo/bar,baud=9600");
        ASSERT_EQ(pal_env_iterator_next(start), end);

        free(data);
    }

    pal_free_env(&env);
}

TEST(handlers, pirate_channel_resource_handler_serial_required_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        const struct resource rsc = {
            .r_name = (char *)"my_channel",
            .r_type = (char *)"pirate_channel",
            .r_ids = nullptr,
            .r_ids_count = 0,
            .r_contents = {
                .cc_channel_type = SERIAL,
                .cc_path = (char *)"/foo/bar",
                .cc_mtu = 9002,
                .cc_max_tx_size = 9003,
                .cc_baud = B9600,
            },
        };

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);
        char *data = nullptr;

        ASSERT_NE(start, end);
        ASSERT_STREQ(data = get_string_iter_data(start),
                "serial,/foo/bar,baud=9600,mtu=9002,max_tx_size=9003");
        ASSERT_EQ(pal_env_iterator_next(start), end);

        free(data);
    }

    pal_free_env(&env);
}

TEST(handlers, pirate_channel_resource_handler_mercury_required_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        struct session sess = {
            .sess_level = 1,
            .sess_src_id = 2,
            .sess_dst_id = 3,
            .sess_messages = nullptr,
            .sess_messages_count = 0,
        };
        const struct resource rsc = {
            .r_name = (char *)"my_channel",
            .r_type = (char *)"pirate_channel",
            .r_ids = nullptr,
            .r_ids_count = 0,
            .r_contents = {
                .cc_channel_type = MERCURY,
                .cc_session = &sess,
            },
        };

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);
        char *data = nullptr;

        ASSERT_NE(start, end);
        ASSERT_STREQ(data = get_string_iter_data(start),
                "mercury,1,2,3");
        ASSERT_EQ(pal_env_iterator_next(start), end);

        free(data);
    }

    pal_free_env(&env);
}

// Broken due to issue #XXX
// TODO: Make an issue
TEST(handlers, DISABLED_pirate_channel_resource_handler_mercury_optional_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        uint32_t msg_ids[] = {4, 5, 6};
        struct session sess = {
            .sess_level = 1,
            .sess_src_id = 2,
            .sess_dst_id = 3,
            .sess_messages = msg_ids,
            .sess_messages_count = alen(msg_ids),
            .sess_id = 7,
        };
        const struct resource rsc = {
            .r_name = (char *)"my_channel",
            .r_type = (char *)"pirate_channel",
            .r_ids = nullptr,
            .r_ids_count = 0,
            .r_contents = {
                .cc_channel_type = MERCURY,
                .cc_mtu = 9002,
                .cc_session = &sess,
            },
        };

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);
        char *data = nullptr;

        ASSERT_NE(start, end);
        ASSERT_STREQ(data = get_string_iter_data(start),
                "mercury,1,2,3,4,5,6,mtu=9002");
        ASSERT_EQ(pal_env_iterator_next(start), end);

        free(data);
    }

    pal_free_env(&env);
}

TEST(handlers, pirate_channel_resource_handler_ge_eth_required_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        const struct resource rsc = {
            .r_name = (char *)"my_channel",
            .r_type = (char *)"pirate_channel",
            .r_ids = nullptr,
            .r_ids_count = 0,
            .r_contents = {
                .cc_channel_type = GE_ETH,
                .cc_host = (char *)"10.0.0.1",
                .cc_port = 9003,
                .cc_message_id = 9004,
            },
        };

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);
        char *data = nullptr;

        ASSERT_NE(start, end);
        ASSERT_STREQ(data = get_string_iter_data(start),
                "ge_eth,10.0.0.1,9003,9004");
        ASSERT_EQ(pal_env_iterator_next(start), end);

        free(data);
    }

    pal_free_env(&env);
}

TEST(handlers, pirate_channel_resource_handler_ge_eth_optional_args)
{
    pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);

    {
        const struct resource rsc = {
            .r_name = (char *)"my_channel",
            .r_type = (char *)"pirate_channel",
            .r_ids = nullptr,
            .r_ids_count = 0,
            .r_contents = {
                .cc_channel_type = GE_ETH,
                .cc_mtu = 9002,
                .cc_host = (char *)"10.0.0.1",
                .cc_port = 9003,
                .cc_message_id = 9004,
            },
        };

        run_handler(&env, &rsc);
    }

    {
        pal_env_iterator_t start = pal_env_iterator_start(&env);
        pal_env_iterator_t end = pal_env_iterator_end(&env);
        char *data = nullptr;

        ASSERT_NE(start, end);
        ASSERT_STREQ(data = get_string_iter_data(start),
                "ge_eth,10.0.0.1,9003,9004,mtu=9002");
        ASSERT_EQ(pal_env_iterator_next(start), end);

        free(data);
    }

    pal_free_env(&env);
}
