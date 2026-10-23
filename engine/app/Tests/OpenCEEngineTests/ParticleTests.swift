import Foundation
import Metal
import OpenCEEngineCore
import OpenCEEngineRender
import opence_ffiFFI
import simd
import XCTest

/// The particles, contrails, weather and detail objects (ParticlePass.swift
/// and Shaders/particle.metal): each row is a quad facing the screen,
/// standing on its axis or spanning its segment, blended with the frame as
/// its blend function says.
final class ParticleTests: XCTestCase {
    private let size = 64

    private func texture(_ device: MTLDevice, _ format: MTLPixelFormat) throws -> MTLTexture {
        let descriptor = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: format, width: size, height: size,
                                                                  mipmapped: false)
        descriptor.usage = [.renderTarget, .shaderRead]
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

    /// A white sprite of half width 1, ten units ahead of the camera.
    private func sprite(blend: UInt32 = 3, alpha: Float = 1, facing: UInt32 = UInt32(OCE_PARTICLE_FACING_SCREEN),
                        axis: SIMD4<Float> = .zero, fade: (Float, Float) = (0, 0),
                        picture: UInt32 = UInt32(OCE_PARTICLE_NO_PICTURE)) -> OCEParticleSnapshot {
        var row = OCEParticleSnapshot()
        row.position = (10, 0, 0, 1)
        row.axis = (axis.x, axis.y, axis.z, axis.w)
        row.color = (1, 1, 1, alpha)
        row.uv = (0, 0, 1, 1)
        row.shape = (0, fade.0, fade.1, 0)
        row.modes = (picture, blend, facing, 0)
        return row
    }

    /// The frame's red at its centre and corner after `rows` are drawn over
    /// a frame of red 0.25, with `details` as the level's detail objects.
    private func draw(_ rows: [OCEParticleSnapshot], details: [OCEParticleSnapshot] = [],
                      eye: SIMD3<Float> = .zero) throws -> (centre: Float, corner: Float) {
        let device = try XCTUnwrap(MTLCreateSystemDefaultDevice(), "no Metal device")
        let queue = try XCTUnwrap(device.makeCommandQueue())
        let pass = try ParticlePass(device: device, source: try ShaderSources.source(.particle), pictures: [],
                                    details: details)
        let frame = try texture(device, LevelRenderer.frameFormat)
        let depth = try texture(device, LevelRenderer.depthFormat)
        let commands = try XCTUnwrap(queue.makeCommandBuffer())
        let clear = MTLRenderPassDescriptor()
        clear.colorAttachments[0].texture = frame
        clear.colorAttachments[0].loadAction = .clear
        clear.colorAttachments[0].clearColor = MTLClearColor(red: 0.25, green: 0.25, blue: 0.25, alpha: 1)
        clear.colorAttachments[0].storeAction = .store
        clear.depthAttachment.texture = depth
        clear.depthAttachment.loadAction = .clear
        // (reversed depth: 0 is the far plane)
        clear.depthAttachment.clearDepth = 0
        clear.depthAttachment.storeAction = .store
        try XCTUnwrap(commands.makeRenderCommandEncoder(descriptor: clear)).endEncoding()
        let camera = ViewCamera(position: eye, yaw: 0, pitch: 0, verticalFov: 70 * .pi / 180)
        pass.encode(into: commands, frame: frame, depth: depth, worldToClip: camera.viewProjection(aspect: 1),
                    eye: eye, right: [0, -1, 0], up: [0, 0, 1], sprites: rows)
        commands.commit()
        commands.waitUntilCompleted()
        let red = try reds(frame, queue: queue)
        return (red[(size / 2) * size + size / 2], red[0])
    }

    func testASpriteFacingTheScreenIsAddedToTheFrame() throws {
        let drawn = try draw([sprite()])
        XCTAssertEqual(drawn.centre, 1.25, accuracy: 0.01)
        XCTAssertEqual(drawn.corner, 0.25, accuracy: 0.01, "the sprite covered more than its square")
    }

    func testTheBlendFunctions() throws {
        // (alpha blend, multiply, double multiply, add, subtract, least,
        // greatest, alpha multiply add)
        let expected: [Float] = [0.625, 0.25, 0.375, 0.75, 0, 0.25, 0.625, 1.125]
        for (blend, value) in expected.enumerated() {
            XCTAssertEqual(try draw([sprite(blend: UInt32(blend), alpha: 0.5)]).centre, value, accuracy: 0.01,
                           "blend function \(blend)")
        }
    }

    func testSpritesOnTheirAxisAndOnSegmentsFaceTheCamera() throws {
        let standing = sprite(facing: UInt32(OCE_PARTICLE_FACING_AXIS), axis: [0, 0, 1, 0])
        XCTAssertEqual(try draw([standing]).centre, 1.25, accuracy: 0.01, "a sprite on its axis was not drawn")
        // (a segment from the sprite's position across the view to its end)
        var segment = sprite(facing: UInt32(OCE_PARTICLE_FACING_SEGMENT), axis: [10, 0, 1, 0])
        segment.position = (10, 0, -1, 1)
        XCTAssertEqual(try draw([segment]).centre, 1.25, accuracy: 0.01, "a strip's segment was not drawn")
        // (one seen end on covers nothing)
        let endOn = sprite(facing: UInt32(OCE_PARTICLE_FACING_SEGMENT), axis: [20, 0, 0, 0])
        XCTAssertEqual(try draw([endOn]).centre, 0.25, accuracy: 0.01, "a strip seen end on was drawn")
    }

    func testSpritesFadeWithDistance() throws {
        XCTAssertEqual(try draw([sprite(fade: (2, 5))]).centre, 0.25, accuracy: 0.01, "a sprite past its fade drew")
        XCTAssertEqual(try draw([sprite(fade: (5, 15))]).centre, 0.75, accuracy: 0.01, "half faded")
        XCTAssertEqual(try draw([sprite(fade: (20, 30))]).centre, 1.25, accuracy: 0.01)
    }

    func testDetailObjectsAreDrawnWithTheFramesSprites() throws {
        XCTAssertEqual(try draw([], details: [sprite()]).centre, 1.25, accuracy: 0.01)
        XCTAssertEqual(try draw([sprite()], details: [sprite()]).centre, 2.25, accuracy: 0.01)
        XCTAssertEqual(try draw([]).centre, 0.25)
    }

    func testRowsThatShareAPictureAreOneRun() {
        let rows = [sprite(picture: 0), sprite(picture: 0), sprite(picture: 2), sprite(picture: 0)]
        let runs = ParticlePass.runs(rows)
        XCTAssertEqual(runs.map(\.first), [0, 2, 3])
        XCTAssertEqual(runs.map(\.count), [2, 1, 1])
        XCTAssertEqual(runs.map(\.picture), [0, 2, 0])
        XCTAssertTrue(ParticlePass.runs([]).isEmpty)
    }

    func testTheSwitchIsReadFromTheShell() {
        XCTAssertTrue(RenderSwitches().particles)
        XCTAssertFalse(RenderSwitches(flag: { $0 != "render.particles" }, value: { _ in "" }).particles)
    }
}
