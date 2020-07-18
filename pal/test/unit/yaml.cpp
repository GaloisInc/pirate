#include "gtest/gtest.h"

extern "C" {
#include "yaml.h"
}

static struct top_level *load_yaml_string(const char *yaml)
{
    char *fname;
    FILE *fp;
    struct top_level *tlp = nullptr;

    if((fname = tempnam("/tmp", "pal")) == nullptr)
        return nullptr;
    else if((fp = fopen(fname, "w")) == nullptr)
        return nullptr;
    else if(fputs(yaml, fp) == EOF)
        return nullptr;
    else if(fclose(fp) != 0)
        return nullptr;
    else if((tlp = load_yaml(fname)) == nullptr)
        return nullptr;
    else if(unlink(fname) != 0)
        return nullptr;

    return tlp;
}

TEST(yaml, load_yaml_enclaves)
{
    struct top_level *tlp;

    ASSERT_NE(
        tlp = load_yaml_string(
            "enclaves:\n"
            "  - name: foo\n"
            "    path: bar\n"
            "    arguments: [\"zip\", \"zap\", \"zot\"]\n"
            "    environment: [\"wiz=woz\", \"bleep=bloop\"]\n"
            "  - name: bat\n"
            "    path: baz\n"
        ), nullptr);

    ASSERT_EQ(tlp->tl_encs_count, 2);
    EXPECT_STREQ(tlp->tl_encs[0].enc_name, "foo");
    EXPECT_STREQ(tlp->tl_encs[0].enc_path, "bar");
    ASSERT_EQ(tlp->tl_encs[0].enc_args_count, 3);
    EXPECT_STREQ(tlp->tl_encs[0].enc_args[0], "zip");
    EXPECT_STREQ(tlp->tl_encs[0].enc_args[1], "zap");
    EXPECT_STREQ(tlp->tl_encs[0].enc_args[2], "zot");
    ASSERT_EQ(tlp->tl_encs[0].enc_env_count, 2);
    EXPECT_STREQ(tlp->tl_encs[0].enc_env[0], "wiz=woz");
    EXPECT_STREQ(tlp->tl_encs[0].enc_env[1], "bleep=bloop");
    EXPECT_STREQ(tlp->tl_encs[1].enc_name, "bat");
    EXPECT_STREQ(tlp->tl_encs[1].enc_path, "baz");
    EXPECT_EQ(tlp->tl_encs[1].enc_args_count, 0);
    EXPECT_EQ(tlp->tl_encs[1].enc_env_count, 0);

    free_yaml(tlp);
}

TEST(yaml, load_yaml_string_resource)
{
    struct top_level *tlp;

    ASSERT_NE(
        tlp = load_yaml_string(
            "enclaves:\n"
            "  - name: a\n"
            "  - name: b\n"
            "resources:\n"
            "  - name: my_string\n"
            "    type: string\n"
            "    ids: [\"a/a_string\", \"b/b_string\"]\n"
            "    contents:\n"
            "      string_value: blargh\n"
        ), nullptr);

    ASSERT_EQ(tlp->tl_rscs_count, 1);
    EXPECT_STREQ(tlp->tl_rscs[0].r_name, "my_string");
    EXPECT_STREQ(tlp->tl_rscs[0].r_type, "string");
    ASSERT_EQ(tlp->tl_rscs[0].r_ids_count, 2);
    EXPECT_STREQ(tlp->tl_rscs[0].r_ids[0], "a/a_string");
    EXPECT_STREQ(tlp->tl_rscs[0].r_ids[1], "b/b_string");
    EXPECT_STREQ(tlp->tl_rscs[0].r_contents.cc_string_value, "blargh");

    free_yaml(tlp);
}

