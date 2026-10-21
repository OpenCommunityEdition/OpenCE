// Draws a level's structure BSP: its buffers, textures, samplers and
// pipelines made once from the engine's LevelGeometry, then each frame the
// level into a half-float target, and the tone map from it into the target
// given (the drawable, or a golden frame's texture). With temporal
// anti-aliasing the level is drawn jittered at the render scale, its motion
// written from its depth, and MetalFX gathers the frames before the tone
// map (TemporalScaling.swift).

import Metal
import OpenCEEngineCore
import simd

public enum LevelRendererError: Error, CustomStringConvertible {
    case shaders(String)
    case resources(String)

    public var description: String {
        switch self {
        case .shaders(let message): "the level's shaders: \(message)"
        case .resources(let message): "the level's \(message) could not be made"
        }
    }
}

/// A material's parameters, as the fragment shader's Material.
struct MaterialUniforms {
    var baseScale: SIMD2<Float>
    var baseOffset: SIMD2<Float>
    var primaryScale: Float
    var secondaryScale: Float
    var maps: UInt32
    var detailFunction: UInt32
    var environmentKind: UInt32
    var alphaTested: UInt32
}

/// The map bits of MaterialUniforms.maps.
enum MapBit {
    static let base: UInt32 = 1
    static let primary: UInt32 = 2
    static let secondary: UInt32 = 4
    static let lightmap: UInt32 = 8
}

public final class LevelRenderer {
    /// What the sky's portals show until the sky is drawn.
    public static let skyColor = MTLClearColor(red: 0.42, green: 0.52, blue: 0.66, alpha: 1)

    struct Draw {
        var firstIndex: Int
        var indexCount: Int
        var blend: BlendFunction?
        var uniforms: MaterialUniforms
        /// Base, primary detail, secondary detail and lightmap.
        var textures: [MTLTexture]
        var center: SIMD3<Float>
    }

    public let device: MTLDevice
    public let outputFormat: MTLPixelFormat
    let vertices: MTLBuffer
    let indices: MTLBuffer
    let opaqueDraws: [Draw]
    let blendedDraws: [Draw]
    let textureCount: Int
    private let opaque: MTLRenderPipelineState
    private let blended: [MTLRenderPipelineState]
    private let tone: MTLRenderPipelineState
    private let motion: MTLRenderPipelineState
    private let depthWrite: MTLDepthStencilState
    private let depthTest: MTLDepthStencilState
    private let wrap: MTLSamplerState
    private let clamp: MTLSamplerState
    private var targets: (frame: MTLTexture, depth: MTLTexture)?
    private(set) var temporal: TemporalScaling?

    /// How the frame is smoothed and the share of it drawn. A change
    /// starts the temporal history again.
    public var settings: VideoSettings {
        didSet {
            if settings != oldValue { temporal = nil }
        }
    }

    /// Whether this device has MetalFX's temporal scaler (without it,
    /// temporal anti-aliasing draws as native).
    public var supportsTemporal: Bool { TemporalScaling.isSupported(device) }

    /// The size the level was last drawn at, before any upscaling.
    public private(set) var lastInputSize: (width: Int, height: Int) = (0, 0)

    public static let frameFormat = MTLPixelFormat.rgba16Float
    public static let depthFormat = MTLPixelFormat.depth32Float

    /// The draws made (opaque and blended), for tests and the app's log.
    public var drawCount: Int { opaqueDraws.count + blendedDraws.count }

