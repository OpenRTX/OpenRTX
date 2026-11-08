<!--
SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
SPDX-License-Identifier: GPL-3.0-or-later
-->

# Horse / fork test results

Snapshot of automated checks for this fork. Update when re-running CI or
`scripts/ci_local.sh`. RF and on-radio behaviour remain **untested on
hardware** unless a row says otherwise.

## GitHub Actions (fork tip `cda59c1a`)

Branch: `master` / `upstream-sync` (same tip).

| Workflow | Run | Result |
|----------|-----|--------|
| Build and test | https://github.com/Supermagnum/OpenRTX/actions/runs/42446884239 | pass |
| Format sources (prior tip `2f41a2f0`) | https://github.com/Supermagnum/OpenRTX/actions/runs/42445660175 | pass |

| Job | Result |
|-----|--------|
| Format sources (clang-format, strings, ruff) | pass |
| unit-test (meson + Horse OpenPGP + provision socket) | pass |
| byte-identical (`openrtx_mduv3x0`, `openrtx_gd77` vs upstream master) | pass |
| build (devcontainer firmware matrix) | pass |
| build-zephyr | pass |
| upload-firmware (per radio) | pass |

Earlier green tip on the upstream PR head (pre-format fix): Build and test
https://github.com/Supermagnum/OpenRTX/actions/runs/42444832508 (`de35aa76`,
OpenPGP extract hardening). Format sources then failed on a one-line
`printf` wrap in `horse_host_interop.cpp` and was fixed in `2f41a2f0`.

Cross-repo PR OpenRTX/OpenRTX#523 historically used tip `f2252090` /
run https://github.com/OpenRTX/OpenRTX/actions/runs/42440337706; later
merged to fork `master` as `b30805cb` and superseded by the tip above.

## Local `scripts/ci_local.sh` (host)

| Check | Result | Notes |
|-------|--------|-------|
| `clang_format.sh --check` | pass | tip `2f41a2f0` |
| `check_strings.py` | pass | |
| ruff (`scripts/horse_*.py`, `tests/python`) | pass | |
| REUSE lint | pass | |
| `cargo test` / `cargo build --release` (`tools/horse-openpgp`) | pass | set `HORSE_OPENPGP_HELPER` if `CARGO_TARGET_DIR` redirects |
| meson native unit + Horse tests | pass | includes `horse_provision_sock_test`, Horse pytest, host interop |
| cross-ARM byte-identical | skipped | no `arm-miosix-eabi` toolchain locally; covered in CI |

`HORSE_REQUIRE_CRYPTO=1` was set; OpenPGP tests ran (not skipped).

## Encrypt / decrypt

| Test | Scope | Local (`build_linux`) | CI (`cda59c1a`) |
|------|-------|------------------------|----------------|
| `Horse OpenPGP encrypt/decrypt` | gpg encrypt/decrypt of packed identity; store v3 roundtrip; CLI `openpgp-wrap` then unwrap/`show`; garbage ciphertext rejected | **pass** (4 pytest cases, 0.59s) | **pass** (0.50s) on [run 42446884239](https://github.com/Supermagnum/OpenRTX/actions/runs/42446884239) |
| `Horse Crypto Unit Test` | voice XChaCha20 encrypt/decrypt roundtrip; bad tag / bad ciphertext rejected; LSF key-bind | **pass** | **pass** (0.01s) on same run |

OpenPGP encrypt/decrypt is a dedicated meson test (also included in
`Horse Provision pytest`) so unit-test job logs show it by name.

## Coverage notes

- Horse host interop, provision socket, crypto, loopback, FEC v2 sims:
  covered by meson tests in the unit-test job. Host interop fails the
  test if OpenPGP extract fails when the helper is present.
- OpenPGP: throwaway `GNUPGHOME` pytest + Rust unit tests for Cv25519
  byte order; checked-in colon-listing fixtures for stub/card/offline
  primary (`tests/fixtures/openpgp/`). Helper uses StandardPolicy with
  skewed reference times and, under `--allow-unmarked`, NullPolicy
  fallback so freshly minted gpg keys do not flake CI. Full v6/sq
  cross-tool fixture matrix is still incomplete relative to the original
  follow-up prompt (see [HORSE_AUDIT.md](./HORSE_AUDIT.md)).
- ASan/UBSan: unsanitized suite is green. UBSan still hits upstream
  `dsp_dcBlockFilter` ([OpenRTX#561](https://github.com/OpenRTX/OpenRTX/issues/561)).

## Hardware

See [HARDWARE_TEST_PLAN.md](./HARDWARE_TEST_PLAN.md). No on-air or
metered deviation results are recorded here yet.
