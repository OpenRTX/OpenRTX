/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN,                     *
 *                                Silvano Seva IU2KWO                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

/**
 * This source file provides an  implementation for the graphics.h interface
 * It is suitable for both color, grayscale and B/W display
 */

 /*
 *  Please Note :- The XY position starts at the top left corner.
 *                 Characters are displayed from their bottom left corner.
 *                 As such the XY position for the lines is from this position.
 */

#include <interfaces/display.h>
#include <hwconfig.h>
#include <graphics.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <stdio.h>
#include <math.h>

#include <gfxfont.h>
#include <TomThumb.h>
#include <FreeSans6pt7b.h>
#include <FreeSans8pt7b.h>
#include <FreeSans9pt7b.h>
#include <FreeSans10pt7b.h>
#include <FreeSans12pt7b.h>
#include <FreeSans16pt7b.h>
#include <FreeSans18pt7b.h>
#include <FreeSans24pt7b.h>
#include <UbuntuRegular6pt7b.h>
#include <UbuntuRegular8pt7b.h>
#include <UbuntuRegular9pt7b.h>
#include <UbuntuRegular10pt7b.h>
#include <UbuntuRegular12pt7b.h>
#include <UbuntuRegular16pt7b.h>
#include <UbuntuRegular18pt7b.h>
#include <UbuntuRegular24pt7b.h>

#include <Symbols5pt7b.h>
#include <Symbols6pt7b.h>
#include <Symbols8pt7b.h>

// Variable swap macro
#define DEG_RAD  0.017453292519943295769236907684886
#define SIN(x) sinf((x) * DEG_RAD)
#define COS(x) cosf((x) * DEG_RAD)

/**
 * Fonts, ordered by the FontSize_t enum.
 */
static const GFXfont fonts[] = { TomThumb            , //  5pt
    #if defined FONT_FREE_SANS
                                 FreeSans6pt7b       , //  6pt
                                 FreeSans8pt7b       , //  8pt
                                 FreeSans9pt7b       , //  9pt
                                 FreeSans10pt7b      , // 10pt
                                 FreeSans12pt7b      , // 12pt
                                 FreeSans16pt7b      , // 16pt
                                 FreeSans18pt7b      , // 16pt
                                 FreeSans24pt7b      , // 24pt
    #elif defined FONT_UBUNTU_REGULAR
                                 UbuntuRegular6pt7b  , //  6pt
                                 UbuntuRegular8pt7b  , //  8pt
                                 UbuntuRegular9pt7b  , //  9pt
                                 UbuntuRegular10pt7b , // 10pt
                                 UbuntuRegular12pt7b , // 12pt
                                 UbuntuRegular16pt7b , // 16pt
                                 UbuntuRegular18pt7b , // 16pt
                                 UbuntuRegular24pt7b , // 24pt
    #else
    #error Unsupported font family!
    #endif
                                 Symbols5pt7b        , //  5pt
                                 Symbols6pt7b        , //  6pt
                                 Symbols8pt7b          //  8pt
                               };

#ifdef PIX_FMT_RGB565

/* This specialization is meant for an RGB565 little endian pixel format.
 * Thus, to accomodate for the endianness, the fields in struct rgb565_t have to
 * be written in reversed order.
 *
 * For more details about endianness and bitfield structs see the following web
 * page: http://mjfrazer.org/mjfrazer/bitfields/
 */

#define PIXEL_T rgb565_t

typedef struct
{
    uint16_t b : 5 ;
    uint16_t g : 6 ;
    uint16_t r : 5 ;
}
rgb565_t;

static rgb565_t _true2highColor( Color_st* true_color )
{
    rgb565_t high_color;
    high_color.r = true_color->red   >> 3 ;
    high_color.g = true_color->green >> 2 ;
    high_color.b = true_color->blue  >> 3 ;

    return high_color;
}

#elif defined PIX_FMT_BW

/**
 * This specialization is meant for black and white pixel format.
 * It is suitable for monochromatic displays with 1 bit per pixel,
 * it will have RGB and grayscale counterparts
 */

#define PIXEL_T uint8_t

typedef enum
{
    WHITE = 0 ,
    BLACK = 1
}
bw_t;

static bw_t _color2bw( Color_st* true_color )
{
    colorType = BLACK ;
    if( ( true_color.red   == 0 ) &&
        ( true_color.green == 0 ) &&
        ( true_color.blue  == 0 )    )
    {
        colorType = WHITE ;
    }
    return colorType ;
}

#else
#error Please define a pixel format type into hwconfig.h or meson.build
#endif

static bool     initialized = 0 ;
static PIXEL_T* buf ;
static uint16_t fbSize ;
static char     text[ 32 ] ;

void gfx_init( void )
{
    display_init();
    buf = (PIXEL_T*)( display_getFrameBuffer() );
    initialized = 1;

// Calculate framebuffer size
#ifdef PIX_FMT_RGB565
    fbSize = SCREEN_HEIGHT * SCREEN_WIDTH * sizeof( PIXEL_T ) ;
#elif defined PIX_FMT_BW
    fbSize = ( SCREEN_HEIGHT * SCREEN_WIDTH ) / 8 ;
    /* Compensate for eventual truncation error in division */
    if( ( fbSize * 8 ) < ( SCREEN_HEIGHT * SCREEN_WIDTH ) )
    {
        fbSize += 1 ;
    }
    fbSize *= sizeof( uint8_t );
#endif
    // Clear text buffer
    memset( text , 0x00 , 32 );
}

