#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

"""
Build-time font generator for OpenRTX.

Reads a TrueType font file and the locale's string table to determine which
Unicode codepoints are needed, then renders bitmap glyphs at each required
point size using FreeType.  Outputs a C source file defining a UniFont array
that is compiled directly into the firmware.

Usage:
    # Generate font C source for a locale
    python3 scripts/gen_font.py \
        --font fonts/NotoSans-Regular.ttf \
        --struct openrtx/include/ui/ui_strings.h \
        [--po openrtx/src/locale/es.po] \
        --sizes 5,6,8,9,10,12,16 \
        --output text_font_generated.c

    # Generate font C source with default ASCII-only (no locale strings)
    python3 scripts/gen_font.py \
        --font fonts/NotoSans-Regular.ttf \
        --sizes 5,6,8,9,10,12,16 \
        --output text_font_generated.c
"""

import argparse
import os
import re
import sys
import datetime

try:
    import freetype
except ImportError:
    print("ERROR: freetype-py is required. Install with: pip install freetype-py",
          file=sys.stderr)
    sys.exit(1)

try:
    import polib
except ImportError:
    polib = None  # optional, only needed when --po is given


# ---------------------------------------------------------------------------
# Struct parsing (reuses logic similar to gen_locale.py)
# ---------------------------------------------------------------------------

