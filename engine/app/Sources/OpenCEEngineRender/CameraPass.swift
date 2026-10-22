// The camera's passes (Shaders/camera.metal): the lens flares tested
// against the depth for the share of each seen, the sprites of the flares
// and of the objects' light volumes and glows (the engine's
// `cameraSprites`) added to the frame, the frame's average brightness the
// exposure adapts to, and the bloom the tone map adds. Each can be
// switched off from the shell (`render.exposure`, `render.bloom`,
// `render.lens_flares`, `render.glows`), and the brightness and the bloom
// shown alone (`render.view`).

import Metal
import OpenCEEngineCore
import simd

/// (as camera.metal's Camera)
struct CameraUniforms {
    var worldToClip: simd_float4x4
    var right: SIMD4<Float>
    var up: SIMD4<Float>
    var size: SIMD4<Float>
    /// The occluders, and the first sprite of the draw.
    var counts: SIMD4<UInt32>
}

/// (as camera.metal's Sprite)
struct GPUSprite {
    var center: SIMD4<Float>
    var extent: SIMD4<Float>
    var color: SIMD4<Float>
    /// Its occluder, its flags, and 1 for a soft disk.
    var refs: SIMD4<UInt32>
}

public final class CameraPass {
    /// The most sprites and occluders a frame draws (as camera.rs's
    /// MAX_SPRITES and MAX_OCCLUDERS).
    public static let maxSprites = 4096
    public static let maxOccluders = 512
    /// The levels of the bloom's chain, from half the frame's size.
    static let bloomLevels = 6
    /// The share of the bloom the tone map adds.
    public static let bloomStrength: Float = 0.04
    /// (as camera.rs's SPRITE_DEPTH_TESTED)
    static let depthTested: UInt32 = 1 << 1
    /// The frames in flight, each with its own buffers.
    static let slots = 3

    /// The sprites of one frame, in its slot's buffers: grouped by their
    /// picture and whether the depth hides them.
    public struct Frame {
        let slot: Int
        let occluders: Int
        let uniforms: CameraUniforms
        let groups: [(first: Int, count: Int, image: MTLTexture?, depthTested: Bool)]
        /// The sprites drawn.
        public var spriteCount: Int { groups.reduce(0) { $0 + $1.count } }
    }

    struct Pipelines {
        var occlusion: MTLComputePipelineState
        var sprite: MTLRenderPipelineState
        var luminance: MTLComputePipelineState
        var bloomDown: MTLComputePipelineState
        var bloomUp: MTLComputePipelineState
    }

    struct Bloom {
        var down: MTLTexture
        var up: MTLTexture
        var downLevels: [MTLTexture]
        var upLevels: [MTLTexture]
    }

    let device: MTLDevice
    private var pipelines: Pipelines
    /// The sprites' pictures, by `CameraSprite.image` (nil: a soft disk).
    private let images: [MTLTexture?]
    private let white: MTLTexture
    private let sprites: [MTLBuffer]
    private let occluders: [MTLBuffer]
    private let seen: [MTLBuffer]
    /// Each frame's average log2 brightness, by slot.
    private let luminance: MTLBuffer
    private var frameCount = 0
    private var luminanceCount = 0
    private var bloom: Bloom?
    private let always: MTLDepthStencilState
    private let test: MTLDepthStencilState

