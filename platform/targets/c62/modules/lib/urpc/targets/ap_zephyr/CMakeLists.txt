zephyr_library_named(urpc)

set(LIB_DIR ../..)

zephyr_library_sources(
    ${LIB_DIR}/src/urpc.c
    ${LIB_DIR}/src/urpc_client.c
    ${LIB_DIR}/src/urpc_mbox.c
    ${LIB_DIR}/src/urpc_mbox_client.c
)

target_include_directories(${ZEPHYR_CURRENT_LIBRARY} PUBLIC ${LIB_DIR}/include)

zephyr_library_sources(mbx_isr.c)

# TODO: 将来要重构
zephyr_library_link_libraries(freertos_shims)
zephyr_library_compile_definitions(
    URPC_CLIENT_OS_PORT_PATH=${CMAKE_CURRENT_SOURCE_DIR}/../ap_freertos/port/urpc_os_port.h
)

zephyr_library_include_directories(${ZEPHYR_BASE}/include/zephyr/posix)

zephyr_library_compile_options(
    -Wno-unused-variable
)