void gfx_terminate( void )
{
    display_terminate();
    initialized = 0;
}

void gfx_renderRows( uint8_t startRow , uint8_t endRow )
{
    display_renderRows( startRow , endRow );
}

void gfx_render()
{
    display_render();
}

bool gfx_renderingInProgress()
{
    return display_renderingInProgress();
}

void gfx_clearRows( uint8_t startRow , uint8_t endRow )
{
    uint16_t startPos ;
    uint16_t length ;

    if( initialized )
    {
        if( endRow > startRow )
        {
            startPos = startRow * SCREEN_WIDTH ;
            length   = ( endRow - startRow ) * ( SCREEN_WIDTH * sizeof( PIXEL_T ) ) ;
            // Set the specified rows to 0x00 = make the screen black
            memset( &buf[ startPos ] , 0x00 , length );
        }
    }
}

void gfx_clearScreen( void )
{
    if( initialized )
    {
        // Set the whole framebuffer to 0x00 = make the screen black
        memset( buf , 0x00 , fbSize );
    }
}

void gfx_fillScreen( Color_st* color )
{
    if( initialized )
    {
        for( int16_t y = 0 ; y < SCREEN_HEIGHT ; y++ )
        {
            for( int16_t x = 0 ; x < SCREEN_WIDTH ; x++ )
            {
                Pos_st pos = { x , y , 0 , 0 };
                gfx_setPixel( &pos , color );
            }
        }
    }
}

inline void gfx_clearPixel( Pos_st* pos )
{
    if( ( pos->x < SCREEN_WIDTH ) && ( pos->y < SCREEN_HEIGHT ) &&
        ( pos->x >= 0           ) && ( pos->y >= 0            )    )
    {
        Color_st color = { 0 , 0 , 0 , 0 };
#ifdef PIX_FMT_RGB565
        buf[ ( pos->y * SCREEN_WIDTH ) + pos->x ] = _true2highColor( &color );
#elif defined PIX_FMT_BW
        // Ignore more than half transparent pixels
        uint16_t cell         = ( ( pos->y * SCREEN_WIDTH ) + pos->x ) / 8 ;
        uint16_t elem         = ( ( pos->y * SCREEN_WIDTH ) + pos->x ) % 8 ;
                 buf[ cell ] &= ~( 1 << elem );
                 buf[ cell ] |= ( _color2bw( &color ) << elem );
#endif
    }
}

inline void gfx_setPixel( Pos_st* pos , Color_st* color )
{
    if( ( pos->x < SCREEN_WIDTH ) && ( pos->y < SCREEN_HEIGHT ) &&
        ( pos->x >= 0           ) && ( pos->y >= 0            )    )
    {
#ifdef PIX_FMT_RGB565
        // Blend old pixel value and new one
        if( color->alpha < 255 )
        {
            uint8_t  alpha     = color->alpha;
            rgb565_t new_pixel = _true2highColor( color );
            rgb565_t old_pixel = buf[ ( pos->y * SCREEN_WIDTH ) + pos->x ] ;
            rgb565_t pixel ;
                     pixel.r   = ( ( ( 255 - alpha ) * old_pixel.r ) + ( alpha * new_pixel.r ) / 255 ) ;
                     pixel.g   = ( ( ( 255 - alpha ) * old_pixel.g ) + ( alpha * new_pixel.g ) / 255 ) ;
                     pixel.b   = ( ( ( 255 - alpha ) * old_pixel.b ) + ( alpha * new_pixel.b ) / 255 ) ;
                     buf[ ( pos->y * SCREEN_WIDTH ) + pos->x ] = pixel ;
        }
        else
        {
            buf[ ( pos->y * SCREEN_WIDTH ) + pos->x ] = _true2highColor( color );
        }
#elif defined PIX_FMT_BW
        // Ignore more than half transparent pixels
        if( color->alpha >= 128 )
        {
            uint16_t cell  = ( ( pos->y * SCREEN_WIDTH ) + pos->x ) / 8 ;
            uint16_t elem  = ( ( pos->y * SCREEN_WIDTH ) + pos->x ) % 8 ;
            buf[ cell ]   &= ~( 1 << elem ) ;
            buf[ cell ]   |= ( _color2bw( color ) << elem );
        }
#endif
    }
}

enum
{
    NUMBER_OF_STEPS       =   256 ,
    RESOLUTION_MULTIPLIER = 32768
};

