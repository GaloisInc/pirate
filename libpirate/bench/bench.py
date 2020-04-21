#!/usr/bin/env python3

import re

import subprocess
from subprocess import STDOUT, PIPE

import sys

MESSAGE_SIZES = []

for i in range(0, 21):
    MESSAGE_SIZES.append(2 ** i)

def usage():
    print("Usage: " + sys.argv[0] + " thr " + " scenario-name [test channel] [sync channel]")
    print("Usage: " + sys.argv[0] + " lat " + " scenario-name [test channel 1] [test channel 2] [sync channel]")
    exit(1)

if len(sys.argv) < 5:
    usage()

def main():
    suite = sys.argv[1]
    scenario = sys.argv[2]

    if suite != "thr" and suite != "lat":
        usage()

    if suite == "thr" and len(sys.argv) != 5:
        usage()

    if suite == "lat" and len(sys.argv) != 6:
        usage()

    if suite == "thr":
        filename1 = "bench_thr_writer"
        filename2 = "bench_thr_reader"
        pattern = re.compile(r'average throughput: ([0-9.]+) MB/s')
        iters = 64
    else:
        filename1 = "bench_lat1"
        filename2 = "bench_lat2"
        pattern = re.compile(r'average latency: (\d+) ns')
        iters = 32

    for message_size in MESSAGE_SIZES:
        if message_size < 32:
            nbytes = 1_000_000
        elif message_size < 1024:
            nbytes = 10_000_000
        elif message_size < 131072:
            nbytes = 100_000_000
        else:
            nbytes = 1_000_000_000
        args1 = ["./" + filename1] + sys.argv[3:] + [str(message_size), str(nbytes)]
        args2 = ["./" + filename2] + sys.argv[3:] + [str(message_size), str(nbytes)]
        for _ in range(iters):
            print(scenario, message_size, end=" ")
            sys.stdout.flush()
            p1 = subprocess.Popen(args1)
            p2 = subprocess.Popen(args2, stdout=PIPE)
            p1.wait()
            out, _ = p2.communicate()
            results = pattern.search(out.decode('utf-8'))
            print(results.group(1))

if __name__ == "__main__":
    main()
