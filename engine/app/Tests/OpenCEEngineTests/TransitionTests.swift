import Foundation
import Metal
import OpenCEEngineCore
import OpenCEEngineRender
import QuartzCore
import XCTest

/// The way from one mission to the next, as the app takes it: a10 drawn,
/// then the loading screen alone while a30 loads on another thread, then
/// the screen fading over a30's frames, then a30 alone. No frame of it may
/// take over 33 ms (skipped without assets/maps).
final class TransitionTests: XCTestCase {
    /// The other thread's load, handed to the frame loop.
    final class Load: @unchecked Sendable {
        private let lock = NSLock()
        private var result: Result<LoadedLevel, Error>?

        var finished: Result<LoadedLevel, Error>? {
            lock.lock()
            defer { lock.unlock() }
            return result
        }

        func finish(_ result: Result<LoadedLevel, Error>) {
            lock.lock()
            self.result = result
            lock.unlock()
        }
    }

    /// The pieces a background load needs (Metal's objects may be made on
    /// any thread).
    struct Job: @unchecked Sendable {
        let map: String
        let cache: String
        let device: MTLDevice
        let width: Int
        let height: Int
    }

    static let longestFrame = 0.033
    static let frameInterval = 1.0 / 60

    func camera(_ level: LoadedLevel) -> ViewCamera {
        ViewCamera(
            position: [level.start.x, level.start.y, level.start.z], yaw: level.start.yaw,
            pitch: level.start.pitch, verticalFov: 70 * .pi / 180)
    }

    func testNoFrameFromTheEndOfA10ToA30TakesOver33Ms() throws {
        let maps = StockLevel.root.appendingPathComponent("assets/maps")
        let a10 = maps.appendingPathComponent("a10.map").path
        let a30 = maps.appendingPathComponent("a30.map").path
        guard FileManager.default.fileExists(atPath: a10), FileManager.default.fileExists(atPath: a30) else {
            throw XCTSkip("no assets/maps/a10.map and a30.map")
        }
        guard let device = MTLCreateSystemDefaultDevice(), let queue = device.makeCommandQueue() else {
            throw XCTSkip("no Metal device")
        }
        let cache = StockLevel.root.appendingPathComponent("build/engine/cache")
        try FileManager.default.createDirectory(at: cache, withIntermediateDirectories: true)
        let settings = VideoSettings()
        // (the light as the cache has it: the bake's own tests bake at load)
        let first = try LoadedLevel(
            map: a10, cacheFolder: cache.path, device: device, settings: settings,
            bakeAtLoad: .cachedOnly)
        let screen = LoadingScreenModel(enabled: true)
        let pass = try LoadingPass(device: device, mask: screen.maskPixels(), outputFormat: .bgra8Unorm)
        let (width, height) = (1920, 1080)
        let descriptor = MTLTextureDescriptor.texture2DDescriptor(
            pixelFormat: .bgra8Unorm, width: width,
            height: height, mipmapped: false)
        descriptor.usage = [.renderTarget]
        descriptor.storageMode = .private
        let target = try XCTUnwrap(device.makeTexture(descriptor: descriptor))

        var times: [(String, Double)] = []
        var last = CACurrentMediaTime()
        /// One frame, its CPU and GPU work timed; then the rest of its
        /// sixtieth of a second is waited out, as the display would.
        func frame(_ name: String, _ encode: (MTLCommandBuffer, Double) -> Void) throws {
            let start = CACurrentMediaTime()
            let commands = try XCTUnwrap(queue.makeCommandBuffer())
            encode(commands, start - last)
            commands.commit()
            commands.waitUntilCompleted()
            let end = CACurrentMediaTime()
            times.append((name, end - start))
            last = start
            let rest = Self.frameInterval - (end - start)
            if rest > 0 {
                Thread.sleep(forTimeInterval: rest)
            }
        }

        // the end of a10 (its first frames warm it, as play would have)
        for index in 0..<30 {
            try frame(index < 10 ? "warm" : "a10") { commands, _ in
                first.renderer.encode(into: commands, target: target, camera: camera(first))
            }
        }
        // a30 loads behind the screen
        let load = Load()
        let job = Job(map: a30, cache: cache.path, device: device, width: width, height: height)
        let thread = Thread {
            load.finish(
                Result {
                    try LoadedLevel(
                        map: job.map, cacheFolder: job.cache, device: job.device, settings: settings,
                        bakeAtLoad: .cachedOnly, prepare: (job.width, job.height),
                        progress: { screen.setProgress(progress: $0) })
                })
        }
        thread.stackSize = 16 << 20
        thread.start()
        let deadline = CACurrentMediaTime() + 600
        var loaded: Result<LoadedLevel, Error>?
        while loaded == nil {
            XCTAssertLessThan(CACurrentMediaTime(), deadline, "a30 took over ten minutes to load")
            if CACurrentMediaTime() > deadline {
                return
            }
            loaded = load.finished
            try frame("loading") { commands, seconds in
                pass.encode(screen.update(seconds: Float(seconds)), into: commands, target: target, clear: true)
            }
        }
        let next = try XCTUnwrap(loaded).get()
        screen.setReady()
        var stage = LoadingStage.settling
        while stage != .done {
            try frame(stage == .settling ? "settling" : "crossfade") { commands, seconds in
                let view = screen.update(seconds: Float(seconds))
                stage = view.stage
                next.renderer.encode(into: commands, target: target, camera: camera(next))
                pass.encode(view, into: commands, target: target, clear: false)
            }
        }
        for _ in 0..<30 {
            try frame("a30") { commands, _ in
                next.renderer.encode(into: commands, target: target, camera: camera(next))
            }
        }

        let measured = times.filter { $0.0 != "warm" }
        let worst = try XCTUnwrap(measured.max { $0.1 < $1.1 })
        let stages = ["a10", "loading", "settling", "crossfade", "a30"].map { name in
            let stage = measured.filter { $0.0 == name }.map(\.1)
            return String(format: "%@ %d frames, worst %.2f ms", name, stage.count, (stage.max() ?? 0) * 1000)
        }
        print("transition: " + stages.joined(separator: "; "))
        XCTAssertGreaterThan(measured.filter { $0.0 == "crossfade" }.count, 0, "the screen faded")
        XCTAssertLessThan(
            worst.1, Self.longestFrame,
            String(format: "a %@ frame took %.2f ms", worst.0, worst.1 * 1000))
    }
}
