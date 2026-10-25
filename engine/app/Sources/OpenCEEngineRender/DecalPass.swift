// The decals (Shaders/decal.metal): the level's `decals`, laid when it is
// loaded, and those the snapshot gives each frame (where shots struck and
// grenades went off, on the level or on objects), each a box about its
// surface that projects its picture onto the surfaces of its kind inside
// it, after the opaque pass and before the screen's passes are applied (so
// that the occlusion darkens them too). Switched off from the shell with
// `render.decals`.

import Metal
import OpenCEEngineCore
import opence_ffiFFI
import simd

/// (as decal.metal's DecalFrame)
struct DecalFrameUniforms {
    var clipToWorld: simd_float4x4
    var worldToClip: simd_float4x4
    var eye: SIMD4<Float>
    var size: SIMD4<Float>
}

/// (as decal.metal's Decal, and the snapshot's OCEDecalSnapshot)
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
        modes = SIMD4(decal.blend, decal.layer, pictured ? 0 : DecalPass.noPicture, DecalPass.onLevel)
    }
}

public final class DecalPass {
    /// (the snapshot's DECAL_NO_PICTURE, DECAL_ON_LEVEL and DECAL_ON_OBJECT)
    static let noPicture = UInt32(OCE_DECAL_NO_PICTURE)
    static let onLevel = UInt32(OCE_DECAL_ON_LEVEL)
    static let onObject = UInt32(OCE_DECAL_ON_OBJECT)
    /// The frames in flight, each with its own buffer of laid decals.
    private static let inFlight = 3

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
    /// The textures the snapshot's decals' pictures number (nil: one not
    /// loaded, drawn as its colour alone).
    private let pictures: [MTLTexture?]
    /// The laid decals' buffers, one for each frame in flight.
    private var laidBuffers: [MTLBuffer] = []
    private var nextBuffer = 0
    /// The laid decals drawn in the last frame.
    public private(set) var laidCount = 0

    /// The pass for `decals`, whose `texture` numbers are made textures by
    /// `texture` (nil: none, drawn as its colour alone), and for the decals
    /// the snapshot gives, whose pictures are `pictures`.
    public init(
        device: MTLDevice, source: String, decals: [RenderDecal],
        texture: (UInt32) -> MTLTexture?, pictures: [MTLTexture?] = []
    ) throws {
        self.device = device
        self.pictures = pictures
        pipeline = try Self.pipeline(device: device, source: source)
        let samplerDescriptor = MTLSamplerDescriptor()
        samplerDescriptor.minFilter = .linear
        samplerDescriptor.magFilter = .linear
        samplerDescriptor.mipFilter = .linear
        samplerDescriptor.sAddressMode = .clampToEdge
        samplerDescriptor.tAddressMode = .clampToEdge
        let whiteDescriptor = MTLTextureDescriptor.texture2DDescriptor(
            pixelFormat: .rgba8Unorm, width: 1, height: 1,
            mipmapped: false)
        guard let sampler = device.makeSamplerState(descriptor: samplerDescriptor),
            let white = device.makeTexture(descriptor: whiteDescriptor)
        else {
            throw LevelRendererError.resources("decal sampler")
        }
        white.replace(
            region: MTLRegionMake2D(0, 0, 1, 1), mipmapLevel: 0, withBytes: [UInt8](repeating: 255, count: 4),
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
        buffer =
            gpu.isEmpty
            ? nil
            : device.makeBuffer(
                bytes: gpu, length: gpu.count * MemoryLayout<GPUDecal>.stride,
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

    /// The buffer for this frame's `laid` decals, and their runs that
    /// share a picture (none if there are none, or no buffer is made).
    private func prepare(_ laid: [OCEDecalSnapshot]) -> (MTLBuffer, [(first: Int, count: Int, picture: MTLTexture?)])? {
        let count = min(laid.count, Int(OCE_MAX_DECALS))
        guard count > 0 else { return nil }
        if laidBuffers.count < Self.inFlight {
            let length = Int(OCE_MAX_DECALS) * MemoryLayout<OCEDecalSnapshot>.stride
            guard
                let made = HitchLog.shared.note(
                    .allocation, "decals",
                    {
                        device.makeBuffer(length: length, options: .storageModeShared)
                    })
            else { return nil }
            laidBuffers.append(made)
        }
        let buffer = laidBuffers[nextBuffer % laidBuffers.count]
        nextBuffer += 1
        var runs: [(first: Int, count: Int, picture: MTLTexture?)] = []
        let decals = buffer.contents().bindMemory(to: OCEDecalSnapshot.self, capacity: count)
        for (index, decal) in laid.prefix(count).enumerated() {
            var decal = decal
            let number = decal.modes.2
            let picture = number == Self.noPicture || Int(number) >= pictures.count ? nil : pictures[Int(number)]
            if picture == nil {
                decal.modes.2 = Self.noPicture
            }
            decals[index] = decal
            if let last = runs.last, last.picture === picture {
                runs[runs.count - 1].count += 1
            } else {
                runs.append((first: index, count: 1, picture: picture))
            }
        }
        return (buffer, runs)
    }

    /// Lays the decals, the level's and the snapshot's `laid`, on `frame`,
    /// where the opaque pass left `depth` and `surface` (its normals and
    /// what each pixel is).
    public func encode(
        into commands: MTLCommandBuffer, frame: MTLTexture, depth: MTLTexture, surface: MTLTexture,
        viewProjection: simd_float4x4, eye: SIMD3<Float>, laid: [OCEDecalSnapshot] = []
    ) {
        let dynamic = prepare(laid)
        laidCount = dynamic?.1.reduce(0) { $0 + $1.count } ?? 0
        let passes = [buffer.map { ($0, runs) }, dynamic].compactMap { $0 }
        guard !passes.isEmpty else { return }
        let pass = MTLRenderPassDescriptor()
        pass.colorAttachments[0].texture = frame
        pass.colorAttachments[0].loadAction = .load
        pass.colorAttachments[0].storeAction = .store
        guard let encoder = commands.timedRenderEncoder("decals", descriptor: pass) else { return }
        var uniforms = DecalFrameUniforms(
            clipToWorld: viewProjection.inverse, worldToClip: viewProjection,
            eye: SIMD4(eye, 1),
            size: SIMD4(Float(frame.width), Float(frame.height), 0, 0))
        encoder.setRenderPipelineState(pipeline)
        encoder.setCullMode(.none)
        encoder.setVertexBytes(&uniforms, length: MemoryLayout<DecalFrameUniforms>.stride, index: 0)
        encoder.setFragmentBytes(&uniforms, length: MemoryLayout<DecalFrameUniforms>.stride, index: 0)
        encoder.setFragmentTexture(depth, index: 0)
        encoder.setFragmentTexture(surface, index: 1)
        encoder.setFragmentSamplerState(sampler, index: 0)
        for (buffer, runs) in passes {
            encoder.setVertexBuffer(buffer, offset: 0, index: 1)
            encoder.setFragmentBuffer(buffer, offset: 0, index: 1)
            for run in runs {
                var first = UInt32(run.first)
                encoder.setVertexBytes(&first, length: MemoryLayout<UInt32>.stride, index: 2)
                encoder.setFragmentTexture(run.picture ?? white, index: 2)
                encoder.drawPrimitives(type: .triangle, vertexStart: 0, vertexCount: 36, instanceCount: run.count)
            }
        }
        encoder.endEncoding()
    }
}
