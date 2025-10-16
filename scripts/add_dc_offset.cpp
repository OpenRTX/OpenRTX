/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/*******************************************************************************
 *                                                                             *
 *  Convert a baseband from m17-cpp-mod to Module17/MD3x0 ADC input equivalent *
 *                                                                             *
 ******************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    FILE *baseband_in = fopen(argv[1], "rb");
    if (!baseband_in)
    {
        perror("Error in reading test baseband");
        return -1;
    }

    // Get file size
    fseek(baseband_in, 0L, SEEK_END);
    long baseband_size = ftell(baseband_in);
    unsigned baseband_samples = baseband_size / 2;
    fseek(baseband_in, 0L, SEEK_SET);
    printf("Baseband is %ld bytes!\n", baseband_size);

    // Read data from input file
    int16_t *baseband_buffer = (int16_t *) malloc(baseband_size);
    if (!baseband_buffer)
    {
        perror("Error in memory allocation");
        return -1;
    }
    size_t read_items = fread(baseband_buffer, 2, baseband_samples, baseband_in);
    if (read_items != baseband_samples)
    {
        perror("Error in reading input file");
        return -1;
    }
    fclose(baseband_in);

    // Add DC bias
    int16_t *filtered_buffer = (int16_t *) malloc(baseband_size);
    for(size_t i = 0; i < baseband_samples; i++)
    {
        float elem = static_cast< float >(baseband_buffer[i]);
        filtered_buffer[i] = static_cast< int16_t >((elem + 32768) / 16);
        printf("%d\n", filtered_buffer[i]);
    }

    // Write back
    FILE *baseband_out = fopen(argv[2], "wb");
    fwrite(filtered_buffer, baseband_samples, 2, baseband_out);
    fclose(baseband_out);


}
