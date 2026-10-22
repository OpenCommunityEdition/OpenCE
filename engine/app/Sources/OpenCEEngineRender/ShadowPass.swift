// The sun's cascaded shadow maps: each frame, the level's surfaces drawn
// from the sun into one set of maps, which are copied into a second that
// the objects are then drawn into too. The objects are lit by the second;
// the level's lightmaps already hold its own shadows, so its surfaces
// take the sun's light out only where the second is shadowed and the
// first is not (an object's shadow), and the two agree with the bake
// wherever the level's surfaces are what casts.
// The cascades' splits and their views from the sun come from the
// engine (render_shadow.rs).

import Metal
import OpenCEEngineCore
import simd

/// The shaders' Shadows.
struct ShadowUniforms {
    var cascades: (simd_float4x4, simd_float4x4, simd_float4x4, simd_float4x4) =
        (matrix_identity_float4x4, matrix_identity_float4x4, matrix_identity_float4x4, matrix_identity_float4x4)
    /// Where each cascade ends, ahead of the camera.
    var far: SIMD4<Float> = .zero
    /// The view's forward, and 1 when the sun's shadows are drawn.
    var forward: SIMD4<Float> = .zero
    /// The width of a texel of each cascade, in world units.
    var texel: SIMD4<Float> = .zero
}

final class ShadowPass {
    /// The maps' width and height, in texels.
    static let resolution = 2048
    /// How far the cascades reach ahead of the camera, in world units.
    static let distance: Float = 64
    static let cascadeCount = 4
    static let format = MTLPixelFormat.depth32Float

    /// The pipelines that draw a kind of caster: depth alone, or cut out by
    /// its base map where it is alpha-tested.
    struct Casting {
        var depth: MTLRenderPipelineState
        var cutout: MTLRenderPipelineState
    }

    struct Pipelines {
        var level: Casting
        var model: Casting
    }

    private let device: MTLDevice
    private var pipelines: Pipelines
    private let depthState: MTLDepthStencilState
    /// The level's casters alone, then the level's and the objects' (made
    /// with the first sun; one texel until then).
    private(set) var levelMaps: MTLTexture
    private(set) var maps: MTLTexture
    /// What the last frame's shaders were given.
    private(set) var uniforms = ShadowUniforms()

    init(device: MTLDevice, library: MTLLibrary, levelStride: Int, modelStride: Int) throws {
        self.device = device
        pipelines = try Self.pipelines(device: device, library: library, levelStride: levelStride,
                                       modelStride: modelStride)
        let state = MTLDepthStencilDescriptor()
        state.depthCompareFunction = .less
        state.isDepthWriteEnabled = true
        guard let depthState = device.makeDepthStencilState(descriptor: state),
              let none = Self.maps(device, size: 1) else {
            throw LevelRendererError.resources("shadow maps")
        }
        self.depthState = depthState
        levelMaps = none
        maps = none
    }

    static func pipelines(device: MTLDevice, library: MTLLibrary, levelStride: Int,
                          modelStride: Int) throws -> Pipelines {
        func casting(vertex: String, descriptor vertices: MTLVertexDescriptor) throws -> Casting {
            let descriptor = MTLRenderPipelineDescriptor()
            descriptor.vertexFunction = try library.required(vertex)
            descriptor.vertexDescriptor = vertices
            descriptor.depthAttachmentPixelFormat = format
            let depth = try device.makeRenderPipelineState(descriptor: descriptor)
            descriptor.fragmentFunction = try library.required("shadow_cutout_fragment")
            return Casting(depth: depth, cutout: try device.makeRenderPipelineState(descriptor: descriptor))
        }
        do {
            return Pipelines(
                level: try casting(vertex: "shadow_level_vertex",
                                   descriptor: LevelRenderer.vertexDescriptor(stride: levelStride)),
                model: try casting(vertex: "shadow_model_vertex",
                                   descriptor: ModelPass.vertexDescriptor(stride: modelStride)))
        } catch {
            throw LevelRendererError.shaders("\(error)")
        }
    }

    func use(_ pipelines: Pipelines) {
        self.pipelines = pipelines
    }

    private static func maps(_ device: MTLDevice, size: Int) -> MTLTexture? {
        let descriptor = MTLTextureDescriptor()
        descriptor.textureType = .type2DArray
        descriptor.pixelFormat = format
        descriptor.width = size
        descriptor.height = size
        descriptor.arrayLength = cascadeCount
        descriptor.storageMode = .private
        descriptor.usage = [.renderTarget, .shaderRead]
        return device.makeTexture(descriptor: descriptor)
    }

