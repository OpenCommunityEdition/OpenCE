import Foundation
import Metal
import OpenCEEngineRender
import simd
import XCTest

/// The shaders' sources (engine/app/Shaders), the watcher noticing edits,
/// and the passes compiling them again: an edited source is swapped in, a
/// broken one keeps the old pipelines and reports why.
final class ShaderTests: XCTestCase {
    func device() throws -> MTLDevice {
        try XCTUnwrap(MTLCreateSystemDefaultDevice(), "no Metal device")
    }

    /// A folder of its own in the temporary folder, removed after the test.
    func folder() throws -> URL {
        let folder = FileManager.default.temporaryDirectory.appendingPathComponent("shaders-\(UUID().uuidString)")
        try FileManager.default.createDirectory(at: folder, withIntermediateDirectories: true)
        addTeardownBlock { try? FileManager.default.removeItem(at: folder) }
        return folder
    }

    func testTheSourcesAreReadFromTheCheckout() throws {
        let checkout = try XCTUnwrap(ShaderSources.checkout, "no engine/app/Shaders")
        XCTAssertEqual(ShaderSources.folder(), checkout)
        let functions: [Shader: String] = [.level: "level_fragment", .hud: "hud_vertex", .text: "text_fragment",
                                           .debug: "debug_vertex", .bake: "bake_irradiance"]
        for shader in Shader.allCases {
            let source = try ShaderSources.source(shader)
            XCTAssertTrue(source.contains(try XCTUnwrap(functions[shader])), shader.file)
        }
        XCTAssertThrowsError(try ShaderSources.source(.level, in: nil))
        XCTAssertThrowsError(try ShaderSources.source(.level, in: try folder()))
    }

    /// An object's change colours reach the model shader as colours, white
    /// for those the snapshot lacks.
    func testChangeColoursAreUnpackedForTheModelShader() {
        let object = DrawnObject(model: 0, transform: matrix_identity_float4x4, center: .zero, radius: 1,
                                 regions: [], nodes: [], changeColors: [0xFFFF_0000, 0xFF00_80FF])
        XCTAssertEqual(object.changeColorUniforms, [SIMD4(1, 0, 0, 1), SIMD4(0, Float(0x80) / 255, 1, 1),
                                                    SIMD4(1, 1, 1, 1), SIMD4(1, 1, 1, 1)])
        let plain = DrawnObject(model: 0, transform: matrix_identity_float4x4, center: .zero, radius: 1,
                                regions: [], nodes: [])
        XCTAssertEqual(plain.changeColorUniforms, Array(repeating: SIMD4<Float>(1, 1, 1, 1), count: 4))
    }

    func testTheWatcherNoticesEditedFiles() throws {
        let folder = try folder()
        let level = folder.appendingPathComponent(Shader.level.file)
        try "a".write(to: level, atomically: true, encoding: .utf8)
        let watcher = ShaderWatcher(folder: folder)
        XCTAssertEqual(watcher.changed(), [])
        try "abc".write(to: level, atomically: true, encoding: .utf8)
        XCTAssertEqual(watcher.changed(), [.level])
        XCTAssertEqual(watcher.changed(), [], "a change is reported once")
        // (a file that appears is a change; one that goes is not)
        try "b".write(to: folder.appendingPathComponent(Shader.text.file), atomically: true, encoding: .utf8)
        XCTAssertEqual(watcher.changed(), [.text])
        try FileManager.default.removeItem(at: level)
        XCTAssertEqual(watcher.changed(), [])
    }

