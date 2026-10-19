// The Metal backend (Sources/OpenCERender): batches written as the guest
// writes them, in guest memory of the test's own, drawn to textures off the
// screen and read back through READ_TEXTURE, as the guest reads them.
import Foundation
import ImageIO
import Metal
import Testing

import OpenCERender
import OpenCERuntime

// opence_render.h
private enum Op: UInt32 {
    case createBuffer = 1, uploadBuffer, createTexture, uploadTexture, destroy, createLibrary, createPipeline,
         createDepthStencil, createSampler, beginPass, endPass, setPipeline, setDepthStencil, setRaster,
         setViewport, setScissor, setBlendColor, setVertexBuffer, setBytes, setTexture, setSampler, draw,
         drawIndexed, setVisibility, copyTexture, readTexture, generateMipmaps, present
}

private let rgba8: UInt32 = 1, bgra8: UInt32 = 2, depth32f: UInt32 = 12
private let plane: UInt32 = 1
private let sampled: UInt32 = 1, renderTarget: UInt32 = 2
private let float2: UInt32 = 2, float3: UInt32 = 3
private let stepVertex: UInt32 = 1
private let blendOne: UInt32 = 2, blendZero: UInt32 = 1, blendAdd: UInt32 = 1
private let writeAll: UInt32 = 15
private let compareLess: UInt32 = 2, compareAlways: UInt32 = 8, stencilKeep: UInt32 = 1
private let loadDontCare: UInt32 = 1, loadLoad: UInt32 = 2, loadClear: UInt32 = 3
private let fragmentStage: UInt32 = 2
private let triangle: UInt32 = 4, triangleStrip: UInt32 = 5
private let uint16: UInt32 = 1
private let nearest: UInt32 = 1, noMip: UInt32 = 0, clampToEdge: UInt32 = 3, transparentBlack: UInt32 = 1
private let objectBuffer: UInt32 = 1, objectLibrary: UInt32 = 3

// where things are in the guest's memory
private let batchAddress: UInt32 = 0x10_0000
private let dataAddress: UInt32 = 0x12_0000
private let readAddress: UInt32 = 0x16_0000

private func float(_ value: Float) -> UInt32 { value.bitPattern }

/// MSL of the two stages the tests draw with: positions from attribute 0,
/// and a colour from the fragment's bytes in slot 0, or from texture 0.
private let flatShader = """
    #include <metal_stdlib>
    using namespace metal;
    struct In { float3 position [[attribute(0)]]; };
    struct Out { float4 position [[position]]; float2 uv; };
    vertex Out vertex_main(In in [[stage_in]]) {
        Out out;
        out.position = float4(in.position, 1);
        out.uv = in.position.xy * float2(0.5, -0.5) + 0.5;
        return out;
    }
    fragment float4 fragment_main(Out in [[stage_in]], constant float4 &color [[buffer(0)]]) { return color; }
    """
private let texturedShader = """
    #include <metal_stdlib>
    using namespace metal;
    struct In { float3 position [[attribute(0)]]; };
    struct Out { float4 position [[position]]; float2 uv; };
    vertex Out vertex_main(In in [[stage_in]]) {
        Out out;
        out.position = float4(in.position, 1);
        out.uv = in.position.xy * float2(0.5, -0.5) + 0.5;
        return out;
    }
    fragment float4 fragment_main(Out in [[stage_in]], texture2d<float> picture [[texture(0)]],
                                  sampler nearest [[sampler(0)]]) {
        return picture.sample(nearest, in.uv);
    }
    """

/// A batch, as the guest's writer makes one.
private struct Batch {
    var words: [UInt32] = [0x4252_434f, 1, 0, 0]

    mutating func add(_ op: Op, _ fields: [UInt32] = []) {
        words.append(op.rawValue | UInt32(fields.count + 1) << 16)
        words.append(contentsOf: fields)
        words[3] += 1
    }

    var data: Data {
        var words = self.words
        words[2] = UInt32(words.count * 4)
        var data = Data()
        for word in words {
            withUnsafeBytes(of: word.littleEndian) { data.append(contentsOf: $0) }
        }
        return data
    }

