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
        pkgs.darwin.sigtool
      ];
      libraries = [ pkgs.sdl3 pkgs.apple-sdk_26 ];
    in
    {
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
