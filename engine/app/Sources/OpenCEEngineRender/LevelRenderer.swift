// Draws a level's structure BSP: its buffers, textures, samplers and
// pipelines made once from the engine's LevelGeometry, then each frame the
// level into a half-float target, and the tone map from it into the target
// given (the drawable, or a golden frame's texture). With temporal
// anti-aliasing the level is drawn jittered at the render scale, its motion
// written from its depth, and MetalFX gathers the frames before the tone
// map (TemporalScaling.swift).

import Metal
import OpenCEEngineCore
import opence_ffiFFI
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
    /// 2 multiplied, 3 double-multiplied, 4 tinted and added.
    var fogMode: UInt32 = 0
    /// A transparent chicago shader's maps (0: not one).
    var chicagoCount: UInt32 = 0
    /// Each map's color function, alpha function << 8, alpha replicate << 16.
    var chicagoFunctions: SIMD4<UInt32> = .zero
    /// Each map's scale and offset.
    var chicagoTransforms: (SIMD4<Float>, SIMD4<Float>, SIMD4<Float>, SIMD4<Float>) = (.zero, .zero, .zero, .zero)
    /// This frame's animation (RenderLevel's `ANIMATION_FLOATS`): the
    /// primary self-illumination colour and the plasma's value, the
    /// secondary colour, and the maps' scrolling in u and v.
    var animation: (SIMD4<Float>, SIMD4<Float>, SIMD4<Float>) = (.zero, .zero, .zero)
    /// An environment shader's material colour (white: none).
    var materialColor: SIMD4<Float> = .one
    /// The plasma's colours, where its band is and is not.
    var plasmaOn: SIMD4<Float> = .zero
    var plasmaOff: SIMD4<Float> = .zero
    /// The specular colour along the normal, and the specular brightness.
    var specularPerpendicular: SIMD4<Float> = .zero
    /// The specular colour across it, and the lightmap's brightness scale.
    var specularParallel: SIMD4<Float> = .zero
    /// The micro detail's and bump map's and self-illumination's scales,
    /// and the reflection's brightness along the normal.
    var environmentScales: SIMD4<Float> = .one
    /// The reflection's brightness across the normal, then unused.
    var reflectionParallel: SIMD4<Float> = .zero
    /// The micro detail function, the specular flags, the reflection's
    /// kind and the EnvironmentBit maps (0: not an environment shader).
    var environment: SIMD4<UInt32> = .zero
    /// A model shader's ModelBit flags, its detail mask, its change
    /// colour's source and its self-illumination colour's (0: not a model
    /// shader). Its reflection's tints and brightnesses are in
    /// `specularPerpendicular` and `specularParallel`, its falloff and
    /// cutoff distances in `environmentScales`.
    var model: SIMD4<UInt32> = .zero
    /// A glass, meter, plasma or water shader's TransparentClassCode, its
    /// flags and which of its maps it has (0: none of them). Its tint is
    /// `materialColor`, its colours `specularPerpendicular` and
    /// `specularParallel`, its maps' scales and offsets `chicagoTransforms`.
    var transparent: SIMD4<UInt32> = .zero

    /// `material`'s glass, meter, plasma or water parameters set, and its
    /// four maps' textures and its reflection cube (white where it has
    /// none), or nil for a material of another class.
    mutating func transparent(_ material: RenderMaterial, texture: (UInt32?) -> MTLTexture?,
                              cube: (UInt32?) -> MTLTexture?, white: MTLTexture,
                              whiteCube: MTLTexture) -> (maps: [MTLTexture], cube: MTLTexture)? {
        guard let shader = material.transparent else { return nil }
        func four(_ values: [Float]) -> SIMD4<Float> {
            values.count == 4 ? SIMD4(values[0], values[1], values[2], values[3]) : .zero
        }
        var transforms = [SIMD4<Float>](repeating: SIMD4(1, 1, 0, 0), count: 4)
        var textures = [MTLTexture](repeating: white, count: 4)
        var present: UInt32 = 0
        for (slot, map) in shader.maps.prefix(4).enumerated() {
            let scale = map.scale.count == 2 ? SIMD2(map.scale[0], map.scale[1]) : SIMD2<Float>(1, 1)
            let offset = map.offset.count == 2 ? SIMD2(map.offset[0], map.offset[1]) : SIMD2<Float>(0, 0)
            transforms[slot] = SIMD4(lowHalf: scale, highHalf: offset)
            if let made = texture(map.map) {
                textures[slot] = made
                present |= 1 << slot
            }
        }
        chicagoTransforms = (transforms[0], transforms[1], transforms[2], transforms[3])
        materialColor = four(shader.tint)
        specularPerpendicular = four(shader.perpendicular)
        specularParallel = four(shader.parallel)
        transparent = SIMD4(TransparentClassCode.of(shader.class), UInt32(shader.flags), present, 0)
        return (textures, cube(shader.reflectionMap) ?? whiteCube)
    }

    /// `material`'s model shader parameters set (if it is a model shader),
    /// and its multipurpose map and an unused map's textures and its
    /// reflection cube, white where it has none.
    mutating func model(_ material: RenderMaterial, texture: (UInt32?) -> MTLTexture?,
                        cube: (UInt32?) -> MTLTexture?, white: MTLTexture,
                        whiteCube: MTLTexture) -> (maps: [MTLTexture], cube: MTLTexture) {
        guard let shader = material.model else { return ([white, white], whiteCube) }
        func four(_ values: [Float]) -> SIMD4<Float> {
            values.count == 4 ? SIMD4(values[0], values[1], values[2], values[3]) : .zero
        }
        let reflection = cube(shader.reflectionMap)
        var bits = ModelBit.present
        if reflection != nil { bits |= ModelBit.reflection }
        if shader.detailAfterReflection { bits |= ModelBit.detailAfterReflection }
        specularPerpendicular = four(shader.perpendicular)
        specularParallel = four(shader.parallel)
        environmentScales = SIMD4(shader.reflectionFalloffDistance, shader.reflectionCutoffDistance, 0, 0)
        model = SIMD4(bits, UInt32(material.detailMask), UInt32(material.changeColorSource),
                      UInt32(shader.selfIlluminationColorSource))
        return ([texture(material.multipurposeMap) ?? white, white], reflection ?? whiteCube)
    }

    /// `environment`'s parameters set, and its four maps' textures (micro
    /// detail, bump or specular mask, self-illumination, the cube), white
    /// where it has none.
    mutating func environment(_ environment: RenderEnvironment, texture: (UInt32?) -> MTLTexture?,
                              cube: (UInt32?) -> MTLTexture?, white: MTLTexture,
                              whiteCube: MTLTexture) -> [MTLTexture] {
        func rgb(_ values: [Float], _ fallback: Float) -> SIMD3<Float> {
            values.count == 3 ? SIMD3(values[0], values[1], values[2]) : SIMD3(repeating: fallback)
        }
        // (a scale of zero is one)
        func scale(_ value: Float) -> Float { value.isFinite && value != 0 ? value : 1 }
        let micro = texture(environment.microDetailMap)
        let bump = texture(environment.bumpMap)
        let mask = texture(environment.specularMaskMap)
        let glow = texture(environment.selfIlluminationMap)
        let reflection = cube(environment.reflectionMap)
        var bits = EnvironmentBit.present
        if micro != nil { bits |= EnvironmentBit.micro }
        if bump != nil { bits |= EnvironmentBit.bump }
        if mask != nil { bits |= EnvironmentBit.specularMask }
        if glow != nil { bits |= EnvironmentBit.selfIllumination }
        if environment.selfIlluminationUnfiltered { bits |= EnvironmentBit.unfiltered }
        if reflection != nil { bits |= EnvironmentBit.reflection }
        materialColor = SIMD4(rgb(environment.materialColor, 1), 1)
        plasmaOn = SIMD4(rgb(environment.plasmaOnColor, 0), 0)
        plasmaOff = SIMD4(rgb(environment.plasmaOffColor, 0), 0)
        specularPerpendicular = SIMD4(rgb(environment.specularPerpendicularColor, 0),
                                      environment.specularBrightness)
        specularParallel = SIMD4(rgb(environment.specularParallelColor, 0), environment.lightmapBrightnessScale)
        environmentScales = SIMD4(scale(environment.microDetailScale), scale(environment.bumpScale),
                                  scale(environment.selfIlluminationScale),
                                  environment.reflectionPerpendicularBrightness)
        reflectionParallel = SIMD4(environment.reflectionParallelBrightness, 0, 0, 0)
        self.environment = SIMD4(UInt32(environment.microDetailFunction), UInt32(environment.specularFlags),
                                 UInt32(environment.reflectionKind), bits)
        return [micro ?? white, bump ?? mask ?? white, glow ?? white, reflection ?? whiteCube]
    }

    /// This frame's animation from `floats`, the level's for every
    /// material, at `material`'s.
    mutating func animate(_ floats: [Float], material: Int) {
        let at = material * 12
        guard material >= 0, at + 12 <= floats.count else { return }
        func four(_ from: Int) -> SIMD4<Float> {
            SIMD4(floats[at + from], floats[at + from + 1], floats[at + from + 2], floats[at + from + 3])
        }
        animation = (four(0), four(4), four(8))
    }

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

