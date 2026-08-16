#!/usr/bin/env python3
"""
Turn the icon paths exported from Figma into LVGL alpha maps.

    python icons/generate.py

LVGL 8.3 has no SVG renderer, so every glyph in the design has to arrive as
image data.  Each icon here is a single filled path in a single colour, which
makes LV_IMG_CF_ALPHA_8BIT the right container: the array stores coverage
only, one byte per pixel, and the colour stays a runtime style property
(`lv_obj_set_style_img_recolor`).  That is what lets the same battery glyph go
green, amber or red without a second copy of the bitmap.

The SVGs in icons/svg/ are the exact assets Figma exported for node 1:2 -
they are the source of truth, not the generated .c files.  Re-export from
Figma, drop the file in, re-run this, done.

Blur filters in those files are deliberately ignored.  Figma expresses the
warning lamp's orange halo and the glow behind the AUTO BALANCE dot as
feGaussianBlur; LVGL draws those far more cheaply as a style shadow, so they
live in ui/cl_screen.c instead of being baked into pixels here.

Rendering is a scanline fill at 4x supersampling with the nonzero winding
rule, which is what SVG's default fill-rule means.  No dependencies beyond
the standard library.
"""

import math
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SVG_DIR = os.path.join(HERE, "svg")
OUT_DIR = HERE

SS = 4                      # supersampling factor per axis
FLATNESS = 0.2              # curve subdivision tolerance, in output pixels


# --------------------------------------------------------------------------
#  Path parsing
# --------------------------------------------------------------------------

TOKEN = re.compile(r"[MmZzLlHhVvCcSsQqTtAa]|[-+]?(?:\d*\.\d+|\d+\.?)(?:[eE][-+]?\d+)?")


def tokenize(d):
    return TOKEN.findall(d)


def parse_path(d):
    """Flatten an SVG path 'd' string into a list of closed polygons."""
    tokens = tokenize(d)
    i = 0
    polys, cur = [], []
    start = (0.0, 0.0)
    pos = (0.0, 0.0)
    prev_cubic_ctl = None
    prev_quad_ctl = None
    cmd = None

    def num():
        nonlocal i
        v = float(tokens[i])
        i += 1
        return v

    def close_current():
        nonlocal cur
        if len(cur) > 2:
            polys.append(cur)
        cur = []

    while i < len(tokens):
        if tokens[i].isalpha():
            cmd = tokens[i]
            i += 1
            if cmd in "Zz":
                close_current()
                pos = start
                prev_cubic_ctl = prev_quad_ctl = None
                continue
        # An implicit repeat of the previous command; M repeats as L.
        rel = cmd.islower()
        c = cmd.upper()

        if c == "M":
            x, y = num(), num()
            if rel:
                x, y = pos[0] + x, pos[1] + y
            close_current()
            pos = start = (x, y)
            cur = [pos]
            cmd = "l" if rel else "L"
            prev_cubic_ctl = prev_quad_ctl = None

        elif c == "L":
            x, y = num(), num()
            if rel:
                x, y = pos[0] + x, pos[1] + y
            pos = (x, y)
            cur.append(pos)
            prev_cubic_ctl = prev_quad_ctl = None

        elif c == "H":
            x = num()
            if rel:
                x = pos[0] + x
            pos = (x, pos[1])
            cur.append(pos)
            prev_cubic_ctl = prev_quad_ctl = None

        elif c == "V":
            y = num()
            if rel:
                y = pos[1] + y
            pos = (pos[0], y)
            cur.append(pos)
            prev_cubic_ctl = prev_quad_ctl = None

        elif c in ("C", "S"):
            if c == "C":
                x1, y1 = num(), num()
                if rel:
                    x1, y1 = pos[0] + x1, pos[1] + y1
            else:
                x1, y1 = pos if prev_cubic_ctl is None else (
                    2 * pos[0] - prev_cubic_ctl[0], 2 * pos[1] - prev_cubic_ctl[1])
            x2, y2 = num(), num()
            x, y = num(), num()
            if rel:
                x2, y2 = pos[0] + x2, pos[1] + y2
                x, y = pos[0] + x, pos[1] + y
            cubic(cur, pos, (x1, y1), (x2, y2), (x, y))
            prev_cubic_ctl = (x2, y2)
            prev_quad_ctl = None
            pos = (x, y)

        elif c in ("Q", "T"):
            if c == "Q":
                x1, y1 = num(), num()
                if rel:
                    x1, y1 = pos[0] + x1, pos[1] + y1
            else:
                x1, y1 = pos if prev_quad_ctl is None else (
                    2 * pos[0] - prev_quad_ctl[0], 2 * pos[1] - prev_quad_ctl[1])
            x, y = num(), num()
            if rel:
                x, y = pos[0] + x, pos[1] + y
            # A quadratic is a cubic with the control points pulled 2/3 in.
            cubic(cur, pos,
                  (pos[0] + 2.0 / 3 * (x1 - pos[0]), pos[1] + 2.0 / 3 * (y1 - pos[1])),
                  (x + 2.0 / 3 * (x1 - x), y + 2.0 / 3 * (y1 - y)),
                  (x, y))
            prev_quad_ctl = (x1, y1)
            prev_cubic_ctl = None
            pos = (x, y)

        elif c == "A":
            rx, ry, rot, large, sweep = num(), num(), num(), num(), num()
            x, y = num(), num()
            if rel:
                x, y = pos[0] + x, pos[1] + y
            arc(cur, pos, rx, ry, rot, large, sweep, (x, y))
            prev_cubic_ctl = prev_quad_ctl = None
            pos = (x, y)

        else:
            raise ValueError("unsupported path command %r" % cmd)

    close_current()
    return polys


