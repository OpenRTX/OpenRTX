/***************************************************************************
 *   Copyright (C) 2020 by Terraneo Federico                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the suorce code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#pragma once

#include "interfaces/arch_registers.h"

/*
 * LCD driver
 * NOTE: the driver has been written considering the following assumptions
 * - the display connected is configured as 1/4 duty, 1/3 bias with 4 COMMON
 * - the refresh rate is 64Hz with low power waveforms, so really 32Hz
 * - although the peripheral supports up to 40 segments, this driver is limited
 *   to 32
 * 
 * In addition, the setDigit function assumes that:
 * - each digit is connected to two consecutive segment pins, the first being
 *   an even number
 * - the first digit and the second one are swapped, as this simplifies PCB
 *   routing, so digit 0 is connected to SEG2,3 while digit 1 to SEG0,1
 * - the mapping of the 7 segments of each digit is as follows:
 * 
 *       SEGx SEGx+1      --a--
 * COM0   d     dp       |     |
 * COM1   e     c        f     b
 * COM2   f     g        |     |
 * COM3   a     b         --g--
 *                       |     |
 *                       e     c
 *                       |     |
 *                        --d--   O dp
 */

namespace miosix {

/**
 * Initialize the LCD controller.
 * Requires the 32kHz oscillator to be started, see start32kHzOscillator() in
 * clock.h
 * 
 * \param numSegments number of segments of the display (1 to 32)
 * The corresponding number of pins will be claimed by the LCD peripheral and
 * will no longer be usable as GPIO
 * \param constrast display constrast (0 to 31)
 */
void initLcd(unsigned int numSegments, unsigned int contrast = 31);

/**
 * Must be called before changing segments on the display, such as by calling
 * setSegment() or setDigit()
 */
inline void beginUpdate()
{
    LCDCA->LCDCA_CFG |= LCDCA_CFG_LOCK;
}

/**
 * Must be called after changing segments to make the changes visible
 */
inline void endUpdate()
{
    LCDCA->LCDCA_CFG &= ~LCDCA_CFG_LOCK;
}

/**
 * Set/clear a single segment on the display
 * \param com which common (0 to 3)
 * \param seg which segment (0 to 31)
 * \param value true to turn on, false to turn off
 */
void setSegment(unsigned int com, unsigned int seg, int value); 

/**
 * Set a digit on the display.
 * \param digit which digit (0 to 15)
 * \param segments segments with the following mapping
 * bit     0 1 2 3 4 5 6 7
 * segment a b c d e f g dp
 */
void setDigit(unsigned int digit, unsigned char segments);

/**
 * Useful table with the 7 segment "font" for digits 0 to 9
 * \code
 * int value = 4;
 * beginUpdate();
 * setDigit(0,digitTbl[value]); //Shows 4 in digit 0 on the display
 * endUpdate();
 * \endcode
 */
extern const unsigned char digitTbl[];

/**
 * Enable a hadrware feature to blink all active segments
 * \param fast true for a ~500ms blink rate, false for ~1s blink rate
 */
void enableBlink(bool fast = true);

/**
 * Disable the blink feature
 */
inline void disableBlink()
{
    LCDCA->LCDCA_CR = LCDCA_CR_BSTOP;
}

} //namespace miosix
