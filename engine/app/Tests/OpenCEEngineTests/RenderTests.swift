import Foundation
import Metal
import OpenCEEngineCore
import OpenCEEngineKit
import OpenCEEngineRender
import simd
import XCTest

final class ViewCameraTests: XCTestCase {
    let camera = ViewCamera(position: [10, 20, 5], yaw: 0, pitch: 0, verticalFov: 70 * .pi / 180)

    func clip(_ point: SIMD3<Float>, aspect: Float = 16.0 / 9.0, camera: ViewCamera? = nil) -> SIMD3<Float> {
        let clip = (camera ?? self.camera).viewProjection(aspect: aspect) * SIMD4(point, 1)
        return SIMD3(clip.x, clip.y, clip.z) / clip.w
    }

    func testAPointAheadIsInTheMiddle() {
        let ahead = clip([110, 20, 5])
        XCTAssertEqual(ahead.x, 0, accuracy: 1e-5)
        XCTAssertEqual(ahead.y, 0, accuracy: 1e-5)
    }

    func testDepthIsReversed() {
        XCTAssertEqual(clip([10 + ViewCamera.near, 20, 5]).z, 1, accuracy: 1e-4)
        let far = clip([10_010, 20, 5]).z
        XCTAssertGreaterThan(far, 0)
        XCTAssertLessThan(far, 1e-5)
        XCTAssertGreaterThan(clip([11, 20, 5]).z, clip([12, 20, 5]).z)
    }

    func testLeftAndUpAreWhereTheyShouldBe() {
        // +Y is to the camera's left, +Z up
        XCTAssertLessThan(clip([20, 21, 5]).x, 0)
        XCTAssertGreaterThan(clip([20, 20, 6]).y, 0)
    }

    func testTheFieldOfViewIsHorPlus() {
        // the top edge stays put; a wider screen sees further to the side
        let edge = Float(tan(35 * Double.pi / 180))
        XCTAssertEqual(clip([11, 20, 5 + edge], aspect: 16.0 / 9.0).y, 1, accuracy: 1e-4)
        XCTAssertEqual(clip([11, 20, 5 + edge], aspect: 21.0 / 9.0).y, 1, accuracy: 1e-4)
        let side: SIMD3<Float> = [11, 20 - edge * 16 / 9, 5]
        XCTAssertEqual(clip(side, aspect: 16.0 / 9.0).x, 1, accuracy: 1e-4)
        XCTAssertLessThan(clip(side, aspect: 21.0 / 9.0).x, 1)
    }

    func testYawTurnsLeft() {
        let turned = ViewCamera(position: [0, 0, 0], yaw: .pi / 2, pitch: 0, verticalFov: 1.2)
        let ahead = clip([0, 100, 0], camera: turned)
        XCTAssertEqual(ahead.x, 0, accuracy: 1e-4)
        XCTAssertEqual(ahead.y, 0, accuracy: 1e-4)
    }

    func testInterpolationGoesBetween() {
        let later = ViewCamera(position: [20, 20, 5], yaw: 0.5, pitch: 0, verticalFov: camera.verticalFov)
        let half = ViewCamera.interpolated(from: camera, to: later, alpha: 0.5)
        XCTAssertEqual(half.position.x, 15, accuracy: 1e-5)
        XCTAssertEqual(half.rotation.angle, 0.25, accuracy: 1e-4)
        XCTAssertEqual(ViewCamera.interpolated(from: camera, to: later, alpha: .nan).position, later.position)
    }
}

final class JitterTests: XCTestCase {
    func testTheSequenceIsHaltons() {
        XCTAssertEqual(Jitter.offset(frame: 0), SIMD2(0, 1.0 / 3.0 - 0.5))
        XCTAssertEqual(Jitter.offset(frame: 1).x, -0.25, accuracy: 1e-6)
        XCTAssertEqual(Jitter.offset(frame: 1).y, 2.0 / 3.0 - 0.5, accuracy: 1e-6)
        XCTAssertEqual(Jitter.offset(frame: Jitter.length), Jitter.offset(frame: 0))
    }

