project(urpc)

add_library(urpc STATIC)

target_sources(
    urpc PRIVATE
    ../../src/urpc.c
    ../../src/urpc_server.c
    ../../src/urpc_mbox.c
    ../../src/urpc_mbox_server.c
)

target_include_directories(urpc PUBLIC ../../include)

target_link_libraries(
    urpc PUBLIC
    xos
    drv
)
