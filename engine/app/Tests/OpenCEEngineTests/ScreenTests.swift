import Foundation
import Metal
import OpenCEEngineCore
import OpenCEEngineRender
import simd
import XCTest

/// The screen's passes (ScreenPass.swift and Shaders/screen.metal): the
/// ambient occlusion and the contact shadows darken the ground by an
/// object, and each pass can be switched off and shown alone.
final class ScreenTests: XCTestCase {
    func testEachPassChangesTheFrameAndCanBeShownAlone() throws {
        let level = try StockLevel(map: "bloodgulch")
        let bake = try level.geometry.bakePlan(quick: true)
        func even(_ points: Data, _ value: Float) -> Data {
            let floats = [Float](repeating: value, count: points.count / 32 * 4)
            return floats.withUnsafeBytes { Data($0) }
        }
        let light = bake.finish(irradiance: even(bake.lightmapPointBytes(), 1),
                                directions: even(bake.lightmapPointBytes(), 0),
                                probes: even(bake.probePointBytes(), 0.5),
                                radiance: even(bake.reflectionPointBytes(), 0))
        let renderer = try level.renderer(VideoSettings(antiAliasing: .native, renderScale: 1))
        try renderer.useBakedLighting(light)
        // (above the largest object, looking down on it and the ground around)
        let object = try XCTUnwrap(level.objects.max { $0.radius < $1.radius })
        let camera = ViewCamera(position: object.center + SIMD3(0, 0, object.radius * 5 + 2), yaw: 0, pitch: -1.45,
                                verticalFov: 70 * .pi / 180)
        func capture(_ change: (inout RenderSwitches) -> Void = { _ in }) throws -> CapturedFrame {
            var switches = RenderSwitches()
            change(&switches)
            renderer.switches = switches
            return try renderer.capture(width: 320, height: 180, camera: camera, objects: level.objects,
                                        queue: level.queue)
        }
        func differing(_ a: CapturedFrame, _ b: CapturedFrame) -> Int {
            var count = 0
            for y in 0..<a.height {
                for x in 0..<a.width {
                    let (first, second) = (a.rgb(x: x, y: y), b.rgb(x: x, y: y))
                    if (0..<3).contains(where: { abs(Int(first[$0]) - Int(second[$0])) > 4 }) {
                        count += 1
                    }
                }
            }
            return count
        }
        let frame = try capture()
        let none = try capture { $0.gtao = false; $0.contactShadows = false; $0.ssr = false }
        XCTAssertGreaterThan(differing(frame, none), 50, "the passes changed nothing")
        XCTAssertGreaterThan(differing(try capture { $0.gtao = false }, frame), 20, "the occlusion did nothing")
        XCTAssertGreaterThan(differing(try capture { $0.contactShadows = false }, frame), 0,
                             "the contact shadows did nothing")
        XCTAssertEqual(differing(try capture(), frame), 0, "a switch stayed off")

        // each view is not the frame; the ground's normals, seen from
        // above, point up (blue)
        var views: [RenderView: CapturedFrame] = [:]
        for view in RenderView.allCases where view != .frame {
            views[view] = try capture { $0.view = view }
            XCTAssertGreaterThan(differing(try XCTUnwrap(views[view]), frame), frame.width * frame.height / 4,
                                 "\(view) showed the frame")
        }
        let normals = try XCTUnwrap(views[.normals]).rgb(x: 4, y: 4)
        XCTAssertGreaterThan(Int(normals[2]), Int(normals[0]), "the ground's normals are not up")
        XCTAssertGreaterThan(Int(normals[2]), Int(normals[1]), "the ground's normals are not up")
        // (shown alone, a pass switched off is still computed)
        XCTAssertEqual(differing(try capture { $0.gtao = false; $0.view = .ao }, try XCTUnwrap(views[.ao])), 0)
    }
}