def parse_ui_strings(struct_path):
    """Parse ui_strings.h and extract field names and their i18n msgids."""
    fields = []
    with open(struct_path, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    in_struct = False
    i18n_msgid = None

    for line in lines:
        stripped = line.strip()

        # Detect struct body
        if re.match(r'typedef\s+struct', stripped):
            in_struct = True
            continue
        if in_struct and stripped.startswith('}'):
            break
        if not in_struct:
            continue

        # Collect i18n annotations
        m_quoted = re.match(r'//\s*i18n:\s*"(.+)"', stripped)
        if m_quoted:
            i18n_msgid = m_quoted.group(1)
            continue

        # Field declaration
        m_field = re.match(r'const\s+char\s*\*\s*(\w+)\s*;', stripped)
        if m_field:
            field_name = m_field.group(1)
            msgid = i18n_msgid if i18n_msgid else field_name
            fields.append((field_name, msgid))
            i18n_msgid = None
            continue

        # Non-quoted i18n comment (translator note) — skip, don't reset msgid
        if re.match(r'//\s*i18n:', stripped):
            continue

    return fields


def collect_locale_codepoints(struct_path, po_path=None):
    """Collect all Unicode codepoints needed for the given locale."""
    codepoints = set()

    # Always include ASCII printable range (0x20-0x7E)
    for cp in range(0x20, 0x7F):
        codepoints.add(cp)

    # Parse the string table for English msgids
    fields = parse_ui_strings(struct_path)
    for _, msgid in fields:
        for ch in msgid:
            codepoints.add(ord(ch))

    # If a PO file is provided, collect codepoints from translations
    if po_path and os.path.isfile(po_path):
        if polib is None:
            print("ERROR: polib is required when --po is given. "
                  "Install with: pip install polib", file=sys.stderr)
            sys.exit(1)
        po = polib.pofile(po_path)
        for entry in po:
            text = entry.msgstr if entry.msgstr else entry.msgid
            for ch in text:
                codepoints.add(ord(ch))

    return sorted(codepoints)


# ---------------------------------------------------------------------------
# FreeType glyph rendering
# ---------------------------------------------------------------------------

# DPI matching Adafruit GFX fontconvert for consistent sizing
ADAFRUIT_DPI = 141


def render_font_size(font_path, pt_size, codepoints):
    """Render all requested codepoints at the given point size.

    Returns (bitmap_bytes, glyphs_list, y_advance) where:
        bitmap_bytes: packed 1-bit bitmap data (contiguous, not row-aligned)
        glyphs_list: list of dicts with glyph metadata, sorted by codepoint
        y_advance: line height in pixels
    """
    face = freetype.Face(font_path)
    face.set_char_size(pt_size * 64, 0, ADAFRUIT_DPI, ADAFRUIT_DPI)

    # Compute yAdvance from font metrics
    y_advance = face.size.height >> 6

    bitmap_data = bytearray()
    glyphs = []
    bit_pos = 0  # current bit position in the packed output

    for cp in codepoints:
        # Load glyph
        char_index = face.get_char_index(cp)
        if char_index == 0 and cp != 0:
            # Glyph not available in this font — skip
            continue

        face.load_char(chr(cp),
                       freetype.FT_LOAD_RENDER | freetype.FT_LOAD_TARGET_MONO)
        ft_bitmap = face.glyph.bitmap
        ft_width = ft_bitmap.width
        ft_height = ft_bitmap.rows
        ft_pitch = ft_bitmap.pitch
        ft_buffer = ft_bitmap.buffer

        x_advance = face.glyph.advance.x >> 6
        x_offset = face.glyph.bitmap_left
        y_offset = -face.glyph.bitmap_top  # negate for GFX convention

        # Record bitmap offset (in bytes, since we byte-align each glyph)
        byte_offset = len(bitmap_data)

        # Pack bitmap bits contiguously (matching Adafruit format)
        glyph_bits = []
        for row in range(ft_height):
            for col in range(ft_width):
                byte_idx = row * ft_pitch + col // 8
                bit_mask = 0x80 >> (col % 8)
                if byte_idx < len(ft_buffer) and (ft_buffer[byte_idx] & bit_mask):
                    glyph_bits.append(1)
                else:
                    glyph_bits.append(0)

        # Pack bits into bytes
        packed = bytearray()
        for i in range(0, len(glyph_bits), 8):
            byte_val = 0
            for j in range(8):
                if i + j < len(glyph_bits):
                    byte_val |= (glyph_bits[i + j] << (7 - j))
            packed.append(byte_val)

        # Empty glyph (e.g. space) — ensure at least one byte
        if len(packed) == 0:
            packed = bytearray([0x00])
            ft_width = 1 if ft_width == 0 else ft_width
            ft_height = 1 if ft_height == 0 else ft_height

        bitmap_data.extend(packed)

        glyphs.append({
            'codepoint': cp,
            'bitmap_offset': byte_offset,
            'width': ft_width,
            'height': ft_height,
            'x_advance': x_advance,
            'x_offset': x_offset,
            'y_offset': y_offset,
        })

    return bytes(bitmap_data), glyphs, y_advance


# ---------------------------------------------------------------------------
# C source generation
# ---------------------------------------------------------------------------

def format_bitmap_array(name, data):
    """Format bitmap data as a C uint8_t array."""
    lines = []
    lines.append(f"static const uint8_t {name}[] = {{")
    for i in range(0, len(data), 12):
        chunk = data[i:i+12]
        hex_vals = ", ".join(f"0x{b:02X}" for b in chunk)
        lines.append(f"    {hex_vals},")
    lines.append("};")
    return "\n".join(lines)


def format_glyph_array(name, glyphs):
    """Format glyph metadata as a C UniGlyph array."""
    lines = []
    lines.append(f"static const UniGlyph {name}[] = {{")
    lines.append("    // codepoint, bmpOff,  W,  H, xAdv, xOff, yOff")
    for g in glyphs:
        cp = g['codepoint']
        # Generate a comment with the character or U+XXXX
        if 0x20 < cp < 0x7F:
            ch_comment = f"'{chr(cp)}'"
        elif cp == 0x20:
            ch_comment = "SPACE"
        else:
            ch_comment = f"U+{cp:04X}"
        lines.append(
            f"    {{ 0x{cp:04X}, {g['bitmap_offset']:5d}, "
            f"{g['width']:2d}, {g['height']:2d}, "
            f"{g['x_advance']:3d}, {g['x_offset']:3d}, {g['y_offset']:4d} }}, "
            f"// {ch_comment}"
        )
    lines.append("};")
    return "\n".join(lines)


def generate_c_source(font_path, sizes, codepoints, output_path):
    """Generate the complete C source file with all font sizes."""

    # Map point sizes to array indices matching fontSize_t enum
    size_list = sorted(sizes)

    all_fonts = []
    for pt in size_list:
        bitmap_data, glyphs, y_advance = render_font_size(
            font_path, pt, codepoints
        )
        all_fonts.append({
            'pt': pt,
            'bitmap_data': bitmap_data,
            'glyphs': glyphs,
            'y_advance': y_advance,
        })
        total_bytes = len(bitmap_data) + len(glyphs) * 9  # approx
        print(f"  {pt}pt: {len(glyphs)} glyphs, "
              f"~{total_bytes} bytes", file=sys.stderr)

    # Build the C source
    out = []
    timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    out.append(f"/* Auto-generated by scripts/gen_font.py on {timestamp}")
    out.append(f" * Font: {os.path.basename(font_path)}")
    out.append(f" * Sizes: {', '.join(str(s) for s in size_list)}pt")
    out.append(f" * Codepoints: {len(codepoints)}")
    out.append(" * DO NOT EDIT")
    out.append(" */")
    out.append("")
    out.append('#include "fonts/adafruit/gfxfont.h"')
    out.append("")

    for font_info in all_fonts:
        pt = font_info['pt']
        safe_name = f"text_font_{pt}pt"

        out.append(f"/* === {pt}pt === */")
        out.append("")
        out.append(format_bitmap_array(f"{safe_name}_bitmaps", font_info['bitmap_data']))
        out.append("")
        out.append(format_glyph_array(f"{safe_name}_glyphs", font_info['glyphs']))
        out.append("")

    # Generate the text_fonts[] array (indexed by fontSize_t)
    out.append("/* Font table indexed by fontSize_t enum */")
    out.append(f"const UniFont text_fonts[{len(all_fonts)}] = {{")
    for font_info in all_fonts:
        pt = font_info['pt']
        safe_name = f"text_font_{pt}pt"
        n = len(font_info['glyphs'])
        ya = font_info['y_advance']
        out.append(
            f"    {{ (const uint8_t *){safe_name}_bitmaps, "
            f"{safe_name}_glyphs, {n}, {ya} }},  // {pt}pt"
        )
    out.append("};")
    out.append("")

    content = "\n".join(out) + "\n"

    if output_path == "/dev/stdout" or output_path == "-":
        sys.stdout.write(content)
    else:
        os.makedirs(os.path.dirname(output_path) or '.', exist_ok=True)
        with open(output_path, 'w', encoding='utf-8') as f:
            f.write(content)
        print(f"Generated {output_path} ({len(content)} bytes)", file=sys.stderr)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Generate bitmap font C source for OpenRTX"
    )
    parser.add_argument(
        "--font", required=True,
        help="Path to TrueType font file (.ttf)"
    )
    parser.add_argument(
        "--struct",
        help="Path to ui_strings.h (to determine needed codepoints from locale)"
    )
    parser.add_argument(
        "--po",
        help="Path to .po file for target locale (optional)"
    )
    parser.add_argument(
        "--sizes", default="5,6,8,9,10,12,16",
        help="Comma-separated list of point sizes (default: 5,6,8,9,10,12,16)"
    )
    parser.add_argument(
        "--output", required=True,
        help="Output C source file path"
    )

    args = parser.parse_args()

    # Validate font file
    if not os.path.isfile(args.font):
        print(f"ERROR: Font file not found: {args.font}", file=sys.stderr)
        sys.exit(1)

    # Parse sizes
    try:
        sizes = [int(s.strip()) for s in args.sizes.split(",")]
    except ValueError:
        print(f"ERROR: Invalid --sizes format: {args.sizes}", file=sys.stderr)
        sys.exit(1)

    # Collect codepoints
    if args.struct:
        codepoints = collect_locale_codepoints(args.struct, args.po)
        print(f"Collected {len(codepoints)} unique codepoints from locale",
              file=sys.stderr)
    else:
        # Default: ASCII printable only
        codepoints = list(range(0x20, 0x7F))
        print(f"Using ASCII printable range ({len(codepoints)} codepoints)",
              file=sys.stderr)

    print(f"Generating {len(sizes)} font sizes from {os.path.basename(args.font)}...",
          file=sys.stderr)

    generate_c_source(args.font, sizes, codepoints, args.output)


if __name__ == "__main__":
    main()
