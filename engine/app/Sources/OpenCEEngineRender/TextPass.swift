// Monospace text for the developer shell, its watch line and the frame
// times: Overpass Mono (port/assets/fonts) drawn by CoreText into a glyph
// atlas at the display's scale, one cell a character, and a pass drawing
// characters and boxes over a finished frame, pixel-aligned and sampled
// nearest, so the text is as sharp as the display.
import CoreGraphics
import CoreText
import Foundation
import Metal

public enum TextError: Error, CustomStringConvertible {
    case font(String)
    case resources(String)

    public var description: String {
        switch self {
        case let .font(text): return "the shell's font: \(text)"
        case let .resources(text): return "the text pass: \(text)"
        }
    }
}

/// Overpass Mono's cells, rasterised at a point size and a scale.
public final class GlyphAtlas {
    /// The font's file.
    public static let file = "OverpassMono-Regular.ttf"

    /// The font's file in the bundle's Resources/fonts or, run from the
    /// package, the repository's port/assets/fonts.
    public static func url(bundle: Bundle = .main) -> URL? {
        let bundled = bundle.resourceURL?.appendingPathComponent("fonts/\(file)")
        var repository = URL(fileURLWithPath: #filePath)
        for _ in 0..<5 {
            repository.deleteLastPathComponent()
        }
        repository.appendPathComponent("port/assets/fonts/\(file)")
        return [bundled, repository].compactMap { $0 }.first { FileManager.default.fileExists(atPath: $0.path) }
    }

    /// The characters the atlas has, beyond printable ASCII (each only if
    /// the font draws it).
    static let extras: [Unicode.Scalar] = ["·", "°", "…", "←", "→", "↑", "↓", "│", "─", "▸", "▾"]

    public let texture: MTLTexture
    /// A cell's size in pixels: the advance by the line's height.
    public let cellWidth: Int
    public let cellHeight: Int
    /// From a cell's top to its baseline, in pixels.
    public let ascent: Int
    public let scale: Double
    public let pointSize: Double
    /// The atlas's coverage, a byte a pixel, rows top first (for tests).
    public let coverage: [UInt8]
    let columns: Int
    /// Each character's cell (0 is solid, for boxes).
    let cells: [Unicode.Scalar: Int]

    /// Draws the atlas of the font at `url` at `pointSize` points on a
    /// display of `scale` pixels a point.
    public init(url: URL, pointSize: Double = 13, scale: Double, device: MTLDevice) throws {
        guard let descriptors = CTFontManagerCreateFontDescriptorsFromURL(url as CFURL) as? [CTFontDescriptor],
            let descriptor = descriptors.first
        else { throw TextError.font("no font in \(url.path)") }
        guard scale > 0, scale <= 8, pointSize > 0, pointSize <= 96 else {
            throw TextError.font("size \(pointSize) at scale \(scale)")
        }
        let font = CTFontCreateWithFontDescriptor(descriptor, pointSize * scale, nil)
        self.scale = scale
        self.pointSize = pointSize

        var scalars: [Unicode.Scalar] = (0x20...0x7E).compactMap { Unicode.Scalar($0) } + Self.extras
        var glyphs: [CGGlyph] = []
        var kept: [Unicode.Scalar] = []
        for scalar in scalars {
            var units = Array(String(scalar).utf16)
            var glyph = [CGGlyph](repeating: 0, count: units.count)
            if CTFontGetGlyphsForCharacters(font, &units, &glyph, units.count), units.count == 1 {
                glyphs.append(glyph[0])
                kept.append(scalar)
            }
        }
        scalars = kept
        guard scalars.contains("M") else { throw TextError.font("no glyphs") }

        var advance = CGSize.zero
        var mGlyph = glyphs[scalars.firstIndex(of: "M") ?? 0]
        CTFontGetAdvancesForGlyphs(font, .horizontal, &mGlyph, &advance, 1)
        let ascentPixels = CTFontGetAscent(font), descentPixels = CTFontGetDescent(font)
        let cellWidth = max(1, Int(advance.width.rounded()))
        let ascent = Int(ascentPixels.rounded(.up))
        let cellHeight = ascent + Int((descentPixels + CTFontGetLeading(font)).rounded(.up)) + 1
        let columns = 16
        self.cellWidth = cellWidth
        self.ascent = ascent
        self.cellHeight = cellHeight
        self.columns = columns
        let rows = (scalars.count + 1 + columns - 1) / columns
        let width = columns * cellWidth, height = rows * cellHeight

        var bytes = [UInt8](repeating: 0, count: width * height)
        try bytes.withUnsafeMutableBytes { raw in
            guard
                let context = CGContext(
                    data: raw.baseAddress, width: width, height: height, bitsPerComponent: 8,
                    bytesPerRow: width, space: CGColorSpaceCreateDeviceGray(),
                    bitmapInfo: CGImageAlphaInfo.none.rawValue)
            else { throw TextError.resources("a bitmap of \(width)x\(height)") }
            context.setAllowsFontSmoothing(false)
            context.setShouldAntialias(true)
            // (cell 0: solid, for boxes and the cursor)
            context.setFillColor(gray: 1, alpha: 1)
            context.fill(CGRect(x: 0, y: height - cellHeight, width: cellWidth, height: cellHeight))
            for (index, glyph) in glyphs.enumerated() {
                let cell = index + 1
                let x = CGFloat(cell % columns * cellWidth)
                // (a bitmap context's first row is its top, y up from the bottom)
                let baseline = CGFloat(height - cell / columns * cellHeight - ascent)
                var glyph = glyph
                var position = CGPoint(x: x, y: baseline)
                CTFontDrawGlyphs(font, &glyph, &position, 1, context)
            }
        }
        coverage = bytes
        var cells: [Unicode.Scalar: Int] = [:]
        for (index, scalar) in scalars.enumerated() {
            cells[scalar] = index + 1
        }
        self.cells = cells

        let textureDescriptor = MTLTextureDescriptor.texture2DDescriptor(
            pixelFormat: .r8Unorm, width: width,
            height: height, mipmapped: false)
        textureDescriptor.usage = .shaderRead
        guard let texture = device.makeTexture(descriptor: textureDescriptor) else {
            throw TextError.resources("the atlas's texture")
        }
        bytes.withUnsafeBytes { raw in
            if let base = raw.baseAddress {
                texture.replace(
                    region: MTLRegionMake2D(0, 0, width, height), mipmapLevel: 0, withBytes: base,
                    bytesPerRow: width)
            }
        }
        self.texture = texture
    }

    /// Whether the atlas draws `scalar`.
    public func has(_ scalar: Unicode.Scalar) -> Bool { cells[scalar] != nil }

    /// The cell drawing `scalar` (a missing character draws as `?`).
    func cell(_ scalar: Unicode.Scalar) -> Int { cells[scalar] ?? cells["?"] ?? 0 }

    /// The solid cell.
    static let solid = 0

    /// The coverage of `scalar`'s cell: its pixels that are drawn.
    public func inkedPixels(_ scalar: Unicode.Scalar) -> Int {
        guard let cell = cells[scalar] else { return 0 }
        let width = columns * cellWidth
        let left = cell % columns * cellWidth, top = cell / columns * cellHeight
        var count = 0
        for y in top..<(top + cellHeight) {
            for x in left..<(left + cellWidth) where coverage[y * width + x] > 0 {
                count += 1
            }
        }
        return count
    }

    /// `cell`'s texture coordinates: left, top, right, bottom.
    func uv(_ cell: Int) -> SIMD4<Float> {
        let width = Float(texture.width), height = Float(texture.height)
        let left = Float(cell % columns * cellWidth), top = Float(cell / columns * cellHeight)
        return SIMD4(
            left / width, top / height, (left + Float(cellWidth)) / width,
            (top + Float(cellHeight)) / height)
    }
}

/// What the text pass draws, in pixels from the target's top left.
public enum TextItem: Equatable {
    /// A filled rectangle.
    case box(x: Int, y: Int, width: Int, height: Int, color: SIMD4<Float>)
    /// A line of text, its top left at x, y, a cell a character.
    case text(String, x: Int, y: Int, color: SIMD4<Float>)
}

struct TextInstance {
    var rect: SIMD4<Float>
    var uv: SIMD4<Float>
    var color: SIMD4<Float>
}

/// Draws text items over a finished frame.
public final class TextPass {
    public let atlas: GlyphAtlas
    private var pipeline: MTLRenderPipelineState
    private let outputFormat: MTLPixelFormat
    private let sampler: MTLSamplerState
    private var buffers: [MTLBuffer?] = [nil, nil, nil]
    private var frame = 0
    /// The quads drawn in the last frame, for tests.
    public private(set) var lastDrawn = 0

