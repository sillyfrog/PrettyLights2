#!/usr/bin/env python3
"""Sample script do downsample a video (at the path FN), convert it to an image
WIDTH x HEIGHT, and save each pixel suitable for importing into OUT_FN
FRAMES is the number of frames configured in patterns

The pixels are written out row by row (ie: English reading order)

"""
import numpy as np
import cv2
import sys

FN = sys.argv[1]
OUT_FN = "rgbpixel.png"
X = 626
Y = 393
FRAMES = 50


def main():
    img = np.zeros([1, FRAMES, 3], np.uint8)
    cap = cv2.VideoCapture(FN)

    framei = 0
    while 1:
        ret, frame = cap.read()
        if frame is None:
            break
        img[0, framei] = frame[Y, X]

        cv2.imshow("frame", frame)
        cv2.imshow("img", img)
        if cv2.waitKey(1000) & 0xFF == ord("q"):
            break
        framei += 1

    cv2.imwrite(OUT_FN, img)
    cap.release()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()
