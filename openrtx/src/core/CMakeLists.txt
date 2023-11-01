#Disabled because backup.c does not work on Linux
file(GLOB_RECURSE SOURCE_FILES *.c *.cpp)

# Hack
#set(SOURCE_FILES
#  state.c
#  threads.c
#  battery.c
#  graphics.c
#  input.c
#  utils.c
#  queue.c
#  chan.c
#  gps.c
#  dsp.cpp
#  cps.c
#  crc.c
#  datetime.c
#  openrtx.c
#  audio_codec.c
#  audio_stream.c
#  audio_path.cpp
#  data_conversion.c
#  memory_profiling.cpp
#  voicePromptData.S
#  voicePrompts.c
#  voicePromptUtils.c
#)

set_property(SOURCE voicePromptData.S PROPERTY LANGUAGE C)

add_library(Core STATIC ${SOURCE_FILES})
target_link_libraries(Core PUBLIC minmea)

if(OPENRTX_TARGET MATCHES "radio")
target_link_libraries(Core PUBLIC miosix)
endif()