TEST(yaml, load_yaml_integer_resource)
{
    struct top_level *tlp;

    ASSERT_NE(
        tlp = load_yaml_string(
            "enclaves:\n"
            "  - name: a\n"
            "  - name: b\n"
            "resources:\n"
            "  - name: my_integer\n"
            "    type: integer\n"
            "    ids: [\"a/a_integer\", \"b/b_integer\"]\n"
            "    contents:\n"
            "      integer_value: 9001\n"
        ), nullptr);

    ASSERT_EQ(tlp->tl_rscs_count, 1);
    EXPECT_STREQ(tlp->tl_rscs[0].r_name, "my_integer");
    EXPECT_STREQ(tlp->tl_rscs[0].r_type, "integer");
    ASSERT_EQ(tlp->tl_rscs[0].r_ids_count, 2);
    EXPECT_STREQ(tlp->tl_rscs[0].r_ids[0], "a/a_integer");
    EXPECT_STREQ(tlp->tl_rscs[0].r_ids[1], "b/b_integer");
    EXPECT_EQ(*tlp->tl_rscs[0].r_contents.cc_integer_value, 9001);

    free_yaml(tlp);
}

TEST(yaml, load_yaml_boolean_resource)
{
    struct top_level *tlp;

    ASSERT_NE(
        tlp = load_yaml_string(
            "enclaves:\n"
            "  - name: a\n"
            "  - name: b\n"
            "resources:\n"
            "  - name: my_boolean\n"
            "    type: boolean\n"
            "    ids: [\"a/a_boolean\", \"b/b_boolean\"]\n"
            "    contents:\n"
            "      boolean_value: true\n"
        ), nullptr);

    ASSERT_EQ(tlp->tl_rscs_count, 1);
    EXPECT_STREQ(tlp->tl_rscs[0].r_name, "my_boolean");
    EXPECT_STREQ(tlp->tl_rscs[0].r_type, "boolean");
    ASSERT_EQ(tlp->tl_rscs[0].r_ids_count, 2);
    EXPECT_STREQ(tlp->tl_rscs[0].r_ids[0], "a/a_boolean");
    EXPECT_STREQ(tlp->tl_rscs[0].r_ids[1], "b/b_boolean");
    EXPECT_EQ(*tlp->tl_rscs[0].r_contents.cc_boolean_value, true);

    free_yaml(tlp);
}

TEST(yaml, load_yaml_cfg)
{
    struct top_level *tlp;

    ASSERT_NE(
        tlp = load_yaml_string(
            "enclaves:\n"
            "  - name: foo\n"
            "config:\n"
            "  log_level: info\n"
        ), nullptr);

    EXPECT_EQ(tlp->tl_cfg.cfg_loglvl, LOGLVL_INFO);

    free_yaml(tlp);
}

TEST(yaml, load_yaml_multi_resources)
{
    struct top_level *tlp;

    ASSERT_NE(
        tlp = load_yaml_string(
            "enclaves:\n"
            "  - name: a\n"
            "  - name: b\n"
            "resources:\n"
            "  - name: my_string\n"
            "    type: string\n"
            "    ids: [\"a/a_string\", \"b/b_string\"]\n"
            "    contents:\n"
            "      string_value: blargh\n"
            "  - name: my_integer\n"
            "    type: integer\n"
            "    ids: [\"a/a_integer\", \"b/b_integer\"]\n"
            "    contents:\n"
            "      integer_value: 9001\n"
            "  - name: my_boolean\n"
            "    type: boolean\n"
            "    ids: [\"a/a_boolean\", \"b/b_boolean\"]\n"
            "    contents:\n"
            "      boolean_value: true\n"
        ), nullptr);

    ASSERT_EQ(tlp->tl_rscs_count, 3);

    EXPECT_STREQ(tlp->tl_rscs[0].r_name, "my_string");
    EXPECT_STREQ(tlp->tl_rscs[0].r_type, "string");
    ASSERT_EQ(tlp->tl_rscs[0].r_ids_count, 2);
    EXPECT_STREQ(tlp->tl_rscs[0].r_ids[0], "a/a_string");
    EXPECT_STREQ(tlp->tl_rscs[0].r_ids[1], "b/b_string");
    EXPECT_STREQ(tlp->tl_rscs[0].r_contents.cc_string_value, "blargh");

    EXPECT_STREQ(tlp->tl_rscs[1].r_name, "my_integer");
    EXPECT_STREQ(tlp->tl_rscs[1].r_type, "integer");
    ASSERT_EQ(tlp->tl_rscs[1].r_ids_count, 2);
    EXPECT_STREQ(tlp->tl_rscs[1].r_ids[0], "a/a_integer");
    EXPECT_STREQ(tlp->tl_rscs[1].r_ids[1], "b/b_integer");
    EXPECT_EQ(*tlp->tl_rscs[1].r_contents.cc_integer_value, 9001);

    EXPECT_STREQ(tlp->tl_rscs[2].r_name, "my_boolean");
    EXPECT_STREQ(tlp->tl_rscs[2].r_type, "boolean");
    ASSERT_EQ(tlp->tl_rscs[2].r_ids_count, 2);
    EXPECT_STREQ(tlp->tl_rscs[2].r_ids[0], "a/a_boolean");
    EXPECT_STREQ(tlp->tl_rscs[2].r_ids[1], "b/b_boolean");
    EXPECT_EQ(*tlp->tl_rscs[2].r_contents.cc_boolean_value, true);

    free_yaml(tlp);
}