    func testTheOffsetsCoverAPixelEvenly() {
        let offsets = (0..<Jitter.length).map { Jitter.offset(frame: $0) }
        for offset in offsets {
            XCTAssertTrue((-0.5..<0.5).contains(offset.x) && (-0.5..<0.5).contains(offset.y), "\(offset)")
        }
        let mean = offsets.reduce(SIMD2<Float>.zero, +) / Float(offsets.count)
        XCTAssertLessThan(abs(mean.x), 0.03)
        XCTAssertLessThan(abs(mean.y), 0.03)
        XCTAssertEqual(Set(offsets.map { "\($0)" }).count, Jitter.length)
    }

    func testAnOffsetMovesTheImageByPixels() {
        let camera = ViewCamera(position: [0, 0, 0], yaw: 0, pitch: 0, verticalFov: 1.2)
        let projection = camera.viewProjection(aspect: 2)
        let moved = Jitter.apply([0.5, 0.25], to: projection, width: 200, height: 100)
        let point = SIMD4<Float>(10, 1, 2, 1)
        let before = projection * point
        let after = moved * point
        // half a pixel right, a quarter down: in clip space x by 2 * 0.5 / 200, y by -2 * 0.25 / 100
        XCTAssertEqual(after.x / after.w - before.x / before.w, 0.005, accuracy: 1e-6)
        XCTAssertEqual(after.y / after.w - before.y / before.w, -0.005, accuracy: 1e-6)
        XCTAssertEqual(after.z / after.w, before.z / before.w, accuracy: 1e-7)
    }

    func testTheRenderScaleSizesTheInput() {
        XCTAssertTrue(VideoSettings(antiAliasing: .native, renderScale: 0.5).inputSize(width: 1920, height: 1080)
            == (1920, 1080))
        XCTAssertTrue(VideoSettings(antiAliasing: .temporal, renderScale: 0.5).inputSize(width: 1920, height: 1080)
            == (960, 540))
        XCTAssertTrue(VideoSettings(antiAliasing: .temporal, renderScale: 0.1).inputSize(width: 1920, height: 1080)
            == (960, 540))
        XCTAssertTrue(VideoSettings(antiAliasing: .temporal, renderScale: 2).inputSize(width: 1920, height: 1080)
            == (1920, 1080))
        XCTAssertTrue(VideoSettings(antiAliasing: .temporal, renderScale: .nan).inputSize(width: 1920, height: 1080)
            == (1920, 1080))
    }
}

/// A stock map's first BSP and the Metal device to draw it with.
struct StockLevel {
    static let root = URL(fileURLWithPath: #filePath).deletingLastPathComponent()
        .deletingLastPathComponent().deletingLastPathComponent().deletingLastPathComponent()
        .deletingLastPathComponent()

    let device: MTLDevice
    let queue: MTLCommandQueue
    let geometry: LevelGeometry
    let camera: ViewCamera
    /// The scenario's objects, standing still after a tick.
    let objects: [DrawnObject]

    init(map: String) throws {
        let mapURL = Self.root.appendingPathComponent("assets/maps/\(map).map")
        guard FileManager.default.fileExists(atPath: mapURL.path) else {
            throw XCTSkip("no assets/maps/\(map).map")
        }
        guard let device = MTLCreateSystemDefaultDevice(), let queue = device.makeCommandQueue() else {
            throw XCTSkip("no Metal device")
        }
        let cache = Self.root.appendingPathComponent("build/engine/cache")
        try FileManager.default.createDirectory(at: cache, withIntermediateDirectories: true)
        self.device = device
        self.queue = queue
        geometry = try LevelGeometry.load(map: mapURL.path, cacheFolder: cache.path, bsp: 0)
        let start = geometry.startView()
        camera = ViewCamera(position: [start.x, start.y, start.z], yaw: start.yaw, pitch: start.pitch,
                            verticalFov: 70 * .pi / 180)
        let engine = Engine()
        let reader = SnapshotReader(engine: engine)
        _ = try engine.startLevel(geometry: geometry, difficulty: 1)
        _ = try engine.step()
        objects = reader.acquire().drawnObjects()
    }

    func renderer(_ settings: VideoSettings) throws -> LevelRenderer {
        let renderer = try LevelRenderer(device: device, geometry: geometry, settings: settings)
        if settings.antiAliasing == .temporal, !renderer.supportsTemporal {
            throw XCTSkip("no MetalFX temporal scaler on this device")
        }
        return renderer
    }
}

/// The golden frames: each stock map's first BSP from its start view, at
/// three sizes, drawn native and with temporal anti-aliasing (after a
/// full jitter sequence), written to build/engine/golden and compared with
/// the approved frames in assets/golden/engine if they are there (never
/// committed: they are the game's art).
final class GoldenFrameTests: XCTestCase {
    static let sizes = [(1920, 1080), (3440, 1440), (3840, 2160)]

