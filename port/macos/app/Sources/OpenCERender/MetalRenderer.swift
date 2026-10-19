// The guest's render batches on Metal (docs/macos_host.md, "The renderer").
// Each batch is decoded and checked by the runtime (RenderDecoder,
// port/macos/runtime/src/render.rs), so that what reaches here is in range
// and in order; what is left to refuse is what Metal itself will not make
// (a shader that does not compile, say), and what uses it.
//
// The commands are carried out in the order the guest wrote them: uploads
// are blits in the same command buffer as the draws, so that a draw sees
// what was uploaded before it and not after; an upload inside a pass ends
// its encoder (storing its targets), and the next draw begins another that
// loads them, with the pass's state set again. The command buffer lasts
// from batch to batch, and is committed at PRESENT, or at READ_TEXTURE,
// which waits for it.

import Foundation
import Metal
import OpenCERuntime

public final class MetalRenderer: @unchecked Sendable {
    public struct Statistics {
        public var batches = 0
        /// Commands the renderer refused (the runtime's refusals are not
        /// counted here).
        public var refused = 0
        public var librariesCompiled = 0
        public var pipelinesMade = 0
        public var depthStencilsMade = 0
        public var samplersMade = 0
    }

    /// Where refusals and Metal's errors are told (none by default).
    public var log: (@Sendable (String) -> Void)?

    private let device: MTLDevice
    private let queue: MTLCommandQueue
    private let memory: GuestMemoryView
    private let decoder: RenderDecoder
    private let window: RenderWindow
    private let lock = NSLock()
    private var counts = Statistics()
    private var logged = 0

    // ---------- the guest's objects, by id
    private struct Texture {
        let texture: MTLTexture
        let format: PixelFormat
        let kind: TextureKind
    }
    private var buffers: [UInt32: MTLBuffer] = [:]
    private var textures: [UInt32: Texture] = [:]
    private var libraries: [UInt32: MTLLibrary] = [:]
    private var pipelines: [UInt32: MTLRenderPipelineState] = [:]
    private var depthStencils: [UInt32: MTLDepthStencilState] = [:]
    private var samplers: [UInt32: MTLSamplerState] = [:]

    // ---------- what was made, by what it was made of: the guest makes the
    // same states again and again (each of Direct3D's state changes is a
    // new one), which Metal makes slowly. (They last as long as the
    // renderer: the guest's kinds of state are few.)
    private struct PipelineKey: Hashable {
        let vertex: ObjectIdentifier
        let fragment: ObjectIdentifier?
        /// the command, its ids 0
        let command: Command
    }
    private var librariesBySource: [String: MTLLibrary] = [:]
    private var pipelinesByKey: [PipelineKey: MTLRenderPipelineState] = [:]
    private var depthStencilsByCommand: [Command: MTLDepthStencilState] = [:]
    private var samplersByCommand: [Command: MTLSamplerState] = [:]

    // ---------- the commands being encoded
    private enum Slot: Hashable {
        case pipeline, depthStencil, raster, viewport, scissor, blendColor, visibility
        case vertexBuffer(UInt32)
        case bytes(Stage, UInt32)
        case texture(Stage, UInt32)
        case sampler(Stage, UInt32)
    }
    private struct Pass {
        let descriptor: MTLRenderPassDescriptor
        let width: Int
        let height: Int
        let resolves: Bool
        let depth: Bool
        let stencil: Bool
        var failed: Bool
        /// whether an encoder was begun (the next loads what it stored)
        var begun = false
        var pipelineSet = false
        /// the state set, to set again in the pass's next encoder
        var state: [Slot: Command] = [:]
    }
    private var commands: MTLCommandBuffer?
    private var committed: MTLCommandBuffer?
    private var blit: MTLBlitCommandEncoder?
    private var render: MTLRenderCommandEncoder?
    private var pass: Pass?
    private var windowFrame: RenderFrame?

    public init?(device: MTLDevice, memory: GuestMemoryView, window: RenderWindow) {
        guard let queue = device.makeCommandQueue() else { return nil }
        self.device = device
        self.queue = queue
        self.memory = memory
        self.window = window
        decoder = RenderDecoder(memory: memory)
    }

