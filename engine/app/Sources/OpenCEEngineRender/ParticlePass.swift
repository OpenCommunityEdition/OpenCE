// The particles, contrails, weather and detail objects (particle.metal):
// the frame's sprites (the snapshot's) and the level's detail objects
// (built once), drawn after the level's blended surfaces, tested against
// the depth but not writing it, in runs that share a picture. Switched by
// `render.particles`. Lit rows take the baked probes and the sun as the
// level's surfaces do (`ParticleLighting`).
import Foundation
import Metal
import opence_ffiFFI
import simd

/// The light lit particles take: the level's baked probes and its sun.
public struct ParticleLighting {
    /// The probes' light from each axis (+x, -x, +y, -y, +z, -z).
    public var probes: [MTLTexture]
    /// As `FrameUniforms`' fields of the same names.
    public var probeOrigin: SIMD4<Float>
    public var probeCounts: SIMD4<Float>
    public var sunTowards: SIMD4<Float>
    public var sunColor: SIMD4<Float>

    public init(probes: [MTLTexture], probeOrigin: SIMD4<Float>, probeCounts: SIMD4<Float>,
                sunTowards: SIMD4<Float>, sunColor: SIMD4<Float>) {
        self.probes = probes
        self.probeOrigin = probeOrigin
        self.probeCounts = probeCounts
        self.sunTowards = sunTowards
        self.sunColor = sunColor
    }
}

public final class ParticlePass {
    private struct Frame {
        var worldToClip: simd_float4x4
        var eye: SIMD4<Float>
        var right: SIMD4<Float>
        var up: SIMD4<Float>
        var probeOrigin: SIMD4<Float> = .zero
        var probeCounts: SIMD4<Float> = .zero
        var sunTowards: SIMD4<Float> = .zero
        var sunColor: SIMD4<Float> = .zero
    }

    private static let framesInFlight = 3
    private let device: MTLDevice
    private var pipeline: MTLRenderPipelineState
    private let depthState: MTLDepthStencilState
    private let sampler: MTLSamplerState
    private let white: MTLTexture
    /// (bound for the probes when there are none)
    private let whiteVolume: MTLTexture
    private let pictures: [MTLTexture?]
    private let details: [OCEParticleSnapshot]
    private var buffers: [MTLBuffer]
    private var slot = 0

    public init(device: MTLDevice, source: String, pictures: [MTLTexture?], details: [OCEParticleSnapshot]) throws {
        self.device = device
        self.pictures = pictures
        self.details = details
        pipeline = try Self.pipeline(device: device, source: source)
        let depth = MTLDepthStencilDescriptor()
        depth.depthCompareFunction = .greaterEqual
        depth.isDepthWriteEnabled = false
        let samplerDescriptor = MTLSamplerDescriptor()
        samplerDescriptor.minFilter = .linear
        samplerDescriptor.magFilter = .linear
        samplerDescriptor.mipFilter = .linear
        samplerDescriptor.sAddressMode = .repeat
        samplerDescriptor.tAddressMode = .repeat
        let whiteDescriptor = MTLTextureDescriptor.texture2DDescriptor(
            pixelFormat: .rgba8Unorm, width: 1, height: 1, mipmapped: false)
        let volumeDescriptor = MTLTextureDescriptor()
        volumeDescriptor.textureType = .type3D
        volumeDescriptor.pixelFormat = .rgba8Unorm
        volumeDescriptor.width = 1
        volumeDescriptor.height = 1
        volumeDescriptor.depth = 1
        guard let depthState = device.makeDepthStencilState(descriptor: depth),
              let sampler = device.makeSamplerState(descriptor: samplerDescriptor),
              let white = device.makeTexture(descriptor: whiteDescriptor),
              let whiteVolume = device.makeTexture(descriptor: volumeDescriptor)
        else { throw LevelRendererError.resources("particle states") }
        var texel: UInt32 = 0xFFFF_FFFF
        white.replace(region: MTLRegionMake2D(0, 0, 1, 1), mipmapLevel: 0, withBytes: &texel, bytesPerRow: 4)
        whiteVolume.replace(region: MTLRegionMake3D(0, 0, 0, 1, 1, 1), mipmapLevel: 0, slice: 0, withBytes: &texel,
                            bytesPerRow: 4, bytesPerImage: 4)
        self.depthState = depthState
        self.sampler = sampler
        self.white = white
        self.whiteVolume = whiteVolume
        buffers = []
    }

