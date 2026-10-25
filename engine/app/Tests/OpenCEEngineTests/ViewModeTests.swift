import Foundation
import Metal
import OpenCEEngineCore
import OpenCEEngineKit
import OpenCEEngineRender
import simd
import XCTest

/// The developer's views of the level and the drawing switches the shell
/// offers, on Blood Gulch's level (devgulch's): each view and each switch
/// gives its own frame, written to build/engine/golden for approval, and
/// the stock view lights a baked level with the map's own lightmaps.
final class ViewModeTests: XCTestCase {
    /// The frame of a new renderer (a frame drawn after another carries its
    /// history), with `change` made to the stock switches.
    func capture(
        _ level: StockLevel, bake: BakedLight? = nil, hud: [HudQuad] = [], objects: [DrawnObject]? = nil,
        _ change: (inout RenderSwitches) -> Void = { _ in }
    ) throws -> CapturedFrame {
        let renderer = try level.renderer(VideoSettings(antiAliasing: .native, renderScale: 1))
        if let bake {
            try renderer.useBakedLighting(bake)
        }
        if !hud.isEmpty {
            let art = try HudArt.load(
                folder: StockLevel.root.appendingPathComponent("port/assets/hud"),
                device: level.device, queue: level.queue)
            renderer.hud = try HudPass(device: level.device, art: art, outputFormat: .bgra8Unorm)
        }
        var switches = RenderSwitches()
        change(&switches)
        renderer.switches = switches
        return try renderer.capture(
            width: 480, height: 270, camera: level.camera, objects: objects ?? level.objects,
            hudQuads: hud, queue: level.queue)
    }

    func share(_ a: CapturedFrame, _ b: CapturedFrame) throws -> Double {
        try XCTUnwrap(a.difference(from: b)).share
    }

    func write(_ frame: CapturedFrame, _ name: String) throws {
        let out = StockLevel.root.appendingPathComponent("build/engine/golden")
        try FileManager.default.createDirectory(at: out, withIntermediateDirectories: true)
        try frame.writePNG(to: out.appendingPathComponent("devgulch_\(name)_480x270.png"))
    }

    func testEachViewAndSwitchGivesItsOwnFrame() throws {
        let level = try StockLevel(map: "bloodgulch")
        let frame = try capture(level)
        try write(frame, "frame")
        var frames: [(String, CapturedFrame)] = []
        for view in [RenderView.albedo, .light] {
            frames.append((view.rawValue, try capture(level) { $0.view = view }))
        }
        frames.append(("wireframe", try capture(level) { $0.wireframe = true }))
        frames.append(("no_lightmaps", try capture(level) { $0.lightmaps = false }))
        frames.append(("no_objects", try capture(level) { $0.objects = false }))
        for (name, drawn) in frames {
            try write(drawn, name)
            // (few objects stand in the start view)
            XCTAssertGreaterThan(
                try share(drawn, frame), name == "no_objects" ? 0.0005 : 0.01,
                "\(name) draws the stock frame")
        }
        for (index, (name, drawn)) in frames.enumerated() {
            for (other, otherDrawn) in frames[(index + 1)...] {
                XCTAssertGreaterThan(try share(drawn, otherDrawn), 0.005, "\(name) draws \(other)'s frame")
            }
        }
        // (the objects switched off: the frame drawn without them)
        XCTAssertEqual(try capture(level) { $0.objects = false }.bytes, try capture(level, objects: []).bytes)
        // (the lightmaps off: no darker anywhere than the albedo lit flat)
        let flat = try XCTUnwrap(frames.first { $0.0 == "no_lightmaps" }).1
        XCTAssertGreaterThan(flat.meanBrightness, frame.meanBrightness, "lit flat, the level is brighter")
    }

    /// The stock view of a baked level draws it as the unbaked renderer
    /// does, with the map's lightmaps; the baked frame does not.
    func testTheStockViewDrawsTheMapsLightmaps() throws {
        let level = try StockLevel(map: "bloodgulch")
        let plan = try level.geometry.bakePlan(quick: true)
        func even(_ points: Data, _ value: Float) -> Data {
            let floats = [Float](repeating: value, count: points.count / 32 * 4)
            return floats.withUnsafeBytes { Data($0) }
        }
        let bake = plan.finish(
            irradiance: even(plan.lightmapPointBytes(), 2),
            directions: even(plan.lightmapPointBytes(), 0),
            probes: even(plan.probePointBytes(), 0.5),
            radiance: even(plan.reflectionPointBytes(), 0))
        let stockFrame = try capture(level, objects: [])
        let baked = try capture(level, bake: bake, objects: [])
        let stock = try capture(level, bake: bake, objects: []) { $0.view = .stock }
        try write(stock, "stock")
        XCTAssertGreaterThan(try share(baked, stockFrame), 0.1, "the bake changed the frame")
        XCTAssertLessThan(try share(stock, stockFrame), 0.01, "the stock view is the map's own lighting")
        // (unbaked, the stock view is the frame)
        XCTAssertEqual(try capture(level, objects: []) { $0.view = .stock }.bytes, stockFrame.bytes)
    }

    func testTheHudCanBeSwitchedOff() throws {
        let level = try StockLevel(map: "bloodgulch")
        let engine = Engine()
        _ = try engine.startLevel(geometry: level.geometry, difficulty: 1)
        try engine.spawnPlayer(start: 0)
        _ = try engine.step()
        let quads = engine.hud(width: 480, height: 270)
        XCTAssertGreaterThan(quads.count, 10)
        let bare = try capture(level)
        let drawn = try capture(level, hud: quads)
        let hidden = try capture(level, hud: quads) { $0.hud = false }
        XCTAssertGreaterThan(try share(drawn, bare), 0.005, "the HUD is drawn")
        XCTAssertEqual(hidden.bytes, bare.bytes, "switched off, no HUD")
    }
}

extension CapturedFrame {
    /// The mean of every texel's red, green and blue, 0 to 255.
    var meanBrightness: Double {
        var sum = 0
        for index in stride(from: 0, to: bytes.count, by: 4) {
            sum += Int(bytes[index]) + Int(bytes[index + 1]) + Int(bytes[index + 2])
        }
        return Double(sum) / Double(max(width * height * 3, 1))
    }
}
