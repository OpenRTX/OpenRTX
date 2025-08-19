add_library(minmea STATIC minmea/minmea.c)
target_include_directories(minmea PUBLIC minmea/include)

add_library(qdec INTERFACE)
target_include_directories(qdec INTERFACE qdec/include)

# For every target except linux, build miosix
if(NOT OPENRTX_TARGET STREQUAL "linux")
add_subdirectory(miosix-kernel)
endif()
