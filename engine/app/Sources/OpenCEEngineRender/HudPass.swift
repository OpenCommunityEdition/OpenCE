// The HUD over the tone-mapped frame: the engine's quads (engine/crates/ffi
// /src/hud.rs) drawn from the hand-drawn redraws of the HUD's bitmaps
// (port/assets/hud, made by tools/hud_assets.py), each the whole of its
// bitmap's image at 8x (4x for the largest), so that a sprite's texture
// coordinates are the same in it as in the map's.
//
// The redraws keep the Xbox's texels' meaning (port/linux/src/hud_hires.h):
// an alpha bitmap's value is in every channel (drawn premultiplied, tinted);
// a meter's blue is how far along the meter each texel is, its green how
// much of the texel its shapes cover, and its red (read without filtering)
// a discrete segment's exact threshold, or zero in a continuous meter.

import Foundation
import ImageIO
import Metal
import OpenCEEngineCore

/// How a redraw's texels are read.
enum HudTextureKind: Float {
    /// The colour and its opacity, tinted.
    case straight = 0
    /// One value in every channel (premultiplied), tinted.
    case premultiplied = 1
    /// A meter's sheet.
    case meter = 2

    init(cells: [String]) {
        if cells.contains("meter") {
            self = .meter
        } else if cells.allSatisfy({ $0 == "alpha" }) {
            self = .premultiplied
        } else {
            self = .straight
        }
    }
}

public enum HudArtError: Error {
    case layout(String)
}

/// The HUD's redraws, by their bitmap's tag path and image.
public struct HudArt {
    struct Entry {
        var texture: MTLTexture
        var kind: HudTextureKind
    }

    var entries: [String: Entry]

    /// The redraws' count.
    public var count: Int { entries.count }

    static func key(_ tag: String, _ image: UInt32) -> String {
        "\(tag.lowercased().replacingOccurrences(of: "/", with: "\\"))#\(image)"
    }

    /// Whether a bitmap's image has a redraw.
    public func has(tag: String, image: UInt32) -> Bool { entries[Self.key(tag, image)] != nil }

    /// The redraws in `folder` (its layout.json and a PNG for each).
    public static func load(folder: URL, device: MTLDevice, queue: MTLCommandQueue) throws -> HudArt {
        struct Layout: Decodable {
            struct Asset: Decodable {
                struct Cell: Decodable { var kind: String }
                var name: String
                var tag: String
                var bitmap: UInt32
                var cells: [Cell]
            }
            var assets: [Asset]
        }
        let data = try Data(contentsOf: folder.appendingPathComponent("layout.json"))
        let layout = try JSONDecoder().decode(Layout.self, from: data)
        var entries: [String: Entry] = [:]
        var made: [MTLTexture] = []
        for asset in layout.assets {
            let url = folder.appendingPathComponent("\(asset.name).png")
            guard let texture = Self.texture(url, device: device) else {
                throw HudArtError.layout("\(asset.name).png could not be read")
            }
            entries[key(asset.tag, asset.bitmap)] = Entry(texture: texture,
                                                          kind: HudTextureKind(cells: asset.cells.map(\.kind)))
            made.append(texture)
        }
        // (the redraws are 8x the size they are drawn at in a 1080p window)
        if let commands = queue.makeCommandBuffer(), let blit = commands.makeBlitCommandEncoder() {
            for texture in made where texture.mipmapLevelCount > 1 {
                blit.generateMipmaps(for: texture)
            }
            blit.endEncoding()
            commands.commit()
            commands.waitUntilCompleted()
        }
        return HudArt(entries: entries)
    }

