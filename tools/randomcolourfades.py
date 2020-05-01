#!/usr/bin/env python3
import argparse
import random
from PIL import Image
import common


def parseargs():
    global args
    parser = argparse.ArgumentParser(
        description="Generate random fades between random colours and specified "
        "min/max intervals."
    )
    parser.add_argument("frames", type=int, help="the total number of frames")
    parser.add_argument("filename", help="the PNG filename to output to")
    parser.add_argument(
        "--max",
        type=int,
        default=200,
        help="The maximum frame count between colour changes",
    )
    parser.add_argument(
        "--min",
        type=int,
        default=5,
        help="The minimum frame count between colour changes",
    )
    parser.add_argument(
        "--bw",
        action="store_const",
        const=True,
        help="The output greyscale images, not full color",
    )
    parser.add_argument(
        "--flash",
        action="store_const",
        const=True,
        help="Flash instantly to each color, don't fade",
    )
    parser.add_argument(
        "--maxbrightness",
        type=int,
        default=100,
        help="The maximum colour brightness used, as a percentage",
    )
    args = parser.parse_args()


def getcolor():
    if args.bw:
        c = random.randint(0, 255)
        col = [c, c, c]
    else:
        col = [random.randint(0, 255), random.randint(0, 255), random.randint(0, 255)]
    brightpcg = args.maxbrightness / 100
    for i in range(3):
        col[i] = int(col[i] * brightpcg)
    if common.brightness(col) > (brightpcg * 255):
        extrapcg = (brightpcg * 255) / common.brightness(col)
        for i in range(3):
            col[i] = int(i * extrapcg)
    return col


def main():
    parseargs()
    img = Image.new("RGB", (args.frames, 1))
    pxls = img.load()

    timings = []
    while sum(timings) < args.frames:
        timings.append(random.randint(args.min, args.max))
    timings.pop()  # We know it's over, so drop an item
    remaining = args.frames - sum(timings)
    if remaining > args.max:
        raise NotImplementedError
        scale = args.max / remaining
    elif remaining < args.min:
        toadd = args.min - remaining
        scale = 1 - (toadd / sum(timings))
    else:
        scale = 0
    if scale:
        for i in range(len(timings)):
            timings[i] = int(timings[i] * scale)
    print("XXX Scale, sum", scale, sum(timings))

    remaining = args.frames - sum(timings)
    startc = getcolor()
    lastc = startc
    i = 0
    while timings:
        thisc = getcolor()
        if args.flash:
            cols = [tuple(lastc)] * timings[0]
        else:
            cols = common.fadecolors(lastc, thisc, timings[0])
        for c in cols:
            pxls[i, 0] = c
            i += 1
        lastc = thisc
        timings.pop(0)
    print("XXX remaining", remaining)
    if args.flash:
        cols = [tuple(lastc)] * remaining
    else:
        cols = common.fadecolors(lastc, thisc, remaining)
    for c in cols:
        pxls[i, 0] = c
        i += 1

    fn = args.filename
    if not fn.endswith(".png"):
        fn += ".png"
    img.save(fn)


if __name__ == "__main__":
    main()
