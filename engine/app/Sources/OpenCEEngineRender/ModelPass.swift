// The objects' models: their vertices and triangles made into buffers once,
// and each frame each object drawn at the level of detail its size on
// screen chooses, each of its regions as the permutation the snapshot
// names, its vertices moved by two of its nodes' matrices (the nodes'
// matrices in the world times their inverses at the default pose), lit
// by its normals until the level's lights are drawn.

import Metal
import OpenCEEngineCore
import simd

/// An object to draw, between its last two ticks.
public struct DrawnObject {
    /// Its render model, among the level's.
    public var model: Int
    /// Where it is in the world.
    public var transform: simd_float4x4
    /// The middle of its bounding sphere.
    public var center: SIMD3<Float>
    /// Its bounding sphere's radius.
    public var radius: Float
    /// The permutation each of its model's regions is drawn with.
    public var regions: [UInt8]
    /// Its nodes' matrices in the world; with none, every node is where
    /// the object is, at the default pose.
    public var nodes: [simd_float4x4]
    /// The first-person weapon or hands: drawn over the world, at the
    /// highest level of detail.
    public var firstPerson: Bool

    public init(model: Int, transform: simd_float4x4, center: SIMD3<Float>, radius: Float, regions: [UInt8],
                nodes: [simd_float4x4], firstPerson: Bool = false) {
        self.firstPerson = firstPerson
        self.model = model
        self.transform = transform
        self.center = center
        self.radius = radius
        self.regions = regions
        self.nodes = nodes
    }

    /// The game's matrix (a point `p` is `scale * (p.x * forward + p.y *
    /// left + p.z * up) + position`) as a 4x4 one.
    public static func matrix(forward: SIMD3<Float>, left: SIMD3<Float>, up: SIMD3<Float>,
                              position: SIMD3<Float>, scale: Float) -> simd_float4x4 {
        simd_float4x4(columns: (SIMD4(forward * scale, 0), SIMD4(left * scale, 0), SIMD4(up * scale, 0),
                                SIMD4(position, 1)))
    }
}

/// The model vertex shader's ModelObject.
struct ModelObjectUniforms {
    var firstNode: UInt32
    var nodeCount: UInt32
}

final class ModelPass {
    struct Draw {
        var firstIndex: Int
        var indexCount: Int
        var blended: Bool
        /// Of a shader class not imported yet (not drawn in the sky).
        var plain: Bool
        var uniforms: MaterialUniforms
        /// Base, detail, and two unused; or a chicago shader's four maps.
        var textures: [MTLTexture]
    }

    struct Model {
        var cutoffs: [Float]
        /// Each node's matrix at the default pose, inverted.
        var defaultInverses: [simd_float4x4]
        /// For each region, each permutation's geometry at each level of
        /// detail (one nearby where the permutation has none at a level).
        var regions: [[[Int?]]]
    }

    let vertices: MTLBuffer
    let indices: MTLBuffer
    let models: [Model]
    let geometries: [[Draw]]
    private var opaque: MTLRenderPipelineState
    private var blended: MTLRenderPipelineState
    private var skyOpaque: MTLRenderPipelineState
    private var skyBlended: MTLRenderPipelineState
    private let vertexStride: Int
    private let skyDepth: MTLDepthStencilState
    /// (one per frame in flight, grown as needed)
    private var skins: [MTLBuffer?] = [nil, nil, nil]
    private var frame = 0

    /// The geometries' parts drawn in the last frame, for tests.
    private(set) var lastPartCount = 0
    /// The sky's parts drawn in the last frame, for tests.
    private(set) var lastSkyPartCount = 0
    /// The first-person parts drawn in the last frame, for tests.
    private(set) var lastFirstPersonPartCount = 0

