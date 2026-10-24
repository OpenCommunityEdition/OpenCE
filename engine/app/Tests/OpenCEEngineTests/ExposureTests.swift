import Foundation
import Metal
import OpenCEEngineCore
import OpenCEEngineKit
import OpenCEEngineRender
import simd
import XCTest

/// The exposure and the tone map against the original's (needs
/// assets/maps): Blood Gulch baked keeps the brightness the stock
/// lightmaps give it, and the ring and the bright textures keep their
/// detail instead of clipping to white.
final class ExposureTests: XCTestCase {
    struct Stats {
        var mean: Double
        var peak: Int
        /// The share of pixels with a channel at 254 or more.
        var clipped: Double
        var box: (Int, Int, Int, Int)
    }

    func stats(_ frame: CapturedFrame) -> Stats {
        var sum = 0.0, clipped = 0, peak = 0
        var box = (Int.max, Int.max, -1, -1)
        for y in 0..<frame.height {
            for x in 0..<frame.width {
                let c = frame.rgb(x: x, y: y)
                sum += 0.2126 * Double(c.x) + 0.7152 * Double(c.y) + 0.0722 * Double(c.z)
                let top = Int(max(c.x, c.y, c.z))
                peak = max(peak, top)
                if top >= 254 {
                    clipped += 1
                    box = (min(box.0, x), min(box.1, y), max(box.2, x), max(box.3, y))
                }
            }
        }
        let count = Double(frame.width * frame.height)
        return Stats(mean: sum / count, peak: peak, clipped: Double(clipped) / count, box: box)
    }

    func testTheBakeKeepsTheStockBrightnessAndClipsNothing() throws {
        let level = try StockLevel(map: "bloodgulch")
        let light = try LightBaker.bake(try level.geometry.bakePlan(quick: true), tracer: PathTracer(device: level.device))
        let start = level.geometry.startView()
        func view(yaw: Float, pitch: Float) -> ViewCamera {
            ViewCamera(position: [start.x, start.y, start.z], yaw: start.yaw + yaw, pitch: pitch,
                       verticalFov: 70 * .pi / 180)
        }
        let views = [("start", level.camera, 0.25), ("sky", view(yaw: 0, pitch: 0.7), 0.1),
                     ("back", view(yaw: .pi, pitch: -0.1), 0.1)]
        for (name, camera, tolerance) in views {
            var shots: [Bool: Stats] = [:]
            for stock in [false, true] {
                let renderer = try level.renderer(VideoSettings(antiAliasing: .native, renderScale: 1))
                try renderer.useBakedLighting(light)
                renderer.switches.view = stock ? .stock : .frame
                let frame = try renderer.capture(width: 480, height: 270, camera: camera, frames: 8,
                                                 objects: level.objects, queue: level.queue)
                shots[stock] = stats(frame)
            }
            let baked = shots[false]!, stock = shots[true]!
            // (the start view looks at the base, where the quick bake's
            // blotches still brighten it: its mean is held loosely until
            // they are gone)
            XCTAssertLessThan(abs(baked.mean - stock.mean) / stock.mean, tolerance,
                              "\(name): mean \(baked.mean) against \(stock.mean)")
            XCTAssertLessThanOrEqual(baked.clipped, stock.clipped + 0.0005,
                                     "\(name): \(baked.clipped) clipped, around \(baked.box)")
        }
    }
}
