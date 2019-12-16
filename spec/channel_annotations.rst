Note.  Enclave channels are not yet implemented, and this
documentation is just here for placeholder purposes until we figure
out the right model.

Enclave Communication
---------------------

Enclaves can communicate between named channels that are created at
enclave startup.  Each channel allows communication between a pair of
channels.  Channels are declared via global variables, and the PIRATE
toolchain will be responsible for ensuring that channels are correctly
initialized, and only accessible to the correct enclaves.  By declaring
channels in this way, the PIRATE development toolchain is aware of
the enclave communication architecture, and is able to map it to
different architectures.

.. code-block:: c

  const enclave_send_channel_t sender
  __attribute__((enclave_send_channel(<channel>, <enclave>)));

This declares that **sender** is a send channel with the name **channel**
that is visible in the enclave **enclave**.

.. code-block:: c

  const enclave_receive_channel_t receiver
  __attribute__((enclave_receive_channel(<channel>, <enclave>)));

This declares that **receiver** is a send channel with the name **channel**
that is visible in the enclave **enclave**.

TODO: Describe channel API.
