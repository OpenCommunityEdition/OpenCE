import Foundation
import Metal
import OpenCEEngineCore
import OpenCEEngineRender
import simd
import XCTest

/// The camera's passes (CameraPass.swift and Shaders/camera.metal): a
/// flare is hidden by what is nearer and its sprite with it, the bloom
/// spreads what is brighter than white, and the brightness is averaged
/// for the exposure.
final class CameraTests: XCTestCase {
    private let size = 64

    private func device() throws -> MTLDevice {
        try XCTUnwrap(MTLCreateSystemDefaultDevice(), "no Metal device")
    }

    private func texture(_ device: MTLDevice, _ format: MTLPixelFormat, usage: MTLTextureUsage,
                         storage: MTLStorageMode = .private) throws -> MTLTexture {
        let descriptor = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: format, width: size, height: size,
                                                                  mipmapped: false)
        descriptor.usage = usage
        descriptor.storageMode = storage
        return try XCTUnwrap(device.makeTexture(descriptor: descriptor))
    }

    /// The red of each texel of `texture` (rgba16Float), copied out.
    private func reds(_ texture: MTLTexture, queue: MTLCommandQueue) throws -> [Float] {
        let device = queue.device
        let length = texture.width * texture.height * 8
        let buffer = try XCTUnwrap(device.makeBuffer(length: length, options: .storageModeShared))
        let commands = try XCTUnwrap(queue.makeCommandBuffer())
        let blit = try XCTUnwrap(commands.makeBlitCommandEncoder())
        blit.copy(from: texture, sourceSlice: 0, sourceLevel: 0, sourceOrigin: MTLOrigin(x: 0, y: 0, z: 0),
                  sourceSize: MTLSize(width: texture.width, height: texture.height, depth: 1), to: buffer,
                  destinationOffset: 0, destinationBytesPerRow: texture.width * 8,
                  destinationBytesPerImage: length)
        blit.endEncoding()
        commands.commit()
        commands.waitUntilCompleted()
        let halves = buffer.contents().bindMemory(to: Float16.self, capacity: length / 2)
        return (0..<(texture.width * texture.height)).map { Float(halves[$0 * 4]) }
    }

    /// A flare's disk in front of the camera, and a soft white sprite on
    /// it that its share seen scales.
    private func flare(at distance: Float) -> CameraSprites {
        CameraSprites(sprites: [CameraSprite(x: distance, y: 0, z: 0, width: 1, height: 1, rotation: 0, red: 1,
                                             green: 1, blue: 1, image: UInt32.max, occluder: 0, flags: 0)],
                      occluders: [FlareOccluder(x: distance, y: 0, z: 0, radius: 0.2)])
    }

    func testAFlareIsHiddenByWhatIsNearerAndItsSpriteWithIt() throws {
        let device = try device()
        let queue = try XCTUnwrap(device.makeCommandQueue())
        let pass = try CameraPass(device: device, source: try ShaderSources.source(.camera), images: [])
        let camera = ViewCamera(position: .zero, yaw: 0, pitch: 0, verticalFov: 70 * .pi / 180)
        let viewProjection = camera.viewProjection(aspect: 1)
        let depth = try texture(device, LevelRenderer.depthFormat, usage: [.renderTarget, .shaderRead])
        let color = try texture(device, LevelRenderer.frameFormat, usage: [.renderTarget, .shaderRead])

        // (reversed depth: 0 is nothing drawn, 0.5 a wall much nearer than
        // the flare ten units away)
        func draw(clearDepth: Double) throws -> (seen: [Float], centre: Float) {
            let commands = try XCTUnwrap(queue.makeCommandBuffer())
            let clear = MTLRenderPassDescriptor()
            clear.depthAttachment.texture = depth
            clear.depthAttachment.loadAction = .clear
            clear.depthAttachment.storeAction = .store
            clear.depthAttachment.clearDepth = clearDepth
            try XCTUnwrap(commands.makeRenderCommandEncoder(descriptor: clear)).endEncoding()
            let frame = try XCTUnwrap(pass.encodeOcclusion(flare(at: 10), into: commands, depth: depth,
                                                           viewProjection: viewProjection, camera: camera))
            XCTAssertEqual(frame.spriteCount, 1)
            let draw = MTLRenderPassDescriptor()
            draw.colorAttachments[0].texture = color
            draw.colorAttachments[0].loadAction = .clear
            draw.colorAttachments[0].storeAction = .store
            draw.colorAttachments[0].clearColor = MTLClearColor(red: 0, green: 0, blue: 0, alpha: 1)
            draw.depthAttachment.texture = depth
            draw.depthAttachment.loadAction = .load
            draw.depthAttachment.storeAction = .dontCare
            let encoder = try XCTUnwrap(commands.makeRenderCommandEncoder(descriptor: draw))
            pass.encodeSprites(frame, into: encoder)
            encoder.endEncoding()
            commands.commit()
            commands.waitUntilCompleted()
            let pixels = try reds(color, queue: queue)
            return (pass.seen(frame), pixels[(size / 2) * size + size / 2])
        }
        let open = try draw(clearDepth: 0)
        XCTAssertEqual(open.seen, [1], "nothing hides the flare")
        XCTAssertGreaterThan(open.centre, 0.5, "the flare's sprite was not drawn")
        let hidden = try draw(clearDepth: 0.5)
        XCTAssertEqual(hidden.seen, [0], "the wall does not hide the flare")
        XCTAssertEqual(hidden.centre, 0, "a hidden flare's sprite was drawn")
        XCTAssertNil(pass.encodeOcclusion(CameraSprites(sprites: [], occluders: []),
                                          into: try XCTUnwrap(queue.makeCommandBuffer()), depth: depth,
                                          viewProjection: viewProjection, camera: camera))
    }

    func testTheBloomSpreadsWhatIsBrighterThanWhite() throws {
        let device = try device()
        let queue = try XCTUnwrap(device.makeCommandQueue())
        let pass = try CameraPass(device: device, source: try ShaderSources.source(.camera), images: [])
        let frame = try texture(device, LevelRenderer.frameFormat, usage: [.shaderRead], storage: .shared)
        func fill(_ bright: Float) {
            var texels = [Float16](repeating: 0.5, count: size * size * 4)
            for y in (size / 2 - 2)..<(size / 2 + 2) {
                for x in (size / 2 - 2)..<(size / 2 + 2) {
                    for channel in 0..<3 { texels[(y * size + x) * 4 + channel] = Float16(bright) }
                }
            }
            frame.replace(region: MTLRegionMake2D(0, 0, size, size), mipmapLevel: 0, withBytes: texels,
                          bytesPerRow: size * 8)
        }
        func bloom() throws -> [Float] {
            let commands = try XCTUnwrap(queue.makeCommandBuffer())
            let result = try XCTUnwrap(pass.encodeBloom(into: commands, frame: frame))
            commands.commit()
            commands.waitUntilCompleted()
            XCTAssertEqual(result.width, size / 2)
            return try reds(result, queue: queue)
        }
        // (no more than white: nothing blooms)
        fill(1)
        XCTAssertEqual(try bloom().max(), 0)
        fill(20)
        let spread = try bloom()
        let half = size / 2
        let centre = spread[(half / 2) * half + half / 2]
        let aside = spread[(half / 2) * half + half / 2 + 6]
        XCTAssertGreaterThan(centre, 1, "the bright texels did not bloom")
        XCTAssertGreaterThan(aside, 0, "the bloom did not spread")
        XCTAssertLessThan(aside, centre, "the bloom does not fall off")
        XCTAssertLessThan(spread[0], aside, "the bloom does not fall off to the corner")
    }

    func testTheBrightnessIsAveragedAndTheExposureAdaptsToIt() throws {
        let device = try device()
        let queue = try XCTUnwrap(device.makeCommandQueue())
        let pass = try CameraPass(device: device, source: try ShaderSources.source(.camera), images: [])
        let frame = try texture(device, LevelRenderer.frameFormat, usage: [.shaderRead], storage: .shared)
        XCTAssertNil(pass.logLuminance(age: 0), "a brightness before any frame")
        for value: Float in [0.25, 4] {
            let texels = [Float16](repeating: Float16(value), count: size * size * 4)
            frame.replace(region: MTLRegionMake2D(0, 0, size, size), mipmapLevel: 0, withBytes: texels,
                          bytesPerRow: size * 8)
            let commands = try XCTUnwrap(queue.makeCommandBuffer())
            pass.encodeLuminance(into: commands, frame: frame)
            commands.commit()
            commands.waitUntilCompleted()
        }
        XCTAssertEqual(try XCTUnwrap(pass.logLuminance(age: 0)), 2, accuracy: 0.01)
        XCTAssertEqual(try XCTUnwrap(pass.logLuminance(age: 1)), -2, accuracy: 0.01)
        XCTAssertNil(pass.logLuminance(age: 2))

        // brighter than the key, the view darkens, and no further than
        // the limits
        let limits: [Float] = [-1, 1.5]
        var exposure: Float = 0
        for _ in 0..<600 {
            exposure = adaptExposure(current: exposure, logLuminance: 2, limits: limits, seconds: 1 / 60)
        }
        XCTAssertEqual(exposure, 1.5, accuracy: 0.01)
        XCTAssertEqual(adaptExposure(current: exposure, logLuminance: 2, limits: limits, seconds: 0), exposure)
    }

    func testTheSwitchesAndViewsAreReadFromTheShell() {
        let flags: Set<String> = ["render.exposure", "render.lens_flares"]
        let switches = RenderSwitches(flag: { flags.contains($0) }, value: { $0 == "render.view" ? "bloom" : "" })
        XCTAssertTrue(switches.exposure)
        XCTAssertFalse(switches.bloom)
        XCTAssertTrue(switches.lensFlares)
        XCTAssertFalse(switches.glows)
        XCTAssertEqual(switches.view, RenderView.bloom)
        XCTAssertEqual(RenderSwitches().view, .frame)
        XCTAssertTrue(RenderSwitches().bloom)
    }

    func testBloodGulchDrawsItsCameraSprites() throws {
        let level = try StockLevel(map: "bloodgulch")
        let renderer = try level.renderer(VideoSettings(antiAliasing: .native, renderScale: 1))
        XCTAssertEqual(renderer.exposureLimits, [-0.5, 0.5], "blood gulch is outdoors")
        // (a bright soft sprite thirty units ahead of the start, on top of
        // whatever is there)
        let camera = level.camera
        let ahead = camera.position + camera.rotation.act(SIMD3<Float>(1, 0, 0)) * 30
        renderer.sprites = CameraSprites(
            sprites: [CameraSprite(x: ahead.x, y: ahead.y, z: ahead.z, width: 4, height: 4, rotation: 0, red: 4,
                                   green: 4, blue: 4, image: UInt32.max, occluder: UInt32.max, flags: 0)],
            occluders: [])
        let with = try renderer.capture(width: 160, height: 90, camera: camera, objects: [], queue: level.queue)
        XCTAssertEqual(renderer.lastSpriteCount, 1)
        renderer.sprites = CameraSprites(sprites: [], occluders: [])
        let without = try renderer.capture(width: 160, height: 90, camera: camera, objects: [], queue: level.queue)
        XCTAssertEqual(renderer.lastSpriteCount, 0)
        let (lit, dark) = (with.rgb(x: 80, y: 45), without.rgb(x: 80, y: 45))
        XCTAssertGreaterThan(Int(lit[0]), Int(dark[0]), "the sprite added no light")
    }
}
