#!/usr/bin/env python3

import re

import subprocess
from subprocess import PIPE

import sys

MESSAGE_SIZES = []

for i in range(0, 21):
    MESSAGE_SIZES.append(2 ** i)

if len(sys.argv) < 3:
    print("Usage: " + sys.argv[0] + " { lat | thr | thr_vector }" + " scenario-name [device1] [device2]")
    exit(1)

def main():
    suite = sys.argv[1]
    scenario = sys.argv[2]

    if not ((suite == "lat") or (suite != "thr") or (suite != "thr_vector")):
        print("Usage: " + sys.argv[0] + " [ lat | thr | thr_vector ]")
        exit(1)

    if suite == "thr":
        filename = "primitives_bench_thr"
        message_count = 1_000_000
        pattern = re.compile(r'average throughput: ([0-9.]+) MB/s')
    elif suite == "thr_vector":
        filename = "primitives_bench_thr_vector"
        message_count = 1_000_000
        pattern = re.compile(r'average throughput: ([0-9.]+) MB/s')
    else:
        filename = "primitives_bench_lat"
        message_count = 1_000_000
        pattern = re.compile(r'average latency: (\d+) ns')

    for message_size in MESSAGE_SIZES:
        for _ in range(64):
            print(scenario, message_size, end=" ")
            sys.stdout.flush()
            args = ["./" + filename, str(message_size), str(message_count)]
            if len(sys.argv) > 3:
                args.append(sys.argv[3])
            if len(sys.argv) > 4:
                args.append(sys.argv[4])
            completed = subprocess.run(args, stdout=PIPE)
            results = pattern.search(completed.stdout.decode('utf-8'))
            print(results.group(1))

if __name__ == "__main__":
    main()
