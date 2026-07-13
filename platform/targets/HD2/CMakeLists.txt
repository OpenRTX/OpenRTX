# OpenRTX threaded app for the Ailunce HD2, built on the vendor Miosix kernel
# (consumed as a dependency from vendor/miosix-kernel).  This build tree lives
# in the OpenRTX target dir; the miosix repo carries no OpenRTX/app code.
cmake_minimum_required(VERSION 3.21)

# Kernel dependency: vendor/miosix-kernel/miosix (this dir is
# vendor/OpenRTX/platform/targets/HD2 -> up 4 to vendor/, then miosix-kernel).
set(MIOSIX_KPATH ${CMAKE_SOURCE_DIR}/../../../../miosix-kernel/miosix)
set(CMAKE_TOOLCHAIN_FILE ${MIOSIX_KPATH}/cmake/Toolchains/gcc-csky.cmake)
set(CMAKE_BUILD_TYPE "Release")

project(OpenRtxHd2 C CXX ASM)

set(MIOSIX_BOARD hrc7000_hd2)
set(MIOSIX_USER_CONFIG_PATH ${MIOSIX_KPATH}/config)

# App-specific low-RAM linker script (codec2/M17 heap): kept beside this app
# build, out of the kernel tree.  Selected via the miosix absolute-path linker
# support (needs the M17/codec2 memory layout only when those are enabled).
if(HD2_M17_VOICE OR HD2_M17)
    set(MIOSIX_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/hd2_m17.ld)
endif()

# Hard-lock trace forensics (debug build): define globally BEFORE the kernel
# subdir so it reaches the miosix kernel lib (bsp/os_timer/interrupts) AND the
# app units.  The kernel's board_settings.h leaves the flag off, so a plain
# kernel build stays trace-free; this app build opts in here.
add_compile_definitions(WITH_HD2_TRACE)

add_subdirectory(${MIOSIX_KPATH} miosix EXCLUDE_FROM_ALL)

# Sibling OpenRTX checkout: this dir is vendor/OpenRTX/platform/targets/HD2,
# so up 3 is the OpenRTX repo root.
set(ORTX ${CMAKE_SOURCE_DIR}/../../..)

set(ORTX_CORE
    ${ORTX}/openrtx/src/core/state.c
    ${ORTX}/openrtx/src/core/threads.c
    ${ORTX}/openrtx/src/core/graphics.c
    ${ORTX}/openrtx/src/core/input.c
    ${ORTX}/openrtx/src/core/utils.c
    ${ORTX}/openrtx/src/core/queue.c
    ${ORTX}/openrtx/src/core/chan.c
    ${ORTX}/openrtx/src/core/gps.c
    ${ORTX}/openrtx/src/core/cps.c
    ${ORTX}/openrtx/src/core/crc.c
    ${ORTX}/openrtx/src/core/datetime.c
    ${ORTX}/openrtx/src/core/openrtx.c
    ${ORTX}/openrtx/src/core/data_conversion.c
    ${ORTX}/openrtx/src/core/nvmem_access.c
    ${ORTX}/openrtx/src/core/vendor_cps_settings.c
    ${ORTX}/openrtx/src/core/audio_path.cpp
    ${ORTX}/openrtx/src/core/audio_stream.c
    ${ORTX}/openrtx/src/core/voicePromptUtils.c)

set(ORTX_UI
    ${ORTX}/openrtx/src/ui/default/ui.c
    ${ORTX}/openrtx/src/ui/default/ui_main.c
    ${ORTX}/openrtx/src/ui/default/ui_menu.c
    ${ORTX}/openrtx/src/ui/default/ui_strings.c)