TEST(yaml, load_yaml_file)
{
    struct top_level *tlp;

    ASSERT_NE(
        tlp = load_yaml_string(
            "enclaves:\n"
            "  - name: a\n"
            "  - name: b\n"
            "resources:\n"
            "  - name: my_file\n"
            "    type: file\n"
            "    ids: [\"a/a_file\", \"b/b_file\"]\n"
            "    contents:\n"
            "      file_path: blargh\n"
            "      file_flags: [ O_RDWR, O_EXCL ]\n"
        ), nullptr);

    ASSERT_EQ(tlp->tl_rscs_count, 1);
    EXPECT_STREQ(tlp->tl_rscs[0].r_name, "my_file");
    EXPECT_STREQ(tlp->tl_rscs[0].r_type, "file");
    ASSERT_EQ(tlp->tl_rscs[0].r_ids_count, 2);
    EXPECT_STREQ(tlp->tl_rscs[0].r_ids[0], "a/a_file");
    EXPECT_STREQ(tlp->tl_rscs[0].r_ids[1], "b/b_file");
    EXPECT_STREQ(tlp->tl_rscs[0].r_contents.cc_file_path, "blargh");
    EXPECT_EQ(*tlp->tl_rscs[0].r_contents.cc_file_flags, O_RDWR | O_EXCL);

    free_yaml(tlp);
}

TEST(yaml, load_yaml_channel_device)
{
    struct top_level *tlp;

    ASSERT_NE(
        tlp = load_yaml_string(
            "enclaves:\n"
            "  - name: a\n"
            "resources:\n"
            "  - name: my_channel\n"
            "    type: pirate_channel\n"
            "    ids: [\"a/a_channel\"]\n"
            "    contents:\n"
            "      channel_type: device\n"
            "      path: blargh\n"
            "      min_tx_size: 512\n"
            "      mtu: 1024\n"
        ), nullptr);

    ASSERT_EQ(tlp->tl_rscs_count, 1);
    EXPECT_STREQ(tlp->tl_rscs[0].r_name, "my_channel");
    EXPECT_STREQ(tlp->tl_rscs[0].r_type, "pirate_channel");
    ASSERT_EQ(tlp->tl_rscs[0].r_ids_count, 1);
    EXPECT_STREQ(tlp->tl_rscs[0].r_ids[0], "a/a_channel");
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_channel_type, DEVICE);
    EXPECT_STREQ(tlp->tl_rscs[0].r_contents.cc_path, "blargh");
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_min_tx_size, 512);
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_mtu, 1024);

    free_yaml(tlp);
}

TEST(yaml, load_yaml_channel_pipe)
{
    struct top_level *tlp;

    ASSERT_NE(
        tlp = load_yaml_string(
            "enclaves:\n"
            "  - name: a\n"
            "resources:\n"
            "  - name: my_channel\n"
            "    type: pirate_channel\n"
            "    ids: [\"a/a_channel\"]\n"
            "    contents:\n"
            "      channel_type: pipe\n"
            "      path: blargh\n"
            "      min_tx_size: 512\n"
            "      mtu: 1024\n"
        ), nullptr);

    ASSERT_EQ(tlp->tl_rscs_count, 1);
    EXPECT_STREQ(tlp->tl_rscs[0].r_name, "my_channel");
    EXPECT_STREQ(tlp->tl_rscs[0].r_type, "pirate_channel");
    ASSERT_EQ(tlp->tl_rscs[0].r_ids_count, 1);
    EXPECT_STREQ(tlp->tl_rscs[0].r_ids[0], "a/a_channel");
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_channel_type, PIPE);
    EXPECT_STREQ(tlp->tl_rscs[0].r_contents.cc_path, "blargh");
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_min_tx_size, 512);
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_mtu, 1024);

    free_yaml(tlp);
}

