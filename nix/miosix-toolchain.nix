# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# arm-miosix-eabi GCC 9.2.0-mp3.2 cross toolchain, as built by the Miosix
# kernel's tools/compiler/gcc-9.2.0-mp3.2/install-script.sh (non-DESTDIR,
# non-HOST Linux path):
#
#   binutils -> gcc (all-gcc) -> newlib -> gcc (all) -> gdb -> mx-postlinker
#
# OpenRTX cannot be built with a stock arm-none-eabi toolchain: the Miosix
# patches add the `miosix` thread model to gcc/libstdc++/libatomic and a
# libc/sys/miosix port plus reentrancy changes to newlib.
#
# Deliberate differences from install-script.sh:
#   * --disable-libcc1: gdb's "compile" plugin is useless for a bare-metal
#     target and its .so embeds host gcc/glibc include paths, dragging the
#     whole host gcc into the runtime closure.
#   * gmp/mpfr/mpc come from nixpkgs (shared libs) instead of static in-tree
#     builds. gmp_arm64.patch only touches mpn/arm64/*.asm (an Apple Silicon
#     symbol naming fix already present in the gmp nixpkgs ships).
#   * The host compiler is gcc 13 (gcc13Stdenv): gdb-9.1's bundled readline
#     does not compile with gcc >= 14 (-Wincompatible-pointer-types and
#     -Wreturn-mismatch became errors).
#   * binutils gets --enable-deterministic-archives so the output is
#     bit-for-bit reproducible (verified with `nix build --rebuild`).
#   * lpc21isp (NXP LPC serial flasher) is not built; OpenRTX does not use it.
#   * No /usr/bin symlinks, no uninstall.sh, no sudo.
{
  lib,
  gcc13Stdenv,
  # Host stdenv. Deliberately NOT called `stdenv`: callPackage would always
  # fill that with the default (gcc 15) stdenv and silently override the
  # default below. Override with e.g. `hostStdenv = pkgs.stdenv` to experiment.
  hostStdenv ? gcc13Stdenv,
  fetchurl,
  fetchFromGitHub,
  gmp,
  mpfr,
  libmpc,
  expat,
  ncurses,
  bison,
  flex,
  perl,
  python3,
  which,
  # Miosix kernel revision providing the toolchain recipe (v3.01). This is the
  # same pin as .devcontainer/Dockerfile; bump both together.
  miosixRev ? "361e8c35b09ad73c15ded5332dff21ce8c9a711d",
}:

let
  target = "arm-miosix-eabi";
  gccVersion = "9.2.0";
  miosixPatchVersion = "mp3.2";

  # Pinned Miosix kernel checkout, only the toolchain recipe directory.
  miosixKernel = fetchFromGitHub {
    owner = "fedetft";
    repo = "miosix-kernel";
    rev = miosixRev;
    sparseCheckout = [ "tools/compiler/gcc-9.2.0-mp3.2" ];
    hash = "sha256-GPo+FtkacFe8/38GFQQKJIv2MoELxayI7Y8mUGW6xPI=";
  };
  recipe = "${miosixKernel}/tools/compiler/gcc-9.2.0-mp3.2";

  binutilsSrc = fetchurl {
    url = "https://ftp.gnu.org/gnu/binutils/binutils-2.32.tar.xz";
    hash = "sha256-CrbFXdhqku1WGXK6Fbm3CoufdVV/iWRGyC6LNuRz7gQ=";
  };
  gccSrc = fetchurl {
    url = "https://ftp.gnu.org/gnu/gcc/gcc-9.2.0/gcc-9.2.0.tar.xz";
    hash = "sha256-6m7wjxISOdpWlfdsmzNjehGNz2PiQWRCIjGRf6YfsgY=";
  };
  newlibSrc = fetchurl {
    url = "https://sourceware.org/pub/newlib/newlib-3.1.0.tar.gz";
    hash = "sha256-+0+hzCHpBgcZIIMAphQg5AidbebvWc9TO1f+dIAdECo=";
  };
  gdbSrc = fetchurl {
    url = "https://ftp.gnu.org/gnu/gdb/gdb-9.1.tar.xz";
    hash = "sha256-aZ4OyDL90vIcgmYXHqW/RAJL0FFk/fBk5NEMxM8NFzc=";
  };

  # Multilibs that install-script.sh's check_multilibs verifies.
  multilibs = [
    "arm/v4t/nofp"
    "thumb/v4t/nofp"
    "thumb/v6-m/nofp"
    "thumb/v7-m/nofp"
    "thumb/v7e-m+dp/hard"
    "thumb/v7e-m+fp/hard"
    "thumb/v6-m/nofp/pie/single-pic-base"
    "thumb/v7-m/nofp/pie/single-pic-base"
    "thumb/v7e-m+dp/hard/pie/single-pic-base"
    "thumb/v7e-m+fp/hard/pie/single-pic-base"
  ];
  stdenv = hostStdenv;
