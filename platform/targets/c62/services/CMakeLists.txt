# Copyright (c) 2023 LISTENAI
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_LSF_AUDIO_SERVICE service_audio.c)
zephyr_library_link_libraries_ifdef(CONFIG_LSF_AUDIO_SERVICE af)

# 由于历史原因，这些头文件需要被暴露在外
if(CONFIG_LSF_AUDIO_SERVICE)
zephyr_include_directories(${ZEPHYR_LSF_MODULE_DIR}/lsf_client/include)
zephyr_include_directories(${ZEPHYR_AF_MODULE_DIR})
endif()

# 由于历史原因，这些头文件需要被暴露在外
if(CONFIG_LSF_AUDIO_SERVICE)
zephyr_include_directories(${ZEPHYR_LSF_MODULE_DIR}/lsf_client/include)
zephyr_include_directories(${ZEPHYR_AF_MODULE_DIR})
endif()

if(CONFIG_LSF_AUDIO_SERVICE AND CONFIG_CSK6_FLASH_STALL_SECOND_CORE)
message(FATAL_ERROR "To use audio service, CONFIG_CSK6_FLASH_STALL_SECOND_CORE should be explicitly disabled (=n)")
endif()

zephyr_library_sources_ifdef(CONFIG_LSF_THINKER_SERVICE service_thinker.c)
zephyr_library_sources_ifdef(CONFIG_LSF_WASM_SERVICE service_wasm.c)
zephyr_library_sources_ifdef(CONFIG_LSF_VAD_SERVICE service_vad.c)