    public var statistics: Statistics {
        lock.lock()
        defer { lock.unlock() }
        return counts
    }

    /// Carries out the batch the guest wrote at `address` (host_render_submit,
    /// on the guest's thread): the number of its commands refused, or -1 for
    /// a batch refused whole.
    public func submit(address: UInt32, size: UInt32) -> Int32 {
        lock.lock()
        defer { lock.unlock() }
        let (width, height) = window.drawableSize
        decoder.setDrawableSize(width: UInt32(clamping: width), height: UInt32(clamping: height))
        let batch: DecodedBatch
        do {
            batch = try decoder.decodeGuest(address: address, size: size)
        } catch {
            tell("render batch refused: \(error)")
            return -1
        }
        counts.batches += 1
        for rejection in batch.rejected {
            tell("render command refused at byte \(rejection.offset) (op \(rejection.op)): \(rejection.reason)")
        }
        var refused = 0
        for command in batch.commands where !execute(command) {
            refused += 1
        }
        counts.refused += refused
        return Int32(clamping: batch.rejected.count + refused)
    }

    /// Commits what is encoded and waits for the GPU to finish it.
    public func finish() {
        lock.lock()
        defer { lock.unlock() }
        endRender(final: false)
        endBlit()
        commit()
        committed?.waitUntilCompleted()
    }

    private func tell(_ message: String) {
        guard let log else { return }
        logged += 1
        if logged < 100 {
            log(message)
        } else if logged == 100 {
            log("render: no more refusals are told")
        }
    }

    // ---------- the commands

