add_library(lsf_server STATIC)

file(GLOB SRCS ../../lsf_server/src/*.c)

target_sources(lsf_server PRIVATE ${SRCS})
target_include_directories(lsf_server PUBLIC ../../lsf_server/src)

if (CONFIG_LSF_IC_MESSAGE)
    target_sources(lsf_server PRIVATE ../../ic_message/ic_message.c)
    target_include_directories(lsf_server PUBLIC ../../ic_message)
endif()

target_link_libraries(lsf_server PUBLIC urpc dbg)

if(CONFIG_LSF_OS_XOS)
    target_compile_options(lsf_server PUBLIC -imacros${CMAKE_BINARY_DIR}/kconfig/include/generated/autoconf.h)
endif()
