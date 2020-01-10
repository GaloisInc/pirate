#ifndef _GAPS_PACKET_H_
#define _GAPS_PACKET_H_

#include <stdint.h>
#include <sys/types.h>


/*
 * Read a packet from a GAPS channel stream
 * 
 * GAPS channel must be opened prior to making this call
 * 
 *  gd      - GAPS channel descriptor
 *  buf     - read buffer
 *  buf_len - read buffer length
 * 
 * Return:
 *  size of the received packet on success
 *  -1 on failure, the GAPS channel must be closed
 */
ssize_t gaps_packet_read(int gd, void *buf, uint32_t buf_len);


/*
 * Write a packet to the GAPS channel stream
 * 
 * GAPS channel must be opened prior to making this call
 * 
 *  gd  - GAPS channel descriptor
 *  buf - write buffer
 *  len - packet length
 * 
 *  Return:
 *    0 on success
 *   -1 on failure, the GAPS channel must be closed
 * 
 */
int gaps_packet_write(int gd, void *buf, size_t len);


#endif /* _GAPS_PACKET_H */
