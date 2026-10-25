# `nix run .#engine -- <arguments>`: OpenCE Engine.app started from the
# folder it is run in, which (with assets/maps) is the data root: the
# launcher without arguments, or e.g. `--map devgulch --station vehicles`
{ inputs, cell }:
let
  pkgs = inputs.cells.repo.pkgs;
in
rec {
  engine = pkgs.writeShellScriptBin "opence-engine" ''
    exec "${cell.packages.engine}/Applications/OpenCE Engine.app/Contents/MacOS/OpenCEEngine" "$@"
  '';
  default = engine;
}
