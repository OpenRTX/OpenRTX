# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Zephyr SDK 0.17.0 (the version used by .github/workflows/main.yml) with
# only the toolchains OpenRTX needs. Prebuilt binaries from the Zephyr
# project's sdk-ng releases, patched to run from the Nix store. Modelled on
# nix-community/zephyr-nix's sdk.nix.
{
  lib,
  stdenv,
  fetchurl,
  autoPatchelfHook,
  which,
  cmake,
  ncurses,
  toolchains ? [ "xtensa-espressif_esp32s3_zephyr-elf" ],
}:

let
  version = "0.17.0";
  baseUrl = "https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v${version}";

  # sha256 values from ${baseUrl}/sha256.sum
  files = {
    "zephyr-sdk-0.17.0_linux-x86_64_minimal.tar.xz" =
      "0514d2c684dfb5f6327374bfed0b3dcf727ff1500195d26b3730f98252fed095";
    "zephyr-sdk-0.17.0_linux-aarch64_minimal.tar.xz" =
      "889f10c68a179f5ff2e4ea84749202c117bb522af705d5a888ebe76373acac5a";
    "zephyr-sdk-0.17.0_macos-aarch64_minimal.tar.xz" =
      "d6dd38ac08d1e3e8d4653a7f03f6f52a704fc9317d8eed6e950e107829f929c4";
    "toolchain_linux-x86_64_xtensa-espressif_esp32s3_zephyr-elf.tar.xz" =
      "8648f63528f7eec27feed9a0939b6ad0c1e4da0c247fd9711e8985a1f709f4a9";
    "toolchain_linux-aarch64_xtensa-espressif_esp32s3_zephyr-elf.tar.xz" =
      "c1811bc97663c2b4b0b01fc23aadc62a0141a4a04a9411605efc122d8b964cda";
    "toolchain_macos-aarch64_xtensa-espressif_esp32s3_zephyr-elf.tar.xz" =
      "f5385d94081ea4e6fd7a2a03b893dd16b4a6239c6445a5f3234ec025f7177d20";
  };

  platform =
    if stdenv.hostPlatform.isLinux then
      "linux"
    else if stdenv.hostPlatform.isDarwin then
      "macos"
    else
      throw "zephyr-sdk: unsupported platform";
  arch =
    if stdenv.hostPlatform.isAarch64 then
      "aarch64"
    else if stdenv.hostPlatform.isx86_64 then
      "x86_64"
    else
      throw "zephyr-sdk: unsupported architecture";

  fetchSdkFile =
    file:
    fetchurl {
      url = "${baseUrl}/${file}";
      sha256 = files.${file};
    };
in
stdenv.mkDerivation {
  pname = "zephyr-sdk";
  inherit version;

  srcs = [
    (fetchSdkFile "zephyr-sdk-${version}_${platform}-${arch}_minimal.tar.xz")
  ]
  ++ map (t: fetchSdkFile "toolchain_${platform}-${arch}_${t}.tar.xz") toolchains;
  sourceRoot = ".";

  nativeBuildInputs = [
    which
    cmake
  ]
  ++ lib.optional stdenv.hostPlatform.isLinux autoPatchelfHook;
  buildInputs = [
    stdenv.cc.cc
    ncurses
  ];

  dontBuild = true;
  dontUseCmakeConfigure = true;

  installPhase = ''
    runHook preInstall
    rm -f zephyr-sdk-${version}/zephyr-sdk-${arch}-hosttools-standalone-*.sh
    rm -f env-vars
    mv zephyr-sdk-${version} "$out"
    # The toolchain tarballs unpack next to the SDK directory.
    for item in *; do
      [ -e "$item" ] && mv "$item" "$out/"
    done
    # The Python-enabled gdb variants are linked against libpython3.10, which
    # nixpkgs no longer ships; OpenRTX does not debug through them.
    rm -f "$out"/*/bin/*-gdb-py "$out"/*/bin/*-gdb-add-index-py
    mkdir -p "$out/nix-support"
    echo "export ZEPHYR_SDK_INSTALL_DIR=$out" > "$out/nix-support/setup-hook"
    runHook postInstall
  '';

  passthru = {
    inherit platform arch toolchains;
  };

  meta = {
    description = "Zephyr SDK ${version} (${lib.concatStringsSep ", " toolchains})";
    homepage = "https://github.com/zephyrproject-rtos/sdk-ng";
    license = lib.licenses.asl20;
    sourceProvenance = [ lib.sourceTypes.binaryNativeCode ];
    # The macOS tarballs are listed above but have never been exercised.
    platforms = [
      "x86_64-linux"
      "aarch64-linux"
    ];
  };
}
