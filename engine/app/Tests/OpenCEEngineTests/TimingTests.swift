import Foundation
import Metal
import OpenCEEngineCore
import OpenCEEngineKit
import OpenCEEngineRender
import XCTest

/// The cost of a frame: each pass's GPU time, the renderer's CPU scopes
/// nested in the app's, the hitches of play with their causes, and the
/// line a second perf.log has of them.
final class TimingTests: XCTestCase {
    final class RecordingTimer: ScopeTimer {
        var calls: [String] = []
        var depth = 0
        /// The depth each scope was opened at.
        var opened: [(name: String, depth: Int)] = []

        func profileBegin(name: String) -> Bool {
            calls.append("begin \(name)")
            opened.append((name, depth))
            depth += 1
            return true
        }

        func profileEnd() {
            calls.append("end")
            depth -= 1
        }
    }

    let camera = ViewCamera(position: [0, 0, 2], yaw: 0, pitch: -0.5, verticalFov: 70 * .pi / 180)

    func renderer() throws -> (LevelRenderer, MTLCommandQueue) {
        let device = try XCTUnwrap(MTLCreateSystemDefaultDevice(), "no Metal device")
        let renderer = try LevelRenderer(
            device: device, geometry: try LevelGeometry.devmap(donor: nil, cacheFolder: ""))
        return (renderer, try XCTUnwrap(device.makeCommandQueue()))
    }

    override func tearDown() {
        GpuTimings.shared.enabled = false
        HitchLog.shared.playing = false
        HitchLog.shared.reset()
        super.tearDown()
    }

    func testEveryPassReportsAGpuTime() throws {
        let (renderer, queue) = try renderer()
        try XCTSkipUnless(GpuTimings.supported(renderer.device), "no timestamps at stage boundaries")
        GpuTimings.shared.enabled = true
        let before = GpuTimings.shared.frames
        _ = try renderer.capture(width: 64, height: 36, camera: camera, queue: queue)
        // (the times are read in the command buffer's completion handler)
        let deadline = Date().addingTimeInterval(1)
        while GpuTimings.shared.frames == before, Date() < deadline {
            RunLoop.current.run(until: Date().addingTimeInterval(0.01))
        }
        let passes = GpuTimings.shared.lastFrame
        let names = Set(passes.map(\.name))
        for name in ["level opaque", "level blended", "screen", "tone map"] {
            XCTAssertTrue(names.contains(name), "\(name) untimed: \(names.sorted())")
        }
        XCTAssertTrue(passes.allSatisfy { $0.milliseconds >= 0 })
        XCTAssertGreaterThan(passes.reduce(0) { $0 + $1.milliseconds }, 0)
    }

    func testAPassTimeIsItsSamplesSpan() {
        XCTAssertEqual(GpuTimings.milliseconds([1_000_000, 3_000_000, 2_000_000, 4_000_000], 1), 3)
        // (a stage the GPU skipped, or a sample it could not take)
        XCTAssertEqual(GpuTimings.milliseconds([0, 2_000_000, MTLCounterErrorValue, 4_000_000], 1), 2)
        XCTAssertEqual(GpuTimings.milliseconds([0, 0, 0, 0], 1), 0)
        XCTAssertEqual(GpuTimings.milliseconds([1, 2_000_000_000_000], 1), 0)
    }