# Real HD2 drivers (reused as-is from the superloop bring-up).
set(ORTX_HD2
    ${ORTX}/platform/targets/HD2/platform.c
    ${ORTX}/platform/targets/HD2/hwconfig.c
    ${ORTX}/platform/drivers/display/ST7735S_HD2.c
    ${ORTX}/platform/drivers/keyboard/keyboard_HD2.c
    ${ORTX}/platform/drivers/backlight/backlight_HD2.c
    ${ORTX}/platform/drivers/GPS/gps_HD2.c
    ${ORTX}/platform/drivers/GPS/nmea_rbuf.c
    ${ORTX}/platform/mcu/CSKY_V2/drivers/gpio_csky.c
    ${ORTX}/platform/mcu/CSKY_V2/drivers/i2c_csky.c
    ${ORTX}/platform/mcu/CSKY_V2/drivers/adc_hd2.c
    ${ORTX}/platform/mcu/CSKY_V2/drivers/rtc_hd2.c
    ${ORTX}/platform/targets/HD2/hd2_router.c
    ${ORTX}/platform/targets/HD2/hd2_cps_settings_export.c
    ${ORTX}/platform/targets/HD2/watchdog_HD2.c
    ${ORTX}/platform/targets/HD2/hd2_diag.cpp
    ${ORTX}/platform/targets/HD2/hd2_pcm_stream.cpp
    ${ORTX}/platform/targets/HD2/hd2_gps.cpp
    ${ORTX}/platform/targets/HD2/hd2_adpcm_sample.c
    # Portable rtx task + OpModes (replaces the HD2 fork's hd2_rtx.c).  rtx.cpp
    # dispatches OpMode_FM (FM voice + 1750/VOX/tail extras + sq_cmp squelch) and
    # OpMode_FMBroadcast (RDA5802E tuner); the HD2 radio/tuner drivers provide
    # the strong HAL overrides (squelch, RSSI peak-hold, TX extras, tuner_*).
    ${ORTX}/openrtx/src/rtx/rtx.cpp
    ${ORTX}/openrtx/src/rtx/OpMode_FM.cpp
    ${ORTX}/openrtx/src/rtx/OpMode_DMR.cpp
    ${ORTX}/openrtx/src/rtx/OpMode_FMBroadcast.cpp)

# Hardware-agnostic protocol modules (openrtx/src/protocols), structured per
# OSI layers like M17.  APRS = 1200-baud AFSK + HDLC + AX.25, RX (Decoder) + TX (Encoder).
set(ORTX_PROTOCOLS
    ${ORTX}/openrtx/src/protocols/APRS/Afsk1200.cpp
    ${ORTX}/openrtx/src/protocols/APRS/Hdlc.cpp
    ${ORTX}/openrtx/src/protocols/APRS/Ax25.cpp
    ${ORTX}/openrtx/src/protocols/APRS/Decoder.cpp
    ${ORTX}/openrtx/src/protocols/APRS/Encoder.cpp)

# Persistent settings + last-VFO storage on the W25Q SPI-NOR (GPIOA
# bit-bang).  nvmem_settings_HD2.c implements the settings half of
# interfaces/nvmem.h (replaces the stubs that used to live in
# hd2_stubs.c); spi_hd2.c is the underlying bit-bang SPI bus driver.
# The codeplug half of nvmem stays stubbed (cps_io_stub.c below).
set(ORTX_NVM
    ${ORTX}/platform/drivers/SPI/spi_hd2.c
    ${ORTX}/platform/drivers/NVM/flash_w25q_HD2.c
    ${ORTX}/platform/drivers/NVM/nvmem_settings_HD2.c
    ${ORTX}/platform/drivers/NVM/nvmem_vendor_settings_HD2.c
    ${ORTX}/platform/drivers/NVM/nvmem_vendor_records_HD2.c)

# Phase C: real analog-FM RX *control* path (tune + RSSI + tone squelch), plus
# the audio_HD2 routing matrix (interfaces/audio.h; replaces audio_stub.c) which
# owns the board-level speaker-amp / RX-route GPIO and the codec/socsys audio
# gate.  hd2_router.c (in ORTX_HD2) drives the matrix + named twiddlers over the
# loader protocol.
set(ORTX_RADIO
    ${ORTX}/platform/drivers/baseband/radio_HD2.cpp
    ${ORTX}/platform/drivers/baseband/dmr_HD2.cpp
    ${ORTX}/platform/drivers/baseband/AT1846S_HD2.cpp
    ${ORTX}/platform/drivers/baseband/RDA5802E_HD2.c
    ${ORTX}/platform/drivers/audio/audio_HD2.c
    ${ORTX}/platform/drivers/audio/outputStream_HD2.cpp)

