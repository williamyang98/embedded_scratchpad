from PIL import Image, ImageDraw
import argparse
import numpy as np
import os
from tabulate import tabulate, SEPARATING_LINE
import humanize

# reserved labels
# 255: fully transparent
# 254: chroma key for text colour
# 254 remaining uint8 values for palette labels
TRANSPARENCY_PALETTE_VALUE = 255
CHROMA_KEY_PALETTE_VALUE = 254
TOTAL_PALETTE_VALUES = 256-2
CHROMA_KEY = np.array([255,0,255], np.uint8)
ENCODING_CPP_ENUM = "glyph::Encoding::RGBA_Q256_PALETTE"

class Icon:
    def __init__(self, image, name):
        self.name = name
        self.image = image
        height, width, channels = image.shape
        assert width <= 255, "Width must fit into uint8_t"
        assert height <= 255, "Height must fit into uint8_t"
        assert channels == 4, "Image must have 4 channels corresponding to RGBA"

        self.transparency_mask = self.image[:,:,3] < 127
        self.chroma_key_mask = np.all(self.image[:,:,0:3] == CHROMA_KEY, axis=-1)
        self.chroma_key_mask = self.chroma_key_mask & ~self.transparency_mask # ignore chroma values that are transparent
        self.colour_mask = ~(self.chroma_key_mask | self.transparency_mask)
        self.colours = self.image[self.colour_mask,0:3]
        self.quantized_image = np.zeros((height, width), dtype=np.uint8)
        self.quantized_image[self.transparency_mask] = TRANSPARENCY_PALETTE_VALUE
        self.quantized_image[self.chroma_key_mask] = CHROMA_KEY_PALETTE_VALUE

        self.width = width
        self.height = height
        self.channels = channels
        self.total_bytes = self.width*self.height
        self.total_transparency_pixels = np.count_nonzero(self.transparency_mask)
        self.total_chroma_key_pixels = np.count_nonzero(self.chroma_key_mask)
        self.total_colour_pixels = np.count_nonzero(self.colour_mask)

    def update_from_palette_labels(self, labels):
        self.quantized_image[self.colour_mask] = labels

class IconCpp:
    def __init__(self, icon):
        self.icon = icon
        self.array_name = f"icon_data_{icon.name}"
        self.glyph_name = f"icon_{icon.name}"
        self.enum_name = icon.name.upper().replace("-", "_")

        array = icon.quantized_image.flatten()
        array_body_cpp = ','.join((f"0x{value:02X}" for value in array))
        self.array_declaration = f"static const uint8_t {self.array_name}[{icon.total_bytes}] PROGMEM = {{{ array_body_cpp }}};"
        self.glyph_declaration = f"static const glyph::Glyph {self.glyph_name} = {{ {icon.width}, {icon.height}, {self.array_name}, {icon.total_bytes}, {ENCODING_CPP_ENUM} }};"


class IconsFolder:
    def __init__(self, namespace, dirpath):
        self.namespace = namespace
        self.dirpath = dirpath
        self.icons = []
        for filename in os.listdir(dirpath):
            filepath = os.path.join(dirpath, filename)
            if not os.path.isfile(filepath):
                continue
            name = os.path.splitext(filename)[0]
            with open(filepath, "rb") as fp:
                image = Image.open(fp)
                image = image.convert("RGBA")
                image = np.array(image)
                icon = Icon(image, name)
                self.icons.append(icon)

        assert len(self.icons) > 0, f"Folder: '{dirpath}' has no icons"

        self.max_width = max((icon.width for icon in self.icons))
        self.max_height = max((icon.height for icon in self.icons))
        self.total_bytes = sum((icon.total_bytes for icon in self.icons))
        self.total_transparency_pixels = sum((icon.total_transparency_pixels for icon in self.icons))
        self.total_chroma_key_pixels = sum((icon.total_chroma_key_pixels for icon in self.icons))
        self.total_colour_pixels = sum((icon.total_colour_pixels for icon in self.icons))

    def print_folder_stats(self):
        table_headers = ["icon", "width", "height", "size", "transparent_pixels", "chroma_key_pixels", "colour_pixels"]
        table_rows = []

        for icon in self.icons:
            table_rows.append([
                icon.name,
                icon.width, icon.height,
                humanize.naturalsize(icon.total_bytes),
                icon.total_transparency_pixels,
                icon.total_chroma_key_pixels,
                icon.total_colour_pixels,
            ])
        table_rows.append(SEPARATING_LINE)
        table_rows.append([
            None,
            None, None,
            humanize.naturalsize(self.total_bytes, format="%.3f"),
            self.total_transparency_pixels,
            self.total_chroma_key_pixels,
            self.total_colour_pixels,
        ])
        print(tabulate(table_rows, table_headers, tablefmt="outline"))

    def get_cpp_string(self):
        icons_cpp = [IconCpp(icon) for icon in self.icons]
        default_icon_cpp = icons_cpp[0]
        return \
