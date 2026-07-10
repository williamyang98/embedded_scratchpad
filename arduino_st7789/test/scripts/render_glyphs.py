from PIL import Image, ImageDraw, ImageFont
import logging
import os
import numpy as np
import argparse
import unicodedata
from tabulate import tabulate, SEPARATING_LINE
import humanize
from image_compression import Encoding

logger = logging.getLogger(__name__)

GLYPH_BACKGROUND_COLOUR = 0
GLYPH_TEXT_COLOUR = 255

def get_char_name(char):
    name = unicodedata.name(char)
    name = name.lower()
    name = name.replace('-','_')
    name = name.replace(' ', '_')
    return name

class FontGlyph:
    def __init__(self, image, char):
        self.image = image
        self.char = char
        self.name = get_char_name(char)
        height, width = image.shape
        assert width <= 255, "Width must fit into uint8_t"
        assert height <= 255, "Height must fit into uint8_t"
        assert image.dtype == np.uint8, f"Font glyph image must be np.uint8 but got {image.dtype}"
        self.width = width
        self.height = height
        self.encoding = None
        self.encoded_image = None

    @property
    def original_size_bytes(self):
        return self.width*self.height

    @property
    def encoded_size_bytes(self):
        return len(self.encoded_image)

    @property
    def compression_ratio(self):
        return self.encoded_size_bytes/self.original_size_bytes

    def pad_to_height(self, height):
        height_pad = height-self.height
        assert height_pad >= 0, f"Attempted to trim height of font glyph from {self.height} px to {height} px"
        if height_pad > 0:
            # note: padding doesn't add much size since it is amortized by running length encoding compression
            # worst case scenario is an extra byte, best case scenario is no change
            y_pad = (height_pad, 0)
            x_pad = (0, 0)
            self.image = np.pad(self.image, [y_pad, x_pad], mode="constant", constant_values=GLYPH_BACKGROUND_COLOUR)
            self.height = height

    def encode(self, encoding):
        encoder = Encoding.get_function(encoding)
        encoded_image = encoder(self.image)
        self.encoding = encoding
        self.encoded_image = encoded_image

class FontGlyphCpp:
    def __init__(self, glyph):
        self.glyph = glyph
        data_body = ','.join((f"0x{value:02X}" for value in glyph.encoded_image))
        if glyph.encoded_size_bytes > 0:
            self.array_name = f"glyph_data_{glyph.name}"
            self.array_declaration = f"static const uint8_t {self.array_name}[{glyph.encoded_size_bytes}] PROGMEM = {{{ data_body }}};"
        else:
            self.array_name = "nullptr"

        self.encoder_cpp_enum = Encoding.get_cpp_enum_string(glyph.encoding)
        self.glyph_name = f"glyph_{glyph.name}"
        self.glyph_declaration = f"static const glyph::Glyph {self.glyph_name} PROGMEM = {{ {glyph.width}, {glyph.height}, {self.array_name}, {glyph.encoded_size_bytes}, glyph::{self.encoder_cpp_enum} }};"

        self.ascii_value = ord(glyph.char)
        assert self.ascii_value <= 255, f"Character must be fit within uint8_t either ASCII or extended ASCII but got '{glyph.char}' which has a value of 0x{self.ascii_value}"

class FontGlyphs:
    def __init__(self, namespace, font, chars, encoding):
        self.namespace = namespace
        self.font = font
        self.chars = chars
        self.encoding = encoding

        self.glyphs = []
        for char in self.chars:
            bbox = self.font.getbbox(char)
            width = bbox[2]-bbox[0]
            height = bbox[3]-bbox[1]
            image = Image.new("L", (width, height), GLYPH_BACKGROUND_COLOUR)
            drawer = ImageDraw.Draw(image)
            drawer.text((-bbox[0], -bbox[1]), char, font=self.font, fill=GLYPH_TEXT_COLOUR)
            image = np.array(image)
            glyph = FontGlyph(image, char)
            self.glyphs.append(glyph)

        self.max_height = max((glyph.height for glyph in self.glyphs))
        self.max_width = max((glyph.width for glyph in self.glyphs))

        for glyph in self.glyphs:
            glyph.pad_to_height(self.max_height)
            glyph.encode(self.encoding)

    def print_stats(self):
        table_headers = ["glyph", "width", "height", "original_size", "encoded_size", "ratio", "name"]
        table_rows = []

        total_encoded_bytes = 0
        total_original_bytes = 0
        for glyph in self.glyphs:
            table_rows.append([
                glyph.char, glyph.width, glyph.height,
                humanize.naturalsize(glyph.original_size_bytes),
                humanize.naturalsize(glyph.encoded_size_bytes),
                f"{glyph.compression_ratio:.02f}",
                glyph.name,
            ])
            total_encoded_bytes += glyph.encoded_size_bytes
            total_original_bytes += glyph.original_size_bytes
        total_compression_ratio = total_encoded_bytes/total_original_bytes
        table_rows.append(SEPARATING_LINE)
        table_rows.append([
            None, self.max_width, self.max_height,
            humanize.naturalsize(total_original_bytes, format="%.3f"),
            humanize.naturalsize(total_encoded_bytes, format="%.3f"),
            f"{total_compression_ratio:.02f}",
            None,
        ])
        print(tabulate(table_rows, table_headers, tablefmt="outline"))

    def get_cpp_string(self):
        glyphs_cpp = [FontGlyphCpp(glyph) for glyph in self.glyphs]
        glyphs_cpp = sorted(glyphs_cpp, key=lambda glyph: glyph.ascii_value)

        return \
