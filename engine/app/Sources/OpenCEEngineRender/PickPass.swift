// The dev panel's pick: the last frame's level and objects drawn again
// into a target of one pixel, each object as its id, through the view
// narrowed to the pixel under the cursor, and that pixel read back. The
// level's surfaces are drawn as no object, so that a wall hides what is
// behind it; the first-person weapon and the sky are not drawn.

import Metal
import simd

final class PickPass {
    /// What the target holds where no object is drawn.
    static let none = UInt32.max
    static let format = MTLPixelFormat.r32Uint

    struct Pipelines {
        var level: MTLRenderPipelineState
        var model: MTLRenderPipelineState
    }

    private(set) var pipelines: Pipelines
    private let depthState: MTLDepthStencilState
    private let target: MTLTexture
    private let depth: MTLTexture
    /// Its own queue, waited on: a pick is a click's, not a frame's.
    private let queue: MTLCommandQueue

    init(device: MTLDevice, library: MTLLibrary, levelStride: Int, modelStride: Int) throws {
        pipelines = try Self.pipelines(
            device: device, library: library, levelStride: levelStride,
            modelStride: modelStride)
        // reversed depth, as the frame's: nearer is greater
        let state = MTLDepthStencilDescriptor()
        state.depthCompareFunction = .greater
        state.isDepthWriteEnabled = true
        let color = MTLTextureDescriptor.texture2DDescriptor(
            pixelFormat: Self.format, width: 1, height: 1,
            mipmapped: false)
        color.usage = [.renderTarget]
        color.storageMode = .shared
        let depthDescriptor = MTLTextureDescriptor.texture2DDescriptor(
            pixelFormat: LevelRenderer.depthFormat,
            width: 1, height: 1, mipmapped: false)
        depthDescriptor.usage = [.renderTarget]
        depthDescriptor.storageMode = .private
        guard let depthState = device.makeDepthStencilState(descriptor: state),
            let target = device.makeTexture(descriptor: color),
            let depth = device.makeTexture(descriptor: depthDescriptor),
            let queue = device.makeCommandQueue()
        else {
            throw LevelRendererError.resources("pick target")
        }
        self.depthState = depthState
        self.target = target
        self.depth = depth
        self.queue = queue
    }

    static func pipelines(
        device: MTLDevice, library: MTLLibrary, levelStride: Int,
        modelStride: Int
    ) throws -> Pipelines {
        func pipeline(vertex: String, descriptor vertices: MTLVertexDescriptor) throws -> MTLRenderPipelineState {
            let descriptor = MTLRenderPipelineDescriptor()
            descriptor.vertexFunction = try library.required(vertex)
            descriptor.fragmentFunction = try library.required("pick_fragment")
            descriptor.vertexDescriptor = vertices
            descriptor.colorAttachments[0].pixelFormat = format
            descriptor.depthAttachmentPixelFormat = LevelRenderer.depthFormat
            return try device.makeRenderPipelineState(descriptor: descriptor)
        }
        do {
            return Pipelines(
                level: try pipeline(
                    vertex: "shadow_level_vertex",
                    descriptor: LevelRenderer.vertexDescriptor(stride: levelStride)),
                model: try pipeline(
                    vertex: "shadow_model_vertex",
                    descriptor: ModelPass.vertexDescriptor(stride: modelStride)))
        } catch {
            throw LevelRendererError.shaders("\(error)")
        }
    }

    func use(_ pipelines: Pipelines) {
        self.pipelines = pipelines
    }

    /// The matrix that takes a view's clip space to that of its pixel
    /// (`x`, `y`) from the top left, of `width` by `height`, drawn alone
    /// in a target of one pixel.
    static func narrowing(x: Int, y: Int, width: Int, height: Int) -> simd_float4x4 {
        let scaleX = Float(max(width, 1))
        let scaleY = Float(max(height, 1))
        // (the pixel's middle in the view's normalized coordinates)
        let middleX = (2 * (Float(x) + 0.5)) / scaleX - 1
        let middleY = 1 - (2 * (Float(y) + 0.5)) / scaleY
        return simd_float4x4(
            columns: (
                SIMD4(scaleX, 0, 0, 0), SIMD4(0, scaleY, 0, 0), SIMD4(0, 0, 1, 0),
                SIMD4(-scaleX * middleX, -scaleY * middleY, 0, 1)
            ))
    }

    /// The id drawn at the pixel `matrix` narrows the view to, nil for
    /// none: `level` draws the level's surfaces and `objects` the objects
    /// (each binding its own id at fragment buffer 1), with the matrix at
    /// vertex buffer 1.
    func pick(
        matrix: simd_float4x4,
        level: (MTLRenderCommandEncoder) -> Void, objects: (MTLRenderCommandEncoder) -> Void
    ) -> UInt32? {
        let pass = MTLRenderPassDescriptor()
        pass.colorAttachments[0].texture = target
        pass.colorAttachments[0].loadAction = .clear
        pass.colorAttachments[0].clearColor = MTLClearColor(red: Double(Self.none), green: 0, blue: 0, alpha: 0)
        pass.colorAttachments[0].storeAction = .store
        pass.depthAttachment.texture = depth
        pass.depthAttachment.loadAction = .clear
        pass.depthAttachment.clearDepth = 0
        pass.depthAttachment.storeAction = .dontCare
        guard let commands = queue.makeCommandBuffer(),
            let encoder = commands.timedRenderEncoder("pick", descriptor: pass)
        else { return nil }
        encoder.setDepthStencilState(depthState)
        encoder.setCullMode(.none)
        var narrowed = matrix
        encoder.setVertexBytes(&narrowed, length: MemoryLayout<simd_float4x4>.stride, index: 1)
        var none = Self.none
        encoder.setRenderPipelineState(pipelines.level)
        encoder.setFragmentBytes(&none, length: MemoryLayout<UInt32>.stride, index: 1)
        level(encoder)
        encoder.setRenderPipelineState(pipelines.model)
        objects(encoder)
        encoder.endEncoding()
        commands.commit()
        commands.waitUntilCompleted()
        var picked = Self.none
        target.getBytes(
            &picked, bytesPerRow: MemoryLayout<UInt32>.stride, from: MTLRegionMake2D(0, 0, 1, 1),
            mipmapLevel: 0)
        return picked == Self.none ? nil : picked
    }
}
