#!/bin/sh
#/***************************************************************************
# *   Copyright (C) 2021 by Philipp Neumann DF5PMF,                         *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU General Public License as published by  *
# *   the Free Software Foundation; either version 3 of the License, or     *
# *   (at your option) any later version.                                   *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU General Public License for more details.                          *
# *                                                                         *
# *   As a special exception, if other files instantiate templates or use   *
# *   macros or inline functions from this file, or you compile this file   *
# *   and link it with other works to produce a work based on this file,    *
# *   this file does not by itself cause the resulting work to be covered   *
# *   by the GNU General Public License. However the source code for this   *
# *   file must still be made available in accordance with the GNU General  *
# *   Public License. This exception does not invalidate any other reasons  *
# *   why a work based on this file might be covered by the GNU General     *
# *   Public License.                                                       *
# *                                                                         *
# *   You should have received a copy of the GNU General Public License     *
# *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
# ***************************************************************************/

#Exporting Paths in case we execute from crontab
export PATH="$HOME/.local/bin:$HOME/bin:/usr/local/bin:/usr/bin:/usr/local/sbin:/usr/sbin"
#Determing where the script is
MY_PATH="`dirname \"$0\"`"              # relative
MY_PATH="`( cd \"$MY_PATH\" && pwd )`"  # absolutized and normalized
#Jumping in main OpenRTX folder
cd $MY_PATH/..

#Preperation
rm -rf build_arm
git reset --hard
git clean -fd
git pull
#Building
meson setup --cross-file cross_arm.txt build_arm
meson compile -C build_arm openrtx_md3x0_wrap
meson compile -C build_arm openrtx_mduv3x0_wrap
#meson compile -C build_arm openrtx_md9600
meson compile -C build_arm openrtx_gd77_wrap
meson compile -C build_arm openrtx_dm1801_wrap
#SCP
cd build_arm
#Using command line option to upload to a scp server
scp *_wrap* $1
