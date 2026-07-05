from PIL import Image, ImageDraw, ImageFont
import logging
import os
import numpy as np
import argparse
import unicodedata

logger = logging.getLogger(__name__)

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

class Encoding:
    BINARY_RLE_U8 = "binary_rle_u8"
    GRAYSCALE_RLE_U4 = "grayscale_rle_u4"

def encoding_to_enum(encoding):
    if encoding == Encoding.BINARY_RLE_U8:
        return "Encoding::BINARY_RLE_U8"
    elif encoding == Encoding.GRAYSCALE_RLE_U4:
        return "Encoding::GRAYSCALE_RLE_U4"
    raise Exception(f"Unknown encoding provided: {encoding}")

# encoding scheme
# 1. quantize grayscale 0-255 to 0-3 inclusive
# 2. store pixel value as 2 bits tightly packed, with successive bits packed into higher bits and bytes
# 3. if pixel value is 0 or 3 store running value as 8 bit value also tightly packed
# 4. if running value reaches 255, push the pixel value again and restart running value from 0
def encode_to_grayscale_running_length_encoded_u4(image):
    assert image.dtype == np.uint8

    U8_MAX_VALUE = 255
    U4_MAX_VALUE = 3
    image = image.astype(np.float32)
    image = image/U8_MAX_VALUE
    image = image*U4_MAX_VALUE
    image = np.round(image)
    image = np.clip(image, 0, U4_MAX_VALUE)
    image = image.astype(np.uint8)

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

    encoding = np.array(bits.get_data(), dtype=np.uint8)
    return encoding

def encode_to_binary_running_length_encoded_u8(image):
    assert image.dtype == np.uint8

    image = (image > 127).flatten()

    running_length = None
    running_value = None
    encoding = []
    for pixel in image:
        if running_value == pixel:
            running_length += 1
            if running_length == 127:
                pixel_bit = 0b10000000 if running_value else 0b00000000
                encoding.append(pixel_bit | running_length)
                running_length = None
                running_value = None
            continue

        if running_length != None:
            pixel_bit = 0b10000000 if running_value else 0b00000000
            encoding.append(pixel_bit | running_length)
            running_length = None
            running_value = None

        running_value = pixel
        running_length = 1

    if running_length != None:
        pixel_bit = 0b10000000 if running_value else 0b00000000
        encoding.append(pixel_bit | running_length)

    encoding = np.array(encoding, dtype=np.uint8)
    return encoding

def create_font_cpp_header(namespace, font, glyphs, encoding):
    glyph_shapes = []
    glyph_encodings = []

    encoder = None
    if encoding == Encoding.BINARY_RLE_U8:
        encoder = encode_to_binary_running_length_encoded_u8
    elif encoding == Encoding.GRAYSCALE_RLE_U4:
        encoder = encode_to_grayscale_running_length_encoded_u4

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
        image = np.array(image)
        encoded_image = encoder(image)
        glyph_shapes.append((width, height))
        glyph_encodings.append(encoded_image)

    total_bytes = sum((len(encoding) for encoding in glyph_encodings))
    logger.info(f"font={namespace}, total_size={total_bytes} bytes, total_glyphs={len(glyphs)}")

    encoding_string = f"glyph::{encoding_to_enum(encoding)}"
    data_declarations = []
    glyph_declarations = []
    switch_statement_cases = []
    max_height = max((y for x, y in glyph_shapes))
    max_width = max((x for x, y in glyph_shapes))

    for glyph, (width, height), encoding in zip(glyphs, glyph_shapes, glyph_encodings):
        glyph_name = unicodedata.name(glyph)
        glyph_name = glyph_name.lower().replace('-','_').replace(' ', '_')
        logger.info(f"glyph='{glyph}', width={width}, height={height}, size_bytes={len(encoding)}, name={glyph_name}")
        data_body = ','.join((f"0x{value:02X}" for value in encoding))
        if len(encoding) > 0:
            glyph_array_name = f"glyph_data_{glyph_name}"
            data_declaration = f"static const uint8_t {glyph_array_name}[{len(encoding)}] PROGMEM = {{{ data_body }}};"
            data_declarations.append(data_declaration)
        else:
            glyph_array_name = "nullptr"

        glyph_declaration = f"static const glyph::Glyph glyph_{glyph_name} = {{ {width}, {height}, {glyph_array_name}, {len(encoding)}, {encoding_string} }};";
        glyph_declarations.append(glyph_declaration)
        switch_statement_cases.append(f"case 0x{ord(glyph):02X}: return &glyph_{glyph_name};")

    header = f"""#pragma once
#include "./glyph.hpp"
#include <stdint.h>

namespace {namespace} {{

{'\n'.join(data_declarations)}

{'\n'.join(glyph_declarations)}

static const uint16_t MAX_HEIGHT = {max_height};
static const uint16_t MAX_WIDTH = {max_height};
static const uint16_t TOTAL_GLYPHS = {len(glyphs)};

static const glyph::Glyph* get_glyph(uint8_t c) {{
    switch (c) {{
{'\n'.join(('    ' + case for case in switch_statement_cases))}
    default: return nullptr;
    }}
}}

}};
"""

    return header

def get_formatted_name(name):
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
    parser.add_argument("--encoding", default=Encoding.GRAYSCALE_RLE_U4, choices=[Encoding.BINARY_RLE_U8, Encoding.GRAYSCALE_RLE_U4], help="Encoding to use")
    args = parser.parse_args()

    log_level = os.environ.get("PYTHON_LOG", "INFO").upper()
    logging.basicConfig(level=log_level)

    font_filename = os.path.basename(args.font)
    font_basename = os.path.splitext(font_filename)[0]
    formatted_namespace = get_formatted_name(font_basename)
    output = args.output
    if output == None:
        output = os.path.join("./glyphs", f"{formatted_namespace}.hpp")
    namespace = args.namespace
    if namespace == None:
        namespace = formatted_namespace

    logger.info(f"Using namespace={namespace}, encoding={args.encoding}, size={args.size}")

    with open(args.font, "rb") as fp:
        font = ImageFont.truetype(fp, args.size)

    glyphs = {}
    for glyph in args.glyphs:
        count = glyphs.get(glyph, 0)
        count += 1
        glyphs[glyph] = count
    for key, value in glyphs.items():
        if value == 1: continue
        logger.warn(f"Duplicate glyph '{key}' ignored with {value} instances")
    glyphs = sorted(glyphs.keys())
    logger.info(f"Generating {len(glyphs)} glyphs")

    header = create_font_cpp_header(namespace, font, glyphs, args.encoding)
    logger.info(f"Writing header to {output}")
    with open(output, "w") as fp:
        fp.write(header)

if __name__ == "__main__":
    main()

