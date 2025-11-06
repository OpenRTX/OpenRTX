/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2020-2025 OpenRTX Contributors
 *
 * This file is part of OpenRTX.
 */

#ifndef FILE_SOURCE_H
#define FILE_SOURCE_H

#include "interfaces/audio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Driver providing an audio input stream from a file. File format should be
 * raw, 16 bit, little endian. The configuration parameter is the file name with
 * the full path.
 */

extern const struct audioDriver file_source_audio_driver;


#ifdef __cplusplus
}
#endif

#endif /* STM32_DAC_H */
