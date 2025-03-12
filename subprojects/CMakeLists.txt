# Don't build codec2 for linux, link against an external one

if(NOT OPENRTX_TARGET STREQUAL "linux")
add_library(codec2 STATIC
    codec2/src/dump.c
    codec2/src/lpc.c
    codec2/src/nlp.c
    codec2/src/phase.c
    codec2/src/quantise.c
    codec2/src/postfilter.c
    codec2/src/codec2.c
    codec2/src/codec2_fft.c
    codec2/src/lsp.c
    codec2/src/sine.c
    codec2/src/interp.c
    codec2/src/kiss_fft.c
    codec2/src/kiss_fftr.c
    codec2/src/newamp1.c
    codec2/src/codebook.c
    codec2/src/codebookd.c
    codec2/src/pack.c
    codec2/src/codebooknewamp1.c
    codec2/src/codebooknewamp1_energy.c
)

target_compile_definitions(codec2 PUBLIC
  -DCODEC2_MODE_EN_DEFAULT=0
  -DCODEC2_MODE_3200_EN=1
  -DCODEC2_MODE_1600_EN=1
  -DFREEDV_MODE_EN_DEFAULT=0
)

target_compile_options(codec2 PRIVATE -w)

target_include_directories(codec2 PUBLIC codec2 codec2/src)
endif()
