if (CONFIG_LSF)
    zephyr_library_named(lsf_client)

    set(LIB_DIR ../../lsf_client)

    file(GLOB LIB_SRCS ${LIB_DIR}/src/*.c)
    zephyr_library_sources(${LIB_SRCS})

    if (CONFIG_LSF_IC_MESSAGE)
    zephyr_library_sources(../../ic_message/ic_message.c)
    zephyr_include_directories(../../ic_message)
    endif()

    zephyr_include_directories(${LIB_DIR}/include)

    # TODO: 将来要重构
    zephyr_library_link_libraries(freertos_shims)
    zephyr_library_compile_definitions(
        LSF_CLIENT_OS_PORT_PATH=${CMAKE_CURRENT_SOURCE_DIR}/../ap_freertos/port/lsf_os_port.h
    )

    zephyr_library_link_libraries(urpc)

    zephyr_library_include_directories(${ZEPHYR_HAL_CSK_MODULE_DIR}/include)

    zephyr_library_compile_options(
        -Wno-unused-function
        -Wno-pointer-arith
        -Wno-discarded-qualifiers
        -Wno-unused-variable
        -Wno-int-conversion
        -Wno-format
        -Wno-format-security
    )
endif()