f"""namespace {self.namespace} {{

static constexpr uint16_t MAX_HEIGHT = {self.max_height};
static constexpr uint16_t MAX_WIDTH = {self.max_height};
static constexpr uint8_t TOTAL_ICONS = {len(self.icons)};
{'\n'.join((icon_cpp.array_declaration for icon_cpp in icons_cpp))}

{'\n'.join((icon_cpp.glyph_declaration for icon_cpp in icons_cpp))}

enum class Icon: uint8_t {{
{'\n'.join((f"    {icon_cpp.enum_name}={index}," for index, icon_cpp in enumerate(icons_cpp)))}
}};

static const glyph::Glyph& get_icon(Icon icon) {{
    switch (icon) {{
{'\n'.join((f"    case Icon::{icon_cpp.enum_name}: return {icon_cpp.glyph_name};" for icon_cpp in icons_cpp))}
    default: return {default_icon_cpp.glyph_name};
    }}
}}

}};
"""

class Palette:
    def __init__(self):
        self.icons = []
        self.labels = None
        self.rgb565_palette = None
        self.all_colours = None

    def add_icon(self, icon):
        self.icons.append(icon)

    def generate(self):
        self.all_colours = np.concat([icon.colours for icon in self.icons], axis=0)
        print(f"Registered {len(self.icons)} icons to palette")
        print(f"Attempting to fit {len(self.all_colours)} coloured pixels into {TOTAL_PALETTE_VALUES} colour palette")

        from sklearn.cluster import KMeans
        self.k_means = KMeans(n_clusters=TOTAL_PALETTE_VALUES)
        self.k_means.fit(self.all_colours)

        print("Finished fitting using k-means clustering")
        end_indices = np.cumsum([icon.colours.shape[0] for icon in self.icons])[:-1]
        self.labels = np.split(self.k_means.labels_, end_indices)

        rgb888_palette = self.k_means.cluster_centers_.astype(np.float32)
        self.rgb565_palette = []
        for rgb888 in rgb888_palette:
            r8,g8,b8 = rgb888
            r5 = int(np.clip(np.round((r8/255.0)*31.0), 0, 31))
            g6 = int(np.clip(np.round((g8/255.0)*63.0), 0, 63))
            b5 = int(np.clip(np.round((b8/255.0)*31.0), 0, 31))
            rgb565 = (r5 << 11) | (g6 << 5) | b5
            self.rgb565_palette.append(rgb565)

        assert len(self.labels) == len(self.icons)
        for icon, label in zip(self.icons, self.labels):
            icon.update_from_palette_labels(label)

    def get_cpp_string(self):
        assert self.rgb565_palette != None
        palette_body = ','.join(f"0x{value:04X}" for value in self.rgb565_palette)
        return \
f"""static const uint16_t RGB565_COLOUR_PALETTE[{len(self.rgb565_palette)}] PROGMEM = {{{palette_body}}};
static constexpr uint8_t TRANSPARENCY_PALETTE_VALUE = {TRANSPARENCY_PALETTE_VALUE};
static constexpr uint8_t CHROMA_KEY_PALETTE_VALUE = {CHROMA_KEY_PALETTE_VALUE};"""


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--images", default="./icons", type=str)
    parser.add_argument("--output", default="./glyphs/icons.hpp", type=str)
    parser.add_argument("--namespace", default="icons", type=str)
    args = parser.parse_args()

    images_dir = args.images
    icon_folders = []
    for filename in os.listdir(images_dir):
        filepath = os.path.join(images_dir, filename)
        if not os.path.isdir(filepath):
            continue
        icon_folder = IconsFolder(filename, filepath)
        icon_folders.append(icon_folder)

    palette = Palette()
    for icon_folder in icon_folders:
        for icon in icon_folder.icons:
            palette.add_icon(icon)

    palette.generate()

    for icon_folder in icon_folders:
        print(f"folder: {icon_folder.namespace}")
        icon_folder.print_folder_stats()

    def indent_code_block(code_block, indent='    '):
        lines = code_block.split("\n")
        indented_lines = []
        for line in lines:
            if len(line) == 0:
                indented_lines.append(line)
                continue
            indented_lines.append(indent+line)
        indented_code_block = "\n".join(indented_lines)
        return indented_code_block

    header = f"""#pragma once
#include "./glyph.hpp"
#include <stdint.h>

namespace {args.namespace} {{

{palette.get_cpp_string()}

{'\n\n'.join((indent_code_block(icon_folder.get_cpp_string()) for icon_folder in icon_folders))}

}};
"""

    print(f"Writing c++ header to: {args.output}")
    with open(args.output, "w") as fp:
        fp.write(header)

if __name__ == "__main__":
    main()
