import Foundation
import Metal
import OpenCEEngineCore
@testable import OpenCEEngineRender
import QuartzCore
import XCTest

/// The Halo UI from ui.map (assets/maps, never committed): its main menu
/// drawn over the level's own structure, and the UI pass's cost at 4K.
final class UiTests: XCTestCase {
    struct UiLevel {
        let device: MTLDevice
        let queue: MTLCommandQueue
        let geometry: LevelGeometry
        let menu: Menu

        init() throws {
            let map = StockLevel.root.appendingPathComponent("assets/maps/ui.map")
            guard FileManager.default.fileExists(atPath: map.path) else {
                throw XCTSkip("no assets/maps/ui.map")
            }
            guard let device = MTLCreateSystemDefaultDevice(), let queue = device.makeCommandQueue() else {
                throw XCTSkip("no Metal device")
            }
            let cache = StockLevel.root.appendingPathComponent("build/engine/cache")
            try FileManager.default.createDirectory(at: cache, withIntermediateDirectories: true)
            self.device = device
            self.queue = queue
            geometry = try LevelGeometry.load(map: map.path, cacheFolder: cache.path, bsp: 0)
            menu = Menu(geometry: geometry)
            XCTAssertTrue(menu.open(widget: mainMenuWidget()))
            XCTAssertTrue(menu.isOpen())
        }

        var camera: ViewCamera {
            let start = geometry.startView()
            return ViewCamera(position: [start.x, start.y, start.z], yaw: start.yaw, pitch: start.pitch,
                              verticalFov: 70 * .pi / 180)
        }
    }

    func testTheMainMenuIsDrawnOverItsScene() throws {
        let level = try UiLevel()
        let renderer = try LevelRenderer(device: level.device, geometry: level.geometry,
                                         settings: VideoSettings(antiAliasing: .native, renderScale: 1))
        renderer.ui = try UiPass(device: level.device, atlas: level.menu.glyphAtlas(), outputFormat: .bgra8Unorm)
        let (width, height) = (1920, 1080)
        level.menu.update(nowMs: 0)
        let quads = level.menu.draw(width: UInt32(width), height: UInt32(height))
        XCTAssertGreaterThan(quads.filter(\.glyph).count, 10, "the menu's text")
        XCTAssertGreaterThan(quads.filter { !$0.glyph }.count, 0, "the menu's panels")

        let scene = try renderer.capture(width: width, height: height, camera: level.camera, queue: level.queue)
        let drawn = try renderer.capture(width: width, height: height, camera: level.camera, uiQuads: quads,
                                         queue: level.queue)
        let ui = try XCTUnwrap(renderer.ui)
        XCTAssertEqual(ui.lastMissing, 0, "quads whose bitmap is not among the level's textures")
        XCTAssertEqual(ui.lastDrawn, quads.count)
        let changed = try XCTUnwrap(drawn.difference(from: scene))
        XCTAssertGreaterThan(changed.share, 0.01, "the menu is not drawn")

        let out = StockLevel.root.appendingPathComponent("build/engine/golden")
        try FileManager.default.createDirectory(at: out, withIntermediateDirectories: true)
        let name = "ui_main_menu_\(width)x\(height).png"
        try drawn.writePNG(to: out.appendingPathComponent(name))
        let approved = StockLevel.root.appendingPathComponent("assets/golden/engine/\(name)")
        if let approvedFrame = CapturedFrame.readPNG(at: approved) {
            let difference = try XCTUnwrap(drawn.difference(from: approvedFrame))
            XCTAssertLessThan(difference.share, 0.005, "\(name) differs from the approved frame")
        } else {
            print("golden frame \(name): none approved yet (assets/golden/engine)")
        }
    }

    /// The keys and the pointer move the main menu's focus, with the
    /// cursor's sound.
    func testTheKeysAndThePointerMoveTheFocus() throws {
        let level = try UiLevel()
        let cursor = "sound\\sfx\\ui\\cursor"
        _ = level.menu.takeSounds()
        level.menu.key(key: .down)
        XCTAssertEqual(level.menu.takeSounds(), [cursor])
        level.menu.key(key: .up)
        XCTAssertEqual(level.menu.takeSounds(), [cursor])
        // somewhere on the screen an item other than the focused one is
        var moved = false
        for y in stride(from: 0, to: 1080, by: 20) where !moved {
            for x in stride(from: 0, to: 1920, by: 40) {
                level.menu.pointerMoved(x: Float(x), y: Float(y), width: 1920, height: 1080)
                if level.menu.takeSounds().contains(cursor) {
                    moved = true
                    break
                }
            }
        }
        XCTAssertTrue(moved, "no item under the pointer took the focus")
        XCTAssertTrue(level.menu.isOpen())
    }

    /// The menu's update, its quads and their pass over a 4K frame: under a
    /// millisecond on the CPU and on the GPU, on average.
    func testTheUiPassTakesUnderAMillisecondAt4K() throws {
        let level = try UiLevel()
        let renderer = try LevelRenderer(device: level.device, geometry: level.geometry)
        let pass = try UiPass(device: level.device, atlas: level.menu.glyphAtlas(), outputFormat: .bgra8Unorm)
        let (width, height) = (3840, 2160)
        let descriptor = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: .bgra8Unorm, width: width,
                                                                  height: height, mipmapped: false)
        descriptor.usage = [.renderTarget]
        descriptor.storageMode = .private
        let target = try XCTUnwrap(level.device.makeTexture(descriptor: descriptor))
        let frames = 60
        var cpu: Double = 0
        var gpu: Double = 0
        for frame in 0..<frames {
            let start = CACurrentMediaTime()
            level.menu.update(nowMs: UInt64(frame) * 16)
            let quads = level.menu.draw(width: UInt32(width), height: UInt32(height))
            let commands = try XCTUnwrap(level.queue.makeCommandBuffer())
            let render = MTLRenderPassDescriptor()
            render.colorAttachments[0].texture = target
            render.colorAttachments[0].loadAction = .clear
            render.colorAttachments[0].storeAction = .store
            let encoder = try XCTUnwrap(commands.makeRenderCommandEncoder(descriptor: render))
            pass.encode(quads, textures: renderer.levelTextures, width: width, height: height, into: encoder)
            encoder.endEncoding()
            commands.commit()
            cpu += CACurrentMediaTime() - start
            commands.waitUntilCompleted()
            XCTAssertGreaterThan(pass.lastDrawn, 0)
            gpu += commands.gpuEndTime - commands.gpuStartTime
        }
        let cpuMs = cpu / Double(frames) * 1000, gpuMs = gpu / Double(frames) * 1000
        print("UI at 4K: \(String(format: "%.3f", cpuMs)) ms CPU, \(String(format: "%.3f", gpuMs)) ms GPU")
        XCTAssertLessThan(cpuMs, 1)
        XCTAssertLessThan(gpuMs, 1)
    }
}