Pos_st gfx_drawLine( Pos_st* startPos , Color_st* color )
{
    Pos_st   pos       = *startPos ;
    Pos_st   prevPos   = pos ;
    int32_t  startPosX = (int32_t)pos.x ;
    int32_t  startPosY = (int32_t)pos.y ;
    int32_t  startPosW = (int32_t)pos.w ;
    int32_t  startPosH = (int32_t)pos.h ;
    int32_t  accumX    = 0 ;
    int32_t  stepX     = ( startPosW * RESOLUTION_MULTIPLIER ) / NUMBER_OF_STEPS ;
    int32_t  accumY    = 0 ;
    int32_t  stepY     = ( startPosH * RESOLUTION_MULTIPLIER ) / NUMBER_OF_STEPS ;
    uint16_t index ;

    // force the displaying of the first pixel
    prevPos.x = ~pos.x ;
    // draw the line
    for( index = 0 ; index < NUMBER_OF_STEPS ; index++ )
    {
        pos.x  = (Pos_t)( startPosX + ( accumX / RESOLUTION_MULTIPLIER ) ) ;
        pos.y  = (Pos_t)( startPosY + ( accumY / RESOLUTION_MULTIPLIER ) ) ;
        if( ( pos.x != prevPos.x ) || ( pos.y != prevPos.y ) )
        {
            gfx_setPixel( &pos , color );
            prevPos = pos ;
        }
        accumX += stepX ;
        accumY += stepY ;
    }

    pos    = *startPos ;
    pos.x += pos.w ;
    pos.y += pos.h ;

    return pos ;

}

void gfx_clearRectangle( Pos_st* startPos )
{
    if( initialized )
    {
        if( ( startPos->w > 0 ) && ( startPos->h > 0 ) )
        {
            uint16_t x_max = startPos->x + startPos->w ;
            uint16_t y_max = startPos->y + startPos->h ;
            Pos_st     pos ;

            if( x_max > ( SCREEN_WIDTH - 1  ) ) x_max = SCREEN_WIDTH  - 1 ;
            if( y_max > ( SCREEN_HEIGHT - 1 ) ) y_max = SCREEN_HEIGHT - 1 ;

            for( pos.y = startPos->y ; pos.y <= y_max ; pos.y++ )
            {
                for( pos.x = startPos->x ; pos.x <= x_max ; pos.x++ )
                {
                    gfx_clearPixel( &pos );
                }
            }
        }
    }
}

void gfx_drawRect( Pos_st* startPos , Color_st* color , bool fillRect )
{
    Pos_st pos ;
    Pos_t  index ;

    if( initialized )
    {
        if( !fillRect )
        {
            // top
            pos.x = startPos->x ;
            pos.y = startPos->y ;
            pos.w = startPos->w ;
            pos.h = 1 ;
            gfx_drawLine( &pos , color );
            // lhs
            pos.x = startPos->x ;
            pos.y = startPos->y ;
            pos.w = 1 ;
            pos.h = startPos->h ;
            gfx_drawLine( &pos , color );
            // rhs
            pos.x = ( startPos->x + startPos->w ) - 1 ;
            pos.y = startPos->y ;
            pos.w = 1 ;
            pos.h = startPos->h ;
            gfx_drawLine( &pos , color );
            // bottom
            pos.x = startPos->x ;
            pos.y = ( startPos->y + startPos->h ) - 1 ;
            pos.w = startPos->w ;
            pos.h = 1 ;
            gfx_drawLine( &pos , color );
        }
        else
        {
            // top
            pos.x = startPos->x ;
            pos.y = startPos->y ;
            pos.w = startPos->w ;
            pos.h = 1 ;

            for( index = 0 ; index < startPos->h ; index++ )
            {
                gfx_drawLine( &pos , color );
                pos.y++ ;
            }

        }
    }

}

void gfx_drawCircle( Pos_st* startPos , Color_st* color )
{
    int16_t f     = 1 - startPos->h ;
    int16_t ddF_x = 1 ;
    int16_t ddF_y = -2 * startPos->h ;
    int16_t x     = 0 ;
    int16_t y     = startPos->h ;

    Pos_st pos  = *startPos ;
    pos.y      += startPos->h ;
    gfx_setPixel( &pos , color );
    pos.y      -= 2 * startPos->h;
    gfx_setPixel( &pos , color );
    pos.y      += startPos->h ;
    pos.x      += startPos->h;
    gfx_setPixel( &pos , color );
    pos.x      -= 2 * startPos->h ;
    gfx_setPixel( &pos , color );

    while( x < y )
    {
        if( f >= 0 )
        {
            y-- ;
            ddF_y += 2 ;
            f     += ddF_y ;
        }

        x++ ;
        ddF_x += 2 ;
        f     += ddF_x ;

        pos.x = startPos->x + x ;
        pos.y = startPos->y + y ;
        gfx_setPixel( &pos , color ) ;
        pos.x = startPos->x - x ;
        pos.y = startPos->y + y ;
        gfx_setPixel( &pos , color );
        pos.x = startPos->x + x ;
        pos.y = startPos->y - y ;
        gfx_setPixel( &pos , color );
        pos.x = startPos->x - x ;
        pos.y = startPos->y - y ;
        gfx_setPixel( &pos , color );
        pos.x = startPos->x + y ;
        pos.y = startPos->y + x ;
        gfx_setPixel( &pos , color );
        pos.x = startPos->x - y ;
        pos.y = startPos->y + x ;
        gfx_setPixel( &pos , color );
        pos.x = startPos->x + y ;
        pos.y = startPos->y - x ;
        gfx_setPixel( &pos , color );
        pos.x = startPos->x - y ;
        pos.y = startPos->y - x ;
        gfx_setPixel( &pos , color );
    }
}

