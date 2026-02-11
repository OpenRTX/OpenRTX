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
# *   You should have received a copy of the GNU General Public License     *
# *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
# ***************************************************************************/

# Help
help()
{
   # Display Help
   echo "Script to build openrtx binaries and copy them to a server"
   echo
   echo "Syntax: nightly_build.sh [-t] <destination>"
   echo "options:"
   echo  "-t    Add timestamp and hash to filename"  
   echo "-h     Print this Help."
   echo
}

# Default to file without details
T=0

# Parse options
while getopts "t" option; do
   case $option in
      t)
         T=1
         ;;
      h) # display Help
         help
         exit;;
      \?) # incorrect option
         echo "Error: Invalid option"
         exit;;
   esac
done

#Exporting Paths in case we execute from crontab
export PATH="$HOME/.local/bin:$HOME/bin:/usr/local/bin:/usr/bin:/usr/local/sbin:/usr/sbin"
#Determing where the script is
MY_PATH="`dirname \"$0\"`"              # relative
MY_PATH="`( cd \"$MY_PATH\" && pwd )`"  # absolutized and normalized
#Jumping in main OpenRTX folder
cd $MY_PATH/..


TARGETS=(
	"openrtx_md3x0_wrap" 
	"openrtx_mduv3x0_wrap" 
	"openrtx_gd77_wrap" 
	"openrtx_dm1801_wrap"
	"openrtx_mod17_wrap"
)

#Preparation
rm -rf build_cm4
git reset --hard
git clean -fd
git pull

#Building

meson setup --cross-file cross_cm4.txt build_cm4

for i in "${!TARGETS[@]}"; do
   meson compile -C build_cm4 "${TARGETS[$i]}"
   if [ "$T" -eq "1" ]; then
     GIT_HASH=`git rev-parse --short HEAD`
     DATE=`date '+%Y%m%d'`
     mv build_cm4/${TARGETS[$i]}* "build_cm4/${TARGETS[$i]}-$DATE-$GIT_HASH-nightly.bin"  
   fi 
done

cd build_cm4
#Using command line option to upload to a scp server
scp *_wrap* "${@: -1}"

