#!/bin/sh

BUILD="build_cm4"
if [ ! -d ${BUILD} ]; then
    echo "Setting up Cortex-M4 cross build directory: ${BUILD}"
    meson setup --cross-file cross_cm4.txt ${BUILD}
fi

BUILD="build_cm7"
if [ ! -d ${BUILD} ]; then
    echo "Setting up Cortex-M7 cross build directory: ${BUILD}"
    meson setup --cross-file cross_cm7.txt ${BUILD}
fi

BUILD="build_linux"
if [ ! -d ${BUILD} ]; then
    echo "Setting up Linux emulator build directory: ${BUILD}"
    meson setup ${BUILD}
fi
