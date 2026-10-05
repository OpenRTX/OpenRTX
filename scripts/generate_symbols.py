#!/usr/bin/env python3
# /***************************************************************************
#  *   Copyright (C)  2023 by Ryan Turner K0RET                              *
#  *                                                                         *
#  *   This program is free software; you can redistribute it and/or modify  *
#  *   it under the terms of the GNU General Public License as published by  *
#  *   the Free Software Foundation; either version 3 of the License, or     *
#  *   (at your option) any later version.                                   *
#  *                                                                         *
#  *   This program is distributed in the hope that it will be useful,       *
#  *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#  *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#  *   GNU General Public License for more details.                          *
#  *                                                                         *
#  *   You should have received a copy of the GNU General Public License     *
#  *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
#  ***************************************************************************/
#
# Usage:
#
# 1. Find your source svg; https://pictogrammers.com/library/mdi/ is a good resource
# 2. Add your source svg to openrtx/include/fonts/symbols/sources
# 3. Execute this script; the corresponding `.h` files in the openrtx/include/fonts/symbols directory will regenerate
# 4. Refer to the `symbols/symbols.h` file to find the enum that corresponds with your new symbol
# 5. Utilize the `gfx_printSymbol()` method with your enum to access your symbol

import cairosvg
import io
from PIL import Image
import numpy
import os

absolute_path = os.path.dirname(__file__)

symbol_source_dir = "../openrtx/include/fonts/symbols/sources/"

debug = False
sizes = [5, 6, 8]
symbols = sorted([f for f in os.listdir(os.path.join(absolute_path, symbol_source_dir)) if "svg" in f])
print(symbols)
###################################################################


def svgToBytes(filename, height):
    # Make memory buffer
    mem = io.BytesIO()
    # Convert SVG to PNG in memory
    cairosvg.svg2png(
        url=filename, write_to=mem, parent_height=height, parent_width=height
    )
    # Convert PNG to Numpy array
    img = Image.open(mem)
    # Split the channels, get just the alpha since that's what we need
    red, green, blue, alpha = img.split()
    # Convert the alpha to 1 and 0 (black and white)
    alpha = alpha.point(lambda x: 0 if x < 128 else 255, "1")
    if debug:
        alpha.show()
    return numpy.array(alpha)

# REUSE-IgnoreStart
first_ascii = 32  # Start at ascii d32 which is " " per https://www.rapidtables.com/code/text/ascii-table.html
dont_edit_banner  = "/*\n"
dont_edit_banner += " * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors\n"
dont_edit_banner += " * \n"
dont_edit_banner += " * SPDX-License-Identifier: GPL-3.0-or-later\n"
dont_edit_banner += " */\n"
dont_edit_banner += "\n"
dont_edit_banner += "// This is a generated file, please do not edit it! Use generate_symbols.py\n"
# REUSE-IgnoreEnd

class FontDefinition:
    scalar = 1.6  # this number is used to roughly convert from pt to px
    yadvance_scalar = (
        7 / 3
    )  # this number is applied to the width to scale up the yadvance; this is naively linear

    def __init__(self, name, size):
        # Prepopulate with a space character
        self.bitmaps = [[0x00]]
        self.glyphNames = ["space"]
        self.initial_offset = 1

        self.size = size
        self.name = name
        self.height = round(size * self.scalar)
        self.width = round(size * self.scalar)
        self.yadvance = round(size * self.scalar * self.yadvance_scalar)
        self.xadvance = round(size * self.scalar * 2 / 3)
        self.dY = round(-1 * self.height / 1.2)

    def addGlyph(self, filename, glyphName):
        res = svgToBytes(filename, self.height)
        packed = numpy.packbits(res)
        charBytes = packed.tobytes()

        self.bitmaps.append(charBytes)

        self.glyphNames.append(glyphName)

    def __str__(self):
        out = dont_edit_banner
        out += "static const uint8_t {}{}pt7bBitmaps[] PROGMEM = {{\n".format(
            self.name, self.size
        )
        for i, bitmap in enumerate(self.bitmaps):
            out += (
                (", ".join("0x{:02x}".format(x) for x in bitmap))
                + ", //"
                + self.glyphNames[i]
                + "\n"
            )
        out += "};\n"
        out += "\n"
        out += (
            "static const GFXglyph {}{}pt7bGlyphs[] PROGMEM = {{".format(
                self.name, self.size
            )
            + "\n"
        )
        out += "  //Index,  W, H,xAdv,dX, dY\n"
        offset = 0
        for glyphNumber, glyphName in enumerate(self.glyphNames):
            out += '{{ {}, {}, {}, {}, 0, {}}},   // "{}" {}\n'.format(
                offset,
                self.width if glyphNumber > 0 else 1,
                self.height if glyphNumber > 0 else 1,
                self.xadvance,
                self.dY,
                chr(first_ascii + glyphNumber),
                glyphName,
            )
            offset += len(self.bitmaps[glyphNumber])
        out += "};\n"
        out += (
            "static const GFXfont {}{}pt7b PROGMEM = {{".format(self.name, self.size)
            + "\n"
        )
        out += "(uint8_t  *){}{}pt7bBitmaps,".format(self.name, self.size) + "\n"
        out += "(GFXglyph *){}{}pt7bGlyphs,".format(self.name, self.size) + "\n"
        out += "//ASCII start, ASCII stop,y Advance \n"
        out += (
            "  {}, {}, {} }};".format(
                first_ascii,
                first_ascii + len(self.glyphNames) - 1,
                round(self.yadvance),
            )
            + "\n"
        )
        return out


###################################################################

# Generate the font definitions
for size in sizes:
    font = FontDefinition("Symbols", size)
    for symbol in symbols:
        font.addGlyph(os.path.join(absolute_path, symbol_source_dir + symbol), symbol)
    with open(os.path.join(absolute_path, "../openrtx/include/fonts/symbols/Symbols{}pt7b.h".format(size)), "w") as f:
        f.write(str(font))

# Generate the enum for convenience
with open(os.path.join(absolute_path, "../openrtx/include/fonts/symbols/symbols.h".format(size)), "w") as f:
    out = dont_edit_banner
    out += "typedef enum {\n"
    symbols.insert(0, "SPACE")
    for symbolNumber, symbolName in enumerate(symbols):
        out += "    SYMBOL_{} = {},\n".format(symbolName.split(".")[0].replace("-", "_").upper(), symbolNumber + first_ascii)
    out += "} symbol_t;\n"
    f.write(out)
