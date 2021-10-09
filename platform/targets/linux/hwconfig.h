/***************************************************************************
 *   Copyright (C) 2020 by Frederik Saraci IU2NRO                          *
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
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

/* Device has a working real time clock */
#define HAS_RTC

/* Device supports an optional GPS chip */
#define HAS_GPS

/* Battery type */
#define BAT_LIPO_2S

#define GPIOA "PA"
#define GPIOB "PB"
#define GPIOC "PC"
#define GPIOD "PD"
#define GPIOE "PE"
#define GPIOF "PF"
#define GPIOG "PG"
#define GPIOH "PH"
#define GPIOI "PI"
#define GPIOJ "PJ"
#define GPIOK "PK"

/* Signalling LEDs */
#define GREEN_LED  "GREEN_LED",0
#define RED_LED    "RED_LED",1

/* Analog inputs */
#define AIN_VOLUME "AIN_VOLUME",0
#define AIN_VBAT   "AIN_VBAT",1
#define AIN_MIC    "AIN_MIC",3
#define AIN_RSSI   "AIN_RSSI",0

/* Channel selection rotary encoder */
#define CH_SELECTOR_0 "CH_SELECTOR_0",14
#define CH_SELECTOR_1 "CH_SELECTOR_1",15
#define CH_SELECTOR_2 "CH_SELECTOR_2",10
#define CH_SELECTOR_3 "CH_SELECTOR_3",11

/* Push-to-talk switch */
#define PTT_SW "PTT_SW",11
