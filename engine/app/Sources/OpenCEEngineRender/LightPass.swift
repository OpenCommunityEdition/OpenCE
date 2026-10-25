// The dynamic lights (the flashlight, the shots', the effects', the
// scenery's): each frame, the snapshot's lights sorted by the engine into
// the cells the view is cut into (lights.rs), for the level's and the
// objects' shaders to add each light that reaches their cell; each
// light's gel, a cube map, colouring it the way it shines; and the
// nearest few lights' shadows, a cube of depth each drawn from the light
// with the sun's shadow pipelines.

import Metal
import opence_ffiFFI
import simd

/// The shaders' DynamicLight.
struct GPULight {
    var positionRadius: SIMD4<Float>
    /// Its forward, and the cosine its cone ends at.
    var forwardCutoff: SIMD4<Float>
    /// Its gel's up, and the cosine its cone starts to fall off at.
    var upFalloff: SIMD4<Float>
    /// Its colour (linear), and how far its highlights reach.
    var colorSpecular: SIMD4<Float>
    /// Its flags, its gel's slot and its shadow's cube (none: `UInt32.max`).
    var flags: SIMD4<UInt32>
}

/// The shaders' LightCluster: the view the cells cut, and how many lights.
struct LightClusterUniforms {
    /// The eye, and the number of lights.
    var eyeCount: SIMD4<Float> = .zero
    var forward: SIMD4<Float> = .zero
    var left: SIMD4<Float> = .zero
    var up: SIMD4<Float> = .zero
    /// The tangents of half the fields of view across and up, and the
    /// cells' nearest and farthest depths.
    var tangents: SIMD4<Float> = .zero
}

final class LightPass {
    /// How many gels a frame draws with (the lights of others shine white).
    static let gelSlots = 8
    /// How many lights cast shadows, and their cubes' size in texels.
    static let shadowCount = 4
    static let shadowResolution = 256
    /// The nearest depth of a light's shadow cube (its farthest is its reach).
    static let shadowNear: Float = 0.05
    /// The depths the cells span, in world units.
    static let near: Float = 0.1
    static let far: Float = 1024
    /// The cells (OCE_CLUSTER_CELLS, which Swift does not import).
    static let cells = Int(OCE_CLUSTER_X * OCE_CLUSTER_Y * OCE_CLUSTER_Z)

    /// What a frame's shaders are given.
    struct Frame {
        var lights: MTLBuffer
        var cells: MTLBuffer
        var indices: MTLBuffer
        var cluster: LightClusterUniforms
        var gels: [MTLTexture]
        /// The shadowed lights, by their shadow cube.
        var shadowed: [OCELightSnapshot]
    }

    private let device: MTLDevice
    /// Each of the level's lights' gel (by a snapshot's `light`), or nil.
    private let gels: [MTLTexture?]
    private let whiteCube: MTLTexture
    /// Bound for a frame without lights.
    private let none: MTLBuffer
    private let depthState: MTLDepthStencilState
    /// The shadow cubes (one texel until a light casts).
    private(set) var shadows: MTLTexture
    /// The last frame's, for tests.
    private(set) var lastLightCount = 0
    private(set) var lastShadowCount = 0

    init(device: MTLDevice, gels: [MTLTexture?], whiteCube: MTLTexture) throws {
        self.device = device
        self.gels = gels
        self.whiteCube = whiteCube
        let state = MTLDepthStencilDescriptor()
        state.depthCompareFunction = .less
        state.isDepthWriteEnabled = true
        guard
            let none = device.makeBuffer(
                length: max(
                    2 * Self.cells * 4,
                    MemoryLayout<GPULight>.stride), options: .storageModeShared),
            let depthState = device.makeDepthStencilState(descriptor: state),
            let shadows = Self.cubes(device, size: 1)
        else {
            throw LevelRendererError.resources("lights")
        }
        self.none = none
        self.depthState = depthState
        self.shadows = shadows
    }

    private static func cubes(_ device: MTLDevice, size: Int) -> MTLTexture? {
        let descriptor = MTLTextureDescriptor()
        descriptor.textureType = .typeCubeArray
        descriptor.pixelFormat = ShadowPass.format
        descriptor.width = size
        descriptor.height = size
        descriptor.arrayLength = shadowCount
        descriptor.storageMode = .private
        descriptor.usage = [.renderTarget, .shaderRead]
        return device.makeTexture(descriptor: descriptor)
    }

