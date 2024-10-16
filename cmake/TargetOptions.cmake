if(OPENRTX_TARGET STREQUAL "linux")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fsanitize=address,undefined")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fsanitize=address,undefined")

    find_package(SDL2 REQUIRED)
    find_package(Codec2 REQUIRED)

    add_compile_definitions(-DPLATFORM_LINUX)
    add_compile_definitions(-Dsniprintf=snprintf -Dvsniprintf=vsnprintf)

    if(OPENRTX_LINUX_FLAVOR STREQUAL "default")
        add_compile_definitions(
            -DCONFIG_SCREEN_WIDTH=160
            -DCONFIG_SCREEN_HEIGHT=128
            -DCONFIG_PIX_FMT_RGB565
            -DCONFIG_GPS
            -DCONFIG_RTC
        )
    elseif(OPENRTX_LINUX_FLAVOR STREQUAL "small")
        add_compile_definitions(
            -DCONFIG_SCREEN_WIDTH=128
            -DCONFIG_SCREEN_HEIGHT=64
            -DCONFIG_PIX_FMT_BW
            -DCONFIG_GPS
            -DCONFIG_RTC
        )
    elseif(OPENRTX_LINUX_FLAVOR STREQUAL "module17")
        set(OPENRTX_UI module17)
        add_compile_definitions(
            -DCONFIG_SCREEN_WIDTH=128
            -DCONFIG_SCREEN_HEIGHT=64
            -DCONFIG_PIX_FMT_BW
        )
    else()
        message(FATAL_ERROR 
            "Invalid variable OPENRTX_LINUX_FLAVOR. "
            "Please choose one between: default, small, module17.")
    endif()

    message("Using linux flavor: ${OPENRTX_LINUX_FLAVOR}")

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
elseif(OPENRTX_TARGET STREQUAL "module17")
    add_compile_definitions(-DPLATFORM_MOD17)
endif()
