project(lsf_client)

add_library(lsf_client STATIC)

file(GLOB SRCS ../../lsf_client/src/*.c)
target_sources(lsf_client PRIVATE ${SRCS})

target_include_directories(lsf_client PUBLIC ../../lsf_client/include)

target_link_libraries(
    lsf_client PUBLIC
    rtos
    urpc
    dbg
)

target_compile_definitions(
    lsf_client PUBLIC
    LSF_CLIENT_OS_PORT_PATH=${CMAKE_CURRENT_SOURCE_DIR}/port/lsf_os_port.h
)