TEST(yaml, load_yaml_channel_unix_socket)
{
    struct top_level *tlp;

    ASSERT_NE(
        tlp = load_yaml_string(
            "enclaves:\n"
            "  - name: a\n"
            "resources:\n"
            "  - name: my_channel\n"
            "    type: pirate_channel\n"
            "    ids: [\"a/a_channel\"]\n"
            "    contents:\n"
            "      channel_type: unix_socket\n"
            "      path: blargh\n"
            "      min_tx_size: 512\n"
            "      mtu: 1024\n"
            "      buffer_size: 2048\n"
        ), nullptr);

    ASSERT_EQ(tlp->tl_rscs_count, 1);
    EXPECT_STREQ(tlp->tl_rscs[0].r_name, "my_channel");
    EXPECT_STREQ(tlp->tl_rscs[0].r_type, "pirate_channel");
    ASSERT_EQ(tlp->tl_rscs[0].r_ids_count, 1);
    EXPECT_STREQ(tlp->tl_rscs[0].r_ids[0], "a/a_channel");
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_channel_type, UNIX_SOCKET);
    EXPECT_STREQ(tlp->tl_rscs[0].r_contents.cc_path, "blargh");
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_min_tx_size, 512);
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_mtu, 1024);
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_buffer_size, 2048);

    free_yaml(tlp);
}

TEST(yaml, load_yaml_channel_tcp_socket)
{
    struct top_level *tlp;

    ASSERT_NE(
        tlp = load_yaml_string(
            "enclaves:\n"
            "  - name: a\n"
            "resources:\n"
            "  - name: my_channel\n"
            "    type: pirate_channel\n"
            "    ids: [\"a/a_channel\"]\n"
            "    contents:\n"
            "      channel_type: tcp_socket\n"
            "      min_tx_size: 512\n"
            "      mtu: 1024\n"
            "      buffer_size: 2048\n"
            "      host: 10.0.0.1\n"
            "      port: 9001\n"
        ), nullptr);

    ASSERT_EQ(tlp->tl_rscs_count, 1);
    EXPECT_STREQ(tlp->tl_rscs[0].r_name, "my_channel");
    EXPECT_STREQ(tlp->tl_rscs[0].r_type, "pirate_channel");
    ASSERT_EQ(tlp->tl_rscs[0].r_ids_count, 1);
    EXPECT_STREQ(tlp->tl_rscs[0].r_ids[0], "a/a_channel");
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_channel_type, TCP_SOCKET);
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_min_tx_size, 512);
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_mtu, 1024);
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_buffer_size, 2048);
    EXPECT_STREQ(tlp->tl_rscs[0].r_contents.cc_host, "10.0.0.1");
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_port, 9001);

    free_yaml(tlp);
}

TEST(yaml, load_yaml_channel_udp_socket)
{
    struct top_level *tlp;

    ASSERT_NE(
        tlp = load_yaml_string(
            "enclaves:\n"
            "  - name: a\n"
            "resources:\n"
            "  - name: my_channel\n"
            "    type: pirate_channel\n"
            "    ids: [\"a/a_channel\"]\n"
            "    contents:\n"
            "      channel_type: udp_socket\n"
            "      mtu: 1024\n"
            "      buffer_size: 2048\n"
            "      host: 10.0.0.1\n"
            "      port: 9001\n"
        ), nullptr);

    ASSERT_EQ(tlp->tl_rscs_count, 1);
    EXPECT_STREQ(tlp->tl_rscs[0].r_name, "my_channel");
    EXPECT_STREQ(tlp->tl_rscs[0].r_type, "pirate_channel");
    ASSERT_EQ(tlp->tl_rscs[0].r_ids_count, 1);
    EXPECT_STREQ(tlp->tl_rscs[0].r_ids[0], "a/a_channel");
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_channel_type, UDP_SOCKET);
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_mtu, 1024);
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_buffer_size, 2048);
    EXPECT_STREQ(tlp->tl_rscs[0].r_contents.cc_host, "10.0.0.1");
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_port, 9001);

    free_yaml(tlp);
}

