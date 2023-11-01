file(GLOB_RECURSE SOURCE_FILES default/*.c default/*.cpp)

add_library(UI STATIC ${SOURCE_FILES})

target_link_libraries(UI PRIVATE Core)

if(OPENRTX_TARGET MATCHES "radio")
target_link_libraries(UI PUBLIC miosix)
endif()
