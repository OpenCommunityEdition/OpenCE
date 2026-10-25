# The pre-commit hooks `nix develop` installs (git-hooks.nix): the tree
# formatted (`nix fmt`'s treefmt) and the engine's Rust free of clippy's
# warnings, before each commit
{ inputs, cell }:
let
  pkgs = cell.pkgs;
in
{
  default = inputs.git-hooks.lib.run {
    src = cell.toolchain.root;
    hooks = {
      treefmt = {
        enable = true;
        packageOverrides.treefmt = cell.formatter.default;
      };
      # (cargo from the shell, for the build it shares with the suite; a
      # commit made outside it enters it)
      clippy = {
        enable = true;
        name = "clippy";
        entry = toString (
          pkgs.writeShellScript "opence-clippy" ''
            run() { cd engine && cargo clippy --workspace --all-targets --locked -q -- -D warnings; }
            if command -v cargo >/dev/null && [ -n "''${IN_NIX_SHELL:-}" ]; then
              run
            else
              exec nix develop --command bash -c "$(declare -f run); run"
            fi
          ''
        );
        files = "^engine/.*\\.(rs|toml)$";
        pass_filenames = false;
      };
    };
  };
}
