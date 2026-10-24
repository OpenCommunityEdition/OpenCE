// The screen's passes (Shaders/screen.metal): between the level's opaque
// draws and its blended ones, ambient occlusion and the sun's contact
// shadows from the depth and the surfaces' normals, and reflections traced
// through the depth; then a resolve that applies them to the frame, and a
// view that shows one buffer alone. Each can be switched off from the
// shell (`render.gtao`, `render.ssr`, `render.contact_shadows`) and shown
// alone (`render.view`); the occlusion is found at half resolution unless
// `render.ao_quality` is full.

import Metal
import simd

/// What `render.view` shows: the frame, or one buffer alone.
public enum RenderView: String, CaseIterable, Sendable {
    case frame, depth, normals, ao, contact, ssr
    /// The brightness the exposure adapts to, and the bloom (drawn by the
    /// tone map).
    case luminance, bloom
    /// The level's surfaces unlit, their light alone (on white), and lit
    /// by the map's own lightmaps where a bake replaced them (drawn by the
    /// level's shader).
    case albedo, light, stock

    var index: UInt32 { UInt32(Self.allCases.firstIndex(of: self) ?? 0) }
}

/// The passes the shell can switch off, and the buffer it shows.
public struct RenderSwitches: Equatable, Sendable {
    public var gtao = true
    public var ssr = true
    public var contactShadows = true
    public var sunShadows = true
    public var lights = true
    public var exposure = true
    public var bloom = true
    public var lensFlares = true
    public var glows = true
    public var decals = true
    public var particles = true
    /// The occlusion and contact shadows found at half resolution and
    /// brought up to the frame's (`render.ao_quality` half).
    public var halfOcclusion = true
    /// The level and the objects drawn as their triangles' edges.
    public var wireframe = false
    /// The level lit by its lightmaps (off: lit flat, white).
    public var lightmaps = true
    /// The objects drawn (off: the level and the sky alone).
    public var objects = true
    /// The HUD drawn over the frame.
    public var hud = true
    public var view = RenderView.frame

    public init() {}

    /// The shell's `render.*` flags these switches are read from: the ones
    /// that change the frame (the launcher's Developer tab offers these
    /// alone).
    public static let flags = [
        "render.gtao", "render.ssr", "render.contact_shadows", "render.sun_shadows", "render.lights",
        "render.exposure", "render.bloom", "render.lens_flares", "render.glows", "render.decals",
        "render.particles", "render.wireframe", "render.lightmaps", "render.objects", "render.hud",
    ]

    /// The switches as the shell's variables hold them (`render.*`), read
    /// through `flag` and `value`.
    public init(flag: (String) -> Bool, value: (String) -> String) {
        gtao = flag("render.gtao")
        ssr = flag("render.ssr")
        contactShadows = flag("render.contact_shadows")
        sunShadows = flag("render.sun_shadows")
        lights = flag("render.lights")
        exposure = flag("render.exposure")
        bloom = flag("render.bloom")
        lensFlares = flag("render.lens_flares")
        glows = flag("render.glows")
        decals = flag("render.decals")
        particles = flag("render.particles")
        halfOcclusion = value("render.ao_quality") != "full"
        wireframe = flag("render.wireframe")
        lightmaps = flag("render.lightmaps")
        objects = flag("render.objects")
        hud = flag("render.hud")
        view = RenderView(rawValue: value("render.view")) ?? .frame
    }
}

/// (as screen.metal's Screen)
struct ScreenUniforms {
    var clipToWorld: simd_float4x4
    var worldToClip: simd_float4x4
    var eye: SIMD4<Float>
    var sun: SIMD4<Float>
    var size: SIMD4<Float>
    var passes: SIMD4<UInt32>
    /// The occlusion's resolution (0 full, 1 half).
    var quality: SIMD4<UInt32>

    init(viewProjection: simd_float4x4, eye: SIMD3<Float>, sunTowards: SIMD4<Float>, width: Int, height: Int,
         switches: RenderSwitches) {
        clipToWorld = viewProjection.inverse
        worldToClip = viewProjection
        self.eye = SIMD4(eye, ViewCamera.near)
        sun = sunTowards
        // (the pixels a world unit spans one unit ahead: the projection's
        // vertical scale over half the height)
        let up = SIMD3(viewProjection.columns.0.y, viewProjection.columns.1.y, viewProjection.columns.2.y)
        size = SIMD4(Float(width), Float(height), 0, length(up) * Float(height) / 2)
        passes = SIMD4(switches.gtao ? 1 : 0, switches.contactShadows ? 1 : 0, switches.ssr ? 1 : 0,
                       switches.view.index)
        quality = SIMD4(switches.halfOcclusion ? 1 : 0, 0, 0, 0)
    }
}

