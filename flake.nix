{
  # The macOS build (docs/macos_host.md): `nix develop` is the shell to work
  # in, `nix build .#host` the build that finishes a track.
  description = "OpenCE: Halo: Combat Evolved on macOS";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    rust-overlay = {
      url = "github:oxalica/rust-overlay";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { self, nixpkgs, rust-overlay }:
    let
      system = "aarch64-darwin";
      pkgs = import nixpkgs {
        inherit system;
        overlays = [ rust-overlay.overlays.default ];
      };
      # clang 22: the guest's compiler (its arm64_32 target), and the
      # version the profiles in pgo/ were recorded with
      llvm = pkgs.llvmPackages_22;
      rust = pkgs.rust-bin.fromRustupToolchainFile ./rust-toolchain.toml;
      python = pkgs.python3.withPackages (ps: [ ps.pytest ps.pyflakes ]);

      # what tools/macos_build.py takes from the environment. The guest's
      # compiler is the unwrapped clang: the wrapper's macOS flags would
      # reach the arm64_32 guest's compiles.
      buildEnvironment = {
        HALO_MACOS_GUEST_CC = "${llvm.clang-unwrapped}/bin/clang";
        HALO_MACOS_LLD = "${llvm.lld}/bin/ld.lld";
        HALO_MACOS_AR = "${llvm.llvm}/bin/llvm-ar";
        # (GLES2, GLES3 and KHR: the guest's OpenGL ES declarations)
        HALO_MACOS_GLES_INCLUDE = "${pkgs.libglvnd.dev}/include";
        # (the SDL3 the host links and whose headers the guest compiles with:
        # 3.4.16, as tools/android_build.py's SDL_TAG)
        HALO_MACOS_SDL_INCLUDE = "${pkgs.sdl3.dev}/include";
        HALO_MACOS_SDL_LIB = "${pkgs.sdl3}/lib";
        HALO_MACOS_CODESIGN = "${pkgs.darwin.sigtool}/bin/codesign";
      };

      tools = [
        python
        pkgs.ninja
        llvm.lld
        llvm.llvm
        rust
        pkgs.cargo-llvm-cov
        pkgs.rust-cbindgen
        pkgs.darwin.sigtool
      ];
      libraries = [ pkgs.sdl3 pkgs.apple-sdk_26 ];

      # the guest's C library, where tools/macos_build.py would download it
      # (the sandbox has no network)
      musl = pkgs.fetchurl {
        url = "https://musl.libc.org/releases/musl-1.2.5.tar.gz";
        hash = "sha256-qaEYu+hNh2TaDqDSizqz+uhHf8fkCF2QECuFlvx8deQ=";
      };

      # configure.py and ninja in the sandbox, building target (a ninja
      # target) and installing what install says
      macosBuild = { pname, target, install }:
        pkgs.stdenv.mkDerivation (buildEnvironment // {
          inherit pname;
          version = self.shortRev or self.dirtyShortRev or "dirty";
          src = self;
          nativeBuildInputs = tools ++ [ pkgs.rustPlatform.cargoSetupHook ];
          buildInputs = libraries;
          # the runtime crate's dependencies, from its Cargo.lock (the
          # sandbox has no network)
          cargoDeps = pkgs.rustPlatform.importCargoLock {
            lockFile = ./port/macos/runtime/Cargo.lock;
          };
          cargoRoot = "port/macos/runtime";
          configurePhase = ''
            runHook preConfigure
            mkdir -p build/macos/third_party
            tar xzf ${musl} -C build/macos/third_party
            python configure.py
            runHook postConfigure
          '';
          buildPhase = ''
            runHook preBuild
            ninja -j $NIX_BUILD_CORES ${target}
            runHook postBuild
          '';
          installPhase = ''
            runHook preInstall
            ${install}
            runHook postInstall
          '';
          # the fixup phase strips and signs binaries again ad hoc, which
          # would drop the host's hypervisor entitlement
          dontFixup = true;
        });
    in
    {
      packages.${system} = rec {
        guest = macosBuild {
          pname = "opence-guest";
          target = "build/macos/halo_guest.elf";
          install = ''
            install -Dm644 build/macos/halo_guest.elf $out/share/opence/halo_guest.elf
          '';
        };
        host = macosBuild {
          pname = "opence-host";
          target = "macos";
          install = ''
            install -Dm755 build/macos/halo_host $out/bin/halo_host
            install -Dm644 build/macos/halo_guest.elf $out/share/opence/halo_guest.elf
          '';
        };
        default = host;
      };

      devShells.${system}.default = pkgs.mkShell (buildEnvironment // {
        packages = tools;
        buildInputs = libraries;
        shellHook = ''
          echo "OpenCE macOS shell: python configure.py && ninja macos"
        '';
      });

      formatter.${system} = pkgs.nixfmt;
    };
}
