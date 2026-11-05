# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Hermetic Nix build of OpenRTX: every firmware image, the Linux emulator, the
# unit tests, the arm-miosix-eabi cross toolchain and all host tools. See
# nix/README.md for usage and design notes.
{
  description = "OpenRTX - modular open source radio firmware";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    nix-github-actions = {
      url = "github:nix-community/nix-github-actions";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs =
    {
      self,
      nixpkgs,
      nix-github-actions,
    }:
    let
      inherit (nixpkgs) lib;

      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "aarch64-darwin"
      ];

      # Release version from meson.build's project() call.
      version =
        let
          line =
            lib.findFirst (l: builtins.match "^ *version *: *'[^']+',?$" l != null)
              (throw "flake.nix: could not find the version in meson.build")
              (lib.splitString "\n" (builtins.readFile ./meson.build));
        in
        builtins.head (builtins.match "^ *version *: *'([^']+)',?$" line);

      # Approximation of meson.build's `git describe --tags --dirty --always`
      # (e.g. v0.4.4-53-g9efa132c): there is no .git in the sandbox, so the
      # commit count and the `g` prefix are dropped and the flake's own
      # revision is used instead (v0.4.4-9efa132c, v0.4.4-9efa132c-dirty, or
      # v0.4.4-unknown when built from a non-git source tree).
      gitVersion = "v${version}-${self.shortRev or self.dirtyShortRev or "unknown"}";

      # The sources the builds actually read: tracked files minus the Nix
      # code, CI/editor configuration and documentation, so that editing those
      # does not rebuild the firmware. Non-git flake sources (path:, tarballs)
      # may also carry build directories, `result` links and wrap checkouts
      # under subprojects/, which are dropped as well.
      src = lib.cleanSourceWith {
        name = "openrtx-src";
        src = self;
        filter =
          path: type:
          let
            rel = lib.removePrefix (toString self + "/") (toString path);
            parts = lib.splitString "/" rel;
            top = builtins.head parts;
            name = baseNameOf path;
          in
          lib.cleanSourceFilter path type
          && !(builtins.elem top [
            ".devcontainer"
            ".git"
            ".github"
            ".gitignore"
            ".vscode"
            "AGENTS.md"
            "CHANGELOG.rst"
            "CONTRIBUTING.md"
            "LICENSES"
            "README.md"
            "REUSE.toml"
            "flake.lock"
            "flake.nix"
            "meta"
            "nix"
          ])
          && !(lib.hasPrefix "build" name && type == "directory" && builtins.length parts == 1)
          && !(lib.hasPrefix "result" name && builtins.length parts == 1)
          && !(
            top == "subprojects"
            && builtins.length parts == 2
            && name != "packagefiles"
            && (type == "directory" || name == ".wraplock")
          );
      };

      pkgsFor =
        system:
        import nixpkgs {
          inherit system;
          overlays = [ self.overlays.default ];
        };
      forAllSystems = f: lib.genAttrs systems (system: f (pkgsFor system));

      availableOn = pkgs: lib.meta.availableOn pkgs.stdenv.hostPlatform;
      available = pkgs: lib.filterAttrs (_: availableOn pkgs);
    in
    {
      # Everything is defined once, in this overlay, as the `openrtx` package
      # scope; packages/apps/checks/devShells are views on it.
      overlays.default = final: _prev: {
        openrtx = lib.makeScope final.newScope (oself: {
          inherit version gitVersion src;

          # nixpkgs f05535a ships python3Packages.reuse 6.2.0 whose Sphinx hook
          # no longer produces the "man" output, so the package fails to build
          # (NixOS/nixpkgs#576624, fix pending in NixOS/nixpkgs#579232). Drop
          # that output until the fix lands; devshell.nix and the reuse-lint
          # check take `reuse` from this scope.
          reuse = final.reuse.overrideAttrs (old: {
            outputs = lib.remove "man" old.outputs;
          });

          targets = import ./nix/targets.nix;

          miosix-toolchain = oself.callPackage ./nix/miosix-toolchain.nix { };
          radio_tool = oself.callPackage ./nix/radio_tool.nix { };
          subprojects = oself.callPackage ./nix/subprojects.nix { };
          udev-rules = oself.callPackage ./nix/udev-rules.nix { };
          zephyr-sdk = oself.callPackage ./nix/zephyr-sdk.nix { };
          zephyrPython = oself.callPackage ./nix/zephyr-python.nix { };

          firmware = lib.mapAttrs (
            name: target: oself.callPackage ./nix/firmware.nix ({ inherit name; } // target)
          ) oself.targets;
          ttwrplus = oself.callPackage ./nix/ttwrplus.nix { };

          emulator = oself.callPackage ./nix/emulator.nix { };
          emulator-asan = oself.emulator.override { withAsan = true; };
          emulator-coverage = oself.emulator.override {
            withCoverage = true;
            doCheck = true;
          };

          # apps.flash-<radio>: one wrapper per flashable image.
          flashers = lib.mapAttrs (
            name: fw:
            oself.callPackage ./nix/flash.nix {
              inherit name;
              inherit (fw.passthru) description flash;
              firmware = fw;
            }
          ) (lib.filterAttrs (_: fw: fw.passthru ? flash) (oself.firmware // { inherit (oself) ttwrplus; }));

          all = final.symlinkJoin {
            name = "openrtx-all-${version}";
            paths = lib.attrValues (
              available final (
                oself.firmware
                // {
                  inherit (oself) ttwrplus emulator;
                }
              )
            );
            meta = {
              description = "Every OpenRTX firmware image this platform can build, plus the Linux emulator";
              homepage = "https://openrtx.org";
              license = lib.licenses.gpl3Plus;
              platforms = lib.platforms.unix;
            };
          };

          devShell = oself.callPackage ./nix/devshell.nix { };
          zephyrShell = oself.callPackage ./nix/zephyr-shell.nix { };
        });
      };

      packages = forAllSystems (
        pkgs:
        let
          o = pkgs.openrtx;
        in
        available pkgs (
          lib.mapAttrs' (name: lib.nameValuePair "openrtx-${name}") o.firmware
          // lib.mapAttrs' (name: lib.nameValuePair "flash-${name}") o.flashers
          // {
            default = o.all;
            inherit (o)
              all
              emulator
              emulator-asan
              emulator-coverage
              miosix-toolchain
              radio_tool
              udev-rules
              zephyr-sdk
              ;
            openrtx-ttwrplus = o.ttwrplus;
          }
        )
      );

      apps = forAllSystems (
        pkgs:
        let
          o = pkgs.openrtx;
          mkApp = program: description: {
            type = "app";
            inherit program;
            meta = {
              inherit description;
            };
          };
          emulatorApp = exe: description: mkApp (lib.getExe' o.emulator exe) description;
        in
        {
          default = emulatorApp "openrtx_linux" "Run the OpenRTX Linux emulator";
          emulator = emulatorApp "openrtx_linux" "Run the OpenRTX Linux emulator";
          emulator-smallscreen = emulatorApp "openrtx_linux_smallscreen" "Run the OpenRTX Linux emulator with the 128x64 UI";
          emulator-mod17 = emulatorApp "openrtx_linux_mod17" "Run the OpenRTX Linux emulator with the Module17 UI";
          radio_tool = mkApp (lib.getExe o.radio_tool) "radio_tool firmware wrapping/flashing utility";
        }
        // lib.mapAttrs' (
          name: drv: lib.nameValuePair "flash-${name}" (mkApp (lib.getExe drv) drv.meta.description)
        ) (available pkgs o.flashers)
      );

      # Kept cheap on purpose: `nix flake check` builds all of these. The
      # firmware images and the toolchain are built with `nix build .#all`.
      checks = forAllSystems (
        pkgs:
        let
          o = pkgs.openrtx;
          nixFiles = map (f: lib.removePrefix (toString self + "/") (toString f)) (
            [ ./flake.nix ]
            ++ lib.filter (f: lib.hasSuffix ".nix" (toString f)) (lib.filesystem.listFilesRecursive ./nix)
          );
          # Guard against a meta.platforms mistake silently hiding a package
          # from the `available` filter on Linux.
          expectedOnLinux = [
            "miosix-toolchain"
            "openrtx-ttwrplus"
            "udev-rules"
            "zephyr-sdk"
          ]
          ++ map (n: "openrtx-${n}") (lib.attrNames o.targets);
          missing = lib.filter (
            n: !(self.packages.${pkgs.stdenv.hostPlatform.system} ? ${n})
          ) expectedOnLinux;
        in
        {
          unit-tests = o.emulator.override { doCheck = true; };
          unit-tests-asan = o.emulator-asan.override { doCheck = true; };
          nix-lint =
            pkgs.runCommand "openrtx-nix-lint"
              {
                nativeBuildInputs = [
                  pkgs.nixfmt
                  pkgs.statix
                  pkgs.deadnix
                ];
              }
              ''
                cd ${self}
                nixfmt --check ${lib.concatStringsSep " " nixFiles}
                statix check .
                deadnix --fail .
                touch "$out"
              '';
          reuse-lint = pkgs.runCommand "openrtx-reuse-lint" { nativeBuildInputs = [ pkgs.openrtx.reuse ]; } ''
            cd ${self}
            reuse lint
            touch "$out"
          '';
        }
        // lib.optionalAttrs pkgs.stdenv.hostPlatform.isLinux {
          packages-present =
            assert lib.assertMsg (missing == [ ]) "flake.nix: packages missing on Linux: ${toString missing}";
            pkgs.runCommand "openrtx-packages-present" { } "touch $out";
        }
      );

      devShells = forAllSystems (
        pkgs:
        {
          default = pkgs.openrtx.devShell;
        }
        // lib.optionalAttrs (availableOn pkgs pkgs.openrtx.zephyr-sdk) {
          zephyr = pkgs.openrtx.zephyrShell;
        }
      );

      # One GitHub Actions job per attribute (.github/workflows/nix.yml): every
      # check, package and dev shell on x86_64-linux, where all of them build.
      # The toolchain has a job of its own that runs first, and `default` is
      # `all`. Other systems are only evaluated, by `nix flake check
      # --all-systems --no-build`.
      githubActions = nix-github-actions.lib.mkGithubMatrix {
        checks.x86_64-linux =
          self.checks.x86_64-linux
          // removeAttrs self.packages.x86_64-linux [
            "default"
            "miosix-toolchain"
          ]
          // lib.mapAttrs' (name: lib.nameValuePair "devshell-${name}") self.devShells.x86_64-linux;
      };

      # `nix fmt` formats every .nix file in the tree; `nix fmt -- --ci` checks.
      formatter = forAllSystems (pkgs: pkgs.nixfmt-tree);

      # Grants non-root access to the radios' USB bootloaders.
      #   imports = [ inputs.openrtx.nixosModules.default ];
      nixosModules.default =
        { pkgs, ... }:
        {
          services.udev.packages = [ self.packages.${pkgs.stdenv.hostPlatform.system}.udev-rules ];
        };
    };
}
