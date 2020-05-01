#!/usr/bin/env python3
"""Create a "chaser" effect with a light going bright, then fading away for the
specified timing.
"""

import argparse
from PIL import Image
from PIL import ImageColor
import random
import common

args = {}


def parseargs():
    global args
    parser = argparse.ArgumentParser(
        description='Generate a light with a fading "tail" to give a chasing effect.'
    )
    parser.add_argument("frames", type=int, help="the total number of frames")
    parser.add_argument("filename", help="the PNG filename to output to")
    parser.add_argument(
        "--ontime",
        type=int,
        default=10,
        help="The total frame count to be full on before fade",
    )
    parser.add_argument(
        "--fadeoff", type=int, default=30, help="The time to fade to off"
    )
    parser.add_argument("--fadeon", type=int, default=0, help="The time to fade to on")
    parser.add_argument(
        "--waittime",
        type=int,
        default=30,
        help="The time to wait before starting again",
    )
    parser.add_argument(
        "--waitmin",
        type=int,
        default=-1,
        help="A random minimum time to wait before starting again, overrides --waittime",
    )
    parser.add_argument(
        "--waitmax",
        type=int,
        default=-1,
        help="A random maximum time to wait before starting again, overrides --waittime",
    )
    parser.add_argument(
        "--off", type=ImageColor.getrgb, default="#000000ff", help="The off colour"
    )
    parser.add_argument(
        "--on", type=ImageColor.getrgb, default="#ffffff00", help="The on colour"
    )
    parser.add_argument(
        "--offset",
        type=int,
        default=0,
        help="The offset in frames for when to start the pattern output "
        "this is implemented by rotating at the end so any tails that would have "
        "been cutoff, will be moved to the start.",
    )
    parser.add_argument(
        "--pattern",
        type=int,
        default=-1,
        help="If set, will update an existing image, at the "
        "corresponding row (pattern), otherwise a new image is created.",
    )
    parser.add_argument(
        "--color",
        default="#000000",
        type=ImageColor.getrgb,
        help="default background colour for the canvas putting anything on."
        "An alpha channel is allowed, eg: '#00000000'. "
        "See the PIL/Pillow ImageColor module for more details",
    )
    args = parser.parse_args()


def main():
    parseargs()

    img = Image.new("RGBA", (args.frames, 1), args.color)
    pxls = img.load()

    # Generage a single full fade, including wait, and apply for the rest of the frames
    fullfade = common.fadecolors(args.off, args.on, args.fadeon)
    fullfade += [args.on] * args.ontime
    fullfade += common.fadecolors(args.on, args.off, args.fadeoff)
    randomwait = False
    if args.waitmin >= 0 or args.waitmax >= 0:
        randomwait = True
        wait = 0
    else:
        wait = args.waittime
    fullfade += [args.off] * wait
    allframe = []
    while len(allframe) < args.frames:
        allframe += fullfade
        if randomwait:
            allframe += [args.off] * random.randint(args.waitmin, args.waitmax)
    if len(allframe) > args.frames:
        print("Warning! A full fade cycle does not evenly fit in a full set of frames")
        allframe = allframe[: args.frames]
    rotatecount = args.offset
    while rotatecount > 0:
        tmp = allframe.pop(-1)
        allframe.insert(0, tmp)
        rotatecount -= 1

    for i in range(args.frames):
        pxls[i, 0] = allframe[i]

    common.savefile(img, args.filename, args.pattern)


if __name__ == "__main__":
    main()
