#!/bin/bash
# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Prebuilt fallback for Horse on Cortex-M4. Prefer the meson wrap
# subprojects/libsodium.wrap (libsodium 1.0.20,
# sha256 ebb65ef6ca439333c2bb41a0c1990587288da07f6c7fd07cb3a18cc18d30ce19).
#
# This script records the Autotools flags used by that wrap. Do not use
# --enable-minimal: it omits crypto_stream_xchacha20. Unused objects are
# dropped by firmware --gc-sections. Do not enable a software CSPRNG.

set -euo pipefail

VERSION=1.0.20
HASH=ebb65ef6ca439333c2bb41a0c1990587288da07f6c7fd07cb3a18cc18d30ce19
SRC="${HOME}/.local/src"
MIOSIX="${HOME}/.local/src/miosix-extract"
HOST=arm-miosix-eabi
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PREFIX="${1:-${ROOT}/subprojects/libsodium-cm4}"

export PATH="${MIOSIX}/bin:${PATH}"
if [[ ! -x "${MIOSIX}/bin/${HOST}-gcc" ]]; then
    echo "Miosix toolchain not found under ${MIOSIX}" >&2
    exit 1
fi

TARBALL=""
for cand in \
    "${ROOT}/subprojects/packagecache/libsodium-${VERSION}.tar.gz" \
    "${SRC}/libsodium-${VERSION}.tar.gz" \
    "/tmp/libsodium-${VERSION}.tar.gz"
do
    if [[ -f "${cand}" ]]; then
        TARBALL="${cand}"
        break
    fi
done

mkdir -p "${SRC}"
if [[ -z "${TARBALL}" ]]; then
    TARBALL="${SRC}/libsodium-${VERSION}.tar.gz"
    curl -L -o "${TARBALL}" \
        "https://download.libsodium.org/libsodium/releases/libsodium-${VERSION}.tar.gz"
fi
echo "${HASH}  ${TARBALL}" | sha256sum -c -

BUILD=$(mktemp -d)
trap 'rm -rf "${BUILD}"' EXIT
tar -C "${BUILD}" -xzf "${TARBALL}"
cd "${BUILD}/libsodium-${VERSION}"

export CFLAGS="-Os -ffunction-sections -fdata-sections -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fstack-usage -Wstack-usage=16384 -DSODIUM_STATIC -DRANDOMBYTES_CUSTOM_IMPLEMENTATION -DRANDOMBYTES_DEFAULT_IMPLEMENTATION=NULL"
./configure --host="${HOST}" --prefix="${PREFIX}" \
    --disable-shared --enable-static --disable-ssp --disable-pie \
    --disable-asm --disable-blocking-random
make -j"$(nproc)"
make install
mkdir -p "${PREFIX}/stack-usage"
find . -name '*.su' -exec cp -t "${PREFIX}/stack-usage" {} +
echo "Installed ${PREFIX}/lib/libsodium.a"