/// The shaders' Frame: the view, the sky's fog as a plane along it, and
/// the eye.
/// The tone map's uniforms (as level.metal's Tone).
struct ToneUniforms {
    /// The exposure's multiplier, and the bloom's strength.
    var exposure: Float
    var bloom: Float
    /// 0 the frame, 1 its brightness, 2 the bloom alone.
    var view: UInt32
}

public struct FrameUniforms {
    public var viewProjection: simd_float4x4
    /// The view's forward over the fog's range, and where it starts.
    public var fogPlane: SIMD4<Float>
    /// The fog's color, and its maximum density (0: no fog).
    public var fogColor: SIMD4<Float>
    /// The camera's position (the reflections' and highlights' eye), and
    /// the mipmap bias.
    public var eye: SIMD4<Float> = .zero
    /// The baked probe grid's first probe, and one over its spacing.
    public var probeOrigin: SIMD4<Float> = .zero
    /// How many probes each way, and 1 when there are baked probes (0:
    /// the unbaked surfaces are lit from their normals alone).
    public var probeCounts: SIMD4<Float> = .zero
    /// The way to the bake's sun, and 1 when there is one (0: none).
    public var sunTowards: SIMD4<Float> = .zero
    /// The sun's irradiance, which the probes leave out for the objects
    /// to add where the probes see it.
    public var sunColor: SIMD4<Float> = .zero

    /// The frame from `camera`, its fog `fog` (none without a sky), its
    /// maps sampled `mipBias` levels sharper (negative) than its size's.
    public init(viewProjection: simd_float4x4, camera: ViewCamera, fog: RenderFog?, mipBias: Float = 0) {
        self.viewProjection = viewProjection
        eye = SIMD4(camera.position, mipBias)
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
        case .tintAndAdd: 4
        case .alphaBlend, nil: 0
        }
    }
}

/// The transparent classes' codes (MaterialUniforms' `transparent.x`).
enum TransparentClassCode {
    static func of(_ transparentClass: TransparentClass) -> UInt32 {
        switch transparentClass {
        case .glass: 1
        case .meter: 2
        case .plasma: 3
        case .water: 4
        }
    }
}