    public init(device: MTLDevice, source: String, images: [MTLTexture?]) throws {
        self.device = device
        pipelines = try Self.pipelines(device: device, source: source)
        self.images = images
        func buffers(_ length: Int) -> [MTLBuffer] {
            (0..<Self.slots).compactMap { _ in device.makeBuffer(length: length, options: .storageModeShared) }
        }
        sprites = buffers(Self.maxSprites * MemoryLayout<GPUSprite>.stride)
        occluders = buffers(Self.maxOccluders * MemoryLayout<SIMD4<Float>>.stride)
        seen = buffers(Self.maxOccluders * MemoryLayout<Float>.stride)
        let descriptor = MTLDepthStencilDescriptor()
        descriptor.depthCompareFunction = .always
        descriptor.isDepthWriteEnabled = false
        let tested = MTLDepthStencilDescriptor()
        tested.depthCompareFunction = .greaterEqual
        tested.isDepthWriteEnabled = false
        let whiteDescriptor = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: .rgba8Unorm, width: 1, height: 1,
                                                                       mipmapped: false)
        guard sprites.count == Self.slots, occluders.count == Self.slots, seen.count == Self.slots,
              let luminance = device.makeBuffer(length: Self.slots * MemoryLayout<Float>.stride,
                                                options: .storageModeShared),
              let always = device.makeDepthStencilState(descriptor: descriptor),
              let test = device.makeDepthStencilState(descriptor: tested),
              let white = device.makeTexture(descriptor: whiteDescriptor) else {
            throw LevelRendererError.resources("camera buffers")
        }
        white.replace(region: MTLRegionMake2D(0, 0, 1, 1), mipmapLevel: 0, withBytes: [UInt8](repeating: 255, count: 4),
                      bytesPerRow: 4)
        self.luminance = luminance
        self.always = always
        self.test = test
        self.white = white
    }

    private static func pipelines(device: MTLDevice, source: String) throws -> Pipelines {
        do {
            let library = try device.makeLibrary(source: source, options: nil)
            func compute(_ name: String) throws -> MTLComputePipelineState {
                try device.makeComputePipelineState(function: try library.required(name))
            }
            let descriptor = MTLRenderPipelineDescriptor()
            descriptor.vertexFunction = try library.required("camera_sprite_vertex")
            descriptor.fragmentFunction = try library.required("camera_sprite_fragment")
            descriptor.colorAttachments[0].pixelFormat = LevelRenderer.frameFormat
            descriptor.colorAttachments[0].isBlendingEnabled = true
            descriptor.colorAttachments[0].sourceRGBBlendFactor = .one
            descriptor.colorAttachments[0].destinationRGBBlendFactor = .one
            descriptor.colorAttachments[0].sourceAlphaBlendFactor = .zero
            descriptor.colorAttachments[0].destinationAlphaBlendFactor = .one
            descriptor.depthAttachmentPixelFormat = LevelRenderer.depthFormat
            return Pipelines(occlusion: try compute("camera_occlusion"),
                             sprite: try device.makeRenderPipelineState(descriptor: descriptor),
                             luminance: try compute("camera_luminance"), bloomDown: try compute("camera_bloom_down"),
                             bloomUp: try compute("camera_bloom_up"))
        } catch let error as LevelRendererError {
            throw error
        } catch {
            throw LevelRendererError.shaders("\(error)")
        }
    }

    /// Compiles the passes from `source` again; kept as they were if it
    /// fails.
    func reloadShaders(source: String) throws {
        pipelines = try Self.pipelines(device: device, source: source)
    }

    /// Writes `sprites` into the next slot's buffers and tests their
    /// flares' disks against `depth` (drawn from `viewProjection` by
    /// `camera`); nil if there is nothing to draw.
    public func encodeOcclusion(_ drawn: CameraSprites, into commands: MTLCommandBuffer, depth: MTLTexture,
                                viewProjection: simd_float4x4, camera: ViewCamera) -> Frame? {
        guard !drawn.sprites.isEmpty else { return nil }
        let slot = frameCount % Self.slots
        frameCount += 1
        let occluderCount = min(drawn.occluders.count, Self.maxOccluders)
        let disks = occluders[slot].contents().bindMemory(to: SIMD4<Float>.self, capacity: Self.maxOccluders)
        for (index, occluder) in drawn.occluders.prefix(occluderCount).enumerated() {
            disks[index] = SIMD4(occluder.x, occluder.y, occluder.z, occluder.radius)
        }
        // (by whether the depth hides them, then by picture)
        func image(_ sprite: CameraSprite) -> Int {
            Int(sprite.image) < images.count ? Int(sprite.image) : -1
        }
        let ordered = drawn.sprites.prefix(Self.maxSprites).sorted {
            let one = ($0.flags & Self.depthTested, image($0)), other = ($1.flags & Self.depthTested, image($1))
            return one < other
        }
        let written = sprites[slot].contents().bindMemory(to: GPUSprite.self, capacity: Self.maxSprites)
        var groups: [(first: Int, count: Int, image: MTLTexture?, depthTested: Bool)] = []
        var lastKey: (UInt32, Int)?
        for (index, sprite) in ordered.enumerated() {
            let picture = image(sprite)
            let texture = picture >= 0 ? images[picture] : nil
            let occluder = Int(sprite.occluder) < occluderCount ? sprite.occluder : UInt32.max
            written[index] = GPUSprite(center: SIMD4(sprite.x, sprite.y, sprite.z, sprite.rotation),
                                       extent: SIMD4(sprite.width, sprite.height, 0, 0),
                                       color: SIMD4(sprite.red, sprite.green, sprite.blue, 0),
                                       refs: SIMD4(occluder, sprite.flags, texture == nil ? 1 : 0, 0))
            let key = (sprite.flags & Self.depthTested, picture)
            if let lastKey, lastKey == key, let last = groups.indices.last {
                groups[last].count += 1
            } else {
                groups.append((index, 1, texture, key.0 != 0))
            }
            lastKey = key
        }
        let right = -camera.rotation.act([0, 1, 0])
        let up = camera.rotation.act([0, 0, 1])
        var uniforms = CameraUniforms(worldToClip: viewProjection, right: SIMD4(right, 0), up: SIMD4(up, 0),
                                      size: SIMD4(Float(depth.width), Float(depth.height), 0, 0),
                                      counts: SIMD4(UInt32(occluderCount), 0, 0, 0))
        if occluderCount > 0, let encoder = commands.makeComputeCommandEncoder() {
            encoder.label = "flare occlusion"
            encoder.setComputePipelineState(pipelines.occlusion)
            encoder.setBuffer(occluders[slot], offset: 0, index: 0)
            encoder.setBuffer(seen[slot], offset: 0, index: 1)
            encoder.setBytes(&uniforms, length: MemoryLayout<CameraUniforms>.stride, index: 2)
            encoder.setTexture(depth, index: 0)
            encoder.dispatchThreadgroups(MTLSize(width: (occluderCount + 63) / 64, height: 1, depth: 1),
                                         threadsPerThreadgroup: MTLSize(width: 64, height: 1, depth: 1))
            encoder.endEncoding()
        }
        return Frame(slot: slot, occluders: occluderCount, uniforms: uniforms, groups: groups)
    }

    /// The share of each of `frame`'s flares' disks seen (once its
    /// commands have run).
    public func seen(_ frame: Frame) -> [Float] {
        let values = seen[frame.slot].contents().bindMemory(to: Float.self, capacity: Self.maxOccluders)
        return (0..<frame.occluders).map { values[$0] }
    }

    /// Adds `frame`'s sprites to what `encoder` draws (the blended pass).
    public func encodeSprites(_ frame: Frame, into encoder: MTLRenderCommandEncoder) {
        encoder.setRenderPipelineState(pipelines.sprite)
        encoder.setCullMode(.none)
        encoder.setVertexBuffer(sprites[frame.slot], offset: 0, index: 20)
        encoder.setVertexBuffer(seen[frame.slot], offset: 0, index: 21)
        for group in frame.groups {
            var uniforms = frame.uniforms
            uniforms.counts.y = UInt32(group.first)
            encoder.setVertexBytes(&uniforms, length: MemoryLayout<CameraUniforms>.stride, index: 22)
            encoder.setFragmentTexture(group.image ?? white, index: 30)
            encoder.setDepthStencilState(group.depthTested ? test : always)
            encoder.drawPrimitives(type: .triangle, vertexStart: 0, vertexCount: 6, instanceCount: group.count)
        }
    }

    /// Averages the log2 brightness of `frame` into the next slot.
    public func encodeLuminance(into commands: MTLCommandBuffer, frame: MTLTexture) {
        guard let encoder = commands.makeComputeCommandEncoder() else { return }
        var slot = UInt32(luminanceCount % Self.slots)
        luminanceCount += 1
        encoder.label = "luminance"
        encoder.setComputePipelineState(pipelines.luminance)
        encoder.setTexture(frame, index: 0)
        encoder.setBuffer(luminance, offset: 0, index: 0)
        encoder.setBytes(&slot, length: MemoryLayout<UInt32>.stride, index: 1)
        encoder.dispatchThreadgroups(MTLSize(width: 1, height: 1, depth: 1),
                                     threadsPerThreadgroup: MTLSize(width: 16, height: 16, depth: 1))
        encoder.endEncoding()
    }

    /// The average log2 brightness of the frame `age` frames before the
    /// last one averaged (0: the last), if there was one.
    public func logLuminance(age: Int) -> Float? {
        guard age >= 0, age < Self.slots, luminanceCount > age else { return nil }
        let slot = (luminanceCount - 1 - age) % Self.slots
        return luminance.contents().bindMemory(to: Float.self, capacity: Self.slots)[slot]
    }

    /// The bloom's chains for a frame `width` by `height`, made again when
    /// the size changes.
    private func bloomTargets(width: Int, height: Int) -> Bloom? {
        let half = (max(width / 2, 1), max(height / 2, 1))
        if let bloom, bloom.down.width == half.0, bloom.down.height == half.1 {
            return bloom
        }
        let descriptor = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: .rgba16Float, width: half.0,
                                                                  height: half.1, mipmapped: true)
        descriptor.mipmapLevelCount = min(Self.bloomLevels, descriptor.mipmapLevelCount)
        descriptor.usage = [.shaderRead, .shaderWrite]
        descriptor.storageMode = .private
        guard let down = device.makeTexture(descriptor: descriptor),
              let up = device.makeTexture(descriptor: descriptor) else { return nil }
        func levels(_ texture: MTLTexture) -> [MTLTexture] {
            (0..<texture.mipmapLevelCount).compactMap {
                texture.makeTextureView(pixelFormat: .rgba16Float, textureType: .type2D, levels: $0..<($0 + 1),
                                        slices: 0..<1)
            }
        }
        bloom = Bloom(down: down, up: up, downLevels: levels(down), upLevels: levels(up))
        return bloom
    }

    /// The bloom of `frame`: what is brighter than white, spread (at half
    /// its size); nil if it could not be made.
    public func encodeBloom(into commands: MTLCommandBuffer, frame: MTLTexture) -> MTLTexture? {
        guard let bloom = bloomTargets(width: frame.width, height: frame.height),
              bloom.downLevels.count == bloom.upLevels.count, !bloom.downLevels.isEmpty,
              let encoder = commands.makeComputeCommandEncoder() else { return nil }
        encoder.label = "bloom"
        let threads = MTLSize(width: 8, height: 8, depth: 1)
        func groups(_ texture: MTLTexture) -> MTLSize {
            MTLSize(width: (texture.width + 7) / 8, height: (texture.height + 7) / 8, depth: 1)
        }
        encoder.setComputePipelineState(pipelines.bloomDown)
        for (index, level) in bloom.downLevels.enumerated() {
            var first: UInt32 = index == 0 ? 1 : 0
            encoder.setTexture(index == 0 ? frame : bloom.downLevels[index - 1], index: 0)
            encoder.setTexture(level, index: 1)
            encoder.setBytes(&first, length: MemoryLayout<UInt32>.stride, index: 0)
            encoder.dispatchThreadgroups(groups(level), threadsPerThreadgroup: threads)
        }
        // (the last level's is its own; each above, its own and the one below's)
        let last = bloom.downLevels.count - 1
        encoder.setComputePipelineState(pipelines.bloomUp)
        for index in stride(from: last, through: 0, by: -1) {
            encoder.setTexture(bloom.downLevels[index], index: 0)
            encoder.setTexture(index == last ? blackLevel() : bloom.upLevels[index + 1], index: 1)
            encoder.setTexture(bloom.upLevels[index], index: 2)
            encoder.dispatchThreadgroups(groups(bloom.upLevels[index]), threadsPerThreadgroup: threads)
        }
        encoder.endEncoding()
        return bloom.upLevels[0]
    }

    private var black: MTLTexture?

    /// A black texture, for the last level of the chain to widen (and
    /// the tone map's bloom when there is none).
    func blackLevel() -> MTLTexture? {
        if let black { return black }
        let descriptor = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: .rgba16Float, width: 1, height: 1,
                                                                  mipmapped: false)
        black = device.makeTexture(descriptor: descriptor)
        black?.replace(region: MTLRegionMake2D(0, 0, 1, 1), mipmapLevel: 0, withBytes: [UInt16](repeating: 0, count: 4),
                       bytesPerRow: 8)
        return black
    }
}

extension ViewCamera {
    /// The camera as the engine's sprites are faced to it.
    public var spriteCamera: SpriteCamera {
        let forward = rotation.act(SIMD3<Float>(1, 0, 0))
        let up = rotation.act(SIMD3<Float>(0, 0, 1))
        return SpriteCamera(position: [position.x, position.y, position.z],
                            forward: [forward.x, forward.y, forward.z], up: [up.x, up.y, up.z])
    }
}
