import Foundation
import Metal
import OpenCEEngineCore
import OpenCEEngineKit
import OpenCEEngineRender
import simd
import XCTest

/// The lightmap seams of Blood Gulch's level (devgulch's) baked (needs
/// assets/maps): where two triangles of one surface meet in the level but
/// lie apart on the lightmap pages, the baked light steps across the edge no
/// more than the stock lightmaps' does, and the longest seam's floor, seen
/// close up, has no more black than stock (a texture's seam drawn without
/// its texels shows as black). The view is written to build/engine/golden
/// (beside the stock view's) and compared with its approved frame, if there
/// is one, in assets/golden/engine.
final class SeamTests: XCTestCase {
    /// The share of `seams`' length whose step is over `limit`, and the
    /// length-weighted mean step, by `step`.
    func summary(_ seams: [BakeSeam], _ step: (BakeSeam) -> Float, over limit: Float) -> (mean: Float, over: Float) {
        let total = seams.reduce(0) { $0 + $1.length }
        let mean = seams.reduce(0) { $0 + step($1) * $1.length } / total
        let over = seams.filter { step($0) > limit }.reduce(0) { $0 + $1.length } / total
        return (mean, over)
    }

    func testTheBakedLightStepsAcrossNoSeamMoreThanStock() throws {
        let level = try StockLevel(map: "bloodgulch")
        let plan = try level.geometry.bakePlan(quick: true)
        let light = try LightBaker.bake(plan, tracer: PathTracer(device: level.device))
        let seams = plan.seams(light: light).filter { $0.baked != nil && $0.stock != nil && $0.length > 0 }
        XCTAssertGreaterThan(seams.count, 100, "Blood Gulch's level is cut into charts")
        let visible: Float = 0.05
        let baked = summary(seams, { $0.baked! }, over: visible)
        let stock = summary(seams, { $0.stock! }, over: visible)
        print("seams: \(seams.count), baked mean \(baked.mean) over \(baked.over), stock mean \(stock.mean) over \(stock.over)")
        XCTAssertLessThanOrEqual(baked.mean, stock.mean, "the mean step across a seam")
        XCTAssertLessThanOrEqual(baked.over, stock.over, "the share of seams that show")

        // the longest seam on a floor, seen from above and to one side
        let floor = try XCTUnwrap(seams.filter { $0.normal[2] > 0.9 }.max { $0.length < $1.length })
        let ends = [SIMD3(floor.ends[0], floor.ends[1], floor.ends[2]), SIMD3(floor.ends[3], floor.ends[4], floor.ends[5])]
        let middle = (ends[0] + ends[1]) / 2, normal = SIMD3(floor.normal[0], floor.normal[1], floor.normal[2])
        let across = simd_normalize(simd_cross(ends[1] - ends[0], normal))
        let position = middle + normal * 1.2 + across * 1.2
        let look = simd_normalize(middle - position)
        let camera = ViewCamera(position: position, yaw: atan2(look.y, look.x),
                                pitch: asin(min(max(look.z, -0.999), 0.999)), verticalFov: 70 * .pi / 180)
        var shots: [Bool: CapturedFrame] = [:]
        for stock in [false, true] {
            let renderer = try level.renderer(VideoSettings(antiAliasing: .native, renderScale: 1))
            try renderer.useBakedLighting(light)
            renderer.switches.view = stock ? .stock : .frame
            shots[stock] = try renderer.capture(width: 480, height: 270, camera: camera, frames: 8,
                                                objects: level.objects, queue: level.queue)
        }
        let exposure = ExposureTests()
        let bakedStats = exposure.stats(shots[false]!), stockStats = exposure.stats(shots[true]!)
        XCTAssertLessThanOrEqual(bakedStats.black, stockStats.black + 0.0005,
                                 "\(bakedStats.black) black against \(stockStats.black)")
        let out = StockLevel.root.appendingPathComponent("build/engine/golden")
        try FileManager.default.createDirectory(at: out, withIntermediateDirectories: true)
        let file = "devgulch_seam_480x270.png"
        try shots[false]!.writePNG(to: out.appendingPathComponent(file))
        try shots[true]!.writePNG(to: out.appendingPathComponent("devgulch_seam_stock_480x270.png"))
        let approved = StockLevel.root.appendingPathComponent("assets/golden/engine/\(file)")
        if let approvedFrame = CapturedFrame.readPNG(at: approved) {
            let difference = try XCTUnwrap(shots[false]!.difference(from: approvedFrame), file)
            XCTAssertLessThan(difference.share, 0.005, "\(file) differs from the approved frame")
        } else {
            print("golden frame \(file): none approved yet (assets/golden/engine)")
        }
    }
}