in
stdenv.mkDerivation {
  pname = "${target}-toolchain";
  version = "${gccVersion}-${miosixPatchVersion}";

  srcs = [
    binutilsSrc
    gccSrc
    newlibSrc
    gdbSrc
  ];
  sourceRoot = ".";

  nativeBuildInputs = [
    bison
    flex
    perl
    python3
    which
  ];
  buildInputs = [
    gmp
    mpfr
    libmpc
    expat
    ncurses
  ];

  # Everything below is either an unwrapped cross compiler (target libs) or
  # 2019-era host code; keep nixpkgs' -fPIE/-fstack-protector/fortify wrappers
  # out of the way like the upstream script (plain gcc/g++) does.
  hardeningDisable = [ "all" ];
  # The nixpkgs strip hook would run the *host* strip over $out/lib, which
  # contains ARM static archives (libgcc.a etc.). Strip by hand in postInstall
  # exactly what install-script.sh strips.
  dontStrip = true;
  # The patchelf fixup hook is left enabled on purpose: it prints "wrong ELF
  # type" for the ARM objects under $out/arm-miosix-eabi/lib and $out/lib/gcc
  # (harmless, failures are ignored) and --shrink-rpath drops unused -rpath
  # entries from the host executables. binutils' ar/ld really do link
  # libfl.so.2 (LEXLIB), so flex legitimately stays in the closure.
  enableParallelBuilding = true;
  # We drive configure/make manually for the four sub-builds.
  dontConfigure = true;
  dontBuild = true;

  postPatch = ''
    # Same order and -p0 layout as install-script.sh's extract().
    for p in binutils gcc newlib gdb; do
      echo "Applying $p.patch"
      patch -p0 < ${recipe}/patches/$p.patch
    done
    # gcc_mac_arm64.patch is only applied on Darwin/arm64 by the script.
    cp -r ${recipe}/mx-postlinker ./mx-postlinker
    chmod -R u+w ./mx-postlinker
  '';

  installPhase = ''
    runHook preInstall

    # nix-build already runs with -e; `nix develop` shells do not.
    set -e -o pipefail
    export PATH="$out/bin:$PATH"
    top=$PWD
    jobs="-j$NIX_BUILD_CORES"
    mkdir -p "$out"

    #
    # Part 4: binutils
    #
    mkdir binutils-obj && cd binutils-obj
    # --enable-deterministic-archives is not in install-script.sh: binutils
    # 2.32's ar/ranlib do not honour SOURCE_DATE_EPOCH, so without it every
    # target libc.a/libgcc.a/libstdc++.a member gets the build's mtime and
    # uid/gid, and `nix build --rebuild` reports the output as
    # non-deterministic.
    ../binutils-2.32/configure \
      --target=${target} \
      --prefix="$out" \
      --enable-interwork \
      --enable-multilib \
      --enable-lto \
      --enable-deterministic-archives \
      --disable-werror
    # MAKEINFO=true: the tarballs already ship .info files; we do not want to
    # depend on a texinfo new enough to reject 2019-era texi sources.
    make all $jobs MAKEINFO=true
    make install MAKEINFO=true
    cd "$top"

    #
    # Part 5: gcc stage 1 (all-gcc), --with-headers copies newlib's headers
    # to $out/arm-miosix-eabi/sys-include so libstdc++ can be configured.
    #
    mkdir objdir && cd objdir
    ../gcc-9.2.0/configure \
      --target=${target} \
      --with-gmp-include=${lib.getDev gmp}/include \
      --with-gmp-lib=${lib.getLib gmp}/lib \
      --with-mpfr-include=${lib.getDev mpfr}/include \
      --with-mpfr-lib=${lib.getLib mpfr}/lib \
      --with-mpc-include=${lib.getDev libmpc}/include \
      --with-mpc-lib=${lib.getLib libmpc}/lib \
      MAKEINFO=missing \
      --prefix="$out" \
      --disable-shared \
      --disable-libssp \
      --disable-nls \
      --disable-libgomp \
      --disable-libstdcxx-pch \
      --disable-libstdcxx-dual-abi \
      --disable-libstdcxx-filesystem-ts \
      --disable-libcc1 \
      --enable-threads=miosix \
      --enable-languages="c,c++" \
      --enable-lto \
      --disable-wchar_t \
      --with-newlib \
      --with-headers="$top/newlib-3.1.0/newlib/libc/include"
    make all-gcc $jobs
    make install-gcc
    # sys-include holds a pre-configure (empty) newlib.h that would shadow the
    # real one newlib installs into include/; see install-script.sh part 5.
    rm -rf "$out/${target}/sys-include"
    cd "$top"

    #
    # Part 6: newlib
    #
    mkdir newlib-obj && cd newlib-obj
    ../newlib-3.1.0/configure \
      --target=${target} \
      --prefix="$out" \
      --enable-multilib \
      --enable-newlib-reent-small \
      --enable-newlib-multithread \
      --enable-newlib-io-long-long \
      --disable-newlib-io-c99-formats \
      --disable-newlib-io-long-double \
      --disable-newlib-io-pos-args \
      --disable-newlib-mb \
      --disable-newlib-supplied-syscalls
    make $jobs
    make install
    cd "$top"

    #
    # Part 7: gcc stage 2 (libgcc, libstdc++, libatomic for every multilib)
    #
    cd objdir
    make all $jobs
    make install
    cd "$top"

    #
    # Part 8A: remove the root (non-multilib) libraries so that unsupported
    # -march/-mcpu combinations fail at link time instead of silently linking
    # arm7tdmi ARM-mode code.
    #
    # Upstream: find lib -mindepth 1 ! -path '*/arm/*' ! -path '*/thumb/*' -print -delete
    # That also tries to rmdir lib/arm and lib/thumb themselves, fails with
    # "Directory not empty" and upstream ignores the exit status; under set -e
    # we must not, so only remove the top-level entries that are not multilibs.
    # Note this also drops lib/ldscripts and the libgloss board files
    # (rdimon, redboot, nosys...) exactly as upstream does.
    find "$out/${target}/lib" -mindepth 1 -maxdepth 1 ! -name arm ! -name thumb -print -exec rm -rf {} +

    #
    # Part 9: gdb (no python, bundled zlib, no lzma)
    #
    mkdir gdb-obj && cd gdb-obj
    ../gdb-9.1/configure \
      --target=${target} \
      --prefix="$out" \
      --with-mpfr \
      --with-expat \
      --with-system-zlib=no \
      --with-lzma=no \
      --with-python=no \
      --enable-interwork \
      --enable-multilib \
      --disable-werror
    make all MAKEINFO=true $jobs
    make install MAKEINFO=true
    cd "$top"

    #
    # Part 10: mx-postlinker
    #
    make -C mx-postlinker CXX="$CXX"
    cp mx-postlinker/mx-postlinker "$out/bin/"

    runHook postInstall
  '';

  postInstall = ''
    # Part 13: final fixups.
    rm -f "$out/bin/${target}-gcc-${gccVersion}"
    # Strip the huge (debug-info laden) host executables. Upstream strips only
    # cc1/cc1plus/lto1 and bin/*; we also strip the rest of libexec/ (collect2,
    # lto-wrapper, liblto_plugin.so) and arm-miosix-eabi/bin/* because their
    # DWARF include-directory entries would otherwise pull the whole host gcc
    # and glibc-dev into the runtime closure. Target static archives and
    # crt*.o are left untouched (with -g), like upstream.
    find "$out/libexec" "$out/bin" "$out/${target}/bin" -type f \
      -exec sh -c 'for f; do $STRIP "$f" 2>/dev/null || true; done' sh {} +
    # Nothing in $out should reference the build tree.
    rm -rf "$out/share/info"
    # fixincludes/mkheaders are only useful for re-running fixincludes on a
    # foreign sysroot and drag sed/bash into the closure (nixpkgs' gcc drops
    # them too).
    rm -rf "$out/libexec/gcc/${target}/${gccVersion}/install-tools" \
           "$out/lib/gcc/${target}/${gccVersion}/install-tools"
  '';

  doInstallCheck = true;
  installCheckPhase = ''
    runHook preInstallCheck
    # Part 8B: check that every multilib has been built.
    for ml in ${lib.concatStringsSep " " multilibs}; do
      for f in libc.a libm.a libg.a libatomic.a libstdc++.a libsupc++.a; do
        test -f "$out/${target}/lib/$ml/$f" || { echo "::Error, $out/${target}/lib/$ml/$f not installed"; exit 1; }
      done
    done
    echo "::All multilibs have been built. OK"
    "$out/bin/${target}-gcc" --version
    "$out/bin/${target}-gcc" -dM -E - < /dev/null | grep -q '_MIOSIX_GCC_PATCH_MAJOR 3'
    "$out/bin/${target}-gcc" -dM -E - < /dev/null | grep -q '_MIOSIX_GCC_PATCH_MINOR 2'
    runHook postInstallCheck
  '';

  passthru = {
    inherit target multilibs miosixKernel;
    inherit
      binutilsSrc
      gccSrc
      newlibSrc
      gdbSrc
      ;
    targetPrefix = "${target}-";
  };

  meta = with lib; {
    description = "GCC ${gccVersion}-${miosixPatchVersion} cross toolchain for the Miosix kernel (arm-miosix-eabi)";
    homepage = "https://miosix.org";
    license = with licenses; [
      gpl3Plus
      lgpl2Plus
      bsd3
    ];
    platforms = platforms.linux;
  };
}
