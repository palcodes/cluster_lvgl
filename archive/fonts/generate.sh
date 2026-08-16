#!/usr/bin/env bash
#
# Regenerate the cluster's LVGL fonts.
#
#   bash fonts/generate.sh
#
# Downloads the Outfit static weights from Google Fonts, then runs
# lv_font_conv over them plus the FontAwesome face that ships inside the
# LVGL tree.  Needs node (for npx) and curl.  Set LVGL_DIR if your LVGL
# checkout is not at ../lvgl_src.
#
# Outfit is SIL Open Font License 1.1 - free to embed in firmware.
# FontAwesome 5 Free is CC BY 4.0 + SIL OFL 1.1 for the fonts themselves.

set -e

HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/.." && pwd)"
LVGL_DIR="${LVGL_DIR:-$ROOT/../lvgl_src}"
TTF="$HERE/.ttf"
FA="$LVGL_DIR/scripts/built_in_font/FontAwesome5-Solid+Brands+Regular.woff"
CONV="npx --yes lv_font_conv"

[ -f "$FA" ] || { echo "FontAwesome not found at $FA - set LVGL_DIR"; exit 1; }

# --- fetch Outfit -------------------------------------------------------
# The css2 endpoint only hands out TrueType to a pre-woff user agent.
UA="Mozilla/5.0 (Macintosh; U; Intel Mac OS X 10_6_3; en-us) AppleWebKit/533.16 (KHTML, like Gecko) Version/5.0 Safari/533.16"
mkdir -p "$TTF"
for w in 100 200 300 400 500; do
    [ -f "$TTF/Outfit-$w.ttf" ] && continue
    echo "fetching Outfit $w"
    url=$(curl -sfL -A "$UA" "https://fonts.googleapis.com/css2?family=Outfit:wght@$w" | grep -o 'https://[^)]*')
    curl -sfL -o "$TTF/Outfit-$w.ttf" "$url"
done

# Glyphs pulled from FontAwesome; keep in step with ui/ui_fonts.h
ICONS=0xF293,0xF3C5,0xF0E7,0xF064,0xF3E5,0xF060,0xF061,0xF062,0xF0E2,0xF185,0xF06E,0xF071,0xF240,0xF124

COMMON="--bpp 4 --format lvgl --no-compress --lv-include lvgl.h"

echo "ev_font_speed_92"
$CONV --font "$TTF/Outfit-100.ttf" --symbols "0123456789" \
      --size 92 $COMMON -o "$HERE/ev_font_speed_92.c"

echo "ev_font_num_44"
$CONV --font "$TTF/Outfit-200.ttf" --symbols "0123456789.,:- kKmM" \
      --size 44 $COMMON -o "$HERE/ev_font_num_44.c"

echo "ev_font_num_28"
$CONV --font "$TTF/Outfit-300.ttf" --symbols "0123456789.,:-%kKmMWwChHrsAB /" \
      -r '0xB0' -r '0xB7' --size 28 $COMMON -o "$HERE/ev_font_num_28.c"

echo "ev_font_ui_16"
$CONV --font "$TTF/Outfit-400.ttf" -r '0x20-0x7F' -r '0xB0' -r '0xB7' \
      --font "$FA" -r "$ICONS" \
      --size 16 $COMMON -o "$HERE/ev_font_ui_16.c"

echo "ev_font_cap_13"
$CONV --font "$TTF/Outfit-500.ttf" -r '0x20-0x7F' -r '0xB0' -r '0xB7' \
      --font "$FA" -r "$ICONS" \
      --size 13 $COMMON -o "$HERE/ev_font_cap_13.c"

echo "ev_font_cap_11"
$CONV --font "$TTF/Outfit-500.ttf" -r '0x20-0x7F' -r '0xB0' -r '0xB7' \
      --size 11 $COMMON -o "$HERE/ev_font_cap_11.c"

echo "ev_font_icon_26"
$CONV --font "$FA" -r "$ICONS" --size 26 $COMMON -o "$HERE/ev_font_icon_26.c"

echo "ev_font_icon_38"
$CONV --font "$FA" -r "$ICONS" --size 38 $COMMON -o "$HERE/ev_font_icon_38.c"

echo
echo "regenerated into $HERE"
du -ch "$HERE"/ev_font_*.c | tail -1