    public init(device: MTLDevice, geometry: LevelGeometry, outputFormat: MTLPixelFormat = .bgra8Unorm,
                settings: VideoSettings = VideoSettings()) throws {
        self.device = device
        self.outputFormat = outputFormat
        self.settings = settings
        let vertexBytes = geometry.vertices()
        let indexBytes = geometry.indices()
        guard let vertices = Self.buffer(device, vertexBytes), let indices = Self.buffer(device, indexBytes) else {
            throw LevelRendererError.resources("buffers")
        }
        self.vertices = vertices
        self.indices = indices

        let textures = geometry.textures().enumerated().map { index, texture in
            Self.texture(device, texture) { level in geometry.textureLevel(texture: UInt32(index), level: level) }
        }
        textureCount = textures.compactMap { $0 }.count
        guard let white = Self.white(device) else { throw LevelRendererError.resources("textures") }
        let materials = geometry.materials()
        func texture(_ index: UInt32?) -> MTLTexture? {
            index.flatMap { Int($0) < textures.count ? textures[Int($0)] : nil }
        }
        var opaqueDraws: [Draw] = []
        var blendedDraws: [Draw] = []
        let indexCount = indexBytes.count / 4
        for draw in geometry.draws() {
            guard Int(draw.material) < materials.count,
                  Int(draw.firstIndex) + Int(draw.indexCount) <= indexCount else { continue }
            let material = materials[Int(draw.material)]
            let maps = [texture(material.baseMap), texture(material.primaryDetailMap),
                        texture(material.secondaryDetailMap), texture(draw.lightmap)]
            var bits: UInt32 = 0
            for (bit, map) in zip([MapBit.base, MapBit.primary, MapBit.secondary, MapBit.lightmap], maps)
            where map != nil {
                bits |= bit
            }
            let pair = { (values: [Float], fallback: Float) -> SIMD2<Float> in
                values.count == 2 ? SIMD2(values[0], values[1]) : SIMD2(repeating: fallback)
            }
            let uniforms = MaterialUniforms(
                baseScale: pair(material.baseScale, 1), baseOffset: pair(material.baseOffset, 0),
                primaryScale: material.primaryDetailScale, secondaryScale: material.secondaryDetailScale,
                maps: bits, detailFunction: UInt32(material.detailFunction),
                environmentKind: UInt32(material.environmentKind),
                alphaTested: material.kind == .alphaTested ? 1 : 0)
            let center = draw.center.count == 3 ? SIMD3(draw.center[0], draw.center[1], draw.center[2]) : .zero
            let made = Draw(firstIndex: Int(draw.firstIndex), indexCount: Int(draw.indexCount),
                            blend: material.kind == .blended ? material.blend : nil, uniforms: uniforms,
                            textures: maps.map { $0 ?? white }, center: center)
            if made.blend == nil {
                opaqueDraws.append(made)
            } else {
                blendedDraws.append(made)
            }
        }
        self.opaqueDraws = opaqueDraws
        self.blendedDraws = blendedDraws

        let library: MTLLibrary
        do {
            library = try device.makeLibrary(source: levelShaderSource, options: nil)
        } catch {
            throw LevelRendererError.shaders("\(error)")
        }
        let stride = Int(geometry.vertexStride())
        func pipeline(_ blend: BlendFunction?) throws -> MTLRenderPipelineState {
            let descriptor = MTLRenderPipelineDescriptor()
            descriptor.vertexFunction = library.makeFunction(name: "level_vertex")
            descriptor.fragmentFunction = library.makeFunction(name: "level_fragment")
            descriptor.vertexDescriptor = Self.vertexDescriptor(stride: stride)
            descriptor.colorAttachments[0].pixelFormat = Self.frameFormat
            descriptor.depthAttachmentPixelFormat = Self.depthFormat
            if let blend {
                let color = descriptor.colorAttachments[0]!
                color.isBlendingEnabled = true
                color.rgbBlendOperation = .add
                color.alphaBlendOperation = .add
                // (the frame's alpha is not shown: kept at one)
                color.sourceAlphaBlendFactor = .zero
                color.destinationAlphaBlendFactor = .one
                switch blend {
                case .alphaBlend:
                    color.sourceRGBBlendFactor = .sourceAlpha
                    color.destinationRGBBlendFactor = .oneMinusSourceAlpha
                case .multiply:
                    color.sourceRGBBlendFactor = .destinationColor
                    color.destinationRGBBlendFactor = .zero
                case .doubleMultiply:
                    color.sourceRGBBlendFactor = .destinationColor
                    color.destinationRGBBlendFactor = .sourceColor
                case .add:
                    color.sourceRGBBlendFactor = .one
                    color.destinationRGBBlendFactor = .one
                }
            }
            return try device.makeRenderPipelineState(descriptor: descriptor)
        }
        do {
            opaque = try pipeline(nil)
            blended = try Self.blendFunctions.map { try pipeline($0) }
            let toneDescriptor = MTLRenderPipelineDescriptor()
            toneDescriptor.vertexFunction = library.makeFunction(name: "tone_vertex")
            toneDescriptor.fragmentFunction = library.makeFunction(name: "tone_fragment")
            toneDescriptor.colorAttachments[0].pixelFormat = outputFormat
            tone = try device.makeRenderPipelineState(descriptor: toneDescriptor)
            let motionDescriptor = MTLRenderPipelineDescriptor()
            motionDescriptor.vertexFunction = library.makeFunction(name: "tone_vertex")
            motionDescriptor.fragmentFunction = library.makeFunction(name: "motion_fragment")
            motionDescriptor.colorAttachments[0].pixelFormat = TemporalScaling.motionFormat
            motion = try device.makeRenderPipelineState(descriptor: motionDescriptor)
        } catch {
            throw LevelRendererError.shaders("\(error)")
        }

        // reversed depth: nearer is greater
        let write = MTLDepthStencilDescriptor()
        write.depthCompareFunction = .greater
        write.isDepthWriteEnabled = true
        let test = MTLDepthStencilDescriptor()
        test.depthCompareFunction = .greater
        test.isDepthWriteEnabled = false
        let wrapDescriptor = MTLSamplerDescriptor()
        wrapDescriptor.minFilter = .linear
        wrapDescriptor.magFilter = .linear
        wrapDescriptor.mipFilter = .linear
        wrapDescriptor.maxAnisotropy = 8
        wrapDescriptor.sAddressMode = .repeat
        wrapDescriptor.tAddressMode = .repeat
        let clampDescriptor = MTLSamplerDescriptor()
        clampDescriptor.minFilter = .linear
        clampDescriptor.magFilter = .linear
        clampDescriptor.sAddressMode = .clampToEdge
        clampDescriptor.tAddressMode = .clampToEdge
        guard let depthWrite = device.makeDepthStencilState(descriptor: write),
              let depthTest = device.makeDepthStencilState(descriptor: test),
              let wrap = device.makeSamplerState(descriptor: wrapDescriptor),
              let clamp = device.makeSamplerState(descriptor: clampDescriptor) else {
            throw LevelRendererError.resources("depth and sampler states")
        }
        self.depthWrite = depthWrite
        self.depthTest = depthTest
        self.wrap = wrap
        self.clamp = clamp
    }

