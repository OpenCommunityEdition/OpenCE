import Foundation
import Metal
import OpenCEEngineCore
import OpenCEEngineKit
import OpenCEEngineRender
import XCTest

/// The edges of Blood Gulch's baked shadows (devgulch's; needs assets/maps):
/// each texel's light is gathered over its footprint on the surface, so a
/// shadow's edge falls across texels smoothly instead of in steps a texel
/// wide. Neighbouring texels of one surface step hard in light (by more
/// than a quarter of the range) no more often than on the stock lightmaps.
final class ShadowEdgeTests: XCTestCase {
    func testTheBakedShadowsEdgesStepNoMoreOftenThanStock() throws {
        let level = try StockLevel(map: "bloodgulch")
        let plan = try level.geometry.bakePlan(quick: false)
        let light = try LightBaker.bake(plan, tracer: PathTracer(device: level.device))
        let edges = plan.edges(light: light)
        XCTAssertGreaterThan(edges.pairs, 10000, "Blood Gulch's pages have neighbouring texels")
        XCTAssertGreaterThan(edges.stockPairs, 1000, "and so do its stock pages")
        let baked = Float(edges.hard) / Float(edges.pairs)
        let stock = Float(edges.stockHard) / Float(edges.stockPairs)
        print(
            "edges: baked \(edges.hard)/\(edges.pairs) hard (\(baked)), mean \(edges.mean); "
                + "stock \(edges.stockHard)/\(edges.stockPairs) hard (\(stock)), mean \(edges.stockMean)")
        XCTAssertLessThanOrEqual(baked, stock, "the share of neighbours that step hard")
    }
}
