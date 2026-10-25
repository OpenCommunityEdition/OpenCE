import Foundation
import Metal
import OpenCEEngineCore
import OpenCEEngineKit
import OpenCEEngineRender
import simd
import XCTest

/// The first-person view and the HUD on Blood Gulch: the player at the
/// first start, two seconds on, the rifle and hands drawn over the world
/// and the HUD from its redraws over them, written as a golden frame to
/// build/engine/golden for approval.
final class HudTests: XCTestCase {
    func testTheRifleAndHudAreDrawn() throws {
        let level = try StockLevel(map: "bloodgulch")
        let renderer = try level.renderer(VideoSettings(antiAliasing: .native, renderScale: 1))
        let folder = StockLevel.root.appendingPathComponent("port/assets/hud")
        let art = try HudArt.load(folder: folder, device: level.device, queue: level.queue)
        XCTAssertEqual(art.count, 73)
        renderer.hud = try HudPass(device: level.device, art: art, outputFormat: .bgra8Unorm)

        let engine = Engine()
        let reader = SnapshotReader(engine: engine)
        _ = try engine.startLevel(geometry: level.geometry, difficulty: 1)
        try engine.spawnPlayer(start: 0)
        for _ in 0..<60 {
            _ = try engine.step()
        }
        let frame = reader.acquire()
        let objects = frame.drawnObjects()
        XCTAssertEqual(objects.filter(\.firstPerson).count, 2)
        let camera = ViewCamera(frame.camera, alpha: 1)
        let (width, height) = (1920, 1080)
        let quads = engine.hud(width: UInt32(width), height: UInt32(height))
        XCTAssertGreaterThan(quads.count, 10)

        let bare = try renderer.capture(
            width: width, height: height, camera: camera,
            objects: objects.filter { !$0.firstPerson }, queue: level.queue)
        let armed = try renderer.capture(
            width: width, height: height, camera: camera, objects: objects,
            queue: level.queue)
        XCTAssertGreaterThan(renderer.lastFirstPersonPartCount, 0)
        let drawn = try renderer.capture(
            width: width, height: height, camera: camera, objects: objects,
            hudQuads: quads, queue: level.queue)
        let hud = try XCTUnwrap(renderer.hud)
        XCTAssertEqual(hud.lastMissing, [], "quads without a redraw")
        XCTAssertEqual(hud.lastDrawn, quads.count)

        // the rifle: the lower right of the frame changed
        let rifle = try XCTUnwrap(armed.difference(from: bare))
        XCTAssertGreaterThan(rifle.share, 0.03, "the rifle is not drawn")
        // the HUD: the shield panel (top right) and the rounds (top left)
        // changed, and the frame's middle (the crosshair)
        func changed(_ x0: Int, _ y0: Int, _ x1: Int, _ y1: Int) -> Int {
            var count = 0
            for y in y0..<y1 {
                for x in x0..<x1 where drawn.rgb(x: x, y: y) != armed.rgb(x: x, y: y) {
                    count += 1
                }
            }
            return count
        }
        XCTAssertGreaterThan(changed(1500, 0, 1920, 200), 2000, "no shield panel")
        XCTAssertGreaterThan(changed(0, 0, 400, 200), 2000, "no rounds")
        XCTAssertGreaterThan(changed(880, 460, 1040, 620), 200, "no crosshair")

        let out = StockLevel.root.appendingPathComponent("build/engine/golden")
        try FileManager.default.createDirectory(at: out, withIntermediateDirectories: true)
        let name = "bloodgulch_hud_\(width)x\(height).png"
        try drawn.writePNG(to: out.appendingPathComponent(name))
        let approved = StockLevel.root.appendingPathComponent("assets/golden/engine/\(name)")
        if let approvedFrame = CapturedFrame.readPNG(at: approved) {
            let difference = try XCTUnwrap(drawn.difference(from: approvedFrame))
            XCTAssertLessThan(difference.share, 0.005, "\(name) differs from the approved frame")
        } else {
            print("golden frame \(name): none approved yet (assets/golden/engine)")
        }
    }
}
