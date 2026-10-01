/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/*
 * M17 baseband replay tool. See m17_replay.hpp for what is measured.
 *
 * Build:   meson compile -C build_linux m17_replay_tool
 * Run:     ./build_linux/m17_replay_tool [options] recording.raw
 * Options: --rate 24000|48000  input sample rate (48 kHz is decimated 2:1)
 *          --invert            invert the baseband polarity
 *          --verbose           time-stamped lock, LSF and packet events
 *
 * Recording with an RTL-SDR near the transmitter (find the ppm offset with
 * `rtl_test -p` and pick a gain that does not clip):
 *
 *     rtl_fm -M fm -f <Hz> -s 24k -g <gain> -p <ppm> -F 9 - > recording.raw
 *
 * Use no --invert for these. A baseband dump taken on an MD-UV3x0 needs
 * --invert, and modulator output such as m17-cpp-mod is usually 48 kHz.
 *
 * To compare two receiver revisions, build this tool from each revision in
 * its own build directory and run both on the same recording. For packets
 * compare packets_ok and packets_crc, for streams stream_frames and
 * stream_missed. Diff the --verbose outputs to find the transmissions whose
 * outcome changed and cut them out as a small reproducer, for example the
 * four seconds starting at 300 s of a 24 kHz file:
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
#include <string>

#include "m17_replay.hpp"

static void usage()
{
    std::fprintf(stderr, "usage: m17_replay_tool [--rate 24000|48000] "
                         "[--invert] [--verbose] recording.raw\n");
}

static bool parseArgs(int argc, char **argv, replay::Options &opt)
{
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if (a == "--invert")
            opt.invert = true;
        else if (a == "--verbose")
            opt.verbose = true;
        else if (a == "--rate" && i + 1 < argc)
            opt.rate = static_cast<uint32_t>(std::atol(argv[++i]));
        else if (a[0] == '-')
            return false;
        else
            opt.path = argv[i];
    }
    if (opt.rate != replay::SAMPLE_RATE && opt.rate != 2 * replay::SAMPLE_RATE)
        return false;
    return opt.path != nullptr;
}

int main(int argc, char **argv)
{
    replay::Options opt;
    if (!parseArgs(argc, argv, opt)) {
        usage();
        return 2;
    }

    replay::Counts count;
    if (!replay::run(opt, count))
        return 1;

    if (opt.verbose)
        std::printf("\n");
    std::printf("%s: rate=%u invert=%d\n", opt.path, opt.rate, opt.invert);
    for (const char *k : replay::KEYS)
        std::printf("  %-16s %u\n", k, count[k]);
    return 0;
}
