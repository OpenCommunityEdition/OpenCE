// swift-tools-version: 6.2
// OpenCE.app's Swift (docs/macos_host.md). `ninja macos_app` builds it with
// Xcode's toolchain and links it with build/macos/libhalo_host.a, the
// runtime crate and SDL3, whose paths it passes on the command line;
// tools/macos_app.py makes the bundle. `ninja macos_app_test` runs the tests.
import PackageDescription

let package = Package(
    name: "OpenCE",
    platforms: [.macOS(.v26)],
    targets: [
        // the C host's entry point, port/macos/host/halo_host.h
        .systemLibrary(name: "CHaloHost", path: "Sources/CHaloHost"),
        // the runtime crate's Swift bindings (port/macos/runtime/src/swift.rs),
        // which the build generates into these two folders
        .systemLibrary(name: "opence_runtimeFFI", path: "Sources/opence_runtimeFFI"),
        .target(name: "OpenCERuntime", dependencies: ["opence_runtimeFFI"],
                // (as uniffi-bindgen writes it, for Swift 5's checks)
                swiftSettings: [.swiftLanguageMode(.v5)]),
        .executableTarget(name: "OpenCE", dependencies: ["CHaloHost", "OpenCERuntime"]),
        .testTarget(name: "OpenCETests", dependencies: ["OpenCERuntime"]),
    ]
)