    public init(
        device: MTLDevice, atlas: GlyphAtlas, outputFormat: MTLPixelFormat,
        source: String? = nil
    ) throws {
        self.atlas = atlas
        self.outputFormat = outputFormat
        pipeline = try Self.pipeline(
            device: device, source: source ?? ShaderSources.source(.text),
            outputFormat: outputFormat)
        let samplerDescriptor = MTLSamplerDescriptor()
        samplerDescriptor.minFilter = .nearest
        samplerDescriptor.magFilter = .nearest
        samplerDescriptor.sAddressMode = .clampToEdge
        samplerDescriptor.tAddressMode = .clampToEdge
        guard let sampler = device.makeSamplerState(descriptor: samplerDescriptor) else {
            throw TextError.resources("sampler")
        }
        self.sampler = sampler
    }

    private static func pipeline(
        device: MTLDevice, source: String,
        outputFormat: MTLPixelFormat
    ) throws -> MTLRenderPipelineState {
        do {
            let library = try device.makeLibrary(source: source, options: nil)
            let descriptor = MTLRenderPipelineDescriptor()
            descriptor.vertexFunction = try library.required("text_vertex")
            descriptor.fragmentFunction = try library.required("text_fragment")
            guard let color = descriptor.colorAttachments[0] else { throw TextError.resources("attachment") }
            color.pixelFormat = outputFormat
            color.isBlendingEnabled = true
            color.sourceRGBBlendFactor = .one
            color.destinationRGBBlendFactor = .oneMinusSourceAlpha
            color.sourceAlphaBlendFactor = .one
            color.destinationAlphaBlendFactor = .oneMinusSourceAlpha
            return try device.makeRenderPipelineState(descriptor: descriptor)
        } catch {
            throw TextError.resources("\(error)")
        }
    }

