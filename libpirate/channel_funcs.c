

#include "device.h"
#include "pipe.h"
#include "unix_socket.h"
#include "tcp_socket.h"
#include "udp_socket.h"
#include "shmem_interface.h"
#include "udp_shmem_interface.h"
#include "uio.h"
#include "serial.h"
#include "mercury.h"
#include "ge_eth.h"

static int initialized;
static pirate_channel_funcs_t gaps_channel_funcs_array[PIRATE_CHANNEL_TYPE_COUNT];

void gaps_channel_funcs_init() {
    pirate_device_init(&gaps_channel_funcs_array[DEVICE]);
    pirate_pipe_init(&gaps_channel_funcs_array[PIPE]);
    pirate_unix_socket_init(&gaps_channel_funcs_array[UNIX_SOCKET]);
    pirate_tcp_socket_init(&gaps_channel_funcs_array[TCP_SOCKET]);
    pirate_udp_socket_init(&gaps_channel_funcs_array[UDP_SOCKET]);
    pirate_shmem_init(&gaps_channel_funcs_array[SHMEM]);
    pirate_udp_shmem_init(&gaps_channel_funcs_array[UDP_SHMEM]);
    pirate_uio_init(&gaps_channel_funcs_array[UIO_DEVICE]);
    pirate_serial_init(&gaps_channel_funcs_array[SERIAL]);
    pirate_mercury_init(&gaps_channel_funcs_array[MERCURY]);
    pirate_ge_eth_init(&gaps_channel_funcs_array[GE_ETH]);
}

pirate_channel_funcs_t* gaps_channel_funcs() {
    if (!initialized) {
        gaps_channel_funcs_init();
        initialized = 1;
    }
    return (pirate_channel_funcs_t*) &gaps_channel_funcs_array;
}
