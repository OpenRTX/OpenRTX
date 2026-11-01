# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# LilyGO T-TWR Plus (ESP32-S3) firmware, built with Zephyr RTOS.
#
# Reproduces the CI job (.github/workflows/main.yml, build-zephyr) inside the
# sandbox:
#   * the three west projects of west.yml are fetched at their pinned SHAs
#     and turned into throw-away git repositories with a `manifest-rev`
#     branch, which west requires for every project it manages;
#   * the codec2 and XPowersLib wrap subprojects are populated because
#     CMakeLists.txt compiles their sources directly;
#   * the Zephyr SDK 0.17.0 provides the xtensa toolchain;
#   * `west build -b ttwrplus` and scripts/uf2conv.py produce the .uf2, as the
#     openrtx_ttwrplus_uf2 meson target does.
# The ESP32 binary blobs of hal_espressif are only linked with
# CONFIG_BT_ESP32/CONFIG_WIFI_ESP32, which this board does not enable.
{
  lib,
  stdenvNoCC,
  fetchgit,
  zephyr-sdk,
  zephyrPython,
  cmake,
  ninja,
  dtc,
  gperf,
  git,
  removeReferencesTo,
  subprojects,
  src,
  version,
  gitVersion,
}:

let
  # west.yml projects. zephyr's `import: submanifests` imports nothing at this
  # revision (the directory only holds a README and an example). west does
  # not fetch submodules, so neither do we (mcuboot has some).
  zephyrRev = "a4de2eb3d1756c445d2e2ecb72e6d562674c118b";
  westProjects = [
    {
      path = "zephyr";
      src = fetchgit {
        fetchSubmodules = false;
        url = "https://github.com/zephyrproject-rtos/zephyr";
        rev = zephyrRev;
        hash = "sha256-2glrzbtpPjHw5NUBSjEKz2Qhenklv3mTmaU4ATdbUIY=";
      };
    }
    {
      path = "bootloader/mcuboot";
      src = fetchgit {
        fetchSubmodules = false;
        url = "https://github.com/zephyrproject-rtos/mcuboot";
        rev = "9bf7ce8c5fe8152836a6e00bd4444153bd950342";
        hash = "sha256-Q76cIJmrejdIyoxC4SyBOBDHkjRjbEDnTXFlNWzQYtM=";
      };
    }
    {
      path = "modules/hal/espressif";
      src = fetchgit {
        fetchSubmodules = false;
        url = "https://github.com/zephyrproject-rtos/hal_espressif";
        rev = "80d910ca89eab9bce03f59a4ade33f1fc30ce0ad";
        hash = "sha256-XR0jZsMUq6LZ8QtpiBJzuvydv3jkxgroT2vV7rtSp3U=";
      };
    }
  ];
in
stdenvNoCC.mkDerivation {
  pname = "openrtx-ttwrplus";
  inherit version src;

  nativeBuildInputs = [
    zephyr-sdk
    zephyrPython
    cmake
    ninja
    dtc
    gperf
    git
    removeReferencesTo
  ];

  strictDeps = true;
  dontUseCmakeConfigure = true;
  dontConfigure = true;

  ZEPHYR_TOOLCHAIN_VARIANT = "zephyr";
  ZEPHYR_SDK_INSTALL_DIR = zephyr-sdk;

  # Assemble a west workspace: ws/OpenRTX (manifest repository), ws/zephyr,
  # ws/bootloader/mcuboot, ws/modules/hal/espressif.
  unpackPhase = ''
    runHook preUnpack
    export HOME=$TMPDIR
    ws=$TMPDIR/ws
    mkdir -p "$ws"
    cp -r "$src" "$ws/OpenRTX"
    chmod -R u+w "$ws/OpenRTX"

    # Fixed dates make the commit hash a function of the tree alone: ESP-IDF
    # embeds `git describe --always` of hal_espressif in the bootloader.
    fakeGit() {
      git init -q
      git config user.email nix@localhost
      git config user.name nix
      git add -A
      GIT_AUTHOR_DATE=1970-01-01T00:00:00Z GIT_COMMITTER_DATE=1970-01-01T00:00:00Z \
        git commit -q -m "nix snapshot"
      git branch -q manifest-rev
    }
    ${lib.concatMapStringsSep "\n" (p: ''
      mkdir -p "$ws/${p.path}"
      cp -r ${p.src}/. "$ws/${p.path}/"
      chmod -R u+w "$ws/${p.path}"
      ( cd "$ws/${p.path}" && fakeGit )
    '') westProjects}

    cd "$ws/OpenRTX"
    ${subprojects.populateHook}
    cd "$ws"
    west init -l OpenRTX
    cd "$ws/OpenRTX"
    runHook postUnpack
  '';

  postPatch = ''
    substituteInPlace CMakeLists.txt \
      --replace-fail "COMMAND git describe --tags --dirty --always" \
                     "COMMAND echo ${gitVersion}"
    patchShebangs scripts
  '';

  buildPhase = ''
    runHook preBuild
    # BUILD_VERSION replaces `git describe --abbrev=12 --always` in ws/zephyr,
    # which would otherwise name the throw-away commit in the boot banner.
    west build -b ttwrplus -d build_ttwr . -- -DBUILD_VERSION=${builtins.substring 0 12 zephyrRev}
    python3 scripts/uf2conv.py build_ttwr/zephyr/zephyr.bin -c -f ESP32S3 -b 0x0 -o openrtx_ttwrplus.uf2
    runHook postBuild
  '';

  installPhase = ''
    runHook preInstall
    install -Dm644 build_ttwr/zephyr/zephyr.elf "$out/openrtx_ttwrplus.elf"
    install -Dm644 build_ttwr/zephyr/zephyr.bin "$out/openrtx_ttwrplus.bin"
    install -Dm644 openrtx_ttwrplus.uf2 "$out/openrtx_ttwrplus.uf2"
    install -Dm644 build_ttwr/esp-idf/build/bootloader/bootloader.bin "$out/bootloader.bin"
    # The ELF's DWARF names the SDK's headers; blank the store hash so the
    # 270 MB SDK is not a runtime dependency (the images never reference it).
    remove-references-to -t ${zephyr-sdk} "$out/openrtx_ttwrplus.elf"
    runHook postInstall
  '';
  disallowedReferences = [ zephyr-sdk ];

  dontStrip = true;
  dontPatchELF = true;

  passthru = {
    inherit gitVersion zephyr-sdk;
    description = "LilyGO T-TWR Plus";
    flash = {
      method = "uf2";
      file = "openrtx_ttwrplus.uf2";
    };
  };

  meta = {
    description = "OpenRTX firmware for the LilyGO T-TWR Plus (ESP32-S3, Zephyr)";
    homepage = "https://openrtx.org";
    license = lib.licenses.gpl3Plus;
    platforms = [
      "x86_64-linux"
      "aarch64-linux"
    ];
  };
}
