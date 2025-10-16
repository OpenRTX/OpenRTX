/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// Test private methods
#define private public

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "protocols/M17/M17DSP.h"
#include "protocols/M17/M17Demodulator.h"
#include "protocols/M17/M17Utils.h"
#include "core/audio_stream.h"

using namespace std;

/**
 * Test the different demodulation steps
 */

int main()
{
    // Open file
    FILE *baseband_file = fopen("../tests/unit/assets/M17_test_baseband_dc.raw", "rb");
    FILE *baseband_out = fopen("M17_test_baseband_rrc.raw", "wb");
    if (!baseband_file)
    {
        perror("Error in reading test baseband");
        return -1;
    }

    // Get file size
    fseek(baseband_file, 0L, SEEK_END);
    long baseband_size = ftell(baseband_file);
    unsigned baseband_samples = baseband_size / 2;
    fseek(baseband_file, 0L, SEEK_SET);
    printf("Baseband is %ld bytes!\n", baseband_size);

    // Read data from input file
    int16_t *baseband_buffer = (int16_t *) malloc(baseband_size);
    if (!baseband_buffer)
    {
        perror("Error in memory allocation");
        return -1;
    }
    size_t read_items = fread(baseband_buffer, 2, baseband_samples, baseband_file);
    if (read_items != baseband_samples)
    {
        perror("Error in reading input file");
        return -1;
    }
    fclose(baseband_file);

    // Apply RRC on the baseband buffer
    int16_t *filtered_buffer = (int16_t *) malloc(baseband_size);
    for(size_t i = 0; i < baseband_samples; i++)
    {
        float elem = static_cast< float >(baseband_buffer[i]);
        filtered_buffer[i] = static_cast< int16_t >(M17::rrc(elem));
    }
    fwrite(filtered_buffer, baseband_samples, 2, baseband_out);
    fclose(baseband_out);

    M17::M17Demodulator m17Demodulator = M17::M17Demodulator();
    m17Demodulator.init();
    dataBlock_t baseband = { nullptr, 0 };
    baseband.data = filtered_buffer;
    baseband.len = baseband_samples;
    dataBlock_t old_baseband = m17Demodulator.baseband;
    m17Demodulator.baseband = baseband;

    FILE *output_csv_1 = fopen("M17_demodulator_output_1.csv", "w");
    fprintf(output_csv_1, "Input,RRCSignal,LSFConvolution,FrameConvolution,Stddev\n");
    // Test convolution
    m17Demodulator.resetCorrelationStats();
    for(unsigned i = 0; i < baseband_samples - m17Demodulator.M17_SYNCWORD_SYMBOLS * m17Demodulator.M17_SAMPLES_PER_SYMBOL; i++)
    {
        int32_t lsf_conv =
            m17Demodulator.convolution(i,
                                       m17Demodulator.lsf_syncword,
                                       m17Demodulator.M17_SYNCWORD_SYMBOLS);
        int32_t stream_conv =
            m17Demodulator.convolution(i,
                                       m17Demodulator.stream_syncword,
                                       m17Demodulator.M17_SYNCWORD_SYMBOLS);
        m17Demodulator.updateCorrelationStats(stream_conv);
        fprintf(output_csv_1, "%" PRId16 ",%" PRId16 ",%d,%d,%f\n",
                baseband_buffer[i],
                baseband.data[i],
                lsf_conv + (int32_t) m17Demodulator.getCorrelationEma(),
                stream_conv - (int32_t) m17Demodulator.getCorrelationEma(),
                m17Demodulator.conv_threshold_factor *
                m17Demodulator.getCorrelationStddev());
    }
    fclose(output_csv_1);

    // Test syncword detection
    printf("Testing syncword detection!\n");
    FILE *syncword_ref = fopen("../tests/unit/assets/M17_test_baseband_dc_syncwords.txt", "r");
    int32_t offset = 0;
    M17::sync_t syncword = { -1, false };
    m17Demodulator.resetCorrelationStats();
    int i = 0;
    do
    {
        int expected_syncword = 0;
        fscanf(syncword_ref, "%d\n", &expected_syncword);
        syncword = m17Demodulator.nextFrameSync(offset);
        offset = syncword.index + m17Demodulator.M17_SYNCWORD_SYMBOLS * m17Demodulator.M17_SAMPLES_PER_SYMBOL;
        printf("%d\n", syncword.index);
        if (syncword.index != expected_syncword)
        {
            fprintf(stderr, "Error in syncwords detection #%d!\n", i);
            return -1;
        }
        else
        {
            printf("SYNC: %d...OK!\n", syncword.index);
        }
        i++;
    } while (syncword.index != -1);
    fclose(syncword_ref);

    FILE *output_csv_2 = fopen("M17_demodulator_output_2.csv", "w");
    fprintf(output_csv_2, "RRCSignal,SyncDetect,QntMax,QntMin,Symbol\n");
    uint32_t detect = 0, symbol = 0;
    offset = 0;
    syncword = { -1, false };
    m17Demodulator.resetCorrelationStats();
    syncword = m17Demodulator.nextFrameSync(offset);
    for(unsigned i = 0; i < baseband_samples - m17Demodulator.M17_SYNCWORD_SYMBOLS * m17Demodulator.M17_SAMPLES_PER_SYMBOL; i++)
    {
        if ((int) i == (syncword.index + 1)) {
            if (syncword.lsf)
                detect = -4000;
            else
                detect = 4000;
            syncword = m17Demodulator.nextFrameSync(syncword.index + m17Demodulator.M17_SYNCWORD_SYMBOLS * m17Demodulator.M17_SAMPLES_PER_SYMBOL);
        } else if (((int) i % 10) == ((syncword.index + 1) % 10)) {
            m17Demodulator.updateQuantizationStats(i);
            symbol = m17Demodulator.quantize(i) * 1000;
            detect = 3000;
        } else
        {
            detect = 0;
            symbol = 0;
        }
        fprintf(output_csv_2, "%" PRId16 ",%d,%f,%f,%d\n",
                m17Demodulator.baseband.data[i] - (int16_t) m17Demodulator.getQuantizationEma(),
                detect,
                m17Demodulator.getQuantizationMax() / 2,
                m17Demodulator.getQuantizationMin() / 2,
                symbol);
    }
    fclose(output_csv_2);

    // TODO: Test symbol quantization
    FILE *symbols_ref = fopen("../tests/unit/assets/M17_test_baseband.bin", "rb");
    // Skip preamble
    fseek(symbols_ref, 0x30, SEEK_SET);
    uint32_t failed_bytes = 0, total_bytes = 0;
    syncword = { -1, false };
    offset = 0;
    syncword = m17Demodulator.nextFrameSync(offset);
    std::array< uint8_t, m17Demodulator.M17_FRAME_BYTES > frame;
    // Preheat quantizer
    for(int i = 0; i < 10; i++)
        m17Demodulator.updateQuantizationStats(i);
    if (syncword.index != -1)
    {
        // Next syncword does not overlap with current syncword
        offset = syncword.index + m17Demodulator.M17_SAMPLES_PER_SYMBOL;
        // Slice the input buffer to extract a frame and quantize
        for(uint32_t j = 0;
            syncword.index + 2 + m17Demodulator.M17_SAMPLES_PER_SYMBOL * (j + i) < m17Demodulator.baseband.len;
            j+= m17Demodulator.M17_FRAME_SYMBOLS)
        {
            for(uint16_t i = 0; i < m17Demodulator.M17_FRAME_SYMBOLS; i++)
            {
                // Quantize
                uint32_t symbol_index = syncword.index + 2 +
                                        m17Demodulator.M17_SAMPLES_PER_SYMBOL * (j + i);
                m17Demodulator.updateQuantizationStats(symbol_index);
                int8_t symbol = m17Demodulator.quantize(symbol_index);
                setSymbol<m17Demodulator.M17_FRAME_BYTES>(frame, i, symbol);
            }
            for(uint16_t i = 0; i < m17Demodulator.M17_FRAME_BYTES; i+=2) {
                if (i % 16 == 0)
                    printf("\n");
                printf(" %02X%02X", frame[i], frame[i+1]);
                // Check with reference bitstream
                //uint8_t ref_byte = 0x00;
                //fread(&ref_byte, 1, 1, symbols_ref);
                //if (frame[i] != ref_byte)
                //{
                //    printf("Mismatch byte #%u!\n", i);
                //    failed_bytes++;
                //}
                //fread(&ref_byte, 1, 1, symbols_ref);
                //if (frame[i+1] != ref_byte)
                //{
                //    printf("Mismatch byte #%u!\n", i);
                //    failed_bytes++;
                //}
                total_bytes += 1;
            }
            printf("\n");
        }
    }
    printf("Failed decoding %d/%d bytes!\n", failed_bytes, total_bytes);

    // TODO: when stream is over pad with zeroes to avoid corrupting the last symbols

    m17Demodulator.baseband = old_baseband;
    free(baseband_buffer);
    free(filtered_buffer);
    return 0;
}
