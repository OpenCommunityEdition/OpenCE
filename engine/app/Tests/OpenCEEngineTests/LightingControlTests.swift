import Foundation
import Metal
import OpenCEEngineCore
import OpenCEEngineKit
import OpenCEEngineRender
import XCTest

/// The lighting controls (`tune.*`) on Blood Gulch's level (devgulch's):
/// each moves the frame the way its name says, and reset (or a state
/// nobody tuned) draws the stock frame to the byte.
final class LightingControlTests: XCTestCase {
    /// The frame of a new renderer (a frame drawn after another carries its
    /// history), lit by `bake` if one is given, with `change` made to the
    /// stock controls.
    func capture(
        _ level: StockLevel, bake: BakedLight? = nil,
        _ change: (inout LightingTuning) -> Void = { _ in }
    ) throws -> CapturedFrame {
        let renderer = try level.renderer(VideoSettings(antiAliasing: .native, renderScale: 1))
        if let bake {
            try renderer.useBakedLighting(bake)
        }
        var tuning = LightingTuning()
        change(&tuning)
        renderer.tuning = tuning
        return try renderer.capture(
            width: 480, height: 270, camera: level.camera, objects: level.objects,
            queue: level.queue)
    }

    /// A bake lit evenly, as the sun's shadows split it.
    func evenBake(_ level: StockLevel) throws -> BakedLight {
        let plan = try level.geometry.bakePlan(quick: true)
        func even(_ points: Data, _ value: Float) -> Data {
            let floats = [Float](repeating: value, count: points.count / 32 * 4)
            return floats.withUnsafeBytes { Data($0) }
        }
        return plan.finish(
            irradiance: even(plan.lightmapPointBytes(), 2),
            directions: even(plan.lightmapPointBytes(), 0),
            probes: even(plan.probePointBytes(), 0.5),
            radiance: even(plan.reflectionPointBytes(), 0))
    }

    func testEachControlMovesTheFrameItsWay() throws {
        let level = try StockLevel(map: "bloodgulch")
        let frame = try capture(level)
        let stock = frame.meanBrightness
        // (by `by` of 255 at least: the bloom and the occlusion change
        // little of the start view, an open valley with little bright sky)
        func brighter(_ name: String, by: Double = 0.5, _ change: (inout LightingTuning) -> Void) throws {
            XCTAssertGreaterThan(try capture(level, change).meanBrightness, stock + by, "\(name) brightens")
        }
        func darker(_ name: String, by: Double = 0.5, _ change: (inout LightingTuning) -> Void) throws {
            XCTAssertLessThan(try capture(level, change).meanBrightness, stock - by, "\(name) darkens")
        }
        try brighter("exposure up") { $0.exposure = 1 }
        try darker("exposure down") { $0.exposure = -1 }
        try brighter("more bounce") { $0.bounce = 2 }
        try darker("less bounce") { $0.bounce = 0.5 }
        try brighter("more bloom", by: 0.05) { $0.bloom = 3 }
        try darker("no bloom", by: 0.02) { $0.bloom = 0 }
        try brighter("no occlusion", by: 0.001) { $0.ambientOcclusion = 0 }
        try darker("more occlusion", by: 0.001) { $0.ambientOcclusion = 2 }
        XCTAssertLessThan(
            try capture(level) { $0.emissive = 0 }.meanBrightness, stock + 1e-9,
            "without their own light no surface is brighter")
        XCTAssertLessThan(try capture(level) { $0.probes = 0 }.meanBrightness, stock, "the objects unlit")
        let curved = try capture(level) { $0.toneCurve = 2 }
        XCTAssertGreaterThan(try XCTUnwrap(curved.difference(from: frame)).share, 0.01, "the tone curve")
        XCTAssertGreaterThan(curved.contrast, frame.contrast, "more contrast")

        // (the sun's light where the bake knows it: brighter for more of
        // it, less red for less red in it)
        let bake = try evenBake(level)
        let baked = try capture(level, bake: bake)
        XCTAssertGreaterThan(
            try capture(level, bake: bake) { $0.sun = 2 }.meanBrightness,
            baked.meanBrightness + 0.5, "more sun")
        let blue = try capture(level, bake: bake) { $0.sunColor = SIMD3(0, 1, 1) }
        XCTAssertLessThan(blue.channelMean(0), baked.channelMean(0) - 0.5, "no red in the sun")
        XCTAssertGreaterThan(blue.channelMean(2), baked.channelMean(2) - 0.1, "its blue kept")
    }

    /// Reset after tuning, as the shell does it, and the frame is the
    /// stock frame again; the stock values set by hand draw it too.
    func testResetDrawsTheStockFrame() throws {
        let level = try StockLevel(map: "bloodgulch")
        let frame = try capture(level)
        let engine = Engine()
        XCTAssertTrue(engine.runCommand(line: "set tune.exposure 2").ok)
        XCTAssertTrue(engine.runCommand(line: "set tune.ao 0").ok)
        func read() -> LightingTuning {
            DevState(flag: { engine.shellFlag(name: $0) }, value: { engine.shellValue(name: $0) }).tuning
        }
        let tuned = read()
        XCTAssertNotEqual(try capture(level) { $0 = tuned }.bytes, frame.bytes)
        XCTAssertTrue(engine.runCommand(line: "tune reset").ok)
        let reset = read()
        XCTAssertTrue(reset.isStock)
        XCTAssertEqual(try capture(level) { $0 = reset }.bytes, frame.bytes, "reset: the stock frame")
        XCTAssertEqual(
            try capture(level) {
                $0.exposure = 0; $0.bloom = 1; $0.toneCurve = 1
            }.bytes, frame.bytes)
    }
}

extension CapturedFrame {
    /// The mean of one channel (0 red, 1 green, 2 blue) of every texel,
    /// 0 to 255.
    func channelMean(_ channel: Int) -> Double {
        // (the frame's texels are blue, green, red, alpha)
        let offset = 2 - channel
        var sum = 0
        for index in stride(from: offset, to: bytes.count, by: 4) {
            sum += Int(bytes[index])
        }
        return Double(sum) / Double(max(width * height, 1))
    }

    /// The standard deviation of the texels' brightness.
    var contrast: Double {
        let mean = meanBrightness
        var sum = 0.0
        for index in stride(from: 0, to: bytes.count, by: 4) {
            let value = Double(Int(bytes[index]) + Int(bytes[index + 1]) + Int(bytes[index + 2])) / 3 - mean
            sum += value * value
        }
        return (sum / Double(max(width * height, 1))).squareRoot()
    }
}
