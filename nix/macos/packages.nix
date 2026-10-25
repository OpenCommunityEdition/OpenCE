# The macOS port (docs/macos_host.md): the guest, the host, OpenCE.app
{ inputs, cell }:
let
  pkgs = inputs.cells.repo.pkgs;
  inherit (inputs.cells.repo.toolchain)
    root
    buildEnvironment
    tools
    libraries
    musl
    version
    ;

  # configure.py and ninja in the sandbox, building target (a ninja
  # target) and installing what install says
  macosBuild =
    {
      pname,
      target,
      install,
    }:
    pkgs.stdenv.mkDerivation (
      buildEnvironment
      // {
        inherit pname version;
        src = inputs.self;
        nativeBuildInputs = tools ++ [ pkgs.rustPlatform.cargoSetupHook ];
        buildInputs = libraries;
        # the runtime crate's dependencies, from its Cargo.lock (the
        # sandbox has no network)
        cargoDeps = pkgs.rustPlatform.importCargoLock {
          lockFile = root + /port/macos/runtime/Cargo.lock;
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
      }
    );
in
{
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
  # OpenCE.app. Its Swift is Xcode's, from outside the store (the
  # tech stack's impure Apple toolchain: nixpkgs' Swift cannot link
  # against the macOS 26 SDK), so it needs Xcode and a build without
  # the sandbox, as this Mac's Nix runs them
  app = macosBuild {
    pname = "opence-app";
    target = "macos_app";
    install = ''
      mkdir -p $out/Applications
      cp -R build/macos/OpenCE.app $out/Applications/OpenCE.app
    '';
  };
}
