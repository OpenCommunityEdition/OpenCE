{
  # The macOS build (docs/macos_host.md): `nix develop` is the shell to work
  # in, `nix build .#host` the build that finishes a track. The outputs are
  # grown with std from the cells in nix/ (each cell a folder, each block a
  # file in it):
  #   repo    pkgs, toolchain, shells, formatter, hooks: what the others share
  #   macos   packages: guest, host, app (the port)
  #   engine  rust, packages, checks, apps: the new engine, its Rust with crane
  description = "OpenCE: Halo: Combat Evolved on macOS";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    rust-overlay = {
      url = "github:oxalica/rust-overlay";
      inputs.nixpkgs.follows = "nixpkgs";
    };
    # (with its own nixpkgs: std grows its own outputs for x86_64-darwin,
    # which nixpkgs-unstable no longer lists as a system)
    std.url = "github:divnix/std";
    crane.url = "github:ipetkov/crane";
    # `nix fmt` (nix/repo/formatter.nix) and the pre-commit hooks
    # (nix/repo/hooks.nix)
    treefmt-nix = {
      url = "github:numtide/treefmt-nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
    git-hooks = {
      url = "github:cachix/git-hooks.nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs =
    {
      self,
      nixpkgs,
      std,
      ...
    }@inputs:
    let
      system = "aarch64-darwin";
      inherit (nixpkgs) lib;
    in
    std.growOn
      {
        inherit inputs;
        systems = [ system ];
        cellsFrom = ./nix;
        cellBlocks = with std.blockTypes; [
          (pkgs "pkgs")
          (anything "toolchain")
          (devshells "shells")
          (anything "formatter")
          (anything "hooks")
          (installables "packages")
          (anything "rust")
          (anything "checks")
          (runnables "apps")
        ];
      }
      {
        packages = std.harvest self [
          [
            "macos"
            "packages"
          ]
          [
            "engine"
            "packages"
          ]
        ];
        devShells = std.harvest self [
          "repo"
          "shells"
        ];
        checks = std.harvest self [
          "engine"
          "checks"
        ];
      }
      {
        packages.${system}.default = self.packages.${system}.host;
        apps =
          lib.mapAttrs
            (
              _:
              lib.mapAttrs (
                _: program: {
                  type = "app";
                  program = lib.getExe program;
                }
              )
            )
            (
              std.harvest self [
                "engine"
                "apps"
              ]
            );
        formatter = lib.mapAttrs (_: formatter: formatter.default) (
          std.harvest self [
            "repo"
            "formatter"
          ]
        );
      };
}
