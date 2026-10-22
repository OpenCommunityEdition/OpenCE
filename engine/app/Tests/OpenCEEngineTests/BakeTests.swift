import Foundation
import Metal
import OpenCEEngineCore
import OpenCEEngineRender
import simd
import XCTest

/// The bake's path tracer on the GPU (Shaders/bake.metal) against scenes
/// whose light is known (opence_bake's layouts) and against the CPU's
/// reference tracer on a level.
final class BakeTests: XCTestCase {
    func tracer() throws -> PathTracer {
        try PathTracer(device: try XCTUnwrap(MTLCreateSystemDefaultDevice(), "no Metal device"))
    }

    func testEveryLayoutConvergesToItsKnownLight() throws {
        let tracer = try tracer()
        let up = BakePoint(position: .zero, normal: SIMD3(0, 0, 1))
        for name in bakeLayoutNames() {
            let scene = try BakeScene.layout(name: name)
            let expected = try XCTUnwrap(bakeLayoutExpected(name: name))
            // (many points at the same place: their mean converges)
            let lights = try tracer.trace(scene, points: Array(repeating: up, count: 256), samples: 64,
                                          occlusionDistance: bakeLayoutOcclusionDistance())
            let mean = lights.map(\.irradiance).reduce(.zero, +) / Float(lights.count)
            let open = lights.map(\.occlusion).reduce(0, +) / Float(lights.count)
            for channel in 0..<3 {
                XCTAssertEqual(mean[channel], expected.irradiance, accuracy: expected.irradiance * 0.04 + 1e-3,
                               "\(name)")
            }
            XCTAssertEqual(open, expected.occlusion, accuracy: 0.03, "\(name)")
        }
    }

    func testTheGpuAgreesWithTheCpuOnALevel() throws {
        let tracer = try tracer()
        let geometry = try LevelGeometry.devmap(donor: nil, cacheFolder: "")
        let scene = try geometry.bakeScene()
        let bounds = scene.bounds()
        let middle = SIMD3(bounds[0] + bounds[3], bounds[1] + bounds[4], bounds[2] + bounds[5]) / 2
        let points = [
            BakePoint(position: SIMD3(middle.x, middle.y, bounds[5] + 0.01), normal: SIMD3(0, 0, 1)),
            BakePoint(position: SIMD3(middle.x + 3, middle.y - 2, bounds[5] + 0.01), normal: SIMD3(0, 0, 1)),
        ]
        for point in points {
            let gpu = try tracer.trace(scene, points: Array(repeating: point, count: 256), samples: 32,
                                       occlusionDistance: 10)
            let mean = gpu.map(\.irradiance).reduce(.zero, +) / Float(gpu.count)
            let cpu = scene.reference(point: [point.position.x, point.position.y, point.position.z],
                                      normal: [0, 0, 1], samples: 8192, seed: 5, occlusionDistance: 10)
            for channel in 0..<3 {
                XCTAssertEqual(mean[channel], cpu.irradiance[channel],
                               accuracy: max(cpu.irradiance[channel] * 0.05, 1e-3))
            }
            XCTAssertGreaterThan(gpu[0].direction.z, 0)
        }
    }

    func testNoPointsIsNoWorkAndABrokenSourceIsAnError() throws {
        let tracer = try tracer()
        XCTAssertEqual(try tracer.trace(try BakeScene.layout(name: "open"), points: [], samples: 1,
                                        occlusionDistance: 1), [])
        XCTAssertThrowsError(try PathTracer(device: tracer.device, source: "broken"))
        XCTAssertThrowsError(try PathTracer(device: tracer.device, source: "kernel void other() {}"))
    }

    func testTheLightAlongARayIsTheSkysUnderAnOpenSky() throws {
        let tracer = try tracer()
        let scene = try BakeScene.layout(name: "open")
        let sky = scene.sky()
        let up = BakePoint(position: .zero, normal: SIMD3(0, 0, 1))
        let seen = try tracer.radiance(scene, points: [up, up], samples: 4)
        XCTAssertEqual(seen.count, 2)
        for channel in 0..<3 {
            XCTAssertEqual(seen[0][channel], sky[channel], accuracy: sky[channel] * 0.01 + 1e-4)
        }
    }

