import Foundation
import Metal
import OpenCEEngineCore
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

/// The golden frames: each stock map's first BSP from its start view, at
/// three sizes, written to build/engine/golden and compared with the
/// approved frames in assets/golden/engine if they are there (never
/// committed: they are the game's art).
final class GoldenFrameTests: XCTestCase {
    static let root = URL(fileURLWithPath: #filePath).deletingLastPathComponent()
        .deletingLastPathComponent().deletingLastPathComponent().deletingLastPathComponent()
        .deletingLastPathComponent()
    static let sizes = [(1920, 1080), (3440, 1440), (3840, 2160)]

    func render(map: String) throws {
        let mapURL = Self.root.appendingPathComponent("assets/maps/\(map).map")
        guard FileManager.default.fileExists(atPath: mapURL.path) else {
            throw XCTSkip("no assets/maps/\(map).map")
        }
        guard let device = MTLCreateSystemDefaultDevice(), let queue = device.makeCommandQueue() else {
            throw XCTSkip("no Metal device")
        }
        let cache = Self.root.appendingPathComponent("build/engine/cache")
        let out = Self.root.appendingPathComponent("build/engine/golden")
        try FileManager.default.createDirectory(at: cache, withIntermediateDirectories: true)
        try FileManager.default.createDirectory(at: out, withIntermediateDirectories: true)
        let geometry = try LevelGeometry.load(map: mapURL.path, cacheFolder: cache.path, bsp: 0)
        let renderer = try LevelRenderer(device: device, geometry: geometry)
        XCTAssertGreaterThan(renderer.drawCount, 10)
        let start = geometry.startView()
        let camera = ViewCamera(position: [start.x, start.y, start.z], yaw: start.yaw, pitch: start.pitch,
                                verticalFov: 70 * .pi / 180)
        for (width, height) in Self.sizes {
            let name = "\(map)_\(width)x\(height).png"
            let frame = try renderer.capture(width: width, height: height, camera: camera, queue: queue)
            try frame.writePNG(to: out.appendingPathComponent(name))
            // drawn: many colours, and most of the frame not the sky's clear colour
            XCTAssertGreaterThan(frame.distinctColors, 256, name)
            let sky = SIMD3<UInt8>(UInt8(LevelRenderer.skyColor.red * 255), UInt8(LevelRenderer.skyColor.green * 255),
                                   UInt8(LevelRenderer.skyColor.blue * 255))
            var level = 0
            for y in stride(from: 0, to: height, by: 16) {
                for x in stride(from: 0, to: width, by: 16) {
                    let texel = SIMD3<Int>(truncatingIfNeeded: frame.rgb(x: x, y: y))
                    let off = texel &- SIMD3<Int>(truncatingIfNeeded: sky)
                    let distance = abs(off.x) + abs(off.y) + abs(off.z)
                    if distance > 6 { level += 1 }
                }
            }
            let samples = ((width + 15) / 16) * ((height + 15) / 16)
            XCTAssertGreaterThan(Double(level) / Double(samples), 0.2, "\(name): mostly the clear colour")
            let approved = Self.root.appendingPathComponent("assets/golden/engine/\(name)")
            if let approvedFrame = CapturedFrame.readPNG(at: approved) {
                let difference = try XCTUnwrap(frame.difference(from: approvedFrame), "\(name): another size")
                XCTAssertLessThan(difference.share, 0.005, "\(name) differs from the approved frame")
            } else {
                print("golden frame \(name): none approved yet (assets/golden/engine)")
            }
        }
    }

    func testBloodGulch() throws { try render(map: "bloodgulch") }
    func testTheMawsFirstBsp() throws { try render(map: "a30") }
}
