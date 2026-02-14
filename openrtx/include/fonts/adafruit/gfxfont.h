// Font structures for newer Adafruit_GFX (1.1 and later).
// Example fonts are included in 'Fonts' directory.
// To use a font in your Arduino sketch, #include the corresponding .h
// file and pass address of GFXfont struct to setFont().  Pass NULL to
// revert to 'classic' fixed-space bitmap font.

#ifndef _GFXFONT_H_
#define _GFXFONT_H_

#include <stdint.h>

/// Font data stored PER GLYPH
typedef struct {
  uint16_t bitmapOffset; ///< Pointer into GFXfont->bitmap
  uint8_t width;         ///< Bitmap dimensions in pixels
  uint8_t height;        ///< Bitmap dimensions in pixels
  uint8_t xAdvance;      ///< Distance to advance cursor (x axis)
  int8_t xOffset;        ///< X dist from cursor pos to UL corner
  int8_t yOffset;        ///< Y dist from cursor pos to UL corner
} GFXglyph;

/// Data stored for FONT AS A WHOLE
typedef struct {
  uint8_t *bitmap;  ///< Glyph bitmaps, concatenated
  GFXglyph *glyph;  ///< Glyph array
  uint16_t first;   ///< ASCII extents (first char)
  uint16_t last;    ///< ASCII extents (last char)
  uint8_t yAdvance; ///< Newline distance (y axis)
} GFXfont;

/// Unicode glyph entry — glyphs sorted by codepoint for binary search
typedef struct {
  uint16_t codepoint;    ///< Unicode codepoint (BMP)
  uint16_t bitmapOffset; ///< Pointer into UniFont->bitmap
  uint8_t  width;        ///< Bitmap dimensions in pixels
  uint8_t  height;       ///< Bitmap dimensions in pixels
  uint8_t  xAdvance;     ///< Distance to advance cursor (x axis)
  int8_t   xOffset;      ///< X dist from cursor pos to UL corner
  int8_t   yOffset;      ///< Y dist from cursor pos to UL corner
} UniGlyph;

/// Unicode font — supports arbitrary codepoint subsets via sorted glyph table
typedef struct {
  const uint8_t  *bitmap;    ///< Glyph bitmaps, concatenated
  const UniGlyph *glyphs;    ///< Glyph array, sorted by codepoint
  uint16_t        numGlyphs; ///< Number of glyphs in array
  uint8_t         yAdvance;  ///< Newline distance (y axis)
} UniFont;

/// Text font table, generated at build time by gen_font.py
/// Indexed by fontSize_t enum (FONT_SIZE_5PT .. FONT_SIZE_16PT)
extern const UniFont text_fonts[];

// Define PROGMEM macro, since it is not used in our platforms
#define PROGMEM

#endif // _GFXFONT_H_
