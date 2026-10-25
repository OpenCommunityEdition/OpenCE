// The GPU's time for each pass of a frame: the passes' encoders take
// timestamps at their stages' boundaries into counter sample buffers, read
// once the command buffer completes. Every encoder made in play is made
// here, named (`timedRenderEncoder`, `timedComputeEncoder`,
// `timedBlitEncoder`), so that none goes untimed; MetalFX's scaler encodes
// its own and is not timed.
import Foundation
import Metal

/// A pass's time on the GPU.
public struct PassTime: Sendable, Equatable {
    /// The encoder's label.
    public let name: String
    public let milliseconds: Double

    public init(name: String, milliseconds: Double) {
        self.name = name
        self.milliseconds = milliseconds
    }
}

public final class GpuTimings: @unchecked Sendable {
    /// The app's (and the tests'): off until switched on.
    public static let shared = GpuTimings()

    /// Command buffers timed at once, and passes timed in each (a
    /// buffer's samples are limited to 4096 on Apple's GPUs).
    static let slots = 4
    static let passesPerSlot = 256

    private enum Kind {
        /// Samples at the vertex and fragment stages' starts and ends.
        case render
        /// Samples at the encoder's start and end.
        case encoder
    }

    private struct Slot {
        let samples: MTLCounterSampleBuffer
        var passes: [(name: String, kind: Kind)] = []
        /// The command buffer it times (one let go uncommitted frees it).
        weak var commands: MTLCommandBuffer?
        var busy = false
    }

    private struct State {
        var enabled = false
        var device: MTLDevice?
        var unsupported = false
        var slots: [Slot] = []
        var lastFrame: [PassTime] = []
        var frames = 0
        /// The first CPU and GPU timestamps, for the GPU's tick in nanoseconds.
        var first: (cpu: MTLTimestamp, gpu: MTLTimestamp)?
        var nanosecondsPerTick = 1.0
    }

    /// (Metal's objects are not Sendable: the state is kept under a lock,
    /// as the class's @unchecked Sendable promises.)
    private let lock = NSLock()
    private var unlockedState = State()

    private func withState<T>(_ body: (inout State) -> T) -> T {
        lock.lock()
        defer { lock.unlock() }
        return body(&unlockedState)
    }

    init() {}

    /// Whether passes are timed.
    public var enabled: Bool {
        get { withState { $0.enabled } }
        set { withState { $0.enabled = newValue } }
    }

    /// The passes of the last frame completed, in the order encoded.
    public var lastFrame: [PassTime] { withState { $0.lastFrame } }

    /// Frames completed since the app started.
    public var frames: Int { withState { $0.frames } }

    /// Whether `device` can be timed (Apple's GPUs sample at stage boundaries only).
    public static func supported(_ device: MTLDevice) -> Bool {
        device.supportsCounterSampling(.atStageBoundary)
    }

    /// Where the pass `name` of `commands` takes its samples, its slot made
    /// the first time the command buffer is seen; nil when timing is off,
    /// unsupported, or every slot is in use.
    private func begin(_ commands: MTLCommandBuffer, _ name: String, _ kind: Kind)
        -> (samples: MTLCounterSampleBuffer, first: Int)?
    {
        withState { state -> (MTLCounterSampleBuffer, Int)? in
            guard state.enabled, !state.unsupported else { return nil }
            if state.device !== commands.device {
                state.slots = Self.makeSlots(commands.device)
                state.device = commands.device
                state.unsupported = state.slots.isEmpty
                state.first = nil
                if state.unsupported { return nil }
            }
            if state.first == nil {
                let (cpu, gpu) = commands.device.sampleTimestamps()
                state.first = (cpu, gpu)
            }
            let index: Int
            if let open = state.slots.firstIndex(where: { $0.busy && $0.commands === commands }) {
                index = open
            } else {
                for free in state.slots.indices where state.slots[free].busy && state.slots[free].commands == nil {
                    state.slots[free].busy = false
                }
                guard let free = state.slots.firstIndex(where: { !$0.busy }) else { return nil }
                index = free
                state.slots[index].busy = true
                state.slots[index].commands = commands
                state.slots[index].passes = []
                commands.addCompletedHandler { [weak self] _ in self?.resolve(index) }
            }
            guard state.slots[index].passes.count < Self.passesPerSlot else { return nil }
            state.slots[index].passes.append((name, kind))
            return (state.slots[index].samples, (state.slots[index].passes.count - 1) * 4)
        }
    }

    private static func makeSlots(_ device: MTLDevice) -> [Slot] {
        guard supported(device),
            let set = device.counterSets?.first(where: { $0.name == MTLCommonCounterSet.timestamp.rawValue })
        else { return [] }
        let descriptor = MTLCounterSampleBufferDescriptor()
        descriptor.counterSet = set
        descriptor.storageMode = .shared
        descriptor.sampleCount = passesPerSlot * 4
        descriptor.label = "pass timings"
        var slots: [Slot] = []
        for _ in 0..<Self.slots {
            guard let samples = try? device.makeCounterSampleBuffer(descriptor: descriptor) else { return [] }
            slots.append(Slot(samples: samples))
        }
        return slots
    }

