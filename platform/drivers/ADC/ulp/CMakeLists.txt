# SPDX-License-Identifier: GPL-3.0-or-later

#
# CMake configuration for ULP RISC-V ADC Sampler
# This builds as a separate project with the ULP RISC-V toolchain
#

cmake_minimum_required(VERSION 3.20.0)

# This CMakeLists.txt is invoked as an external project by the board CMakeLists.txt
# It uses the ESP32-S3 ULP RISC-V toolchain to compile the ULP coprocessor program

# Include ESP-IDF ULP utilities if available
if(DEFINED CMAKE_MODULE_PATH)
    include(IDFULPProject)
endif()

# Create ULP executable target
add_executable()

# Add our ULP source file
target_sources( PRIVATE ulp_adc_sampler.c)

# Add ULP core sources and apply default options
if(COMMAND ulp_apply_default_sources)
    ulp_apply_default_sources()
endif()

if(COMMAND ulp_apply_default_options)
    ulp_apply_default_options()
endif()

# Add build targets to generate .bin, .sym, .h, .ld files
if(COMMAND ulp_add_build_binary_targets)
    ulp_add_build_binary_targets()
endif()

# Include directories for ULP program
target_include_directories( PRIVATE
    
)
