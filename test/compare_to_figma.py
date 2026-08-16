#!/usr/bin/env python3
"""
Measure the build against the Figma frame.

    python test/compare_to_figma.py build/preview.png docs/figma-reference.png

Eyeballing a render next to a design catches gross mistakes and nothing
else - a caption three pixels low looks fine until it is next to the thing
it was copied from.  So this does two jobs:

  * per-element ink offsets.  For each region listed in ELEMENTS it finds
    the bounding box of the drawn pixels in both images and reports the
    difference, which is the number to move the widget by.

  * profile probes.  For the soft things - the pool of light, the pill's
    inner glow, the range panel's ramp - a bounding box means nothing, so
    those are sampled along a line and compared value by value.

It also writes a side-by-side and a difference image next to the input.
"""

import os
import sys

from PIL import Image, ImageChops

# name, (x0, y0, x1, y1), brightness threshold, neutral-only?
#
# `neutral` restricts the measurement to greys and whites - type and the
# monochrome glyphs.  Without it a caption sitting on the mode pill measures
# the blue glow behind it instead of the letters, and the numbers move
# around for reasons that have nothing to do with the text.
#
# Regions are kept clear of the stadium hairline at y=3 and of each other;
# the two status-strip captions in particular have to start below it.
ELEMENTS = [
    ("italic I",        (5, 66, 40, 100),    70,  False),
    ("headlight",       (33, 30, 70, 66),    70,  False),
    ("warning lamp",    (72, 6, 106, 40),    150, True),
    ("clock",           (300, 8, 392, 30),   60,  True),
    ("state of charge", (408, 8, 446, 30),   60,  True),
    ("battery",         (446, 2, 480, 32),   90,  False),
    ("nav lamp",        (694, 4, 730, 40),   70,  False),
    ("bluetooth",       (729, 30, 766, 66),  60,  False),
    ("network",         (756, 70, 792, 100), 90,  True),
    ("nav disc",        (300, 44, 366, 110), 150, True),
    ("street",          (370, 48, 560, 76),  90,  True),
    ("distance",        (370, 78, 500, 104), 90,  True),
    ("turn left",       (240, 118, 302, 182), 90, False),
    ("turn right",      (487, 118, 549, 182), 90, False),
    ("speed",           (300, 120, 500, 276), 150, True),
    ("km/hr unit",      (368, 278, 434, 304), 120, True),
    ("mode prev",       (78, 158, 220, 196), 120, True),
    ("mode next",       (78, 288, 220, 326), 120, True),
    ("mode current",    (98, 222, 205, 262), 150, True),
    ("mode dot",        (46, 229, 72, 255),  200, True),
    ("odometer",        (600, 172, 764, 203), 120, True),
    ("odometer cap",    (600, 204, 764, 232), 90, True),
    ("range panel",     (610, 234, 760, 308), 90, True),
    ("auto balance",    (300, 386, 500, 416), 120, True),
    ("ab hint",         (300, 418, 500, 442), 70, True),
]

# name, list of sample points.  These avoid the mode pill, which is opaque
# enough to hide the pool underneath it.
PROFILES = [
    ("pool across",  [(x, 330) for x in range(20, 790, 55)]),
    ("pool down",    [(400, y) for y in [20, 50, 80, 110, 330, 350, 470]] +
                     [(250, 430)]),
    ("pill glow top",   [(147, y) for y in range(214, 226)]),
    ("pill glow right", [(x, 239) for x in range(197, 212)]),
    ("range ramp",      [(700, y) for y in range(241, 268, 3)]),
    ("balance curve",   [(400, y) for y in range(358, 372)]),
]


def load(path):
    im = Image.open(path).convert("RGB")
    if im.size != (800, 480):
        sys.exit("%s is %dx%d, expected 800x480" % (path, im.size[0], im.size[1]))
    return im


def ink_bbox(im, box, thresh, neutral):
    x0, y0, x1, y1 = box
    px = im.load()
    xs, ys = [], []
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            p = px[x, y]
            if max(p) < thresh:
                continue
            if neutral and (max(p) - min(p)) > 40:
                continue
            xs.append(x)
            ys.append(y)
    if not xs:
        return None
    return (min(xs), min(ys), max(xs), max(ys))


def main():
    mine_path = sys.argv[1] if len(sys.argv) > 1 else "build/preview.png"
    ref_path = sys.argv[2] if len(sys.argv) > 2 else "docs/figma-reference.png"
    mine, ref = load(mine_path), load(ref_path)

    print("%-17s %-22s %-22s %s" % ("element", "figma  x,y  w,h", "build  x,y  w,h", "offset"))
    print("-" * 78)
    worst = []
    for name, box, thresh, neutral in ELEMENTS:
        a = ink_bbox(ref, box, thresh, neutral)
        b = ink_bbox(mine, box, thresh, neutral)
        if a is None or b is None:
            print("%-17s %s" % (name, "MISSING in figma" if a is None else "MISSING in build"))
            continue
        dx, dy = b[0] - a[0], b[1] - a[1]
        dw = (b[2] - b[0]) - (a[2] - a[0])
        dh = (b[3] - b[1]) - (a[3] - a[1])
        flag = "  <<" if max(abs(dx), abs(dy)) > 2 or max(abs(dw), abs(dh)) > 3 else ""
        print("%-17s %-22s %-22s dx%+d dy%+d dw%+d dh%+d%s"
              % (name,
                 "%d,%d  %dx%d" % (a[0], a[1], a[2] - a[0] + 1, a[3] - a[1] + 1),
                 "%d,%d  %dx%d" % (b[0], b[1], b[2] - b[0] + 1, b[3] - b[1] + 1),
                 dx, dy, dw, dh, flag))
        if flag:
            worst.append(name)

    print()
    for name, pts in PROFILES:
        print("%s" % name)
        pr, pm = ref.load(), mine.load()
        for (x, y) in pts:
            a, b = pr[x, y], pm[x, y]
            d = max(abs(a[i] - b[i]) for i in range(3))
            print("   (%3d,%3d)  figma %-16s build %-16s  max delta %3d%s"
                  % (x, y, str(a), str(b), d, "  <<" if d > 22 else ""))

    diff = ImageChops.difference(mine, ref)
    stat = diff.convert("L")
    hist = stat.histogram()
    total = sum(hist)
    over8 = sum(hist[9:])
    print("\npixels differing by more than 8/255: %d (%.2f%%)" % (over8, 100.0 * over8 / total))
    if worst:
        print("elements out of place: %s" % ", ".join(worst))

    out = os.path.dirname(mine_path) or "."
    sbs = Image.new("RGB", (800, 968), (24, 24, 28))
    sbs.paste(ref, (0, 0))
    sbs.paste(mine, (0, 488))
    sbs.save(os.path.join(out, "compare.png"))
    diff.point(lambda v: min(255, v * 4)).save(os.path.join(out, "diff.png"))
    print("wrote %s/compare.png and %s/diff.png" % (out, out))


if __name__ == "__main__":
    main()
