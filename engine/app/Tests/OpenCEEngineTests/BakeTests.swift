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
}
