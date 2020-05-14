#!/usr/bin/env python3

import argparse
import numpy as np
import matplotlib.pyplot as plt

if __name__ == "__main__":
    p = argparse.ArgumentParser(description='Run performance analysis on the captured data counters')
    p.add_argument('-n', '--intervals', help='Number of averaged intervals', type=int)
    p.add_argument('inputs', type=str, nargs='+', help='Input counts binary files')
    args = p.parse_args()

    cols = min(int(np.sqrt(len(args.inputs))), 4)
    rows = int(np.ceil(len(args.inputs) / cols))

    for i,f in enumerate(args.inputs):
        counts = np.fromfile(f, dtype=np.uint8)
        
        intervals = args.intervals
        if intervals is None:
            intervals = counts.size
        elif counts.size < intervals:
            intervals = counts.size

        counts_interval_mean = np.asarray([np.mean(i) for i in np.array_split(counts, intervals)])

        subplt = plt.subplot(rows, cols, i+1)
        subplt.plot(counts_interval_mean, linestyle='-', marker='.', color='k')
        subplt.set_xlim([0, intervals])
        subplt.set_ylim([0, 1.05])
        subplt.set_title('Len {}, Count {}, Delay {:,} ns'.format(*[int(i) for i in f.split('_')[1:4]]))
        subplt.set_ylabel('Average {}'.format(np.mean(counts)))
        subplt.grid(linestyle='--', linewidth='0.5', color='k')

    plt.show()