    /// Whether the command was carried out.
    private func execute(_ command: Command) -> Bool {
        switch command {
        case let .createBuffer(id, length):
            // (zeroed, as Metal makes them)
            buffers[id] = device.makeBuffer(length: max(Int(length), 1), options: .storageModeShared)
            return buffers[id] != nil
        case let .uploadBuffer(id, offset, address, length):
            guard let buffer = buffers[id], let staging = staging(address, length), let blit = ensureBlit() else {
                return false
            }
            blit.copy(from: staging, sourceOffset: 0, to: buffer, destinationOffset: Int(offset), size: Int(length))
            return true
        case let .createTexture(id, kind, format, width, height, depth, levels, sampled, renderTarget, samples):
            let descriptor = MTLTextureDescriptor()
            switch kind {
            case .plane: descriptor.textureType = samples > 1 ? .type2DMultisample : .type2D
            case .cube: descriptor.textureType = .typeCube
            case .volume: descriptor.textureType = .type3D
            }
            descriptor.pixelFormat = format.metal
            descriptor.width = Int(width)
            descriptor.height = Int(height)
            descriptor.depth = kind == .volume ? Int(depth) : 1
            descriptor.mipmapLevelCount = Int(levels)
            descriptor.sampleCount = Int(samples)
            descriptor.storageMode = .private
            descriptor.usage = []
            if sampled { descriptor.usage.insert(.shaderRead) }
            if renderTarget { descriptor.usage.insert(.renderTarget) }
            textures[id] = device.makeTexture(descriptor: descriptor).map {
                Texture(texture: $0, format: format, kind: kind)
            }
            return textures[id] != nil
        case let .uploadTexture(id, level, slice, x, y, z, width, height, depth, address, bytesPerRow, bytesPerImage,
                                length):
            guard let texture = textures[id], let staging = staging(address, length), let blit = ensureBlit() else {
                return false
            }
            let volume = texture.kind == .volume
            blit.copy(from: staging, sourceOffset: 0, sourceBytesPerRow: Int(bytesPerRow),
                      sourceBytesPerImage: volume ? Int(bytesPerImage) : 0,
                      sourceSize: MTLSize(width: Int(width), height: Int(height), depth: Int(depth)),
                      to: texture.texture, destinationSlice: volume ? 0 : Int(slice), destinationLevel: Int(level),
                      destinationOrigin: MTLOrigin(x: Int(x), y: Int(y), z: volume ? Int(z) : 0))
            return true
        case let .destroy(kind, id):
            // (the command buffer keeps what it uses until it has run)
            switch kind {
            case .buffer: buffers[id] = nil
            case .texture: textures[id] = nil
            case .library: libraries[id] = nil
            case .pipeline: pipelines[id] = nil
            case .depthStencil: depthStencils[id] = nil
            case .sampler: samplers[id] = nil
            }
            return true
        case let .createLibrary(id, address, length):
            libraries[id] = library(address, length)
            return libraries[id] != nil
        case let .createPipeline(id, vertexLibrary, fragmentLibrary, colorFormat, depthFormat, samples, blend,
                                 writeMask, attributes, layouts):
            pipelines[id] = pipeline(vertexLibrary, fragmentLibrary, key: .createPipeline(
                id: 0, vertexLibrary: 0, fragmentLibrary: 0, colorFormat: colorFormat, depthFormat: depthFormat,
                samples: samples, blend: blend, writeMask: writeMask, attributes: attributes, layouts: layouts))
            return pipelines[id] != nil
        case let .createDepthStencil(id, depthCompare, depthWrite, stencil, front, back):
            let key = Command.createDepthStencil(id: 0, depthCompare: depthCompare, depthWrite: depthWrite,
                                                 stencil: stencil, front: front, back: back)
            if depthStencilsByCommand[key] == nil {
                let descriptor = MTLDepthStencilDescriptor()
                descriptor.depthCompareFunction = depthCompare.metal
                descriptor.isDepthWriteEnabled = depthWrite
                if stencil {
                    descriptor.frontFaceStencil = front.metal
                    descriptor.backFaceStencil = back.metal
                }
                depthStencilsByCommand[key] = device.makeDepthStencilState(descriptor: descriptor)
                counts.depthStencilsMade += depthStencilsByCommand[key] == nil ? 0 : 1
            }
            depthStencils[id] = depthStencilsByCommand[key]
            return depthStencils[id] != nil
        case let .createSampler(id, minFilter, magFilter, mipFilter, addressU, addressV, addressW, maxAnisotropy,
                                minLod, maxLod, border):
            let key = Command.createSampler(id: 0, minFilter: minFilter, magFilter: magFilter, mipFilter: mipFilter,
                                            addressU: addressU, addressV: addressV, addressW: addressW,
                                            maxAnisotropy: maxAnisotropy, minLod: minLod, maxLod: maxLod,
                                            border: border)
            if samplersByCommand[key] == nil {
                let descriptor = MTLSamplerDescriptor()
                descriptor.minFilter = minFilter.metal
                descriptor.magFilter = magFilter.metal
                descriptor.mipFilter = mipFilter.metalMip
                descriptor.sAddressMode = addressU.metal
                descriptor.tAddressMode = addressV.metal
                descriptor.rAddressMode = addressW.metal
                descriptor.maxAnisotropy = Int(maxAnisotropy)
                descriptor.lodMinClamp = minLod
                descriptor.lodMaxClamp = maxLod
                descriptor.borderColor = border.metal
                samplersByCommand[key] = device.makeSamplerState(descriptor: descriptor)
                counts.samplersMade += samplersByCommand[key] == nil ? 0 : 1
            }
            samplers[id] = samplersByCommand[key]
            return samplers[id] != nil
        case let .beginPass(color, depth, colorLoad, depthLoad, stencilLoad, clearRed, clearGreen, clearBlue,
                            clearAlpha, clearDepth, clearStencil, resolve, visibility, _, _):
            return beginPass(color: color, depth: depth, colorLoad: colorLoad, depthLoad: depthLoad,
                             stencilLoad: stencilLoad,
                             clear: MTLClearColor(red: Double(clearRed), green: Double(clearGreen),
                                                  blue: Double(clearBlue), alpha: Double(clearAlpha)),
                             clearDepth: clearDepth, clearStencil: clearStencil, resolve: resolve,
                             visibility: visibility)
        case .endPass:
            guard let pass else { return false }
            // (a pass whose encoder an upload ended has its resolve still to do)
            if pass.resolves {
                _ = ensureRender()
            }
            endRender(final: true)
            self.pass = nil
            return true
        case .setPipeline, .setDepthStencil, .setRaster, .setViewport, .setScissor, .setBlendColor, .setVertexBuffer,
             .setBytes, .setTexture, .setSampler, .setVisibility:
            guard pass != nil, let encoder = ensureRender() else { return false }
            let applied = apply(command, to: encoder)
            pass?.state[slot(of: command)] = command
            if case .setPipeline = command {
                pass?.pipelineSet = applied
            }
            return applied
        case let .draw(primitive, first, vertices, instances):
            guard pass?.pipelineSet == true, let encoder = ensureRender() else { return false }
            encoder.drawPrimitives(type: primitive.metal, vertexStart: Int(first), vertexCount: Int(vertices),
                                   instanceCount: Int(instances))
            return true
        case let .drawIndexed(primitive, indices, indexType, buffer, offset, baseVertex, instances):
            guard pass?.pipelineSet == true, let indexBuffer = buffers[buffer], let encoder = ensureRender() else {
                return false
            }
            encoder.drawIndexedPrimitives(type: primitive.metal, indexCount: Int(indices), indexType: indexType.metal,
                                          indexBuffer: indexBuffer, indexBufferOffset: Int(offset),
                                          instanceCount: Int(instances), baseVertex: Int(baseVertex), baseInstance: 0)
            return true
        case let .copyTexture(source, sourceLevel, sourceSlice, sourceX, sourceY, destination, destinationLevel,
                              destinationSlice, destinationX, destinationY, width, height):
            guard let from = textures[source], let to = textures[destination], let blit = ensureBlit() else {
                return false
            }
            let (fromSlice, fromOrigin) = place(from, sourceSlice, sourceX, sourceY)
            let (toSlice, toOrigin) = place(to, destinationSlice, destinationX, destinationY)
            blit.copy(from: from.texture, sourceSlice: fromSlice, sourceLevel: Int(sourceLevel), sourceOrigin: fromOrigin,
                      sourceSize: MTLSize(width: Int(width), height: Int(height), depth: 1), to: to.texture,
                      destinationSlice: toSlice, destinationLevel: Int(destinationLevel), destinationOrigin: toOrigin)
            return true
        case let .readTexture(id, level, slice, x, y, width, height, address, bytesPerRow, length):
            guard let texture = textures[id],
                  let staging = device.makeBuffer(length: Int(length), options: .storageModeShared),
                  let blit = ensureBlit() else { return false }
            let (fromSlice, fromOrigin) = place(texture, slice, x, y)
            blit.copy(from: texture.texture, sourceSlice: fromSlice, sourceLevel: Int(level), sourceOrigin: fromOrigin,
                      sourceSize: MTLSize(width: Int(width), height: Int(height), depth: 1), to: staging,
                      destinationOffset: 0, destinationBytesPerRow: Int(bytesPerRow),
                      destinationBytesPerImage: Int(length))
            endBlit()
            commit()
            committed?.waitUntilCompleted()
            do {
                try memory.write(address: address, bytes: Data(bytes: staging.contents(), count: Int(length)))
                return true
            } catch {
                return false
            }
        case let .generateMipmaps(id):
            guard let texture = textures[id], texture.format.generatesMipmaps else { return false }
            if texture.texture.mipmapLevelCount > 1 {
                guard let blit = ensureBlit() else { return false }
                blit.generateMipmaps(for: texture.texture)
            }
            return true
        case .present:
            endBlit()
            guard let commands = ensureCommands() else { return false }
            if let frame = windowFrame {
                window.present(frame, in: commands)
                windowFrame = nil
            }
            commit()
            if let message = window.committed(commands) {
                log?(message)
            }
            return true
        }
    }

