cmake_minimum_required(VERSION 3.10)
include(cmake/ConfigureTarget.cmake)
project(OpenRTX)

find_program(RADIO_TOOL radio_tool)
find_program(BIN2SGL "bin2sgl.${CMAKE_HOST_SYSTEM_NAME}"
    PATHS scripts 
)

include(cmake/TargetOptions.cmake)

# These should not be global
include_directories(
  openrtx/include
  openrtx/include/core
  openrtx/include/rtx
  openrtx/include/calibration
  openrtx/include/protocols
  openrtx/include/fonts/adafruit
  openrtx/include/fonts/symbols
)

# Get GIT version and define a variable with that
execute_process(
    COMMAND git describe --tags --always
    OUTPUT_VARIABLE GIT_REPO_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

add_compile_definitions(
  -DGIT_VERSION="${GIT_REPO_VERSION}"
  -DFONT_UBUNTU_REGULAR
)

if(OPENRTX_TARGET STREQUAL "linux")
add_compile_definitions(-DVP_USE_FILESYSTEM)
endif()

add_subdirectory(platform)
add_subdirectory(openrtx)
add_subdirectory(lib)
add_subdirectory(subprojects)

add_executable(OpenRTX
    openrtx/src/main.c
)

target_link_libraries(OpenRTX PUBLIC Core Protocols RTX UI codec2 minmea)

if(OPENRTX_TARGET STREQUAL "linux")
    target_link_libraries(OpenRTX PUBLIC readline SDL2::SDL2)
elseif( OPENRTX_TARGET STREQUAL "md3x0"
     OR OPENRTX_TARGET STREQUAL "mduv3x0"
     OR OPENRTX_TARGET STREQUAL "md9600"
)
    target_link_libraries(OpenRTX PUBLIC Miosix)

    target_link_options(OpenRTX PUBLIC 
        -Wl,-T../platform/mcu/STM32F4xx/linker_script_MDx.ld
        -Wl,--print-memory-usage
    )

    add_custom_target(OpenRTX_bin ALL
        COMMAND ${CMAKE_OBJCOPY} -O binary OpenRTX OpenRTX.bin
        DEPENDS OpenRTX
        BYPRODUCTS OpenRTX.bin
        COMMENT "Objcopy OpenRTX -> OpenRTX.bin"
    )
elseif( OPENRTX_TARGET STREQUAL "gd77"
     OR OPENRTX_TARGET STREQUAL "dm1801")
    target_link_libraries(OpenRTX PUBLIC Miosix)

    target_link_options(OpenRTX PUBLIC 
        -Wl,-T../platform/mcu/MK22FN512xxx12/linker_script.ld
        -Wl,--print-memory-usage
    )

    add_custom_target(OpenRTX_bin ALL
        COMMAND ${CMAKE_OBJCOPY} -O binary OpenRTX OpenRTX.bin
        DEPENDS OpenRTX
        BYPRODUCTS OpenRTX.bin
        COMMENT "Objcopy OpenRTX -> OpenRTX.bin"
    )
endif()

# Sanity checks (TODO: de-hardcode target names)
if(TARGET_WRAP STREQUAL "GD-77" OR OPENRTX_TARGET STREQUAL "DM-1801")
    if(NOT BIN2SGL)
        message(WARNING "Disabling wrap because bin2sgl has not been found")
        unset(TARGET_WRAP)
    endif()
else()
    if(NOT RADIO_TOOL)
        message(WARNING "Disabling wrap because radio_tool has not been found")
        unset(TARGET_WRAP)
    endif()
endif()

if(TARGET_WRAP)
    message("Enabled target wrap: ${TARGET_WRAP}")
    set(BIN2SGL ${CMAKE_SOURCE_DIR}/scripts/bin2sgl.Linux)

    if (TARGET_WRAP STREQUAL "GD-77" OR OPENRTX_TARGET STREQUAL "DM-1801")
        add_custom_target(OpenRTX_wrap ALL
            DEPENDS OpenRTX_bin
            COMMAND ${BIN2SGL} -f OpenRTX.bin -m ${TARGET_WRAP} 
            COMMAND mv OpenRTX.sgl OpenRTX_wrap.sgl
            BYPRODUCTS OpenRTX_wrap.sgl
            COMMENT "WRAP using ${BIN2SGL}"
        )
    else()
        if(NOT TARGET_LOAD_ADDR)
            message(FATAL_ERROR "TARGET_WRAP defined but not TARGET_LOAD_ADDR")
        endif()

        add_custom_target(OpenRTX_wrap ALL
            DEPENDS OpenRTX_bin
            COMMAND ${RADIO_TOOL} --wrap
                -o OpenRTX_wrap.sgl
                -r ${TARGET_WRAP}
                -s ${TARGET_LOAD_ADDR}:OpenRTX.bin
            BYPRODUCTS OpenRTX_wrap.sgl
            COMMENT "WRAP using ${RADIO_TOOL}"
        )
    endif()
endif()