# Stubs for the not-yet-wired subsystems (UI scope).
set(ORTX_STUBS
    ${ORTX}/platform/drivers/NVM/cps_io_HD2.c)

# Vendored helper libs OpenRTX core pulls in (cf. meson openrtx_src/inc).
set(ORTX_LIBS
    ${ORTX}/lib/minmea/minmea.c
    ${ORTX}/lib/QRCode/qrcode.c)

# Voice prompts: HD2 IMA-ADPCM player (voicePrompts_HD2.c replaces the stock
# codec2-based core/voicePrompts.c -- codec2 is too slow on this soft-float
# core).  Clip data = an ADPCM container embedded via voicePromptDataHD2.S
# (.incbin voiceprompts.vpa, built by scripts/vpc_to_adpcm.py).  No codec2,
# no audio_codec.c, no dsp.cpp.
set(ORTX_VP
    ${ORTX}/platform/targets/HD2/voicePrompts_HD2.c
    ${ORTX}/platform/targets/HD2/voicePromptDataHD2.S)

# voicePromptDataHD2.S does `.incbin "voiceprompts.vpa"`; GAS resolves .incbin
# via -I (the .vpa is copied beside the .S in platform/targets/HD2).
set_source_files_properties(${ORTX}/platform/targets/HD2/voicePromptDataHD2.S
    PROPERTIES COMPILE_OPTIONS "-Wa,-I${ORTX}/platform/targets/HD2")

add_executable(openrtx_hd2
    ${CMAKE_SOURCE_DIR}/main.cpp
    ${CMAKE_SOURCE_DIR}/hd2_glue.cpp
    ${CMAKE_SOURCE_DIR}/hd2_stubs.c
    ${CMAKE_SOURCE_DIR}/hd2_fm_broadcast.cpp
    ${ORTX_CORE} ${ORTX_UI} ${ORTX_HD2} ${ORTX_NVM} ${ORTX_RADIO} ${ORTX_STUBS}
    ${ORTX_VP} ${ORTX_LIBS} ${ORTX_PROTOCOLS})

target_include_directories(openrtx_hd2 PRIVATE
    ${CMAKE_SOURCE_DIR}                                 # fork: app-tree headers
    ${MIOSIX_KPATH}/arch/chip/hr_c7000                 # hd2_trace.h (kernel trace facility)
    ${ORTX}/openrtx/include
    ${ORTX}/platform
    ${ORTX}/platform/targets/HD2
    ${ORTX}/platform/mcu/CSKY_V2
    ${ORTX}/lib/minmea/include
    ${ORTX}/lib/QRCode/include
    ${ORTX}/lib/qdec/include)

target_compile_definitions(openrtx_hd2 PRIVATE
    PLATFORM_HD2 _MIOSIX FONT_UBUNTU_REGULAR timegm=mktime
    CONFIG_DMR
    "GIT_VERSION=\"openrtx-hd2-miosix\"")
# WITH_HD2_TRACE is set globally above (add_compile_definitions) so it reaches
# the kernel lib and the app uniformly; the kernel board_settings.h keeps it off.

# Optional debug: echo raw GPS NMEA from UART2 to the console UART0 (visible in
# the loader/diag bridge log).  Enable with:  cmake -DHD2_GPS_DEBUG=ON .
option(HD2_GPS_DEBUG "Echo raw GPS NMEA to the console UART" OFF)
if(HD2_GPS_DEBUG)
    target_compile_definitions(openrtx_hd2 PRIVATE HD2_GPS_DEBUG=1)
endif()

