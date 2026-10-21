// swift-tools-version: 6.2
// OpenCE Engine.app: the new engine's app (conductor/tracks/native_engine_20261021).
// tools/engine_build.py builds the Rust library (engine/crates/ffi) first,
// writes its UniFFI bindings and the snapshot's header into the two
// generated folders, and links the library by passing its path.
import PackageDescription

let package = Package(
    name: "OpenCEEngine",
    platforms: [.macOS(.v26)],
    targets: [
        // the ffi crate's C declarations: UniFFI's and opence_snapshot.h (generated)
        .systemLibrary(name: "opence_ffiFFI", path: "Sources/opence_ffiFFI"),
        // the ffi crate's Swift bindings (generated)
        .target(name: "OpenCEEngineCore", dependencies: ["opence_ffiFFI"],
                // (as uniffi-bindgen writes it, for Swift 5's checks)
                swiftSettings: [.swiftLanguageMode(.v5)]),
        // the app's side of the contract: the engine and its snapshots
        .target(name: "OpenCEEngineKit", dependencies: ["OpenCEEngineCore", "opence_ffiFFI"]),
        // the window: its Metal layer, display link, mouse capture and keys
        .target(name: "OpenCEEngineWindow"),
        .executableTarget(name: "OpenCEEngine", dependencies: ["OpenCEEngineKit", "OpenCEEngineWindow"]),
        .testTarget(name: "OpenCEEngineTests",
                    dependencies: ["OpenCEEngineKit", "OpenCEEngineCore", "opence_ffiFFI", "OpenCEEngineWindow"]),
    ]
)
