
if(CONFIG_BK4819)
  zephyr_library()
  zephyr_library_sources(bk4819.c)
  zephyr_library_sources(bk4819.h)
  zephyr_library_include_directories(.)
  
  # Add OpenRTX include paths for interfaces
  zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/openrtx/include)
  zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/platform/mcu/CSK6011B/drivers)
endif()