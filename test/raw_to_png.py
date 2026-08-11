"""Convert the raw BGRA dump from render_preview into a PNG.

    python test/raw_to_png.py frame.bin preview.png [width] [height]

Pure stdlib - no Pillow needed.
"""
import sys, zlib, struct

src = sys.argv[1]
dst = sys.argv[2]
W = int(sys.argv[3]) if len(sys.argv) > 3 else 800
H = int(sys.argv[4]) if len(sys.argv) > 4 else 480

raw = open(src, 'rb').read()

# Pixel size is inferred from the dump: 4 = LVGL LV_COLOR_DEPTH 32 (BGRA),
# 2 = LV_COLOR_DEPTH 16 (RGB565 little-endian).
bpp = len(raw) // (W * H)
if bpp not in (2, 4):
    sys.exit("expected %dx%d at 2 or 4 bytes/px, got %d bytes" % (W, H, len(raw)))

rows = bytearray()
for y in range(H):
    rows.append(0)                       # PNG filter type: none
    base = y * W * bpp
    for x in range(W):
        o = base + x * bpp
        if bpp == 4:
            b, g, r = raw[o], raw[o + 1], raw[o + 2]
        else:
            v = raw[o] | (raw[o + 1] << 8)
            r = ((v >> 11) & 0x1F) * 255 // 31
            g = ((v >> 5) & 0x3F) * 255 // 63
            b = (v & 0x1F) * 255 // 31
        rows += bytes((r, g, b))


def chunk(tag, data):
    return (struct.pack('>I', len(data)) + tag + data +
            struct.pack('>I', zlib.crc32(tag + data) & 0xffffffff))


png = b'\x89PNG\r\n\x1a\n'
png += chunk(b'IHDR', struct.pack('>IIBBBBB', W, H, 8, 2, 0, 0, 0))
png += chunk(b'IDAT', zlib.compress(bytes(rows), 9))
png += chunk(b'IEND', b'')
open(dst, 'wb').write(png)
print('wrote', dst)