    static let blendFunctions: [BlendFunction] = [.alphaBlend, .multiply, .doubleMultiply, .add]

    /// Draws the level from `camera` into `target` (of `outputFormat`).
    public func encode(into commands: MTLCommandBuffer, target: MTLTexture, camera: ViewCamera) {
        let aspect = Float(target.width) / Float(max(target.height, 1))
        let viewProjection = camera.viewProjection(aspect: aspect)
        let shown: MTLTexture
        if settings.antiAliasing == .temporal, let temporal = temporalScaling(width: target.width,
                                                                               height: target.height) {
            let jitter = Jitter.offset(frame: temporal.frame)
            let jittered = Jitter.apply(jitter, to: viewProjection, width: temporal.inputWidth,
                                        height: temporal.inputHeight)
            drawLevel(into: commands, frame: temporal.color, depth: temporal.depth, keepDepth: true,
                      viewProjection: jittered, camera: camera)
            var uniforms = temporal.motionUniforms(current: viewProjection)
            let motionPass = MTLRenderPassDescriptor()
            motionPass.colorAttachments[0].texture = temporal.motion
            motionPass.colorAttachments[0].loadAction = .dontCare
            motionPass.colorAttachments[0].storeAction = .store
            guard let encoder = commands.makeRenderCommandEncoder(descriptor: motionPass) else { return }
            encoder.label = "motion"
            encoder.setRenderPipelineState(motion)
            encoder.setFragmentTexture(temporal.depth, index: 0)
            encoder.setFragmentBytes(&uniforms, length: MemoryLayout<MotionUniforms>.stride, index: 0)
            encoder.drawPrimitives(type: .triangle, vertexStart: 0, vertexCount: 3)
            encoder.endEncoding()
            temporal.encode(into: commands, jitter: jitter)
            lastInputSize = (temporal.inputWidth, temporal.inputHeight)
            shown = temporal.output
        } else {
            guard let (frame, depth) = targets(width: target.width, height: target.height) else { return }
            drawLevel(into: commands, frame: frame, depth: depth, keepDepth: false, viewProjection: viewProjection,
                      camera: camera)
            lastInputSize = (target.width, target.height)
            shown = frame
        }

        let tonePass = MTLRenderPassDescriptor()
        tonePass.colorAttachments[0].texture = target
        tonePass.colorAttachments[0].loadAction = .dontCare
        tonePass.colorAttachments[0].storeAction = .store
        guard let toneEncoder = commands.makeRenderCommandEncoder(descriptor: tonePass) else { return }
        toneEncoder.label = "tone map"
        toneEncoder.setRenderPipelineState(tone)
        toneEncoder.setFragmentTexture(shown, index: 0)
        toneEncoder.drawPrimitives(type: .triangle, vertexStart: 0, vertexCount: 3)
        toneEncoder.endEncoding()
    }