    /// Compiles `source` again; the pipeline is replaced only if it builds.
    public func reloadShaders(source: String) throws {
        pipeline = try Self.pipeline(device: pipeline.device, source: source, outputFormat: outputFormat)
    }

    /// The quads of `items` over a target `width` by `height`.
    func instances(_ items: [TextItem], width: Int, height: Int) -> [TextInstance] {
        let x = { (pixels: Int) in Float(pixels) / Float(width) * 2 - 1 }
        let y = { (pixels: Int) in 1 - Float(pixels) / Float(height) * 2 }
        let solid = atlas.uv(GlyphAtlas.solid)
        // (the solid cell's middle, so that a box's edges never sample a neighbour)
        let middle = SIMD4(
            (solid.x + solid.z) / 2, (solid.y + solid.w) / 2, (solid.x + solid.z) / 2,
            (solid.y + solid.w) / 2)
        var out: [TextInstance] = []
        for item in items {
            switch item {
            case let .box(left, top, boxWidth, boxHeight, color):
                guard boxWidth > 0, boxHeight > 0 else { continue }
                out.append(
                    TextInstance(
                        rect: SIMD4(x(left), y(top), x(left + boxWidth), y(top + boxHeight)),
                        uv: middle, color: color))
            case let .text(text, left, top, color):
                var column = 0
                for scalar in text.unicodeScalars {
                    defer { column += 1 }
                    if scalar == " " { continue }
                    let cellLeft = left + column * atlas.cellWidth
                    if cellLeft >= width { break }
                    out.append(
                        TextInstance(
                            rect: SIMD4(x(cellLeft), y(top), x(cellLeft + atlas.cellWidth), y(top + atlas.cellHeight)),
                            uv: atlas.uv(atlas.cell(scalar)), color: color))
                }
            }
        }
        return out
    }

    /// Draws `items` over `target` (keeping what is there); nothing is
    /// encoded when there are none.
    public func encode(_ items: [TextItem], into commands: MTLCommandBuffer, target: MTLTexture) {
        lastDrawn = 0
        guard !items.isEmpty else { return }
        let quads = instances(items, width: target.width, height: target.height)
        guard !quads.isEmpty else { return }
        frame = (frame + 1) % buffers.count
        let length = quads.count * MemoryLayout<TextInstance>.stride
        if (buffers[frame]?.length ?? 0) < length {
            buffers[frame] = HitchLog.shared.note(.allocation, "text quads \(length) bytes") {
                commands.device.makeBuffer(length: max(length, 64 * 1024), options: .storageModeShared)
            }
        }
        guard let buffer = buffers[frame] else { return }
        quads.withUnsafeBytes { raw in
            if let base = raw.baseAddress { buffer.contents().copyMemory(from: base, byteCount: raw.count) }
        }
        let pass = MTLRenderPassDescriptor()
        pass.colorAttachments[0].texture = target
        pass.colorAttachments[0].loadAction = .load
        pass.colorAttachments[0].storeAction = .store
        guard let encoder = commands.timedRenderEncoder("text", descriptor: pass) else { return }
        encoder.setRenderPipelineState(pipeline)
        encoder.setVertexBuffer(buffer, offset: 0, index: 0)
        encoder.setFragmentTexture(atlas.texture, index: 0)
        encoder.setFragmentSamplerState(sampler, index: 0)
        encoder.drawPrimitives(type: .triangle, vertexStart: 0, vertexCount: 6, instanceCount: quads.count)
        encoder.endEncoding()
        lastDrawn = quads.count
    }
}
