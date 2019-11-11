Trusted timestamping
====================

Trusted timestamping is the process of tracking the time that data was
created or modified.  A trusted timestamp generally identifies the
data that timestamped (typically by a secure hash of the data), the
time that the data was timestamped, and a digital signature or other
evidence that the timestamp should be trusted.  This is typically a
digitial signature signed by a trusted third party, but could include
additional evidence such as information needed to locate the timestamp
in a blockchain ledger.  The additional information could be used to
provide alternate methods of establishing trust if the private key
associated with the digital signature is lost.

Minimal Timestamping Prototype
------------------------------

We minimal trusted timestamping architecture would consist of two
parties: an application with data to sign, and a signing service.  For
GAPS, we think an interesting scenario is one where the application
contains sensitive data that it needs signed, but at a minimum the
signing service should not be able to infer information about the data
to sign.  As many applications may involve signing data regularly,
this may include not leaking the timing of when data changes
necesitate a new signature, and whether two different signature
requests may reflect the same underlying data.

To account for these side channels, as a strawman, we propose a three
component architecture that inserts a proxy between the application
and the signing service.  The proxy is responsible for regularly
sending signature requests, and all communications between the
application and signing service go through the proxy.  At a minimum,
the proxy should be separated from the signing service using a GAPS
channel, but there are significant security benefits if the proxy and
application are also separated by a GAPS channel.

The architecture has the following components:

::

    |Application| <- Preferred GAPS channel -> |Proxy| <- Required GAPS channel -> |Signing Service|

The protocol would consists of a two-step process for obtaining
messages.  Internally, the application server will need to maintain a
map from message ids to message contents, and the proxy will need to
maintain a queue of requests to send.  The exact protocol will need to
be completely worked out, but we describe a simplified abstract
version of the protocol below that doesn't account for all potential
resource exhaustion and contention conditions.  The protocol also
assumes that we are able to keep the application and timestamp server
clocks sufficiently in agreement for checking the correctness of
timestamps times.

Step 1. Queuing a signing request.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

1. The application generates a sequence of bytes ``D`` containing the data
   to sign.
2. The application generates a request id ``N`` that will be used to
   disambiguate responses, compute ``H_D = hash(D)``, associates N to the
   request ``H_D`` in a outstanding request map, and issues sends a request
   pair ``(N, H_D)`` to the proxy.
3. The proxy receives the pair ``(N, H_D)``, and appends it to the message
   queue.

Step 2. Signing a queued request.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

1. Once per time interval I, the proxy pops the next signature request
   from ``(N, H_D)`` from its internal queue, and performs the following steps:

   - Generates a random number R with a sufficient entropy
   - Computes a new hash ``H_R = hash(R ++ H_D)``
   - Sends a signature request for ``H_R`` to the signing service.

   If the queue is empty, then
   the proxy will generates a random hash ``H_D`` and follow the same interaction
   with the signing service, but discard the result.
   Care should be taken so that the time to dequeue a message
   and generate a random message are the same so that the signing
   service cannot infer the length of the queue.

#. The signing service receives the request to sign ``H_R``, generates
   a signature and serializes the certificate ``C_R`` containing the time
   of the timestamp, a signature, and other data.

#. The proxy receives the certificate ``C_R`` from the timestamping
   service.  If the queue was empty in step 1, then this is dropped.
   Otherwise, the proxy forwards the tuple ``(N, R, C_R)`` to the
   application.

#. The application receives the tuple ``(N, R, C_R)`` from the proxy,
   finds the hash ``H_D`` associated with request ``N``, and checks that the
   signature on ``C_R`` is indeed a timestamp on ``hash(R ++ H_D)``, and
   that the time is correct with respect to the application server's
   time.

The security claim is that with the above protocol, we can build a trusted
timestamping service that does not inadvertently leak sensitive
information including timing of signatures and data distribution to
the timestamping service.  Furthermore, if the application and proxy
can be isolated via GAPS channels, then a compromised application will
not be able to collude with a compromised timestamping service to leak
sensitive information.  The proxy must be compromised as well.  As the
proxy maintains minimal runtime state, and is designed solely for
this, we think that it will be easier to audit than the complete
application server.

Risks and Mitigations
---------------------

We include a preliminary risk and mitigations that we have identified
so far in our design.  We will expand this list as needed.


Risk: Signature request queue on proxy becomes full.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Mitigation. If the application needs more than one signature between
signature rounds for a sustained period of time, then the queue of
signatures will grow arbitrarily.  If this is an issue, we can merge
multiple application requests into a single timestamp request using
a Merkle tree, and provide more complex signatures back to the
application server.

Risk: Signing service is compromised.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If the secret key used by the signing service is compromised, then the
signatures are all invalidated.  As a mitigation, the signing service
may chose to publish a ledger of signature requests, perhaps in the
form of a blockchain, and disseminate this widely.  The certificate
will then need to include sufficient information to locate the
signature in the blockchain.  Potentially, one could use techniques
based on Merkle trees to combine multiple hashes into a single one.
If secret key is compromised at some point, and one can establish the
earliest time this could be, then the ledger lookup can be used to
validate the signatures.