    /// The level's opaque then blended draws into `frame`.
    private func drawLevel(into commands: MTLCommandBuffer, frame: MTLTexture, depth: MTLTexture, keepDepth: Bool,
                           viewProjection: simd_float4x4, camera: ViewCamera) {
        var uniforms = viewProjection
        let pass = MTLRenderPassDescriptor()
        pass.colorAttachments[0].texture = frame
        pass.colorAttachments[0].loadAction = .clear
        pass.colorAttachments[0].storeAction = .store
        pass.colorAttachments[0].clearColor = Self.skyColor
        pass.depthAttachment.texture = depth
        pass.depthAttachment.loadAction = .clear
        pass.depthAttachment.storeAction = keepDepth ? .store : .dontCare
        pass.depthAttachment.clearDepth = 0
        guard let encoder = commands.makeRenderCommandEncoder(descriptor: pass) else { return }
        encoder.label = "level"
        encoder.setCullMode(.none)
        encoder.setVertexBuffer(vertices, offset: 0, index: 0)
        encoder.setVertexBytes(&uniforms, length: MemoryLayout<simd_float4x4>.size, index: 1)
        encoder.setFragmentSamplerState(wrap, index: 0)
        encoder.setFragmentSamplerState(clamp, index: 1)
        encoder.setRenderPipelineState(opaque)
        encoder.setDepthStencilState(depthWrite)
        for draw in opaqueDraws {
            encode(draw, encoder)
        }
        encoder.setDepthStencilState(depthTest)
        // far to near
        let far = blendedDraws.sorted {
            distance_squared($0.center, camera.position) > distance_squared($1.center, camera.position)
        }
        for draw in far {
            guard let blend = draw.blend, let index = Self.blendFunctions.firstIndex(of: blend) else { continue }
            encoder.setRenderPipelineState(blended[index])
            encode(draw, encoder)
        }
        encoder.endEncoding()
    }

    /// The temporal scaler for an output `width` by `height`, made again
    /// when the size or the render scale changes; nil without MetalFX.
    private func temporalScaling(width: Int, height: Int) -> TemporalScaling? {
        guard width > 0, height > 0 else { return nil }
        let input = settings.inputSize(width: width, height: height)
        if let temporal, temporal.outputWidth == width, temporal.outputHeight == height,
           temporal.inputWidth == input.width, temporal.inputHeight == input.height {
            return temporal
        }
        guard supportsTemporal else { return nil }
        temporal = TemporalScaling(device: device, input: input, output: (width, height))
        return temporal
    }

    /// Starts the temporal history again (a cut: the camera moved too far
    /// for the last frames to be of use).
    public func resetHistory() {
        temporal?.frame = 0
        temporal?.previous = nil
    }

    private func encode(_ draw: Draw, _ encoder: MTLRenderCommandEncoder) {
        var uniforms = draw.uniforms
        encoder.setFragmentBytes(&uniforms, length: MemoryLayout<MaterialUniforms>.stride, index: 0)
        encoder.setFragmentTextures(draw.textures, range: 0..<draw.textures.count)
        encoder.drawIndexedPrimitives(type: .triangle, indexCount: draw.indexCount, indexType: .uint32,
                                      indexBuffer: indices, indexBufferOffset: draw.firstIndex * 4)
    }

