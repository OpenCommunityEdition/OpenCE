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
    /// How the fog takes the fragment: 0 mixed toward its color, 1 added,
    /// 2 multiplied, 3 double-multiplied.
    var fogMode: UInt32 = 0
    /// A transparent chicago shader's maps (0: not one).
    var chicagoCount: UInt32 = 0
    /// Each map's color function, alpha function << 8, alpha replicate << 16.
    var chicagoFunctions: SIMD4<UInt32> = .zero
    /// Each map's scale and offset.
    var chicagoTransforms: (SIMD4<Float>, SIMD4<Float>, SIMD4<Float>, SIMD4<Float>) = (.zero, .zero, .zero, .zero)

    /// `material`'s chicago maps set, and their textures (four, white
    /// where a map has none), or nil for a material of another class.
    mutating func chicago(_ material: RenderMaterial, texture: (UInt32?) -> MTLTexture?,
                          white: MTLTexture) -> [MTLTexture]? {
        let maps = material.chicagoMaps.prefix(4)
        guard !maps.isEmpty else { return nil }
        chicagoCount = UInt32(maps.count)
        var transforms = [SIMD4<Float>](repeating: .zero, count: 4)
        var textures = [MTLTexture](repeating: white, count: 4)
        for (stage, map) in maps.enumerated() {
            chicagoFunctions[stage] = UInt32(map.colorFunction) | UInt32(map.alphaFunction) << 8
                | (map.alphaReplicate ? 1 << 16 : 0)
            let scale = map.scale.count == 2 ? SIMD2(map.scale[0], map.scale[1]) : SIMD2<Float>(1, 1)
            let offset = map.offset.count == 2 ? SIMD2(map.offset[0], map.offset[1]) : SIMD2<Float>(0, 0)
            transforms[stage] = SIMD4(lowHalf: scale, highHalf: offset)
            textures[stage] = texture(map.map) ?? white
        }
        chicagoTransforms = (transforms[0], transforms[1], transforms[2], transforms[3])
        return textures
    }
}

/// The shaders' Frame: the view, and the sky's fog as a plane along it.
public struct FrameUniforms {
    public var viewProjection: simd_float4x4
    /// The view's forward over the fog's range, and where it starts.
    public var fogPlane: SIMD4<Float>
    /// The fog's color, and its maximum density (0: no fog).
    public var fogColor: SIMD4<Float>

    /// The frame from `camera`, its fog `fog` (none without a sky).
    public init(viewProjection: simd_float4x4, camera: ViewCamera, fog: RenderFog?) {
        self.viewProjection = viewProjection
        guard let fog, fog.maximumDensity > 0, fog.color.count == 3 else {
            fogPlane = .zero
            fogColor = .zero
            return
        }
        let forward = camera.rotation.act([1, 0, 0])
        let range = max(fog.endDistance - fog.startDistance, 0.0001)
        fogPlane = SIMD4(forward / range, -(fog.startDistance + dot(camera.position, forward)) / range)
        fogColor = SIMD4(fog.color[0], fog.color[1], fog.color[2], fog.maximumDensity)
    }

    /// A blend's fog mode.
    static func fogMode(_ blend: BlendFunction?) -> UInt32 {
        switch blend {
        case .add: 1
        case .multiply: 2
        case .doubleMultiply: 3
        case .alphaBlend, nil: 0
        }
    }
}

/// The map bits of MaterialUniforms.maps.
enum MapBit {
    static let base: UInt32 = 1
    static let primary: UInt32 = 2
    static let secondary: UInt32 = 4
    static let lightmap: UInt32 = 8
    /// Lit from its normals: a BSP shipped with no lightmaps.
    static let unbaked: UInt32 = 16
}

public final class LevelRenderer {
    /// What the sky's portals show where the level has no sky or fog.
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
    let models: ModelPass
    /// The level's sky (its model is drawn first, at infinity).
    let sky: RenderSky?
    private var opaque: MTLRenderPipelineState
    private var blended: [MTLRenderPipelineState]
    private var tone: MTLRenderPipelineState
    private var motion: MTLRenderPipelineState
    private let vertexStride: Int
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

    /// The level's render models.
    public var modelCount: Int { models.models.count }

    /// The objects' parts drawn in the last frame.
    public var lastModelPartCount: Int { models.lastPartCount }

    /// The sky's parts drawn in the last frame.
    public var lastSkyPartCount: Int { models.lastSkyPartCount }

    /// The first-person weapon's and hands' parts drawn in the last frame.
    public var lastFirstPersonPartCount: Int { models.lastFirstPersonPartCount }

    /// What draws the HUD (without it, the HUD's quads are not drawn).
    public var hud: HudPass?
    /// The debug lines' pass.
    public let debug: DebugPass

    /// The level's fog (none without a sky), for tests.
    public var fog: RenderFog? { sky?.fog }

