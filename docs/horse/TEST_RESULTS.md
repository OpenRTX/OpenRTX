<!--
SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
SPDX-License-Identifier: GPL-3.0-or-later
-->

# Horse / fork test results

Snapshot of automated checks for this fork. Update when re-running CI or
`scripts/ci_local.sh`. RF and on-radio behaviour remain **untested on
hardware** unless a row says otherwise.

## GitHub Actions (OpenRTX/OpenRTX#523, tip `f2252090`)

Run: https://github.com/OpenRTX/OpenRTX/actions/runs/42440337706  
(Triggered from cross-repo PR head `Supermagnum/OpenRTX` branch
`upstream-sync`; later merged to fork `master` as `b30805cb`.)

| Job | Result |
|-----|--------|
| Format sources (clang-format, strings, ruff) | pass |
| unit-test (meson + Horse OpenPGP + provision socket) | pass |
| byte-identical (`openrtx_mduv3x0`, `openrtx_gd77` vs upstream master) | pass |
| build (devcontainer firmware matrix) | pass |
| build-zephyr | pass |
| upload-firmware (per radio) | pass |

## Local `scripts/ci_local.sh` (host)

Last full local run on the development machine before the merge:

| Check | Result | Notes |
|-------|--------|-------|
| `clang_format.sh --check` | pass | |
| `check_strings.py` | pass | |
| ruff (`scripts/horse_*.py`, `tests/python`) | pass | |
| REUSE lint | pass | |
| `cargo test` / `cargo build --release` (`tools/horse-openpgp`) | pass | |
| meson native unit + Horse tests | pass | includes `horse_provision_sock_test`, Horse pytest |
| cross-ARM byte-identical | skipped | no `arm-miosix-eabi` toolchain locally; covered in CI |

`HORSE_REQUIRE_CRYPTO=1` was set; OpenPGP tests ran (not skipped).

## Coverage notes

- Horse host interop, provision socket, crypto, loopback, FEC v2 sims:
  covered by meson tests in the unit-test job.
- OpenPGP: throwaway `GNUPGHOME` pytest + Rust unit tests for Cv25519
  byte order; checked-in colon-listing fixtures for stub/card/offline
  primary. Full v6/sq cross-tool fixture matrix is still incomplete
  relative to the follow-up prompt (see [HORSE_AUDIT.md](./HORSE_AUDIT.md)).
- ASan/UBSan: unsanitized suite is green. UBSan still hits upstream
  `dsp_dcBlockFilter` ([OpenRTX#561](https://github.com/OpenRTX/OpenRTX/issues/561)).

## Hardware

See [HARDWARE_TEST_PLAN.md](./HARDWARE_TEST_PLAN.md). No on-air or
metered deviation results are recorded here yet.