    init(device: MTLDevice, library: MTLLibrary, geometry: LevelGeometry, textures: [MTLTexture?],
         white: MTLTexture) throws {
        guard let vertices = LevelRenderer.buffer(device, geometry.modelVertices()),
              let indices = LevelRenderer.buffer(device, geometry.modelIndices()) else {
            throw LevelRendererError.resources("model buffers")
        }
        self.vertices = vertices
        self.indices = indices
        let indexCount = geometry.modelIndices().count / 4
        let materials = geometry.materials()
        func texture(_ index: UInt32?) -> MTLTexture? {
            index.flatMap { Int($0) < textures.count ? textures[Int($0)] : nil }
        }
        let rendered = geometry.models()
        var geometries: [[Draw]] = []
        var models: [Model] = []
        for model in rendered {
            let firstGeometry = geometries.count
            for made in model.geometries {
                var draws: [Draw] = []
                for part in made.parts {
                    guard Int(part.material) < materials.count,
                          Int(part.firstIndex) + Int(part.indexCount) <= indexCount else { continue }
                    let material = materials[Int(part.material)]
                    let maps = [texture(material.baseMap), texture(material.primaryDetailMap)]
                    var bits: UInt32 = 0
                    if maps[0] != nil { bits |= MapBit.base }
                    if maps[1] != nil { bits |= MapBit.primary }
                    let pair = { (values: [Float], fallback: Float) -> SIMD2<Float> in
                        values.count == 2 ? SIMD2(values[0], values[1]) : SIMD2(repeating: fallback)
                    }
                    var uniforms = MaterialUniforms(
                        baseScale: pair(material.baseScale, 1), baseOffset: pair(material.baseOffset, 0),
                        primaryScale: material.primaryDetailScale, secondaryScale: 1, maps: bits,
                        detailFunction: UInt32(material.detailFunction), environmentKind: 0,
                        alphaTested: material.kind == .alphaTested ? 1 : 0,
                        fogMode: FrameUniforms.fogMode(material.kind == .blended ? .alphaBlend : nil))
                    let chicagoTextures = uniforms.chicago(material, texture: texture, white: white)
                    draws.append(Draw(firstIndex: Int(part.firstIndex), indexCount: Int(part.indexCount),
                                      blended: material.kind == .blended, plain: material.kind == .plain,
                                      uniforms: uniforms,
                                      textures: chicagoTextures ?? maps.map { $0 ?? white } + [white, white]))
                }
                geometries.append(draws)
            }
            let geometryCount = model.geometries.count
            let regions = model.regions.map { region in
                region.permutations.map { permutation in
                    (0..<5).map { level -> Int? in
                        Self.nearest(permutation.geometries, level: level)
                            .flatMap { Int($0) < geometryCount ? firstGeometry + Int($0) : nil }
                    }
                }
            }
            let inverses = stride(from: 0, to: model.defaultInverses.count - 12, by: 13).map { at in
                let value = { (index: Int) in SIMD3(model.defaultInverses[at + index],
                                                    model.defaultInverses[at + index + 1],
                                                    model.defaultInverses[at + index + 2]) }
                return DrawnObject.matrix(forward: value(1), left: value(4), up: value(7), position: value(10),
                                          scale: model.defaultInverses[at])
            }
            models.append(Model(cutoffs: model.detailCutoffs, defaultInverses: inverses, regions: regions))
        }
        self.models = models
        self.geometries = geometries

        vertexStride = Int(geometry.modelVertexStride())
        let made = try Self.pipelines(device: device, library: library, stride: vertexStride)
        (opaque, blended, skyOpaque, skyBlended) = (made.opaque, made.blended, made.skyOpaque, made.skyBlended)
        // the sky is behind everything: drawn first, over the cleared
        // depth, without writing it
        let skyDepthDescriptor = MTLDepthStencilDescriptor()
        skyDepthDescriptor.depthCompareFunction = .always
        skyDepthDescriptor.isDepthWriteEnabled = false
        guard let skyDepth = device.makeDepthStencilState(descriptor: skyDepthDescriptor) else {
            throw LevelRendererError.resources("sky depth state")
        }
        self.skyDepth = skyDepth
    }

    /// The objects' and the sky's pipelines.
    struct Pipelines {
        var opaque: MTLRenderPipelineState
        var blended: MTLRenderPipelineState
        var skyOpaque: MTLRenderPipelineState
        var skyBlended: MTLRenderPipelineState
    }

