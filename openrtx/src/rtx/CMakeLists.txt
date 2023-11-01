file(GLOB_RECURSE SOURCE_FILES *.c *.cpp)

add_library(RTX STATIC ${SOURCE_FILES})

target_link_libraries(RTX PRIVATE Protocols)

if(OPENRTX_TARGET MATCHES "radio")
target_link_libraries(RTX PUBLIC miosix platform)
endif()