/// The map bits of MaterialUniforms.maps.
enum MapBit {
    static let base: UInt32 = 1
    static let primary: UInt32 = 2
    static let secondary: UInt32 = 4
    static let lightmap: UInt32 = 8
    /// Lit from its normals: a BSP shipped with no lightmaps (or from the
    /// baked probes, once there are some).
    static let unbaked: UInt32 = 16
    /// The lightmap is the bake's: irradiance, and its direction in
    /// texture 8.
    static let baked: UInt32 = 32
}

/// The bits of MaterialUniforms.environment.w.
enum EnvironmentBit {
    /// An environment shader: lit as its combiners light it.
    static let present: UInt32 = 1
    static let micro: UInt32 = 2
    static let bump: UInt32 = 4
    /// The bump map's slot holds the specular mask instead.
    static let specularMask: UInt32 = 8
    static let selfIllumination: UInt32 = 16
    /// The self-illumination map sampled without filtering.
    static let unfiltered: UInt32 = 32
    static let reflection: UInt32 = 64
}

/// The model fragment shader's MODEL bits (MaterialUniforms' `model.x`).
enum ModelBit {
    /// A model shader: lit as the model combiner lights it.
    static let present: UInt32 = 1
    static let reflection: UInt32 = 2
    /// The detail map applies to the lit and reflected colour.
    static let detailAfterReflection: UInt32 = 4
}

public final class LevelRenderer {
    /// What the sky's portals show where the level has no sky or fog.
    public static let skyColor = MTLClearColor(red: 0.42, green: 0.52, blue: 0.66, alpha: 1)

    struct Draw {
        var firstIndex: Int
        var indexCount: Int
        var blend: BlendFunction?
        var uniforms: MaterialUniforms
        /// Base, primary detail, secondary detail, lightmap, micro detail,
        /// bump or specular mask, self-illumination, the reflection's cube
        /// (a chicago shader's four maps, then white) and the baked
        /// light's directions.
        var textures: [MTLTexture]
        var center: SIMD3<Float>
        /// Its material, whose animation it takes each frame.
        var material: Int
        /// Its page in the BSP's lightmaps, when it is drawn with one.
        var lightmapPage: UInt32?
    }

    public let device: MTLDevice
    public let outputFormat: MTLPixelFormat
    let vertices: MTLBuffer
    let indices: MTLBuffer
    private(set) var opaqueDraws: [Draw]
    private(set) var blendedDraws: [Draw]
    /// The baked probes' light from each axis (+x, -x, +y, -y, +z, -z):
    /// white and unused until the bake is drawn with.
    private var probes: [MTLTexture]
    /// The baked probes' grid, for the frame's uniforms.
    private var probeGrid: (origin: SIMD4<Float>, counts: SIMD4<Float>) = (.zero, .zero)
    /// The bake's sun for the frame's uniforms (none until the bake).
    private var sun: (towards: SIMD4<Float>, color: SIMD4<Float>) = (.zero, .zero)
    /// The bake's reflection probes' radiance, a cube each: one of zero
    /// and unused until the bake is drawn with.
    private var reflections: MTLTexture
    /// Whether the bake's light is drawn.
    public private(set) var baked = false
    private let white: MTLTexture
    let textureCount: Int
    let models: ModelPass
    /// The level's sky (its model is drawn first, at infinity).
    let sky: RenderSky?
    /// The level, for its materials' animation each frame.
    private let geometry: LevelGeometry
    /// The game's time in seconds, which the shaders' animations follow
    /// (set before each frame; 0 draws them as at the start).
    public var time: Float = 0
    /// The frame's dynamic lights, already between their ticks (set
    /// before each frame, as `time` is).
    public var lights: [OCELightSnapshot] = []
    /// The dynamic lights' cells, gels and shadows.
    let lightPass: LightPass
    /// How many dynamic lights the last frame drew, and how many cast
    /// shadows.
    public var lightsDrawn: Int { lightPass.lastLightCount }
    public var lightShadowsDrawn: Int { lightPass.lastShadowCount }
    private var opaque: MTLRenderPipelineState
    private var blended: [MTLRenderPipelineState]
    private var tone: MTLRenderPipelineState
    private var motion: MTLRenderPipelineState
    private let vertexStride: Int
    private let depthWrite: MTLDepthStencilState
    private let depthTest: MTLDepthStencilState
    private let wrap: MTLSamplerState
    private let clamp: MTLSamplerState
    /// Wrapped and unfiltered, for the self-illumination maps that ask.
    private let nearest: MTLSamplerState
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
    /// The sun's shadow maps.
    let shadows: ShadowPass
    /// The screen's passes: occlusion, contact shadows, reflections.
    let screen: ScreenPass
    /// The camera's passes: lens flares, glows, exposure, bloom.
    let cameraPass: CameraPass
    let decals: DecalPass
    /// The frame's lens flares, light volumes and glows (set before each
    /// frame, as `lights` is).
    public var sprites = CameraSprites(sprites: [], occluders: [])
    /// The sprites the last frame drew.
    public private(set) var lastSpriteCount = 0
    /// The decals the level lays.
    public var decalCount: Int { decals.count }
    /// The exposure, in stops darker than the level's own light, adapted
    /// from frame to frame as `time` goes on (`render.exposure`).
    public private(set) var exposure: Float = 0
    /// The exposure's limits in the BSP drawn.
    public let exposureLimits: [Float]
    private var exposureTime: Float?
    /// The passes switched on, and the buffer shown (`render.*`).
    public var switches = RenderSwitches()
    /// Whether the sun's shadows are drawn (with a baked sun).
    public var sunShadows: Bool {
        get { switches.sunShadows }
        set { switches.sunShadows = newValue }
    }
    /// Whether the last frame drew the sun's shadows, and the width of
    /// their maps in texels.
    public var sunShadowsDrawn: Bool { shadows.uniforms.forward.w > 0 }
    public var sunShadowMapWidth: Int { shadows.maps.width }

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
        self.geometry = geometry

