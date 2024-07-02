/***************************************************************************
 *   Copyright (C) 2010, 2011, 2012, 2013, 2014 by Terraneo Federico       *
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
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include "config/miosix_settings.h"

// These two #if are here because version checking for config files in
// out-of-git-tree projects has to be done somewhere.

#if MIOSIX_SETTINGS_VERSION != 300
#error You need to update miosix_settings.h to match the version in the kernel.
#endif

#if BOARD_SETTINGS_VERSION != 300
#error You need to update board_settings.h to match the version in the kernel.
#endif

namespace miosix {

#define tts(x) #x
#define ts(x) tts(x)

#if defined(__GNUC__) && !defined(__clang__)
#define CV ", gcc " \
    ts(__GNUC__) "." ts(__GNUC_MINOR__) "." ts(__GNUC_PATCHLEVEL__) \
    "-mp" ts(_MIOSIX_GCC_PATCH_MAJOR) "." ts(_MIOSIX_GCC_PATCH_MINOR)
#define AU __attribute__((used))
#else
#define CV
#define AU
#endif

const char AU ver[]="Miosix v2.7 (" _MIOSIX_BOARDNAME ", " __DATE__ " " __TIME__ CV ")";

const char *getMiosixVersion()
{
    return ver;
}

} //namespace miosix