void gfx_drawHLine(int16_t y, uint16_t height, Color_st* color)
{
    Pos_st startPos = { 0 , y , SCREEN_WIDTH , height };
    gfx_drawRect( &startPos , color , true );
}

void gfx_drawVLine(int16_t x, uint16_t width, Color_st* color)
{
    Pos_st startPos = { x , 0 , width , SCREEN_HEIGHT };
    gfx_drawRect( &startPos , color , true );
}

/**
 * Compute the pixel size of the first text line
 * @param f: font used as the source of glyphs
 * @param text: the input text
 * @param length: the length of the input text, used for boundary checking
 */
static inline uint16_t get_line_size( uint16_t startX , GFXfont font , const char* text , uint16_t length )
{
    uint16_t line_size = 0 ;

    for( unsigned index = 0 ; ( index < length ) && ( text[ index ] != '\n' ) && ( text[ index ] != '\r' ) ; index++ )
    {
        GFXglyph glyph = font.glyph[text[ index ] - font.first ];

        if( ( startX + line_size + glyph.xAdvance ) < SCREEN_WIDTH )
        {
            line_size += glyph.xAdvance ;
        }
        else
        {
            break ;
        }
    }

    return line_size ;

}

/**
 * Compute the display startPos x coordinate of a new line of given pixel size
 * @param alinment: enum representing the text alignment
 * @param lineLen: the size of the current text line in pixels
 */
static inline uint16_t get_display_x( uint16_t startX , uint16_t lineLen , Align_t alignment )
{
    uint16_t displayX ;

    switch( alignment )
    {
        case GFX_ALIGN_LEFT :
        {
            displayX = startX ;
            break ;
        }
        case GFX_ALIGN_CENTER :
        {
            displayX = startX + ( ( SCREEN_WIDTH - ( startX + lineLen ) ) / 2 ) ;
            break ;
        }
        case GFX_ALIGN_RIGHT:
        {
            displayX = SCREEN_WIDTH - lineLen ;
            break ;
        }
        default :
        {
            displayX = startX ;
            break ;
        }
    }

    return displayX ;
}

uint8_t gfx_getFontHeight( FontSize_t size )
{
    GFXfont  font  = fonts[ size ] ;
    GFXglyph glyph = font.glyph[ '|' - font.first ] ;
    return   glyph.height ;
}

Pos_st gfx_printBuffer( Pos_st*   startPos , FontSize_t  fontSize , Align_t alignment ,
                        Color_st* color    , const char* buf                            )
{
    GFXfont  font         = fonts[ fontSize ];
    size_t   strLen       = strlen( buf );
    Pos_st   textStartPos = *startPos ;
    // Compute fontSize of the first row in pixels
    Pos_st   textPos ;

    textStartPos.w = get_line_size( startPos->x , font , buf , strLen );
    textStartPos.x = get_display_x( startPos->x , textStartPos.w , alignment );
    textStartPos.h = font.yAdvance ;

    textPos        = textStartPos ;

    /* For each char in the string */
    for( unsigned index = 0 ; index < strLen ; index++ )
    {
        char     ch     = buf[ index ] ;
        GFXglyph glyph  = font.glyph[ ch - font.first ] ;
        uint8_t* bitmap = font.bitmap ;
        uint16_t bo     = glyph.bitmapOffset ;
        uint8_t  w      = glyph.width ;
        uint8_t  h      = glyph.height ;
        int8_t   xo     = glyph.xOffset ;
        int8_t   yo     = glyph.yOffset ;
        uint8_t  xx ;
        uint8_t  yy ;
        uint8_t  bits   = 0 ;
        uint8_t  bit    = 0 ;

        // Handle newline and carriage return
        if( ch == '\n' )
        {
          if( alignment != GFX_ALIGN_CENTER )
          {
            textPos.x = startPos->x ;
          }
          else
          {
            textPos.w = get_line_size( startPos->x , font , &buf[ index + 1 ] , strLen - ( index + 1 ) );
            textPos.x = get_display_x( startPos->x , textPos.w , alignment );
            if( textPos.x < textStartPos.x )
            {
                textStartPos.x = textPos.x ;
                textStartPos.w = textPos.w ;
            }
          }
          textPos.y      += font.yAdvance ;
          textStartPos.h += font.yAdvance ;
          continue ;
        }
        else
        {
            if( ch == '\r' )
            {
              textPos.x = startPos->x ;
              continue ;
            }
        }

        // Handle wrap around
        if( ( textPos.x + glyph.xAdvance ) > SCREEN_WIDTH )
        {
            // Compute fontSize of the first row in pixels
            textPos.w       = get_line_size( startPos->x , font , buf , strLen );
            textPos.x       = get_display_x( startPos->x , textPos.w , alignment );
            textPos.y      += font.yAdvance ;
            textStartPos.h += font.yAdvance ;
            if( textPos.x < textStartPos.x )
            {
                textStartPos.x = textPos.x ;
                textStartPos.w = textPos.w ;
            }
        }

        // Draw bitmap
        for( yy = 0 ; yy < h ; yy++ )
        {
            for( xx = 0 ; xx < w ; xx++ )
            {
                if( !( bit++ & 7 ) )
                {
                    bits = bitmap[ bo++ ] ;
                }

                if( bits & 0x80 )
                {
                    if( ( ( textPos.y + yo + yy ) < SCREEN_HEIGHT ) &&
                        ( ( textPos.x + xo + xx ) < SCREEN_WIDTH  ) &&
                        ( ( textPos.y + yo + yy ) > 0             ) &&
                        ( ( textPos.x + xo + xx ) > 0             )    )
                    {
                        Pos_st pos;
                        pos.x = textPos.x + xo + xx ;
                        pos.y = textPos.y + yo + yy ;
                        gfx_setPixel( &pos , color );
                    }
                }

                bits <<= 1 ;
            }
        }

        textPos.x += glyph.xAdvance ;

    }

    if( textStartPos.y < textStartPos.h )
    {
        textStartPos.y  = 0 ;
    }
    else
    {
        textStartPos.y -= textStartPos.h ;
    }

    return textStartPos ;

}

