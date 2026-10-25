// The bake's path tracer on the GPU (Shaders/bake.metal): a scene from
// the engine (BakeScene: the BSP's hierarchy, materials and lights) in
// buffers, and the light found at each point asked for, one thread a
// point, in batches small enough for the GPU's watchdog. opence_bake's
// trace.rs is its reference.
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
    /// How much of the light comes from that direction, 0 to 1.
    public var directionality: Float
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
    private let radiancePipeline: MTLComputePipelineState
    /// The most paths one command buffer traces (its points times their
    /// samples).
    public var pathsPerBatch = 2_000_000

    /// The kernel's parameters, as bake.metal's BakeParameters (48 bytes).
    struct Parameters {
        var sky: (Float, Float, Float)
        var lightCount: UInt32
        var triangleCount: UInt32
        var samples: UInt32
        var seed: UInt32
        var occlusionDistance: Float
        var pointCount: UInt32
        var emitterCount: UInt32
        /// The sky's light left out, plus one (0: none).
        var sun: UInt32 = 0
        var pad: UInt32 = 0
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
            radiancePipeline = try device.makeComputePipelineState(
                function: try library.required("bake_radiance_along"))
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
        guard
            let buffer = bytes.withUnsafeBytes({ raw in
                device.makeBuffer(bytes: raw.baseAddress!, length: raw.count, options: .storageModeShared)
            })
        else {
            throw PathTracerError.trace("no buffer of \(bytes.count) bytes")
        }
        return buffer
    }

    /// The light at each of `points` in `scene`, from `samples` paths each;
    /// with `sun` (a light's index), the probes' light: without that
    /// light's direct light, and the share of it seen as the occlusion.
    public func trace(
        _ scene: BakeScene, points: [BakePoint], samples: UInt32, seed: UInt32 = 1,
        occlusionDistance: Float, sun: UInt32? = nil
    ) throws -> [BakeLight] {
        let (light, direction) = try run(
            .irradiance, scene, points: Self.pack(points), samples: samples,
            seed: seed, occlusionDistance: occlusionDistance, sun: sun)
        return (0..<points.count).map { index in
            BakeLight(
                irradiance: SIMD3(light[index].x, light[index].y, light[index].z),
                direction: SIMD3(direction[index].x, direction[index].y, direction[index].z),
                directionality: direction[index].w, occlusion: light[index].w)
        }
    }

    /// The light arriving at each of `points` along the way it faces (its
    /// "normal"), from `samples` paths each: what is seen from there.
    public func radiance(
        _ scene: BakeScene, points: [BakePoint], samples: UInt32,
        seed: UInt32 = 1
    ) throws -> [SIMD3<Float>] {
        try run(
            .radiance, scene, points: Self.pack(points), samples: samples, seed: seed,
            occlusionDistance: 0
        ).0.map { SIMD3($0.x, $0.y, $0.z) }
    }

    /// What a kernel finds.
    public enum Kernel: Sendable {
        /// Each point's irradiance and occlusion, and its light's
        /// direction and directionality.
        case irradiance
        /// The light along each point's way (its second result is unused).
        case radiance
    }

    /// The points as the kernel reads them: 32 bytes each.
    static func pack(_ points: [BakePoint]) -> [UInt8] {
        var packed: [Float] = []
        packed.reserveCapacity(points.count * 8)
        for point in points {
            packed += [
                point.position.x, point.position.y, point.position.z, 0,
                point.normal.x, point.normal.y, point.normal.z, 0,
            ]
        }
        return packed.withUnsafeBytes { Array($0) }
    }

    /// `kernel`'s two results at each of `points` (32 bytes each, as
    /// opence_bake's point_bytes writes them) in `scene`: a float4 each.
    /// With `sun`, the irradiance kernel leaves that light out (`trace`).
    public func run(
        _ kernel: Kernel, _ scene: BakeScene, points: [UInt8], samples: UInt32, seed: UInt32 = 1,
        occlusionDistance: Float, sun: UInt32? = nil
    ) throws -> ([SIMD4<Float>], [SIMD4<Float>]) {
        let count = points.count / 32
        if count == 0 {
            return ([], [])
        }
        let nodes = try buffer(Array(scene.nodeBytes()))
        let triangles = try buffer(Array(scene.triangleBytes()))
        let materials = try buffer(Array(scene.materialBytes()))
        let lightBytes = Array(scene.lightBytes())
        let lights = try buffer(lightBytes)
        let emitterBytes = Array(scene.emitterBytes())
        let emitters = try buffer(emitterBytes)
        let sky = scene.sky()
        let pipeline = kernel == .irradiance ? pipeline : radiancePipeline
        var first = [SIMD4<Float>]()
        var second = [SIMD4<Float>]()
        first.reserveCapacity(count)
        second.reserveCapacity(count)
        let batch = max(1024, pathsPerBatch / Int(max(samples, 1)))
        var start = 0
        while start < count {
            let size = min(batch, count - start)
            // (each batch's paths its own: the seed mixed with its first point)
            var parameters = Parameters(
                sky: (sky[0], sky[1], sky[2]), lightCount: UInt32(lightBytes.count / 32),
                triangleCount: scene.triangleCount(), samples: samples,
                seed: seed &+ UInt32(truncatingIfNeeded: start) &* 0x9E37_79B9,
                occlusionDistance: occlusionDistance, pointCount: UInt32(size),
                emitterCount: UInt32(emitterBytes.count / 64),
                sun: sun.map { $0 + 1 } ?? 0)
            let pointBuffer = try buffer(Array(points[(start * 32)..<((start + size) * 32)]))
            let length = size * MemoryLayout<SIMD4<Float>>.stride
            guard let results = device.makeBuffer(length: length, options: .storageModeShared),
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
            encoder.setBuffer(results, offset: 0, index: 6)
            encoder.setBuffer(directions, offset: 0, index: 7)
            encoder.setBuffer(emitters, offset: 0, index: 8)
            let width = min(pipeline.maxTotalThreadsPerThreadgroup, 64)
            encoder.dispatchThreads(
                MTLSize(width: size, height: 1, depth: 1),
                threadsPerThreadgroup: MTLSize(width: width, height: 1, depth: 1))
            encoder.endEncoding()
            commands.commit()
            commands.waitUntilCompleted()
            if let error = commands.error {
                throw PathTracerError.trace("\(error)")
            }
            first += UnsafeBufferPointer(
                start: results.contents().bindMemory(to: SIMD4<Float>.self, capacity: size),
                count: size)
            second += UnsafeBufferPointer(
                start: directions.contents().bindMemory(to: SIMD4<Float>.self, capacity: size), count: size)
            start += size
        }
        return (first, second)
    }
}