TEST(yaml, load_yaml_channel_shmem)
{
    struct top_level *tlp;

    ASSERT_NE(
        tlp = load_yaml_string(
            "enclaves:\n"
            "  - name: a\n"
            "resources:\n"
            "  - name: my_channel\n"
            "    type: pirate_channel\n"
            "    ids: [\"a/a_channel\"]\n"
            "    contents:\n"
            "      channel_type: shmem\n"
            "      path: blargh\n"
            "      mtu: 1024\n"
            "      buffer_size: 2048\n"
            "      max_tx_size: 1024\n"
        ), nullptr);

    ASSERT_EQ(tlp->tl_rscs_count, 1);
    EXPECT_STREQ(tlp->tl_rscs[0].r_name, "my_channel");
    EXPECT_STREQ(tlp->tl_rscs[0].r_type, "pirate_channel");
    ASSERT_EQ(tlp->tl_rscs[0].r_ids_count, 1);
    EXPECT_STREQ(tlp->tl_rscs[0].r_ids[0], "a/a_channel");
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_channel_type, SHMEM);
    EXPECT_STREQ(tlp->tl_rscs[0].r_contents.cc_path, "blargh");
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_mtu, 1024);
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_buffer_size, 2048);
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_max_tx_size, 1024);

    free_yaml(tlp);
}

TEST(yaml, load_yaml_channel_udp_shmem)
{
    struct top_level *tlp;

    ASSERT_NE(
        tlp = load_yaml_string(
            "enclaves:\n"
            "  - name: a\n"
            "resources:\n"
            "  - name: my_channel\n"
            "    type: pirate_channel\n"
            "    ids: [\"a/a_channel\"]\n"
            "    contents:\n"
            "      channel_type: udp_shmem\n"
            "      path: blargh\n"
            "      mtu: 1024\n"
            "      buffer_size: 2048\n"
            "      packet_size: 1024\n"
            "      packet_count: 2\n"
        ), nullptr);

    ASSERT_EQ(tlp->tl_rscs_count, 1);
    EXPECT_STREQ(tlp->tl_rscs[0].r_name, "my_channel");
    EXPECT_STREQ(tlp->tl_rscs[0].r_type, "pirate_channel");
    ASSERT_EQ(tlp->tl_rscs[0].r_ids_count, 1);
    EXPECT_STREQ(tlp->tl_rscs[0].r_ids[0], "a/a_channel");
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_channel_type, UDP_SHMEM);
    EXPECT_STREQ(tlp->tl_rscs[0].r_contents.cc_path, "blargh");
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_mtu, 1024);
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_buffer_size, 2048);
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_packet_size, 1024);
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_packet_count, 2);

    free_yaml(tlp);
}

TEST(yaml, load_yaml_channel_uio)
{
    struct top_level *tlp;

    ASSERT_NE(
        tlp = load_yaml_string(
            "enclaves:\n"
            "  - name: a\n"
            "resources:\n"
            "  - name: my_channel\n"
            "    type: pirate_channel\n"
            "    ids: [\"a/a_channel\"]\n"
            "    contents:\n"
            "      channel_type: uio\n"
            "      path: blargh\n"
            "      mtu: 1024\n"
            "      max_tx_size: 2048\n"
            "      region: 1\n"
        ), nullptr);

    ASSERT_EQ(tlp->tl_rscs_count, 1);
    EXPECT_STREQ(tlp->tl_rscs[0].r_name, "my_channel");
    EXPECT_STREQ(tlp->tl_rscs[0].r_type, "pirate_channel");
    ASSERT_EQ(tlp->tl_rscs[0].r_ids_count, 1);
    EXPECT_STREQ(tlp->tl_rscs[0].r_ids[0], "a/a_channel");
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_channel_type, UIO_DEVICE);
    EXPECT_STREQ(tlp->tl_rscs[0].r_contents.cc_path, "blargh");
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_mtu, 1024);
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_max_tx_size,2048);
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_region, 1);

    free_yaml(tlp);
}

