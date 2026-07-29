#!/bin/sh

SCRIPT_DIR=./scripts/src
SRC_DIR=./components/st7789/glyphs

set -x

python $SCRIPT_DIR/render_glyphs.py --size 50 --glyphs "0123456789C-°.%:APM " --encoding grayscale_rle_q4 --namespace large_font --output $SRC_DIR/large_font.hpp
python $SCRIPT_DIR/render_glyphs.py --size 20 --glyphs "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ°.:% " --encoding grayscale_q4 --namespace small_font --output $SRC_DIR/small_font.hpp
python $SCRIPT_DIR/quantize_images.py --namespace icons --output $SRC_DIR/icons.hpp
