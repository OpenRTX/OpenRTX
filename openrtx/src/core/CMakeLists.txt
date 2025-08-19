# Hack
if(OPENRTX_TARGET STREQUAL "linux")
set(SOURCE_FILES
  audio_codec.c
  audio_path.cpp
  audio_stream.c
  battery.c
  chan.c
  cps.c
  crc.c
  data_conversion.c
  datetime.c
  dsp.cpp
  gps.c
  graphics.c
  input.c
  memory_profiling.cpp
  nvmem_access.c
  openrtx.c
  queue.c
  state.c
  threads.c
  utils.c
  voicePromptUtils.c
  voicePrompts.c
)
else()
file(GLOB_RECURSE SOURCE_FILES *.c *.cpp *.S)
endif()

set_property(SOURCE voicePromptData.S APPEND PROPERTY LANGUAGE CXX)

add_library(Core STATIC ${SOURCE_FILES})
target_link_libraries(Core PUBLIC minmea codec2 Platform)
