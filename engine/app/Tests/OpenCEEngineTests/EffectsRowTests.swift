import Foundation
import Metal
import OpenCEEngineCore
import OpenCEEngineKit
import OpenCEEngineRender
import XCTest

/// devgulch's effects row (needs assets/maps): the effects station starts
/// each effect in turn before the player, and each is drawn in its first
/// half second, with its particles and without. An effect whose particles
/// are alive changes the frame, by any amount: the two pictures are of one
/// frame, and the same picture drawn twice is the same to the bit (faint
/// effects, like the shotgun's on skin, change a few pixels by less than
/// the golden frames' tolerance); its frame half a second in is written to build/engine/golden (never
/// committed: it is the game's art) and compared with its approved frame,
/// if there is one, in assets/golden/engine.
final class EffectsRowTests: XCTestCase {
    /// The station's results so far, and whether its run is done, from the
    /// shell's `station`.
    func results(_ engine: Engine) throws -> (results: [[String: Any]], done: Bool) {
        let shown = engine.runCommand(line: "station")
        XCTAssertTrue(shown.ok, shown.text)
        let json = try JSONSerialization.jsonObject(with: Data(shown.json.utf8)) as? [String: Any]
        let run = json?["run"] as? [String: Any]
        return (run?["results"] as? [[String: Any]] ?? [], run?["done"] as? Bool ?? false)
    }

    func testEachEffectOfTheRowIsDrawn() throws {
        let maps = StockLevel.root.appendingPathComponent("assets/maps")
        guard FileManager.default.fileExists(atPath: maps.appendingPathComponent("bloodgulch.map").path) else {
            throw XCTSkip("no assets/maps/bloodgulch.map")
        }
        guard let device = MTLCreateSystemDefaultDevice(), let queue = device.makeCommandQueue() else {
            throw XCTSkip("no Metal device")
        }
        let cache = StockLevel.root.appendingPathComponent("build/engine/cache")
        try FileManager.default.createDirectory(at: cache, withIntermediateDirectories: true)
        let geometry = try LevelGeometry.devgulch(mapsFolder: maps.path, cacheFolder: cache.path)
        let engine = Engine()
        let reader = SnapshotReader(engine: engine)
        _ = try engine.startLevel(geometry: geometry, difficulty: 1)
        try engine.spawnPlayer(start: 0)
        _ = try engine.advance(frameSeconds: 1.0 / 30)
        let renderer = try LevelRenderer(
            device: device, geometry: geometry,
            settings: VideoSettings(antiAliasing: .native, renderScale: 1))
        let started = engine.runCommand(line: "station effects")
        XCTAssertTrue(started.ok, started.text)
        let out = StockLevel.root.appendingPathComponent("build/engine/golden")
        try FileManager.default.createDirectory(at: out, withIntermediateDirectories: true)
        var finished = 0, frames = 0, drawn = 0, alive = 0, unchanged: [Int] = []
        var share = 0.0, largest = 0, laid = 0
        do {
            let frame = reader.acquire()
            let camera = ViewCamera(frame.camera, alpha: frame.alpha)
            renderer.laidParticles = []
            let first = try renderer.capture(
                width: 480, height: 270, camera: camera,
                objects: frame.drawnObjects(), queue: queue)
            let second = try renderer.capture(
                width: 480, height: 270, camera: camera,
                objects: frame.drawnObjects(), queue: queue)
            XCTAssertEqual(
                first.difference(from: second, tolerance: 0)?.largest, 0,
                "the same frame drawn twice differs")
        }
        // (an effect lasts three seconds; it is drawn a tenth, a quarter and
        // half a second in, for sparks that are gone by the half second)
        for _ in 0..<(30 * 900) {
            _ = try engine.advance(frameSeconds: 1.0 / 30)
            frames += 1
            let run = try results(engine)
            if run.results.count != finished {
                if laid > 0 {
                    alive += 1
                    if largest == 0 { unchanged.append(finished) }
                    print(
                        String(
                            format: "effects row: %03d share %.5f largest %d laid %d",
                            finished, share, largest, laid))
                }
                finished = run.results.count
                frames = 0
                share = 0
                largest = 0
                laid = 0
            }
            if run.done { break }
            guard [3, 8, 15].contains(frames) else { continue }
            let frame = reader.acquire()
            let camera = ViewCamera(frame.camera, alpha: frame.alpha)
            renderer.time = Float((Double(frame.tick) + Double(frame.alpha)) / 60)
            renderer.laidParticles = []
            let without = try renderer.capture(
                width: 480, height: 270, camera: camera,
                objects: frame.drawnObjects(), queue: queue)
            renderer.laidParticles = Array(frame.particles)
            let with = try renderer.capture(
                width: 480, height: 270, camera: camera,
                objects: frame.drawnObjects(), queue: queue)
            laid = max(laid, renderer.laidParticles.count)
            share = max(share, with.difference(from: without)?.share ?? 0)
            largest = max(largest, with.difference(from: without, tolerance: 0)?.largest ?? 0)
            guard frames == 15 else { continue }
            drawn += 1
            let file = String(format: "devgulch_effects_%03d_480x270.png", finished)
            try with.writePNG(to: out.appendingPathComponent(file))
            let approved = StockLevel.root.appendingPathComponent("assets/golden/engine/\(file)")
            if let approvedFrame = CapturedFrame.readPNG(at: approved) {
                let difference = try XCTUnwrap(with.difference(from: approvedFrame), file)
                XCTAssertLessThan(difference.share, 0.005, "\(file) differs from the approved frame")
            }
        }
        let done = try results(engine).results
        print("effects row: \(done.count) effects, \(drawn) drawn, \(alive) with particles, unchanged \(unchanged)")
        for index in unchanged where index < done.count {
            print("unchanged: \(index) \(done[index]["effect"] ?? "") particles \(done[index]["particles"] ?? 0)")
        }
        XCTAssertGreaterThan(done.count, 0, "the row has no effects")
        XCTAssertGreaterThan(alive, 0, "no effect's particles were alive in its first half second")
        XCTAssertTrue(unchanged.isEmpty, "the particles of effects \(unchanged) changed nothing in the frame")
    }
}