    private static func pipeline(device: MTLDevice, source: String) throws -> MTLRenderPipelineState {
        do {
            let library = try device.makeLibrary(source: source, options: nil)
            let descriptor = MTLRenderPipelineDescriptor()
            descriptor.vertexFunction = try library.required("particle_vertex")
            descriptor.fragmentFunction = try library.required("particle_fragment")
            descriptor.colorAttachments[0].pixelFormat = LevelRenderer.frameFormat
            descriptor.depthAttachmentPixelFormat = LevelRenderer.depthFormat
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

    /// The runs of `rows` that share a picture: their first row, their
    /// count and their picture's index (or `OCE_PARTICLE_NO_PICTURE`).
    public static func runs(_ rows: [OCEParticleSnapshot]) -> [(first: Int, count: Int, picture: UInt32)] {
        var out: [(first: Int, count: Int, picture: UInt32)] = []
        for (index, row) in rows.enumerated() {
            if let last = out.last, last.picture == row.modes.0, last.first + last.count == index {
                out[out.count - 1].count += 1
            } else {
                out.append((index, 1, row.modes.0))
            }
        }
        return out
    }

    /// Draws `sprites` and the level's detail objects into `frame`, the lit
    /// ones by `lighting` (white without it).
    public func encode(into commands: MTLCommandBuffer, frame: MTLTexture, depth: MTLTexture,
                       worldToClip: simd_float4x4, eye: SIMD3<Float>, right: SIMD3<Float>, up: SIMD3<Float>,
                       sprites: [OCEParticleSnapshot], lighting: ParticleLighting? = nil) {
        let rows = details + sprites
        guard !rows.isEmpty else { return }
        let size = rows.count * MemoryLayout<OCEParticleSnapshot>.stride
        if buffers.count < Self.framesInFlight {
            guard let buffer = device.makeBuffer(length: max(size, 4096), options: .storageModeShared) else { return }
            buffers.append(buffer)
            slot = buffers.count - 1
        } else {
            slot = (slot + 1) % Self.framesInFlight
            if buffers[slot].length < size {
                guard let buffer = device.makeBuffer(length: size * 2, options: .storageModeShared) else { return }
                buffers[slot] = buffer
            }
        }
        let buffer = buffers[slot]
        rows.withUnsafeBytes { buffer.contents().copyMemory(from: $0.baseAddress!, byteCount: size) }
        var uniforms = Frame(worldToClip: worldToClip, eye: SIMD4(eye, 1), right: SIMD4(right, 0), up: SIMD4(up, 0))
        var probes = Array(repeating: whiteVolume, count: 6)
        if let lighting, lighting.probes.count == 6 {
            uniforms.probeOrigin = lighting.probeOrigin
            uniforms.probeCounts = lighting.probeCounts
            uniforms.sunTowards = lighting.sunTowards
            uniforms.sunColor = lighting.sunColor
            probes = lighting.probes
        }
        let pass = MTLRenderPassDescriptor()
        pass.colorAttachments[0].texture = frame
        pass.colorAttachments[0].loadAction = .load
        pass.colorAttachments[0].storeAction = .store
        pass.depthAttachment.texture = depth
        pass.depthAttachment.loadAction = .load
        pass.depthAttachment.storeAction = .store
        guard let encoder = commands.makeRenderCommandEncoder(descriptor: pass) else { return }
        encoder.label = "Particles"
        encoder.setRenderPipelineState(pipeline)
        encoder.setDepthStencilState(depthState)
        encoder.setVertexBytes(&uniforms, length: MemoryLayout<Frame>.stride, index: 0)
        encoder.setVertexBuffer(buffer, offset: 0, index: 1)
        encoder.setVertexTextures(probes, range: 0..<6)
        encoder.setFragmentSamplerState(sampler, index: 0)
        for run in Self.runs(rows) {
            var first = UInt32(run.first)
            let picture = Int(run.picture) < pictures.count ? pictures[Int(run.picture)] ?? white : white
            encoder.setVertexBytes(&first, length: 4, index: 2)
            encoder.setFragmentTexture(picture, index: 0)
            encoder.drawPrimitives(type: .triangle, vertexStart: 0, vertexCount: 6, instanceCount: run.count)
        }
        encoder.endEncoding()
    }
}