# DMR TX live RF.  OFF by default: the DMR op-mode is a scaffold (the software
# AMBE vocoder + the per-slot pump are still being brought up), so a default
# build configures the modem/AT1846S for DMR but never keys a carrier.  Turn
# ON only for hardware bring-up:  cmake -DHD2_DMR_TX_LIVE=ON .
option(HD2_DMR_TX_LIVE "Enable live DMR TX keying (PA + slot engine + TS_TX ISR)" OFF)
if(HD2_DMR_TX_LIVE)
    target_compile_definitions(openrtx_hd2 PRIVATE HD2_DMR_TX_LIVE=1)
endif()

# DMR voice: the device-resident software AMBE+2 codec, copied to its native
# VMA at runtime and driven by hd2_ambe.c.  OFF by default and NOT in the repo:
# the AMBE blob is DVSI-patented, so it is carved from the user's own firmware
# at build time (scripts/ambe_carve.py -> build-ambe/, gitignored) and the .S
# .incbin's it from there.  Enable for voice bring-up:
#   python3 scripts/ambe_carve.py && cmake -DHD2_DMR_VOICE=ON .
option(HD2_DMR_VOICE "Link the carved AMBE+2 voice codec (needs build-ambe/)" OFF)
if(HD2_DMR_VOICE)
    set(AMBE_BLOB_DIR ${CMAKE_SOURCE_DIR}/../../../../../build-ambe)
    if(NOT EXISTS ${AMBE_BLOB_DIR}/ambe_lo.bin)
        message(FATAL_ERROR
            "HD2_DMR_VOICE=ON but ${AMBE_BLOB_DIR}/ambe_lo.bin is missing -- "
            "run: python3 scripts/ambe_carve.py")
    endif()
    target_sources(openrtx_hd2 PRIVATE
        ${ORTX}/platform/targets/HD2/hd2_ambe.c
        ${ORTX}/platform/targets/HD2/ambe_blob_HD2.S)
    # GAS resolves .incbin "ambe_lo.bin" via -I (blob lives in build-ambe/).
    set_source_files_properties(${ORTX}/platform/targets/HD2/ambe_blob_HD2.S
        PROPERTIES COMPILE_OPTIONS "-Wa,-I${AMBE_BLOB_DIR}")
    target_compile_definitions(openrtx_hd2 PRIVATE HD2_DMR_VOICE=1)
endif()

# M17 digital voice -- Phase 1.  Links M17-Project's Codec2-mod fork (vendor/
# Codec2-mod): a minimal, 3200-only, bit-exact refactor of Codec2 that is ~2.25x
# faster than reference (their STM32F4 benchmark) and uses STATIC, malloc-free
# fixed-size state (codec2_t) -- so it avoids the codec2_create heap FAULT the
# stock codec2 hit.  BUT the ~12 KB static codec2_t lands in .bss and shrinks the
# heap below the boot threshold (>~12 KB static hangs the UI splash -- memory
# hd2-memory-constraints), so this build STILL needs the low-RAM-origin script:
#   cmake -DHD2_M17=ON -DMIOSIX_LINKER_SCRIPT=hd2_m17.ld .
# (valid only with HD2_DMR_VOICE=OFF -- it reuses the AMBE window).  Still float,
# so on the FPU-less soft-float CK803 it is NOT real-time (measured via diag op
# 'i'); this build gives the best achievable non-fixed-point number.
# -fsingle-precision-constant stops float literals from promoting whole
# expressions to (much slower) double soft-float -- a free win the upstream STM32
# build uses but the earlier HD2 build had dropped.
# M17 digital VOICE TX -- the full on-device chain: our fixed-point Codec2
# (C2_FIXED, 15 ms/frame) -> OpenRTX M17 FrameEncoder + Modulator -> a PLATFORM_HD2
# baseband tap that decimates 48 kHz -> 8 kHz -> the FM-TX-RAM ping-pong pump
# (the proven HD2_M17_TXTEST keying path).  Exposed as diag op 'g'.  Implies
# HD2_M17 (pulls in the codec2 lib) and, like it, needs the low-RAM linker script
# with DMR voice OFF:
#   cmake -DHD2_M17_VOICE=ON -DMIOSIX_LINKER_SCRIPT=hd2_m17.ld .
option(HD2_M17_VOICE "Full on-device M17 voice TX (codec2 -> M17 -> FM-TX-RAM pump, diag op 'g')" OFF)
if(HD2_M17_VOICE)
    # Normal variable set before the option() below -> with CMP0077 NEW the
    # option() is a no-op, so HD2_M17 stays ON and the codec2 lib links in.
    set(HD2_M17 ON)