    func render(map: String) throws {
        let level = try StockLevel(map: map)
        let out = StockLevel.root.appendingPathComponent("build/engine/golden")
        try FileManager.default.createDirectory(at: out, withIntermediateDirectories: true)
        for antiAliasing in AntiAliasing.allCases {
            let renderer = try level.renderer(VideoSettings(antiAliasing: antiAliasing, renderScale: 1))
            XCTAssertGreaterThan(renderer.drawCount, 10)
            let frames = antiAliasing == .temporal ? Jitter.length : 1
            for (width, height) in Self.sizes {
                let name = "\(map)_\(antiAliasing.rawValue)_\(width)x\(height).png"
                let frame = try renderer.capture(width: width, height: height, camera: level.camera, frames: frames,
                                                 objects: level.objects, queue: level.queue)
                try frame.writePNG(to: out.appendingPathComponent(name))
                check(frame, name: name)
            }
        }
    }

    func check(_ frame: CapturedFrame, name: String) {
        // drawn: many colours, and most of the frame not the sky's clear colour
        XCTAssertGreaterThan(frame.distinctColors, 256, name)
        let sky = SIMD3<UInt8>(UInt8(LevelRenderer.skyColor.red * 255), UInt8(LevelRenderer.skyColor.green * 255),
                               UInt8(LevelRenderer.skyColor.blue * 255))
        var level = 0
        for y in stride(from: 0, to: frame.height, by: 16) {
            for x in stride(from: 0, to: frame.width, by: 16) {
                let texel = SIMD3<Int>(truncatingIfNeeded: frame.rgb(x: x, y: y))
                let off = texel &- SIMD3<Int>(truncatingIfNeeded: sky)
                let distance = abs(off.x) + abs(off.y) + abs(off.z)
                if distance > 6 { level += 1 }
            }
        }
        let samples = ((frame.width + 15) / 16) * ((frame.height + 15) / 16)
        XCTAssertGreaterThan(Double(level) / Double(samples), 0.2, "\(name): mostly the clear colour")
        let approved = StockLevel.root.appendingPathComponent("assets/golden/engine/\(name)")
        if let approvedFrame = CapturedFrame.readPNG(at: approved) {
            guard let difference = frame.difference(from: approvedFrame) else {
                XCTFail("\(name): another size than the approved frame")
                return
            }
            XCTAssertLessThan(difference.share, 0.005, "\(name) differs from the approved frame")
        } else {
            print("golden frame \(name): none approved yet (assets/golden/engine)")
        }
    }