Pos_st gfx_print( Pos_st*   startPos , FontSize_t  size , Align_t alignment ,
                  Color_st* color , const char* fmt  , ...                 )
{
    // Get format string and arguments from var char
    va_list ap ;
    va_start( ap , fmt );
    vsnprintf( text , sizeof( text ) - 1 , fmt , ap );
    va_end( ap );

    return gfx_printBuffer( startPos , size , alignment , color , text );
}

Pos_st gfx_printLine( uint8_t   cur    , uint8_t    tot  , int16_t startY    , int16_t endY ,
                      int16_t   startX , FontSize_t size , Align_t alignment ,
                      Color_st* color  , const char* fmt, ... )
{
    // Get format string and arguments from var char
    va_list ap ;
    va_start( ap , fmt );
    vsnprintf( text , sizeof( text ) - 1 , fmt , ap );
    va_end( ap );

    // Estimate font height by reading the gliph | height
    uint8_t fontH = gfx_getFontHeight( size );

    // If endY is 0 set it to default value = SCREEN_HEIGHT
    if( endY == 0 )
    {
       endY = SCREEN_HEIGHT ;
    }
    // Calculate print coordinates
    int16_t height  = endY - startY ;
    // to print 2 lines we need 3 padding spaces
    int16_t gap     = ( height - ( fontH * tot ) ) / ( tot + 1 );
    // We need a gap and a line height for each line
    int16_t printY  = startY + ( cur * ( gap + fontH ) ) ;

    Pos_st startPos = { startX , printY , 0 , 0 };
    return gfx_printBuffer( &startPos , size , alignment , color , text );
}

// Print an error message to the center of the screen, surronded by a red (when possible) box
void gfx_printError( const char* text , FontSize_t size )
{
    // 3 px box padding
    uint16_t box_padding  = 16 ;
    Color_st white        = { 255 , 255 , 255 , 255 } ;
    Color_st red          =   { 255 ,   0 ,   0 , 255 } ;
    Pos_st   startPos     = { 0 , SCREEN_HEIGHT / 2 + 5 , 0 , 0 } ;

    // Print the error message
    Pos_st text_size      = gfx_print( &startPos , size , GFX_ALIGN_CENTER , &white , text );
    text_size.x          += box_padding ;
    text_size.y          += box_padding ;
    Pos_st box_start      = { 0 , 0 , 0 , 0 };
    box_start.x           = ( SCREEN_WIDTH  / 2 ) - ( text_size.x / 2 ) ;
    box_start.y           = ( SCREEN_HEIGHT / 2 ) - ( text_size.y / 2 ) ;
    box_start.w           = text_size.x ;
    box_start.h           = text_size.y ;
    // Draw the error box
    gfx_drawRect( &box_start , &red , false );
}

Pos_st gfx_drawSymbol( Pos_st*   startPos , SymbolSize_t size   , Align_t alignment ,
                       Color_st* color    , symbol_t     symbol )
{
    /*
     * Symbol tables come immediately after fonts in the general font table.
     * But, to prevent errors where symbol size is used instead of font size and
     * vice-versa, their enums are separate. The trickery below is used to put
     * together again the two enums in a single consecutive index.
     *
     * TODO: improve this.
     */
    int  symSize     = size + FONT_SIZE_24PT + 1 ;
    char buffer[ 2 ] = { 0 } ;

    buffer[ 0 ]      = (char)symbol ;
    return gfx_printBuffer( startPos , symSize , alignment , color , buffer );
}

/*
 * Function to draw battery of arbitrary size
 * starting coordinates are relative to the top left point.
 *
 *  ****************       |
 * *                *      |
 * *  *******       *      |
 * *  *******       **     |
 * *  *******       **     | <-- Height (px)
 * *  *******       *      |
 * *                *      |
 *  ****************       |
 *
 * __________________
 *
 * ^
 * |
 *
 * Width (px)
 *
 */