    // ---------- the commands the tests use most

    mutating func createTarget(_ id: UInt32, size: UInt32, format: UInt32 = rgba8) {
        add(.createTexture, [id, plane, format, size, size, 1, 1, sampled | renderTarget, 1])
    }

    mutating func beginPass(color: UInt32, depth: UInt32 = 0, load: UInt32 = loadClear,
                            clear: (Float, Float, Float, Float) = (0, 0, 0, 1)) {
        add(.beginPass, [color, depth, load, depth == 0 ? loadDontCare : loadClear, loadDontCare,
                         float(clear.0), float(clear.1), float(clear.2), float(clear.3), float(1), 0, 0, 0])
    }

    /// A pipeline of `library`'s two stages, positions float3 from slot 1.
    mutating func createPipeline(_ id: UInt32, library: UInt32, color: UInt32 = rgba8, depth: UInt32 = 0) {
        add(.createPipeline, [id, library, library, color, depth, 1, 0, blendOne, blendZero, blendAdd, blendOne,
                              blendZero, blendAdd, writeAll, 1, 1, 0, float3, 0, 1, 1, 12, stepVertex, 1])
    }

    mutating func setColor(_ red: Float, _ green: Float, _ blue: Float, _ alpha: Float = 1) {
        add(.setBytes, [fragmentStage, 0, 16, float(red), float(green), float(blue), float(alpha)])
    }

    /// Reads the `size` by `size` RGBA8 texture to readAddress.
    mutating func read(_ id: UInt32, size: UInt32) {
        add(.readTexture, [id, 0, 0, 0, 0, size, size, readAddress, size * 4, size * size * 4])
    }
}

/// Guest memory, a renderer for it, and an off-screen window.
private struct Fixture {
    let memory: GuestMemoryView
    let window: OffscreenWindow
    let renderer: MetalRenderer

    init(windowSize: Int = 8, capture: FrameCapture? = nil) throws {
        let device = try #require(MTLCreateSystemDefaultDevice())
        memory = try GuestMemoryView.withRanges(ranges: [GuestRangeSpec(address: batchAddress, size: 0x8_0000)])
        window = try #require(OffscreenWindow(device: device, width: windowSize, height: windowSize,
                                              capture: capture))
        renderer = try #require(MetalRenderer(device: device, memory: memory, window: window))
    }

    /// Writes `bytes` at dataAddress + offset; its guest address.
    @discardableResult
    func put(_ bytes: Data, at offset: UInt32 = 0) throws -> UInt32 {
        try memory.write(address: dataAddress + offset, bytes: bytes)
        return dataAddress + offset
    }

    func put(floats: [Float], at offset: UInt32) throws -> UInt32 {
        try put(floats.withUnsafeBytes { Data($0) }, at: offset)
    }

    /// Gives the batch to the renderer from guest memory, as the guest
    /// does; the commands it refused.
    @discardableResult
    func submit(_ batch: Batch) throws -> Int32 {
        let data = batch.data
        try memory.write(address: batchAddress, bytes: data)
        return renderer.submit(address: batchAddress, size: UInt32(data.count))
    }

    /// The RGBA8 pixels read to readAddress.
    func pixels(_ count: Int) throws -> [[UInt8]] {
        let bytes = [UInt8](try memory.read(address: readAddress, size: UInt64(count * 4)))
        return stride(from: 0, to: bytes.count, by: 4).map { Array(bytes[$0..<$0 + 4]) }
    }

    /// A library of `source`, written at `offset` in the data.
    func library(_ id: UInt32, _ source: String, at offset: UInt32, in batch: inout Batch) throws {
        let bytes = Data(source.utf8)
        let address = try put(bytes, at: offset)
        batch.add(.createLibrary, [id, address, UInt32(bytes.count)])
    }

    /// Two triangles covering the target at depth z, as a buffer `id`
    /// filled from `offset` in the data.
    func quad(_ id: UInt32, z: Float = 0.5, left: Float = -1, right: Float = 1, at offset: UInt32,
              in batch: inout Batch) throws {
        let corners: [Float] = [left, -1, z, right, -1, z, left, 1, z, right, 1, z]
        let address = try put(floats: corners, at: offset)
        batch.add(.createBuffer, [id, 48])
        batch.add(.uploadBuffer, [id, 0, address, 48])
    }
}

