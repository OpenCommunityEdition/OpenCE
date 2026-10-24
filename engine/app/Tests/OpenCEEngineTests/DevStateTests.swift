import Foundation
import Metal
import OpenCEEngineCore
import OpenCEEngineKit
import OpenCEEngineMenu
import OpenCEEngineRender
import XCTest

/// The developer state as the app reads it: what the engine's shell holds,
/// read again only when it changed, untouched the stock frame; and the
/// Developer tab's lighting controls, launched as the shell reads them.
final class DevStateTests: XCTestCase {
    func read(_ engine: Engine) -> DevState {
        DevState(flag: { engine.shellFlag(name: $0) }, value: { engine.shellValue(name: $0) })
    }

    func testTheStateIsTheShells() throws {
        let engine = Engine()
        XCTAssertEqual(read(engine), DevState())
        XCTAssertTrue(read(engine).tuning.isStock)
        XCTAssertEqual(Set(engine.tuneControls().map(\.name)), Set(LightingTuning.names),
                       "the app reads every control the shell has")
        try engine.setShellVariable(name: "tune.exposure", value: "-1.5")
        try engine.setShellVariable(name: "tune.sun_green", value: "0.25")
        XCTAssertTrue(engine.runCommand(line: "set tune.tone_curve 1.5").ok)
        XCTAssertTrue(engine.runCommand(line: "dev.panel true").ok)
        XCTAssertTrue(engine.runCommand(line: "render.bloom false").ok)
        let state = read(engine)
        XCTAssertEqual(state.tuning.exposure, -1.5)
        XCTAssertEqual(state.tuning.sunColor, SIMD3(1, 0.25, 1))
        XCTAssertEqual(state.tuning.toneCurve, 1.5)
        XCTAssertFalse(state.tuning.isStock)
        XCTAssertTrue(state.panel)
        XCTAssertFalse(state.switches.bloom)
        XCTAssertTrue(engine.runCommand(line: "tune reset").ok)
        XCTAssertTrue(read(engine).tuning.isStock)
    }

    func testTheStateIsReadOnlyWhenItChanged() throws {
        let engine = Engine()
        let reader = DevStateReader()
        var asked = 0
        func update() -> DevState {
            reader.update(generation: engine.devGeneration(),
                          flag: { asked += 1; return engine.shellFlag(name: $0) },
                          value: { asked += 1; return engine.shellValue(name: $0) })
        }
        XCTAssertEqual(update(), DevState())
        XCTAssertEqual(reader.reads, 1)
        let first = asked
        for _ in 0..<10 {
            _ = update()
        }
        XCTAssertEqual(reader.reads, 1, "nothing changed: nothing read")
        XCTAssertEqual(asked, first)
        try engine.setShellVariable(name: "tune.bloom", value: "2")
        XCTAssertEqual(update().tuning.bloom, 2)
        XCTAssertEqual(reader.reads, 2)
        // (set to what it is: no change counted)
        try engine.setShellVariable(name: "tune.bloom", value: "2")
        _ = update()
        XCTAssertEqual(reader.reads, 2)
        reader.invalidate()
        _ = update()
        XCTAssertEqual(reader.reads, 3)
    }

    /// The state of an engine nobody tuned draws the frame drawn without
    /// it, to the byte. (Each frame is a new renderer's first: a frame
    /// drawn after another carries its history.)
    func testTheUntouchedStateDrawsTheStockFrame() throws {
        let device = try XCTUnwrap(MTLCreateSystemDefaultDevice(), "no Metal device")
        let queue = try XCTUnwrap(device.makeCommandQueue())
        let geometry = try LevelGeometry.devmap(donor: nil, cacheFolder: "")
        let camera = ViewCamera(position: [0, 0, 2], yaw: 0, pitch: -0.5, verticalFov: 70 * .pi / 180)
        func capture(_ switches: RenderSwitches?) throws -> CapturedFrame {
            let renderer = try LevelRenderer(device: device, geometry: geometry)
            if let switches {
                renderer.switches = switches
            }
            return try renderer.capture(width: 96, height: 54, camera: camera, queue: queue)
        }
        let engine = Engine()
        let state = DevStateReader().update(generation: engine.devGeneration(), flag: { engine.shellFlag(name: $0) },
                                            value: { engine.shellValue(name: $0) })
        XCTAssertEqual(state, DevState())
        XCTAssertEqual(try capture(state.switches).bytes, try capture(nil).bytes)
    }

    @MainActor
    func testTheDeveloperTabLaunchesTheLightingControls() throws {
        let engine = Engine()
        let model = DeveloperModel(variables: engine.shellVariables(), tuning: engine.tuneControls(),
                                   root: FileManager.default.temporaryDirectory)
        XCTAssertEqual(model.tuning.map(\.name), engine.tuneControls().map(\.name))
        let sun = try XCTUnwrap(model.tuning.first { $0.name == "tune.sun" })
        let exposure = try XCTUnwrap(model.tuning.first { $0.name == "tune.exposure" })
        XCTAssertEqual(model.value(sun), 1)
        model.set(sun, to: 2.5)
        model.set(exposure, to: -99)
        XCTAssertEqual(model.value(exposure), exposure.minimum, "held to its range")
        XCTAssertEqual(model.launch.variables, ["tune.exposure": "-4.0", "tune.sun": "2.5"])
        model.set(sun, to: sun.defaultValue)
        XCTAssertNil(model.launch.variables["tune.sun"], "at its default: the map's tuning keeps it")

        // (the game reads the launch back and the shell takes each value)
        let launched = try XCTUnwrap(DeveloperLaunch(arguments: model.arguments()))
        for (name, value) in launched.settings {
            try engine.setShellVariable(name: name, value: value)
        }
        XCTAssertEqual(read(engine).tuning.exposure, -4)
    }
}