    /// A volume's slice is its z.
    private func place(_ texture: Texture, _ slice: UInt32, _ x: UInt32, _ y: UInt32) -> (Int, MTLOrigin) {
        texture.kind == .volume
            ? (0, MTLOrigin(x: Int(x), y: Int(y), z: Int(slice)))
            : (Int(slice), MTLOrigin(x: Int(x), y: Int(y), z: 0))
    }

    /// The guest's bytes, copied out for a blit.
    private func staging(_ address: UInt32, _ length: UInt32) -> MTLBuffer? {
        guard let host = try? memory.hostAddress(address: address, size: UInt64(length)),
              let pointer = UnsafeRawPointer(bitPattern: UInt(host)) else { return nil }
        return device.makeBuffer(bytes: pointer, length: Int(length), options: .storageModeShared)
    }

    private func library(_ address: UInt32, _ length: UInt32) -> MTLLibrary? {
        guard let bytes = try? memory.read(address: address, size: UInt64(length)) else { return nil }
        let source = String(decoding: bytes, as: UTF8.self)
        if let library = librariesBySource[source] {
            return library
        }
        do {
            let library = try device.makeLibrary(source: source, options: nil)
            librariesBySource[source] = library
            counts.librariesCompiled += 1
            return library
        } catch {
            tell("render library refused: \(error.localizedDescription)")
            return nil
        }
    }

