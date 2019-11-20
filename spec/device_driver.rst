Device driver
^^^^^^^^^^^^^

.. toctree::
   :maxdepth: 2
   :caption: Contents:

Overview
--------

Based on the kickoff discussion, we have put together a strawman proposal for
the GAPS channel behavior. The goal is to reach consensus on the prerequisites
for a six month demonstration.

Behavior
--------

- A character device driver.
- Each side of a channel will be used by one process or thread.
- Channels should behave as posix named pipes (fifos) with the following exceptions below. See `pipe <http://man7.org/linux/man-pages/man7/pipe.7.html>`_, `open <https://pubs.opengroup.org/onlinepubs/009695399/functions/open.html>`_, `read <https://pubs.opengroup.org/onlinepubs/009695399/functions/read.html>`_, `write <https://pubs.opengroup.org/onlinepubs/009695399/functions/write.html>`_, and `close <https://pubs.opengroup.org/onlinepubs/009695399/functions/close.html>`_. 
- The read-only side is opened in O_RDONLY mode.
- The write-only side is opened in O_WRONLY mode.
- O_RDWR mode is not permitted.
- O_NONBLOCK changes the behavior of read requests (see below).
- Other flags to open() are ignored.

The following differences from posix named pipe (fifo) semantics are requested. The differences are to ensure that API calls by the writer do not reveal any information about the reader.
 
- open() is a non-blocking operation. Opening for read-only succeeds even if no one has opened on the write side yet and opening for write-only succeeds even if no one has opened on the read side yet.
- write() is a non-blocking operation. On success, the number of bytes written is returned. On error, -1 is returned, and errno is set to indicate the cause of the error. write() never returns the errno values EAGAIN or EWOULDBLOCK. A successful write() may transfer fewer than count bytes. When the writer has not filled or exceeded the capacity of the channel then writes must be delivered reliably. When the writer has filled or exceeded the capacity of the channel then the channel enters a non-recoverable error state (see below). Writes are silently dropped in the error state. The return value of the write() must be indistinguishable from writes in the non-error state. Specifically, the device device continues to behave as if a successful write() may transfer fewer than count bytes.
- When a write() request attempts to fill the buffer or exceed the capacity of the buffer, the channel enters a nonrecoverable error state. All existing data in the channel is discarded. Subsequent read() requests return ENOBUFS. The current write() and subsequent write() requests fail silently. writes continue to behave as if they are successful.
- When the read end of a channel is closed, a write() does not cause a SIGPIPE to be generated for the calling process. Nor does the write() fail with the error EPIPE.
- When the write end of a channel is closed, semantics are unchanged from posix.

Configuration
-------------

- Out of band communication to configure channels
- Of lower priority, it would be beneficial if the character device could be configured for use in a trusted mode. In trusted mode the device driver behaves identically to a posix named pipe (fifo).
