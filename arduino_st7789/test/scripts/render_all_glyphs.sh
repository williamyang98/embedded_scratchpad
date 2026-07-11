#!/bin/sh

python ./render_glyphs.py --size 50 --glyphs "0123456789C-°.%:APM " --encoding grayscale_rle_q4 --namespace large_font --output ../src/glyphs/large_font.hpp
python ./render_glyphs.py --size 20 --glyphs "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ°.:% " --encoding grayscale_q4 --namespace small_font --output ../src/glyphs/small_font.hpp
python ./quantize_images.py --namespace icons --output ../src/glyphs/icons.hpp
