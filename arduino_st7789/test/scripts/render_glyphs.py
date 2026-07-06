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

def create_font_cpp_header(namespace, font, glyphs, encoding):
    encoder = Encoding.get_function(encoding)
    encoder_cpp_enum = Encoding.get_cpp_enum_string(encoding)

    glyph_images = []

    background_colour = 0
    text_colour = 255

    for glyph in glyphs:
        bbox = font.getbbox(glyph)
        width = bbox[2]-bbox[0]
        height = bbox[3]-bbox[1]

        assert width <= 255, "Width must fit into uint8_t"
        assert height <= 255, "Height must fit into uint8_t"

        image = Image.new("L", (width, height), 0)
        drawer = ImageDraw.Draw(image)
        drawer.text((-bbox[0], -bbox[1]), glyph, font=font, fill=text_colour)
        image = np.array(image)
        glyph_images.append(image)

    max_height = max((image.shape[0] for image in glyph_images))
    max_width = max((image.shape[1] for image in glyph_images))

    glyph_encodings = []
    glyph_shapes = []
    for image in glyph_images:
        height, width = image.shape
        height_pad = max_height-height
        if height_pad > 0:
            # note: padding doesn't add much size since it is amortized by running length encoding compression
            # worst case scenario is an extra byte, best case scenario is no change
            y_pad = (height_pad, 0)
            x_pad = (0, 0)
            image = np.pad(image, [y_pad, x_pad], mode="constant", constant_values=background_colour)
        height, width = image.shape
        encoded_image = encoder(image)
        glyph_shapes.append((width, height))
        glyph_encodings.append(encoded_image)

    table_headers = ["glyph", "width", "height", "original_size", "encoded_size", "ratio", "name"]
    table_rows = []

    data_declarations = []
    glyph_declarations = []
    switch_statement_cases = []

    total_encoded_bytes = 0
    total_original_bytes = 0

    for glyph, (width, height), encoding in zip(glyphs, glyph_shapes, glyph_encodings):
        glyph_name = unicodedata.name(glyph)
        glyph_name = glyph_name.lower().replace('-','_').replace(' ', '_')
        data_body = ','.join((f"0x{value:02X}" for value in encoding))
        encoded_size_bytes = len(encoding)
        if encoded_size_bytes > 0:
            glyph_array_name = f"glyph_data_{glyph_name}"
            data_declaration = f"static const uint8_t {glyph_array_name}[{encoded_size_bytes}] PROGMEM = {{{ data_body }}};"
            data_declarations.append(data_declaration)
        else:
            glyph_array_name = "nullptr"

        glyph_declaration = f"static const glyph::Glyph glyph_{glyph_name} = {{ {width}, {height}, {glyph_array_name}, {encoded_size_bytes}, glyph::{encoder_cpp_enum} }};";
        glyph_declarations.append(glyph_declaration)
        switch_statement_cases.append(f"case 0x{ord(glyph):02X}: return &glyph_{glyph_name};")

        original_size_bytes = width*height
        compression_ratio = encoded_size_bytes/original_size_bytes
        table_rows.append([
            glyph, width, height,
            humanize.naturalsize(original_size_bytes),
            humanize.naturalsize(encoded_size_bytes),
            f"{compression_ratio:.02f}",
            glyph_name,
        ])

        total_encoded_bytes += encoded_size_bytes
        total_original_bytes += original_size_bytes

    total_compression_ratio = total_encoded_bytes/total_original_bytes
    table_rows.append(SEPARATING_LINE)
    table_rows.append([
        None, None, None,
        humanize.naturalsize(total_original_bytes, format="%.3f"),
        humanize.naturalsize(total_encoded_bytes, format="%.3f"),
        f"{total_compression_ratio:.02f}",
        None,
    ])
    print(tabulate(table_rows, table_headers, tablefmt="outline"))

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
    parser.add_argument("--encoding", default=Encoding.GRAYSCALE_RLE_Q4, choices=Encoding.get_choices(), help="Encoding to use")
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

    print(f"Generating glyphs: total={len(glyphs)}, namespace={namespace}, encoding={args.encoding}, size={args.size}")

    header = create_font_cpp_header(namespace, font, glyphs, args.encoding)
    print(f"Writing c++ header to: {output}")
    with open(output, "w") as fp:
        fp.write(header)

if __name__ == "__main__":
    main()

