#!/usr/bin/env python3

import re
import sys
import subprocess
import argparse

def parse_sizes(arg):
    args = arg.split(',')
    params = [int(i.strip()) for i in args[1:]]
    if args[0] == 'pow':
        return [2 ** i for i in range(*params)]
    elif args[0] == 'inc':
        return range(*params)
    elif args[0] == 'list':
        return params
    print("Invalid length specifications '{}'".format(arg), file=sys.stderr)
    sys,exit(1)

def extract(out, pattern):
    if pattern is None:
        return None
    s = pattern.search(out.decode('utf-8'))
    if s is None:
        return None
    return s.group(1)

def main():
    p = argparse.ArgumentParser(description="GAPS pirate benchmark suite")
    p.add_argument("-t", "--test_type", help="Test suite type", choices=["thr", "lat"], default="thr")
    p.add_argument("-r", "--role", help="Bench component", choices=["reader", "writer", "both"], default="both")
    p.add_argument("-n", "--scenario_name", help="Test scenario type", default="unnamed-scenario")
    p.add_argument("-c1", "--test_channel_1", help="Test channel 1 configuration", required=True)
    p.add_argument("-c2", "--test_channel_2", help="Test channel 2 configuration")
    p.add_argument("-s1", "--sync_channel_1", help="Synchronization channel 1 specification", default="tcp_socket,127.0.0.1,10000,0.0.0.0,0")
    p.add_argument("-s2", "--sync_channel_2", help="Synchronization channel 2 specification", default="tcp_socket,127.0.0.1,10001,0.0.0.0,0")
    p.add_argument("-l", "--message_sizes", help="Test sizes <pow,start,stop|inc,start,stop,step>", type=parse_sizes)
    p.add_argument("-i", "--iterations", help="Number of iterations for each test size", type=int)
    p.add_argument("-d", "--packet_delay", help="Inter-packet delay in nanoseconds", type=float, default=0)
    p.add_argument("-w", "--receive_timeout", help="Receive timeout in seconds", type=int, default=2)
    p.add_argument("-v", "--validate", help="Validate received packets", action="store_true")
    args = p.parse_args()

    if args.message_sizes is None:
        args.message_sizes = [2 ** i for i in range(21)]

    if args.test_type == 'lat' and args.test_channel_2 is None:
        print("Must provide channel 2 configuration for 'lat' test type", file=sys.stderr)
        sys,exit(1)

    if args.test_type == "thr":
        writer_app = "bench_thr_writer"
        reader_app = "bench_thr_reader"

        channel_args = ["-c",  args.test_channel_1, "-s", args.sync_channel_1, "-S", args.sync_channel_2]
        pattern1 = re.compile(r'average throughput: ([0-9.]+) MB/s')
        pattern2 = re.compile(r'drop rate: ([0-9.]+) %')
        if args.iterations is None:
            args.iterations = 64
    else:
        writer_app = "bench_lat1"
        reader_app = "bench_lat2"
        channel_args = ["-c",  args.test_channel_1, "-C",  args.test_channel_2, "-s", args.sync_channel_1, "-S", args.sync_channel_2]
        pattern1 = re.compile(r'average latency: (\d+) ns')
        pattern2 = None
        if args.iterations is None:
            args.iterations = 32

    for message_size in args.message_sizes:
        if message_size < 32:
            nbytes = 1_000_000
        elif message_size < 1024:
            nbytes = 10_000_000
        elif message_size < 131072:
            nbytes = 100_000_000
        else:
            nbytes = 1_000_000_000

        test_args = channel_args + ["-m", str(message_size), "-n", str(nbytes), "-d", str(args.packet_delay), "-w", str(args.receive_timeout)]

        if args.validate:
            test_args.append("-v")

        for _ in range(args.iterations):
            if args.role == "both" or args.role == "writer":
                writer_proc = subprocess.Popen(["./" + writer_app] + test_args)

            if args.role == "both" or args.role == "reader":
                reader_proc = subprocess.Popen(["./" + reader_app] + test_args, stdout=subprocess.PIPE)

            if args.role == "both" or args.role == "writer":
                writer_proc.wait()

            if args.role == "both" or args.role == "reader":
                out, _ = reader_proc.communicate()
                results1 = extract(out, pattern1)
                results2 = extract(out, pattern2)
                if results1 is None:
                    results1 = "error"
                if results2 is None:
                    results2 = ""
                print(args.scenario_name, message_size, results1, results2, flush=True)

if __name__ == "__main__":
    main()