    /// The cells for `camera` seeing a view `aspect` times as wide as high.
    static func clusterView(camera: ViewCamera, aspect: Float) -> OCEClusterView {
        let fov = min(max(camera.verticalFov, 0.01), Float.pi - 0.01)
        let tangentY = tan(fov / 2)
        let rotation = camera.rotation.vector
        return OCEClusterView(
            position: OCEVec3(x: camera.position.x, y: camera.position.y, z: camera.position.z),
            tangent_x: tangentY * max(aspect, 0.01),
            rotation: OCEQuat(x: rotation.x, y: rotation.y, z: rotation.z, w: rotation.w),
            tangent_y: tangentY, near: near, far: far, reserved: 0)
    }

    /// World to the face `face` (+x, -x, +y, -y, +z, -z, as a cube map's
    /// slices are) of a shadow cube at `centre` reaching `far`: the face's
    /// view at 90 degrees, its depth from `shadowNear` (0) to `far` (1).
    static func faceMatrix(face: Int, centre: SIMD3<Float>, far: Float) -> simd_float4x4 {
        // (each face's direction and the way up its texels are, as a cube
        // map is sampled: across is cross(up, look))
        let faces: [(look: SIMD3<Float>, up: SIMD3<Float>)] = [
            ([1, 0, 0], [0, 1, 0]), ([-1, 0, 0], [0, 1, 0]),
            ([0, 1, 0], [0, 0, -1]), ([0, -1, 0], [0, 0, 1]),
            ([0, 0, 1], [0, 1, 0]), ([0, 0, -1], [0, 1, 0]),
        ]
        let (look, up) = faces[min(max(face, 0), 5)]
        let right = cross(up, look)
        let near = shadowNear
        let far = max(far, near * 2)
        let a = far / (far - near)
        let b = -near * far / (far - near)
        return simd_float4x4(rows: [
            SIMD4(right, -dot(right, centre)),
            SIMD4(up, -dot(up, centre)),
            SIMD4(look * a, -dot(look, centre) * a + b),
            SIMD4(look, -dot(look, centre)),
        ])
    }

    /// The frame's lights (already between their ticks) for `camera`.
    func prepare(_ lights: [OCELightSnapshot], camera: ViewCamera, aspect: Float) -> Frame {
        let lights = Array(lights.prefix(Int(OCE_MAX_LIGHTS)))
        lastLightCount = lights.count
        var frame = Frame(
            lights: none, cells: none, indices: none, cluster: LightClusterUniforms(),
            gels: Array(repeating: whiteCube, count: Self.gelSlots), shadowed: [])
        guard !lights.isEmpty else {
            lastShadowCount = 0
            return frame
        }
        var view = Self.clusterView(camera: camera, aspect: aspect)
        var cells = [UInt32](repeating: 0, count: 2 * Self.cells)
        var indices = [UInt16](repeating: 0, count: Int(OCE_MAX_CLUSTER_INDICES))
        let written = lights.withUnsafeBufferPointer { lights in
            cells.withUnsafeMutableBufferPointer { cells in
                indices.withUnsafeMutableBufferPointer { indices in
                    opence_cluster_lights(
                        lights.baseAddress, UInt32(lights.count), &view, cells.baseAddress,
                        indices.baseAddress, UInt32(indices.count))
                }
            }
        }
        var shadowed = [UInt32](repeating: 0, count: Self.shadowCount)
        let shadowedCount = lights.withUnsafeBufferPointer { lights in
            shadowed.withUnsafeMutableBufferPointer { out in
                opence_shadowed_lights(
                    lights.baseAddress, UInt32(lights.count), view.position, out.baseAddress,
                    UInt32(out.count))
            }
        }
        var shadowOf: [Int: UInt32] = [:]
        for (cube, index) in shadowed.prefix(Int(shadowedCount)).enumerated() where Int(index) < lights.count {
            shadowOf[Int(index)] = UInt32(cube)
            frame.shadowed.append(lights[Int(index)])
        }
        // (the gels, a slot each in the order the lights first use them)
        var slots: [Int: UInt32] = [:]
        let records = lights.enumerated().map { index, light -> GPULight in
            var slot = UInt32.max
            if Int(light.light) < gels.count, let gel = gels[Int(light.light)] {
                if let used = slots[Int(light.light)] {
                    slot = used
                } else if slots.count < Self.gelSlots {
                    slot = UInt32(slots.count)
                    slots[Int(light.light)] = slot
                    frame.gels[Int(slot)] = gel
                }
            }
            return GPULight(
                positionRadius: SIMD4(
                    light.position.x, light.position.y, light.position.z,
                    light.radius),
                forwardCutoff: SIMD4(
                    light.forward.x, light.forward.y, light.forward.z,
                    light.cosine_cutoff),
                upFalloff: SIMD4(light.up.x, light.up.y, light.up.z, light.cosine_falloff),
                colorSpecular: SIMD4(
                    light.color.x, light.color.y, light.color.z,
                    light.specular_radius),
                flags: SIMD4(light.flags, slot, shadowOf[index] ?? UInt32.max, 0))
        }
        lastShadowCount = frame.shadowed.count
        // (made each frame: Phase 10 keeps them)
        guard
            let (lightBuffer, cellBuffer, indexBuffer) = HitchLog.shared.note(
                .allocation, "light lists",
                {
                    () -> (MTLBuffer, MTLBuffer, MTLBuffer)? in
                    guard
                        let lights = device.makeBuffer(
                            bytes: records, length: records.count * MemoryLayout<GPULight>.stride,
                            options: .storageModeShared),
                        let cells = device.makeBuffer(
                            bytes: cells, length: cells.count * 4, options: .storageModeShared),
                        let indices = device.makeBuffer(
                            bytes: indices, length: max(Int(written), 1) * 2,
                            options: .storageModeShared)
                    else { return nil }
                    return (lights, cells, indices)
                })
        else {
            return frame
        }
        frame.lights = lightBuffer
        frame.cells = cellBuffer
        frame.indices = indexBuffer
        let forward = camera.rotation.act([1, 0, 0])
        let left = camera.rotation.act([0, 1, 0])
        let up = camera.rotation.act([0, 0, 1])
        frame.cluster = LightClusterUniforms(
            eyeCount: SIMD4(camera.position, Float(lights.count)), forward: SIMD4(forward, 0),
            left: SIMD4(left, 0), up: SIMD4(up, 0),
            tangents: SIMD4(view.tangent_x, view.tangent_y, Self.near, Self.far))
        return frame
    }