    /// Every encoder of play is made named and timed: those made directly
    /// are the captures' and the loaders' only.
    func testNoEncoderGoesUntimed() throws {
        let app = URL(fileURLWithPath: #filePath).deletingLastPathComponent().deletingLastPathComponent()
            .deletingLastPathComponent()
        let allowed: Set<String> = ["GpuTimings.swift", "FrameCapture.swift", "HudPass.swift", "PathTracer.swift"]
        let pattern = try NSRegularExpression(pattern: #"\.make(Render|Compute|Blit)CommandEncoder\("#)
        var files = try FileManager.default.contentsOfDirectory(
            at: app.appendingPathComponent("Sources/OpenCEEngineRender"), includingPropertiesForKeys: nil)
        files.append(app.appendingPathComponent("Sources/OpenCEEngine/main.swift"))
        var untimed: [String] = []
        for file in files where file.pathExtension == "swift" && !allowed.contains(file.lastPathComponent) {
            let text = try String(contentsOf: file, encoding: .utf8)
            if pattern.firstMatch(in: text, range: NSRange(text.startIndex..., in: text)) != nil {
                untimed.append(file.lastPathComponent)
            }
        }
        XCTAssertEqual(untimed, [])
    }

    func testTheRenderersScopesNestInTheApps() throws {
        let (renderer, queue) = try renderer()
        let timer = RecordingTimer()
        renderer.scopes = timer
        _ = try profiled("encode", timer) {
            try renderer.capture(width: 64, height: 36, camera: camera, queue: queue)
        }
        XCTAssertEqual(timer.depth, 0)
        XCTAssertEqual(timer.calls.filter { $0.hasPrefix("begin") }.count, timer.calls.filter { $0 == "end" }.count)
        let names = timer.opened.map(\.name)
        for name in ["level", "shadows", "lights", "screen"] {
            XCTAssertTrue(names.contains(name), "\(name) not timed: \(names)")
        }
        XCTAssertEqual(timer.opened.first?.depth, 0)
        XCTAssertTrue(timer.opened.dropFirst().allSatisfy { $0.depth >= 1 })
        XCTAssertEqual(timer.opened.first { $0.name == "shadows" }?.depth, 2)
    }

    func testTheEnginesProfilerKeepsTheNesting() throws {
        let engine = Engine()
        engine.startEmptyWorld()
        XCTAssertTrue(engine.runCommand(line: "stat frame").ok)
        _ = try engine.advance(frameSeconds: 1.0 / 30.0)
        profiled("outer", engine) { profiled("inner", engine) {} }
        _ = try engine.advance(frameSeconds: 1.0 / 30.0)
        let shown = engine.runCommand(line: "stat frame")
        let rows = try XCTUnwrap(
            JSONSerialization.jsonObject(with: Data(shown.json.utf8)) as? [[String: Any]],
            shown.json)
        let outer = try XCTUnwrap(rows.first { $0["name"] as? String == "outer" }?["depth"] as? Int, shown.json)
        let inner = try XCTUnwrap(rows.first { $0["name"] as? String == "inner" }?["depth"] as? Int, shown.json)
        XCTAssertEqual(inner, outer + 1)
        XCTAssertTrue(engine.runCommand(line: "stat frame off").ok)
    }

    func testAHitchInPlayIsLoggedWithItsCause() throws {
        let (renderer, queue) = try renderer()
        _ = try renderer.capture(width: 64, height: 36, camera: camera, queue: queue)
        HitchLog.shared.reset()
        HitchLog.shared.playing = true
        try renderer.reloadShaders(source: ShaderSources.source(.level))
        // (a new size: the frame's targets made again)
        _ = try renderer.capture(width: 80, height: 40, camera: camera, queue: queue)
        let bake = try LevelGeometry.devmap(donor: nil, cacheFolder: "").bakePlan(quick: true)
        try renderer.useBakedLighting(try LightBaker.bake(bake, tracer: try PathTracer(device: renderer.device)))
        let entries = HitchLog.shared.entries
        func logged(_ kind: HitchLog.Kind, _ cause: String) -> Bool {
            entries.contains { $0.kind == kind && $0.cause.hasPrefix(cause) && $0.count > 0 }
        }
        XCTAssertTrue(logged(.compile, "level shaders"), "\(entries)")
        // (the frame's targets, or the temporal scaler's when it scales)
        XCTAssertTrue(
            entries.contains { $0.kind == .allocation && $0.cause.hasSuffix("targets 80×40") }
                || logged(.allocation, "temporal scaler 80×40"), "\(entries)")
        XCTAssertTrue(logged(.upload, "baked light"), "\(entries)")
        XCTAssertTrue(entries.allSatisfy { $0.totalMilliseconds >= $0.longestMilliseconds })
        XCTAssertEqual(HitchLog.shared.take().count, entries.count)
        XCTAssertEqual(HitchLog.shared.take(), [])
    }

    func testNothingIsLoggedWhileALevelLoads() throws {
        let (renderer, queue) = try renderer()
        HitchLog.shared.reset()
        HitchLog.shared.playing = false
        try renderer.reloadShaders(source: ShaderSources.source(.level))
        _ = try renderer.capture(width: 72, height: 40, camera: camera, queue: queue)
        XCTAssertEqual(HitchLog.shared.entries, [])
    }

    func testPerfLogWritesALineASecond() throws {
        let url = FileManager.default.temporaryDirectory
            .appendingPathComponent("perf-\(UUID().uuidString)/perf.log")
        defer { try? FileManager.default.removeItem(at: url.deletingLastPathComponent()) }
        let log = try XCTUnwrap(PerfLog(url: url))
        let gpu = [
            PassTime(name: "level opaque", milliseconds: 2), PassTime(name: "light shadows 0.1", milliseconds: 1),
            PassTime(name: "light shadows 0.2", milliseconds: 1),
        ]
        let hitch = HitchLog.Entry(
            kind: .allocation, cause: "light lists", count: 3, totalMilliseconds: 0.3,
            longestMilliseconds: 0.2)
        log.frame(cpuMilliseconds: 8, now: 10, gpu: gpu, hitches: { [hitch] })
        log.frame(cpuMilliseconds: 12, now: 10.5, gpu: gpu, hitches: { [hitch] })
        XCTAssertEqual(try String(contentsOf: url, encoding: .utf8), "")
        log.frame(cpuMilliseconds: 10, now: 11.1, gpu: gpu, hitches: { [hitch] })
        let lines = try String(contentsOf: url, encoding: .utf8).split(separator: "\n")
        XCTAssertEqual(lines.count, 1)
        let line = try XCTUnwrap(lines.first)
        XCTAssertTrue(line.contains("cpu 10.00 mean 12.00 max (3 frames)"), String(line))
        XCTAssertTrue(line.contains("gpu 4.00: level opaque 2.00, light shadows 2.00"), String(line))
        XCTAssertTrue(line.contains("allocation light lists 3×"), String(line))
    }
}
