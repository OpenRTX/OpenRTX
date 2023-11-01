file(GLOB_RECURSE SOURCE_FILES *.c *.cpp)

add_library(Protocols STATIC ${SOURCE_FILES})

if(OPENRTX_TARGET MATCHES "radio")
target_link_libraries(Protocols PUBLIC miosix)
endif()