    private static func pipelines(device: MTLDevice, library: MTLLibrary, stride: Int) throws -> Pipelines {
        func pipeline(blended: Bool, sky: Bool = false) throws -> MTLRenderPipelineState {
            let descriptor = MTLRenderPipelineDescriptor()
            descriptor.vertexFunction = try library.required(sky ? "sky_vertex" : "model_vertex")
            descriptor.fragmentFunction = try library.required(sky ? "sky_fragment" : "model_fragment")
            descriptor.vertexDescriptor = Self.vertexDescriptor(stride: stride)
            descriptor.colorAttachments[0].pixelFormat = LevelRenderer.frameFormat
            descriptor.depthAttachmentPixelFormat = LevelRenderer.depthFormat
            if blended {
                let color = descriptor.colorAttachments[0]!
                color.isBlendingEnabled = true
                color.sourceRGBBlendFactor = .sourceAlpha
                color.destinationRGBBlendFactor = .oneMinusSourceAlpha
                color.sourceAlphaBlendFactor = .zero
                color.destinationAlphaBlendFactor = .one
            }
            return try device.makeRenderPipelineState(descriptor: descriptor)
        }
        do {
            return Pipelines(opaque: try pipeline(blended: false), blended: try pipeline(blended: true),
                             skyOpaque: try pipeline(blended: false, sky: true),
                             skyBlended: try pipeline(blended: true, sky: true))
        } catch {
            throw LevelRendererError.shaders("\(error)")
        }
    }

    /// The pipelines from a library compiled again (not yet used).
    func pipelines(library: MTLLibrary) throws -> Pipelines {
        try Self.pipelines(device: library.device, library: library, stride: vertexStride)
    }

    func use(_ pipelines: Pipelines) {
        (opaque, blended, skyOpaque, skyBlended) = (pipelines.opaque, pipelines.blended, pipelines.skyOpaque,
                                                    pipelines.skyBlended)
    }

    /// A permutation's geometry at `level`, or the nearest below it, or
    /// above it.
    static func nearest(_ geometries: [UInt32?], level: Int) -> UInt32? {
        guard !geometries.isEmpty, level >= 0 else { return nil }
        let below = (0...min(level, geometries.count - 1)).reversed().lazy.compactMap { geometries[$0] }.first
        return below ?? geometries[min(level, geometries.count - 1)...].lazy.compactMap { $0 }.first
    }

    static func vertexDescriptor(stride: Int) -> MTLVertexDescriptor {
        let descriptor = MTLVertexDescriptor()
        let attributes: [(MTLVertexFormat, Int)] = [(.float3, 0), (.float3, 12), (.float2, 24), (.ushort2, 32),
                                                    (.float2, 36)]
        for (index, (format, offset)) in attributes.enumerated() {
            descriptor.attributes[index].format = format
            descriptor.attributes[index].offset = offset
            descriptor.attributes[index].bufferIndex = 0
        }
        descriptor.layouts[0].stride = stride
        return descriptor
    }

    /// What to draw of each object: its model's parts and its nodes' skin
    /// matrices, from `camera` in a view `viewportHeight` pixels high.
    struct Prepared {
        var skins: MTLBuffer?
        var objects: [(uniforms: ModelObjectUniforms, parts: [Draw])]
        /// The sky's, if the level has one.
        var sky: (uniforms: ModelObjectUniforms, parts: [Draw])?
        /// The first-person weapon and hands.
        var firstPerson: [(uniforms: ModelObjectUniforms, parts: [Draw])] = []
    }

    /// The highest level of detail, which the sky is always drawn at.
    static let highestLevel = 4

