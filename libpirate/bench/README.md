# Benchmarks

This directory has throughput benchmarks and latency benchmarks
for the libpirate library.

The throughput benchmarks use one gaps channel to measure
the MB/sec transmitted across the channel. The latency benchmarks
use two gaps channels in opposite directions to measure
the roundtrip time (ns) it takes to send and receive a message.

Throughput and latency benchmarks require two additional gaps channels
used for synchronization. The synchronization
channels must be TCP sockets. The synchronization channels
are only used during benchmark initialization and termination.

## bench.py

`bench.py` is a wrapper script that can run the benchmarks
for several iterations across different message sizes.
If you are running benchmarks on a single machine then use
`bench.py -r both`. If you are running benchmarks across
two machines then one machine will use `bench.py -r reader`
and the other will use `bench.py -r writer`.

The following command-line options are available:

```
usage: bench.py [-h] [-t {thr,lat}] [-r {reader,writer,both}]
                [-n SCENARIO_NAME] -c1 TEST_CHANNEL_1 [-c2 TEST_CHANNEL_2]
                [-s1 SYNC_CHANNEL_1] [-s2 SYNC_CHANNEL_2] [-l MESSAGE_SIZES]
                [-i ITERATIONS] [-d PACKET_DELAY] [-w RECEIVE_TIMEOUT] [-v]

GAPS pirate benchmark suite

optional arguments:
  -h, --help            show this help message and exit
  -t {thr,lat}, --test_type {thr,lat}
                        Test suite type (required)
  -r {reader,writer,both}, --role {reader,writer,both}
                        Bench component
  -n SCENARIO_NAME, --scenario_name SCENARIO_NAME
                        Test scenario type
  -c1 TEST_CHANNEL_1, --test_channel_1 TEST_CHANNEL_1
                        Test channel 1 configuration (required)
  -c2 TEST_CHANNEL_2, --test_channel_2 TEST_CHANNEL_2
                        Test channel 2 configuration (required for lat)
  -s1 SYNC_CHANNEL_1, --sync_channel_1 SYNC_CHANNEL_1
                        Synchronization channel 1 specification
  -s2 SYNC_CHANNEL_2, --sync_channel_2 SYNC_CHANNEL_2
                        Synchronization channel 2 specification
  -l MESSAGE_SIZES, --message_sizes MESSAGE_SIZES
                        Test sizes <pow,start,stop|inc,start,stop,step>
  -i ITERATIONS, --iterations ITERATIONS
                        Number of iterations for each test size (default 64)
  -d PACKET_DELAY, --packet_delay PACKET_DELAY
                        Inter-packet delay in nanoseconds (default 0)
  -w RECEIVE_TIMEOUT, --receive_timeout RECEIVE_TIMEOUT
                        Receive timeout in seconds (default 2)
  -v, --validate        Validate received packets (default false)
```

## Throughput

`bench_thr_reader` and `bench_thr_writer` are the
executable programs. The reader opens the test channel
in O_RDONLY mode, sync channel 1 in O_RDONLY mode, and
sync channel 2 in O_WRONLY mode. The writer opens the
test channel is O_WRONLY mode, sync channel 1 in O_RDONLY
mode, and sync channel 2 in O_WRONLY mode.

The following command-line options are available:

```
  -c, --channel=CONFIG       Test channel configuration
  -d, --tx_delay=USEC        Inter-message delay
  -m, --message_len=BYTES    Transfer message size
  -n, --nbytes=BYTES         Number of bytes to receive
  -s, --sync1=CONFIG         Sync channel 1 configuration
  -S, --sync2=CONFIG         Sync channel 2 configuration
  -v, --validate             Validate received data
  -w, --rx_timeout=SEC       Message receive timeout
```

## Latency

`bench_lat1` and `bench_lat1` are the executable programs.
`bench_lat1` (considered to be the "writer" in bench.py)
opens test channel 1 and sync channel 1 in O_WRONLY mode,
and test channel 2 and sync channel 2 in O_RDONLY mode.
`bench_lat2` (considered to be the "reader") does the opposite.

The following command-line options are available:

```
  -c, --channel1=CONFIG      Test channel 1 configuration
  -C, --channel2=CONFIG      Test channel 2 configuration
  -d, --tx_delay=USEC        Inter-message delay
  -m, --message_len=BYTES    Transfer message size
  -n, --nbytes=BYTES         Number of bytes to receive
  -s, --sync1=CONFIG         Sync channel 1 configuration
  -S, --sync2=CONFIG         Sync channel 2 configuration
  -w, --rx_timeout=SEC       Message receive timeout
```
