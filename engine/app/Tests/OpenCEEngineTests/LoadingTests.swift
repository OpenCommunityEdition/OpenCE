import AVFoundation
import Foundation
import Metal
import OpenCEEngineCore
import OpenCEEngineKit
import OpenCEEngineMenu
@testable import OpenCEEngineRender
import XCTest

/// The loading screen between missions: its model's stages, its pass's
/// band of light and fade into the frame under it, its chant (silent
/// without the player's executable), and the next mission.
final class LoadingTests: XCTestCase {
    func device() throws -> MTLDevice {
        try XCTUnwrap(MTLCreateSystemDefaultDevice(), "no Metal device")
    }

    /// A shared target cleared to `color`.
    func target(_ device: MTLDevice, width: Int, height: Int) throws -> MTLTexture {
        let descriptor = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: .bgra8Unorm, width: width,
                                                                  height: height, mipmapped: false)
        descriptor.usage = [.renderTarget, .shaderRead]
        descriptor.storageMode = .shared
        return try XCTUnwrap(device.makeTexture(descriptor: descriptor))
    }

    func clear(_ texture: MTLTexture, _ queue: MTLCommandQueue, red: Double) throws {
        let commands = try XCTUnwrap(queue.makeCommandBuffer())
        let pass = MTLRenderPassDescriptor()
        pass.colorAttachments[0].texture = texture
        pass.colorAttachments[0].loadAction = .clear
        pass.colorAttachments[0].storeAction = .store
        pass.colorAttachments[0].clearColor = MTLClearColor(red: red, green: 0, blue: 0, alpha: 1)
        commands.makeRenderCommandEncoder(descriptor: pass)?.endEncoding()
        commands.commit()
        commands.waitUntilCompleted()
    }

    /// Each pixel's BGRA.
    func pixels(_ texture: MTLTexture) -> [UInt8] {
        var bytes = [UInt8](repeating: 0, count: texture.width * texture.height * 4)
        texture.getBytes(&bytes, bytesPerRow: texture.width * 4,
                         from: MTLRegionMake2D(0, 0, texture.width, texture.height), mipmapLevel: 0)
        return bytes
    }

    /// The brightest green across column `x` (the light is white; the
    /// target's own red is left out).
    func green(_ bytes: [UInt8], width: Int, height: Int, x: Int) -> UInt8 {
        (0..<height).map { bytes[($0 * width + x) * 4 + 1] }.max() ?? 0
    }

    func view(color: Float, mask: Float, fade: Float = 0, alpha: Float = 1) -> LoadingView {
        LoadingView(color: color, maskPosition: mask, fadeAlpha: fade, gains: [0, 0, 0, 0], rates: [1, 1, 1, 1],
                    screenAlpha: alpha, stage: .loading, skippable: false)
    }

    func testTheModelGoesThroughItsStages() {
        let screen = LoadingScreenModel(enabled: true)
        screen.setProgress(progress: 0)
        XCTAssertEqual(screen.update(seconds: 1.0 / 60).stage, .loading)
        XCTAssertFalse(screen.skip(), "not before the map is ready")
        screen.setProgress(progress: 0.5)
        screen.setReady()
        let settling = screen.update(seconds: 1.0 / 60)
        XCTAssertEqual(settling.stage, .settling)
        XCTAssertTrue(settling.skippable)
        XCTAssertTrue(screen.skip())
        var last = screen.update(seconds: 0.1)
        XCTAssertEqual(last.stage, .crossfade)
        XCTAssertLessThan(last.screenAlpha, 1)
        for _ in 0..<10 {
            last = screen.update(seconds: 0.1)
        }
        XCTAssertEqual(last.stage, .done)
        XCTAssertEqual(last.screenAlpha, 0)
        XCTAssertEqual(last.gains, [0, 0, 0, 0])
        XCTAssertEqual(screen.maskPixels().count, 128 * 16)
    }

    func testTheBandIsLitWhereItsMaskIs() throws {
        let device = try device()
        let queue = try XCTUnwrap(device.makeCommandQueue())
        let screen = LoadingScreenModel(enabled: true)
        let pass = try LoadingPass(device: device, mask: screen.maskPixels(), outputFormat: .bgra8Unorm)
        let (width, height) = (320, 240)
        let texture = try target(device, width: width, height: height)
        // the band's left edge at 192 of the screen's 640: lit across 192 to 320, so 96 to 160 here
        let commands = try XCTUnwrap(queue.makeCommandBuffer())
        pass.encode(view(color: 1, mask: 192), into: commands, target: texture, clear: true)
        commands.commit()
        commands.waitUntilCompleted()
        let bytes = pixels(texture)
        XCTAssertGreaterThan(green(bytes, width: width, height: height, x: 128), 200, "the band's middle")
        XCTAssertLessThan(green(bytes, width: width, height: height, x: 40), 8, "left of the band")
        XCTAssertLessThan(green(bytes, width: width, height: height, x: 250), 8, "right of the band")
    }

    func testTheTrailIsLeftBehindAndFades() throws {
        let device = try device()
        let queue = try XCTUnwrap(device.makeCommandQueue())
        let pass = try LoadingPass(device: device, mask: LoadingScreenModel(enabled: true).maskPixels(),
                                   outputFormat: .bgra8Unorm)
        let (width, height) = (320, 240)
        let texture = try target(device, width: width, height: height)
        func draw(_ frame: LoadingView) throws -> [UInt8] {
            let commands = try XCTUnwrap(queue.makeCommandBuffer())
            pass.encode(frame, into: commands, target: texture, clear: true)
            commands.commit()
            commands.waitUntilCompleted()
            return pixels(texture)
        }
        _ = try draw(view(color: 1, mask: 0, fade: 0.9))
        // the band moves on; where it was keeps a fainter light
        let after = try draw(view(color: 1, mask: 400, fade: 0.9))
        let trail = green(after, width: width, height: height, x: 32)
        XCTAssertGreaterThan(trail, 60, "the trail")
        XCTAssertLessThan(trail, 250)
        // a new screen starts without the last one's trail
        pass.reset()
        let fresh = try draw(view(color: 1, mask: 400, fade: 0.9))
        XCTAssertLessThan(green(fresh, width: width, height: height, x: 32), 8)
    }

    func testTheScreenFadesIntoTheFrameUnderIt() throws {
        let device = try device()
        let queue = try XCTUnwrap(device.makeCommandQueue())
        let pass = try LoadingPass(device: device, mask: LoadingScreenModel(enabled: true).maskPixels(),
                                   outputFormat: .bgra8Unorm)
        let texture = try target(device, width: 64, height: 48)
        for (alpha, red) in [(Float(0), 255), (1, 0)] {
            try clear(texture, queue, red: 1)
            let commands = try XCTUnwrap(queue.makeCommandBuffer())
            pass.encode(view(color: 0, mask: -128, alpha: alpha), into: commands, target: texture, clear: false)
            commands.commit()
            commands.waitUntilCompleted()
            let bytes = pixels(texture)
            XCTAssertEqual(Int(bytes[2]), red, "alpha \(alpha): the mission's red")
        }
        // half way, half of each
        try clear(texture, queue, red: 1)
        let commands = try XCTUnwrap(queue.makeCommandBuffer())
        pass.encode(view(color: 0, mask: -128, alpha: 0.5), into: commands, target: texture, clear: false)
        commands.commit()
        commands.waitUntilCompleted()
        XCTAssertEqual(Double(pixels(texture)[2]), 127.5, accuracy: 3)
    }

    func testTheShaderReloads() throws {
        let device = try device()
        let pass = try LoadingPass(device: device, mask: Data(), outputFormat: .bgra8Unorm)
        XCTAssertNoThrow(try pass.reloadShaders(source: ShaderSources.source(.loading)))
        XCTAssertThrowsError(try pass.reloadShaders(source: "not metal"))
        XCTAssertThrowsError(try LoadingPass(device: device, mask: Data(), outputFormat: .bgra8Unorm, source: "#include"))
    }

    func testTheChantIsSilentWithoutTheExecutable() throws {
        let cache = FileManager.default.temporaryDirectory.appendingPathComponent("opence-chant-\(UUID())")
        defer { try? FileManager.default.removeItem(at: cache) }
        let folders = [URL(fileURLWithPath: "/nonexistent/assets/maps")]
        let executables = ChantPlayer.executables(folders: folders, root: URL(fileURLWithPath: "/nonexistent"))
        XCTAssertEqual(executables.map(\.lastPathComponent), ["default.xbe", "default.xbe", "default.xbe"])
        XCTAssertEqual(executables.first?.path, "/nonexistent/assets/default.xbe")
        XCTAssertNil(ChantPlayer.wave(executables: executables, cacheFolder: cache.path))
        XCTAssertNil(ChantPlayer.wave(executables: [], cacheFolder: cache.path))
        let player = ChantPlayer(wave: nil)
        XCTAssertFalse(player.hasTone)
        let loud = LoadingView(color: 1, maskPosition: 0, fadeAlpha: 0.9, gains: [1, 1, 1, 1],
                               rates: [0.5, 0.5, 0.5, 0.5], screenAlpha: 1, stage: .loading, skippable: false)
        player.apply(loud)
        XCTAssertFalse(player.isPlaying)
    }

    func testTheChantPlaysAToneAndStops() throws {
        // a tone of our own, as the cache's file would be
        let folder = FileManager.default.temporaryDirectory.appendingPathComponent("opence-tone-\(UUID())")
        try FileManager.default.createDirectory(at: folder, withIntermediateDirectories: true)
        defer { try? FileManager.default.removeItem(at: folder) }
        let url = folder.appendingPathComponent("chant.wav")
        let format = try XCTUnwrap(AVAudioFormat(commonFormat: .pcmFormatInt16, sampleRate: 22_050, channels: 1,
                                                 interleaved: true))
        do {
            let file = try AVAudioFile(forWriting: url, settings: format.settings, commonFormat: .pcmFormatInt16,
                                       interleaved: true)
            let buffer = try XCTUnwrap(AVAudioPCMBuffer(pcmFormat: format, frameCapacity: 2205))
            buffer.frameLength = 2205
            let samples = try XCTUnwrap(buffer.int16ChannelData)[0]
            for n in 0..<2205 {
                samples[n] = Int16(sin(Double(n) * 0.1) * 8000)
            }
            try file.write(from: buffer)
        }
        let player = ChantPlayer(wave: url)
        XCTAssertTrue(player.hasTone)
        var loud = LoadingView(color: 1, maskPosition: 0, fadeAlpha: 0.9, gains: [0, 0, 0, 0],
                               rates: [0.3, 0.3, 0.5, 0.2], screenAlpha: 1, stage: .loading, skippable: false)
        player.apply(loud)
        XCTAssertFalse(player.isPlaying, "not before a voice is heard")
        loud.gains = [0.1, 0.1, 0.1, 0.1]
        player.apply(loud)
        // (a machine without an output device stays silent)
        loud.stage = .done
        player.apply(loud)
        XCTAssertFalse(player.isPlaying)
        player.stop()
    }

    func testTheNextMissionIsTheNextWhoseMapIsThere() {
        let maps = ["a10", "a50", "b30", "ui"].map { URL(fileURLWithPath: "/maps/\($0).map") }
        XCTAssertEqual(HaloMenu.nextMission(after: "/maps/a10.map", in: maps)?.lastPathComponent, "a50.map",
                       "a30 is not there")
        XCTAssertEqual(HaloMenu.nextMission(after: "/maps/A50.map", in: maps)?.lastPathComponent, "b30.map")
        XCTAssertNil(HaloMenu.nextMission(after: "/maps/b30.map", in: maps), "the last one there")
        XCTAssertNil(HaloMenu.nextMission(after: "/maps/d40.map", in: maps))
        XCTAssertNil(HaloMenu.nextMission(after: "/maps/bloodgulch.map", in: maps))
    }

    func testTheLoadReportsItsProgress() {
        XCTAssertLessThan(LoadedLevel.importedProgress, LoadedLevel.drawnProgress)
        XCTAssertLessThan(LoadedLevel.drawnProgress, LoadedLevel.bakedProgress)
        XCTAssertLessThan(LoadedLevel.bakedProgress, 1)
    }

    /// The screen half way through a load, its trail built up over a
    /// second of frames: a golden frame written to build/engine/golden and
    /// compared with the approved one in assets/golden/engine if it is
    /// there (it holds nothing of the game's: the field is white, as
    /// loading.tga is not read).
    func testTheGoldenFrameOfTheScreen() throws {
        let device = try device()
        let queue = try XCTUnwrap(device.makeCommandQueue())
        let screen = LoadingScreenModel(enabled: true)
        let pass = try LoadingPass(device: device, mask: screen.maskPixels(), outputFormat: .bgra8Unorm)
        let (width, height) = (1280, 720)
        let target = try target(device, width: width, height: height)
        screen.setProgress(progress: 0)
        screen.setProgress(progress: 0.5)
        for _ in 0..<60 {
            let commands = try XCTUnwrap(queue.makeCommandBuffer())
            pass.encode(screen.update(seconds: 1.0 / 60), into: commands, target: target, clear: true)
            commands.commit()
            commands.waitUntilCompleted()
        }
        let drawn = CapturedFrame(width: width, height: height, bytes: pixels(target))
        XCTAssertGreaterThan(drawn.distinctColors, 16, "the band and its trail")

        let out = StockLevel.root.appendingPathComponent("build/engine/golden")
        try FileManager.default.createDirectory(at: out, withIntermediateDirectories: true)
        let name = "loading_screen_\(width)x\(height).png"
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
