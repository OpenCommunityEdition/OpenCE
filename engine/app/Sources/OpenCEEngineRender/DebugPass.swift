// The debug drawing (engine/crates/dev's draw): lines in the world, drawn
// into the frame after the level and its objects with the depth test, and
// labels, placed on the screen for the text pass.
import Metal
import simd

/// A debug line in the world.
public struct DebugSegment: Equatable {
    public var from: SIMD3<Float>
    public var to: SIMD3<Float>
    /// Red, green, blue and alpha a byte each from the top.
    public var color: UInt32

    public init(from: SIMD3<Float>, to: SIMD3<Float>, color: UInt32) {
        self.from = from
        self.to = to
        self.color = color
    }
}

/// A debug label at a point in the world.
public struct DebugMark: Equatable {
    public var at: SIMD3<Float>
    public var color: UInt32
    public var text: String

    public init(at: SIMD3<Float>, color: UInt32, text: String) {
        self.at = at
        self.color = color
        self.text = text
    }
}

/// A packed colour's channels, from 0 to 1.
public func debugColor(_ packed: UInt32) -> SIMD4<Float> {
    SIMD4(Float((packed >> 24) & 0xff), Float((packed >> 16) & 0xff), Float((packed >> 8) & 0xff),
          Float(packed & 0xff)) / 255
}

/// Draws debug lines into the level's frame.
public final class DebugPass {
    private var pipeline: MTLRenderPipelineState
    private let colorFormat: MTLPixelFormat
    private let depthFormat: MTLPixelFormat
    private var buffers: [MTLBuffer?] = [nil, nil, nil]
    private var frame = 0
    /// The lines drawn in the last frame, for tests.
    public private(set) var lastDrawn = 0

    public init(device: MTLDevice, colorFormat: MTLPixelFormat, depthFormat: MTLPixelFormat,
                source: String? = nil) throws {
        self.colorFormat = colorFormat
        self.depthFormat = depthFormat
        pipeline = try Self.pipeline(device: device, source: source ?? ShaderSources.source(.debug),
                                     colorFormat: colorFormat, depthFormat: depthFormat)
    }

    private static func pipeline(device: MTLDevice, source: String, colorFormat: MTLPixelFormat,
                                 depthFormat: MTLPixelFormat) throws -> MTLRenderPipelineState {
        do {
            let library = try device.makeLibrary(source: source, options: nil)
            let descriptor = MTLRenderPipelineDescriptor()
            descriptor.vertexFunction = try library.required("debug_vertex")
            descriptor.fragmentFunction = try library.required("debug_fragment")
            descriptor.colorAttachments[0].pixelFormat = colorFormat
            descriptor.depthAttachmentPixelFormat = depthFormat
            return try device.makeRenderPipelineState(descriptor: descriptor)
        } catch {
            throw LevelRendererError.shaders("debug: \(error)")
        }
    }

    /// Compiles `source` again; the pipeline is replaced only if it builds.
    public func reloadShaders(source: String) throws {
        pipeline = try Self.pipeline(device: pipeline.device, source: source, colorFormat: colorFormat,
                                     depthFormat: depthFormat)
    }

    /// The vertices of `segments`: each end's point, its colour's bits last.
    public static func vertices(_ segments: [DebugSegment]) -> [SIMD4<Float>] {
        var out: [SIMD4<Float>] = []
        out.reserveCapacity(segments.count * 2)
        for segment in segments {
            let bits = Float(bitPattern: segment.color)
            out.append(SIMD4(segment.from, bits))
            out.append(SIMD4(segment.to, bits))
        }
        return out
    }

    /// Draws `segments` into an encoder of the level's frame (which keeps
    /// its depth state); nothing is encoded when there are none. The
    /// encoder's vertex buffers 0 and 1 are left changed.
    func encode(_ segments: [DebugSegment], viewProjection: simd_float4x4, into encoder: MTLRenderCommandEncoder) {
        lastDrawn = 0
        guard !segments.isEmpty else { return }
        let vertices = Self.vertices(segments)
        frame = (frame + 1) % buffers.count
        let length = vertices.count * MemoryLayout<SIMD4<Float>>.stride
        if (buffers[frame]?.length ?? 0) < length {
            buffers[frame] = encoder.device.makeBuffer(length: max(length, 64 * 1024), options: .storageModeShared)
        }
        guard let buffer = buffers[frame] else { return }
        vertices.withUnsafeBytes { raw in
            if let base = raw.baseAddress { buffer.contents().copyMemory(from: base, byteCount: raw.count) }
        }
        var matrix = viewProjection
        encoder.pushDebugGroup("debug lines")
        encoder.setRenderPipelineState(pipeline)
        encoder.setVertexBuffer(buffer, offset: 0, index: 0)
        encoder.setVertexBytes(&matrix, length: MemoryLayout<simd_float4x4>.stride, index: 1)
        encoder.drawPrimitives(type: .line, vertexStart: 0, vertexCount: vertices.count)
        encoder.popDebugGroup()
        lastDrawn = segments.count
    }

    /// The labels in front of the camera as text on a target `width` by
    /// `height`, each centred on its point, a dark box behind it.
    public static func labels(_ marks: [DebugMark], viewProjection: simd_float4x4, width: Int, height: Int,
                              cellWidth: Int, cellHeight: Int) -> [TextItem] {
        var items: [TextItem] = []
        for mark in marks where !mark.text.isEmpty {
            let clip = viewProjection * SIMD4(mark.at, 1)
            // (behind the eye, or at it)
            guard clip.w > ViewCamera.near else { continue }
            let ndc = SIMD2(clip.x, clip.y) / clip.w
            guard abs(ndc.x) <= 1, abs(ndc.y) <= 1 else { continue }
            let columns = mark.text.unicodeScalars.count
            let x = Int((ndc.x + 1) / 2 * Float(width)) - columns * cellWidth / 2
            let y = Int((1 - ndc.y) / 2 * Float(height)) - cellHeight / 2
            items.append(.box(x: x - 2, y: y, width: columns * cellWidth + 4, height: cellHeight,
                              color: SIMD4(0, 0, 0, 0.6)))
            items.append(.text(mark.text, x: x, y: y, color: debugColor(mark.color)))
        }
        return items
    }
}