    /// Draws the shadowed lights' cubes: each face's casters by `casters`,
    /// called with the encoder (the face's matrix at vertex buffer 1,
    /// `wrap` at sampler 0) and the sun's shadow pipelines.
    func encodeShadows(
        _ frame: Frame, into commands: MTLCommandBuffer, wrap: MTLSamplerState,
        casters: (MTLRenderCommandEncoder) -> Void
    ) {
        guard !frame.shadowed.isEmpty else { return }
        if shadows.width != Self.shadowResolution {
            guard let made = Self.cubes(device, size: Self.shadowResolution) else { return }
            shadows = made
        }
        for (cube, light) in frame.shadowed.enumerated() {
            let centre = SIMD3(light.position.x, light.position.y, light.position.z)
            let far = max(light.radius, light.specular_radius)
            for face in 0..<6 {
                let pass = MTLRenderPassDescriptor()
                pass.depthAttachment.texture = shadows
                pass.depthAttachment.slice = cube * 6 + face
                pass.depthAttachment.loadAction = .clear
                pass.depthAttachment.storeAction = .store
                pass.depthAttachment.clearDepth = 1
                guard let encoder = commands.timedRenderEncoder("light shadows \(cube).\(face)", descriptor: pass)
                else { continue }
                encoder.setCullMode(.none)
                encoder.setDepthStencilState(depthState)
                encoder.setDepthBias(1, slopeScale: 2, clamp: 0.01)
                var matrix = Self.faceMatrix(face: face, centre: centre, far: far)
                encoder.setVertexBytes(&matrix, length: MemoryLayout<simd_float4x4>.stride, index: 1)
                encoder.setFragmentSamplerState(wrap, index: 0)
                casters(encoder)
                encoder.endEncoding()
            }
        }
    }

    /// Binds the frame's lights for the level's and the objects' shaders.
    func bind(_ frame: Frame, into encoder: MTLRenderCommandEncoder) {
        encoder.setFragmentBuffer(frame.lights, offset: 0, index: 5)
        encoder.setFragmentBuffer(frame.cells, offset: 0, index: 6)
        encoder.setFragmentBuffer(frame.indices, offset: 0, index: 7)
        var cluster = frame.cluster
        encoder.setFragmentBytes(&cluster, length: MemoryLayout<LightClusterUniforms>.stride, index: 8)
        encoder.setFragmentTextures(frame.gels, range: 18..<(18 + Self.gelSlots))
        encoder.setFragmentTexture(shadows, index: 26)
    }
}