    func testBloodGulch() throws { try render(map: "bloodgulch") }
    func testTheMawsFirstBsp() throws { try render(map: "a30") }
}

/// MetalFX's temporal scaler on Blood Gulch: a still camera settles, the
/// gathered frame is the native one smoothed, and a moving camera's frame
/// follows it without trails.
final class TemporalTests: XCTestCase {
    static let width = 1280
    static let height = 720

    func testAStillCameraConverges() throws {
        let level = try StockLevel(map: "bloodgulch")
        let renderer = try level.renderer(VideoSettings(antiAliasing: .temporal, renderScale: 0.67))
        let settled = try renderer.capture(width: Self.width, height: Self.height, camera: level.camera,
                                           frames: Jitter.length, queue: level.queue)
        XCTAssertTrue(renderer.lastInputSize == (858, 482), "\(renderer.lastInputSize)")
        let next = try renderer.capture(width: Self.width, height: Self.height, camera: level.camera,
                                        frames: 1, queue: level.queue)
        let later = try renderer.capture(width: Self.width, height: Self.height, camera: level.camera,
                                         frames: Jitter.length / 2, queue: level.queue)
        let step = try XCTUnwrap(next.difference(from: settled))
        let drift = try XCTUnwrap(later.difference(from: settled))
        print("temporal: a frame on, \(step.share) differ; half a sequence on, \(drift.share)")
        // (settled, about 0.0003 and 0.0008; with the jitter's sign the
        // wrong way for the scaler, 0.003 and 0.006)
        XCTAssertLessThan(step.share, 0.001, "the next frame still moves")
        XCTAssertLessThan(drift.share, 0.002, "the image drifts")
    }

    /// The share of texels of `frame` further than `tolerance` from the
    /// native frame from `camera`.
    func departure(_ frame: CapturedFrame, level: StockLevel, camera: ViewCamera) throws -> Double {
        let native = try level.renderer(VideoSettings(antiAliasing: .native))
        let reference = try native.capture(width: Self.width, height: Self.height, camera: camera, queue: level.queue)
        return try XCTUnwrap(frame.difference(from: reference, tolerance: 48)).share
    }

    func testTheGatheredFrameIsTheNativeOneSmoothed() throws {
        let level = try StockLevel(map: "bloodgulch")
        for scale: Float in [1, 0.5] {
            let renderer = try level.renderer(VideoSettings(antiAliasing: .temporal, renderScale: scale))
            let frame = try renderer.capture(width: Self.width, height: Self.height, camera: level.camera,
                                             frames: Jitter.length, queue: level.queue)
            let share = try departure(frame, level: level, camera: level.camera)
            print("temporal at \(scale): \(share) of texels far from native")
            // (about 0.001 and 0.0025; 0.007 at half scale with the jitter wrong)
            XCTAssertLessThan(share, 0.004, "at render scale \(scale)")
        }
    }

    func testAMovingCameraLeavesNoTrails() throws {
        let level = try StockLevel(map: "bloodgulch")
        let start = level.camera
        let forward = start.rotation.act([1, 0, 0])
        // (about ten pixels a frame of turn, and a step forward)
        let path = (0..<48).map { step in
            ViewCamera(position: start.position + forward * Float(step) * 0.03,
                       rotation: simd_quatf(angle: Float(step) * 0.02, axis: [0, 0, 1]) * start.rotation,
                       verticalFov: start.verticalFov)
        }
        let moving = try level.renderer(VideoSettings(antiAliasing: .temporal, renderScale: 1))
        let frame = try moving.capture(width: Self.width, height: Self.height, cameras: path, queue: level.queue)
        // what the scaler gathers when the camera has stood there all along
        let still = try level.renderer(VideoSettings(antiAliasing: .temporal, renderScale: 1))
        let settled = try still.capture(width: Self.width, height: Self.height, camera: path[path.count - 1],
                                        frames: Jitter.length, queue: level.queue)
        let share = try XCTUnwrap(frame.difference(from: settled, tolerance: 16)).share
        print("temporal, moving: \(share) of texels far from the settled frame")
        // (MetalFX drops history it cannot match, so motion that is wrong
        // loses the smoothing rather than trailing: about 0.008 reversed or
        // missing, against 0.004)
        XCTAssertLessThan(share, 0.006, "the moving camera's frame lost its history")
    }