    /// The cascades for `camera` seeing a view `aspect` times as wide as
    /// high, with the sun towards `towards` (none: no shadows).
    static func uniforms(camera: ViewCamera, aspect: Float, towards: SIMD3<Float>?) -> ShadowUniforms {
        var uniforms = ShadowUniforms()
        guard let towards else { return uniforms }
        let forward = camera.rotation.act([1, 0, 0])
        let cascades = sunShadowCascades(
            position: [camera.position.x, camera.position.y, camera.position.z],
            forward: [forward.x, forward.y, forward.z], verticalFov: camera.verticalFov, aspect: aspect,
            near: ViewCamera.near, distance: distance, towardsSun: [towards.x, towards.y, towards.z],
            resolution: UInt32(resolution))
        guard cascades.count == cascadeCount, cascades.allSatisfy({ $0.viewProjection.count == 16 }) else {
            return uniforms
        }
        let matrices = cascades.map { cascade in
            let m = cascade.viewProjection
            return simd_float4x4(columns: (SIMD4(m[0], m[1], m[2], m[3]), SIMD4(m[4], m[5], m[6], m[7]),
                                           SIMD4(m[8], m[9], m[10], m[11]), SIMD4(m[12], m[13], m[14], m[15])))
        }
        uniforms.cascades = (matrices[0], matrices[1], matrices[2], matrices[3])
        uniforms.far = SIMD4(cascades.map(\.far))
        uniforms.texel = SIMD4(cascades.map(\.texel))
        uniforms.forward = SIMD4(forward, 1)
        return uniforms
    }

    /// Draws the casters for `camera` into the maps, the level's by
    /// `level` and the objects' by `objects`, each called once a cascade
    /// with the encoder (its light at vertex buffer 1, `wrap` at sampler
    /// 0) and the pipelines for its kind; with no sun, only clears the
    /// uniforms.
    func encode(into commands: MTLCommandBuffer, camera: ViewCamera, aspect: Float, towards: SIMD3<Float>?,
                wrap: MTLSamplerState, level: (MTLRenderCommandEncoder, Casting) -> Void,
                objects: (MTLRenderCommandEncoder, Casting) -> Void) {
        uniforms = Self.uniforms(camera: camera, aspect: aspect, towards: towards)
        guard uniforms.forward.w > 0 else { return }
        if maps.width != Self.resolution {
            guard let levelMaps = Self.maps(device, size: Self.resolution),
                  let maps = Self.maps(device, size: Self.resolution) else {
                uniforms = ShadowUniforms()
                return
            }
            self.levelMaps = levelMaps
            self.maps = maps
        }
        let matrices = [uniforms.cascades.0, uniforms.cascades.1, uniforms.cascades.2, uniforms.cascades.3]
        func draw(into target: MTLTexture, clear: Bool, label: String, casting: Casting,
                  casters: (MTLRenderCommandEncoder, Casting) -> Void) {
            for (slice, matrix) in matrices.enumerated() {
                let pass = MTLRenderPassDescriptor()
                pass.depthAttachment.texture = target
                pass.depthAttachment.slice = slice
                pass.depthAttachment.loadAction = clear ? .clear : .load
                pass.depthAttachment.storeAction = .store
                pass.depthAttachment.clearDepth = 1
                guard let encoder = commands.makeRenderCommandEncoder(descriptor: pass) else { continue }
                encoder.label = "\(label) shadows \(slice)"
                encoder.setCullMode(.none)
                encoder.setDepthStencilState(depthState)
                // (and pushed back a little more where the surface slopes
                // away from the sun, against shadow acne)
                encoder.setDepthBias(1, slopeScale: 2, clamp: 0.01)
                var light = matrix
                encoder.setVertexBytes(&light, length: MemoryLayout<simd_float4x4>.stride, index: 1)
                encoder.setFragmentSamplerState(wrap, index: 0)
                casters(encoder, casting)
                encoder.endEncoding()
            }
        }
        draw(into: levelMaps, clear: true, label: "level", casting: pipelines.level, casters: level)
        guard let blit = commands.makeBlitCommandEncoder() else { return }
        blit.label = "shadows copied"
        blit.copy(from: levelMaps, to: maps)
        blit.endEncoding()
        draw(into: maps, clear: false, label: "object", casting: pipelines.model, casters: objects)
    }
}
