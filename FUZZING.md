# Fuzzing Horse with libFuzzer

Horse frame and repeat-2 voice decoders can be built with **libFuzzer**.
This fork does not ship M17 or minmea fuzz targets.

## Requirements

- **Clang** (libFuzzer is part of compiler-rt). GCC rejects `-fsanitize=fuzzer`.
- Meson with `-Dfuzzing=true`.

```bash
export CC=clang
export CXX=clang++
meson setup build_fuzz -Dfuzzing=true
meson compile -C build_fuzz fuzz_horse_frame fuzz_horse_voice
```

| Target | Input | Code under test |
|--------|--------|------------------|
| `fuzz_horse_frame` | 48 bytes | Horse frame decoder (LSF, voice, EOT) |
| `fuzz_horse_voice` | 46 bytes | Horse option-C voice decoder |

Seeds: `tests/fuzz/corpus/<target>/`. Optional dict:
`tests/fuzz/dict/frame_sync_words.dict`.

```bash
python3 scripts/gen_fuzz_corpus.py
./build_fuzz/fuzz_horse_frame tests/fuzz/corpus/fuzz_horse_frame fuzz_artifacts/fuzz_horse_frame
./scripts/run_all_fuzzers_3h.sh build_fuzz
./scripts/run_fuzz_smoke_test.sh build_fuzz 20
```

Do not run `ninja -C <dir>` with no target if that directory also
builds Miosix firmware and the ARM toolchain is missing.