    /// The text pass drawing a green box on black: its pixels' channels.
    func drawBox(_ pass: TextPass, device: MTLDevice, queue: MTLCommandQueue) throws -> SIMD4<UInt8> {
        let descriptor = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: .bgra8Unorm, width: 16, height: 16,
                                                                  mipmapped: false)
        descriptor.usage = [.renderTarget, .shaderRead]
        descriptor.storageMode = .shared
        let target = try XCTUnwrap(device.makeTexture(descriptor: descriptor))
        let commands = try XCTUnwrap(queue.makeCommandBuffer())
        let clear = MTLRenderPassDescriptor()
        clear.colorAttachments[0].texture = target
        clear.colorAttachments[0].loadAction = .clear
        clear.colorAttachments[0].clearColor = MTLClearColor(red: 0, green: 0, blue: 0, alpha: 1)
        clear.colorAttachments[0].storeAction = .store
        commands.makeRenderCommandEncoder(descriptor: clear)?.endEncoding()
        pass.encode([.box(x: 0, y: 0, width: 16, height: 16, color: SIMD4(0, 1, 0, 1))], into: commands,
                    target: target)
        commands.commit()
        commands.waitUntilCompleted()
        var bytes = [UInt8](repeating: 0, count: 4)
        target.getBytes(&bytes, bytesPerRow: 16 * 4, from: MTLRegionMake2D(8, 8, 1, 1), mipmapLevel: 0)
        // (bgra)
        return SIMD4(bytes[2], bytes[1], bytes[0], bytes[3])
    }

    func testAnEditedShaderIsSwappedInAndABrokenOneKept() throws {
        let device = try device()
        let queue = try XCTUnwrap(device.makeCommandQueue())
        let atlas = try GlyphAtlas(url: try XCTUnwrap(GlyphAtlas.url()), scale: 1, device: device)
        let pass = try TextPass(device: device, atlas: atlas, outputFormat: .bgra8Unorm)
        XCTAssertEqual(try drawBox(pass, device: device, queue: queue), SIMD4(0, 255, 0, 255))

        // (the same quads, drawn blue)
        let source = try ShaderSources.source(.text)
        let blue = source.replacingOccurrences(of: "return float4(in.color.rgb * alpha, alpha);",
                                               with: "return float4(0, 0, alpha, alpha);")
        XCTAssertNotEqual(blue, source)
        try pass.reloadShaders(source: blue)
        XCTAssertEqual(try drawBox(pass, device: device, queue: queue), SIMD4(0, 0, 255, 255))

        // a source that does not compile, or lacks a function, changes nothing
        XCTAssertThrowsError(try pass.reloadShaders(source: source + "\nthis is not metal")) { error in
            XCTAssertTrue("\(error)".contains("error"), "\(error)")
        }
        XCTAssertThrowsError(try pass.reloadShaders(source: source.replacingOccurrences(
            of: "text_fragment", with: "other_fragment")))
        XCTAssertEqual(try drawBox(pass, device: device, queue: queue), SIMD4(0, 0, 255, 255))
    }

    func testTheReloaderCompilesTheShadersThatChange() throws {
        let device = try device()
        let folder = try folder()
        for shader in Shader.allCases {
            try ShaderSources.source(shader).write(to: folder.appendingPathComponent(shader.file), atomically: true,
                                                   encoding: .utf8)
        }
        let reloader = ShaderReloader(folder: folder)
        let atlas = try GlyphAtlas(url: try XCTUnwrap(GlyphAtlas.url()), scale: 1, device: device)
        let text = try TextPass(device: device, atlas: atlas, outputFormat: .bgra8Unorm)
        XCTAssertTrue(reloader.poll(level: nil, hud: nil, text: text).isEmpty)

        let file = folder.appendingPathComponent(Shader.text.file)
        let source = try String(contentsOf: file, encoding: .utf8)
        try (source + "\nbroken").write(to: file, atomically: true, encoding: .utf8)
        // (the level's shader changed too, but there is no level: skipped)
        try (try ShaderSources.source(.level) + "\n").write(to: folder.appendingPathComponent(Shader.level.file),
                                                            atomically: true, encoding: .utf8)
        let broken = reloader.poll(level: nil, hud: nil, text: text)
        XCTAssertEqual(broken.map(\.shader), [.text])
        XCTAssertNotNil(broken.first?.error)

        try (source + "\n// fixed\n").write(to: file, atomically: true, encoding: .utf8)
        let fixed = reloader.poll(level: nil, hud: nil, text: text)
        XCTAssertEqual(fixed.map(\.shader), [.text])
        XCTAssertNil(fixed.first?.error)
    }

    func testTheLevelsShadersReloadIntoTheLevelAndItsObjects() throws {
        let level = try StockLevel(map: "bloodgulch")
        let renderer = try level.renderer(VideoSettings(antiAliasing: .native, renderScale: 1))
        let before = try renderer.capture(width: 160, height: 90, camera: level.camera, queue: level.queue)
        let source = try ShaderSources.source(.level)
        // (the tone map writing magenta: every pixel)
        let toned = source.replacingOccurrences(of: "float3 color = max(frame.sample(nearest, in.texcoord).rgb, 0);",
                                                with: "float3 color = float3(1, 0, 1);")
        XCTAssertNotEqual(toned, source)
        try renderer.reloadShaders(source: toned)
        // (eased by the tone map's shoulder: 1 is 236)
        let magenta = try renderer.capture(width: 160, height: 90, camera: level.camera, queue: level.queue)
        XCTAssertEqual(magenta.rgb(x: 80, y: 45), SIMD3(236, 0, 236))
        XCTAssertThrowsError(try renderer.reloadShaders(source: source.replacingOccurrences(
            of: "model_fragment", with: "other_fragment")), "the objects' functions are checked too")
        XCTAssertEqual(try renderer.capture(width: 160, height: 90, camera: level.camera, queue: level.queue)
            .rgb(x: 80, y: 45), SIMD3(236, 0, 236))
        try renderer.reloadShaders(source: source)
        let after = try renderer.capture(width: 160, height: 90, camera: level.camera, queue: level.queue)
        XCTAssertEqual(try XCTUnwrap(after.difference(from: before)).share, 0)
        // (the debug drawing's pass reloads by itself)
        XCTAssertNoThrow(try renderer.debug.reloadShaders(source: try ShaderSources.source(.debug)))
        XCTAssertThrowsError(try renderer.debug.reloadShaders(source: "broken"))
    }
}
