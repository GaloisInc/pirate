# Challenge Problem

This project contains an example application that has been implemented using
several different system architectures. The application accepts user
input from standard input, encrypts the input data, encrypts the output
from the previous step, and prints on standard output a base-64
representation of the doubly-encrypted data.

```
$ echo 'hello world' | ./challenge_baseline_encrypt
lr8cuejxb/Y9WA0m6ueaH1Fq1urnobW/8Xc4XJIJFy3YTrGLpUR4Y15eFg==
```

Assume that each encryption step uses a separate encryption algorithm
that is provided by a separate cryptographic library implementation. For
simplicity both encryption steps are performed with the same encryption
algorithm provided by the same encryption library. The two encryption steps
uses different secret keys.

The security architecture of this application consists of
two encryption enclave and one central enclave. Each encryption
enclave contains a secret key that should not be leaked to
the other enclaves.

## System architectures

### Monolithic application

This is the baseline implementation. The encryption enclaves are encapsulated
inside functions. The central enclave is a single-threaded, single-process
application that invokes the encryption functions. The monolithic
application is implemented by
[challenge_baseline_encrypt.c](/demos/challenge/challenge_baseline_encrypt.c).

### Producer-consumer communication

This is a partioned version of the baseline implementation where the
three enclaves are communicating using a producer-consumer model.
This is a one-to-one communication pattern. Each communication channel
has one reader and one writer. The producer-consumer application
is implemented by
[challenge_socket_main.c](/demos/challenge/challenge_socket_main.c),
[challenge_socket_encrypt1.c](/demos/challenge/challenge_socket_encrypt1.c),
and
[challenge_socket_encrypt2.c](/demos/challenge/challenge_socket_encrypt2.c).


### Publish-subscribe communication

This is a partioned version of the baseline implementation where the
three enclaves are communicating using a publish-subscribe model.
This is a many-to-many communication pattern. Each communication channel
has one or more publishers and one or more subscribers. The
implementation uses the [ActiveMQ](https://activemq.apache.org/) framework.
The publish-subscribe application is implemented by
[challenge_pubsub_main.cpp](/demos/challenge/challenge_pubsub_main.cpp),
[challenge_pubsub_encrypt1.cpp](/demos/challenge/challenge_pubsub_encrypt1.cpp),
and
[challenge_pubsub_encrypt2.cpp](/demos/challenge/challenge_pubsub_encrypt2.cpp).

### Multithreaded application

Each security enclave is running as a separate thread. The primary thread
and the security threads use synchronization primitives (semaphores)
to coordinate their operation. The multithreaded
application is implemented by
[challenge_multithreaded.c](/demos/challenge/challenge_multithreaded.c).

### Remote procedure call computation

This is a service-oriented architecture. The encryption domains
provide a remote produce call that can be invoked by the primary
domain. The implementation uses the [gRPC](https://grpc.io/) framework.
The remove procedure call application is implemented by
[challenge_rpc_main.cpp](/demos/challenge/challenge_rpc_main.cpp),
[challenge_rpc_encrypt1.cpp](/demos/challenge/challenge_rpc_encrypt1.cpp),
and
[challenge_rpc_encrypt2.cpp](/demos/challenge/challenge_rpc_encrypt2.cpp).

### Asynchronous computation

This is an event-driven architecture. The encryption domains
are invoked as two asynchronous tasks that are chained together.
The implementation uses the [asyncplusplus](https://github.com/Amanieu/asyncplusplus) library.
The asynchronous application is implemented by
[challenge_async.cpp](/demos/challenge/challenge_async.cpp).

### Monolithic application w/ spaghetti code

This is a degenerate version of the baseline implementation. The two
encryption enclaves have been combined in a single function call. Either
static analysis or manual refactoring will be necessary to partition the
security enclaves. The spaghetti application is implemented by
[challenge_spaghetti.c](/demos/challenge/challenge_spaghetti.c).
