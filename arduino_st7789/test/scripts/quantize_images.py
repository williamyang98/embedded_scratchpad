from PIL import Image, ImageDraw
import argparse
import numpy as np
import os
from tabulate import tabulate, SEPARATING_LINE
import humanize

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--images", default="./icons", type=str)
    parser.add_argument("--output", default="./glyphs/icons.hpp", type=str)
    parser.add_argument("--namespace", default="icons", type=str)
    args = parser.parse_args()

    all_images = []
    all_names = []

    images_dir = args.images
    for filename in os.listdir(images_dir):
        filepath = os.path.join(images_dir, filename)
        if not os.path.isfile(filepath):
            continue
        name = os.path.splitext(filename)[0]
        with open(filepath, "rb") as fp:
            image = Image.open(fp)
            image = image.convert("RGBA")
            image = np.array(image)
            all_images.append(image)
            all_names.append(name)

    all_total_solid_colours = []
    all_solid_masks = []
    all_solid_colours = []
    for image in all_images:
        height, width, channels = image.shape
        assert width <= 255, "Width must fit into uint8_t"
        assert height <= 255, "Height must fit into uint8_t"
        solid_mask = image[:,:,3] > 127
        all_solid_masks.append(solid_mask)

        solid_colours = image[solid_mask,0:3]
        all_total_solid_colours.append(solid_colours.shape[0])
        all_solid_colours.append(solid_colours)

    all_solid_colours = np.concat(all_solid_colours, axis=0)
    print(f"Quantizing {all_solid_colours.shape[0]} colours from {len(all_images)} images")

    from sklearn.cluster import KMeans
    # reserve 0xFF=255 for full transparency
    k_means = KMeans(n_clusters=255)
    k_means.fit(all_solid_colours)

    rgb888_colour_palette = k_means.cluster_centers_.astype(np.float32)
    rgb565_colour_palette = []
    for rgb888 in rgb888_colour_palette:
        r8,g8,b8 = rgb888
        r5 = int(np.clip(np.round((r8/255.0)*31.0), 0, 31))
        g6 = int(np.clip(np.round((g8/255.0)*63.0), 0, 63))
        b5 = int(np.clip(np.round((b8/255.0)*31.0), 0, 31))
        rgb565 = (r5 << 11) | (g6 << 5) | b5
        rgb565_colour_palette.append(rgb565)

    palette_body = ','.join(f"0x{value:04X}" for value in rgb565_colour_palette)

    all_quantized_images = []
    end_indices = np.cumsum(all_total_solid_colours)[:-1]
    all_labels = np.split(k_means.labels_, end_indices)

    table_headers = ["icon", "width", "height", "size"]
    table_rows = []
    data_declarations = []
    glyph_declarations = []
    all_total_bytes = 0

    encoding_cpp_enum = "glyph::Encoding::RGBA_Q256_PALETTE"

    max_width = max((image.shape[1] for image in all_images))
    max_height = max((image.shape[0] for image in all_images))
 
    for solid_mask, labels, name in zip(all_solid_masks, all_labels, all_names):
        height, width = solid_mask.shape
        quantized_image = np.zeros((height, width), dtype=np.uint8)
        quantized_image[~solid_mask] = 255
        quantized_image[solid_mask] = labels
        total_bytes = width*height

        data_body = ','.join((f"0x{value:02X}" for value in quantized_image.flatten()))
        icon_array_name = f"icon_data_{name}"
        data_declaration = f"static const uint8_t {icon_array_name}[{total_bytes}] PROGMEM = {{{ data_body }}};"
        data_declarations.append(data_declaration)

        glyph_declaration = f"static const glyph::Glyph icon_{name} = {{ {width}, {height}, {icon_array_name}, {total_bytes}, {encoding_cpp_enum} }};";
        glyph_declarations.append(glyph_declaration)

        all_quantized_images.append(quantized_image)
        all_total_bytes += total_bytes
        table_rows.append([
            name,
            width, height,
            humanize.naturalsize(total_bytes),
        ])

    table_rows.append(SEPARATING_LINE)
    table_rows.append([
        None,
        None, None,
        humanize.naturalsize(all_total_bytes, format="%.3f"),
    ])
    print(tabulate(table_rows, table_headers, tablefmt="outline"))

    header = f"""#pragma once
#include "./glyph.hpp"
#include <stdint.h>

namespace {args.namespace} {{

static const uint16_t RGB565_COLOUR_PALETTE[{len(rgb565_colour_palette)}] PROGMEM = {{{palette_body}}};

{'\n'.join(data_declarations)}

{'\n'.join(glyph_declarations)}


static const uint16_t MAX_HEIGHT = {max_height};
static const uint16_t MAX_WIDTH = {max_height};
static const uint8_t TOTAL_ICONS = {len(all_names)};
static const glyph::Glyph* icons[TOTAL_ICONS] = {{{','.join(f"&icon_{name}" for name in all_names)}}};

}};
"""

    print(f"Writing c++ header to: {args.output}")
    with open(args.output, "w") as fp:
        fp.write(header)

if __name__ == "__main__":
    main()