        let descriptions = geometry.textures()
        let textures = descriptions.enumerated().map { index, texture in
            Self.texture(device, texture) { level in geometry.textureLevel(texture: UInt32(index), level: level) }
        }
        textureCount = textures.compactMap { $0 }.count
        guard let white = Self.white(device), let whiteCube = Self.white(device, cube: true),
              let whiteVolume = Self.whiteVolume(device) else {
            throw LevelRendererError.resources("textures")
        }
        self.white = white
        probes = Array(repeating: whiteVolume, count: 6)
        guard let darkCubes = Self.cubeArray(device, size: 1, count: 1, bytes: Array(repeating: 0, count: 48)) else {
            throw LevelRendererError.resources("reflection probes")
        }
        reflections = darkCubes
        let materials = geometry.materials()
        func texture(_ index: UInt32?) -> MTLTexture? {
            index.flatMap { Int($0) < textures.count ? textures[Int($0)] : nil }
                .flatMap { $0.textureType == .type2D ? $0 : nil }
        }
        func cube(_ index: UInt32?) -> MTLTexture? {
            index.flatMap { Int($0) < textures.count ? textures[Int($0)] : nil }
                .flatMap { $0.textureType == .typeCube ? $0 : nil }
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
            var bound: [MTLTexture]
            if let shaded = uniforms.transparent(material, texture: texture, cube: cube, white: white,
                                                 whiteCube: whiteCube) {
                bound = shaded.maps + [white, white, white, shaded.cube]
            } else if let chicago = uniforms.chicago(material, texture: texture, white: white) {
                bound = chicago + [white, white, white, whiteCube]
            } else {
                bound = maps.map { $0 ?? white }
                bound += material.environment.map {
                    uniforms.environment($0, texture: texture, cube: cube, white: white, whiteCube: whiteCube)
                } ?? [white, white, white, whiteCube]
            }
            let made = Draw(firstIndex: Int(draw.firstIndex), indexCount: Int(draw.indexCount),
                            blend: blend, uniforms: uniforms, textures: bound + [white], center: center,
                            material: Int(draw.material), lightmapPage: draw.lightmapPage)
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
        models = try ModelPass(device: device, library: library, geometry: geometry, textures: textures, white: white,
                               whiteCube: whiteCube)
        shadows = try ShadowPass(device: device, library: library, levelStride: vertexStride,
                                 modelStride: Int(geometry.modelVertexStride()))
        lightPass = try LightPass(device: device, gels: geometry.lightGels().map(cube), whiteCube: whiteCube)
        screen = try ScreenPass(device: device, source: ShaderSources.source(.screen))
        cameraPass = try CameraPass(device: device, source: ShaderSources.source(.camera),
                                    images: geometry.cameraTextures().map { texture($0) })
        decals = try DecalPass(device: device, source: ShaderSources.source(.decal), decals: geometry.decals(),
                               texture: { texture($0) })
        exposureLimits = geometry.exposureLimits()
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
        let nearestDescriptor = MTLSamplerDescriptor()
        nearestDescriptor.sAddressMode = .repeat
        nearestDescriptor.tAddressMode = .repeat
        guard let depthWrite = device.makeDepthStencilState(descriptor: write),
              let depthTest = device.makeDepthStencilState(descriptor: test),
              let wrap = device.makeSamplerState(descriptor: wrapDescriptor),
              let clamp = device.makeSamplerState(descriptor: clampDescriptor),
              let nearest = device.makeSamplerState(descriptor: nearestDescriptor) else {
            throw LevelRendererError.resources("depth and sampler states")
        }
        self.depthWrite = depthWrite
        self.depthTest = depthTest
        self.wrap = wrap
        self.clamp = clamp
        self.nearest = nearest
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
            descriptor.fragmentFunction = try library.required(blend == .tintAndAdd ? "transparent_fragment"
                                                                                    : "level_fragment")
            descriptor.vertexDescriptor = Self.vertexDescriptor(stride: stride)
            descriptor.colorAttachments[0].pixelFormat = Self.frameFormat
            descriptor.depthAttachmentPixelFormat = Self.depthFormat
            if let blend {
                Self.blending(descriptor.colorAttachments[0], blend)
            } else {
                ScreenPass.attachments(descriptor, written: true)
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
        let casting = try ShadowPass.pipelines(device: device, library: library, levelStride: vertexStride,
                                               modelStride: Int(geometry.modelVertexStride()))
        (opaque, blended, tone, motion) = level
        models.use(objects)
        shadows.use(casting)
    }

    static let blendFunctions: [BlendFunction] = [.alphaBlend, .multiply, .doubleMultiply, .add, .tintAndAdd]

    /// `color` blended with the frame as `blend` blends.
    static func blending(_ color: MTLRenderPipelineColorAttachmentDescriptor, _ blend: BlendFunction) {
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
        case .tintAndAdd:
            color.sourceRGBBlendFactor = .one
            color.destinationRGBBlendFactor = .source1Color
        }
    }

