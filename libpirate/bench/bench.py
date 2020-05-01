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

def main():
    p = argparse.ArgumentParser(description="GAPS pirate benchmark suite")
    p.add_argument("-t", "--test_type", help="Test suite type", choices=["thr", "lat"], default="thr")
    p.add_argument("-r", "--role", help="Bench component", choices=["reader", "writer", "both"], default="both")
    p.add_argument("-n", "--scenario_name", help="Test scenario type", default="Default bench")
    p.add_argument("-c1", "--test_channel_1", help="Test channel 1 configuration", required=True)
    p.add_argument("-c2", "--test_channel_2", help="Test channel 2 configuration")
    p.add_argument("-s", "--sync_channel", help="Synchronization channel specification", default="tcp_socket,127.0.0.1,10000")
    p.add_argument("-l", "--message_sizes", help="Test sizes <pow,start,stop|inc,start,stop,step>", type=parse_sizes)
    p.add_argument("-i", "--iterations", help="Number of iterations for each test size", type=int)
    args = p.parse_args()

    if args.message_sizes is None:
        args.message_sizes = [2 ** i for i in range(21)]

    if args.test_type == 'lat' and args.test_channel_2 is None:
        print("Must provide channel 2 configuration for 'lat' test type", file=sys.stderr)
        sys,exit(1)

    if args.test_type == "thr":
        writer_app = "bench_thr_writer"
        reader_app = "bench_thr_reader"
        channel_args = [args.test_channel_1, args.sync_channel]
        pattern = re.compile(r'average throughput: ([0-9.]+) MB/s')
        if args.iterations is None:
            args.iterations = 64
    else:
        writer_app = "bench_lat1"
        reader_app = "bench_lat2"
        channel_args = [args.test_channel_1, args.test_channel_2, args.sync_channel]
        pattern = re.compile(r'average latency: (\d+) ns')
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

        test_args = channel_args + [str(message_size), str(nbytes)]

        for _ in range(args.iterations):
            if args.role == "both" or args.role == "writer":
                writer_proc = subprocess.Popen(["./" + writer_app] + test_args)

            if args.role == "both" or args.role == "reader":
                reader_proc = subprocess.Popen(["./" + reader_app] + test_args, stdout=subprocess.PIPE)

            if args.role == "both" or args.role == "writer":
                writer_proc.wait()

            if args.role == "both" or args.role == "reader":
                out, _ = reader_proc.communicate()
                results = pattern.search(out.decode('utf-8'))
                print(args.scenario_name, message_size, results.group(1), flush=True)

if __name__ == "__main__":
    main()
