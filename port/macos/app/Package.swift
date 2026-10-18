// swift-tools-version: 6.2
// OpenCE.app's Swift (docs/macos_host.md). `ninja macos_app` builds it with
// Xcode's toolchain and links it with build/macos/libhalo_host.a, the
// runtime crate and SDL3, whose paths it passes on the command line;
// tools/macos_app.py makes the bundle.
import PackageDescription

let package = Package(
    name: "OpenCE",
    platforms: [.macOS(.v26)],
    targets: [
        // the C host's entry point, port/macos/host/halo_host.h
        .systemLibrary(name: "CHaloHost", path: "Sources/CHaloHost"),
        .executableTarget(name: "OpenCE", dependencies: ["CHaloHost"]),
    ]
)
