<!--
SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
SPDX-License-Identifier: GPL-3.0-or-later
-->

# OpenPGP throwaway fixtures

All keys and listings here are **throwaway test material**. Do not use
them outside automated tests. Secret material, when present, is labelled
and generated only for CI/pytest.

| File | Purpose |
|------|---------|
| `colon_ssb_local.txt` | gpg `--with-colons --with-secret` with local `+` subkeys |
| `colon_ssb_stub.txt` | subkey field 15 `#` (stub) |
| `colon_ssb_card.txt` | subkey field 15 card serial |
| `colon_offline_primary.txt` | primary `sec` stub/card, subkeys local `+` (must accept) |
