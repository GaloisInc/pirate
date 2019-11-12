#ifndef __PIRATE_PRIMITIVES_H
#define __PIRATE_PRIMITIVES_H

#include <fcntl.h>
#include <sys/types.h>

#include "shmem.h"

#define PIRATE_FILENAME "/tmp/gaps.channel.%d"
#define PIRATE_SHM_NAME "/gaps.channel.%d"
#define PIRATE_LEN_NAME 64

#define PIRATE_NUM_CHANNELS 16

typedef enum {
  // The gaps channel is implemented using a FIFO special file
  // (a named pipe). The name of the pipe is formatted
  // with PIRATE_FILENAME where %d is the gaps descriptor.
  PIPE,
  // The gaps channel is implemented using a filepath.
  // This filepath points to a character device or a
  // named pipe. pirate_set_pathname(int, char *) must
  // be used to specify the pathname.
  DEVICE,
  // The gaps channel is implemented using shared memory.
  // This feature is disabled by default. It must be enabled
  // by setting PIRATE_SHMEM_FEATURE in CMakeLists.txt
  SHMEM,
  // The gaps channel is unavailable for operations.
  INVALID,
} channel_t;

typedef struct {
  int fd;                       // file descriptor
  channel_t channel;            // channel type
  char *pathname;               // optional device path
  shmem_buffer_t *shmem_buffer; // optional shared memory buffer
} pirate_channel_t;

// Opens the gaps channel specified by the gaps descriptor.
//
// Channels must be opened in order from smaller to largest
// gaps descriptor. pirate_open() will block until both the
// reader and the writer have opened the channel.

// The return value is the input gaps descriptor, or -1 if an
// error occurred (in which case, errno is set appropriately).
//
// The argument flags must be O_RDONLY or O_WRONLY.
int pirate_open(int gd, int flags);

// pirate_read() attempts to read up to count bytes from
// gaps descriptor gd into the buffer starting at buf.
//
// On success, the number of bytes read is returned.
// On error, -1 is returned, and errno is set appropriately.
ssize_t pirate_read(int gd, void *buf, size_t count);

// pirate_write() writes up to count bytes from the buffer
// starting at buf to the gaps descriptor gd.
//
// On success, the number of bytes written is returned
// (zero indicates nothing was written). On error,
// -1 is returned, and errno is set appropriately.
ssize_t pirate_write(int gd, const void *buf, size_t count);

// Closes the gaps channel specified by the gaps descriptor.
//
// pirate_close() returns zero on success.  On error,
// -1 is returned, and errno is set appropriately.
int pirate_close(int gd, int flags);

// Invoke fcntl() on the underlying file descriptor
int pirate_fcntl0(int gd, int flags, int cmd);
int pirate_fcntl1_int(int gd, int flags, int cmd, int arg);

// Sets the channel type for the read and write ends
// of the gaps descriptor. Must be configured before
// the channel is opened. Returns zero on success.
// On error -1 is returned, and erro is set appropriately.
int pirate_set_channel_type(int gd, channel_t channel_type);

// Gets the channel type for the read and write ends
// of the channel descriptor. Returns INVALID on error.
channel_t pirate_get_channel_type(int gd);

// Sets the pathname for the read and write ends
// of the gaps descriptor. Only valid if the channel
// type is DEVICE. Returns zero on success.
// On error -1 is returned, and errno is set appropriately.
// If pathname is NULL, then the memory allocated
// for the channel pathname is free'd.
int pirate_set_pathname(int gd, char *pathname);

// Gets the pathname for the read and write ends
// of the gaps descriptor. There must be PIRATE_LEN_NAME
// bytes allocated at pathname. Returns zero on success.
// On error -1 is returned, and errno is set appropriately.
int pirate_get_pathname(int gd, char *pathname);

#endif //__PIRATE_PRIMITIVES_H
