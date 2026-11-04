"""Extract, tint, and BC1-compress the two vanilla atlas patches we reuse.

Run through Blender Python so its image reader can decode the game's DDS files.
The resulting local textures no longer rely on game material color multipliers,
which were not visibly affecting these atlas-backed submaterials in the game.
"""

from array import array
import struct

import bpy


def _rgb565(rgb):
    r, g, b = rgb
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def _from_rgb565(value):
    r = (value >> 11) & 31
    g = (value >> 5) & 63
    b = value & 31
    return ((r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2))


def _encode_bc1(width, height, pixels):
    """Encode top-down RGB pixels as opaque DXT1/BC1 with edge padding."""
    blocks_x = (width + 3) // 4
    blocks_y = (height + 3) // 4
    output = bytearray()
    for by in range(blocks_y):
        for bx in range(blocks_x):
            block = []
            for dy in range(4):
                y = min(by * 4 + dy, height - 1)
                for dx in range(4):
                    x = min(bx * 4 + dx, width - 1)
                    block.append(pixels[y * width + x])

            # Luminance endpoints preserve the broad light/dark metal variation.
            ordered = sorted(block, key=lambda c: 0.2126 * c[0] + 0.7152 * c[1] + 0.0722 * c[2])
            c0 = _rgb565(ordered[-1])
            c1 = _rgb565(ordered[0])
            if c0 <= c1:
                if c0 < 0xFFFF:
                    c0, c1 = c1, c0
                else:
                    c0, c1 = 0xFFFF, 0xFFFE
            p0, p1 = _from_rgb565(c0), _from_rgb565(c1)
            palette = (
                p0,
                p1,
                tuple((2 * p0[i] + p1[i]) // 3 for i in range(3)),
                tuple((p0[i] + 2 * p1[i]) // 3 for i in range(3)),
            )
            bits = 0
            for index, color in enumerate(block):
                best = min(range(4), key=lambda i: sum((color[c] - palette[i][c]) ** 2 for c in range(3)))
                bits |= best << (index * 2)
            output.extend(struct.pack("<HHI", c0, c1, bits))
    return bytes(output)


def _write_dds(path, width, height, pixels):
    blocks_x = (width + 3) // 4
    blocks_y = (height + 3) // 4
    linear_size = blocks_x * blocks_y * 8
    # DDS_HEADER + opaque DXT1 payload; no mip chain is required for these assets.
    header = struct.pack(
        "<7I",
        124,
        0x00081007,  # CAPS | HEIGHT | WIDTH | PIXELFORMAT | LINEARSIZE
        height,
        width,
        linear_size,
        0,
        0,
    )
    header += bytes(44)  # reserved1[11]
    header += struct.pack("<8I", 32, 0x4, int.from_bytes(b"DXT1", "little"), 0, 0, 0, 0, 0)
    header += struct.pack("<5I", 0x1000, 0, 0, 0, 0)  # texture cap, no mipmaps
    with open(path, "wb") as target:
        target.write(b"DDS ")
        target.write(header)
        target.write(_encode_bc1(width, height, pixels))


def bake_patch(source_path, output_path, rect, tint, contrast=1.0, lift=0.0, pad_to_block=True):
    image = bpy.data.images.load(str(source_path), check_existing=False)
    image.colorspace_settings.name = "Non-Color"
    width, height = (int(image.size[0]), int(image.size[1]))
    raw = array("f", [0.0]) * (width * height * 4)
    image.pixels.foreach_get(raw)
    x0, y0, x1, y1 = rect
    patch_width, patch_height = x1 - x0, y1 - y0
    padded_width = (patch_width + 3) & ~3 if pad_to_block else patch_width
    padded_height = (patch_height + 3) & ~3 if pad_to_block else patch_height
    pixels = []
    for y in range(padded_height):
        source_y = y0 + min(y, patch_height - 1)
        blender_y = height - 1 - source_y
        for x in range(padded_width):
            source_x = x0 + min(x, patch_width - 1)
            offset = (blender_y * width + source_x) * 4
            rgb = tuple(
                max(0, min(255, round(((raw[offset + i] * 255 - 128) * contrast + 128 + lift) * tint[i])))
                for i in range(3)
            )
            pixels.append(rgb)
    _write_dds(output_path, padded_width, padded_height, pixels)
    bpy.data.images.remove(image)
    return padded_width, padded_height
