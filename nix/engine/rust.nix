# The engine's Rust workspace (engine/) for crane: its source, and its
# dependencies built once for the packages and the checks
{ inputs, cell }:
let
  pkgs = inputs.cells.repo.pkgs;
  inherit (inputs.cells.repo.toolchain) root craneLib version;
  inherit (pkgs) lib;
  fs = lib.fileset;

  # crane vendors each crate as a link into the store, and askama (in
  # uniffi_bindgen) reaches its askama.toml by a path made relative to the
  # link's real place, which then climbs out of the store; uniffi_bindgen
  # is copied in as a directory, the others stay links
  vendored = craneLib.vendorCargoDeps { cargoLock = root + /engine/Cargo.lock; };
  cargoVendorDir = pkgs.runCommand "opence-engine-vendor" { } ''
    mkdir $out
    for registry in ${vendored}/*/; do
      registry=''${registry%/}
      mkdir $out/$(basename $registry)
      for crate in $registry/*; do
        into=$out/$(basename $registry)/$(basename $crate)
        case $(basename $crate) in
          uniffi_bindgen-*) cp -R $(readlink -f $crate) $into ;;
          *) ln -s $(readlink -f $crate) $into ;;
        esac
      done
    done
    substitute ${vendored}/config.toml $out/config.toml --replace-fail ${vendored} $out
  '';
in
rec {
  # the workspace, without the Swift app and the fuzzing workspace (but
  # with its manifest and targets, which the importer's fuzz smoke test
  # reads), and what its builds read outside it: Jolt and the physics
  # wrapper (the physics crate's build.rs), the ragdolls' overrides (the
  # sim's include_str!) and the build script
  commonArgs = {
    pname = "opence-engine-rust";
    inherit version;
    src = fs.toSource {
      inherit root;
      fileset = fs.unions [
        (fs.difference (root + /engine) (
          fs.unions [
            (root + /engine/app)
            (root + /engine/fuzz)
          ]
        ))
        (root + /engine/fuzz/Cargo.toml)
        (root + /engine/fuzz/fuzz_targets)
        (root + /port/third_party/jolt)
        (root + /port/macos/host/physics)
        (root + /port/assets/ragdolls/ragdolls.txt)
        (root + /tools/engine_build.py)
      ];
    };
    cargoLock = root + /engine/Cargo.lock;
    cargoToml = root + /engine/Cargo.toml;
    inherit cargoVendorDir;
    # (the workspace is engine/, not the source's root)
    postUnpack = ''
      cd $sourceRoot/engine
      sourceRoot="."
    '';
    strictDeps = true;
    buildInputs = [ pkgs.apple-sdk_26 ];
  };

  # every dependency, for the library, the bindings' generator (the
  # bindgen feature) and the tests; its version is fixed, since the
  # commit's would rename it, and so build it again, at every commit
  cargoArtifacts = craneLib.buildDepsOnly (
    commonArgs
    // {
      version = "locked";
      buildPhaseCargoCommand = ''
        cargoWithProfile check --locked --workspace --all-targets
        cargoWithProfile build --locked --workspace
        cargoWithProfile build --locked -p opence_ffi --features bindgen --bin uniffi-bindgen
        cargoWithProfile test --locked --workspace --no-run
      '';
    }
  );

  # the tests' dependencies, in the profile the suite tests in (test-opt:
  # optimised, with debug assertions and overflow checks, which the sim's
  # checks tests look for)
  testArtifacts = craneLib.buildDepsOnly (
    commonArgs
    // {
      version = "locked";
      CARGO_PROFILE = "test-opt";
      buildPhaseCargoCommand = ''
        cargoWithProfile test --locked --workspace --no-run
      '';
    }
  );
}