final class ScreenPass {
    /// The opaque passes' extra attachments (level.metal's Opaque): the
    /// surface, the reflection it added and the reflection's weight.
    static let attachmentFormats: [MTLPixelFormat] = [.rgba16Float, .rgba16Float, .rgba16Float]

    /// The textures the passes read and write, for one size of frame.
    struct Targets {
        var surface: MTLTexture
        var reflection: MTLTexture
        var reflectance: MTLTexture
        /// The occlusion (r), the contact shadow (g) and the depth's shade (b).
        var occlusion: MTLTexture
        /// The occlusion at half resolution: as `occlusion`, with the
        /// distance ahead (b) and whether the texel was screened (a).
        var halfOcclusion: MTLTexture
        /// The traced reflection (rgb) and its trust (a).
        var reflected: MTLTexture
        var attachments: [MTLTexture] { [surface, reflection, reflectance] }
    }

    struct Pipelines {
        var occlusion: MTLComputePipelineState
        var halfOcclusion: MTLComputePipelineState
        var reflection: MTLComputePipelineState
        var resolve: MTLRenderPipelineState
        var view: MTLRenderPipelineState
    }

    let device: MTLDevice
    private var pipelines: Pipelines
    private var targets: Targets?
    private let always: MTLDepthStencilState

    init(device: MTLDevice, source: String) throws {
        self.device = device
        pipelines = try Self.pipelines(device: device, source: source)
        let descriptor = MTLDepthStencilDescriptor()
        descriptor.depthCompareFunction = .always
        descriptor.isDepthWriteEnabled = false
        guard let always = device.makeDepthStencilState(descriptor: descriptor) else {
            throw LevelRendererError.resources("screen depth state")
        }
        self.always = always
    }

