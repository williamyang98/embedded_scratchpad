from PIL import Image, ImageDraw, ImageFont
import logging
import os
import numpy as np

logger = logging.getLogger(__name__)

# input image is u8 grayscale
# output image is u4 grayscale
def quantize_u8_to_u4(image):
    assert image.dtype == np.uint8
    U8_MAX_VALUE = 255
    U4_MAX_VALUE = 3
    image = image.astype(np.float32)
    image = image/U8_MAX_VALUE
    image = image*U4_MAX_VALUE
    image = np.round(image)
    # image[image >= 2] = 3
    image = image.astype(np.uint8)
    return image

# image is u4 grayscale
def get_running_length_encoding(image):
    assert image.dtype == np.uint8

    class BitPusher:
        def __init__(self):
            self._data = [0x00]
            self.bit_index = 0

        def push(self, data, n_bits):
            self._data[-1] |= (data << self.bit_index) & 0xFF

            remaining_bits = 8-self.bit_index
            shift_bits = min(n_bits, remaining_bits)

            self.bit_index += shift_bits
            if self.bit_index == 8:
                self.bit_index = 0
                self._data.append(0x00)

            data = data >> shift_bits
            n_bits -= shift_bits
            if n_bits > 0:
                self.push(data, n_bits)

        def get_data(self):
            if self.bit_index == 0:
                return self._data[:-1]
            return self._data

    bits = BitPusher()
    running_length = None
    running_value = None

    image = image.flatten()
    for pixel in image:
        if pixel == running_value:
            running_length += 1
            if running_length == 255:
                bits.push(running_length, 8)
                running_length = None
                running_value = None
            continue

        if running_length != None:
            bits.push(running_length, 8)
            running_length = None
            running_value = None

        bits.push(pixel, 2)
        if pixel == 0 or pixel == 3:
            running_length = 1
            running_value = pixel
        else:
            running_length = None
            running_value = None

    if running_length != None:
        bits.push(running_length, 8)

    rle_data = np.array(bits.get_data(), dtype=np.uint8)
    return rle_data


def create_font_cpp_header(name, font, glyphs):
    glyph_shapes = []
    glyph_rles = []

    for glyph in glyphs:
        bbox = font.getbbox(glyph)
        width = bbox[2]-bbox[0]
        height = bbox[3]-bbox[1]

        assert width <= 255
        assert height <= 255

        background_colour = 0
        text_colour = 255

        image = Image.new("L", (width, height), 0)
        drawer = ImageDraw.Draw(image)
        drawer.text((-bbox[0], -bbox[1]), glyph, font=font, fill=text_colour)
        logger.info(f"width={width}, height={height}, glyph={glyph}")
        image_u8 = np.array(image)
        image_u4 = quantize_u8_to_u4(image_u8)
        image_rle = get_running_length_encoding(image_u4)
        glyph_shapes.append((width, height))
        glyph_rles.append(image_rle)

    total_bytes = sum((len(rle) for rle in glyph_rles))
    logger.info(f"font={name}, total_size={total_bytes} bytes, total_glyphs={len(glyphs)}")

    declarations = []
    for glyph, (width, height), rle in zip(glyphs, glyph_shapes, glyph_rles):
        data_body = ','.join((f"0x{value:02X}" for value in rle))
        data_declaration = f"static const uint8_t glyph_data_{glyph}[{len(rle)}] PROGMEM = {{{ data_body }}};"
        glyph_declaration = f"static const Glyph glyph_{glyph} = {{ {width}, {height}, glyph_data_{glyph}, {len(rle)} }};";
        declarations.append(data_declaration)
        declarations.append(glyph_declaration);

    switch_statement_cases = []
    for glyph in glyphs:
        switch_statement_cases.append(f"case '{glyph}': return &glyph_{glyph};")

    header = f"""#pragma once
#include "./glyph.hpp"

#ifndef PROGMEM
#define PROGMEM
#endif

namespace {name} {{

{'\n'.join(declarations)}

static const Glyph* get_glyph(char c) {{
    switch (c) {{
{'\n'.join(('    ' + case for case in switch_statement_cases))}
    default: return nullptr;
    }}
}}

}};
"""

    return header

def main():
    log_level = os.environ.get("PYTHON_LOG", "INFO").upper()
    logging.basicConfig(level=log_level)

    with open("../fonts/SpaceGrotesk-Medium.ttf", "rb") as fp:
        font = ImageFont.truetype(fp, 64)

    glyphs = "0123456789CF"
    font_name = "space_grotesk_medium"
    header = create_font_cpp_header(font_name, font, glyphs)

    glyph_dir = "./glyphs"
    os.makedirs(glyph_dir, exist_ok=True)
    glyph_path = os.path.join(glyph_dir, f"{font_name}.hpp")
    with open(glyph_path, "w") as fp:
        fp.write(header)

if __name__ == "__main__":
    main()

