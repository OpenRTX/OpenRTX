if (CONFIG_FREERTOS_SHIMS)
    zephyr_library_named(freertos_shims)

    file(GLOB SHIMS_SRCS src/*.c)
    zephyr_library_sources(${SHIMS_SRCS})

    target_include_directories(${ZEPHYR_CURRENT_LIBRARY} PUBLIC include)

    zephyr_library_compile_options(
        -Wno-discarded-qualifiers
        -Wno-return-type
        -Wno-format
        -Wno-format-security
    )
endif()
