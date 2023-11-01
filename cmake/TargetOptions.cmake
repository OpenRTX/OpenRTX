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

elseif(OPENRTX_TARGET STREQUAL "mduv3x0")
elseif(OPENRTX_TARGET STREQUAL "md9600")
elseif(OPENRTX_TARGET STREQUAL "gd77")
elseif(OPENRTX_TARGET STREQUAL "dm1801")
elseif(OPENRTX_TARGET STREQUAL "mod17")
endif()
