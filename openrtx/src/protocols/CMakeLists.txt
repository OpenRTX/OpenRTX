file(GLOB_RECURSE SOURCE_FILES *.c *.cpp)

add_library(Protocols STATIC ${SOURCE_FILES})

target_link_libraries(Protocols PUBLIC Platform)
