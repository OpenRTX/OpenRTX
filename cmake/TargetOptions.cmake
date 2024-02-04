if(OPENRTX_TARGET STREQUAL "linux")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fsanitize=address,undefined")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fsanitize=address,undefined")

    find_package(SDL2 REQUIRED)
    find_package(Codec2 REQUIRED)

    add_compile_definitions(-DPLATFORM_LINUX)
    add_compile_definitions(-DSCREEN_WIDTH=160 -DSCREEN_HEIGHT=128 -DPIX_FMT_RGB565)
elseif(OPENRTX_TARGET STREQUAL "md3x0")
    add_compile_definitions(-DPLATFORM_MD3x0 -Dtimegm=mktime)
    add_compile_definitions(STM32F405xx HSE_VALUE=8000000)

    set(TARGET_WRAP MD380)
    set(TARGET_LOAD_ADDR 0x0800C000)
elseif(OPENRTX_TARGET STREQUAL "mduv3x0")
    add_compile_definitions(-DPLATFORM_MDUV3x0 -Dtimegm=mktime)
    add_compile_definitions(STM32F405xx HSE_VALUE=8000000)

    set(TARGET_WRAP UV3X0)
    set(TARGET_LOAD_ADDR 0x0800C000)
elseif(OPENRTX_TARGET STREQUAL "md9600")
    add_compile_definitions(-DPLATFORM_MD9600)
    add_compile_definitions(STM32F405xx HSE_VALUE=8000000)

    set(TARGET_WRAP MD9600)
    set(TARGET_LOAD_ADDR 0x0800C000)
elseif(OPENRTX_TARGET STREQUAL "gd77")
    add_compile_definitions(-DPLATFORM_GD77)

    set(TARGET_WRAP GD-77)
    set(TARGET_LOAD_ADDR 0x0800C000)
elseif(OPENRTX_TARGET STREQUAL "dm1801")
    add_compile_definitions(-DPLATFORM_DM1801)

    set(TARGET_WRAP DM-1801)
    set(TARGET_LOAD_ADDR 0x0800C000)
elseif(OPENRTX_TARGET STREQUAL "mod17")
endif()
