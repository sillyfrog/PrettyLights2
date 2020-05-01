#!/usr/bin/env python3
# Used to make the lightning "effect"
import argparse
import random
from PIL import Image
from PIL import ImageColor
import common


args = None

# Each tuple here is min/max
FLASH_PER_CLUSTER = (2, 5)
DELAY_BETWEEN_FLASH = (3, 9)
FLASH_ON_TIME = (1, 3)
DELAY_BETWEEN_CLUSTER = (25, 50)


def parseargs():
    global args
    parser = argparse.ArgumentParser(
        description="Flicker between 2 colours, for random durations."
    )
    parser.add_argument("frames", type=int, help="the total number of frames")
    parser.add_argument("filename", help="the PNG filename to output to")
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

    pixels = []
    while len(pixels) < args.frames:
        # A cluster of flashes
        for flash in range(random.randint(*FLASH_PER_CLUSTER)):
            pixels += [args.on] * random.randint(*FLASH_ON_TIME)
            pixels += [args.off] * random.randint(*DELAY_BETWEEN_FLASH)

        pixels += [args.off] * random.randint(*DELAY_BETWEEN_CLUSTER)

    for i, p in zip(range(args.frames), pixels[: args.frames]):
        pxls[i, 0] = p

    common.savefile(img, args.filename, args.pattern)


if __name__ == "__main__":
    main()
