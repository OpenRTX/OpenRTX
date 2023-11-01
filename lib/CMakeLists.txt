add_library(minmea STATIC minmea/minmea.c)
target_include_directories(minmea PUBLIC minmea/include)

add_library(qdec INTERFACE)
target_include_directories(qdec INTERFACE qdec/include)

if(OPENRTX_TARGET MATCHES "radio")
add_subdirectory(miosix-kernel)
endif()
