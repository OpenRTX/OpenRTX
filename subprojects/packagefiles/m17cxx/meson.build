project('m17cxx', 'cpp')

cmake = import('cmake') 

m17cxx_inc = include_directories('include/m17cxx')

# David Rowe's codec2 is needed by M17, included as a submodule
codec2_opt = cmake.subproject_options()
codec2_opt.add_cmake_defines({'UNITTEST': false})
codec2_opt.add_cmake_defines({'BUILD_SHARED_LIBS': false})
codec2_opt.append_compile_args('c', '-DCODEC2_MODE_EN_DEFAULT=0')
codec2_opt.append_compile_args('c', '-DCODEC2_MODE_3200_EN=1')
codec2_opt.append_compile_args('c', '-DCODEC2_MODE_1600_EN=1')
codec2_opt.append_compile_args('c', '-DFREEDV_MODE_EN_DEFAULT=0')
codec2_proj = cmake.subproject('codec2', options : codec2_opt)
codec2_lib = codec2_proj.dependency('codec2')

m17cxx = static_library('m17cxx',
                        dependencies : codec2_lib,
                        include_directories : m17cxx_inc,
                        install : false)

m17cxx_dep = declare_dependency(include_directories : m17cxx_inc,
                                link_with : m17cxx)
