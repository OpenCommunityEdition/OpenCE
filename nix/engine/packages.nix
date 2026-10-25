# OpenCE Engine.app, the new engine (engine/, track native_engine_20261021)
{ inputs, cell }:
let
  pkgs = inputs.cells.repo.pkgs;
  inherit (inputs.cells.repo.toolchain) craneLib buildEnvironment tools libraries python version;
  inherit (cell.rust) commonArgs cargoArtifacts;

  # the Rust half (tools/engine_build.py --rust-only): libopence_ffi.a in
  # lib/, its UniFFI bindings and the snapshot's header in bindings/,
  # built in the sandbox with crane
  engine-rust = craneLib.mkCargoDerivation (commonArgs // {
    inherit cargoArtifacts;
    nativeBuildInputs = [ python pkgs.rust-cbindgen ];
    buildPhaseCargoCommand = "python ../tools/engine_build.py --rust-only $out";
    installPhaseCommand = "";
    doInstallCargoArtifacts = false;
  });
in
{
  inherit engine-rust;

  # the app around engine-rust. Its Swift is Xcode's, from outside the
  # store, so it builds without the sandbox as OpenCE.app does
  engine = pkgs.stdenv.mkDerivation (buildEnvironment // {
    pname = "opence-engine";
    inherit version;
    src = inputs.self;
    nativeBuildInputs = tools;
    buildInputs = libraries;
    buildPhase = ''
      runHook preBuild
      python tools/engine_build.py --out build/engine --rust ${engine-rust}
      runHook postBuild
    '';
    installPhase = ''
      runHook preInstall
      mkdir -p $out/Applications
      cp -R "build/engine/OpenCE Engine.app" "$out/Applications/OpenCE Engine.app"
      runHook postInstall
    '';
    dontFixup = true;
  });
}
