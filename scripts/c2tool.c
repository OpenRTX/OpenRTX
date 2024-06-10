/***************************************************************************
 *   Copyright (C) 2024 by Silvano Seva IU2KWO                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <codec2/codec2.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>


#define AUDIO_SIZE 160
#define DATA_SIZE  8


static void doEncode(FILE *input, FILE *output, struct CODEC2 *codec2)
{
    int16_t audioIn[AUDIO_SIZE];
    uint8_t dataOut[DATA_SIZE];

    while(feof(input) == 0)
    {
        fread(&audioIn, sizeof(int16_t), AUDIO_SIZE, input);
        codec2_encode(codec2, dataOut, audioIn);
        fwrite(dataOut, sizeof(uint8_t), DATA_SIZE, output);
    }
}

static void doDecode(FILE *input, FILE *output, struct CODEC2 *codec2)
{
    uint8_t dataIn[DATA_SIZE];
    int16_t audioOut[AUDIO_SIZE];

    while(feof(input) == 0)
    {
        fread(dataIn, sizeof(uint8_t), DATA_SIZE, input);
        codec2_decode(codec2, audioOut, dataIn);
        fwrite(audioOut, sizeof(int16_t), AUDIO_SIZE, output);
    }
}

static void printHelp()
{
    puts("Simple codec2 encode/decode tool, for 3200 data rate only.");
    puts("Audio data must be signed 16-bit little endian sampled at 8kHz");
    puts("Usage: c2tool [OPTIONS]...");
    puts("Options:");
    puts("-e\t Encode audio");
    puts("-d\t Decode codec2 data");
    puts("-i\t Input file");
    puts("-o\t Output file");
}


int main(int argc, char *argv[])
{
    char ifName[512];
    char ofName[512];
    bool encode = false;
    bool decode = false;

    if(argc <= 2)
    {
        printHelp();
        return 0;
    }
    
    while(1)
    {
        int opt = getopt(argc, argv, "edi:o:");
        if(opt == -1)
            break;

        switch(opt)
        {
            case 'e':
                encode = true;
                break;

            case 'd':
                decode = true;
                break;

            case 'i':
                if(strlen(optarg) >= sizeof(ifName))
                {
                    puts("Error: input file name is too long!");
                    return -1;
                }

                strncpy(ifName, optarg, sizeof(ifName));
                break;

            case 'o':
                if(strlen(optarg) >= sizeof(ofName))
                {
                    puts("Error: output file name is too long!");
                    return -1;
                }

                strncpy(ofName, optarg, sizeof(ofName));
                break;

            default:
                printHelp();
                return 0;
                break;
        }
    }

    if(encode && decode)
    {
        puts("Error: encode and decode selected at the same time!");
        return -1;
    }

    FILE *infile  = fopen(ifName, "rb");
    if(infile == NULL)
    {
        puts("Error opening input file!");
        return -1;
    }

    FILE *outfile = fopen(ofName, "ab");
    if(outfile == NULL)
    {
        fclose(infile);
        puts("Error opening output file!");
        return -1;
    }

    struct CODEC2 *codec2 = codec2_create(CODEC2_MODE_3200);
    if(codec2 == NULL)
    {
        fclose(infile);
        fclose(outfile);
        puts("Error creating codec2 instance!");
        return -1;
    }

    if(encode)
        doEncode(infile, outfile, codec2);

    if(decode)
        doDecode(infile, outfile, codec2);

    fflush(outfile);

    fclose(infile);
    fclose(outfile);
    codec2_destroy(codec2);

    return 0;
}