    /// A PNG's texels as they are in the file: unpremultiplied (a meter's
    /// channels are not colours, so they must not be multiplied by its
    /// alpha as Core Graphics would draw them).
    static func texture(_ url: URL, device: MTLDevice) -> MTLTexture? {
        guard let source = CGImageSourceCreateWithURL(url as CFURL, nil),
              let image = CGImageSourceCreateImageAtIndex(source, 0, nil), image.bitsPerComponent == 8,
              let pixels = image.dataProvider?.data as Data? else { return nil }
        let width = image.width, height = image.height
        let channels = image.bitsPerPixel / 8
        let alphaFirst = [.first, .premultipliedFirst, .noneSkipFirst].contains(image.alphaInfo)
        guard (1...4).contains(channels), image.bytesPerRow >= width * channels,
              pixels.count >= image.bytesPerRow * height else { return nil }
        var rgba = [UInt8](repeating: 255, count: width * height * 4)
        pixels.withUnsafeBytes { (raw: UnsafeRawBufferPointer) in
            for y in 0..<height {
                for x in 0..<width {
                    let from = y * image.bytesPerRow + x * channels
                    let to = (y * width + x) * 4
                    switch channels {
                    case 1:
                        rgba[to] = raw[from]; rgba[to + 1] = raw[from]; rgba[to + 2] = raw[from]
                    case 2:
                        rgba[to] = raw[from]; rgba[to + 1] = raw[from]; rgba[to + 2] = raw[from]
                        rgba[to + 3] = raw[from + 1]
                    case 3:
                        rgba[to] = raw[from]; rgba[to + 1] = raw[from + 1]; rgba[to + 2] = raw[from + 2]
                    default:
                        let colour = alphaFirst ? from + 1 : from
                        rgba[to] = raw[colour]; rgba[to + 1] = raw[colour + 1]; rgba[to + 2] = raw[colour + 2]
                        rgba[to + 3] = raw[alphaFirst ? from : from + 3]
                    }
                }
            }
        }
        let descriptor = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: .rgba8Unorm, width: width,
                                                                  height: height, mipmapped: true)
        descriptor.usage = .shaderRead
        guard let texture = device.makeTexture(descriptor: descriptor) else { return nil }
        texture.replace(region: MTLRegionMake2D(0, 0, width, height), mipmapLevel: 0, withBytes: rgba,
                        bytesPerRow: width * 4)
        return texture
    }
}

/// The HUD shader's HudInstance.
struct HudInstance {
    /// Left, top, right and bottom, in clip space.
    var rect: SIMD4<Float>
    /// u0, v0, u1, v1.
    var uv: SIMD4<Float>
    var color: SIMD4<Float>
    var minColor: SIMD4<Float>
    var maxColor: SIMD4<Float>
    var emptyColor: SIMD4<Float>
    /// The meter's threshold (or -1: not a meter), and the texture's kind.
    var parameters: SIMD4<Float>
}

public final class HudPass {
    let art: HudArt
    private var pipeline: MTLRenderPipelineState
    private let outputFormat: MTLPixelFormat
    private let sampler: MTLSamplerState
    /// (one per frame in flight, grown as needed)
    private var buffers: [MTLBuffer?] = [nil, nil, nil]
    private var frame = 0

    /// The quads drawn in the last frame, and those without a redraw (not
    /// drawn), for tests.
    public private(set) var lastDrawn = 0
    public private(set) var lastMissing: Set<String> = []

    public init(device: MTLDevice, art: HudArt, outputFormat: MTLPixelFormat,
                source: String? = nil) throws {
        self.art = art
        self.outputFormat = outputFormat
        pipeline = try Self.pipeline(device: device, source: source ?? ShaderSources.source(.hud),
                                     outputFormat: outputFormat)
        let samplerDescriptor = MTLSamplerDescriptor()
        samplerDescriptor.minFilter = .linear
        samplerDescriptor.magFilter = .linear
        samplerDescriptor.mipFilter = .linear
        samplerDescriptor.sAddressMode = .clampToEdge
        samplerDescriptor.tAddressMode = .clampToEdge
        guard let sampler = device.makeSamplerState(descriptor: samplerDescriptor) else {
            throw LevelRendererError.resources("HUD sampler")
        }
        self.sampler = sampler
    }

