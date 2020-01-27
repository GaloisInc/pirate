#!/usr/bin/env python3

import pathlib
import subprocess
import sys
import time

import requests

def main():
    high = subprocess.Popen(
        ["valgrind", "--leak-check=full", "--error-exitcode=1", "./high", "26081"])
    low = subprocess.Popen(
        ["valgrind", "--leak-check=full", "--error-exitcode=1", "./low", "26080"])
    expected_high = pathlib.Path("index.html").read_text()
    expected_low = pathlib.Path("index.filtered.html").read_text()
    time.sleep(1)
    req_high = requests.get('http://localhost:26081')
    req_low = requests.get('http://localhost:26080')
    observed_high = req_high.text
    observed_low = req_low.text
    if expected_high != observed_high:
        print("did not receive expected response from http://localhost:26081")
        sys.exit(2)
    if expected_low != observed_low:
        print("did not receive expected response from http://localhost:26080")
        sys.exit(2)
    low.terminate()
    high.terminate()
    low.communicate()
    high.communicate()
    if low.returncode != 0:
        sys.exit(low.returncode)
    if high.returncode != 0:
        sys.exit(high.returncode)


if __name__ == "__main__":
    main()