    /// The times of the passes of the slot `index`'s completed command buffer.
    private func resolve(_ index: Int) {
        withState { state in
            guard index < state.slots.count else { return }
            let slot = state.slots[index]
            state.slots[index].busy = false
            state.slots[index].commands = nil
            guard !slot.passes.isEmpty,
                let data = try? slot.samples.resolveCounterRange(0..<(slot.passes.count * 4))
            else { return }
            if let device = state.device, let first = state.first {
                let (cpu, gpu) = device.sampleTimestamps()
                // (over a tenth of a second at least, for a steady ratio)
                if gpu > first.gpu, cpu > first.cpu, cpu - first.cpu > 100_000_000 {
                    state.nanosecondsPerTick = Double(cpu - first.cpu) / Double(gpu - first.gpu)
                }
            }
            let ticks = data.withUnsafeBytes { Array($0.bindMemory(to: UInt64.self)) }
            state.lastFrame = slot.passes.enumerated().map { pass, entry in
                let first = pass * 4
                let taken = entry.kind == .render ? ticks[first..<(first + 4)] : ticks[first..<(first + 2)]
                return PassTime(
                    name: entry.name,
                    milliseconds: Self.milliseconds(Array(taken), state.nanosecondsPerTick))
            }
            state.frames += 1
        }
    }

    /// The time from the first valid sample to the last (0 for a pass the
    /// GPU skipped, or samples that cannot be a pass's).
    public static func milliseconds(_ samples: [UInt64], _ nanosecondsPerTick: Double) -> Double {
        let valid = samples.filter { $0 != 0 && $0 != MTLCounterErrorValue }
        guard let start = valid.min(), let end = valid.max() else { return 0 }
        let milliseconds = Double(end - start) * nanosecondsPerTick / 1e6
        return milliseconds < 1000 ? milliseconds : 0
    }

    fileprivate func render(
        _ commands: MTLCommandBuffer, _ name: String,
        _ descriptor: MTLRenderPassDescriptor
    ) -> MTLRenderCommandEncoder? {
        guard let (samples, first) = begin(commands, name, .render),
            let attachment = descriptor.sampleBufferAttachments[0]
        else {
            return commands.makeRenderCommandEncoder(descriptor: descriptor)
        }
        attachment.sampleBuffer = samples
        attachment.startOfVertexSampleIndex = first
        attachment.endOfVertexSampleIndex = first + 1
        attachment.startOfFragmentSampleIndex = first + 2
        attachment.endOfFragmentSampleIndex = first + 3
        let encoder = commands.makeRenderCommandEncoder(descriptor: descriptor)
        // (the descriptor may be kept and used again)
        attachment.sampleBuffer = nil
        return encoder
    }

    fileprivate func compute(_ commands: MTLCommandBuffer, _ name: String) -> MTLComputeCommandEncoder? {
        guard let (samples, first) = begin(commands, name, .encoder) else {
            return commands.makeComputeCommandEncoder()
        }
        let descriptor = MTLComputePassDescriptor()
        descriptor.sampleBufferAttachments[0].sampleBuffer = samples
        descriptor.sampleBufferAttachments[0].startOfEncoderSampleIndex = first
        descriptor.sampleBufferAttachments[0].endOfEncoderSampleIndex = first + 1
        return commands.makeComputeCommandEncoder(descriptor: descriptor)
    }

    fileprivate func blit(_ commands: MTLCommandBuffer, _ name: String) -> MTLBlitCommandEncoder? {
        guard let (samples, first) = begin(commands, name, .encoder) else {
            return commands.makeBlitCommandEncoder()
        }
        let descriptor = MTLBlitPassDescriptor()
        descriptor.sampleBufferAttachments[0].sampleBuffer = samples
        descriptor.sampleBufferAttachments[0].startOfEncoderSampleIndex = first
        descriptor.sampleBufferAttachments[0].endOfEncoderSampleIndex = first + 1
        return commands.makeBlitCommandEncoder(descriptor: descriptor)
    }
}

extension MTLCommandBuffer {
    /// A render encoder labelled `name`, its time on the GPU taken.
    public func timedRenderEncoder(_ name: String, descriptor: MTLRenderPassDescriptor) -> MTLRenderCommandEncoder? {
        let encoder = GpuTimings.shared.render(self, name, descriptor)
        encoder?.label = name
        return encoder
    }

    /// A compute encoder labelled `name`, its time on the GPU taken.
    public func timedComputeEncoder(_ name: String) -> MTLComputeCommandEncoder? {
        let encoder = GpuTimings.shared.compute(self, name)
        encoder?.label = name
        return encoder
    }

    /// A blit encoder labelled `name`, its time on the GPU taken.
    public func timedBlitEncoder(_ name: String) -> MTLBlitCommandEncoder? {
        let encoder = GpuTimings.shared.blit(self, name)
        encoder?.label = name
        return encoder
    }
}
