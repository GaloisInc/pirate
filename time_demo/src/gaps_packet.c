#include "primitives.h"
#include "gaps_packet.h"


static int gaps_read_len(int gd, void *buf, size_t len) {
    do {
        ssize_t rd_len = pirate_read(gd, buf, len);
        if (rd_len == -1) {
            return -1;
        }

        buf = (uint8_t *)buf + rd_len;
        len -= rd_len;
    } while (len > 0);

    return 0;
}


static int gaps_write_len(int gd, void *buf, ssize_t len) {
    do {
        ssize_t wr_len = pirate_write(gd, buf, len);
        if (wr_len == -1) {
            return -1;
        }

        buf = (uint8_t *)buf + wr_len;
        len -= wr_len;
    } while (len > 0);

    return 0;
}


ssize_t gaps_packet_read(int gd, void *buf, uint32_t buf_len) {
    /* Read the length */
    uint32_t len = 0;
    if ((gaps_read_len(gd, &len, sizeof(len)) == -1) || (len > buf_len)) {
        return -1;
    }

    /* Read the packet */
    if (gaps_read_len(gd, buf, len) == -1) {
        return -1;
    }

    return len;
}


int gaps_packet_write(int gd, void *buf, size_t len) {
    /* Write the length */
    uint32_t pkt_len = len;
    if (gaps_write_len(gd, &pkt_len, sizeof(pkt_len)) == -1) {
        return -1;
    }

    /* Write the packet */
    return gaps_write_len(gd, buf, len); 
}
