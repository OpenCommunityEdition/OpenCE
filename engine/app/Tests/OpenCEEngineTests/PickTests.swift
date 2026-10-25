import Metal
import OpenCEEngineCore
import OpenCEEngineKit
@testable import OpenCEEngineRender
import simd
import XCTest

/// The dev panel's mouse pick: the object drawn under the cursor, on Blood
/// Gulch's level from its start view.
final class PickTests: XCTestCase {
    /// The narrowed view draws the pixel's middle at the middle of its
    /// one pixel.
    func testTheNarrowedViewCentresThePixel() {
        let (width, height, x, y) = (480, 270, 100, 40)
        let middle = SIMD4<Float>(
            (2 * (Float(x) + 0.5)) / Float(width) - 1,
            1 - (2 * (Float(y) + 0.5)) / Float(height), 0.5, 1)
        let narrowed = PickPass.narrowing(x: x, y: y, width: width, height: height) * (middle * 2)
        XCTAssertEqual(narrowed.x / narrowed.w, 0, accuracy: 1e-4)
        XCTAssertEqual(narrowed.y / narrowed.w, 0, accuracy: 1e-4)
        XCTAssertEqual(narrowed.z / narrowed.w, 0.5, accuracy: 1e-6)
        // (the next pixel to the right is a pixel's width away: out of view)
        let next = PickPass.narrowing(x: x + 1, y: y, width: width, height: height) * middle
        XCTAssertEqual(next.x / next.w, -2, accuracy: 1e-3)
    }

    func testAPickReturnsTheObjectUnderTheCursor() throws {
        let level = try StockLevel(map: "bloodgulch")
        let renderer = try level.renderer(VideoSettings(antiAliasing: .native, renderScale: 1))
        let (width, height) = (480, 270)
        XCTAssertNil(
            renderer.pick(x: 10, y: 10, width: width, height: height),
            "a pick before a frame")
        _ = try renderer.capture(
            width: width, height: height, camera: level.camera, objects: level.objects,
            queue: level.queue)
        let viewProjection = level.camera.viewProjection(aspect: Float(width) / Float(height))
        let ids = Set(level.objects.map(\.id))
        var picked = 0
        for object in level.objects where !object.firstPerson && object.id != .max {
            let clip = viewProjection * SIMD4(object.center, 1)
            guard clip.w > 0 else { continue }
            let x = Int(((clip.x / clip.w) + 1) / 2 * Float(width))
            let y = Int((1 - clip.y / clip.w) / 2 * Float(height))
            guard (0..<width).contains(x), (0..<height).contains(y) else { continue }
            guard let id = renderer.pick(x: x, y: y, width: width, height: height) else {
                continue
            }
            XCTAssertTrue(ids.contains(id), "picked \(id), not an object")
            if id == object.id { picked += 1 }
        }
        XCTAssertGreaterThan(picked, 0, "no object in view was picked at its middle")
        // (the sky, above the view's middle at its top: no object)
        XCTAssertNil(renderer.pick(x: width / 2, y: 0, width: width, height: height))
        XCTAssertNil(renderer.pick(x: -1, y: 0, width: width, height: height))
    }
}
