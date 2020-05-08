#!/usr/bin/env python3

import argparse
import numpy as np
import matplotlib.pyplot as plt


if __name__ == "__main__":
    p = argparse.ArgumentParser(description='Run performance analysis on the captured data counters')
    p.add_argument('-n', '--intervals', help='Number of interval buckets', default=128, type=int)
    p.add_argument('-i', '--input', help='Input counts binary file', required=True)
    args = p.parse_args()

    counts = np.fromfile(args.input, dtype=np.uint8)
    if counts.size < args.intervals:
        args.intervals = counts.size

    print('Length               {}'.format(counts.size))
    print('Average reliability  {}'.format(np.mean(counts)))

    counts_interval_mean = np.asarray([np.mean(i) for i in np.split(counts, args.intervals)])

    print(counts_interval_mean)

    
    plt.plot(counts_interval_mean)
    plt.ylabel('Channel reliability')
    plt.title('Channel reliability')
    plt.show()