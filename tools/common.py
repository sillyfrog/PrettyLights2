#!/usr/bin/env python3

from PIL import Image


def savefile(img, fn, pattern=-1, stripalpha=False):
    """Saves the image to fn

    If pattern is set (>= 0), then fn is opened, and the img is inserted at
    row pattern.
    """
    if not fn.endswith(".png"):
        fn += ".png"
    if pattern >= 0:
        srcimg = Image.open(fn)
        # Ensure we have an alpha channel to allow an alpha overlay
        if srcimg.mode != "RGBA":
            srcimg = srcimg.convert("RGBA")
        srcimg.alpha_composite(img, (0, pattern))
        if stripalpha:
            img = Image.new("RGB", (srcimg.width, srcimg.height))
            img.paste(srcimg, mask=srcimg)
        else:
            img = srcimg
    img.save(fn)


def brightness(c):
    return 0.299 * c[0] + 0.587 * c[1] + 0.114 * c[2]


def fadecolors(start, end, steps):
    if steps < 1:
        return []
    ranges = []
    start = list(start)
    end = list(end)
    while len(start) < 4:
        start.append(0)
    while len(end) < 4:
        end.append(0)
    for c in range(4):
        ranges.append(end[c] - start[c])
    ret = []
    for i in range(steps):
        col = []
        pcg = i / steps
        for c in range(4):
            colc = start[c] + ranges[c] * pcg
            col.append(int(colc))
        ret.append(tuple(col))
    return ret
