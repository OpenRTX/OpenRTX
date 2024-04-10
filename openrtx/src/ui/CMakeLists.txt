file(GLOB_RECURSE DEFAULT_SRC default/*.c default/*.cpp)
file(GLOB_RECURSE MODULE17_SRC module17/*.c module17/*.cpp)

if(OPENRTX_TARGET STREQUAL "linux")
# Hack: build UI anyway for testing
add_library(UIDefault STATIC ${DEFAULT_SRC})
endif()

add_library(UI STATIC)

if(NOT OPENRTX_UI)
    set(OPENRTX_UI "default")
endif()

if(OPENRTX_UI STREQUAL "default")
    target_sources(UI INTERFACE ${DEFAULT_SRC})
elseif(OPENRTX_UI STREQUAL "module17")
    target_sources(UI PRIVATE ${MODULE17_SRC})
else()
    message(FATAL_ERROR "Invalid OPENRTX_UI '${OPENRTX_UI}'."
                        "Please choose one between: default, module17.")
endif()

message("Using OpenRTX UI: ${OPENRTX_UI}")
target_link_libraries(UI PRIVATE Core Platform)
