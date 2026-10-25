import Foundation
import Metal
import OpenCEEngineCore
import OpenCEEngineKit
import OpenCEEngineRender
import XCTest

/// devgulch's stations, each seen as its player sees it a second after it
/// starts (needs assets/maps): the frame drawn twice is the same to the
/// bit, and it is written to build/engine/golden (never committed: it is
/// the game's art) and compared with its approved frame, if there is one,
/// in assets/golden/engine (`tools/engine_golden.py approve` approves it).
final class StationFramesTests: XCTestCase {
    func stations(_ engine: Engine) throws -> [String] {
        let shown = engine.runCommand(line: "station")
        XCTAssertTrue(shown.ok, shown.text)
        let json = try JSONSerialization.jsonObject(with: Data(shown.json.utf8)) as? [String: Any]
        return json?["stations"] as? [String] ?? []
    }

    func testEachStationMatchesItsApprovedFrame() throws {
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
        let renderer = try LevelRenderer(device: device, geometry: geometry,
                                         settings: VideoSettings(antiAliasing: .native, renderScale: 1))
        // (a renderer's first frame with particles differs from its second
        // by a level in a few pixels, so one is drawn and thrown away first)
        do {
            let frame = reader.acquire()
            renderer.laidParticles = Array(frame.particles)
            _ = try renderer.capture(width: 480, height: 270, camera: ViewCamera(frame.camera, alpha: frame.alpha),
                                     objects: frame.drawnObjects(), queue: queue)
        }
        let out = StockLevel.root.appendingPathComponent("build/engine/golden")
        try FileManager.default.createDirectory(at: out, withIntermediateDirectories: true)
        // (the vehicles station seats the player, and stopping it leaves them
        // seated, which the others refuse: it is seen last)
        let names = try stations(engine).sorted { ($0 == "vehicles" ? 1 : 0) < ($1 == "vehicles" ? 1 : 0) }
        XCTAssertGreaterThan(names.count, 0, "devgulch has no stations")
        var compared = 0
        for name in names {
            let started = engine.runCommand(line: "station \(name)")
            XCTAssertTrue(started.ok, "\(name): \(started.text)")
            for _ in 0..<30 { _ = try engine.advance(frameSeconds: 1.0 / 30) }
            let frame = reader.acquire()
            let camera = ViewCamera(frame.camera, alpha: frame.alpha)
            renderer.time = Float((Double(frame.tick) + Double(frame.alpha)) / 60)
            renderer.laidParticles = Array(frame.particles)
            let shot = try renderer.capture(width: 480, height: 270, camera: camera,
                                            objects: frame.drawnObjects(), queue: queue)
            let again = try renderer.capture(width: 480, height: 270, camera: camera,
                                             objects: frame.drawnObjects(), queue: queue)
            XCTAssertEqual(shot.difference(from: again, tolerance: 0)?.largest, 0,
                           "\(name): the same frame drawn twice differs")
            let file = "devgulch_station_\(name)_480x270.png"
            try shot.writePNG(to: out.appendingPathComponent(file))
            let approved = StockLevel.root.appendingPathComponent("assets/golden/engine/\(file)")
            if let approvedFrame = CapturedFrame.readPNG(at: approved) {
                let difference = try XCTUnwrap(shot.difference(from: approvedFrame), file)
                print(String(format: "station %@: share %.5f largest %d", name, difference.share, difference.largest))
                XCTAssertLessThan(difference.share, 0.005, "\(file) differs from the approved frame")
                compared += 1
            } else {
                print("golden frame \(file): none approved yet (assets/golden/engine)")
            }
            _ = engine.runCommand(line: "station stop")
        }
        print("stations: \(names.count) drawn, \(compared) compared with approved frames")
    }
}