private func near(_ pixel: [UInt8], _ expected: [UInt8], within tolerance: Int = 1) -> Bool {
    zip(pixel, expected).allSatisfy { abs(Int($0) - Int($1)) <= tolerance }
}

@Suite(.serialized)
struct MetalRendererTests {
    @Test func aClearedTargetIsReadBack() throws {
        let fixture = try Fixture()
        var batch = Batch()
        batch.createTarget(1, size: 4)
        batch.beginPass(color: 1, clear: (1, 0.5, 0, 1))
        batch.add(.endPass)
        batch.read(1, size: 4)
        #expect(try fixture.submit(batch) == 0)
        let pixels = try fixture.pixels(16)
        #expect(pixels.allSatisfy { near($0, [255, 128, 0, 255]) }, "\(pixels)")
    }

    @Test func aDrawFillsTheTargetWithItsColour() throws {
        let fixture = try Fixture()
        var batch = Batch()
        batch.createTarget(1, size: 4)
        try fixture.library(1, flatShader, at: 0x1000, in: &batch)
        batch.createPipeline(1, library: 1)
        try fixture.quad(1, at: 0, in: &batch)
        batch.beginPass(color: 1)
        batch.add(.setPipeline, [1])
        batch.add(.setVertexBuffer, [1, 1, 0])
        batch.setColor(0, 1, 0)
        batch.add(.draw, [triangleStrip, 0, 4, 1])
        batch.add(.endPass)
        batch.read(1, size: 4)
        #expect(try fixture.submit(batch) == 0)
        #expect(try fixture.pixels(16).allSatisfy { near($0, [0, 255, 0, 255]) })
    }

    @Test func indexedDrawsAndTheScissorAreHonoured() throws {
        let fixture = try Fixture()
        var batch = Batch()
        batch.createTarget(1, size: 4)
        try fixture.library(1, flatShader, at: 0x1000, in: &batch)
        batch.createPipeline(1, library: 1)
        try fixture.quad(1, at: 0, in: &batch)
        let indices: [UInt16] = [0, 1, 2, 2, 1, 3]
        let address = try fixture.put(indices.withUnsafeBytes { Data($0) }, at: 0x800)
        batch.add(.createBuffer, [2, 12])
        batch.add(.uploadBuffer, [2, 0, address, 12])
        batch.beginPass(color: 1)
        batch.add(.setPipeline, [1])
        batch.add(.setVertexBuffer, [1, 1, 0])
        // (the left half only; and a scissor past the target is clipped)
        batch.add(.setScissor, [0, 0, 2, 400])
        batch.setColor(0, 0, 1)
        batch.add(.drawIndexed, [triangle, 6, uint16, 2, 0, 0, 1])
        batch.add(.endPass)
        batch.read(1, size: 4)
        #expect(try fixture.submit(batch) == 0)
        let pixels = try fixture.pixels(16)
        for (index, pixel) in pixels.enumerated() {
            #expect(near(pixel, index % 4 < 2 ? [0, 0, 255, 255] : [0, 0, 0, 255]), "pixel \(index): \(pixel)")
        }
    }

