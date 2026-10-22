import Foundation
import Metal
import OpenCEEngineCore
import OpenCEEngineRender
import simd
import XCTest

/// The decals laid on the level (DecalPass.swift and Shaders/decal.metal):
/// a decal is blended onto the level's surfaces inside its box that face
/// its way, and on nothing else.
final class DecalTests: XCTestCase {
    private let size = 64

    private func texture(_ device: MTLDevice, _ format: MTLPixelFormat, usage: MTLTextureUsage) throws -> MTLTexture {
        let descriptor = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: format, width: size, height: size,
                                                                  mipmapped: false)
        descriptor.usage = usage
        descriptor.storageMode = .private
        return try XCTUnwrap(device.makeTexture(descriptor: descriptor))
    }

    /// The red of each texel of `texture` (rgba16Float), copied out.
    private func reds(_ texture: MTLTexture, queue: MTLCommandQueue) throws -> [Float] {
        let length = texture.width * texture.height * 8
        let buffer = try XCTUnwrap(queue.device.makeBuffer(length: length, options: .storageModeShared))
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

    /// A decal two units across on a wall ten units ahead, facing back at
    /// the camera.
    private func decal(blend: UInt32, red: Float, facing: Float = -1) -> RenderDecal {
        RenderDecal(x: 10, y: 0, z: 0, upX: facing, upY: 0, upZ: 0, forwardX: 0, forwardY: 1, forwardZ: 0, leftX: 0,
                    leftY: 0, leftZ: 1, forwardLeast: -1, forwardGreatest: 1, leftLeast: -1, leftGreatest: 1,
                    depth: 0.5, u0: 0, v0: 0, u1: 1, v1: 1, texture: UInt32.max, red: red, green: 1, blue: 1,
                    alpha: 1, blend: blend, layer: 0)
    }

    /// The frame's red at its centre and corner after `decals` are laid on
    /// a grey wall whose surfaces are `kind` (1: the level's).
    private func lay(_ decals: [RenderDecal], kind: Float = 1, eye: SIMD3<Float> = .zero) throws
        -> (centre: Float, corner: Float) {
        let device = try XCTUnwrap(MTLCreateSystemDefaultDevice(), "no Metal device")
        let queue = try XCTUnwrap(device.makeCommandQueue())
        let pass = try DecalPass(device: device, source: try ShaderSources.source(.decal), decals: decals,
                                 texture: { _ in nil })
        XCTAssertEqual(pass.count, decals.count)
        let camera = ViewCamera(position: eye, yaw: 0, pitch: 0, verticalFov: 70 * .pi / 180)
        let viewProjection = camera.viewProjection(aspect: 1)
        let wall = viewProjection * SIMD4<Float>(10, 0, 0, 1)
        let frame = try texture(device, LevelRenderer.frameFormat, usage: [.renderTarget, .shaderRead])
        let depth = try texture(device, LevelRenderer.depthFormat, usage: [.renderTarget, .shaderRead])
        let surface = try texture(device, .rgba16Float, usage: [.renderTarget, .shaderRead])
        let commands = try XCTUnwrap(queue.makeCommandBuffer())
        let clear = MTLRenderPassDescriptor()
        clear.colorAttachments[0].texture = frame
        clear.colorAttachments[0].loadAction = .clear
        clear.colorAttachments[0].storeAction = .store
        clear.colorAttachments[0].clearColor = MTLClearColor(red: 0.5, green: 0.5, blue: 0.5, alpha: 1)
        clear.colorAttachments[1].texture = surface
        clear.colorAttachments[1].loadAction = .clear
        clear.colorAttachments[1].storeAction = .store
        clear.colorAttachments[1].clearColor = MTLClearColor(red: -1, green: 0, blue: 0, alpha: Double(kind))
        clear.depthAttachment.texture = depth
        clear.depthAttachment.loadAction = .clear
        clear.depthAttachment.storeAction = .store
        clear.depthAttachment.clearDepth = Double(wall.z / wall.w)
        try XCTUnwrap(commands.makeRenderCommandEncoder(descriptor: clear)).endEncoding()
        pass.encode(into: commands, frame: frame, depth: depth, surface: surface, viewProjection: viewProjection,
                    eye: eye)
        commands.commit()
        commands.waitUntilCompleted()
        let pixels = try reds(frame, queue: queue)
        return (pixels[(size / 2) * size + size / 2], pixels[0])
    }

    func testADecalIsBlendedOntoTheWallInsideItsBox() throws {
        let multiplied = try lay([decal(blend: 1, red: 0.5)])
        XCTAssertEqual(multiplied.centre, 0.25, accuracy: 0.01, "the decal did not multiply the wall")
        XCTAssertEqual(multiplied.corner, 0.5, "the decal reached past its box")
        let added = try lay([decal(blend: 3, red: 0.5)])
        XCTAssertEqual(added.centre, 1, accuracy: 0.01, "the decal was not added")
        let blended = try lay([decal(blend: 0, red: 0)])
        XCTAssertEqual(blended.centre, 0, accuracy: 0.01, "the decal was not alpha blended")
        // (two in order: the second over the first)
        let both = try lay([decal(blend: 1, red: 0.5), decal(blend: 3, red: 0.25)])
        XCTAssertEqual(both.centre, 0.5, accuracy: 0.01, "the decals were not laid in order")
        // (from inside its box, it is laid once)
        let inside = try lay([decal(blend: 1, red: 0.5)], eye: SIMD3(9.8, 0, 0))
        XCTAssertEqual(inside.centre, 0.25, accuracy: 0.01, "seen from inside its box")
    }

    func testADecalIsNotLaidOnObjectsOrSurfacesFacingAway() throws {
        let object = try lay([decal(blend: 1, red: 0.5)], kind: 2)
        XCTAssertEqual(object.centre, 0.5, "a decal was laid on an object")
        let away = try lay([decal(blend: 1, red: 0.5, facing: 1)])
        XCTAssertEqual(away.centre, 0.5, "a decal was laid on a surface facing away from it")
        XCTAssertEqual(try lay([]).centre, 0.5)
    }

    func testTheSwitchIsReadFromTheShell() {
        XCTAssertTrue(RenderSwitches().decals)
        XCTAssertFalse(RenderSwitches(flag: { $0 != "render.decals" }, value: { _ in "" }).decals)
    }

    func testBloodGulchLaysItsDecals() throws {
        let level = try StockLevel(map: "bloodgulch")
        let renderer = try level.renderer(VideoSettings(antiAliasing: .native, renderScale: 1))
        XCTAssertGreaterThan(renderer.decalCount, 0, "blood gulch has no decals")
        let with = try renderer.capture(width: 160, height: 90, camera: level.camera, objects: [], queue: level.queue)
        renderer.switches.decals = false
        let without = try renderer.capture(width: 160, height: 90, camera: level.camera, objects: [],
                                           queue: level.queue)
        var changed = 0
        for y in 0..<90 {
            for x in 0..<160 where with.rgb(x: x, y: y) != without.rgb(x: x, y: y) {
                changed += 1
            }
        }
        XCTAssertGreaterThan(changed, 0, "no decal is seen from the start")
    }
}