    private func pipeline(_ vertexLibrary: UInt32, _ fragmentLibrary: UInt32, key command: Command)
        -> MTLRenderPipelineState? {
        guard case let .createPipeline(_, _, _, colorFormat, depthFormat, samples, blend, writeMask, attributes,
                                       layouts) = command,
              let vertex = libraries[vertexLibrary], let vertexFunction = vertex.makeFunction(name: "vertex_main")
        else { return nil }
        var fragment: MTLLibrary?
        var fragmentFunction: MTLFunction?
        if fragmentLibrary != 0 {
            guard let library = libraries[fragmentLibrary], let function = library.makeFunction(name: "fragment_main")
            else { return nil }
            fragment = library
            fragmentFunction = function
        }
        let key = PipelineKey(vertex: ObjectIdentifier(vertex as AnyObject),
                              fragment: fragment.map { ObjectIdentifier($0 as AnyObject) }, command: command)
        if let state = pipelinesByKey[key] {
            return state
        }
        let descriptor = MTLRenderPipelineDescriptor()
        descriptor.vertexFunction = vertexFunction
        descriptor.fragmentFunction = fragmentFunction
        descriptor.rasterSampleCount = Int(samples)
        let color = descriptor.colorAttachments[0]!
        color.pixelFormat = colorFormat.metal
        color.isBlendingEnabled = blend.enabled
        color.sourceRGBBlendFactor = blend.sourceColor.metal
        color.destinationRGBBlendFactor = blend.destinationColor.metal
        color.rgbBlendOperation = blend.colorOperation.metal
        color.sourceAlphaBlendFactor = blend.sourceAlpha.metal
        color.destinationAlphaBlendFactor = blend.destinationAlpha.metal
        color.alphaBlendOperation = blend.alphaOperation.metal
        color.writeMask = colorWriteMask(writeMask)
        if let depthFormat {
            descriptor.depthAttachmentPixelFormat = depthFormat.metal
            if depthFormat.hasStencil {
                descriptor.stencilAttachmentPixelFormat = depthFormat.metal
            }
        }
        if !attributes.isEmpty {
            let vertices = MTLVertexDescriptor()
            for attribute in attributes {
                let metal = vertices.attributes[Int(attribute.index)]!
                metal.format = attribute.format.metal
                metal.offset = Int(attribute.offset)
                metal.bufferIndex = Int(attribute.slot)
            }
            for layout in layouts {
                let metal = vertices.layouts[Int(layout.slot)]!
                metal.stride = Int(layout.stride)
                switch layout.step {
                case .vertex:
                    metal.stepFunction = .perVertex
                    metal.stepRate = 1
                case .instance:
                    metal.stepFunction = .perInstance
                    metal.stepRate = Int(layout.stepRate)
                case .constant:
                    metal.stepFunction = .constant
                    metal.stepRate = 0
                }
            }
            descriptor.vertexDescriptor = vertices
        }
        do {
            let state = try device.makeRenderPipelineState(descriptor: descriptor)
            pipelinesByKey[key] = state
            counts.pipelinesMade += 1
            return state
        } catch {
            tell("render pipeline refused: \(error.localizedDescription)")
            return nil
        }
    }

    // ---------- passes

