project('radio_tool', 'cpp')

# Building radio_tool natively in meson will be possible when native
# subproject building will be implemented.

#usb = dependency('libusb-1.0', version : '>=1.0.0')
#
#conf_data = configuration_data()
#conf_data.set('GIT_SHA1', '58ff5701466a6dc651e0d04778c4abec780f4643')
#conf_data.set('GIT_SHA1_SHORT', '58ff570')
#conf_data.set('PROJECT_VERSION', '0.1.0')
#conf_data.set('PROJECT_NAME', 'radio_tool')
#version_file = configure_file(input : 'src/version.cpp.in',
#               output : 'version.cpp',
#               configuration : conf_data)
#
#src = ['src/radio_tool.cpp',
#       'src/dfu.cpp',
#       'src/radio_factory.cpp',
#       'src/tyt_radio.cpp',
#       'src/tyt_dfu.cpp',
#       'src/tyt_fw.cpp',
#       'src/cs_fw.cpp',
#       'src/rdt.cpp',
#       version_file]
#
#inc = include_directories('include')

run_command('mkdir', 'build')
run_command('cmake', '.')
run_command('make', '-j4')

radio_tool = find_program('./radio_tool')

meson.override_find_program('radio_tool', radio_tool)
