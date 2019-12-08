#!/bin/sh
convert -density 150 -trim channel-throughput-table.pdf -quality 100 -flatten -sharpen 0x1.0 channel-throughput-table.png
convert -density 150 -trim channel-throughput-plot.pdf -quality 100 -flatten -sharpen 0x1.0 channel-throughput-plot.png
convert -density 150 -trim channel-latency-plot.pdf -quality 100 -flatten -sharpen 0x1.0 channel-latency-plot.png
convert -density 150 -trim channel-latency-table.pdf -quality 100 -flatten -sharpen 0x1.0 channel-latency-table.png
