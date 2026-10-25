import Foundation
import Metal
import OpenCEEngineCore
import OpenCEEngineKit
import OpenCEEngineMenu
import OpenCEEngineRender
import XCTest

/// The HUD's state message on Blood Gulch: the stock keys as its controls'
/// names, nothing at the start, and by a Warthog its words drawn in the
/// top left with the menus' glyph pass.
final class HudMessageTests: XCTestCase {
    func testTheStockKeysNameTheControls() {
        XCTAssertEqual(KeyBindings.stock.promptKeys, [
            "E", "Right Button", "Left Button", "Q", "Space", "None", "Tab", "G", "Control", "Z",
            "Return", "Escape", "WASD", "Mouse",
        ])
    }

    func testByAWarthogItsMessageIsDrawnTopLeft() throws {
        let level = try StockLevel(map: "bloodgulch")
        let renderer = try level.renderer(VideoSettings(antiAliasing: .native, renderScale: 1))
        renderer.ui = try UiPass(device: level.device, atlas: Menu(geometry: level.geometry).glyphAtlas(),
                                   outputFormat: .bgra8Unorm)
        let engine = Engine()
        let reader = SnapshotReader(engine: engine)
        _ = try engine.startLevel(geometry: level.geometry, difficulty: 1)
        try engine.spawnPlayer(start: 0)
        for _ in 0..<30 {
            _ = try engine.step()
        }
        let keys = KeyBindings.stock.promptKeys
        let (width, height) = (1920, 1080)
        XCTAssertEqual(engine.hudText(width: UInt32(width), height: UInt32(height), keys: keys).glyphs, [],
                       "a message at the start")

        let found = engine.runCommand(line: "objects warthog")
        XCTAssertTrue(found.ok, found.text)
        let rows = try XCTUnwrap(JSONSerialization.jsonObject(with: Data(found.json.utf8)) as? [[String: Any]])
        let at = try XCTUnwrap(rows.first?["position"] as? [Double], "no warthog")
        // (round it, a step at a time, until a seat's entrance offers itself)
        var message = HudText(glyphs: [], icons: [])
        for step in 0..<24 where message.glyphs.isEmpty {
            let angle = Double(step) * .pi / 6, radius = step < 12 ? 1.5 : 2.5
            let moved = engine.runCommand(line: "teleport \(at[0] + radius * cos(angle)) "
                                              + "\(at[1] + radius * sin(angle)) \(at[2] + 0.5)")
            XCTAssertTrue(moved.ok, moved.text)
            for _ in 0..<20 {
                _ = try engine.step()
            }
            message = engine.hudText(width: UInt32(width), height: UInt32(height), keys: keys)
        }
        XCTAssertGreaterThan(message.glyphs.count, 10, "no message by the warthog")

        let frame = reader.acquire()
        let camera = ViewCamera(frame.camera, alpha: 1)
        let bare = try renderer.capture(width: width, height: height, camera: camera, queue: level.queue)
        let drawn = try renderer.capture(width: width, height: height, camera: camera, uiQuads: message.glyphs,
                                         queue: level.queue)
        // (its words: the top left changed, the rest of the frame not)
        func changed(_ x0: Int, _ y0: Int, _ x1: Int, _ y1: Int) -> Int {
            var count = 0
            for y in y0..<y1 {
                for x in x0..<x1 where drawn.rgb(x: x, y: y) != bare.rgb(x: x, y: y) {
                    count += 1
                }
            }
            return count
        }
        XCTAssertGreaterThan(changed(0, 100, 800, 220), 2000, "the message is not drawn")
        XCTAssertEqual(changed(0, height / 2, width, height), 0, "drawn below the top half")
    }
}