    public init(device: MTLDevice, geometry: LevelGeometry, outputFormat: MTLPixelFormat = .bgra8Unorm,
                settings: VideoSettings = VideoSettings(), source: String? = nil) throws {
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
            if draw.unbaked {
                bits |= MapBit.unbaked
            }
            let pair = { (values: [Float], fallback: Float) -> SIMD2<Float> in
                values.count == 2 ? SIMD2(values[0], values[1]) : SIMD2(repeating: fallback)
            }
            let blend = material.kind == .blended ? material.blend : nil
            var uniforms = MaterialUniforms(
                baseScale: pair(material.baseScale, 1), baseOffset: pair(material.baseOffset, 0),
                primaryScale: material.primaryDetailScale, secondaryScale: material.secondaryDetailScale,
                maps: bits, detailFunction: UInt32(material.detailFunction),
                environmentKind: UInt32(material.environmentKind),
                alphaTested: material.kind == .alphaTested ? 1 : 0, fogMode: FrameUniforms.fogMode(blend))
            let center = draw.center.count == 3 ? SIMD3(draw.center[0], draw.center[1], draw.center[2]) : .zero
            let chicagoTextures = uniforms.chicago(material, texture: texture, white: white)
            let made = Draw(firstIndex: Int(draw.firstIndex), indexCount: Int(draw.indexCount),
                            blend: blend, uniforms: uniforms,
                            textures: chicagoTextures ?? maps.map { $0 ?? white }, center: center)
            if made.blend == nil {
                opaqueDraws.append(made)
            } else {
                blendedDraws.append(made)
            }
        }
        self.opaqueDraws = opaqueDraws
        self.blendedDraws = blendedDraws

        let library = try Self.library(device: device, source: source ?? ShaderSources.source(.level))
        vertexStride = Int(geometry.vertexStride())
        (opaque, blended, tone, motion) = try Self.pipelines(device: device, library: library,
                                                              stride: vertexStride, outputFormat: outputFormat)
        debug = try DebugPass(device: device, colorFormat: Self.frameFormat, depthFormat: Self.depthFormat)
        models = try ModelPass(device: device, library: library, geometry: geometry, textures: textures, white: white)
        sky = geometry.sky()

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

    private static func library(device: MTLDevice, source: String) throws -> MTLLibrary {
        do {
            return try device.makeLibrary(source: source, options: nil)
        } catch {
            throw LevelRendererError.shaders("\(error)")
        }
    }

    private static func pipelines(device: MTLDevice, library: MTLLibrary, stride: Int, outputFormat: MTLPixelFormat)
        throws -> (MTLRenderPipelineState, [MTLRenderPipelineState], MTLRenderPipelineState, MTLRenderPipelineState) {
        func pipeline(_ blend: BlendFunction?) throws -> MTLRenderPipelineState {
            let descriptor = MTLRenderPipelineDescriptor()
            descriptor.vertexFunction = try library.required("level_vertex")
            descriptor.fragmentFunction = try library.required("level_fragment")
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
            let opaque = try pipeline(nil)
            let blended = try Self.blendFunctions.map { try pipeline($0) }
            let toneDescriptor = MTLRenderPipelineDescriptor()
            toneDescriptor.vertexFunction = try library.required("tone_vertex")
            toneDescriptor.fragmentFunction = try library.required("tone_fragment")
            toneDescriptor.colorAttachments[0].pixelFormat = outputFormat
            let tone = try device.makeRenderPipelineState(descriptor: toneDescriptor)
            let motionDescriptor = MTLRenderPipelineDescriptor()
            motionDescriptor.vertexFunction = try library.required("tone_vertex")
            motionDescriptor.fragmentFunction = try library.required("motion_fragment")
            motionDescriptor.colorAttachments[0].pixelFormat = TemporalScaling.motionFormat
            let motion = try device.makeRenderPipelineState(descriptor: motionDescriptor)
            return (opaque, blended, tone, motion)
        } catch {
            throw LevelRendererError.shaders("\(error)")
        }
    }

    /// Compiles the level's shaders from `source` again, for the level and
    /// its objects; the pipelines are replaced only if they all build.
    public func reloadShaders(source: String) throws {
        let library = try Self.library(device: device, source: source)
        let level = try Self.pipelines(device: device, library: library, stride: vertexStride,
                                       outputFormat: outputFormat)
        let objects = try models.pipelines(library: library)
        (opaque, blended, tone, motion) = level
        models.use(objects)
    }

    static let blendFunctions: [BlendFunction] = [.alphaBlend, .multiply, .doubleMultiply, .add]

