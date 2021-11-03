#!/bin/bash -e

#  Copyright (C) 2021 by Alain Carlucci.
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, see <http://www.gnu.org/licenses/>

function show_help {
    echo "OpenRTX clang-format tool."
    echo ""
    echo "Usage: $0 [--check] [--help]"
    echo ""
    echo "To apply clang-format style to the whole codebase, run without arguments"
    echo "To check if the codebase is compliant, run with --check"
}

if [ $# -gt 1 ]; then
    show_help
    exit 1
fi

if [ $# -eq 1 ]; then
    if [ "$1" == "--help" ]; then
        show_help
        exit 0
    fi

    if [ "$1" != "--check" ]; then
        echo "Invalid argument $1"
        show_help
        exit 1
    fi
fi

FILE_LIST=$(git ls-files | egrep '\.(c|cpp|h)$' | egrep -v 'lib/|subprojects/|platform/mcu')

CHECK_ARGS=""
if [ "$1" == "--check" ]; then
    CHECK_ARGS="--dry-run -Werror"
fi

clang-format $CHECK_ARGS -i $FILE_LIST
