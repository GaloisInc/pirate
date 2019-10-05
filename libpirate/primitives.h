#ifndef __PIRATE_PRIMITIVES_H
#define __PIRATE_PRIMITIVES_H

#include <fcntl.h>
#include <sys/types.h>

#define PIRATE_FILENAME     "/tmp/gaps.channel.%d"
#define PIRATE_LEN_NAME     64

#define PIRATE_NUM_CHANNELS  16

// Opens the gaps channel specified by the gaps descriptor.
//
// The return value is the input gaps descriptor, or -1 if an
// error occurred (in which case, errno is set appropriately).
//
// The argument flags must be O_RDONLY or O_WRONLY.
//
// The gaps chnnel is implemented using a FIFO special file
// (a named pipe). The name of the pipe is formatted
// with PIRATE_FILENAME where %d is the gaps descriptor.
//
// If flags is O_WRONLY then pirate_open() will create
// the named pipe or return an error if the file already
// exists. If flags is O_RDONLY then pirate_open() will
// block until the named pipe file exists.
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
//
// If the gaps descriptor was opened in read mode (O_RDONLY)
// then pirate_close() will delete the named pipe. If the gaps
// descriptor was opened in read mode and write mode
// then pirate_close() will delete the named pipe on the
// second invocation.
int pirate_close(int gd);

#endif //__PIRATE_PRIMITIVES_H
