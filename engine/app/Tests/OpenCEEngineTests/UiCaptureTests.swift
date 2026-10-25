import Foundation
import Metal
import OpenCEEngineCore
@testable import OpenCEEngineRender
import XCTest

/// The reference captures of the original menus (--capture-ui): every
/// screen of ui.map drawn headless, and the palette sampled from them.
/// They stay in assets/ui_reference, which git ignores.
final class UiCaptureTests: XCTestCase {
    /// A frame of `width` by `height` in one colour, with the texels
    /// `painted` says in another.
    func frame(
        width: Int, height: Int, _ base: SIMD3<UInt8>, _ other: SIMD3<UInt8>,
        painted: (Int, Int) -> Bool
    ) -> CapturedFrame {
        var bytes = [UInt8](repeating: 255, count: width * height * 4)
        for y in 0..<height {
            for x in 0..<width {
                let colour = painted(x, y) ? other : base
                let at = (y * width + x) * 4
                bytes[at] = colour.z
                bytes[at + 1] = colour.y
                bytes[at + 2] = colour.x
            }
        }
        return CapturedFrame(width: width, height: height, bytes: bytes)
    }

    func testThePaletteIsTheFramesCommonestColoursByShare() {
        let blue = SIMD3<UInt8>(20, 40, 120)
        let amber = SIMD3<UInt8>(230, 170, 40)
        let drawn = frame(width: 8, height: 8, blue, amber) { x, _ in x >= 6 }
        let swatches = UiPalette.sample([(drawn, nil)], count: 4)
        XCTAssertEqual(swatches.count, 2)
        XCTAssertEqual(swatches[0].rgb, blue)
        XCTAssertEqual(swatches[0].share, 0.75, accuracy: 1e-9)
        XCTAssertEqual(swatches[1].rgb, amber)
        XCTAssertEqual(swatches[1].hex, "#E6AA28")
    }

    func testThePaletteCountsOnlyWhatTheMenuDrewOverItsScene() {
        let sky = SIMD3<UInt8>(90, 120, 160)
        let panel = SIMD3<UInt8>(10, 30, 70)
        let scene = frame(width: 8, height: 8, sky, sky) { _, _ in false }
        let drawn = frame(width: 8, height: 8, sky, panel) { _, y in y < 2 }
        let swatches = UiPalette.sample([(drawn, scene)], count: 4)
        XCTAssertEqual(swatches.map(\.rgb), [panel])
        XCTAssertEqual(swatches[0].share, 1, accuracy: 1e-9)
    }

    func testThePaletteJoinsNearColoursAndKeepsTheCount() {
        let near = frame(width: 10, height: 10, SIMD3(100, 100, 100), SIMD3(102, 101, 100)) { x, _ in x < 5 }
        let joined = UiPalette.sample([(near, nil)], count: 4)
        XCTAssertEqual(joined.count, 1, "colours within a bucket are one swatch")
        let many = frame(width: 16, height: 16, SIMD3(0, 0, 0), SIMD3(0, 0, 0)) { _, _ in false }
        var bytes = many.bytes
        for index in 0..<256 { bytes[index * 4 + 2] = UInt8(index) }
        let ramp = CapturedFrame(width: 16, height: 16, bytes: bytes)
        XCTAssertEqual(UiPalette.sample([(ramp, nil)], count: 5).count, 5)
    }

    func testTheReferenceFolderIsIgnoredByGit() throws {
        let git = Process()
        git.executableURL = URL(fileURLWithPath: "/usr/bin/git")
        git.currentDirectoryURL = StockLevel.root
        git.arguments = ["check-ignore", "-q", "assets/ui_reference/palette.json"]
        try git.run()
        git.waitUntilExit()
        XCTAssertEqual(git.terminationStatus, 0, "assets/ui_reference must never be committed")
        XCTAssertEqual(
            UiCapture.defaultFolder(root: StockLevel.root).path,
            StockLevel.root.appendingPathComponent("assets/ui_reference").path)
    }

    func testAScreensFileIsNamedByItsPathUnderTheShell() {
        XCTAssertEqual(UiCapture.fileName(screen: "ui\\shell\\main_menu\\main_menu"), "main_menu-main_menu.png")
        XCTAssertEqual(
            UiCapture.fileName(screen: "ui\\shell\\solo_game\\pause_game\\pause_game"),
            "solo_game-pause_game-pause_game.png")
        XCTAssertEqual(UiCapture.fileName(screen: "ui/odd name"), "ui-odd_name.png")
    }

    func testEveryScreenOfTheShellIsCaptured() throws {
        let map = StockLevel.root.appendingPathComponent("assets/maps/ui.map")
        guard FileManager.default.fileExists(atPath: map.path) else { throw XCTSkip("no assets/maps/ui.map") }
        guard let device = MTLCreateSystemDefaultDevice(), let queue = device.makeCommandQueue() else {
            throw XCTSkip("no Metal device")
        }
        let cache = StockLevel.root.appendingPathComponent("build/engine/cache")
        try FileManager.default.createDirectory(at: cache, withIntermediateDirectories: true)
        let out = FileManager.default.temporaryDirectory.appendingPathComponent("ui_capture_\(UUID().uuidString)")
        defer { try? FileManager.default.removeItem(at: out) }
        let geometry = try LevelGeometry.load(map: map.path, cacheFolder: cache.path, bsp: 0)
        let capture = UiCapture(device: device, queue: queue, width: 640, height: 480)
        let report = try capture.capture(geometry: geometry, widget: mainMenuWidget(), into: out)
        let files = try FileManager.default.contentsOfDirectory(atPath: out.path)
        for leaf in [
            "main_menu", "difficulty_select_list_screen", "multiplayer_type_select_screen",
            "server_list_screen", "player_profile_select_screen",
        ] {
            XCTAssertTrue(files.contains { $0.hasSuffix("\(leaf).png") }, "\(leaf) not captured: \(files)")
        }
        XCTAssertEqual(report.screens.count, files.filter { $0.hasSuffix(".png") }.count)
        XCTAssertGreaterThanOrEqual(report.palette.count, 4, "the palette has the menus' colours")
        let first = try XCTUnwrap(CapturedFrame.readPNG(at: out.appendingPathComponent(report.screens[0])))
        XCTAssertEqual(first.width, 640)
        XCTAssertGreaterThan(first.distinctColors, 50, "a screen drawn, not a cleared frame")
    }
}