    private func beginPass(color: UInt32, depth: UInt32, colorLoad: Load, depthLoad: Load, stencilLoad: Load,
                           clear: MTLClearColor, clearDepth: Float, clearStencil: UInt32, resolve: UInt32,
                           visibility: UInt32) -> Bool {
        endBlit()
        let descriptor = MTLRenderPassDescriptor()
        let target: MTLTexture?
        if color == 0 {
            // (one frame of the window until PRESENT, however many passes draw it)
            if windowFrame == nil {
                windowFrame = window.nextFrame()
            }
            target = windowFrame?.texture
        } else {
            target = textures[color]?.texture
        }
        var failed = target == nil
        let attachment = descriptor.colorAttachments[0]!
        attachment.texture = target
        attachment.loadAction = colorLoad.metal
        attachment.clearColor = clear
        // (each encoder's stores are set as it ends: endRender)
        attachment.storeAction = .unknown
        if resolve != 0 {
            attachment.resolveTexture = textures[resolve]?.texture
            failed = failed || attachment.resolveTexture == nil
        }
        var hasStencil = false
        if depth != 0 {
            if let texture = textures[depth] {
                descriptor.depthAttachment.texture = texture.texture
                descriptor.depthAttachment.loadAction = depthLoad.metal
                descriptor.depthAttachment.clearDepth = Double(clearDepth)
                descriptor.depthAttachment.storeAction = .unknown
                if texture.format.hasStencil {
                    hasStencil = true
                    descriptor.stencilAttachment.texture = texture.texture
                    descriptor.stencilAttachment.loadAction = stencilLoad.metal
                    descriptor.stencilAttachment.clearStencil = clearStencil
                    descriptor.stencilAttachment.storeAction = .unknown
                }
            } else {
                failed = true
            }
        }
        if visibility != 0 {
            descriptor.visibilityResultBuffer = buffers[visibility]
            failed = failed || descriptor.visibilityResultBuffer == nil
        }
        // (the target's own size, which a window's may differ from by a resize)
        pass = Pass(descriptor: descriptor, width: target?.width ?? 0, height: target?.height ?? 0,
                    resolves: resolve != 0, depth: depth != 0, stencil: hasStencil, failed: failed)
        return !failed && ensureRender() != nil
    }

    /// The pass's encoder, begun again (loading what the last stored, with
    /// the state set again) if an upload ended it.
    private func ensureRender() -> MTLRenderCommandEncoder? {
        if let render {
            return render
        }
        guard let pass, !pass.failed, let commands = ensureCommands() else { return nil }
        endBlit()
        guard let encoder = commands.makeRenderCommandEncoder(descriptor: pass.descriptor) else {
            self.pass?.failed = true
            return nil
        }
        render = encoder
        if pass.begun {
            for command in pass.state.values {
                _ = apply(command, to: encoder)
            }
        } else {
            // (the descriptor was copied: these are for the next encoder)
            pass.descriptor.colorAttachments[0].loadAction = .load
            pass.descriptor.depthAttachment.loadAction = .load
            pass.descriptor.stencilAttachment.loadAction = .load
            self.pass?.begun = true
        }
        return encoder
    }

    /// Ends the pass's encoder, storing its targets, and resolving them if
    /// the pass is over.
    private func endRender(final: Bool) {
        guard let render, let pass else { return }
        render.setColorStoreAction(final && pass.resolves ? .storeAndMultisampleResolve : .store, index: 0)
        if pass.depth {
            render.setDepthStoreAction(.store)
        }
        if pass.stencil {
            render.setStencilStoreAction(.store)
        }
        render.endEncoding()
        self.render = nil
    }

    private func slot(of command: Command) -> Slot {
        switch command {
        case .setPipeline: .pipeline
        case .setDepthStencil: .depthStencil
        case .setRaster: .raster
        case .setViewport: .viewport
        case .setScissor: .scissor
        case .setBlendColor: .blendColor
        case .setVisibility: .visibility
        case let .setVertexBuffer(slot, _, _): .vertexBuffer(slot)
        case let .setBytes(stage, slot, _): .bytes(stage, slot)
        case let .setTexture(stage, slot, _): .texture(stage, slot)
        case let .setSampler(stage, slot, _): .sampler(stage, slot)
        default: fatalError("not a pass's state: \(command)")
        }
    }