def cubic(out, p0, p1, p2, p3):
    """Adaptive-ish cubic flattening: step count from the control polygon."""
    d = (abs(p1[0] - p0[0]) + abs(p1[1] - p0[1]) +
         abs(p2[0] - p1[0]) + abs(p2[1] - p1[1]) +
         abs(p3[0] - p2[0]) + abs(p3[1] - p2[1]))
    n = max(2, min(160, int(math.sqrt(d / FLATNESS)) + 2))
    for k in range(1, n + 1):
        t = k / n
        u = 1 - t
        out.append((
            u * u * u * p0[0] + 3 * u * u * t * p1[0] + 3 * u * t * t * p2[0] + t * t * t * p3[0],
            u * u * u * p0[1] + 3 * u * u * t * p1[1] + 3 * u * t * t * p2[1] + t * t * t * p3[1],
        ))


def arc(out, p0, rx, ry, rot_deg, large, sweep, p1):
    """SVG endpoint-parameterised elliptical arc -> line segments."""
    if rx == 0 or ry == 0 or p0 == p1:
        out.append(p1)
        return
    rx, ry = abs(rx), abs(ry)
    phi = math.radians(rot_deg)
    cos_p, sin_p = math.cos(phi), math.sin(phi)

    dx2, dy2 = (p0[0] - p1[0]) / 2.0, (p0[1] - p1[1]) / 2.0
    x1p = cos_p * dx2 + sin_p * dy2
    y1p = -sin_p * dx2 + cos_p * dy2

    lam = (x1p * x1p) / (rx * rx) + (y1p * y1p) / (ry * ry)
    if lam > 1:
        s = math.sqrt(lam)
        rx, ry = rx * s, ry * s

    num = rx * rx * ry * ry - rx * rx * y1p * y1p - ry * ry * x1p * x1p
    den = rx * rx * y1p * y1p + ry * ry * x1p * x1p
    co = math.sqrt(max(0.0, num / den)) if den else 0.0
    if large == sweep:
        co = -co
    cxp, cyp = co * rx * y1p / ry, -co * ry * x1p / rx
    cx = cos_p * cxp - sin_p * cyp + (p0[0] + p1[0]) / 2.0
    cy = sin_p * cxp + cos_p * cyp + (p0[1] + p1[1]) / 2.0

    def angle(ux, uy, vx, vy):
        dot = ux * vx + uy * vy
        n = math.hypot(ux, uy) * math.hypot(vx, vy)
        a = math.acos(max(-1.0, min(1.0, dot / n))) if n else 0.0
        return -a if ux * vy - uy * vx < 0 else a

    th1 = angle(1, 0, (x1p - cxp) / rx, (y1p - cyp) / ry)
    dth = angle((x1p - cxp) / rx, (y1p - cyp) / ry,
                (-x1p - cxp) / rx, (-y1p - cyp) / ry)
    if not sweep and dth > 0:
        dth -= 2 * math.pi
    elif sweep and dth < 0:
        dth += 2 * math.pi

    n = max(2, int(abs(dth) / (math.pi / 32)) + 1)
    for k in range(1, n + 1):
        t = th1 + dth * k / n
        out.append((cx + rx * math.cos(t) * cos_p - ry * math.sin(t) * sin_p,
                    cy + rx * math.cos(t) * sin_p + ry * math.sin(t) * cos_p))


