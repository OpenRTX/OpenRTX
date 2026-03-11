
if(CONFIG_BK1080)
  zephyr_library()
  zephyr_library_sources(bk1080.c)
  zephyr_library_sources(bk1080.h)
  zephyr_library_include_directories(.)
  
  # Add OpenRTX include paths for interfaces
  zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/openrtx/include)
  zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/platform/mcu/CSK6011B/drivers)
endif()