    @Test func aTextureIsUploadedAndSampled() throws {
        let fixture = try Fixture()
        var batch = Batch()
        batch.createTarget(1, size: 2)
        // a 2 by 2 picture: red, green / blue, white
        let texels: [UInt8] = [255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 255]
        let address = try fixture.put(Data(texels), at: 0x800)
        batch.add(.createTexture, [2, plane, rgba8, 2, 2, 1, 1, sampled, 1])
        batch.add(.uploadTexture, [2, 0, 0, 0, 0, 0, 2, 2, 1, address, 8, 16, 16])
        batch.add(.createSampler, [1, nearest, nearest, noMip, clampToEdge, clampToEdge, clampToEdge, 1,
                                   float(0), float(0), transparentBlack])
        try fixture.library(1, texturedShader, at: 0x1000, in: &batch)
        batch.createPipeline(1, library: 1)
        try fixture.quad(1, at: 0, in: &batch)
        batch.beginPass(color: 1)
        batch.add(.setPipeline, [1])
        batch.add(.setVertexBuffer, [1, 1, 0])
        batch.add(.setTexture, [fragmentStage, 0, 2])
        batch.add(.setSampler, [fragmentStage, 0, 1])
        batch.add(.draw, [triangleStrip, 0, 4, 1])
        batch.add(.endPass)
        batch.read(1, size: 2)
        #expect(try fixture.submit(batch) == 0)
        #expect(try fixture.pixels(4) == texels.chunked())
    }

    @Test func theNearerDrawWinsTheDepthTest() throws {
        let fixture = try Fixture()
        var batch = Batch()
        batch.createTarget(1, size: 4)
        batch.add(.createTexture, [2, plane, depth32f, 4, 4, 1, 1, renderTarget, 1])
        try fixture.library(1, flatShader, at: 0x1000, in: &batch)
        batch.createPipeline(1, library: 1, depth: depth32f)
        batch.add(.createDepthStencil, [1, compareLess, 1, 0] + [UInt32](repeating: 0, count: 12).enumerated().map {
            [stencilKeep, stencilKeep, stencilKeep, compareAlways, 0xff, 0xff][$0.offset % 6]
        })
        try fixture.quad(1, z: 0.25, at: 0, in: &batch)
        try fixture.quad(2, z: 0.75, at: 0x100, in: &batch)
        batch.beginPass(color: 1, depth: 2)
        batch.add(.setPipeline, [1])
        batch.add(.setDepthStencil, [1, 0])
        // the near one first, so the far one drawn after must be hidden
        batch.add(.setVertexBuffer, [1, 1, 0])
        batch.setColor(1, 0, 0)
        batch.add(.draw, [triangleStrip, 0, 4, 1])
        batch.add(.setVertexBuffer, [1, 2, 0])
        batch.setColor(0, 0, 1)
        batch.add(.draw, [triangleStrip, 0, 4, 1])
        batch.add(.endPass)
        batch.read(1, size: 4)
        #expect(try fixture.submit(batch) == 0)
        #expect(try fixture.pixels(16).allSatisfy { near($0, [255, 0, 0, 255]) })
    }

    @Test func anUploadInsideAPassReachesOnlyTheDrawsAfterIt() throws {
        let fixture = try Fixture()
        var batch = Batch()
        batch.createTarget(1, size: 4)
        batch.add(.createDepthStencil, [1, compareAlways, 0, 0] + [UInt32](repeating: 1, count: 12))
        try fixture.library(1, flatShader, at: 0x1000, in: &batch)
        batch.createPipeline(1, library: 1)
        try fixture.quad(1, left: -1, right: 0, at: 0, in: &batch)
        let right = try fixture.put(floats: [0, -1, 0.5, 1, -1, 0.5, 0, 1, 0.5, 1, 1, 0.5], at: 0x100)
        batch.beginPass(color: 1, clear: (0, 0, 0, 0))
        batch.add(.setPipeline, [1])
        batch.add(.setVertexBuffer, [1, 1, 0])
        batch.setColor(1, 1, 0)
        batch.add(.draw, [triangleStrip, 0, 4, 1])
        // the same buffer, now the right half: the draw before keeps the left
        batch.add(.uploadBuffer, [1, 0, right, 48])
        batch.setColor(0, 1, 1)
        batch.add(.draw, [triangleStrip, 0, 4, 1])
        batch.add(.endPass)
        batch.read(1, size: 4)
        #expect(try fixture.submit(batch) == 0)
        let pixels = try fixture.pixels(16)
        for (index, pixel) in pixels.enumerated() {
            #expect(near(pixel, index % 4 < 2 ? [255, 255, 0, 255] : [0, 255, 255, 255]), "pixel \(index): \(pixel)")
        }
    }

