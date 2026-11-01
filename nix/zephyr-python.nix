# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Python environment for the Zephyr build (zephyr/scripts/requirements-base.txt
# at the revision pinned in west.yml), shared by the T-TWR Plus package and
# the Zephyr development shell.
{
  python3,
  extraPackages ? (_: [ ]),
}:
python3.withPackages (
  ps:
  (with ps; [
    west
    pyelftools
    pyyaml
    pykwalify
    packaging
    progress
    psutil
    pylink-square
    pyserial
    requests
    anytree
    intelhex
    canopen
  ])
  ++ extraPackages ps
)
