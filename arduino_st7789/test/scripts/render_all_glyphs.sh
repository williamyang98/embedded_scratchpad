#!/bin/sh

python ./render_glyphs.py --size 50 --glyphs "0123456789C-°.%:APM " --encoding grayscale_rle_q4 --namespace large_font --output ./glyphs/large_font.hpp
python ./render_glyphs.py --size 20 --glyphs "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ°.:% " --encoding grayscale_q4 --namespace small_font --output ./glyphs/small_font.hpp