    @Test func passesAndCopiesLastFromBatchToBatch() throws {
        let fixture = try Fixture()
        var first = Batch()
        first.createTarget(1, size: 4)
        first.createTarget(2, size: 4)
        first.beginPass(color: 1, clear: (0, 0, 1, 1))
        #expect(try fixture.submit(first) == 0)
        var second = Batch()
        second.add(.endPass)
        second.add(.copyTexture, [1, 0, 0, 0, 0, 2, 0, 0, 2, 2, 2, 2])
        second.read(2, size: 4)
        #expect(try fixture.submit(second) == 0)
        let pixels = try fixture.pixels(16)
        // (2's own pixels are what a new texture has: not read)
        #expect(near(pixels[10], [0, 0, 255, 255]) && near(pixels[15], [0, 0, 255, 255]), "\(pixels)")
    }

    @Test func theWindowsFrameIsDrawnAndPresented() throws {
        let fixture = try Fixture(windowSize: 8)
        var batch = Batch()
        batch.beginPass(color: 0, clear: (0, 1, 0, 1))
        batch.add(.endPass)
        batch.add(.present)
        #expect(try fixture.submit(batch) == 0)
        fixture.renderer.finish()
        #expect(fixture.window.presented == 1)
        // the window is BGRA
        let pixels = fixture.window.pixels()
        #expect(pixels.count == 64 && pixels.allSatisfy { near($0, [0, 255, 0, 255]) }, "\(pixels)")
    }

    @Test func identicalStatesAreMadeOnce() throws {
        let fixture = try Fixture()
        var batch = Batch()
        try fixture.library(1, flatShader, at: 0x1000, in: &batch)
        try fixture.library(2, flatShader, at: 0x1000, in: &batch)
        batch.createPipeline(1, library: 1)
        batch.createPipeline(2, library: 2)
        batch.createPipeline(3, library: 1, color: bgra8)
        let stencil = [UInt32](repeating: 1, count: 12)
        batch.add(.createDepthStencil, [1, compareLess, 1, 0] + stencil)
        batch.add(.createDepthStencil, [2, compareLess, 1, 0] + stencil)
        #expect(try fixture.submit(batch) == 0)
        #expect(fixture.renderer.statistics.librariesCompiled == 1)
        #expect(fixture.renderer.statistics.pipelinesMade == 2)
        #expect(fixture.renderer.statistics.depthStencilsMade == 1)
        // and a destroyed one is made again from the cache
        var again = Batch()
        again.add(.destroy, [objectLibrary, 1])
        try fixture.library(1, flatShader, at: 0x1000, in: &again)
        again.add(.destroy, [objectBuffer, 7])
        #expect(try fixture.submit(again) == 1)
        #expect(fixture.renderer.statistics.librariesCompiled == 1)
    }

    @Test func whatMetalCannotMakeIsRefusedAndWhatUsesItSkipped() throws {
        let fixture = try Fixture()
        var batch = Batch()
        batch.createTarget(1, size: 4)
        try fixture.library(1, "this is not Metal", at: 0x1000, in: &batch)
        // (the validator does not compile: the pipeline is the renderer's to refuse)
        batch.createPipeline(1, library: 1)
        try fixture.quad(1, at: 0, in: &batch)
        batch.beginPass(color: 1, clear: (1, 1, 1, 1))
        batch.add(.setPipeline, [1])
        batch.add(.setVertexBuffer, [1, 1, 0])
        batch.add(.draw, [triangleStrip, 0, 4, 1])
        batch.add(.endPass)
        batch.read(1, size: 4)
        // the library, the pipeline, setting it, and the draw
        #expect(try fixture.submit(batch) == 4)
        #expect(try fixture.pixels(16).allSatisfy { near($0, [255, 255, 255, 255]) })
        #expect(fixture.renderer.statistics.refused == 4)
    }

