#!/usr/bin/env python3

from bcc import BPF
import time

device = "eno1"
b = BPF(src_file="forward.c")
fn = b.load_func("mpegts_filter", BPF.XDP)
b.attach_xdp(device, fn, 0)

try:
  b.trace_print()
except KeyboardInterrupt:
  pass

b.remove_xdp(device, 0)