Pos_st gfx_drawBattery( Pos_st* startPos , uint16_t prcntg )
{
    Pos_st   outerPos ;
    Pos_st   innerPos ;
    Pos_st   cornerPos ;

    Color_st white      = { 255 , 255 , 255 , 255 } ;
    Color_st black      = {   0 ,   0 ,   0 , 255 } ;

    // Cap percentage to 100
    uint16_t percentage = ( prcntg > 100 ) ? 100 : prcntg ;

#ifdef PIX_FMT_RGB565
    Color_st green      = {   0 , 255 ,  0 , 255 } ;
    Color_st yellow     = { 250 , 180 , 19 , 255 } ;
    Color_st red        = { 255 ,   0 ,  0 , 255 } ;

    outerPos    = *startPos ;
    outerPos.w -= 2 ; // offset for the button width

    innerPos    = outerPos ;
    innerPos.x += 1 ;
    innerPos.y += 1 ;
    innerPos.w -= 2 ;
    innerPos.h -= 2 ;

    // Select color according to percentage
    Color_st batColor  = yellow;

    if( percentage < 30 )
    {
        batColor = red ;
    }
    else
    {
        if( percentage > 60 )
        {
            batColor = green ;
        }
    }
#elif defined PIX_FMT_BW
    Color_st batColor = white ;
#endif // defined PIX_FMT_BW

    // Draw the battery outline
    gfx_drawRect( &outerPos , &white , false );
    // Draw the battery fill
    gfx_drawRect( &innerPos , &batColor , true );

    // Round corners
    cornerPos.w = 1 ;
    cornerPos.h = 1 ;
    // tlc
    cornerPos.x = outerPos.x ;
    cornerPos.y = outerPos.y ;
    gfx_setPixel( &cornerPos , &black );
    // trc
    cornerPos.x = ( outerPos.x + outerPos.w ) - 1 ;
    cornerPos.y = outerPos.y ;
    gfx_setPixel( &cornerPos , &black );
    // blc
    cornerPos.x = outerPos.x ;
    cornerPos.y = ( outerPos.y + outerPos.h ) - 1 ;
    gfx_setPixel( &cornerPos , &black );
    // brc
    cornerPos.x = ( outerPos.x + outerPos.w ) - 1 ;
    cornerPos.y = ( outerPos.y + outerPos.h ) - 1 ;
    gfx_setPixel( &cornerPos , &black );

    // Draw the button
    Pos_st buttonStart ;

    buttonStart.x       = outerPos.x + outerPos.w ;
    buttonStart.y       = outerPos.y + 3 ;
    buttonStart.w       = 2 ;
    buttonStart.h       = outerPos.h - 6 ;
    gfx_drawRect( &buttonStart , &white , true );

    return *startPos ;
}

/*
 * Function to draw RSSI-meter of arbitrary size
 * starting coordinates are relative to the top left point.
 *
 * *     *     *     *     *     *     *     *      *      *      *|
 * ***************             <-- Squelch                         |
 * ***************                                                 |
 * ******************************************                      |
 * ******************************************    <- RSSI           |
 * ******************************************                      |  <-- Height (px)
 * ******************************************                      |
 * 1     2     3     4     5     6     7     8     9     +10    +20|
 * _________________________________________________________________
 *
 * ^
 * |
 *
 * Width (px)
 *
 */
void gfx_drawSmeter( Pos_st* startPos , float rssi , float squelch , Color_st* color )
{
    Color_st   white      = { 255 , 255 , 255 , 255 } ;
    Color_st   yellow     = { 250 , 180 ,  19 , 255 } ;
    Color_st   red        = { 255 ,   0 ,   0 , 255 } ;
    FontSize_t font       = FONT_SIZE_5PT ;
    uint8_t    fontHeight = gfx_getFontHeight( font );
    uint16_t   barHeight  = ( startPos->h - 3 ) - fontHeight ;

    // S-level marks and numbers
    for( int index = 0 ; index < 12 ; index++ )
    {
        Color_st color    = ( ( index % 3 ) == 0 ) ? yellow : white ;
                 color    = ( index > 9 ) ? red : color ;
        Pos_st   pixelPos = *startPos;

        pixelPos.x += index * ( ( startPos->w - 1) / 11 ) ;
        gfx_setPixel( &pixelPos , &color );
        pixelPos.y += startPos->h ;

        if( index == 10 )
        {
            pixelPos.x -= 8 ;
            gfx_print( &pixelPos , font , GFX_ALIGN_LEFT , &color , "+%d" , index );
        }
        else
        {
            if( index == 11 )
            {
                pixelPos.x -= 10 ;
                gfx_print( &pixelPos , font , GFX_ALIGN_LEFT , &red , "+20" );
            }
            else
            {
                gfx_print( &pixelPos , font , GFX_ALIGN_LEFT , &color , "%d" , index );
            }
        }
        if( index == 10 )
        {
            pixelPos.x += 8 ;
        }
    }

    // Squelch bar
    Pos_st squelchPos = { startPos->x , (uint8_t)( startPos->y + 2 ) , startPos->w * squelch , barHeight / 3 };

    gfx_drawRect( &squelchPos , color , true );

    // RSSI bar
    uint16_t rssiHeight = ( barHeight * 2 ) / 3 ;
    float    s_level    = ( 127.0f + rssi ) / 6.0f ;
    uint16_t rssiWidth  = ( s_level < 0.0f ) ? 0 : ( s_level * ( startPos->w - 1 ) / 11 ) ;
             rssiWidth  = ( s_level > 10.0f ) ? startPos->w : rssiWidth ;
    Pos_st   rssiPos    = { startPos->x , (uint8_t) ( startPos->y + 2 + squelchPos.h ) , rssiWidth , rssiHeight };
    gfx_drawRect( &rssiPos , &white , true );
}