TEST(yaml, load_yaml_channel_serial)
{
    struct top_level *tlp;

    ASSERT_NE(
        tlp = load_yaml_string(
            "enclaves:\n"
            "  - name: a\n"
            "resources:\n"
            "  - name: my_channel\n"
            "    type: pirate_channel\n"
            "    ids: [\"a/a_channel\"]\n"
            "    contents:\n"
            "      channel_type: serial\n"
            "      path: blargh\n"
            "      mtu: 1024\n"
            "      max_tx_size: 2048\n"
            "      baud: 9600\n"
        ), nullptr);

    ASSERT_EQ(tlp->tl_rscs_count, 1);
    EXPECT_STREQ(tlp->tl_rscs[0].r_name, "my_channel");
    EXPECT_STREQ(tlp->tl_rscs[0].r_type, "pirate_channel");
    ASSERT_EQ(tlp->tl_rscs[0].r_ids_count, 1);
    EXPECT_STREQ(tlp->tl_rscs[0].r_ids[0], "a/a_channel");
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_channel_type, SERIAL);
    EXPECT_STREQ(tlp->tl_rscs[0].r_contents.cc_path, "blargh");
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_mtu, 1024);
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_max_tx_size,2048);
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_baud, 9600);

    free_yaml(tlp);
}

TEST(yaml, load_yaml_channel_mercury)
{
    struct top_level *tlp;

    ASSERT_NE(
        tlp = load_yaml_string(
            "enclaves:\n"
            "  - name: a\n"
            "resources:\n"
            "  - name: my_channel\n"
            "    type: pirate_channel\n"
            "    ids: [\"a/a_channel\"]\n"
            "    contents:\n"
            "      channel_type: mercury\n"
            "      mtu: 1024\n"
            "      session:\n"
            "        level: 1\n"
            "        src_id: 2\n"
            "        dst_id: 3\n"
            "        messages: [4, 5, 6]\n"
            "        id: 7\n"
        ), nullptr);

    ASSERT_EQ(tlp->tl_rscs_count, 1);
    EXPECT_STREQ(tlp->tl_rscs[0].r_name, "my_channel");
    EXPECT_STREQ(tlp->tl_rscs[0].r_type, "pirate_channel");
    ASSERT_EQ(tlp->tl_rscs[0].r_ids_count, 1);
    EXPECT_STREQ(tlp->tl_rscs[0].r_ids[0], "a/a_channel");
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_channel_type, MERCURY);
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_mtu, 1024);
    EXPECT_NE(tlp->tl_rscs[0].r_contents.cc_session, nullptr);
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_session->sess_level, 1);
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_session->sess_src_id, 2);
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_session->sess_dst_id, 3);
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_session->sess_messages_count, 3);
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_session->sess_messages[0], 4);
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_session->sess_messages[1], 5);
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_session->sess_messages[2], 6);
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_session->sess_id, 7);

    free_yaml(tlp);
}

TEST(yaml, load_yaml_channel_ge_eth)
{
    struct top_level *tlp;

    ASSERT_NE(
        tlp = load_yaml_string(
            "enclaves:\n"
            "  - name: a\n"
            "resources:\n"
            "  - name: my_channel\n"
            "    type: pirate_channel\n"
            "    ids: [\"a/a_channel\"]\n"
            "    contents:\n"
            "      channel_type: ge_eth\n"
            "      host: 10.0.0.1\n"
            "      port: 9001\n"
            "      mtu: 1024\n"
        ), nullptr);

    ASSERT_EQ(tlp->tl_rscs_count, 1);
    EXPECT_STREQ(tlp->tl_rscs[0].r_name, "my_channel");
    EXPECT_STREQ(tlp->tl_rscs[0].r_type, "pirate_channel");
    ASSERT_EQ(tlp->tl_rscs[0].r_ids_count, 1);
    EXPECT_STREQ(tlp->tl_rscs[0].r_ids[0], "a/a_channel");
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_channel_type, GE_ETH);
    EXPECT_STREQ(tlp->tl_rscs[0].r_contents.cc_host, "10.0.0.1");
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_port, 9001);
    EXPECT_EQ(tlp->tl_rscs[0].r_contents.cc_mtu, 1024);
}

// FIXME: Add negative tests once sanity checking is in place.