    // ---------- frame captures (HALO_HOST_CAPTURE)

    @Test func theCaptureSettingIsRead() throws {
        #expect(try FrameCapture.parse([:]) == nil)
        let plain = try #require(try FrameCapture.parse(["HALO_HOST_CAPTURE": "/tmp/frame.png"]))
        #expect(plain == FrameCapture(path: URL(fileURLWithPath: "/tmp/frame.png"), frame: 1, width: 640, height: 480))
        let set = try #require(try FrameCapture.parse(["HALO_HOST_CAPTURE": "frame.png", "HALO_HOST_CAPTURE_FRAME": "3",
                                                       "HALO_HOST_CAPTURE_SIZE": "320x200"]))
        #expect(set.frame == 3 && set.width == 320 && set.height == 200)
        for (name, value) in [("HALO_HOST_CAPTURE_FRAME", "0"), ("HALO_HOST_CAPTURE_FRAME", "two"),
                              ("HALO_HOST_CAPTURE_SIZE", "640"), ("HALO_HOST_CAPTURE_SIZE", "0x480"),
                              ("HALO_HOST_CAPTURE_SIZE", "99999x480")] {
            #expect(throws: FrameCapture.Invalid.self, "\(name)=\(value)") {
                try FrameCapture.parse(["HALO_HOST_CAPTURE": "frame.png", name: value])
            }
        }
    }

    @Test func theCapturedFrameIsWrittenAsAPNG() throws {
        let path = FileManager.default.temporaryDirectory.appendingPathComponent("capture-\(UUID().uuidString).png")
        defer { try? FileManager.default.removeItem(at: path) }
        let fixture = try Fixture(windowSize: 8, capture: FrameCapture(path: path, frame: 2, width: 8, height: 8))
        let messages = Messages()
        fixture.renderer.log = { messages.add($0) }
        for clear in [(Float(0), Float(1), Float(0), Float(1)), (1, 0.5, 0, 0.5), (0, 0, 1, 1)] {
            var batch = Batch()
            batch.beginPass(color: 0, clear: clear)
            batch.add(.endPass)
            batch.add(.present)
            #expect(try fixture.submit(batch) == 0)
            // (written before the second PRESENT's batch is answered)
            #expect(FileManager.default.fileExists(atPath: path.path) == (fixture.window.presented >= 2))
        }
        // the second frame's, not the first's or the third's, alpha too
        let pixels = try pngPixels(path)
        #expect(pixels.count == 64 && pixels.allSatisfy { near($0, [255, 128, 0, 128]) }, "\(pixels)")
        #expect(messages.all.contains { $0.contains("captured frame 2") }, "\(messages.all)")
    }

    @Test func aBatchThatIsNotOneIsRefusedWhole() throws {
        let fixture = try Fixture()
        try fixture.memory.write(address: batchAddress, bytes: Data(count: 16))
        #expect(fixture.renderer.submit(address: batchAddress, size: 16) == -1)
        #expect(fixture.renderer.submit(address: 0xffff_f000, size: 16) == -1)
    }
}

private extension Array where Element == UInt8 {
    func chunked() -> [[UInt8]] {
        stride(from: 0, to: count, by: 4).map { Array(self[$0..<$0 + 4]) }
    }
}

/// A PNG's pixels, RGBA, as ImageIO reads them back.
private func pngPixels(_ path: URL) throws -> [[UInt8]] {
    let source = try #require(CGImageSourceCreateWithURL(path as CFURL, nil))
    let image = try #require(CGImageSourceCreateImageAtIndex(source, 0, nil))
    let data = try #require(image.dataProvider?.data as Data?)
    #expect(image.bitsPerComponent == 8 && image.bitsPerPixel == 32 && image.alphaInfo == .last)
    return [UInt8](data).chunked()
}

/// What the renderer logged, from the thread it logs on.
private final class Messages: @unchecked Sendable {
    private let lock = NSLock()
    private var messages: [String] = []

    func add(_ message: String) { lock.withLock { messages.append(message) } }
    var all: [String] { lock.withLock { messages } }
}
