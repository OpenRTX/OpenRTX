project('codec2', 'cpp')

cmake = import('cmake')

codec2_inc = include_directories('.', 'src')
codec2_src = ['src/dump.c',
              'src/lpc.c',
              'src/nlp.c',
              'src/phase.c',
              'src/quantise.c',
              'src/postfilter.c',
              'src/codec2.c',
              'src/codec2_fft.c',
              'src/lsp.c',
              'src/sine.c',
              'src/interp.c',
              'src/kiss_fft.c',
              'src/kiss_fftr.c',
              'src/newamp1.c',
              'src/codebook.c',
              'src/codebookd.c',
              'src/pack.c',
              'src/codebooknewamp1.c',
              'src/codebooknewamp1_energy.c']

add_project_arguments('-DCODEC2_MODE_EN_DEFAULT=0', language : 'c')
add_project_arguments('-DCODEC2_MODE_3200_EN=1'   , language : 'c')
add_project_arguments('-DCODEC2_MODE_1600_EN=1'   , language : 'c')
add_project_arguments('-DFREEDV_MODE_EN_DEFAULT=0', language : 'c')

codec2 = static_library('codec2',
                        codec2_src,
                        include_directories : codec2_inc,
                        install : false)

codec2_dep = declare_dependency(include_directories : codec2_inc,
                                link_with : codec2)