    /// Draws the level and `objects` from `camera` into `target` (of
    /// `outputFormat`), `debugLines` in the world, and `hudQuads` over them.
    public func encode(into commands: MTLCommandBuffer, target: MTLTexture, camera: ViewCamera,
                       objects: [DrawnObject] = [], hudQuads: [HudQuad] = [], debugLines: [DebugSegment] = []) {
        let aspect = Float(target.width) / Float(max(target.height, 1))
        let viewProjection = camera.viewProjection(aspect: aspect)
        let shown: MTLTexture
        if settings.antiAliasing == .temporal, let temporal = temporalScaling(width: target.width,
                                                                               height: target.height) {
            let jitter = Jitter.offset(frame: temporal.frame)
            let jittered = Jitter.apply(jitter, to: viewProjection, width: temporal.inputWidth,
                                        height: temporal.inputHeight)
            drawLevel(into: commands, frame: temporal.color, depth: temporal.depth, keepDepth: true,
                      viewProjection: jittered, camera: camera, objects: objects, debugLines: debugLines)
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
                      camera: camera, objects: objects, debugLines: debugLines)
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
        hud?.encode(hudQuads, width: target.width, height: target.height, into: toneEncoder)
        toneEncoder.endEncoding()
    }

    /// The level's opaque draws and the objects' then the blended ones of
    /// both into `frame`.
    private func drawLevel(into commands: MTLCommandBuffer, frame: MTLTexture, depth: MTLTexture, keepDepth: Bool,
                           viewProjection: simd_float4x4, camera: ViewCamera, objects: [DrawnObject],
                           debugLines: [DebugSegment]) {
        let skyObject = sky?.model.map { model in
            DrawnObject(model: Int(model), transform: simd_float4x4(translation: camera.position),
                        center: camera.position, radius: 1, regions: [], nodes: [])
        }
        let prepared = models.prepare(objects, sky: skyObject, camera: camera, viewportHeight: frame.height,
                                      device: device)
        var uniforms = FrameUniforms(viewProjection: viewProjection, camera: camera, fog: sky?.fog)
        let pass = MTLRenderPassDescriptor()
        pass.colorAttachments[0].texture = frame
        pass.colorAttachments[0].loadAction = .clear
        pass.colorAttachments[0].storeAction = .store
        pass.colorAttachments[0].clearColor = uniforms.fogColor.w > 0
            ? MTLClearColor(red: Double(uniforms.fogColor.x), green: Double(uniforms.fogColor.y),
                            blue: Double(uniforms.fogColor.z), alpha: 1)
            : Self.skyColor
        pass.depthAttachment.texture = depth
        pass.depthAttachment.loadAction = .clear
        pass.depthAttachment.storeAction = keepDepth ? .store : .dontCare
        pass.depthAttachment.clearDepth = 0
        guard let encoder = commands.makeRenderCommandEncoder(descriptor: pass) else { return }
        encoder.label = "level"
        encoder.setCullMode(.none)
        encoder.setVertexBuffer(vertices, offset: 0, index: 0)
        encoder.setVertexBytes(&uniforms, length: MemoryLayout<FrameUniforms>.stride, index: 1)
        encoder.setFragmentBytes(&uniforms, length: MemoryLayout<FrameUniforms>.stride, index: 1)
        encoder.setFragmentSamplerState(wrap, index: 0)
        encoder.setFragmentSamplerState(clamp, index: 1)
        models.encodeSky(prepared, into: encoder)
        encoder.setVertexBuffer(vertices, offset: 0, index: 0)
        encoder.setRenderPipelineState(opaque)
        encoder.setDepthStencilState(depthWrite)
        for draw in opaqueDraws {
            encode(draw, encoder)
        }
        models.encode(prepared, blendedParts: false, into: encoder)
        encoder.setVertexBuffer(vertices, offset: 0, index: 0)
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
        models.encode(prepared, blendedParts: true, into: encoder)
        // the debug lines, hidden by what is nearer (and the buffers they took back)
        debug.encode(debugLines, viewProjection: viewProjection, into: encoder)
        if debug.lastDrawn > 0 {
            encoder.setVertexBuffer(vertices, offset: 0, index: 0)
            encoder.setVertexBytes(&uniforms, length: MemoryLayout<FrameUniforms>.stride, index: 1)
        }
        // the first-person weapon and hands over the world: in the nearer
        // half of the depth range (reversed: 0.5 to 1), which the world
        // reaches only nearer the eye than twice the near plane
        if !prepared.firstPerson.isEmpty {
            encoder.setViewport(MTLViewport(originX: 0, originY: 0, width: Double(frame.width),
                                            height: Double(frame.height), znear: 0.5, zfar: 1))
            encoder.setDepthStencilState(depthWrite)
            models.encode(prepared, blendedParts: false, firstPerson: true, into: encoder)
            encoder.setDepthStencilState(depthTest)
            models.encode(prepared, blendedParts: true, firstPerson: true, into: encoder)
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

extension simd_float4x4 {
    /// A move by `offset`.
    init(translation offset: SIMD3<Float>) {
        self = matrix_identity_float4x4
        columns.3 = SIMD4(offset, 1)
    }
}