    private static func pipeline(device: MTLDevice, source: String,
                                 outputFormat: MTLPixelFormat) throws -> MTLRenderPipelineState {
        do {
            let library = try device.makeLibrary(source: source, options: nil)
            let descriptor = MTLRenderPipelineDescriptor()
            descriptor.vertexFunction = try library.required("hud_vertex")
            descriptor.fragmentFunction = try library.required("hud_fragment")
            guard let color = descriptor.colorAttachments[0] else { throw LevelRendererError.shaders("attachment") }
            color.pixelFormat = outputFormat
            color.isBlendingEnabled = true
            color.sourceRGBBlendFactor = .one
            color.destinationRGBBlendFactor = .oneMinusSourceAlpha
            color.sourceAlphaBlendFactor = .one
            color.destinationAlphaBlendFactor = .oneMinusSourceAlpha
            return try device.makeRenderPipelineState(descriptor: descriptor)
        } catch {
            throw LevelRendererError.shaders("HUD: \(error)")
        }
    }

    /// Compiles `source` again; the pipeline is replaced only if it builds.
    public func reloadShaders(source: String) throws {
        pipeline = try Self.pipeline(device: pipeline.device, source: source, outputFormat: outputFormat)
    }

    static func color(_ argb: UInt32) -> SIMD4<Float> {
        SIMD4(Float((argb >> 16) & 0xFF), Float((argb >> 8) & 0xFF), Float(argb & 0xFF), Float(argb >> 24)) / 255
    }

    /// `quads` over a target `width` by `height`, in order.
    func encode(_ quads: [HudQuad], width: Int, height: Int, into encoder: MTLRenderCommandEncoder) {
        lastDrawn = 0
        lastMissing = []
        guard !quads.isEmpty, width > 0, height > 0 else { return }
        var instances: [HudInstance] = []
        var runs: [(texture: MTLTexture, first: Int, count: Int)] = []
        instances.reserveCapacity(quads.count)
        for quad in quads {
            guard let entry = art.entries[HudArt.key(quad.bitmap, quad.image)] else {
                lastMissing.insert(HudArt.key(quad.bitmap, quad.image))
                continue
            }
            let x = { (pixels: Float) in pixels / Float(width) * 2 - 1 }
            let y = { (pixels: Float) in 1 - pixels / Float(height) * 2 }
            let meter = quad.meter
            instances.append(HudInstance(
                rect: SIMD4(x(quad.left), y(quad.top), x(quad.right), y(quad.bottom)),
                uv: SIMD4(quad.u0, quad.v0, quad.u1, quad.v1), color: Self.color(quad.color),
                minColor: Self.color(meter?.minColor ?? 0), maxColor: Self.color(meter?.maxColor ?? 0),
                emptyColor: Self.color(meter?.emptyColor ?? 0),
                parameters: SIMD4(meter.map { min(max($0.threshold, 0), 1) } ?? -1, entry.kind.rawValue, 0, 0)))
            if let last = runs.last, last.texture === entry.texture {
                runs[runs.count - 1].count += 1
            } else {
                runs.append((entry.texture, instances.count - 1, 1))
            }
        }
        guard !instances.isEmpty else { return }
        frame = (frame + 1) % buffers.count
        let length = instances.count * MemoryLayout<HudInstance>.stride
        if (buffers[frame]?.length ?? 0) < length {
            buffers[frame] = HitchLog.shared.note(.allocation, "HUD quads \(length) bytes") {
                encoder.device.makeBuffer(length: max(length, 16 * 1024), options: .storageModeShared)
            }
        }
        guard let buffer = buffers[frame] else { return }
        instances.withUnsafeBytes { raw in
            if let base = raw.baseAddress { buffer.contents().copyMemory(from: base, byteCount: raw.count) }
        }
        encoder.setRenderPipelineState(pipeline)
        encoder.setVertexBuffer(buffer, offset: 0, index: 0)
        encoder.setFragmentBuffer(buffer, offset: 0, index: 0)
        encoder.setFragmentSamplerState(sampler, index: 0)
        for run in runs {
            encoder.setFragmentTexture(run.texture, index: 0)
            encoder.drawPrimitives(type: .triangle, vertexStart: 0, vertexCount: 6, instanceCount: run.count,
                                   baseInstance: run.first)
        }
        lastDrawn = instances.count
    }
}
