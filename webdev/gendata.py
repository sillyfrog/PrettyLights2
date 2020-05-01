#!/usr/bin/env python3
"""
Generates some sample frame/pixel data for testing
"""

import os

BASE_DIR = "./../data/"

FRAMES_PER_FILE = 25
BYTES_PER_COLOR = 3
FRAMES = 1500
PATTERNS = 15


def main():
    f = 0
    while f < FRAMES:
        d = int(f / FRAMES_PER_FILE / 10)
        os.makedirs(BASE_DIR + "rgb%02d" % (d), exist_ok=True)
        fh = open(BASE_DIR + "rgb%02d/frames%04d.dat" % (d, f / FRAMES_PER_FILE), "wb")
        for i in range(FRAMES_PER_FILE):
            for p in range(PATTERNS):
                data = bytearray([p]) * BYTES_PER_COLOR
                if p == 0:
                    data = bytearray([255, 0, 255])
                elif p == 1:
                    data = bytearray([0, 255, 0])
                fh.write(data)
            print(fh, f, i, p)
            f += 1
        fh.close()


if __name__ == "__main__":
    main()