    private static func pipelines(device: MTLDevice, source: String) throws -> Pipelines {
        do {
            let library = try device.makeLibrary(source: source, options: nil)
            func full(_ fragment: String) throws -> MTLRenderPipelineState {
                let descriptor = MTLRenderPipelineDescriptor()
                descriptor.vertexFunction = try library.required("screen_vertex")
                descriptor.fragmentFunction = try library.required(fragment)
                descriptor.colorAttachments[0].pixelFormat = LevelRenderer.frameFormat
                descriptor.depthAttachmentPixelFormat = LevelRenderer.depthFormat
                return try device.makeRenderPipelineState(descriptor: descriptor)
            }
            return Pipelines(
                occlusion: try device.makeComputePipelineState(function: try library.required("screen_occlusion")),
                halfOcclusion: try device.makeComputePipelineState(
                    function: try library.required("screen_occlusion_half")),
                reflection: try device.makeComputePipelineState(function: try library.required("screen_reflection")),
                resolve: try full("screen_resolve"), view: try full("screen_view"))
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

    /// Declares the opaque passes' extra attachments on `descriptor`,
    /// written or (for the sky's) left alone.
    static func attachments(_ descriptor: MTLRenderPipelineDescriptor, written: Bool) {
        for (offset, format) in attachmentFormats.enumerated() {
            descriptor.colorAttachments[offset + 1].pixelFormat = format
            descriptor.colorAttachments[offset + 1].writeMask = written ? .all : []
        }
    }

    /// The targets for a frame `width` by `height`, made again when the
    /// size changes.
    func targets(width: Int, height: Int) -> Targets? {
        if let targets, targets.surface.width == width, targets.surface.height == height {
            return targets
        }
        guard width > 0, height > 0 else { return nil }
        func texture(_ format: MTLPixelFormat, _ usage: MTLTextureUsage, halved: Bool = false) -> MTLTexture? {
            let descriptor = MTLTextureDescriptor.texture2DDescriptor(
                pixelFormat: format, width: halved ? (width + 1) / 2 : width,
                height: halved ? (height + 1) / 2 : height, mipmapped: false)
            descriptor.usage = usage
            descriptor.storageMode = .private
            return HitchLog.shared.note(.allocation, "screen targets \(width)×\(height)") {
                device.makeTexture(descriptor: descriptor)
            }
        }
        let attached: MTLTextureUsage = [.renderTarget, .shaderRead]
        guard let surface = texture(Self.attachmentFormats[0], attached),
              let reflection = texture(Self.attachmentFormats[1], attached),
              let reflectance = texture(Self.attachmentFormats[2], attached),
              let occlusion = texture(.rgba16Float, [.shaderRead, .shaderWrite]),
              let halfOcclusion = texture(.rgba16Float, [.shaderRead, .shaderWrite], halved: true),
              let reflected = texture(.rgba16Float, [.shaderRead, .shaderWrite]) else { return nil }
        targets = Targets(surface: surface, reflection: reflection, reflectance: reflectance, occlusion: occlusion,
                          halfOcclusion: halfOcclusion, reflected: reflected)
        return targets
    }

    /// The occlusion and the traced reflections, from the opaque frame and
    /// its depth (each pass only when it is on or shown).
    func encode(into commands: MTLCommandBuffer, targets: Targets, frame: MTLTexture, depth: MTLTexture,
                uniforms: ScreenUniforms) {
        let occlusionNeeded = uniforms.passes.x != 0 || uniforms.passes.y != 0
            || [RenderView.depth, .ao, .contact].map(\.index).contains(uniforms.passes.w)
        let reflectionNeeded = uniforms.passes.z != 0 || uniforms.passes.w == RenderView.ssr.index
        guard occlusionNeeded || reflectionNeeded,
              let encoder = commands.timedComputeEncoder("screen") else { return }
        let threads = MTLSize(width: 8, height: 8, depth: 1)
        let groups = MTLSize(width: (frame.width + 7) / 8, height: (frame.height + 7) / 8, depth: 1)
        if occlusionNeeded {
            // (shown alone, the passes are computed even when switched off)
            var computed = uniforms
            if uniforms.passes.w == RenderView.ao.index { computed.passes.x = 1 }
            if uniforms.passes.w == RenderView.contact.index { computed.passes.y = 1 }
            encoder.setTexture(depth, index: 0)
            encoder.setTexture(targets.surface, index: 1)
            encoder.setBytes(&computed, length: MemoryLayout<ScreenUniforms>.stride, index: 0)
            if computed.quality.x != 0 {
                // (found at half resolution first, then brought up)
                let half = targets.halfOcclusion
                encoder.setComputePipelineState(pipelines.halfOcclusion)
                encoder.setTexture(half, index: 2)
                encoder.dispatchThreadgroups(MTLSize(width: (half.width + 7) / 8, height: (half.height + 7) / 8,
                                                     depth: 1), threadsPerThreadgroup: threads)
                encoder.memoryBarrier(resources: [half])
            }
            encoder.setComputePipelineState(pipelines.occlusion)
            encoder.setTexture(targets.occlusion, index: 2)
            encoder.setTexture(targets.halfOcclusion, index: 3)
            encoder.dispatchThreadgroups(groups, threadsPerThreadgroup: threads)
        }
        if reflectionNeeded {
            var computed = uniforms
            computed.passes.z = 1
            encoder.setComputePipelineState(pipelines.reflection)
            encoder.setTexture(depth, index: 0)
            encoder.setTexture(targets.surface, index: 1)
            encoder.setTexture(targets.reflectance, index: 2)
            encoder.setTexture(frame, index: 3)
            encoder.setTexture(targets.reflected, index: 4)
            encoder.setBytes(&computed, length: MemoryLayout<ScreenUniforms>.stride, index: 0)
            encoder.dispatchThreadgroups(groups, threadsPerThreadgroup: threads)
        }
        encoder.endEncoding()
    }

    private func bind(_ targets: Targets, _ uniforms: inout ScreenUniforms, into encoder: MTLRenderCommandEncoder) {
        encoder.setFragmentTexture(targets.surface, index: 0)
        encoder.setFragmentTexture(targets.reflection, index: 1)
        encoder.setFragmentTexture(targets.reflectance, index: 2)
        encoder.setFragmentTexture(targets.occlusion, index: 3)
        encoder.setFragmentTexture(targets.reflected, index: 4)
        encoder.setFragmentBytes(&uniforms, length: MemoryLayout<ScreenUniforms>.stride, index: 0)
        encoder.setDepthStencilState(always)
    }

    /// Applies the passes that are on to the frame `encoder` draws into
    /// (the blended pass's first draw).
    func resolve(_ targets: Targets, uniforms: ScreenUniforms, into encoder: MTLRenderCommandEncoder) {
        guard uniforms.passes.x != 0 || uniforms.passes.y != 0 || uniforms.passes.z != 0 else { return }
        var uniforms = uniforms
        encoder.setRenderPipelineState(pipelines.resolve)
        bind(targets, &uniforms, into: encoder)
        encoder.drawPrimitives(type: .triangle, vertexStart: 0, vertexCount: 3)
    }

    /// Draws the buffer `render.view` asks for over the frame (its last
    /// draw), if it is not the frame.
    func view(_ targets: Targets, uniforms: ScreenUniforms, into encoder: MTLRenderCommandEncoder) {
        guard uniforms.passes.w != RenderView.frame.index, uniforms.passes.w <= RenderView.ssr.index else { return }
        var uniforms = uniforms
        encoder.setRenderPipelineState(pipelines.view)
        bind(targets, &uniforms, into: encoder)
        encoder.drawPrimitives(type: .triangle, vertexStart: 0, vertexCount: 3)
    }
}
