zephyr_library_named(af)

set(LIB_DIR ../..)

file(GLOB LIB_SRCS ${LIB_DIR}/*.c)
zephyr_library_sources(${LIB_SRCS})

target_include_directories(${ZEPHYR_CURRENT_LIBRARY} PUBLIC ${LIB_DIR})
target_include_directories(${ZEPHYR_CURRENT_LIBRARY} PRIVATE ${ZEPHYR_HAL_CSK_MODULE_DIR}/include)

zephyr_library_link_libraries(lsf_client)

zephyr_library_compile_options(
    -Wno-unused-function
    -Wno-unused-variable
    -Wno-pointer-arith
)
