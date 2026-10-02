/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/*
 * M17 baseband replay tool. See M17Replay.hpp for what is counted.
 *
 * Build:   meson compile -C build_linux m17_replay_tool
 * Run:     ./build_linux/m17_replay_tool [-r 24000|48000] [-i] [-v] file.raw
 *
 * Recording with an RTL-SDR near the transmitter (find the ppm offset with
 * `rtl_test -p` and pick a gain that does not clip):
 *
 *     rtl_fm -M fm -f <Hz> -s 24k -g <gain> -p <ppm> -F 9 - > recording.raw
 *
 * Use no -i for these. A baseband dump taken on an MD-UV3x0 needs -i, and
 * modulator output such as m17-cpp-mod is usually 48 kHz (-r 48000).
 *
 * To compare two receiver revisions, build this tool from each revision in
 * its own build directory and run both on the same recording. For packets
 * compare packetsOk and packetsCrc, for streams streamFrames and
 * streamMissed. Diff the -v outputs to find the transmissions whose outcome
 * changed and cut them out as a small reproducer, for example the four
 * seconds starting at 300 s of a 24 kHz file:
 *
 *     dd if=recording.raw of=clip.raw bs=2 \
 *        skip=$((300 * 24000)) count=$((4 * 24000))
 *
 * Keep about three seconds of audio before the transmission of interest.
 * The demodulator carries level, timing and lock state across transmissions,
 * and a failure seen in a long recording often disappears when the
 * transmission is replayed on its own from a cold start.
 */

#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#include "M17Replay.hpp"

static void printHelp()
{
    puts("Replay a recorded M17 baseband through the receive chain.");
    puts("Samples must be signed 16-bit little endian, mono.");
    puts("Usage: m17_replay_tool [OPTIONS] recording.raw");
    puts("Options:");
    puts("-r RATE\t Sample rate of the recording, 24000 (default) or 48000");
    puts("-i\t Invert the baseband polarity");
    puts("-v\t Print time-stamped lock, LSF and packet events");
}

int main(int argc, char *argv[])
{
    uint32_t rate = 24000;
    bool invert = false;
    bool verbose = false;

    while (1) {
        int opt = getopt(argc, argv, "r:ivh");
        if (opt == -1)
            break;

        switch (opt) {
            case 'r':
                rate = atoi(optarg);
                break;
            case 'i':
                invert = true;
                break;
            case 'v':
                verbose = true;
                break;
            case 'h':
                printHelp();
                return 0;
            default:
                printHelp();
                return -1;
        }
    }

    if (optind >= argc) {
        puts("Error: no recording given");
        printHelp();
        return -1;
    }

    if ((rate != 24000) && (rate != 48000)) {
        puts("Error: sample rate must be 24000 or 48000");
        return -1;
    }

    const char *path = argv[optind];
    M17Replay replay(invert, verbose);
    if (!replay.replay(path, rate)) {
        perror(path);
        return -1;
    }

    const M17Replay::Counts &c = replay.counts();
    if (verbose)
        puts("");
    printf("%s: rate=%u invert=%d\n", path, rate, invert);
    printf("  locks            %u\n", c.locks);
    printf("  lsfValid         %u\n", c.lsfValid);
    printf("  lsfInvalid       %u\n", c.lsfInvalid);
    printf("  streamFrames     %u\n", c.streamFrames);
    printf("  streamMissed     %u\n", c.streamMissed);
    printf("  streamEnds       %u\n", c.streamEnds);
    printf("  packetFrames     %u\n", c.packetFrames);
    printf("  packetsOk        %u\n", c.packetsOk);
    printf("  packetsCrc       %u\n", c.packetsCrc);
    printf("  packetsAborted   %u\n", c.packetsAborted);

    return 0;
}