/*
 * Function to draw RSSI-meter with level-meter of arbitrary size
 * Version without squelch bar for digital protocols
 * starting coordinates are relative to the top left point.
 *
 * *             *                *               *               *|
 * ******************************************                      |
 * ******************************************    <- level          |
 * ******************************************                      |
 * ******************************************                      |
 * *             *                *               *               *|
 * ******************************************                      |  <-- Height (px)
 * ******************************************    <- RSSI           |
 * ******************************************                      |
 * ******************************************                      |
 * 1     2     3     4     5     6     7     8     9     +10    +20|
 * _________________________________________________________________
 *
 * ^
 * |
 *
 * Width (px)
 *
 */
void gfx_drawSmeterLevel( Pos_st* startPos , float rssi , uint8_t level )
{
    Color_st   red        = { 255 ,   0 ,   0 , 255 } ;
    Color_st   green      = {   0 , 255 ,   0 , 255 } ;
    Color_st   white      = { 255 , 255 , 255 , 255 } ;
    Color_st   yellow     = { 250 , 180 , 19  , 255 } ;
    FontSize_t font       = FONT_SIZE_5PT ;
    uint8_t    fontHeight = gfx_getFontHeight(font);
    uint16_t   barHeight  = ( ( startPos->h - 6 ) - fontHeight ) / 2;

    // Level meter marks
    for( int index = 0 ; index <= 4 ; index++ )
    {
        Pos_st pixelPos  = *startPos ;
        pixelPos.x      += ( index * ( startPos->w - 1 ) ) / 4;
        gfx_setPixel( &pixelPos , &white ) ;
        pixelPos.y      += ( barHeight + 3 ) ;
        gfx_setPixel( &pixelPos , &white );
    }
    // Level bar
    uint16_t levelWidth = ( level / ( 255.0 * startPos->w ) ) ;
    Pos_st   levelPos   = { startPos->x , (uint8_t)( startPos->y + 2 ) , levelWidth , barHeight };
    gfx_drawRect( &levelPos , &green , true );
    // RSSI bar
    float    s_level    = ( 127.0f + rssi ) / 6.0f ;
    uint16_t rssiWidth  = ( s_level < 0.0f ) ? 0 : ( s_level * ( startPos->w - 1 ) / 11 );
             rssiWidth  = ( s_level > 10.0f ) ? startPos->w : rssiWidth ;
    Pos_st   rssiPos    = { startPos->x , (uint8_t) (startPos->y + 5 + barHeight) , rssiWidth , barHeight };
    gfx_drawRect( &rssiPos , &white, true);
    // S-level marks and numbers
    for(int index = 0; index < 12; index++)
    {
        Color_st color     = ( ( index % 3 ) == 0 ) ? yellow : white ;
                 color     = ( index > 9 ) ? red : color ;
        Pos_st   pixelPos  = *startPos ;
        pixelPos.x        += ( index * ( startPos->w - 1 ) ) / 11 ;
        pixelPos.y        += startPos->h ;

        if( index == 10 )
        {
            pixelPos.x -= 8 ;
            gfx_print( &pixelPos , font , GFX_ALIGN_LEFT , &color , "+%d" , index );
        }
        else
        {
            if( index == 11 )
            {
                pixelPos.x -= 10;
                gfx_print( &pixelPos , font , GFX_ALIGN_LEFT , &red , "+20" );
            }
            else
            {
                gfx_print( &pixelPos , font , GFX_ALIGN_LEFT , &color , "%d" , index );
            }
            if( index == 10 )
            {
                pixelPos.x += 8 ;
            }
        }
    }
}

/*
 * Function to draw GPS satellites snr bar graph of arbitrary size
 * starting coordinates are relative to the top left point.
 *
 * ****            |
 * ****      ****  |
 * **** **** ****  |  <-- Height (px)
 * **** **** ****  |
 * **** **** ****  |
 * **** **** ****  |
 *                 |
 *  N    N+1  N+2  |
 * __________________
 *
 * ^
 * |
 *
 * Width (px)
 *
 */
void gfx_drawGPSgraph( Pos_st* startPos , gpssat_t* sats , uint32_t activeSats )
{
    Color_st white    = { 255 , 255 , 255 , 255 } ;
    Color_st yellow   = { 250 , 180 ,  19 , 255 } ;
    // SNR Bars and satellite identifiers
    uint8_t barWidth  = ( startPos->w - 26 ) / 12;

    for( int index = 0 ; index < 12 ; index++ )
    {
        uint8_t  barHeight = ( ( ( startPos->h - 8 ) * sats[ index ].snr ) / 100 ) + 1 ;
        Pos_st   barPos    = *startPos ;
                 barPos.x += 2 + index * ( barWidth + 2 ) ;
                 barPos.y += ( startPos->h - 8 ) - barHeight ;
                 barPos.w  = barWidth ;
                 barPos.h  = barHeight ;
        Color_st barColor  = ( activeSats & ( 1 << ( sats[ index ].id - 1 ) ) ) ? yellow : white ;

        gfx_drawRect( &barPos , &barColor, true );
        Pos_st idPos = { barPos.x , (uint8_t)( startPos->y + startPos->h ) , 0 , 0 };
        gfx_print( &idPos, FONT_SIZE_5PT, GFX_ALIGN_LEFT, &barColor, "%2d ", sats[ index ].id );
    }
    uint8_t barsWidth        = 9 + ( 11 * ( barWidth + 2 ) ) ;

    Pos_st left_line    = *startPos ;
    Pos_st right_line   = *startPos ;

    left_line.h         = startPos->h - 9 ;

    right_line.x       += barsWidth ;
    right_line.w        = barsWidth ;
    right_line.h        = startPos->h - 9 ;

    gfx_drawLine( &left_line , &white );
    gfx_drawLine( &right_line , &white );
}

