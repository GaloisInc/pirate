#include <sys/types.h>

#include "gtest/gtest.h"

extern "C" {
#include "yaml.h"
}
#include "pal/pal.h"

#define alen(_arr) (sizeof (_arr) / sizeof *(_arr))

class pal: public ::testing::Test {
    protected:

        int client_fd, server_fd;
        struct top_level *tlp;

        pal() : client_fd(0), server_fd(0), tlp(nullptr) {}

        void SetUp() override
        {
            char client_fd_string[64];
            int fds[2];

            ASSERT_EQ(socketpair(AF_UNIX, SOCK_DGRAM, 0, fds), 0);
            server_fd = fds[0];
            client_fd = fds[1];
            ASSERT_GT(snprintf(client_fd_string, sizeof client_fd_string,
                        "%d", client_fd), 0);

            ASSERT_EQ(setenv("PAL_FD", client_fd_string, 1), 0);
        }

        void TearDown() override
        {
            free_yaml(tlp);
            close(client_fd);
            close(server_fd);
        }
};

TEST_F(pal, get_pal_fd)
{
    EXPECT_EQ(get_pal_fd(), client_fd);
}

TEST_F(pal, lookup_pirate_resource_param)
{
    struct pirate_resource_param prp[5] = {
        { (char *)"bleep", (char *)"bloop"   },
        { (char *)"foo",   (char *)"bar"     },
        { (char *)"fizz",  (char *)"bang"    },
        { (char *)"fizz",  (char *)"whimper" },
        { (char *)"wiz",   (char *)"woz"     },
    };
    struct pirate_resource pr {
        .pr_name       = (char *)"baz",
        .pr_obj        = nullptr,
        .pr_params     = prp,
        .pr_params_len = alen(prp),
    };

    EXPECT_STREQ(lookup_pirate_resource_param(&pr, "bleep"), "bloop");
    EXPECT_STREQ(lookup_pirate_resource_param(&pr, "fizz"),  "bang");
    EXPECT_STREQ(lookup_pirate_resource_param(&pr, "wiz"),   "woz");
}

// TODO: The following will require threads.

TEST_F(pal, DISABLED_get_boolean_res)
{   // TODO
}

TEST_F(pal, DISABLED_get_integer_res)
{   // TODO
}

TEST_F(pal, DISABLED_get_string_res)
{   // TODO
}

TEST_F(pal, DISABLED_get_file_res)
{   // TODO
}

TEST_F(pal, DISABLED_get_pirate_channel_cfg)
{   // TODO
}
