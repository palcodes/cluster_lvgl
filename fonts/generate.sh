#!/usr/bin/env bash
#
# Regenerate the cluster's LVGL fonts.
#
#   bash fonts/generate.sh
#
# The Figma frame (node 1:2) specifies exactly two families:
#
#   Bai Jamjuree Medium   the speed numeral and the RANGE caption
#   Jura SemiBold / Bold  everything else
#
# Both are SIL Open Font License 1.1, so they are free to embed in firmware.
# Needs node (for npx) and curl; nothing is installed globally.
#
# Faces are subsetted by what they actually have to render.  Captions that
# never change carry only their own letters; anything that shows live data
# carries printable ASCII so the string can be anything at runtime.  If you
# add a face, keep cl_fonts.h in step.

set -e

HERE="$(cd "$(dirname "$0")" && pwd)"
TTF="$HERE/.ttf"
CONV="npx --yes lv_font_conv"

# The css2 endpoint only serves TrueType to a pre-woff user agent.
UA="Mozilla/5.0 (Macintosh; U; Intel Mac OS X 10_6_3; en-us) AppleWebKit/533.16 (KHTML, like Gecko) Version/5.0 Safari/533.16"

mkdir -p "$TTF"

fetch() {                       # fetch <family> <weight> <outfile>
    [ -f "$TTF/$3" ] && return 0
    echo "fetching $1 $2"
    url=$(curl -sfL -A "$UA" "https://fonts.googleapis.com/css2?family=$1:wght@$2" \
          | grep -o 'https://[^)]*')
    [ -n "$url" ] || { echo "no TTF url for $1 $2"; exit 1; }
    curl -sfL -o "$TTF/$3" "$url"
}

fetch "Bai+Jamjuree" 500 "BaiJamjuree-500.ttf"
fetch "Jura"         600 "Jura-600.ttf"
fetch "Jura"         700 "Jura-700.ttf"

BAI="$TTF/BaiJamjuree-500.ttf"
SEMI="$TTF/Jura-600.ttf"
BOLD="$TTF/Jura-700.ttf"

COMMON="--bpp 4 --format lvgl --no-compress --lv-include lvgl.h"
ASCII="-r 0x20-0x7E"
# Capitals plus the space, which has to come in as a range: a trailing blank
# inside --symbols is eaten by word splitting and the caption then renders
# with a placeholder box between the words.
CAPS="ABCDEFGHIJKLMNOPQRSTUVWXYZ"
SPACE="-r 0x20"

# --- Bai Jamjuree -------------------------------------------------------
echo "cl_font_speed_136"          # "56" - digits only, this face is expensive
$CONV --font "$BAI" --symbols "0123456789" \
      --size 136 $COMMON -o "$HERE/cl_font_speed_136.c"

echo "cl_font_range_15"           # "RANGE"
$CONV --font "$BAI" --symbols "$CAPS" $SPACE \
      --size 15 $COMMON -o "$HERE/cl_font_range_15.c"

# --- Jura Bold ----------------------------------------------------------
echo "cl_font_bold_22"            # "AUTO BALANCE"
$CONV --font "$BOLD" --symbols "$CAPS" $SPACE \
      --size 22 $COMMON -o "$HERE/cl_font_bold_22.c"

echo "cl_font_bold_16"            # "KM/HR"
$CONV --font "$BOLD" --symbols "${CAPS}0123456789/" $SPACE \
      --size 16 $COMMON -o "$HERE/cl_font_bold_16.c"

echo "cl_font_bold_14"            # "Press AB to turn on"
$CONV --font "$BOLD" $ASCII \
      --size 14 $COMMON -o "$HERE/cl_font_bold_14.c"

# --- Jura SemiBold ------------------------------------------------------
echo "cl_font_semi_24"            # mode name, odometer, range value
$CONV --font "$SEMI" $ASCII \
      --size 24 $COMMON -o "$HERE/cl_font_semi_24.c"

echo "cl_font_semi_20"            # street name, CRAWL / RUSH
$CONV --font "$SEMI" $ASCII \
      --size 20 $COMMON -o "$HERE/cl_font_semi_20.c"

echo "cl_font_semi_18"            # distance to manoeuvre, network label
$CONV --font "$SEMI" $ASCII \
      --size 18 $COMMON -o "$HERE/cl_font_semi_18.c"

echo "cl_font_semi_16"            # "TOTAL KM"
$CONV --font "$SEMI" --symbols "$CAPS" $SPACE \
      --size 16 $COMMON -o "$HERE/cl_font_semi_16.c"

echo "cl_font_semi_14"            # clock, battery percent
$CONV --font "$SEMI" $ASCII \
      --size 14 $COMMON -o "$HERE/cl_font_semi_14.c"

echo
echo "regenerated into $HERE"
du -ch "$HERE"/cl_font_*.c | tail -1