    /// Sets a pass's state on its encoder. What names an object Metal did
    /// not make is unbound, and refused.
    private func apply(_ command: Command, to encoder: MTLRenderCommandEncoder) -> Bool {
        switch command {
        case let .setPipeline(id):
            guard let state = pipelines[id] else { return false }
            encoder.setRenderPipelineState(state)
            return true
        case let .setDepthStencil(id, reference):
            let state = id == 0 ? nil : depthStencils[id]
            encoder.setDepthStencilState(state)
            encoder.setStencilReferenceValue(reference)
            return id == 0 || state != nil
        case let .setRaster(cull, winding, fill, depthBias, slopeScale, clamp, clampDepth):
            encoder.setCullMode(cull.metal)
            encoder.setFrontFacing(winding.metal)
            encoder.setTriangleFillMode(fill.metal)
            encoder.setDepthBias(depthBias, slopeScale: slopeScale, clamp: clamp)
            encoder.setDepthClipMode(clampDepth ? .clamp : .clip)
            return true
        case let .setViewport(x, y, width, height, near, far):
            encoder.setViewport(MTLViewport(originX: Double(x), originY: Double(y), width: Double(width),
                                            height: Double(height), znear: Double(near), zfar: Double(far)))
            return true
        case let .setScissor(x, y, width, height):
            // (clipped to the target, as Metal requires)
            let (targetWidth, targetHeight) = (pass?.width ?? 0, pass?.height ?? 0)
            let left = min(Int(x), targetWidth), top = min(Int(y), targetHeight)
            encoder.setScissorRect(MTLScissorRect(x: left, y: top, width: min(Int(width), targetWidth - left),
                                                  height: min(Int(height), targetHeight - top)))
            return true
        case let .setBlendColor(red, green, blue, alpha):
            encoder.setBlendColor(red: red, green: green, blue: blue, alpha: alpha)
            return true
        case let .setVertexBuffer(slot, id, offset):
            let buffer = id == 0 ? nil : buffers[id]
            encoder.setVertexBuffer(buffer, offset: buffer == nil ? 0 : Int(offset), index: Int(slot))
            return id == 0 || buffer != nil
        case let .setBytes(stage, slot, bytes):
            guard !bytes.isEmpty else { return false }
            bytes.withUnsafeBytes { raw in
                if stage == .vertex {
                    encoder.setVertexBytes(raw.baseAddress!, length: raw.count, index: Int(slot))
                } else {
                    encoder.setFragmentBytes(raw.baseAddress!, length: raw.count, index: Int(slot))
                }
            }
            return true
        case let .setTexture(stage, slot, id):
            let texture = id == 0 ? nil : textures[id]?.texture
            if stage == .vertex {
                encoder.setVertexTexture(texture, index: Int(slot))
            } else {
                encoder.setFragmentTexture(texture, index: Int(slot))
            }
            return id == 0 || texture != nil
        case let .setSampler(stage, slot, id):
            let sampler = id == 0 ? nil : samplers[id]
            if stage == .vertex {
                encoder.setVertexSamplerState(sampler, index: Int(slot))
            } else {
                encoder.setFragmentSamplerState(sampler, index: Int(slot))
            }
            return id == 0 || sampler != nil
        case let .setVisibility(mode, offset):
            encoder.setVisibilityResultMode(mode.metal, offset: Int(offset))
            return true
        default:
            return false
        }
    }

    // ---------- command buffers and blits

    private func ensureCommands() -> MTLCommandBuffer? {
        if commands == nil {
            commands = queue.makeCommandBuffer()
        }
        return commands
    }

    /// The blit encoder, ending the pass's encoder for it if there is one.
    private func ensureBlit() -> MTLBlitCommandEncoder? {
        endRender(final: false)
        if blit == nil {
            blit = ensureCommands()?.makeBlitCommandEncoder()
        }
        return blit
    }

    private func endBlit() {
        blit?.endEncoding()
        blit = nil
    }

    private func commit() {
        guard let commands else { return }
        let log = self.log
        commands.addCompletedHandler { buffer in
            if let error = buffer.error {
                log?("render commands failed: \(error.localizedDescription)")
            }
        }
        commands.commit()
        committed = commands
        self.commands = nil
    }
}