    /// The half-float frame and the depth buffer, made again when the
    /// target's size changes.
    private func targets(width: Int, height: Int) -> (MTLTexture, MTLTexture)? {
        if let targets, targets.frame.width == width, targets.frame.height == height {
            return targets
        }
        guard width > 0, height > 0 else { return nil }
        let frame = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: Self.frameFormat, width: width,
                                                             height: height, mipmapped: false)
        frame.usage = [.renderTarget, .shaderRead]
        frame.storageMode = .private
        let depth = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: Self.depthFormat, width: width,
                                                             height: height, mipmapped: false)
        depth.usage = .renderTarget
        depth.storageMode = .private
        guard let frameTexture = device.makeTexture(descriptor: frame),
              let depthTexture = device.makeTexture(descriptor: depth) else { return nil }
        targets = (frameTexture, depthTexture)
        return targets
    }

    // ---------- made once

    static func buffer(_ device: MTLDevice, _ bytes: Data) -> MTLBuffer? {
        // (an empty level still has buffers to bind)
        let bytes = bytes.isEmpty ? Data(count: 16) : bytes
        return bytes.withUnsafeBytes { raw in
            raw.baseAddress.flatMap { device.makeBuffer(bytes: $0, length: raw.count, options: .storageModeShared) }
        }
    }

    static func vertexDescriptor(stride: Int) -> MTLVertexDescriptor {
        let descriptor = MTLVertexDescriptor()
        let attributes: [(MTLVertexFormat, Int)] = [(.float3, 0), (.float3, 12), (.float2, 24), (.float2, 32)]
        for (index, (format, offset)) in attributes.enumerated() {
            descriptor.attributes[index].format = format
            descriptor.attributes[index].offset = offset
            descriptor.attributes[index].bufferIndex = 0
        }
        descriptor.layouts[0].stride = stride
        return descriptor
    }

    /// The bytes of a row of a level `width` texels wide, and its rows.
    static func layout(_ format: RenderTextureFormat, width: Int, height: Int) -> (rowBytes: Int, rows: Int) {
        let blocks = (max(width, 1) + 3) / 4
        let rows = (max(height, 1) + 3) / 4
        switch format {
        case .bc1: return (blocks * 8, rows)
        case .bc2, .bc3: return (blocks * 16, rows)
        case .rgba8: return (max(width, 1) * 4, max(height, 1))
        }
    }

    /// A texture with its levels, or nil if the GPU cannot take its format
    /// or a level's bytes are short.
    static func texture(_ device: MTLDevice, _ texture: RenderTexture,
                        level bytes: (UInt32) -> Data) -> MTLTexture? {
        let format: MTLPixelFormat
        switch texture.format {
        case .bc1: format = .bc1_rgba
        case .bc2: format = .bc2_rgba
        case .bc3: format = .bc3_rgba
        case .rgba8: format = .rgba8Unorm
        }
        if texture.format != .rgba8, !device.supportsBCTextureCompression { return nil }
        let width = Int(texture.width)
        let height = Int(texture.height)
        guard width > 0, height > 0, texture.levelCount > 0 else { return nil }
        let fullChain = Int(log2(Double(max(width, height)))) + 1
        let levels = min(Int(texture.levelCount), fullChain)
        let descriptor = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: format, width: width,
                                                                  height: height, mipmapped: levels > 1)
        descriptor.mipmapLevelCount = levels
        descriptor.usage = .shaderRead
        descriptor.storageMode = .shared
        guard let made = device.makeTexture(descriptor: descriptor) else { return nil }
        for level in 0..<levels {
            let levelWidth = max(width >> level, 1)
            let levelHeight = max(height >> level, 1)
            let (rowBytes, rows) = layout(texture.format, width: levelWidth, height: levelHeight)
            let data = bytes(UInt32(level))
            guard data.count >= rowBytes * rows else { return nil }
            data.withUnsafeBytes { raw in
                guard let base = raw.baseAddress else { return }
                made.replace(region: MTLRegionMake2D(0, 0, levelWidth, levelHeight), mipmapLevel: level,
                             withBytes: base, bytesPerRow: rowBytes)
            }
        }
        return made
    }

    /// One white texel, bound where a map is missing.
    static func white(_ device: MTLDevice) -> MTLTexture? {
        let descriptor = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: .rgba8Unorm, width: 1, height: 1,
                                                                  mipmapped: false)
        descriptor.usage = .shaderRead
        descriptor.storageMode = .shared
        guard let texture = device.makeTexture(descriptor: descriptor) else { return nil }
        let white: [UInt8] = [255, 255, 255, 255]
        texture.replace(region: MTLRegionMake2D(0, 0, 1, 1), mipmapLevel: 0, withBytes: white, bytesPerRow: 4)
        return texture
    }
}
