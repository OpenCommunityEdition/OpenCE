// The loading screen between missions (Shaders/loading.metal; its timing
// is engine/crates/ui/src/loading.rs's): the band of light and its trail
// are drawn at the game's 640 by 480 into one of two textures, from the
// other, and the result is laid over the target at the screen's alpha.
// The loading picture is not among the maps' files, so the light falls on
// a white field.

import Foundation
import Metal
import OpenCEEngineCore

/// The loading shader's LoadingUniforms.
struct LoadingUniforms {
    var frame: SIMD4<Float>
    var trail: SIMD4<Float>
}

public final class LoadingPass {
    /// The screen's size in the game's pixels.
    public static let width = 640, height = 480
    /// How much larger the trail is drawn each frame (progress_bar.c's 1/128).
    static let zoom: Float = 1.0 / 128
    /// The trail's blur, in texels.
    static let blur: Float = 0.1

    private var trail: MTLRenderPipelineState
    private var light: MTLRenderPipelineState
    private var composite: MTLRenderPipelineState
    private let outputFormat: MTLPixelFormat
    private let linear: MTLSamplerState
    /// (no light outside the band, across)
    private let border: MTLSamplerState
    private let mask: MTLTexture
    private let picture: MTLTexture
    /// The last frame and the next.
    private var screens: [MTLTexture]
    private var current = 0
    /// Whether the last frame is empty (the first of a screen).
    private var fresh = true

