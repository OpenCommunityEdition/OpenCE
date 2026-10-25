# The compilers, tools and libraries the builds and the shell share
{ inputs, cell }:
let
  pkgs = cell.pkgs;
  # the repository as a path. std copies each cell into the store alone,
  # so a path such as ../.. would leave the repository; and a path, not
  # "${inputs.self}/...", copies only the file named (a Cargo.lock's
  # vendored crates then build again only when it changes)
  root = /. + builtins.unsafeDiscardStringContext inputs.self.outPath;
  # clang 22: the guest's compiler (its arm64_32 target), and the
  # version the profiles in pgo/ were recorded with
  llvm = pkgs.llvmPackages_22;
  rust = pkgs.rust-bin.fromRustupToolchainFile (root + /rust-toolchain.toml);
  python = pkgs.python3.withPackages (ps: [ ps.pytest ps.pyflakes ]);
in
{
  inherit root llvm rust python;

  version = inputs.self.shortRev or inputs.self.dirtyShortRev or "dirty";

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
    pkgs.cargo-nextest
    pkgs.cargo-mutants
    pkgs.cargo-insta
    pkgs.rust-cbindgen
    pkgs.cargo-deny
    pkgs.darwin.sigtool
  ];
  libraries = [ pkgs.sdl3 pkgs.apple-sdk_26 ];

  # the guest's C library, where tools/macos_build.py would download it
  # (the sandbox has no network)
  musl = pkgs.fetchurl {
    url = "https://musl.libc.org/releases/musl-1.2.5.tar.gz";
    hash = "sha256-qaEYu+hNh2TaDqDSizqz+uhHf8fkCF2QECuFlvx8deQ=";
  };

  # crane with the repository's Rust (the engine's packages and checks)
  craneLib = (inputs.crane.mkLib pkgs).overrideToolchain (_: rust);
}
