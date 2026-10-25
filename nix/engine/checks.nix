# `nix flake check`: the workflow's Rust checks (conductor/workflow.md),
# on the dependencies engine-rust builds with. The tests that need the
# player's maps or default.xbe skip themselves here.
{ inputs, cell }:
let
  inherit (inputs.cells.repo.toolchain) craneLib;
  inherit (cell.rust) commonArgs cargoArtifacts testArtifacts;
in
{
  fmt = craneLib.cargoFmt commonArgs;
  clippy = craneLib.cargoClippy (commonArgs // {
    inherit cargoArtifacts;
    cargoClippyExtraArgs = "--workspace -- --deny warnings";
  });
  # (in test-opt, as tools/engine_build.py tests)
  tests = craneLib.cargoNextest (commonArgs // {
    cargoArtifacts = testArtifacts;
    CARGO_PROFILE = "test-opt";
    cargoNextestExtraArgs = "--workspace";
  });
  # bans, licences and sources (engine/deny.toml); the advisories need
  # the network
  deny = craneLib.cargoDeny commonArgs;
}
