#!/usr/bin/env python3
import argparse
import random
from PIL import Image
from PIL import ImageColor
import common


args = None


def parseargs():
    global args
    parser = argparse.ArgumentParser(
        description="Flicker between 2 colours, for random durations."
    )
    parser.add_argument("frames", type=int, help="the total number of frames")
    parser.add_argument("filename", help="the PNG filename to output to")
    parser.add_argument(
        "--onmax", type=int, default=200, help="The maximum frame count to be on"
    )
    parser.add_argument(
        "--onmin", type=int, default=0, help="The minimum frame count to be on"
    )
    parser.add_argument(
        "--offmax", type=int, default=200, help="The maximum frame count to be off"
    )
    parser.add_argument(
        "--offmin", type=int, default=0, help="The minimum frame count to be off"
    )
    parser.add_argument(
        "--off", type=ImageColor.getrgb, default="#000000", help="The off colour"
    )
    parser.add_argument(
        "--on", type=ImageColor.getrgb, default="#ffffff", help="The on colour"
    )
    parser.add_argument(
        "--pattern",
        type=int,
        default=-1,
        help="If set, will update an existing image, at the "
        "corresponding row (pattern), otherwise a new image is created.",
    )
    args = parser.parse_args()


def main():
    parseargs()
    img = Image.new("RGB", (args.frames, 1))
    pxls = img.load()

    i = 0
    onoff = True
    while i < args.frames:
        if onoff:
            fmin = args.onmin
            fmax = args.onmax
            color = args.on
        else:
            fmin = args.offmin
            fmax = args.offmax
            color = args.off
        count = random.randint(fmin, fmax)
        if i + count > args.frames:
            count = args.frames - i
        for c in range(count):
            pxls[i, 0] = color
            i += 1
        onoff = not onoff

    common.savefile(img, args.filename, args.pattern)


if __name__ == "__main__":
    main()
