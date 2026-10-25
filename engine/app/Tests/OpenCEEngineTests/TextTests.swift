import CryptoKit
import Foundation
import Metal
import OpenCEEngineRender
import XCTest

/// The shell's monospace text: Overpass Mono's atlas at scales 1 and 2, its
/// licence and checksum, and the pass drawing it.
final class TextTests: XCTestCase {
    func device() throws -> MTLDevice {
        try XCTUnwrap(MTLCreateSystemDefaultDevice(), "no Metal device")
    }

    func testTheAtlasIsDrawnAtEachScale() throws {
        let url = try XCTUnwrap(GlyphAtlas.url(), "no OverpassMono-Regular.ttf")
        let one = try GlyphAtlas(url: url, pointSize: 13, scale: 1, device: try device())
        let two = try GlyphAtlas(url: url, pointSize: 13, scale: 2, device: try device())
        XCTAssertGreaterThan(one.cellWidth, 4)
        XCTAssertGreaterThan(one.cellHeight, one.cellWidth)
        XCTAssertLessThanOrEqual(abs(two.cellWidth - 2 * one.cellWidth), 1)
        XCTAssertLessThanOrEqual(abs(two.cellHeight - 2 * one.cellHeight), 2)
        XCTAssertEqual(two.texture.pixelFormat, .r8Unorm)
        for atlas in [one, two] {
            for scalar in "AMgW09~|_".unicodeScalars {
                XCTAssertTrue(atlas.has(scalar), "\(scalar)")
                XCTAssertGreaterThan(atlas.inkedPixels(scalar), 0, "\(scalar) at \(atlas.scale)")
            }
            XCTAssertEqual(atlas.inkedPixels(" "), 0)
            XCTAssertFalse(atlas.has("漢"))
        }
        // (twice the scale, about four times the ink)
        let ratio = Double(two.inkedPixels("M")) / Double(one.inkedPixels("M"))
        XCTAssertGreaterThan(ratio, 2.5)
        XCTAssertLessThan(ratio, 6)
        XCTAssertThrowsError(try GlyphAtlas(url: url, scale: 0, device: try device()))
        XCTAssertThrowsError(
            try GlyphAtlas(
                url: URL(fileURLWithPath: "/nonexistent.ttf"), scale: 1,
                device: try device()))
    }

    func testTheFontsLicenceAndChecksumArePresent() throws {
        let url = try XCTUnwrap(GlyphAtlas.url())
        let folder = url.deletingLastPathComponent()
        let licence = try String(contentsOf: folder.appendingPathComponent("OverpassMono-OFL.txt"), encoding: .utf8)
        XCTAssertTrue(licence.contains("SIL Open Font License"))
        XCTAssertTrue(licence.contains("Red Hat"))
        let digest = SHA256.hash(data: try Data(contentsOf: url)).map { String(format: "%02x", $0) }.joined()
        let readme = try String(contentsOf: folder.appendingPathComponent("README.md"), encoding: .utf8)
        XCTAssertTrue(readme.contains("sha256 \(digest)  \(GlyphAtlas.file)"), "README's checksum")
    }

    func testThePassDrawsTextAndBoxes() throws {
        let device = try device()
        let queue = try XCTUnwrap(device.makeCommandQueue())
        let atlas = try GlyphAtlas(url: try XCTUnwrap(GlyphAtlas.url()), scale: 2, device: device)
        let pass = try TextPass(device: device, atlas: atlas, outputFormat: .bgra8Unorm)
        let (width, height) = (256, 64)
        let descriptor = MTLTextureDescriptor.texture2DDescriptor(
            pixelFormat: .bgra8Unorm, width: width,
            height: height, mipmapped: false)
        descriptor.usage = [.renderTarget, .shaderRead]
        descriptor.storageMode = .shared
        let target = try XCTUnwrap(device.makeTexture(descriptor: descriptor))

        func draw(_ items: [TextItem]) throws -> [UInt8] {
            let commands = try XCTUnwrap(queue.makeCommandBuffer())
            let clear = MTLRenderPassDescriptor()
            clear.colorAttachments[0].texture = target
            clear.colorAttachments[0].loadAction = .clear
            clear.colorAttachments[0].clearColor = MTLClearColor(red: 0, green: 0, blue: 0, alpha: 1)
            clear.colorAttachments[0].storeAction = .store
            commands.makeRenderCommandEncoder(descriptor: clear)?.endEncoding()
            pass.encode(items, into: commands, target: target)
            commands.commit()
            commands.waitUntilCompleted()
            var bytes = [UInt8](repeating: 0, count: width * height * 4)
            target.getBytes(
                &bytes, bytesPerRow: width * 4, from: MTLRegionMake2D(0, 0, width, height),
                mipmapLevel: 0)
            return bytes
        }
        func lit(_ bytes: [UInt8], x: Range<Int>, y: Range<Int>) -> Int {
            var count = 0
            for row in y {
                for column in x where bytes[(row * width + column) * 4 + 1] > 64 {
                    count += 1
                }
            }
            return count
        }

        // nothing to draw: nothing encoded
        _ = try draw([])
        XCTAssertEqual(pass.lastDrawn, 0)

        // a box fills its rectangle exactly
        let boxed = try draw([.box(x: 10, y: 10, width: 20, height: 8, color: SIMD4(0, 1, 0, 1))])
        XCTAssertEqual(lit(boxed, x: 0..<width, y: 0..<height), 160)
        XCTAssertEqual(lit(boxed, x: 10..<30, y: 10..<18), 160)

        // text: a quad a character, spaces skipped, drawn in its cells
        let text = try draw([.text("ab c", x: 4, y: 4, color: SIMD4(1, 1, 1, 1))])
        XCTAssertEqual(pass.lastDrawn, 3)
        let cell = atlas.cellWidth
        XCTAssertGreaterThan(lit(text, x: 4..<(4 + cell), y: 4..<(4 + atlas.cellHeight)), 10)
        XCTAssertEqual(lit(text, x: (4 + 2 * cell)..<(4 + 3 * cell), y: 0..<height), 0, "the space")
        XCTAssertGreaterThan(lit(text, x: (4 + 3 * cell)..<(4 + 4 * cell), y: 4..<(4 + atlas.cellHeight)), 10)
        XCTAssertEqual(lit(text, x: (4 + 4 * cell)..<width, y: 0..<height), 0)
        // (a character the atlas lacks draws as ?)
        let missing = try draw([.text("漢", x: 0, y: 0, color: SIMD4(1, 1, 1, 1))])
        let question = try draw([.text("?", x: 0, y: 0, color: SIMD4(1, 1, 1, 1))])
        XCTAssertEqual(missing, question)
        // (text past the right edge is not drawn)
        _ = try draw([.text(String(repeating: "x", count: 100), x: 0, y: 0, color: SIMD4(1, 1, 1, 1))])
        XCTAssertEqual(pass.lastDrawn, (width + cell - 1) / cell)
    }
}