void gfx_drawGPScompass( Pos_st* startPos , uint16_t radius ,
                         float   deg      , bool     active   )
{
    Color_st white      = { 255 , 255 , 255 , 255 } ;
    Color_st black      = {   0 ,   0 ,   0 , 255 } ;
    Color_st yellow     = { 250 , 180 ,  19 , 255 } ;
    // Compass circle
    Pos_st circlePos    = *startPos ;
           circlePos.x += radius + 1 ;
           circlePos.y += radius + 3 ;
           circlePos.w  = 1 ;
           circlePos.h += radius ;

    gfx_drawCircle( &circlePos , &white );

    Pos_st nBox = { (uint8_t)( ( startPos->x + radius ) - 5 ) , startPos->y , 13 , 13 };

    gfx_drawRect( &nBox , &black , true );

    float needle_radius = radius - 4;
    if( active )
    {
        // Needle
        deg -= 90.0f;
        Pos_st pos1 = { (uint8_t)( circlePos.x + ( needle_radius         * COS( deg           ) ) ) ,
                        (uint8_t)( circlePos.y + ( needle_radius         * SIN( deg           ) ) ) , 0 , 0 };
        Pos_st pos2 = { (uint8_t)( circlePos.x + ( needle_radius         * COS( deg + 145.0f  ) ) ) ,
                        (uint8_t)( circlePos.y + ( needle_radius         * SIN( deg + 145.0f  ) ) ) , 0 , 0 };
        Pos_st pos3 = { (uint8_t)( circlePos.x + ( ( needle_radius / 2 ) * COS( deg + 180.0f  ) ) ) ,
                        (uint8_t)( circlePos.y + ( ( needle_radius / 2 ) * SIN( deg + 180.0f  ) ) ) , 0 , 0 };
        Pos_st pos4 = { (uint8_t)( circlePos.x + ( needle_radius         * COS( deg - 145.0f  ) ) ) ,
                        (uint8_t)( circlePos.y + ( needle_radius         * SIN( deg - 145.0f  ) ) ) , 0 , 0 };
        Pos_st pos ;
        pos   = pos1 ;
        pos.w = pos2.x - pos1.x ;
        pos.h = pos2.y - pos1.y ;
        gfx_drawLine( &pos , &yellow );
        pos   = pos2 ;
        pos.w = pos3.x - pos2.x ;
        pos.h = pos3.y - pos2.y ;
        gfx_drawLine( &pos , &yellow );
        pos   = pos3 ;
        pos.w = pos4.x - pos3.x ;
        pos.h = pos4.y - pos3.y ;
        gfx_drawLine( &pos , &yellow );
        pos   = pos4 ;
        pos.w = pos1.x - pos4.x ;
        pos.h = pos1.y - pos4.y ;
        gfx_drawLine( &pos , &yellow );
    }
    // North indicator
    Pos_st n_pos = { (uint8_t)(startPos->x + radius - 3) ,
                     (uint8_t)(startPos->y + 7) , 0 , 0 };
    gfx_print( &n_pos , FONT_SIZE_6PT , GFX_ALIGN_LEFT , &white, "N");
}

void gfx_plotData( Pos_st* startPos , const int16_t* data , size_t len )
{
    uint16_t horizontalPos  = startPos->x ;
    Color_st white          = {255 , 255 , 255 , 255 } ;
    Pos_st   prevPos        = { 0 , 0 , 0 , 0 } ;
    Pos_st   pos            = { 0 , 0 , 0 , 0 } ;
    Pos_st   linePos ;
    bool     firstIteration = true ;

    for( size_t index = 0 ; index < len ; index++ )
    {
        horizontalPos++ ;
        if( horizontalPos > ( startPos->x + startPos->w ) )
        {
            break ;
        }
        pos.x = horizontalPos ;
        pos.y = startPos->y + ( startPos->h / 2 ) + ( ( data[ index ] * 4 ) / ( 2 * SHRT_MAX ) * startPos->h );
        if( pos.y > SCREEN_HEIGHT )
        {
            pos.y = SCREEN_HEIGHT ;
        }
        if( !firstIteration )
        {
            linePos   = prevPos ;
            linePos.w = pos.x - prevPos.x ;
            linePos.h = pos.y - prevPos.y ;
            gfx_drawLine( &linePos , &white );
        }
        prevPos = pos ;
        if( firstIteration )
        {
            firstIteration = false ;
        }
    }
}