endif()

option(HD2_M17 "Link Codec2-mod (M17 3200 fork) + M17 bring-up self-test" OFF)
if(HD2_M17)
    set(C2M ${ORTX}/../Codec2-mod)   # sibling of OpenRTX; C2_FIXED = real fixed-point path
    add_library(codec2_hd2 STATIC
        ${C2M}/src/analysis.c   ${C2M}/src/codec2_mod.c ${C2M}/src/delta_lsp_cb.c
        ${C2M}/src/interp.c     ${C2M}/src/kiss_fft.c   ${C2M}/src/kiss_fftr.c
        ${C2M}/src/lpc.c        ${C2M}/src/nlp.c        ${C2M}/src/quantise.c
        ${C2M}/src/synthesis.c  ${C2M}/src/util.c
        ${C2M}/src/fft_fixed.c)  # int32 fixed-point FFT bridge
    # Keep codec2 at the build's -O2 for an accurate encode-speed benchmark, but
    # build the rest of the app -Os to fit flash (the BFP analysis grew .text).
    target_compile_options(openrtx_hd2 PRIVATE -Os)
    target_include_directories(codec2_hd2 PRIVATE ${C2M}/inc)
    target_compile_options(codec2_hd2 PRIVATE
        -w -ffunction-sections -fdata-sections
        -fsingle-precision-constant -fno-math-errno)
    target_compile_definitions(codec2_hd2 PRIVATE C2_FIXED)
    target_link_libraries(openrtx_hd2 PRIVATE codec2_hd2)
    target_include_directories(openrtx_hd2 PRIVATE ${C2M}/inc)
    target_compile_definitions(openrtx_hd2 PRIVATE HD2_M17=1)
endif()

if(HD2_M17_VOICE)
    # OpenRTX M17 stack (hardware-agnostic).  ConvolutionalEncoder / Interleaver /
    # CodePuncturing / Decorrelator / StreamFrame / PacketFrame are header-only;
    # crc.c (crc_m17) is already compiled in ORTX_CORE.  Modulator.cpp's
    # PLATFORM_HD2 branch decimates 48 kHz -> 8 kHz into the diag op's sink.
    set(ORTX_M17
        ${ORTX}/openrtx/src/protocols/M17/FrameEncoder.cpp
        ${ORTX}/openrtx/src/protocols/M17/LinkSetupFrame.cpp
        ${ORTX}/openrtx/src/protocols/M17/Callsign.cpp
        ${ORTX}/openrtx/src/protocols/M17/Golay.cpp
        ${ORTX}/openrtx/src/protocols/M17/Modulator.cpp
        ${ORTX}/openrtx/src/protocols/M17/DSP.cpp)
    target_sources(openrtx_hd2 PRIVATE ${ORTX_M17})
    # CONFIG_M17 is scoped to the M17 sources ONLY.  DSP.cpp gates the rrc_48k FIR
    # behind it, but ui*.c / rtx.cpp also test CONFIG_M17 (M17 op-mode menu +
    # dispatch) and OpMode_M17 is NOT linked here -- a global define would drag in
    # an unlinked op-mode.  hd2_diag.cpp keys on HD2_M17_VOICE instead.
    set_source_files_properties(${ORTX_M17}
        PROPERTIES COMPILE_DEFINITIONS "CONFIG_M17")
    target_compile_definitions(openrtx_hd2 PRIVATE HD2_M17_VOICE=1)
endif()

