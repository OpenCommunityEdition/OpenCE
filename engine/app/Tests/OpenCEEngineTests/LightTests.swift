import Foundation
import Metal
import opence_ffiFFI
import OpenCEEngineCore
import OpenCEEngineRender
import simd
import XCTest

/// The dynamic lights (LightPass.swift and level.metal's dynamic_light):
/// a light brightens what it reaches and nothing past its radius, a
/// spot only inside its cone, and the nearest lights cast shadows.
final class LightTests: XCTestCase {
    func light(at position: SIMD3<Float>, radius: Float, color: SIMD3<Float>) -> OCELightSnapshot {
        var light = OCELightSnapshot()
        light.position = OCEVec3(x: position.x, y: position.y, z: position.z)
        light.previous = light.position
        light.radius = radius
        light.specular_radius = radius
        light.cosine_cutoff = -1
        light.cosine_falloff = -1
        light.forward = OCEVec3(x: 1, y: 0, z: 0)
        light.up = OCEVec3(x: 0, y: 0, z: 1)
        light.color = OCEVec3(x: color.x, y: color.y, z: color.z)
        light.light = .max
        return light
    }

    func testALightBrightensWhatItReachesAndASpotOnlyInItsCone() throws {
        let level = try StockLevel(map: "bloodgulch")
        let renderer = try level.renderer(VideoSettings(antiAliasing: .native, renderScale: 1))
        // (above the largest object, looking down on it and the ground around)
        let object = try XCTUnwrap(level.objects.max { $0.radius < $1.radius })
        let camera = ViewCamera(position: object.center + SIMD3(0, 0, object.radius * 5 + 2), yaw: 0, pitch: -1.45,
                                verticalFov: 70 * .pi / 180)
        func capture(_ lights: [OCELightSnapshot]) throws -> CapturedFrame {
            renderer.lights = lights
            return try renderer.capture(width: 320, height: 180, camera: camera, objects: level.objects,
                                        queue: level.queue)
        }
        func brighter(_ a: CapturedFrame, than b: CapturedFrame) -> Int {
            var count = 0
            for y in 0..<a.height {
                for x in 0..<a.width {
                    let (first, second) = (a.rgb(x: x, y: y), b.rgb(x: x, y: y))
                    if (0..<3).contains(where: { Int(first[$0]) - Int(second[$0]) > 8 }) {
                        count += 1
                    }
                }
            }
            return count
        }
        let dark = try capture([])
        XCTAssertEqual(renderer.lightsDrawn, 0)
        let reach = object.radius * 4 + 4
        let above = object.center + SIMD3(0, 0, object.radius + 1)
        let point = light(at: above, radius: reach, color: [4, 3, 2])
        let lit = try capture([point])
        XCTAssertEqual(renderer.lightsDrawn, 1)
        XCTAssertEqual(renderer.lightShadowsDrawn, 1, "the nearest light casts shadows")
        XCTAssertGreaterThan(brighter(lit, than: dark), lit.width * lit.height / 20, "the light lit nothing")
        XCTAssertEqual(try capture([]).rgb(x: 160, y: 90), dark.rgb(x: 160, y: 90), "the light stayed")

        // (one far beyond its radius lights nothing)
        let far = light(at: above + SIMD3(0, 0, reach * 3), radius: reach * 0.5, color: [4, 3, 2])
        XCTAssertLessThan(brighter(try capture([far]), than: dark), 20, "a light lit past its radius")

        // a spot pointed away from the ground lights nothing; pointed down,
        // less than the point light around it
        var away = point
        away.flags = UInt32(OCE_LIGHT_SPOT)
        away.forward = OCEVec3(x: 0, y: 0, z: 1)
        away.up = OCEVec3(x: 1, y: 0, z: 0)
        away.cosine_cutoff = cos(0.4)
        away.cosine_falloff = cos(0.3)
        XCTAssertLessThan(brighter(try capture([away]), than: dark), 20, "a spot lit behind it")
        var down = away
        down.forward = OCEVec3(x: 0, y: 0, z: -1)
        let spot = brighter(try capture([down]), than: dark)
        XCTAssertGreaterThan(spot, 20, "a spot lit nothing in its cone")
        XCTAssertLessThan(spot, brighter(lit, than: dark), "a spot lit outside its cone")
    }
}
