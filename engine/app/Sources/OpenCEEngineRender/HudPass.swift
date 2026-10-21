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
    static let source = """
    #include <metal_stdlib>
    using namespace metal;

    struct HudInstance {
        float4 rect;
        float4 uv;
        float4 color;
        float4 min_color;
        float4 max_color;
        float4 empty_color;
        float4 parameters;
    };

    struct HudVarying {
        float4 position [[position]];
        float2 uv;
        uint instance [[flat]];
    };

    vertex HudVarying hud_vertex(uint vertex_id [[vertex_id]], uint instance [[instance_id]],
                                 const device HudInstance *instances [[buffer(0)]]) {
        // (two triangles: 0 1 2, 2 1 3 of the corners left top, right top,
        // left bottom, right bottom)
        const uint corners[6] = {0, 1, 2, 2, 1, 3};
        uint corner = corners[vertex_id];
        HudInstance quad = instances[instance];
        bool right = (corner & 1) != 0, bottom = (corner & 2) != 0;
        HudVarying out;
        out.position = float4(right ? quad.rect.z : quad.rect.x, bottom ? quad.rect.w : quad.rect.y, 0, 1);
        out.uv = float2(right ? quad.uv.z : quad.uv.x, bottom ? quad.uv.w : quad.uv.y);
        out.instance = instance;
        return out;
    }

    // premultiplied out
    fragment float4 hud_fragment(HudVarying in [[stage_in]], texture2d<float> art [[texture(0)]],
                                 sampler linear [[sampler(0)]],
                                 const device HudInstance *instances [[buffer(0)]]) {
        HudInstance quad = instances[in.instance];
        float4 texel = art.sample(linear, in.uv);
        float threshold = quad.parameters.x;
        float kind = quad.parameters.y;
        if (kind == 2.0 && threshold >= 0.0) {
            float along = texel.b;
            uint2 size = uint2(art.get_width(), art.get_height());
            uint2 pixel = uint2(clamp(floor(in.uv * float2(size)), float2(0), float2(size) - 1));
            float segment = art.read(pixel).r;
            if (segment > 0.0) along = segment;
            float4 colour = along <= threshold ? mix(quad.min_color, quad.max_color, threshold) : quad.empty_color;
            float alpha = colour.a * texel.g * quad.color.a;
            return float4(colour.rgb * alpha, alpha);
        }
        // (the game's "alpha multiply add", one and one less the source's
        // alpha: the colour's alpha darkens what is behind but doesn't fade
        // the colour, so a crosshair of no alpha is added to the frame)
        if (kind == 1.0) {
            return float4(texel.rgb * quad.color.rgb, texel.a * quad.color.a);
        }
        return float4(texel.rgb * texel.a * quad.color.rgb, texel.a * quad.color.a);
    }
    """

    let art: HudArt
    private let pipeline: MTLRenderPipelineState
    private let sampler: MTLSamplerState
    /// (one per frame in flight, grown as needed)
    private var buffers: [MTLBuffer?] = [nil, nil, nil]
    private var frame = 0

    /// The quads drawn in the last frame, and those without a redraw (not
    /// drawn), for tests.
    public private(set) var lastDrawn = 0
    public private(set) var lastMissing: Set<String> = []

    public init(device: MTLDevice, art: HudArt, outputFormat: MTLPixelFormat) throws {
        self.art = art
        do {
            let library = try device.makeLibrary(source: Self.source, options: nil)
            let descriptor = MTLRenderPipelineDescriptor()
            descriptor.vertexFunction = library.makeFunction(name: "hud_vertex")
            descriptor.fragmentFunction = library.makeFunction(name: "hud_fragment")
            let color = descriptor.colorAttachments[0]!
            color.pixelFormat = outputFormat
            color.isBlendingEnabled = true
            color.sourceRGBBlendFactor = .one
            color.destinationRGBBlendFactor = .oneMinusSourceAlpha
            color.sourceAlphaBlendFactor = .one
            color.destinationAlphaBlendFactor = .oneMinusSourceAlpha
            pipeline = try device.makeRenderPipelineState(descriptor: descriptor)
        } catch {
            throw LevelRendererError.shaders("HUD: \(error)")
        }
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
            buffers[frame] = encoder.device.makeBuffer(length: max(length, 16 * 1024), options: .storageModeShared)
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