f"""#pragma once
#include "../../src/glyph.hpp"
#include <stdint.h>

namespace {self.namespace} {{

{'\n'.join((glyph_cpp.array_declaration for glyph_cpp in glyphs_cpp))}

{'\n'.join((glyph_cpp.glyph_declaration for glyph_cpp in glyphs_cpp))}

static constexpr uint16_t MAX_HEIGHT = {self.max_height};
static constexpr uint16_t MAX_WIDTH = {self.max_height};
static constexpr uint8_t TOTAL_GLYPHS = {len(self.glyphs)};

struct GlyphEntry {{
    uint8_t character;
    const glyph::Glyph* glyph;
}};

static const GlyphEntry glyph_entries[TOTAL_GLYPHS] PROGMEM = {{
{'\n'.join((f"    {{ 0x{glyph_cpp.ascii_value:02X}, &{glyph_cpp.glyph_name} }}," for glyph_cpp in glyphs_cpp))}
}};

static const FlashMemory<glyph::Glyph>* get_glyph(uint8_t c) {{
    // binary search through sorted array of glyphs by ascii value
    uint8_t low_i = 0;
    uint8_t high_i = TOTAL_GLYPHS;
    GlyphEntry entry;
    while (low_i < high_i) {{
        const uint8_t mid_i = low_i + (high_i-low_i)/2;
        memcpy_P(&entry, glyph_entries + mid_i, sizeof(GlyphEntry));
        if (entry.character < c) {{
            low_i = mid_i+1;
        }} else if (entry.character > c) {{
            high_i = mid_i;
        }} else {{
            return reinterpret_cast<const FlashMemory<glyph::Glyph>*>(entry.glyph);
        }}
    }}
    return nullptr;
}}

}};
"""

def get_formatted_namespace(name):
    new_name = []
    name = name.replace("-", "_")
    for c in name:
        if c.isupper():
            if len(new_name) > 0 and new_name[-1] != '_':
                new_name.append('_')
        new_name.append(c.lower())
    new_name = ''.join(new_name)
    return new_name

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("font", default="./fonts/SpaceGrotesk-Medium.ttf", nargs="?", help="Truefont filepath")
    parser.add_argument("--output", default=None, help="Filepath for generated header file")
    parser.add_argument("--namespace", default=None, help="Namespace used by generated header file")
    parser.add_argument("--size", default=64, type=float, help="Font size")
    parser.add_argument("--glyphs", default="0123456789CF", type=str, help="Glyphs to generate")
    parser.add_argument("--encoding", default=Encoding.GRAYSCALE_RLE_Q4, choices=Encoding.get_choices(), help="Encoding to use")
    args = parser.parse_args()

    log_level = os.environ.get("PYTHON_LOG", "INFO").upper()
    logging.basicConfig(level=log_level)

    font_filename = os.path.basename(args.font)
    font_basename = os.path.splitext(font_filename)[0]
    formatted_namespace = get_formatted_namespace(font_basename)
    output = args.output
    if output == None:
        output = os.path.join("./glyphs", f"{formatted_namespace}.hpp")
    namespace = args.namespace
    if namespace == None:
        namespace = formatted_namespace

    with open(args.font, "rb") as fp:
        font = ImageFont.truetype(fp, args.size)

    # deduplicate glyphs
    glyphs = {}
    for glyph in args.glyphs:
        count = glyphs.get(glyph, 0)
        count += 1
        glyphs[glyph] = count
    for key, value in glyphs.items():
        if value == 1: continue
        logger.warn(f"Duplicate glyph '{key}' ignored with {value} instances")
    glyphs = sorted(glyphs.keys())
    assert len(glyphs) <= 255, f"Total number of glyphs must fit in uint8_t but got {len(glyphs)} glyphs"

    print(f"Generating glyphs: total={len(glyphs)}, namespace={namespace}, encoding={args.encoding}, size={args.size}")

    font_glyphs = FontGlyphs(namespace, font, glyphs, args.encoding)
    font_glyphs.print_stats()
    cpp_header = font_glyphs.get_cpp_string()
    print(f"Writing c++ header to: {output}")
    with open(output, "w") as fp:
        fp.write(cpp_header)

if __name__ == "__main__":
    main()