    public init(device: MTLDevice, mask pixels: Data, outputFormat: MTLPixelFormat,
                source: String? = nil) throws {
        self.outputFormat = outputFormat
        (trail, light, composite) = try Self.pipelines(device: device, source: source ?? ShaderSources.source(.loading),
                                                       outputFormat: outputFormat)
        func sampler(across: MTLSamplerAddressMode) -> MTLSamplerState? {
            let descriptor = MTLSamplerDescriptor()
            descriptor.minFilter = .linear
            descriptor.magFilter = .linear
            descriptor.sAddressMode = across
            descriptor.tAddressMode = .clampToEdge
            return device.makeSamplerState(descriptor: descriptor)
        }
        guard let linear = sampler(across: .clampToEdge), let border = sampler(across: .clampToZero) else {
            throw LevelRendererError.resources("loading samplers")
        }
        self.linear = linear
        self.border = border
        func texture(_ format: MTLPixelFormat, _ width: Int, _ height: Int, target: Bool) throws -> MTLTexture {
            let descriptor = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: format, width: width,
                                                                      height: height, mipmapped: false)
            descriptor.usage = target ? [.shaderRead, .renderTarget] : .shaderRead
            descriptor.storageMode = .private
            if !target {
                descriptor.storageMode = .shared
            }
            guard let made = device.makeTexture(descriptor: descriptor) else {
                throw LevelRendererError.resources("loading textures")
            }
            return made
        }
        let (maskWidth, maskHeight) = (128, 16)
        let mask = try texture(.r8Unorm, maskWidth, maskHeight, target: false)
        if pixels.count >= maskWidth * maskHeight {
            pixels.withUnsafeBytes { raw in
                if let base = raw.baseAddress {
                    mask.replace(region: MTLRegionMake2D(0, 0, maskWidth, maskHeight), mipmapLevel: 0,
                                 withBytes: base, bytesPerRow: maskWidth)
                }
            }
        }
        self.mask = mask
        let picture = try texture(.rgba8Unorm, 1, 1, target: false)
        let white: [UInt8] = [255, 255, 255, 255]
        white.withUnsafeBytes { raw in
            if let base = raw.baseAddress {
                picture.replace(region: MTLRegionMake2D(0, 0, 1, 1), mipmapLevel: 0, withBytes: base, bytesPerRow: 4)
            }
        }
        self.picture = picture
        screens = try (0..<2).map { _ in try texture(.rgba16Float, Self.width, Self.height, target: true) }
    }

    private static func pipelines(device: MTLDevice, source: String, outputFormat: MTLPixelFormat) throws
        -> (MTLRenderPipelineState, MTLRenderPipelineState, MTLRenderPipelineState) {
        do {
            let library = try device.makeLibrary(source: source, options: nil)
            func make(_ vertex: String, _ fragment: String, format: MTLPixelFormat,
                      blend: (MTLBlendFactor, MTLBlendFactor)?) throws -> MTLRenderPipelineState {
                let descriptor = MTLRenderPipelineDescriptor()
                descriptor.vertexFunction = try library.required(vertex)
                descriptor.fragmentFunction = try library.required(fragment)
                guard let color = descriptor.colorAttachments[0] else { throw LevelRendererError.shaders("attachment") }
                color.pixelFormat = format
                if let (source, destination) = blend {
                    color.isBlendingEnabled = true
                    color.sourceRGBBlendFactor = source
                    color.destinationRGBBlendFactor = destination
                    color.sourceAlphaBlendFactor = source
                    color.destinationAlphaBlendFactor = destination
                }
                return try device.makeRenderPipelineState(descriptor: descriptor)
            }
            return (try make("loading_trail_vertex", "loading_trail", format: .rgba16Float, blend: nil),
                    try make("loading_vertex", "loading_light", format: .rgba16Float, blend: (.one, .one)),
                    try make("loading_vertex", "loading_composite", format: outputFormat,
                             blend: (.one, .oneMinusSourceAlpha)))
        } catch {
            throw LevelRendererError.shaders("loading: \(error)")
        }
    }

    /// Compiles `source` again; the pipelines are replaced only if they build.
    public func reloadShaders(source: String) throws {
        (trail, light, composite) = try Self.pipelines(device: mask.device, source: source, outputFormat: outputFormat)
    }

    /// Starts a new screen, without the last one's trail.
    public func reset() {
        fresh = true
    }

    /// The uniforms of `view`.
    static func uniforms(_ view: LoadingView, fresh: Bool) -> LoadingUniforms {
        LoadingUniforms(frame: SIMD4(view.color, view.maskPosition, fresh ? 0 : view.fadeAlpha, view.screenAlpha),
                        trail: SIMD4(zoom, blur / Float(width), blur / Float(height), 0))
    }

    /// The next frame of the screen `view` describes, laid over `target`
    /// (cleared to black first, with `clear`: nothing is under it).
    public func encode(_ view: LoadingView, into commands: MTLCommandBuffer, target: MTLTexture, clear: Bool) {
        var uniforms = Self.uniforms(view, fresh: fresh)
        let next = 1 - current
        let draw = MTLRenderPassDescriptor()
        draw.colorAttachments[0].texture = screens[next]
        draw.colorAttachments[0].loadAction = .clear
        draw.colorAttachments[0].clearColor = MTLClearColor(red: 0, green: 0, blue: 0, alpha: 1)
        draw.colorAttachments[0].storeAction = .store
        if let encoder = commands.makeRenderCommandEncoder(descriptor: draw) {
            encoder.label = "loading screen"
            encoder.setVertexBytes(&uniforms, length: MemoryLayout<LoadingUniforms>.stride, index: 0)
            encoder.setFragmentBytes(&uniforms, length: MemoryLayout<LoadingUniforms>.stride, index: 0)
            encoder.setFragmentSamplerState(linear, index: 0)
            encoder.setFragmentSamplerState(border, index: 1)
            if !fresh {
                encoder.setRenderPipelineState(trail)
                encoder.setFragmentTexture(screens[current], index: 0)
                encoder.drawPrimitives(type: .triangleStrip, vertexStart: 0, vertexCount: 4)
            }
            encoder.setRenderPipelineState(light)
            encoder.setFragmentTexture(picture, index: 0)
            encoder.setFragmentTexture(mask, index: 1)
            encoder.drawPrimitives(type: .triangleStrip, vertexStart: 0, vertexCount: 4)
            encoder.endEncoding()
        }
        current = next
        fresh = false
        let over = MTLRenderPassDescriptor()
        over.colorAttachments[0].texture = target
        over.colorAttachments[0].loadAction = clear ? .clear : .load
        over.colorAttachments[0].clearColor = MTLClearColor(red: 0, green: 0, blue: 0, alpha: 1)
        over.colorAttachments[0].storeAction = .store
        guard view.screenAlpha > 0 || clear, let encoder = commands.makeRenderCommandEncoder(descriptor: over) else {
            return
        }
        encoder.label = "loading screen over the frame"
        encoder.setRenderPipelineState(composite)
        encoder.setFragmentBytes(&uniforms, length: MemoryLayout<LoadingUniforms>.stride, index: 0)
        encoder.setFragmentSamplerState(linear, index: 0)
        encoder.setFragmentTexture(screens[current], index: 0)
        encoder.drawPrimitives(type: .triangleStrip, vertexStart: 0, vertexCount: 4)
        encoder.endEncoding()
    }
}