# --------------------------------------------------------------------------
#  Rasterising
# --------------------------------------------------------------------------

def rasterise(polys, w, h, ox, oy, scale):
    """Nonzero-winding scanline fill, SSxSS supersampled, into a w*h alpha map."""
    edges = []
    for poly in polys:
        n = len(poly)
        for k in range(n):
            x0, y0 = poly[k]
            x1, y1 = poly[(k + 1) % n]
            x0 = (x0 - ox) * scale * SS
            y0 = (y0 - oy) * scale * SS
            x1 = (x1 - ox) * scale * SS
            y1 = (y1 - oy) * scale * SS
            if y0 == y1:
                continue
            edges.append((y0, y1, x0, (x1 - x0) / (y1 - y0)))

    acc = [0] * (w * h)
    if not edges:
        return bytearray(acc)

    for sy in range(h * SS):
        yc = sy + 0.5
        xs = []
        for y0, y1, x0, slope in edges:
            if (y0 <= yc < y1) or (y1 <= yc < y0):
                xs.append((x0 + (yc - y0) * slope, 1 if y1 > y0 else -1))
        if not xs:
            continue
        xs.sort()

        wind = 0
        row = (sy // SS) * w
        span_start = 0.0
        for x, dirn in xs:
            if wind == 0:
                span_start = x
            wind += dirn
            if wind != 0:
                continue
            # Close a filled span [span_start, x) and accumulate coverage.
            a, b = span_start, x
            if b <= 0 or a >= w * SS:
                continue
            a, b = max(a, 0.0), min(b, float(w * SS))
            ia, ib = int(a), int(math.ceil(b))
            for sx in range(ia, ib):
                cov = min(b, sx + 1.0) - max(a, float(sx))
                if cov > 0:
                    acc[row + sx // SS] += cov

    total = float(SS * SS)
    return bytearray(min(255, int(v / total * 255.0 + 0.5)) for v in acc)


# --------------------------------------------------------------------------
#  SVG reading
# --------------------------------------------------------------------------

def load_svg(name):
    """Return (list of 'd' strings, viewBox width, viewBox height)."""
    with open(os.path.join(SVG_DIR, name), "r", encoding="utf-8") as f:
        text = f.read()
    vb = re.search(r'viewBox="([\d.\-+eE]+)\s+([\d.\-+eE]+)\s+([\d.\-+eE]+)\s+([\d.\-+eE]+)"', text)
    _, _, vw, vh = (float(g) for g in vb.groups())
    return re.findall(r'<path[^>]*\sd="([^"]+)"', text), vw, vh


def rotate180(buf, w, h):
    return bytearray(buf[(h - 1 - y) * w + (w - 1 - x)]
                     for y in range(h) for x in range(w))


# --------------------------------------------------------------------------
#  Emitting
# --------------------------------------------------------------------------

HEADER = """/*
 * %(sym)s.c
 *
 * %(note)s
 *
 * GENERATED by icons/generate.py from icons/svg/%(svg)s - do not hand-edit.
 * %(w)d x %(h)d alpha map, %(bytes)d bytes.  Colour comes from the widget's
 * img_recolor style, not from here.
 */

#include "lvgl.h"

#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

static const LV_ATTRIBUTE_MEM_ALIGN uint8_t %(sym)s_map[] = {
"""

FOOTER = """};

const lv_img_dsc_t %(sym)s = {
    .header.cf          = LV_IMG_CF_ALPHA_8BIT,
    .header.always_zero = 0,
    .header.reserved    = 0,
    .header.w           = %(w)d,
    .header.h           = %(h)d,
    .data_size          = %(bytes)d,
    .data               = %(sym)s_map,
};
"""


def emit(sym, buf, w, h, svg, note):
    fields = dict(sym=sym, w=w, h=h, bytes=w * h, svg=svg, note=note)
    lines = []
    for y in range(h):
        row = buf[y * w:(y + 1) * w]
        for k in range(0, w, 16):
            lines.append("    " + " ".join("0x%02x," % b for b in row[k:k + 16]))
    with open(os.path.join(OUT_DIR, sym + ".c"), "w", encoding="utf-8") as f:
        f.write(HEADER % fields)
        f.write("\n".join(lines))
        f.write("\n" + FOOTER % fields)
    ink = sum(1 for b in buf if b)
    print("  %-24s %3dx%-3d  %5d B  %d%% ink" % (sym, w, h, w * h, 100 * ink // (w * h)))


# --------------------------------------------------------------------------
#  The manifest
# --------------------------------------------------------------------------
#
# size    output pixel size, matching the frame size in Figma
# crop    (x, y) in viewBox units of the output's top-left corner; only the
#         manoeuvre arrow needs one, because Figma exported it inside the
#         83x83 box its drop-shadow occupies rather than its own 44x44 frame
# span    width in viewBox units mapped onto `size` pixels
# paths   which <path> elements to fill; the manoeuvre file also contains the
#         white disc, which LVGL draws as a real circle instead

ICONS = [
    dict(svg="italic_i.svg",   sym="cl_icon_italic_i",  size=28,
         note="Italic I tell-tale, top left of the frame (node 1:6)."),
    dict(svg="headlight.svg",  sym="cl_icon_headlight", size=28,
         note="Low-beam headlamp tell-tale (node 1:14)."),
    dict(svg="warning.svg",    sym="cl_icon_warning",   size=28,
         note="Warning lamp; its orange halo is a style shadow (node 1:19)."),
    dict(svg="bluetooth.svg",  sym="cl_icon_bluetooth", size=28,
         note="Bluetooth link status, top right (node 1:37)."),
    dict(svg="navigation.svg", sym="cl_icon_navigation", size=28,
         note="Navigation-active arrow, top right (node 1:39)."),
    dict(svg="battery.svg",    sym="cl_icon_battery",   size=24,
         note="Battery with four bars, in the status strip (node 1:48)."),
    dict(svg="turn_arrow.svg", sym="cl_icon_turn_right", size=60,
         note="Right turn indicator (node 1:10)."),
    dict(svg="manoeuvre.svg",  sym="cl_icon_manoeuvre", size=44,
         crop=(20.5, 18.5), span=44.0, paths=[-1],
         note="Turn arrow inside the white nav disc (node 1:28)."),
]


# --------------------------------------------------------------------------
#  The mode pill's inner glow
# --------------------------------------------------------------------------
#
# Node 1:32 carries `box-shadow: inset -3px 0 8px 2px #0095ff`, and LVGL has
# no inset shadow - it only casts outwards.  Stacking concentric rounded
# rects to fake it looks banded, so the glow is baked here instead, exactly
# the way CSS defines it:
#
#   take the padding box, shrink it by the spread, shift it by the offset,
#   blur it, and the shadow is whatever coverage that blurred shape does NOT
#   account for, clipped back to the padding box.
#
# The result is one 126x50 alpha map that the screen recolours to #0095ff.
# Because the offset is -3px in x, the hole sits left of centre and the glow
# comes out brightest along the right inner edge - which is what the frame
# shows.

def rounded_rect_coverage(w, h, x0, y0, bw, bh, r):
    """Analytic-ish coverage of a rounded rect, supersampled, as floats."""
    cov = [0.0] * (w * h)
    for py in range(h):
        for px_ in range(w):
            hit = 0
            for sy in range(SS):
                y = py + (sy + 0.5) / SS
                for sx in range(SS):
                    x = px_ + (sx + 0.5) / SS
                    dx = max(x0 + r - x, x - (x0 + bw - r), 0.0)
                    dy = max(y0 + r - y, y - (y0 + bh - r), 0.0)
                    if x0 <= x <= x0 + bw and y0 <= y <= y0 + bh and \
                       math.hypot(dx, dy) <= r:
                        hit += 1
            cov[py * w + px_] = hit / float(SS * SS)
    return cov


def gaussian_blur(buf, w, h, sigma):
    """Separable gaussian on a float buffer, edges extended."""
    if sigma <= 0:
        return list(buf)
    radius = max(1, int(math.ceil(sigma * 3)))
    k = [math.exp(-(i * i) / (2.0 * sigma * sigma)) for i in range(-radius, radius + 1)]
    s = sum(k)
    k = [v / s for v in k]

    tmp = [0.0] * (w * h)
    for y in range(h):
        row = y * w
        for x in range(w):
            acc = 0.0
            for i, kv in enumerate(k):
                xx = min(w - 1, max(0, x + i - radius))
                acc += buf[row + xx] * kv
            tmp[row + x] = acc
    out = [0.0] * (w * h)
    for x in range(w):
        for y in range(h):
            acc = 0.0
            for i, kv in enumerate(k):
                yy = min(h - 1, max(0, y + i - radius))
                acc += tmp[yy * w + x] * kv
            out[y * w + x] = acc
    return out


def bake_inset_glow(w, h, r, dx, dy, blur, spread, amplitude=1.0):
    inside = rounded_rect_coverage(w, h, 0.0, 0.0, w, h, r)
    hole = rounded_rect_coverage(w, h, dx + spread, dy + spread,
                                 w - 2 * spread, h - 2 * spread, max(0.0, r - spread))
    blurred = gaussian_blur(hole, w, h, blur / 2.0)
    return bytearray(
        min(255, max(0, int(amplitude * inside[i] * (1.0 - blurred[i]) * 255.0 + 0.5)))
        for i in range(w * h))


# --------------------------------------------------------------------------
#  The pool of light
# --------------------------------------------------------------------------
#
# Node 1:3 fills the frame with a radial gradient.  Figma gives it exactly:
#
#   radialGradient cx=0 cy=0 r=10
#   gradientTransform matrix(35.874 26.079 -40.037 21.487 400 238.5)
#   stop rgba(16,53,87,1) at 0.21154, stop rgba(5,22,41,0) at 0.82212
#
# so it can be evaluated rather than approximated: map a point back through
# the inverse matrix, take its length over r, and interpolate.  Sampling
# that against the exported frame agrees to within one count per channel,
# which is why this is baked instead of being faked with shadows - LVGL's
# shadow is a box blur bounded by shadow_width, and covering a 400px fade
# would need a blur whose corner buffer runs to megabytes.
#
# It is stored as an alpha map at a quarter scale and drawn zoomed.  The
# gradient's steepest run changes by about three counts across four pixels,
# so bilinear upscaling costs well under one count, and 24 KB of flash
# replaces 384 KB.
#
# Colour also shifts along the ramp, from (16,53,87) to (5,22,41), which a
# single-colour alpha map cannot carry.  It does not need to: the two ends
# are near enough the same hue that encoding the composite as one alpha
# against the core colour reproduces all three channels to within a count.

POOL_SCALE = 4
POOL_W, POOL_H = 800 // POOL_SCALE, 480 // POOL_SCALE

# Node 1:3's own box, whose radius clamps to half its height - so the
# gradient is clipped to a stadium, not to the whole frame.
POOL_BOX_Y, POOL_BOX_H = 3, 477


def bake_pool():
    cx, cy = 400.0, 238.5
    a, b, c, d = 35.874, 26.079, -40.037, 21.487     # column-major, per SVG
    det = a * d - c * b
    r0, r1 = 0.21154, 0.82212
    c0 = (16.0, 53.0, 87.0)
    c1 = (5.0, 22.0, 41.0)
    core_b = c0[2]

    rad = POOL_BOX_H / 2.0
    top, bottom = POOL_BOX_Y, POOL_BOX_Y + POOL_BOX_H
    lcx, rcx = rad, 800.0 - rad                      # stadium cap centres

    out = bytearray(POOL_W * POOL_H)
    for py in range(POOL_H):
        for px_ in range(POOL_W):
            x = (px_ + 0.5) * POOL_SCALE
            y = (py + 0.5) * POOL_SCALE

            # Clip to the stadium the gradient is drawn inside.
            if y < top or y > bottom:
                continue
            if x < lcx and math.hypot(x - lcx, y - (top + rad)) > rad:
                continue
            if x > rcx and math.hypot(x - rcx, y - (top + rad)) > rad:
                continue

            ux, uy = x - cx, y - cy
            gx = (d * ux - c * uy) / det
            gy = (-b * ux + a * uy) / det
            t = math.hypot(gx, gy) / 10.0

            f = (t - r0) / (r1 - r0)
            f = 0.0 if f < 0.0 else (1.0 if f > 1.0 else f)
            alpha = 1.0 - f                          # the stops fade to zero
            blue = (c0[2] + (c1[2] - c0[2]) * f) * alpha
            out[py * POOL_W + px_] = min(255, int(blue / core_b * 255.0 + 0.5))
    return out


def main():
    print("rasterising %d icons at %dx supersampling" % (len(ICONS) + 1, SS))
    for spec in ICONS:
        ds, vw, vh = load_svg(spec["svg"])
        wanted = spec.get("paths")
        if wanted is not None:
            ds = [ds[k] for k in wanted]

        polys = []
        for d in ds:
            polys.extend(parse_path(d))

        size = spec["size"]
        ox, oy = spec.get("crop", (0.0, 0.0))
        span = spec.get("span", vw)
        buf = rasterise(polys, size, size, ox, oy, size / span)
        emit(spec["sym"], buf, size, size, spec["svg"], spec["note"])

        # The left indicator is the right one turned around, per node 1:12.
        if spec["sym"] == "cl_icon_turn_right":
            emit("cl_icon_turn_left", rotate180(buf, size, size), size, size,
                 spec["svg"], "Left turn indicator: node 1:10 rotated 180 (node 1:12).")

    # Node 1:32's stroke is centre-aligned, so its painted box is 132x56 at
    # radius 23, not the 130x54 the node reports - which puts the padding box
    # the shadow lives in at 128x52, radius 21.
    #
    # The shadow parameters are not the ones in the node either.  That says
    # `inset -3px 0 8px 2px`; taking it literally gives a glow half again too
    # bright and twice too wide, because Figma's inner shadow is not the CSS
    # formula its export writes down.  These come from a least-squares fit
    # against the rendered frame and land within four counts of it along both
    # the top edge and the right, where the offset makes the glow strongest.
    emit("cl_glow_pill",
         bake_inset_glow(128, 52, 21.0, dx=-3.0, dy=0.0, blur=5.5, spread=1.0,
                         amplitude=0.81),
         128, 52, "-",
         "Inner glow of the selected ride-mode pill (node 1:32),\n"
         " * baked because LVGL's shadows only cast outwards.")

    # Node 1:45's `inset 0 7px 8px rgba(0,0,0,0.5)`, the shade across the top
    # of the range panel.  A vertical bg gradient gets the ends right and the
    # middle wrong - a blurred edge is an S, a two-stop gradient is a straight
    # line, and the two part company by about 17 counts halfway down.  Fitted
    # the same way as the pill; the amplitude lands on 0.50, which is the
    # alpha the node actually states.
    emit("cl_shade_panel",
         bake_inset_glow(137, 60, 15.0, dx=0.0, dy=8.0, blur=7.0, spread=-1.0,
                         amplitude=0.50),
         137, 60, "-",
         "Inset shade along the top of the range panel (node 1:45).")

    emit("cl_pool_src", bake_pool(), POOL_W, POOL_H, "-",
         "The radial pool of light behind everything (node 1:3),\n"
         " * evaluated from the node's own gradient at 1/%d scale and drawn\n"
         " * zoomed %dx." % (POOL_SCALE, POOL_SCALE))

    print("wrote into %s" % OUT_DIR)


if __name__ == "__main__":
    sys.exit(main())
