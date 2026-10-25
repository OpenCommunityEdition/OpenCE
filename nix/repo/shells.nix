# `nix develop`, and `nix develop .#fuzz` for engine/fuzz
{ inputs, cell }:
let
  pkgs = cell.pkgs;
  inherit (cell.toolchain) buildEnvironment tools libraries;
  hooks = cell.hooks.default;
in
{
  default = pkgs.mkShell (
    buildEnvironment
    // {
      packages = tools ++ hooks.enabledPackages;
      buildInputs = libraries;
      # (the engine's shaders compile with Xcode's Metal toolchain, an
      # Xcode component nixpkgs cannot carry: tools/engine_build.py)
      # (and the pre-commit hooks installed: nix/repo/hooks.nix)
      shellHook = hooks.shellHook + ''
        echo "OpenCE macOS shell: python configure.py && ninja macos"
        if ! env -u SDKROOT -u DEVELOPER_DIR /usr/bin/xcrun -sdk macosx metal --version >/dev/null 2>&1; then
          echo "Xcode's Metal toolchain is missing: xcodebuild -downloadComponent MetalToolchain"
        fi
      '';
    }
  );

  # the importer's fuzzing (engine/fuzz): cargo fuzz needs a nightly
  # compiler for libFuzzer's instrumentation
  fuzz = pkgs.mkShell {
    packages = [
      (pkgs.rust-bin.selectLatestNightlyWith (toolchain: toolchain.minimal))
      pkgs.cargo-fuzz
    ];
    buildInputs = libraries;
  };
}
