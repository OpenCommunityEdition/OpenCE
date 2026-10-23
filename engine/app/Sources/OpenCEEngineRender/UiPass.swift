// The Halo UI over the tone-mapped frame: the menu's quads (engine/crates
// /ffi/src/ui.rs), its widgets' bitmaps from the level's textures (ui.map's,
// whose structure is the scene behind the menus) and its text from one
// atlas of the fonts' signed distance fields.

import Foundation
import Metal
import OpenCEEngineCore

/// The UI shader's UiInstance.
struct UiInstance {
    /// Left, top, right and bottom, in clip space.
    var rect: SIMD4<Float>
    /// u0, v0, u1, v1.
    var uv: SIMD4<Float>
    var color: SIMD4<Float>
    /// 1 for a glyph.
    var parameters: SIMD4<Float>
}

public final class UiPass {
    private var pipeline: MTLRenderPipelineState
    private let outputFormat: MTLPixelFormat
    /// (the widgets' backgrounds tile: their uv go past 1)
    private let wrap: MTLSamplerState
    private let clamp: MTLSamplerState
    /// The glyphs' distance fields (R8).
    let atlas: MTLTexture
    /// (one per frame in flight, grown as needed)
    private var buffers: [MTLBuffer?] = [nil, nil, nil]
    private var frame = 0

    /// The quads drawn in the last frame, and those whose texture is not
    /// the level's (not drawn), for tests.
    public private(set) var lastDrawn = 0
    public private(set) var lastMissing = 0

    public init(device: MTLDevice, atlas glyphs: UiGlyphAtlas, outputFormat: MTLPixelFormat,
                source: String? = nil) throws {
        self.outputFormat = outputFormat
        pipeline = try Self.pipeline(device: device, source: source ?? ShaderSources.source(.ui),
                                     outputFormat: outputFormat)
        func sampler(_ address: MTLSamplerAddressMode) -> MTLSamplerState? {
            let descriptor = MTLSamplerDescriptor()
            descriptor.minFilter = .linear
            descriptor.magFilter = .linear
            descriptor.mipFilter = .linear
            descriptor.sAddressMode = address
            descriptor.tAddressMode = address
            return device.makeSamplerState(descriptor: descriptor)
        }
        guard let wrap = sampler(.repeat), let clamp = sampler(.clampToEdge) else {
            throw LevelRendererError.resources("UI samplers")
        }
        self.wrap = wrap
        self.clamp = clamp
        let width = max(Int(glyphs.width), 1), height = max(Int(glyphs.height), 1)
        let descriptor = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: .r8Unorm, width: width,
                                                                  height: height, mipmapped: false)
        descriptor.usage = .shaderRead
        guard let atlas = device.makeTexture(descriptor: descriptor) else {
            throw LevelRendererError.resources("UI glyph atlas")
        }
        if glyphs.texels.count >= width * height {
            glyphs.texels.withUnsafeBytes { raw in
                if let base = raw.baseAddress {
                    atlas.replace(region: MTLRegionMake2D(0, 0, width, height), mipmapLevel: 0, withBytes: base,
                                  bytesPerRow: width)
                }
            }
        }
        self.atlas = atlas
    }

    private static func pipeline(device: MTLDevice, source: String,
                                 outputFormat: MTLPixelFormat) throws -> MTLRenderPipelineState {
        do {
            let library = try device.makeLibrary(source: source, options: nil)
            let descriptor = MTLRenderPipelineDescriptor()
            descriptor.vertexFunction = try library.required("ui_vertex")
            descriptor.fragmentFunction = try library.required("ui_fragment")
            guard let color = descriptor.colorAttachments[0] else { throw LevelRendererError.shaders("attachment") }
            color.pixelFormat = outputFormat
            color.isBlendingEnabled = true
            color.sourceRGBBlendFactor = .one
            color.destinationRGBBlendFactor = .oneMinusSourceAlpha
            color.sourceAlphaBlendFactor = .one
            color.destinationAlphaBlendFactor = .oneMinusSourceAlpha
            return try device.makeRenderPipelineState(descriptor: descriptor)
        } catch {
            throw LevelRendererError.shaders("UI: \(error)")
        }
    }

    /// Compiles `source` again; the pipeline is replaced only if it builds.
    public func reloadShaders(source: String) throws {
        pipeline = try Self.pipeline(device: pipeline.device, source: source, outputFormat: outputFormat)
    }

    /// `quads` over a target `width` by `height`, in order, their bitmaps
    /// from `textures` (the level's, by their index).
    func encode(_ quads: [UiQuad], textures: [MTLTexture?], width: Int, height: Int,
                into encoder: MTLRenderCommandEncoder) {
        lastDrawn = 0
        lastMissing = 0
        guard !quads.isEmpty, width > 0, height > 0 else { return }
        var instances: [UiInstance] = []
        var runs: [(texture: MTLTexture, first: Int, count: Int)] = []
        instances.reserveCapacity(quads.count)
        let x = { (pixels: Float) in pixels / Float(width) * 2 - 1 }
        let y = { (pixels: Float) in 1 - pixels / Float(height) * 2 }
        for quad in quads {
            let texture: MTLTexture
            if quad.glyph {
                texture = atlas
            } else if Int(quad.texture) < textures.count, let found = textures[Int(quad.texture)],
                      found.textureType == .type2D {
                texture = found
            } else {
                lastMissing += 1
                continue
            }
            instances.append(UiInstance(
                rect: SIMD4(x(quad.left), y(quad.top), x(quad.right), y(quad.bottom)),
                uv: SIMD4(quad.u0, quad.v0, quad.u1, quad.v1), color: HudPass.color(quad.color),
                parameters: SIMD4(quad.glyph ? 1 : 0, 0, 0, 0)))
            if let last = runs.last, last.texture === texture {
                runs[runs.count - 1].count += 1
            } else {
                runs.append((texture, instances.count - 1, 1))
            }
        }
        guard !instances.isEmpty else { return }
        frame = (frame + 1) % buffers.count
        let length = instances.count * MemoryLayout<UiInstance>.stride
        if (buffers[frame]?.length ?? 0) < length {
            buffers[frame] = encoder.device.makeBuffer(length: max(length, 16 * 1024), options: .storageModeShared)
        }
        guard let buffer = buffers[frame] else { return }
        instances.withUnsafeBytes { raw in
            if let base = raw.baseAddress { buffer.contents().copyMemory(from: base, byteCount: raw.count) }
        }
        encoder.setRenderPipelineState(pipeline)
        encoder.setVertexBuffer(buffer, offset: 0, index: 0)
        encoder.setFragmentBuffer(buffer, offset: 0, index: 0)
        for run in runs {
            encoder.setFragmentSamplerState(run.texture === atlas ? clamp : wrap, index: 0)
            encoder.setFragmentTexture(run.texture, index: 0)
            encoder.drawPrimitives(type: .triangle, vertexStart: 0, vertexCount: 6, instanceCount: run.count,
                                   baseInstance: run.first)
        }
        lastDrawn = instances.count
    }
}