    func testBatchesTraceEveryPointWithTheirOwnPaths() throws {
        let tracer = try tracer()
        tracer.pathsPerBatch = 1
        let scene = try BakeScene.layout(name: "furnace")
        let up = BakePoint(position: .zero, normal: SIMD3(0, 0, 1))
        let lights = try tracer.trace(scene, points: Array(repeating: up, count: 2500), samples: 4,
                                      occlusionDistance: 1)
        XCTAssertEqual(lights.count, 2500)
        // (the batches' first points are not traced alike)
        XCTAssertNotEqual(lights[0].irradiance, lights[1024].irradiance)
        XCTAssertTrue(lights.allSatisfy { $0.irradiance.x > 0 })
    }

    /// A folder of its own in the temporary folder, removed after the test.
    func folder() throws -> URL {
        let folder = FileManager.default.temporaryDirectory.appendingPathComponent("bake-\(UUID().uuidString)")
        addTeardownBlock { try? FileManager.default.removeItem(at: folder) }
        return folder
    }

    func testALevelsBakeIsKeptAndDrawnWith() throws {
        let tracer = try tracer()
        let geometry = try LevelGeometry.devmap(donor: nil, cacheFolder: "")
        let bake = try geometry.bakePlan(quick: true)
        let folder = try folder().path
        XCTAssertNil(LightBaker.cached(bake, folder: folder))
        let light = try LightBaker.light(bake, folder: folder, tracer: tracer)
        let file = try XCTUnwrap(LightBaker.file(bake, folder: folder))
        XCTAssertTrue(FileManager.default.fileExists(atPath: file.path))
        let kept = try XCTUnwrap(LightBaker.cached(bake, folder: folder))
        XCTAssertEqual(kept.encode(), light.encode())
        XCTAssertNil(LightBaker.file(bake, folder: ""))
        try LightBaker.store(light, bake: bake, folder: "")
        // (the devmap's floor is lit from above: its probes see light from below)
        let counts = light.probeCounts()
        XCTAssertEqual(counts.count, 3)
        let below = Array(light.probeBytes(axis: 5))
        XCTAssertTrue(below.contains { $0 != 0 })

        let renderer = try LevelRenderer(device: tracer.device, geometry: geometry)
        let camera = ViewCamera(position: [0, 0, 2], yaw: 0, pitch: -0.5, verticalFov: 70 * .pi / 180)
        let queue = try XCTUnwrap(tracer.device.makeCommandQueue())
        let stock = try renderer.capture(width: 64, height: 36, camera: camera, queue: queue)
        XCTAssertFalse(renderer.baked)
        try renderer.useBakedLighting(kept)
        XCTAssertTrue(renderer.baked)
        // (the devmap has no stock lightmaps: the probes light it)
        XCTAssertEqual(renderer.bakedDrawCount, 0)
        let lit = try renderer.capture(width: 64, height: 36, camera: camera, queue: queue)
        XCTAssertGreaterThan(lit.distinctColors, 1)
        // (lit by the bake, not by the normals' stand-in)
        XCTAssertGreaterThan(try XCTUnwrap(lit.difference(from: stock)).share, 0)
    }

    func testABakeInTheBackgroundComesBackOnTheMainThread() throws {
        let tracer = try tracer()
        let bake = try LevelGeometry.devmap(donor: nil, cacheFolder: "").bakePlan(quick: true)
        let done = expectation(description: "baked")
        LightBaker.bakeInBackground(bake, folder: try folder().path, device: tracer.device) { result in
            XCTAssertTrue(Thread.isMainThread)
            XCTAssertNotNil(try? result.get())
            done.fulfill()
        }
        wait(for: [done], timeout: 120)
    }

    func testAStockLevelsPagesAreBaked() throws {
        let level = try StockLevel(map: "bloodgulch")
        let tracer = try PathTracer(device: level.device)
        let bake = try level.geometry.bakePlan(quick: true)
        XCTAssertFalse(bake.lightmapPointBytes().isEmpty)
        let light = try LightBaker.bake(bake, tracer: tracer)
        XCTAssertFalse(light.pages().isEmpty)
        let renderer = try LevelRenderer(device: level.device, geometry: level.geometry)
        try renderer.useBakedLighting(light)
        XCTAssertGreaterThan(renderer.bakedDrawCount, 0)
        let frame = try renderer.capture(width: 320, height: 180, camera: level.camera, queue: level.queue)
        XCTAssertGreaterThan(frame.distinctColors, 64)
    }
}