    /// Draws the level and `objects` from `camera` into `target` (of
    /// `outputFormat`), `debugLines` in the world, and `hudQuads` over them.
    public func encode(into commands: MTLCommandBuffer, target: MTLTexture, camera: ViewCamera,
                       objects: [DrawnObject] = [], hudQuads: [HudQuad] = [], debugLines: [DebugSegment] = []) {
        let aspect = Float(target.width) / Float(max(target.height, 1))
        let viewProjection = camera.viewProjection(aspect: aspect)
        adaptExposure()
        let shown: MTLTexture
        if settings.antiAliasing == .temporal, let temporal = temporalScaling(width: target.width,
                                                                               height: target.height) {
            let jitter = Jitter.offset(frame: temporal.frame)
            let jittered = Jitter.apply(jitter, to: viewProjection, width: temporal.inputWidth,
                                        height: temporal.inputHeight)
            // (the scaler's input samples its maps as the output would, as
            // MetalFX asks: log2 of the scale, a level sharper at half)
            let mipBias = log2(Float(temporal.inputWidth) / Float(max(target.width, 1)))
            drawLevel(into: commands, frame: temporal.color, depth: temporal.depth, keepDepth: true,
                      viewProjection: jittered, camera: camera, objects: objects, debugLines: debugLines,
                      mipBias: mipBias)
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

        if switches.exposure || switches.view == .luminance {
            cameraPass.encodeLuminance(into: commands, frame: shown)
        }
        let bloom = switches.bloom || switches.view == .bloom
            ? cameraPass.encodeBloom(into: commands, frame: shown) : nil
        var toneUniforms = ToneUniforms(exposure: exp2(-exposure),
                                        bloom: switches.bloom ? CameraPass.bloomStrength : 0,
                                        view: switches.view == .luminance ? 1 : switches.view == .bloom ? 2 : 0)

        let tonePass = MTLRenderPassDescriptor()
        tonePass.colorAttachments[0].texture = target
        tonePass.colorAttachments[0].loadAction = .dontCare
        tonePass.colorAttachments[0].storeAction = .store
        guard let toneEncoder = commands.makeRenderCommandEncoder(descriptor: tonePass) else { return }
        toneEncoder.label = "tone map"
        toneEncoder.setRenderPipelineState(tone)
        toneEncoder.setFragmentTexture(shown, index: 0)
        toneEncoder.setFragmentTexture(bloom ?? cameraPass.blackLevel(), index: 1)
        toneEncoder.setFragmentBytes(&toneUniforms, length: MemoryLayout<ToneUniforms>.stride, index: 0)
        toneEncoder.drawPrimitives(type: .triangle, vertexStart: 0, vertexCount: 3)
        hud?.encode(hudQuads, width: target.width, height: target.height, into: toneEncoder)
        toneEncoder.endEncoding()
    }

    /// The exposure for this frame: towards the brightness of the frame
    /// before the last (the last may still be drawing), as far as the time
    /// since the last frame lets it; none when it is switched off.
    private func adaptExposure() {
        defer { exposureTime = time }
        guard switches.exposure else {
            exposure = 0
            return
        }
        guard let average = cameraPass.logLuminance(age: 1) else { return }
        let seconds = exposureTime.map { time - $0 } ?? 0
        exposure = OpenCEEngineCore.adaptExposure(current: exposure, logLuminance: average, limits: exposureLimits,
                                                  seconds: seconds)
    }

    /// The level's opaque draws and the objects' then the blended ones of
    /// both into `frame`.
    private func drawLevel(into commands: MTLCommandBuffer, frame: MTLTexture, depth: MTLTexture, keepDepth: Bool,
                           viewProjection: simd_float4x4, camera: ViewCamera, objects: [DrawnObject],
                           debugLines: [DebugSegment], mipBias: Float = 0) {
        let skyObject = sky?.model.map { model in
            DrawnObject(model: Int(model), transform: simd_float4x4(translation: camera.position),
                        center: camera.position, radius: 1, regions: [], nodes: [])
        }
        let prepared = models.prepare(objects, sky: skyObject, camera: camera, viewportHeight: frame.height,
                                      device: device)
        let animation = geometry.materialAnimation(seconds: time)
        var uniforms = FrameUniforms(viewProjection: viewProjection, camera: camera, fog: sky?.fog, mipBias: mipBias)
        uniforms.probeOrigin = probeGrid.origin
        uniforms.probeCounts = probeGrid.counts
        uniforms.sunTowards = sun.towards
        uniforms.sunColor = sun.color
        let towards = sunShadows && sun.towards.w > 0
            ? SIMD3(sun.towards.x, sun.towards.y, sun.towards.z) : nil
        shadows.encode(into: commands, camera: camera, aspect: Float(frame.width) / Float(max(frame.height, 1)),
                       towards: towards, wrap: wrap,
                       level: { encoder, casting in self.encodeShadowCasters(encoder, casting) },
                       objects: { encoder, casting in
                           self.models.encodeShadowCasters(prepared, casting, into: encoder)
                       })
        var shadowUniforms = shadows.uniforms
        let lightFrame = lightPass.prepare(switches.lights ? lights : [], camera: camera,
                                           aspect: Float(frame.width) / Float(max(frame.height, 1)))
        let casting = shadows.pipelines
        lightPass.encodeShadows(lightFrame, into: commands, wrap: wrap) { encoder in
            self.encodeShadowCasters(encoder, casting.level)
            self.models.encodeShadowCasters(prepared, casting.model, into: encoder)
        }
        let screenTargets = screen.targets(width: frame.width, height: frame.height)
        let fullViewport = MTLViewport(originX: 0, originY: 0, width: Double(frame.width),
                                       height: Double(frame.height), znear: 0, zfar: 1)
        // the first-person weapon and hands over the world: in the nearer
        // half of the depth range (reversed: 0.5 to 1), which the world
        // reaches only nearer the eye than twice the near plane
        let nearViewport = MTLViewport(originX: 0, originY: 0, width: Double(frame.width),
                                       height: Double(frame.height), znear: 0.5, zfar: 1)
        func bind(_ encoder: MTLRenderCommandEncoder) {
            encoder.setCullMode(.none)
            encoder.setVertexBuffer(vertices, offset: 0, index: 0)
            encoder.setVertexBytes(&uniforms, length: MemoryLayout<FrameUniforms>.stride, index: 1)
            encoder.setFragmentBytes(&uniforms, length: MemoryLayout<FrameUniforms>.stride, index: 1)
            encoder.setFragmentSamplerState(wrap, index: 0)
            encoder.setFragmentSamplerState(clamp, index: 1)
            encoder.setFragmentSamplerState(nearest, index: 2)
            encoder.setFragmentTextures(probes, range: 9..<15)
            encoder.setFragmentTexture(reflections, index: 15)
            encoder.setFragmentTexture(shadows.levelMaps, index: 16)
            encoder.setFragmentTexture(shadows.maps, index: 17)
            encoder.setFragmentBytes(&shadowUniforms, length: MemoryLayout<ShadowUniforms>.stride, index: 4)
            lightPass.bind(lightFrame, into: encoder)
        }

        // the opaque surfaces, with their normals and reflections for the
        // screen's passes
        let pass = MTLRenderPassDescriptor()
        pass.colorAttachments[0].texture = frame
        pass.colorAttachments[0].loadAction = .clear
        pass.colorAttachments[0].storeAction = .store
        pass.colorAttachments[0].clearColor = uniforms.fogColor.w > 0
            ? MTLClearColor(red: Double(uniforms.fogColor.x), green: Double(uniforms.fogColor.y),
                            blue: Double(uniforms.fogColor.z), alpha: 1)
            : Self.skyColor
        for (index, texture) in (screenTargets?.attachments ?? []).enumerated() {
            pass.colorAttachments[index + 1].texture = texture
            pass.colorAttachments[index + 1].loadAction = .clear
            pass.colorAttachments[index + 1].storeAction = .store
            pass.colorAttachments[index + 1].clearColor = MTLClearColor(red: 0, green: 0, blue: 0, alpha: 0)
        }
        pass.depthAttachment.texture = depth
        pass.depthAttachment.loadAction = .clear
        pass.depthAttachment.storeAction = .store
        pass.depthAttachment.clearDepth = 0
        guard screenTargets != nil, let encoder = commands.makeRenderCommandEncoder(descriptor: pass) else { return }
        encoder.label = "level opaque"
        bind(encoder)
        models.encodeSky(prepared, into: encoder)
        encoder.setVertexBuffer(vertices, offset: 0, index: 0)
        encoder.setRenderPipelineState(opaque)
        encoder.setDepthStencilState(depthWrite)
        for draw in opaqueDraws {
            encode(draw, animation, encoder)
        }
        models.encode(prepared, blendedParts: false, animation: animation, into: encoder)
        if !prepared.firstPerson.isEmpty {
            encoder.setViewport(nearViewport)
            encoder.setDepthStencilState(depthWrite)
            models.encode(prepared, blendedParts: false, firstPerson: true, animation: animation, into: encoder)
        }
        encoder.endEncoding()

        // the decals on the level's surfaces, then occlusion and
        // reflections from what the opaque pass left
        guard let screenTargets else { return }
        if switches.decals {
            decals.encode(into: commands, frame: frame, depth: depth, surface: screenTargets.surface,
                          viewProjection: viewProjection, eye: camera.position)
        }
        let screenUniforms = ScreenUniforms(viewProjection: viewProjection, eye: camera.position,
                                            sunTowards: towards == nil ? .zero : sun.towards, width: frame.width,
                                            height: frame.height, switches: switches)
        screen.encode(into: commands, targets: screenTargets, frame: frame, depth: depth, uniforms: screenUniforms)
        let cameraFrame = cameraPass.encodeOcclusion(sprites, into: commands, depth: depth,
                                                     viewProjection: viewProjection, camera: camera)
        lastSpriteCount = cameraFrame?.spriteCount ?? 0

        // applied to the frame, then the blended surfaces over it
        let over = MTLRenderPassDescriptor()
        over.colorAttachments[0].texture = frame
        over.colorAttachments[0].loadAction = .load
        over.colorAttachments[0].storeAction = .store
        over.depthAttachment.texture = depth
        over.depthAttachment.loadAction = .load
        over.depthAttachment.storeAction = keepDepth ? .store : .dontCare
        guard let blendedEncoder = commands.makeRenderCommandEncoder(descriptor: over) else { return }
        blendedEncoder.label = "level blended"
        screen.resolve(screenTargets, uniforms: screenUniforms, into: blendedEncoder)
        bind(blendedEncoder)
        blendedEncoder.setDepthStencilState(depthTest)
        // far to near
        let far = blendedDraws.sorted {
            distance_squared($0.center, camera.position) > distance_squared($1.center, camera.position)
        }
        for draw in far {
            guard let blend = draw.blend, let index = Self.blendFunctions.firstIndex(of: blend) else { continue }
            blendedEncoder.setRenderPipelineState(blended[index])
            encode(draw, animation, blendedEncoder)
        }
        models.encode(prepared, blendedParts: true, animation: animation, into: blendedEncoder)
        // the lens flares, light volumes and glows, added
        if let cameraFrame {
            cameraPass.encodeSprites(cameraFrame, into: blendedEncoder)
            blendedEncoder.setVertexBuffer(vertices, offset: 0, index: 0)
            blendedEncoder.setVertexBytes(&uniforms, length: MemoryLayout<FrameUniforms>.stride, index: 1)
        }
        // the debug lines, hidden by what is nearer (and the buffers they took back)
        debug.encode(debugLines, viewProjection: viewProjection, into: blendedEncoder)
        if debug.lastDrawn > 0 {
            blendedEncoder.setVertexBuffer(vertices, offset: 0, index: 0)
            blendedEncoder.setVertexBytes(&uniforms, length: MemoryLayout<FrameUniforms>.stride, index: 1)
        }
        if !prepared.firstPerson.isEmpty {
            blendedEncoder.setViewport(nearViewport)
            blendedEncoder.setDepthStencilState(depthTest)
            models.encode(prepared, blendedParts: true, firstPerson: true, animation: animation,
                          into: blendedEncoder)
            blendedEncoder.setViewport(fullViewport)
        }
        screen.view(screenTargets, uniforms: screenUniforms, into: blendedEncoder)
        blendedEncoder.endEncoding()
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

    /// Draws with `light`, the BSP's baked light: each lightmapped draw
    /// takes its page's baked irradiance and directions, and the surfaces
    /// without lightmaps (and, later, the objects) take the probes'.
    /// Pages the bake lacks keep their stock lightmaps.
    public func useBakedLighting(_ light: BakedLight) throws {
        var pages: [UInt32: (irradiance: MTLTexture, directions: MTLTexture)] = [:]
        for page in light.pages() {
            let width = Int(page.width), height = Int(page.height)
            guard width > 0, height > 0, page.irradiance.count == width * height * 8,
                  page.directions.count == width * height * 4,
                  let irradiance = Self.texture2D(device, .rgba16Float, width, height, Array(page.irradiance), 8),
                  let directions = Self.texture2D(device, .rgba8Unorm, width, height, Array(page.directions), 4)
            else {
                throw LevelRendererError.resources("baked lightmap \(page.page)")
            }
            pages[page.page] = (irradiance, directions)
        }
        let counts = light.probeCounts().map(Int.init)
        let origin = light.probeOrigin()
        var volumes: [MTLTexture] = []
        if counts.count == 3, origin.count == 3, counts.allSatisfy({ $0 > 0 }) {
            for axis in 0..<6 {
                guard let volume = Self.volume(device, counts, Array(light.probeBytes(axis: UInt32(axis)))) else {
                    throw LevelRendererError.resources("baked probes")
                }
                volumes.append(volume)
            }
            probes = volumes
            probeGrid = (SIMD4(origin[0], origin[1], origin[2], 1 / max(light.probeSpacing(), 1e-3)),
                         SIMD4(Float(counts[0]), Float(counts[1]), Float(counts[2]), 1))
        }
        let towards = light.sunTowards(), color = light.sunIrradiance()
        if towards.count == 3, color.count == 3 {
            sun = (SIMD4(towards[0], towards[1], towards[2], 1), SIMD4(color[0], color[1], color[2], 0))
        }
        let size = Int(light.reflectionSize())
        let positions = light.reflectionPositions()
        let probeCount = positions.count / 3
        if probeCount > 0 {
            guard let cubes = Self.cubeArray(device, size: size, count: probeCount,
                                             bytes: Array(light.reflectionBytes())) else {
                throw LevelRendererError.resources("baked reflection probes")
            }
            reflections = cubes
            models.reflectionProbes = (0..<probeCount).map {
                SIMD3(positions[$0 * 3], positions[$0 * 3 + 1], positions[$0 * 3 + 2])
            }
        }
        func bake(_ draw: inout Draw) {
            guard let page = draw.lightmapPage.flatMap({ pages[$0] }), draw.uniforms.maps & MapBit.lightmap != 0,
                  draw.uniforms.chicagoCount == 0, draw.uniforms.transparent.x == 0 else { return }
            draw.textures[3] = page.irradiance
            draw.textures[8] = page.directions
            draw.uniforms.maps |= MapBit.baked
        }
        for index in opaqueDraws.indices {
            bake(&opaqueDraws[index])
        }
        for index in blendedDraws.indices {
            bake(&blendedDraws[index])
        }
        baked = true
    }

    /// How many draws take the bake's lightmaps.
    public var bakedDrawCount: Int {
        (opaqueDraws + blendedDraws).filter { $0.uniforms.maps & MapBit.baked != 0 }.count
    }

    /// A shared 2D texture of `bytes`, `texel` bytes a texel.
    static func texture2D(_ device: MTLDevice, _ format: MTLPixelFormat, _ width: Int, _ height: Int,
                          _ bytes: [UInt8], _ texel: Int) -> MTLTexture? {
        let descriptor = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: format, width: width, height: height,
                                                                  mipmapped: false)
        descriptor.usage = .shaderRead
        guard let texture = device.makeTexture(descriptor: descriptor) else { return nil }
        texture.replace(region: MTLRegionMake2D(0, 0, width, height), mipmapLevel: 0, withBytes: bytes,
                        bytesPerRow: width * texel)
        return texture
    }

    /// A half-float RGBA volume `counts` texels each way, of `bytes`.
    static func volume(_ device: MTLDevice, _ counts: [Int], _ bytes: [UInt8]) -> MTLTexture? {
        guard counts.count == 3, bytes.count == counts.reduce(1, *) * 8 else { return nil }
        let descriptor = MTLTextureDescriptor()
        descriptor.textureType = .type3D
        descriptor.pixelFormat = .rgba16Float
        (descriptor.width, descriptor.height, descriptor.depth) = (counts[0], counts[1], counts[2])
        descriptor.usage = .shaderRead
        guard let texture = device.makeTexture(descriptor: descriptor) else { return nil }
        texture.replace(region: MTLRegionMake3D(0, 0, 0, counts[0], counts[1], counts[2]), mipmapLevel: 0, slice: 0,
                        withBytes: bytes, bytesPerRow: counts[0] * 8, bytesPerImage: counts[0] * counts[1] * 8)
        return texture
    }

    /// `count` cubes of `size` texels each way from `bytes`: four half
    /// floats a texel, cube by cube, face by face, rows from the top.
    static func cubeArray(_ device: MTLDevice, size: Int, count: Int, bytes: [UInt8]) -> MTLTexture? {
        let face = size * size * 8
        guard size > 0, count > 0, bytes.count == count * 6 * face else { return nil }
        let descriptor = MTLTextureDescriptor()
        descriptor.textureType = .typeCubeArray
        descriptor.pixelFormat = .rgba16Float
        (descriptor.width, descriptor.height, descriptor.arrayLength) = (size, size, count)
        descriptor.usage = .shaderRead
        guard let texture = device.makeTexture(descriptor: descriptor) else { return nil }
        bytes.withUnsafeBytes { raw in
            guard let base = raw.baseAddress else { return }
            for slice in 0..<(count * 6) {
                texture.replace(region: MTLRegionMake2D(0, 0, size, size), mipmapLevel: 0, slice: slice,
                                withBytes: base + slice * face, bytesPerRow: size * 8, bytesPerImage: face)
            }
        }
        return texture
    }

    /// A white volume of one texel.
    static func whiteVolume(_ device: MTLDevice) -> MTLTexture? {
        volume(device, [1, 1, 1], [0x00, 0x3c, 0x00, 0x3c, 0x00, 0x3c, 0x00, 0x3c])
    }

    /// Starts the temporal history again (a cut: the camera moved too far
    /// for the last frames to be of use).
    public func resetHistory() {
        temporal?.frame = 0
        temporal?.previous = nil
    }

    /// The level's opaque and alpha-tested surfaces, as the sun's
    /// shadows' casters.
    private func encodeShadowCasters(_ encoder: MTLRenderCommandEncoder, _ casting: ShadowPass.Casting) {
        encoder.setVertexBuffer(vertices, offset: 0, index: 0)
        var current: MTLRenderPipelineState?
        for draw in opaqueDraws {
            let cutout = draw.uniforms.alphaTested != 0
            let pipeline = cutout ? casting.cutout : casting.depth
            if current !== pipeline {
                encoder.setRenderPipelineState(pipeline)
                current = pipeline
            }
            if cutout {
                var material = draw.uniforms
                encoder.setFragmentBytes(&material, length: MemoryLayout<MaterialUniforms>.stride, index: 0)
                encoder.setFragmentTexture(draw.textures.first, index: 0)
            }
            encoder.drawIndexedPrimitives(type: .triangle, indexCount: draw.indexCount, indexType: .uint32,
                                          indexBuffer: indices, indexBufferOffset: draw.firstIndex * 4)
        }
    }

    private func encode(_ draw: Draw, _ animation: [Float], _ encoder: MTLRenderCommandEncoder) {
        var uniforms = draw.uniforms
        uniforms.animate(animation, material: draw.material)
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
        depth.usage = [.renderTarget, .shaderRead]
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
        // position, normal, texcoord, lightmap texcoord, tangent, binormal
        // and the incident light (RenderLevel's VERTEX_STRIDE)
        let attributes: [(MTLVertexFormat, Int)] = [(.float3, 0), (.float3, 12), (.float2, 24), (.float2, 32),
                                                    (.float3, 40), (.float3, 52), (.float3, 64)]
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
        let faces = Int(texture.faces)
        guard width > 0, height > 0, texture.levelCount > 0, faces == 1 || (faces == 6 && width == height) else {
            return nil
        }
        let fullChain = Int(log2(Double(max(width, height)))) + 1
        let levels = min(Int(texture.levelCount), fullChain)
        let descriptor = faces == 6
            ? MTLTextureDescriptor.textureCubeDescriptor(pixelFormat: format, size: width, mipmapped: levels > 1)
            : MTLTextureDescriptor.texture2DDescriptor(pixelFormat: format, width: width, height: height,
                                                       mipmapped: levels > 1)
        descriptor.mipmapLevelCount = levels
        descriptor.usage = .shaderRead
        descriptor.storageMode = .shared
        guard let made = device.makeTexture(descriptor: descriptor) else { return nil }
        for level in 0..<levels {
            let levelWidth = max(width >> level, 1)
            let levelHeight = max(height >> level, 1)
            let (rowBytes, rows) = layout(texture.format, width: levelWidth, height: levelHeight)
            let data = bytes(UInt32(level))
            let faceBytes = rowBytes * rows
            guard data.count >= faceBytes * faces else { return nil }
            data.withUnsafeBytes { raw in
                guard let base = raw.baseAddress else { return }
                // (a cube's faces in turn, +x, -x, +y, -y, +z, -z: Metal's slices)
                for face in 0..<faces {
                    made.replace(region: MTLRegionMake2D(0, 0, levelWidth, levelHeight), mipmapLevel: level,
                                 slice: face, withBytes: base + face * faceBytes, bytesPerRow: rowBytes,
                                 bytesPerImage: faceBytes)
                }
            }
        }
        return made
    }

    /// One white texel (on each face of a cube), bound where a map is
    /// missing.
    static func white(_ device: MTLDevice, cube: Bool = false) -> MTLTexture? {
        let descriptor = cube
            ? MTLTextureDescriptor.textureCubeDescriptor(pixelFormat: .rgba8Unorm, size: 1, mipmapped: false)
            : MTLTextureDescriptor.texture2DDescriptor(pixelFormat: .rgba8Unorm, width: 1, height: 1,
                                                       mipmapped: false)
        descriptor.usage = .shaderRead
        descriptor.storageMode = .shared
        guard let texture = device.makeTexture(descriptor: descriptor) else { return nil }
        let white: [UInt8] = [255, 255, 255, 255]
        for face in 0..<(cube ? 6 : 1) {
            texture.replace(region: MTLRegionMake2D(0, 0, 1, 1), mipmapLevel: 0, slice: face, withBytes: white,
                            bytesPerRow: 4, bytesPerImage: 4)
        }
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
