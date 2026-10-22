// The bake's path tracer on the GPU (Shaders/bake.metal): a scene from
// the engine (BakeScene: the BSP's hierarchy, materials and lights) in
// buffers, and the light found at each point asked for, one thread a
// point. opence_bake's trace.rs is its reference.
import Foundation
import Metal
import OpenCEEngineCore
import simd

/// A point to find the light at: on a surface, facing its normal.
public struct BakePoint: Equatable, Sendable {
    public var position: SIMD3<Float>
    public var normal: SIMD3<Float>

    public init(position: SIMD3<Float>, normal: SIMD3<Float>) {
        self.position = position
        self.normal = normal
    }
}

/// The light found at a point.
public struct BakeLight: Equatable, Sendable {
    /// Red, green and blue, linear.
    public var irradiance: SIMD3<Float>
    /// The direction most of it comes from (unit, or zero).
    public var direction: SIMD3<Float>
    /// The open share of the hemisphere, within the occlusion distance.
    public var occlusion: Float
}

public enum PathTracerError: Error, CustomStringConvertible {
    case setup(String)
    case trace(String)

    public var description: String {
        switch self {
        case let .setup(why): "the path tracer could not be set up: \(why)"
        case let .trace(why): "the path tracer failed: \(why)"
        }
    }
}

public final class PathTracer {
    public let device: MTLDevice
    private let queue: MTLCommandQueue
    private let pipeline: MTLComputePipelineState

    /// The kernel's parameters, as bake.metal's BakeParameters (48 bytes).
    struct Parameters {
        var sky: (Float, Float, Float)
        var lightCount: UInt32
        var triangleCount: UInt32
        var samples: UInt32
        var seed: UInt32
        var occlusionDistance: Float
        var pointCount: UInt32
        var pad: (UInt32, UInt32, UInt32) = (0, 0, 0)
    }

    public init(device: MTLDevice, source: String? = nil) throws {
        self.device = device
        guard let queue = device.makeCommandQueue() else {
            throw PathTracerError.setup("no command queue")
        }
        self.queue = queue
        let options = MTLCompileOptions()
        // (the slab test divides by zero and relies on the infinities)
        options.mathMode = .safe
        do {
            let library = try device.makeLibrary(source: source ?? ShaderSources.source(.bake), options: options)
            pipeline = try device.makeComputePipelineState(function: try library.required("bake_irradiance"))
        } catch let error as PathTracerError {
            throw error
        } catch {
            throw PathTracerError.setup("\(error)")
        }
    }

    /// A buffer of `bytes`, or of one zero word when there are none (Metal
    /// makes no empty buffers).
    private func buffer(_ bytes: [UInt8]) throws -> MTLBuffer {
        let bytes = bytes.isEmpty ? [UInt8](repeating: 0, count: 48) : bytes
        guard let buffer = bytes.withUnsafeBytes({ raw in
            device.makeBuffer(bytes: raw.baseAddress!, length: raw.count, options: .storageModeShared)
        }) else {
            throw PathTracerError.trace("no buffer of \(bytes.count) bytes")
        }
        return buffer
    }

    /// The light at each of `points` in `scene`, from `samples` paths each.
    public func trace(_ scene: BakeScene, points: [BakePoint], samples: UInt32, seed: UInt32 = 1,
                      occlusionDistance: Float) throws -> [BakeLight] {
        if points.isEmpty {
            return []
        }
        let nodes = try buffer(Array(scene.nodeBytes()))
        let triangles = try buffer(Array(scene.triangleBytes()))
        let materials = try buffer(Array(scene.materialBytes()))
        let lightBytes = Array(scene.lightBytes())
        let lights = try buffer(lightBytes)
        let sky = scene.sky()
        var parameters = Parameters(sky: (sky[0], sky[1], sky[2]), lightCount: UInt32(lightBytes.count / 32),
                                    triangleCount: scene.triangleCount(), samples: samples, seed: seed,
                                    occlusionDistance: occlusionDistance, pointCount: UInt32(points.count))
        var packed: [Float] = []
        packed.reserveCapacity(points.count * 8)
        for point in points {
            packed += [point.position.x, point.position.y, point.position.z, 0,
                       point.normal.x, point.normal.y, point.normal.z, 0]
        }
        let pointBuffer = try buffer(packed.withUnsafeBytes { Array($0) })
        let length = points.count * MemoryLayout<SIMD4<Float>>.stride
        guard let irradiance = device.makeBuffer(length: length, options: .storageModeShared),
              let directions = device.makeBuffer(length: length, options: .storageModeShared),
              let commands = queue.makeCommandBuffer(),
              let encoder = commands.makeComputeCommandEncoder()
        else {
            throw PathTracerError.trace("no buffers or encoder")
        }
        encoder.setComputePipelineState(pipeline)
        for (index, buffer) in [nodes, triangles, materials, lights].enumerated() {
            encoder.setBuffer(buffer, offset: 0, index: index)
        }
        encoder.setBytes(&parameters, length: MemoryLayout<Parameters>.stride, index: 4)
        encoder.setBuffer(pointBuffer, offset: 0, index: 5)
        encoder.setBuffer(irradiance, offset: 0, index: 6)
        encoder.setBuffer(directions, offset: 0, index: 7)
        let width = min(pipeline.maxTotalThreadsPerThreadgroup, 64)
        encoder.dispatchThreads(MTLSize(width: points.count, height: 1, depth: 1),
                                threadsPerThreadgroup: MTLSize(width: width, height: 1, depth: 1))
        encoder.endEncoding()
        commands.commit()
        commands.waitUntilCompleted()
        if let error = commands.error {
            throw PathTracerError.trace("\(error)")
        }
        let light = irradiance.contents().bindMemory(to: SIMD4<Float>.self, capacity: points.count)
        let direction = directions.contents().bindMemory(to: SIMD4<Float>.self, capacity: points.count)
        return (0..<points.count).map { index in
            BakeLight(irradiance: SIMD3(light[index].x, light[index].y, light[index].z),
                      direction: SIMD3(direction[index].x, direction[index].y, direction[index].z),
                      occlusion: light[index].w)
        }
    }
}
