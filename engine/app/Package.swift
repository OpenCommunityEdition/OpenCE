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
        .target(name: "OpenCEEngineKit", dependencies: ["OpenCEEngineCore", "OpenCEEngineRender", "opence_ffiFFI"]),
        // the window: its Metal layer, display link, mouse capture and keys
        .target(name: "OpenCEEngineWindow"),
        // the renderer: the level's buffers, textures and passes, and frame capture
        .target(name: "OpenCEEngineRender", dependencies: ["OpenCEEngineCore", "opence_ffiFFI"]),
        // the menus: their models, and their SwiftUI views over the Metal view
        .target(name: "OpenCEEngineMenu", dependencies: ["OpenCEEngineCore", "OpenCEEngineKit"]),
        .executableTarget(name: "OpenCEEngine",
                          dependencies: ["OpenCEEngineKit", "OpenCEEngineWindow", "OpenCEEngineRender",
                                         "OpenCEEngineMenu", "opence_ffiFFI"]),
        .testTarget(name: "OpenCEEngineTests",
                    dependencies: ["OpenCEEngineKit", "OpenCEEngineCore", "opence_ffiFFI", "OpenCEEngineWindow",
                                   "OpenCEEngineRender", "OpenCEEngineMenu"]),
    ]
)
