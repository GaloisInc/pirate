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
- Channels should behave as posix named pipes (fifos).
- The read-only side is opened in O_RDONLY mode.
- The write-only side is opened in O_WRONLY mode.
- O_RDWR is not permitted. Other flags to open() are ignored.
- Opening a channel for reading will block until the other side for writing is opened.
- Opening a channel for writing will block until the other side for reading is opened. See `open <https://pubs.opengroup.org/onlinepubs/009695399/functions/open.html>`_ for behavior (under “When opening a FIFO with O_RDONLY or O_WRONLY set…”).
- write() will error with EPIPE on an attempt is made to write to a channel that is not open for reading by any process, or that only has one end open. A SIGPIPE signal shall also be sent to the thread. See `write <https://pubs.opengroup.org/onlinepubs/009695399/functions/write.html>`_
- If no process has the channel open for writing, read() shall return 0 to indicate end-of-file. See `read <https://pubs.opengroup.org/onlinepubs/009695399/functions/read.html>`_
- When all file descriptors associated with a channel are closed, any data remaining in the channel shall be discarded. See `close <https://pubs.opengroup.org/onlinepubs/009695399/functions/close.html>`_

Configuration
-------------

- Out of band communication to configure channels