# M17 packet-mode physical-layer bring-up (no vocoder, no codec2).  Embeds a
# host-generated M17 baseband test frame (m17_frame.raw) and adds diag op 'f' to
# play it on loop through the FM-TX-RAM pump, so an external m17-demod can lock
# on our over-the-air LSF and confirm the TX physical layer (sync + RRC +
# deviation).  Independent of HD2_M17/codec2.  cmake -DHD2_M17_TXTEST=ON .
option(HD2_M17_TXTEST "Embed an M17 baseband frame + diag op 'f' to TX it (packet PHY bring-up)" OFF)
if(HD2_M17_TXTEST)
    target_sources(openrtx_hd2 PRIVATE ${ORTX}/platform/targets/HD2/m17_blob_HD2.S)
    set_source_files_properties(${ORTX}/platform/targets/HD2/m17_blob_HD2.S
        PROPERTIES COMPILE_OPTIONS "-Wa,-I${ORTX}/platform/targets/HD2")
    target_compile_definitions(openrtx_hd2 PRIVATE HD2_M17_TXTEST=1)
endif()

# Radio-bus (AT1846S/RDA5802E) transport: the HW DesignWare I2C1 controller
# (i2c_csky.c).  Full Test-A checklist HW-verified -- RX, TX, PTT, codeplug, FM
# broadcast incl. the RDA5802E 64-byte init burst (2026-06-13).  The historical
# "wedge" was platform_init stealing the pad mux (docs/TASK_hw_i2c_migration.md);
# the post-boot hard lock was the missing no-PendSV drain in
# PKrestartKernelAndWaitImpl (same doc).  The old software bit-bang transport was
# removed once HW I2C1 proved reliable.

# LCD transport: default = the HW i8080 controller @0x12000000 (live-verified
# 2026-06-13, red-stripe test; ~6 AHB cycles/byte vs hundreds for the GPIO
# latch).  Keypad scans transiently re-mux the shared PTC pins (keyboard_HD2.c).
# Fallback: cmake -DHD2_LCD_BITBANG=ON .
option(HD2_LCD_BITBANG "Use the GPIO-latch bit-bang for the LCD (fallback; default = HW i8080)" OFF)
if(HD2_LCD_BITBANG)
    target_compile_definitions(openrtx_hd2 PRIVATE HD2_LCD_BITBANG=1)
endif()

# W25Q flash transport: default = HW SPI0 @0x140a0000 (live-verified
# 2026-06-13, JEDEC ef4020 over the controller; full-duplex mode -- the
# custom variant's EEPROM-read TMOD is broken).  CS stays GPIO-owned.
# Fallback: cmake -DHD2_SPI_BITBANG=ON .
option(HD2_SPI_BITBANG "Use software bit-bang SPI for the W25Q flash (fallback; default = HW SPI0)" OFF)
if(HD2_SPI_BITBANG)
    target_compile_definitions(openrtx_hd2 PRIVATE HD2_SPI_BITBANG=1)
endif()

# Isolation aid: stub the vendor-codeplug readers (cps_read* return -1 -> core
# uses the default codeplug, no W25Q channel enumeration at boot).  Used to
# isolate whether boot stalls / lockups come from the codeplug reader's W25Q
# traffic vs the radio-bus transport.  cmake -DHD2_CODEPLUG_STUB=ON .
option(HD2_CODEPLUG_STUB "Stub the vendor codeplug readers (no W25Q enumeration)" OFF)
if(HD2_CODEPLUG_STUB)
    target_compile_definitions(openrtx_hd2 PRIVATE HD2_CODEPLUG_STUB=1)
endif()

miosix_link_target(openrtx_hd2)

# Always refresh the build-time version stamp. hd2_diag.cpp bakes the version
# string from __DATE__/__TIME__, which the compiler only re-evaluates when that
# TU is recompiled — so when only other files change, the 'P' probe reports a
# stale build time and can't be trusted as a "did my flash take" signal. Touch
# the source before every build so its object is always rebuilt with a fresh
# timestamp. Cheap (one small TU) and keeps the reported version honest.
add_custom_target(hd2_refresh_version ALL
    COMMAND ${CMAKE_COMMAND} -E touch ${ORTX}/platform/targets/HD2/hd2_diag.cpp
    COMMENT "Refreshing version timestamp (touch hd2_diag.cpp)")
add_dependencies(openrtx_hd2 hd2_refresh_version)
