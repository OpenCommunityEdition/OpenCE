import Foundation
import Metal
import OpenCEEngineCore
import OpenCEEngineKit
import OpenCEEngineRender
import simd
import XCTest

/// Each shader class's surfaces on Blood Gulch's level (devgulch's), and
/// its brightest glowing surfaces, seen close up with the baked light (needs
/// assets/maps): no class's surface has more black than the stock lightmaps
/// leave it (a NaN or a lost path shows as black), and each view is written
/// to build/engine/golden (beside the stock view's) and compared with its
/// approved frame, if there is one, in assets/golden/engine. The glowing
/// surfaces are Blood Gulch's cave fungus, whose caves the bake leaves darker
/// than stock (docs/engine.md, "The caves are dark"): they are held to half
/// of stock's mean and a little black in their corners, so that a lost glow
/// still fails.
final class ShaderClassTests: XCTestCase {
    /// The class `material` is drawn as.
    func shaderClass(_ material: RenderMaterial) -> String {
        if material.kind == .plain { return "plain" }
        if let transparent = material.transparent { return "\(transparent.class)" }
        if !material.chicagoMaps.isEmpty { return "chicago" }
        if let environment = material.environment {
            return environment.selfIlluminationMap == nil ? "environment" : "environment_glowing"
        }
        return material.model == nil ? "other" : "model"
    }

    /// A camera `distance` in front of the triangle `corners` (its front is
    /// against its winding's cross product, as the bake's is), looking at
    /// its middle.
    func view(_ corners: [SIMD3<Float>], distance: Float = 1.5) -> ViewCamera? {
        let cross = simd_cross(corners[1] - corners[0], corners[2] - corners[0])
        guard simd_length(cross) > 1e-6 else { return nil }
        let normal = -simd_normalize(cross)
        let middle = (corners[0] + corners[1] + corners[2]) / 3
        let look = -normal
        return ViewCamera(position: middle + normal * distance, yaw: atan2(look.y, look.x),
                          pitch: asin(min(max(look.z, -0.999), 0.999)), verticalFov: 70 * .pi / 180)
    }

    /// Each class's largest draw, seen in front of its largest triangle
    /// (of the first thousand).
    func classViews(_ geometry: LevelGeometry) -> [(String, ViewCamera)] {
        let stride = Int(geometry.vertexStride())
        let vertices = geometry.vertices(), indices = geometry.indices()
        func position(_ vertex: UInt32) -> SIMD3<Float> {
            vertices.withUnsafeBytes { raw in
                let at = Int(vertex) * stride
                return SIMD3(raw.loadUnaligned(fromByteOffset: at, as: Float.self),
                             raw.loadUnaligned(fromByteOffset: at + 4, as: Float.self),
                             raw.loadUnaligned(fromByteOffset: at + 8, as: Float.self))
            }
        }
        func index(_ at: Int) -> UInt32 {
            indices.withUnsafeBytes { $0.loadUnaligned(fromByteOffset: at * 4, as: UInt32.self) }
        }
        let materials = geometry.materials()
        var largest: [String: RenderDraw] = [:]
        for draw in geometry.draws() where Int(draw.material) < materials.count {
            let name = shaderClass(materials[Int(draw.material)])
            if draw.indexCount > largest[name]?.indexCount ?? 0 { largest[name] = draw }
        }
        var views: [(String, ViewCamera)] = []
        for (name, draw) in largest.sorted(by: { $0.key < $1.key }) {
            var best: (Float, [SIMD3<Float>])?
            for triangle in 0..<min(Int(draw.indexCount) / 3, 1000) {
                let first = Int(draw.firstIndex) + triangle * 3
                let corners = (0..<3).map { position(index(first + $0)) }
                let area = simd_length(simd_cross(corners[1] - corners[0], corners[2] - corners[0]))
                if area > best?.0 ?? 0 { best = (area, corners) }
            }
            if let corners = best?.1, let camera = view(corners) { views.append((name, camera)) }
        }
        return views
    }

    /// The `count` brightest glowing triangles' views (by the light each
    /// gives: its emission's luminance times its area), seen from the side
    /// that glows.
    func emitterViews(_ scene: BakeScene, count: Int) -> [(String, ViewCamera)] {
        let bytes = scene.emitterBytes()
        var emitters: [(Float, [SIMD3<Float>])] = []
        bytes.withUnsafeBytes { raw in
            for at in Swift.stride(from: 0, to: raw.count - 63, by: 64) {
                func word(_ index: Int) -> Float { raw.loadUnaligned(fromByteOffset: at + index * 4, as: Float.self) }
                let corners = [SIMD3(word(0), word(1), word(2)), SIMD3(word(4), word(5), word(6)),
                               SIMD3(word(8), word(9), word(10))]
                let power = (0.2126 * word(12) + 0.7152 * word(13) + 0.0722 * word(14)) * word(7)
                emitters.append((power, corners))
            }
        }
        return emitters.sorted { $0.0 > $1.0 }.prefix(count).enumerated().compactMap { index, emitter in
            view(emitter.1, distance: 2).map { ("emitter\(index)", $0) }
        }
    }

    func testNoShaderClassOrGlowDrawsBlack() throws {
        let level = try StockLevel(map: "bloodgulch")
        let light = try LightBaker.bake(try level.geometry.bakePlan(quick: false), tracer: PathTracer(device: level.device))
        let views = classViews(level.geometry) + emitterViews(try level.geometry.bakeScene(), count: 3)
        XCTAssertGreaterThanOrEqual(views.filter { !$0.0.hasPrefix("emitter") }.count, 2, "\(views.map(\.0))")
        XCTAssertEqual(views.filter { $0.0.hasPrefix("emitter") }.count, 3, "Blood Gulch's cave fungus glows")
        let out = StockLevel.root.appendingPathComponent("build/engine/golden")
        try FileManager.default.createDirectory(at: out, withIntermediateDirectories: true)
        let exposure = ExposureTests()
        for (name, camera) in views {
            var shots: [Bool: CapturedFrame] = [:]
            for stock in [false, true] {
                let renderer = try level.renderer(VideoSettings(antiAliasing: .native, renderScale: 1))
                try renderer.useBakedLighting(light)
                renderer.switches.view = stock ? .stock : .frame
                shots[stock] = try renderer.capture(width: 480, height: 270, camera: camera, frames: 8,
                                                    objects: level.objects, queue: level.queue)
            }
            let baked = exposure.stats(shots[false]!), stock = exposure.stats(shots[true]!)
            let glow = name.hasPrefix("emitter")
            XCTAssertLessThanOrEqual(baked.black, stock.black + (glow ? 0.005 : 0.0005),
                                     "\(name): \(baked.black) black against \(stock.black)")
            if glow {
                XCTAssertGreaterThan(baked.mean, stock.mean / 2, "\(name): mean \(baked.mean) against \(stock.mean)")
            }
            let file = "devgulch_shader_\(name)_480x270.png"
            try shots[false]!.writePNG(to: out.appendingPathComponent(file))
            try shots[true]!.writePNG(to: out.appendingPathComponent("devgulch_shader_\(name)_stock_480x270.png"))
            let approved = StockLevel.root.appendingPathComponent("assets/golden/engine/\(file)")
            if let approvedFrame = CapturedFrame.readPNG(at: approved) {
                let difference = try XCTUnwrap(shots[false]!.difference(from: approvedFrame), file)
                XCTAssertLessThan(difference.share, 0.005, "\(file) differs from the approved frame")
            } else {
                print("golden frame \(file): none approved yet (assets/golden/engine)")
            }
        }
    }
}
