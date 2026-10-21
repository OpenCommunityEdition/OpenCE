// swift-tools-version: 6.2
// OpenCE Engine.app: the new engine's app (conductor/tracks/native_engine_20261021).
// tools/engine_build.py builds the Rust library first and copies its UniFFI
// bindings into Sources/OpenCEEngineFFI and Sources/OpenCEEngineCore.
import PackageDescription

let package = Package(
    name: "OpenCEEngine",
    platforms: [.macOS(.v26)],
    targets: [
        .systemLibrary(name: "OpenCEEngineFFI", path: "Sources/OpenCEEngineFFI"),
        .target(
            name: "OpenCEEngineCore",
            dependencies: ["OpenCEEngineFFI"],
            path: "Sources/OpenCEEngineCore",
            swiftSettings: [.swiftLanguageMode(.v5)]
        ),
        .executableTarget(
            name: "OpenCEEngine",
            dependencies: ["OpenCEEngineCore"],
            path: "Sources/OpenCEEngine"
        ),
    ]
)
