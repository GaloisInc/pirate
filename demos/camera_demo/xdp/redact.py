#!/usr/bin/env python3

# This work was authored by Two Six Labs, LLC and is sponsored by a subcontract
# agreement with Galois, Inc.  This material is based upon work supported by
# the Defense Advanced Research Projects Agency (DARPA) under Contract No.
# HR0011-19-C-0103.
#
# The Government has unlimited rights to use, modify, reproduce, release,
# perform, display, or disclose computer software or computer software
# documentation marked with this legend. Any reproduction of technical data,
# computer software, or portions thereof marked with this legend must also
# reproduce this marking.
#
# Copyright 2021 Two Six Labs, LLC.  All rights reserved.

from bcc import BPF
import time

device = "lo"
b = BPF(src_file="redact.c")
fn = b.load_func("mpegts_filter", BPF.XDP)
b.attach_xdp(device, fn, 0)

try:
  b.trace_print()
except KeyboardInterrupt:
  pass

b.remove_xdp(device, 0)

