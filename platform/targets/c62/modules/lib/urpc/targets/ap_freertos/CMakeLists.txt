project(urpc)

add_library(urpc STATIC)

target_sources(
    urpc PRIVATE
    ../../src/urpc.c
    ../../src/urpc_client.c
    ../../src/urpc_mbox.c
    ../../src/urpc_mbox_client.c
)

target_include_directories(urpc PUBLIC ../../include)

target_link_libraries(
    urpc PUBLIC
    bsp
    rtos
    dbg
    drv
)

target_compile_definitions(
    urpc PUBLIC
    URPC_CLIENT_OS_PORT_PATH=${CMAKE_CURRENT_SOURCE_DIR}/port/urpc_os_port.h
)
