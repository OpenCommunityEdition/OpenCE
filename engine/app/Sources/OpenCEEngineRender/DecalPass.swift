// The decals laid on the level (Shaders/decal.metal): the engine's
// `decals`, each a box about its surface that projects its picture onto
// the level surfaces inside it, after the opaque pass and before the
// screen's passes are applied (so that the occlusion darkens them too).
// Switched off from the shell with `render.decals`.

import Metal
import OpenCEEngineCore
import simd

/// (as decal.metal's DecalFrame)
struct DecalFrameUniforms {
    var clipToWorld: simd_float4x4
    var worldToClip: simd_float4x4
    var eye: SIMD4<Float>
    var size: SIMD4<Float>
}

/// (as decal.metal's Decal)
struct GPUDecal {
    var center: SIMD4<Float>
    var forward: SIMD4<Float>
    var left: SIMD4<Float>
    var up: SIMD4<Float>
    var reach: SIMD4<Float>
    var uv: SIMD4<Float>
    var color: SIMD4<Float>
    var modes: SIMD4<UInt32>

    init(_ decal: RenderDecal, pictured: Bool) {
        center = SIMD4(decal.x, decal.y, decal.z, decal.depth)
        forward = SIMD4(decal.forwardX, decal.forwardY, decal.forwardZ, decal.forwardLeast)
        left = SIMD4(decal.leftX, decal.leftY, decal.leftZ, decal.leftLeast)
        up = SIMD4(decal.upX, decal.upY, decal.upZ, 0)
        reach = SIMD4(decal.forwardGreatest, decal.leftGreatest, 0, 0)
        uv = SIMD4(decal.u0, decal.v0, decal.u1, decal.v1)
        color = SIMD4(decal.red, decal.green, decal.blue, decal.alpha)
        modes = SIMD4(decal.blend, decal.layer, pictured ? 1 : 0, 0)
    }
}

public final class DecalPass {
    let device: MTLDevice
    private var pipeline: MTLRenderPipelineState
    private let sampler: MTLSamplerState
    private let white: MTLTexture
    private let buffer: MTLBuffer?
    /// The decals' runs that share a picture, in their order (layer by
    /// layer).
    let runs: [(first: Int, count: Int, picture: MTLTexture?)]
    /// The decals laid.
    public var count: Int { runs.reduce(0) { $0 + $1.count } }

    /// The pass for `decals`, whose `texture` numbers are made textures by
    /// `texture` (nil: none, drawn as its colour alone).
    public init(device: MTLDevice, source: String, decals: [RenderDecal],
                texture: (UInt32) -> MTLTexture?) throws {
        self.device = device
        pipeline = try Self.pipeline(device: device, source: source)
        let samplerDescriptor = MTLSamplerDescriptor()
        samplerDescriptor.minFilter = .linear
        samplerDescriptor.magFilter = .linear
        samplerDescriptor.mipFilter = .linear
        samplerDescriptor.sAddressMode = .clampToEdge
        samplerDescriptor.tAddressMode = .clampToEdge
        let whiteDescriptor = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: .rgba8Unorm, width: 1, height: 1,
                                                                       mipmapped: false)
        guard let sampler = device.makeSamplerState(descriptor: samplerDescriptor),
              let white = device.makeTexture(descriptor: whiteDescriptor) else {
            throw LevelRendererError.resources("decal sampler")
        }
        white.replace(region: MTLRegionMake2D(0, 0, 1, 1), mipmapLevel: 0, withBytes: [UInt8](repeating: 255, count: 4),
                      bytesPerRow: 4)
        self.sampler = sampler
        self.white = white
        var gpu: [GPUDecal] = []
        var runs: [(first: Int, count: Int, picture: MTLTexture?)] = []
        for decal in decals {
            let picture = decal.texture == UInt32.max ? nil : texture(decal.texture)
            gpu.append(GPUDecal(decal, pictured: picture != nil))
            if let last = runs.last, last.picture === picture {
                runs[runs.count - 1].count += 1
            } else {
                runs.append((first: gpu.count - 1, count: 1, picture: picture))
            }
        }
        self.runs = runs
        buffer = gpu.isEmpty ? nil
            : device.makeBuffer(bytes: gpu, length: gpu.count * MemoryLayout<GPUDecal>.stride,
                                options: .storageModeShared)
        if !gpu.isEmpty && buffer == nil {
            throw LevelRendererError.resources("decal buffer")
        }
    }

    private static func pipeline(device: MTLDevice, source: String) throws -> MTLRenderPipelineState {
        do {
            let library = try device.makeLibrary(source: source, options: nil)
            let descriptor = MTLRenderPipelineDescriptor()
            descriptor.vertexFunction = try library.required("decal_vertex")
            descriptor.fragmentFunction = try library.required("decal_fragment")
            descriptor.colorAttachments[0].pixelFormat = LevelRenderer.frameFormat
            return try device.makeRenderPipelineState(descriptor: descriptor)
        } catch let error as LevelRendererError {
            throw error
        } catch {
            throw LevelRendererError.shaders("\(error)")
        }
    }

    /// Compiles the pass from `source` again; kept as it was if it fails.
    func reloadShaders(source: String) throws {
        pipeline = try Self.pipeline(device: device, source: source)
    }

    /// Lays the decals on `frame`, where the opaque pass left `depth` and
    /// `surface` (its normals and what each pixel is).
    public func encode(into commands: MTLCommandBuffer, frame: MTLTexture, depth: MTLTexture, surface: MTLTexture,
                viewProjection: simd_float4x4, eye: SIMD3<Float>) {
        guard let buffer, !runs.isEmpty else { return }
        let pass = MTLRenderPassDescriptor()
        pass.colorAttachments[0].texture = frame
        pass.colorAttachments[0].loadAction = .load
        pass.colorAttachments[0].storeAction = .store
        guard let encoder = commands.makeRenderCommandEncoder(descriptor: pass) else { return }
        encoder.label = "decals"
        var uniforms = DecalFrameUniforms(clipToWorld: viewProjection.inverse, worldToClip: viewProjection,
                                          eye: SIMD4(eye, 1),
                                          size: SIMD4(Float(frame.width), Float(frame.height), 0, 0))
        encoder.setRenderPipelineState(pipeline)
        encoder.setCullMode(.none)
        encoder.setVertexBytes(&uniforms, length: MemoryLayout<DecalFrameUniforms>.stride, index: 0)
        encoder.setVertexBuffer(buffer, offset: 0, index: 1)
        encoder.setFragmentBytes(&uniforms, length: MemoryLayout<DecalFrameUniforms>.stride, index: 0)
        encoder.setFragmentBuffer(buffer, offset: 0, index: 1)
        encoder.setFragmentTexture(depth, index: 0)
        encoder.setFragmentTexture(surface, index: 1)
        encoder.setFragmentSamplerState(sampler, index: 0)
        for run in runs {
            var first = UInt32(run.first)
            encoder.setVertexBytes(&first, length: MemoryLayout<UInt32>.stride, index: 2)
            encoder.setFragmentTexture(run.picture ?? white, index: 2)
            encoder.drawPrimitives(type: .triangle, vertexStart: 0, vertexCount: 36, instanceCount: run.count)
        }
        encoder.endEncoding()
    }
}
