PIRATE Enclave Tutorial
=======================

This tutorial demonstrates a simple use of the Pirate LLVM tools to generate a two-enclave application. We'll develop a sentence censoring application where the complete list of sensitive words is held by a separate host enclave. The UI enclave will only be able to ask the host enclave to censor a sentence but it will not be able to enumerate the list directly.

The source code is available in whole form at https://github.com/GaloisInc/pirate/blob/master/demos/word_filter/filter.c

Imports and helper prototypes
-----------------------------

.. code-block:: c

    #include "libpirate.h"

    #include <fcntl.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>

    #define ARRAY_LEN(arr) (sizeof arr / sizeof *arr)

    static void expand_buffer(size_t n, char ** msg_ptr, size_t * n_ptr);

Enclave and Capability Declarations
-----------------------------------

A Pirate source file can declare both enclave and capabilities used in a program. It can also declare which capabilities each enclave can support.

Each enclave will eventually be realized as a separate executable. Each enclave can have its own main function.

We can mark aspects of a source file with capability restrictions. These restrictions will ensure the marked elements will only be available on enclaves declared to have that capability.

In this tutorial we will associate a capability with a sensitive list of words. This capability will only be available to the word list host enclave. The UI will not have this capability, which will guarantee that the word list will not be accessible in the UI executable.

.. code-block:: c

    #pragma pirate enclave declare(filter_ui)
    #pragma pirate enclave declare(filter_host)
    #pragma pirate capability declare(sensitive_words)
    #pragma pirate enclave capability(filter_host, sensitive_words)

Sensitive Words
---------------

Next we'll define the list of sensitive words and annotate the list to only be available on enclaves with the :code:`sensitive_words` capability.

This annotation :code:`pirate_capability` can be assigned to various language structures including global variables as seen here.

Because :code:`censor` refers to :code:`word_list` it will also inherit that restriction.

.. code-block:: c

    static const char *word_list[]
    __attribute__((pirate_capability("sensitive_words")))
    = {
        "agile", "disruptive", "ecosystem", "incentivize",
        "low-hanging fruit", "negative growth", "paradigm shift",
        "rightsizing", "synergies",
    };

    static void censor(char *msg)
    {
        for (size_t i = 0; i < ARRAY_LEN(word_list); i++) {
            char const* word = word_list[i];
            char *found;
            while ((found = strstr(msg, word))) {
            memset(found, '*', strlen(word));
            }
        }
    }

Communication with libpirate channels
-------------------------------------

We'll use libpirates channels to communicate between these two enclaves. This library provides an API that is quite comparable to the standard POSIX file I/O API.

libpirate can operate in both stream and datagram modes. For this demonstration we're using streams.

.. code-block:: c

    static void write_all(int c, char const* buf, size_t count)
    {
    size_t sofar = 0;
    while (sofar < count) {
        ssize_t result = pirate_write(c, buf + sofar, count - sofar);
        if (result < 0) {
            perror("pirate_write");
            exit(EXIT_FAILURE);
        }
        sofar += result;
    }
    }

    static void read_all(int c, char * buf, size_t count)
    {
        size_t sofar = 0;
        while (sofar < count) {
            ssize_t result = pirate_read(c, buf + sofar, count - sofar);
            if (result < 0) {
                perror("pirate_read");
                exit(EXIT_FAILURE);
            }
            sofar += result;
        }
    }

Message Framing
---------------

For this simple demonstration we'll use a trivial framing protocol where the size of a message is sent first as a fixed-length integer and then the variable length message will follow.

.. code-block:: c

    static void transmit(int c, char const* msg, size_t n)
    {
        write_all(c, (char const*)&n, sizeof n);
        write_all(c, msg, n);
    }

    static void receive(int c, char **msg_ptr, size_t *n_ptr)
    {
        size_t n;
        read_all(c, (char *)&n, sizeof n);
        expand_buffer(n, msg_ptr, n_ptr);
        read_all(c, *msg_ptr, n);
    }

Enclave Entry-points
--------------------

Each enclave will need a main function defined. These functions are designated using the :code:`pirate_enclave_main` attribute.

Both enclaves start their communication channels using libpirate's :code:`pirate_open_parse`. This variation of opening a channel takes a connection string to pick the channel type and parameters.

.. code-block:: c

    int ui(void)
    __attribute__((pirate_enclave_main("filter_ui")))
    {
        puts("Connecting");

        int writechan = pirate_open_parse("pipe,filter_ui_to_host", O_WRONLY);
        if (-1 == writechan) { perror("pirate_open_parse"); exit(EXIT_FAILURE); }

        int readchan = pirate_open_parse("pipe,filter_host_to_ui", O_RDONLY);
        if (-1 == readchan) { perror("pirate_open_parse"); exit(EXIT_FAILURE); }

        puts("Connected");

        char *line = NULL;
        size_t line_sz = 0;

        for(;;) {
            printf("Input> ");
            fflush(stdout);

            if (-1 == getline(&line, &line_sz, stdin)) {
            puts("\n");
            break;
            }

            transmit(writechan, line, line_sz);
            receive(readchan, &line, &line_sz);

            printf("Response> %s", line);
        }

        return 0;
    }

    int host(void)
    __attribute__((pirate_enclave_main("filter_host")))
    {
        puts("Connecting");

        int readchan = pirate_open_parse("pipe,filter_ui_to_host", O_RDONLY);
        if (-1 == readchan) { perror("pirate_open_parse"); exit(EXIT_FAILURE); }

        int writechan = pirate_open_parse("pipe,filter_host_to_ui", O_WRONLY);
        if (-1 == writechan) { perror("pirate_open_parse"); exit(EXIT_FAILURE); }

        puts("Connected");

        char *line = NULL;
        size_t line_sz = 0;
        for (;;) {
            receive(readchan, &line, &line_sz);
            printf("Got> %s", line);
            censor(line);
            transmit(writechan, line, line_sz);
            printf("Sent> %s", line);
        }

        return 0;
    }

Helper function implementations
-------------------------------

.. code-block:: c

    static void expand_buffer(size_t n, char ** msg_ptr, size_t * n_ptr)
    {
        if (n > *n_ptr) {
            char *msg = realloc(*msg_ptr, n);
            if (NULL == msg) { exit(EXIT_FAILURE); }
            *msg_ptr = msg;
            *n_ptr = n;
        }
    }
