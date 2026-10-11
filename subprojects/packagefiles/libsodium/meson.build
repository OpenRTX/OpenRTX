# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Cross-only Autotools build of libsodium 1.0.20 for Horse on Cortex-M4.
# Not --enable-minimal: that drops crypto_stream_xchacha20. Unused objects
# are dropped by the firmware --gc-sections link.

project('libsodium', 'c', version: '1.0.20', license: 'ISC',
        meson_version: '>=1.2.0')

mod = import('unstable-external_project')

extra_c = ' '.join([
  '-Os',
  '-ffunction-sections',
  '-fdata-sections',
  '-DSODIUM_STATIC',
  '-DRANDOMBYTES_CUSTOM_IMPLEMENTATION',
  '-DRANDOMBYTES_DEFAULT_IMPLEMENTATION=NULL',
])

p = mod.add_project(
  'configure',
  configure_options: [
    '--prefix=@PREFIX@',
    '--disable-shared',
    '--enable-static',
    '--disable-ssp',
    '--disable-pie',
    '--disable-asm',
    '--disable-blocking-random',
  ],
  cross_configure_options: [
    '--host=arm-miosix-eabi',
  ],
  env: {
    'CFLAGS': extra_c,
    'CPPFLAGS': '-DSODIUM_STATIC -DRANDOMBYTES_CUSTOM_IMPLEMENTATION -DRANDOMBYTES_DEFAULT_IMPLEMENTATION=NULL',
  },
  verbose: true,
)

libsodium_dep = p.dependency('sodium')