    func testTheMotionIsTheCamerasTurn() throws {
        let level = try StockLevel(map: "bloodgulch")
        let renderer = try level.renderer(VideoSettings(antiAliasing: .temporal, renderScale: 0.5))
        let before = level.camera
        let after = ViewCamera(position: before.position,
                               rotation: simd_quatf(angle: 0.05, axis: [0, 0, 1]) * before.rotation,
                               verticalFov: before.verticalFov)
        _ = try renderer.capture(width: Self.width, height: Self.height, cameras: [before, after], queue: level.queue)
        let read = try XCTUnwrap(renderer.readMotion(queue: level.queue))
        XCTAssertTrue((read.width, read.height) == (640, 360))
        // a turn moves each texel the same whatever its depth: where the
        // direction it shows now was in the last frame
        let aspect = Float(Self.width) / Float(Self.height)
        let focalY = 1 / tan(after.verticalFov / 2)
        let focalX = focalY / aspect
        for (x, y) in [(320, 180), (100, 60), (600, 300), (40, 340)] {
            let uv = (SIMD2<Float>(Float(x), Float(y)) + 0.5) / SIMD2(Float(read.width), Float(read.height))
            let ndc = SIMD2(uv.x * 2 - 1, 1 - uv.y * 2)
            let direction = after.rotation.act([1, 0, 0]) - after.rotation.act([0, 1, 0]) * (ndc.x / focalX)
                + after.rotation.act([0, 0, 1]) * (ndc.y / focalY)
            let clip = before.viewProjection(aspect: aspect) * SIMD4(direction, 0)
            let was = SIMD2(clip.x / clip.w * 0.5 + 0.5, 0.5 - clip.y / clip.w * 0.5)
            let motion = read.motion[y * read.width + x]
            XCTAssertEqual(motion.x, was.x - uv.x, accuracy: 2e-3, "texel \(x), \(y)")
            XCTAssertEqual(motion.y, was.y - uv.y, accuracy: 2e-3, "texel \(x), \(y)")
            XCTAssertGreaterThan(abs(motion.x), 0.01, "texel \(x), \(y) did not move")
        }
    }
}

/// The objects' models: placed where the scenario puts them, skinned by
/// their nodes and drawn at the level of detail their size chooses.
final class ModelTests: XCTestCase {
    func testTheGamesMatrixIsAFourByFour() {
        let matrix = DrawnObject.matrix(forward: [0, 1, 0], left: [-1, 0, 0], up: [0, 0, 1], position: [5, 6, 7],
                                        scale: 2)
        let point = matrix * SIMD4<Float>(1, 0, 0, 1)
        XCTAssertEqual(SIMD3(point.x, point.y, point.z), [5, 8, 7])
        let along = matrix * SIMD4<Float>(0, 1, 0, 0)
        XCTAssertEqual(SIMD3(along.x, along.y, along.z), [-2, 0, 0])
    }

    func testBloodGulchsObjectsAreDrawn() throws {
        let level = try StockLevel(map: "bloodgulch")
        XCTAssertFalse(level.objects.isEmpty)
        for object in level.objects {
            XCTAssertTrue(object.radius.isFinite)
            XCTAssertEqual(object.regions.count, 8)
        }
        let renderer = try level.renderer(VideoSettings(antiAliasing: .native, renderScale: 1))
        XCTAssertGreaterThan(renderer.modelCount, 0)
        let without = try renderer.capture(width: 640, height: 360, camera: level.camera, queue: level.queue)
        XCTAssertEqual(renderer.lastModelPartCount, 0)
        let with = try renderer.capture(width: 640, height: 360, camera: level.camera, objects: level.objects,
                                        queue: level.queue)
        XCTAssertGreaterThan(renderer.lastModelPartCount, 0)
        let difference = try XCTUnwrap(with.difference(from: without))
        XCTAssertGreaterThan(difference.share, 0, "the objects changed nothing in the frame")
    }