    func prepare(_ objects: [DrawnObject], sky: DrawnObject? = nil, camera: ViewCamera, viewportHeight: Int,
                 device: MTLDevice) -> Prepared {
        let forward = camera.rotation.act([1, 0, 0])
        var matrices: [simd_float4x4] = []
        var drawn: [(uniforms: ModelObjectUniforms, parts: [Draw])] = []
        var skyDrawn: (uniforms: ModelObjectUniforms, parts: [Draw])?
        var firstPerson: [(uniforms: ModelObjectUniforms, parts: [Draw])] = []
        let all = (sky.map { [$0] } ?? []) + objects
        for (index, object) in all.enumerated() where models.indices.contains(object.model) {
            let isSky = sky != nil && index == 0
            let model = models[object.model]
            let level: Int
            if isSky || object.firstPerson {
                level = Self.highestLevel
            } else {
                let depth = dot(object.center - camera.position, forward)
                guard depth > -object.radius else { continue }
                let pixels = sphereDiameterPixels(depth: depth, radius: object.radius,
                                                  verticalFov: camera.verticalFov,
                                                  viewportHeight: Float(viewportHeight))
                guard let chosen = modelLevelOfDetail(detailCutoffs: model.cutoffs, pixels: pixels) else { continue }
                level = Int(chosen)
            }
            var parts: [Draw] = []
            for (index, region) in model.regions.enumerated() where !region.isEmpty {
                let chosen = index < object.regions.count ? Int(object.regions[index]) : 0
                let permutation = region[min(chosen, region.count - 1)]
                if let geometry = permutation[level] {
                    parts += geometries[geometry]
                }
            }
            guard !parts.isEmpty else { continue }
            let nodeCount = max(model.defaultInverses.count, 1)
            let first = matrices.count
            for node in 0..<nodeCount {
                if node < object.nodes.count, node < model.defaultInverses.count {
                    matrices.append(object.nodes[node] * model.defaultInverses[node])
                } else {
                    matrices.append(object.transform)
                }
            }
            let uniforms = ModelObjectUniforms(firstNode: UInt32(first), nodeCount: UInt32(nodeCount))
            if isSky {
                skyDrawn = (uniforms, parts)
            } else if object.firstPerson {
                firstPerson.append((uniforms, parts))
            } else {
                drawn.append((uniforms, parts))
            }
        }
        frame = (frame + 1) % skins.count
        let length = max(matrices.count, 1) * MemoryLayout<simd_float4x4>.stride
        if (skins[frame]?.length ?? 0) < length {
            skins[frame] = device.makeBuffer(length: max(length, 64 * 1024), options: .storageModeShared)
        }
        if let buffer = skins[frame], !matrices.isEmpty {
            matrices.withUnsafeBytes { raw in
                if let base = raw.baseAddress { buffer.contents().copyMemory(from: base, byteCount: raw.count) }
            }
        }
        lastPartCount = drawn.reduce(0) { $0 + $1.parts.count }
        lastSkyPartCount = skyDrawn?.parts.count ?? 0
        lastFirstPersonPartCount = firstPerson.reduce(0) { $0 + $1.parts.count }
        return Prepared(skins: skins[frame], objects: drawn, sky: skyDrawn, firstPerson: firstPerson)
    }

    /// The prepared sky, its opaque parts then its blended ones, behind
    /// everything (the encoder's depth state is left for the caller to set).
    func encodeSky(_ prepared: Prepared, into encoder: MTLRenderCommandEncoder) {
        guard let skins = prepared.skins, let (uniforms, parts) = prepared.sky else { return }
        encoder.setDepthStencilState(skyDepth)
        encoder.setVertexBuffer(vertices, offset: 0, index: 0)
        encoder.setVertexBuffer(skins, offset: 0, index: 2)
        var object = uniforms
        encoder.setVertexBytes(&object, length: MemoryLayout<ModelObjectUniforms>.stride, index: 3)
        for blendedParts in [false, true] {
            encoder.setRenderPipelineState(blendedParts ? skyBlended : skyOpaque)
            for part in parts where part.blended == blendedParts && !part.plain {
                encode(part, into: encoder)
            }
        }
    }

    private func encode(_ part: Draw, into encoder: MTLRenderCommandEncoder) {
        var material = part.uniforms
        encoder.setFragmentBytes(&material, length: MemoryLayout<MaterialUniforms>.stride, index: 0)
        encoder.setFragmentTextures(part.textures, range: 0..<part.textures.count)
        encoder.drawIndexedPrimitives(type: .triangle, indexCount: part.indexCount, indexType: .uint32,
                                      indexBuffer: indices, indexBufferOffset: part.firstIndex * 4)
    }

    /// The prepared objects' opaque and alpha-tested parts, or their
    /// blended ones (or the first-person objects').
    func encode(_ prepared: Prepared, blendedParts: Bool, firstPerson: Bool = false,
                into encoder: MTLRenderCommandEncoder) {
        let objects = firstPerson ? prepared.firstPerson : prepared.objects
        guard let skins = prepared.skins, !objects.isEmpty else { return }
        encoder.setRenderPipelineState(blendedParts ? blended : opaque)
        encoder.setVertexBuffer(vertices, offset: 0, index: 0)
        encoder.setVertexBuffer(skins, offset: 0, index: 2)
        for (uniforms, parts) in objects {
            var object = uniforms
            encoder.setVertexBytes(&object, length: MemoryLayout<ModelObjectUniforms>.stride, index: 3)
            for part in parts where part.blended == blendedParts && !part.plain {
                encode(part, into: encoder)
            }
        }
    }
}
