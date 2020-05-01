#!/usr/bin/env python3
"""Repeats the given pattern to dest filename at the requested interval.
The source pattern can be multiple lines/patterns
"""
import argparse
from PIL import Image
from PIL import ImageColor
import common

args = {}


def parseargs():
    global args
    parser = argparse.ArgumentParser(
        description="Paste the given image, repeating along X at the requested interval"
    )
    parser.add_argument(
        "--color",
        default="#000000",
        type=ImageColor.getrgb,
        help="default background colour for the canvas before the repeating "
        "pattern is applied. an alpha channel is allowed, eg: '#00000000'. "
        "See the PIL/Pillow ImageColor module for more details",
    )
    parser.add_argument("frames", type=int, help="the total number of frames")
    parser.add_argument(
        "patternfilename", help="the source PNG filename get the patterns from"
    )
    parser.add_argument("destfilename", help="the PNG filename to output to")
    parser.add_argument(
        "--repeatcount",
        type=int,
        default=-1,
        help="Number of times to repeat the source is the specified frames "
        "Not compatible and will override --waittime",
    )
    parser.add_argument(
        "--waittime",
        type=int,
        default=0,
        help="The time to wait before inserting the pattern again",
    )
    parser.add_argument(
        "--offset", type=int, default=0, help="The initial offset for the pattern"
    )
    parser.add_argument(
        "--pattern",
        type=int,
        default=0,
        help="Which pattern to start replacing from, default to the first pattern",
    )
    parser.add_argument(
        "--stripalpha",
        action="store_const",
        const=True,
        default=False,
        help="If set, will strip out the alpha channel in the image",
    )
    args = parser.parse_args()


def main():
    parseargs()

    srcimg = Image.open(args.patternfilename)
    img = Image.new("RGBA", (args.frames, srcimg.height), args.color)
    if args.repeatcount > 0:
        spacing = args.frames - (srcimg.width * args.repeatcount)
        spacing = spacing / args.repeatcount
        spacing += srcimg.width
        if spacing < 0:
            raise ValueError(
                "Not enough frames to fit {} repeats at {} wide".format(
                    args.repeatcount, srcimg.width
                )
            )
        for i in range(args.repeatcount):
            offset = i * spacing
            img.paste(srcimg, (int(offset) + args.offset, 0))
    else:
        spacing = srcimg.width + args.waittime
        nextpos = args.offset
        while nextpos < args.frames:
            img.paste(srcimg, (nextpos, 0))
            nextpos += spacing
    common.savefile(img, args.destfilename, args.pattern, args.stripalpha)


if __name__ == "__main__":
    main()
