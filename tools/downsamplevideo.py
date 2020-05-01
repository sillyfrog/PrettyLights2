#!/usr/bin/env python3
"""Sample script do downsample a video (at the path given on sys.argv), 
convert it to an image WIDTH x HEIGHT, and save each pixel suitable for
importing into OUT_FN FRAMES is the number of frames configured in patterns

The pixels are written out row by row (ie: English reading order)

"""
import numpy as np
import cv2
import sys

FN = sys.argv[1]
OUT_FN = "rgbpixels.png"
WIDTH = 5
HEIGHT = 4
FRAMES = 1500


def main():
    img = np.zeros([(WIDTH * HEIGHT), FRAMES, 3], np.uint8)
    cap = cv2.VideoCapture(FN)

    framei = 0
    while 1:
        ret, frame = cap.read()
        if frame is None:
            break
        res = cv2.resize(frame, (WIDTH, HEIGHT), interpolation=cv2.INTER_AREA)
        for w in range(WIDTH):
            for h in range(HEIGHT):
                x = h * WIDTH + w
                img[x, framei] = res[h, w]

        cv2.imshow("frame", res)
        cv2.imshow("img", img)
        if cv2.waitKey(1) & 0xFF == ord("q"):
            break
        framei += 1

    cv2.imwrite(OUT_FN, img)
    cap.release()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()