    func testAnObjectBehindTheCameraIsNotDrawn() throws {
        let level = try StockLevel(map: "bloodgulch")
        let renderer = try level.renderer(VideoSettings(antiAliasing: .native, renderScale: 1))
        let behind = level.objects.map { object -> DrawnObject in
            var moved = object
            moved.center = level.camera.position - level.camera.rotation.act([1, 0, 0]) * (object.radius + 10)
            return moved
        }
        _ = try renderer.capture(width: 640, height: 360, camera: level.camera, objects: behind, queue: level.queue)
        XCTAssertEqual(renderer.lastModelPartCount, 0)
    }
}

/// The sky's model drawn behind everything at infinity, and its fog over
/// the level and the objects.
final class SkyTests: XCTestCase {
    func testTheFogPlaneMeasuresAlongTheView() {
        let camera = ViewCamera(position: [10, 0, 0], yaw: 0, pitch: 0, verticalFov: 1)
        let fog = RenderFog(color: [0.5, 0.6, 0.7], maximumDensity: 0.75, startDistance: 10, endDistance: 110)
        let frame = FrameUniforms(viewProjection: matrix_identity_float4x4, camera: camera, fog: fog)
        let amount = { (point: SIMD3<Float>) in
            min(max(dot(SIMD3(frame.fogPlane.x, frame.fogPlane.y, frame.fogPlane.z), point) + frame.fogPlane.w, 0),
                1) * frame.fogColor.w
        }
        XCTAssertEqual(amount([15, 0, 0]), 0)
        XCTAssertEqual(amount([70, 40, 0]), 0.375, accuracy: 1e-5)
        XCTAssertEqual(amount([500, 0, 0]), 0.75, accuracy: 1e-5)
        let none = FrameUniforms(viewProjection: matrix_identity_float4x4, camera: camera, fog: nil)
        XCTAssertEqual(none.fogColor.w, 0)
    }

    func testBloodGulchsSkyIsDrawn() throws {
        let level = try StockLevel(map: "bloodgulch")
        let renderer = try level.renderer(VideoSettings(antiAliasing: .native, renderScale: 1))
        let fog = try XCTUnwrap(renderer.fog)
        XCTAssertEqual(fog.color.count, 3)
        _ = try renderer.capture(width: 640, height: 360, camera: level.camera, queue: level.queue)
        XCTAssertGreaterThan(renderer.lastSkyPartCount, 0)
    }
}

/// The benchmark's measures: its percentiles and its path.
final class BenchmarkTests: XCTestCase {
    func testThePercentilesAreByNearestRank() {
        let times = (1...100).map(Double.init)
        XCTAssertEqual(BenchmarkResult.percentile(50, of: times), 50)
        XCTAssertEqual(BenchmarkResult.percentile(99, of: times), 99)
        XCTAssertEqual(BenchmarkResult.percentile(100, of: times), 100)
        XCTAssertEqual(BenchmarkResult.percentile(99, of: [3]), 3)
        XCTAssertEqual(BenchmarkResult.percentile(99, of: []), 0)
    }

    func testThePathLoopsBackToItsStart() {
        let path = BenchmarkPath(centre: [10, 20, 5])
        let start = path.camera(at: 0)
        let around = path.camera(at: path.period)
        XCTAssertLessThan(simd_distance(start.position, around.position), 1e-3)
        XCTAssertEqual(simd_distance(path.camera(at: 17).position * [1, 1, 0], [10, 20, 0]), path.radius,
                       accuracy: 1e-3)
    }
}
