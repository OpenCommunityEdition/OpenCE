// The game's sound (docs/macos_host.md): each stream the host opens is an
// AVAudioEngine whose source node pulls the stream's frames from the host
// (halo_host_audio_render, given here as `render`) on Core Audio's real-time
// thread. The engine's mixer converts them to the device's rate and channels.
// Offline engines render with no device, for the tests.

import AVFoundation
import os

public final class GameAudio: @unchecked Sendable {
    /// Fills `frames` frames of a stream, a channel each, into the pointers.
    public typealias Render = @Sendable (UInt32, UnsafePointer<UnsafeMutablePointer<Float>?>, Int32, Int32) -> Void

    public enum Failure: Error {
        case noStream
        case rendering
    }

    static let maximumChannels = 8
    static let offlineFrames: AVAudioFrameCount = 4096

    /// The pointers a render is given, kept so that the real-time thread
    /// allocates nothing.
    final class Channels: @unchecked Sendable {
        let pointers = UnsafeMutablePointer<UnsafeMutablePointer<Float>?>.allocate(capacity: GameAudio.maximumChannels)
        deinit { pointers.deallocate() }
    }

    // (an engine is used from one guest thread at a time, and its renders
    // from Core Audio's)
    struct Stream: @unchecked Sendable {
        let engine: AVAudioEngine
        let format: AVAudioFormat
        let observer: NSObjectProtocol?
    }

    private let render: Render
    private let offline: Bool
    private let lock = OSAllocatedUnfairLock()
    private var streams: [UInt32: Stream] = [:]
    private let log = Logger(subsystem: "org.opence.OpenCE", category: "audio")

    public init(offline: Bool = false, render: @escaping Render) {
        self.offline = offline
        self.render = render
    }

    /// A stream of `channels` channels at `rate` Hz, paused; false if it
    /// cannot be made (or `stream` has one already).
    public func open(stream: UInt32, channels: Int, rate: Double) -> Bool {
        guard (1...Self.maximumChannels).contains(channels), rate > 0,
              let format = AVAudioFormat(standardFormatWithSampleRate: rate, channels: AVAudioChannelCount(channels))
        else { return false }
        if lock.withLock({ streams[stream] != nil }) { return false }
        let engine = AVAudioEngine()
        let pointers = Channels()
        let render = self.render
        let source = AVAudioSourceNode(format: format) { _, _, frameCount, bufferList -> OSStatus in
            let buffers = UnsafeMutableAudioBufferListPointer(bufferList)
            let count = min(buffers.count, GameAudio.maximumChannels)
            for index in 0..<count {
                pointers.pointers[index] = buffers[index].mData?.assumingMemoryBound(to: Float.self)
            }
            render(stream, UnsafePointer(pointers.pointers), Int32(count), Int32(frameCount))
            return noErr
        }
        engine.attach(source)
        engine.connect(source, to: engine.mainMixerNode, format: format)
        let observer: NSObjectProtocol?
        if offline {
            do {
                try engine.enableManualRenderingMode(.offline, format: format, maximumFrameCount: Self.offlineFrames)
            } catch {
                log.error("an offline engine cannot be made: \(error.localizedDescription)")
                return false
            }
            observer = nil
        } else {
            // (a new output device stops the engine: it is started again)
            observer = NotificationCenter.default.addObserver(
                forName: .AVAudioEngineConfigurationChange, object: engine, queue: nil
            ) { [weak self, log] _ in
                // (a stream closed since is not started)
                guard let self, let engine = self.lock.withLock({ self.streams[stream] })?.engine,
                      !engine.isRunning else { return }
                do { try engine.start() } catch {
                    log.error("the sound cannot start again: \(error.localizedDescription)")
                }
            }
        }
        engine.prepare()
        let opened = Stream(engine: engine, format: format, observer: observer)
        lock.withLock { streams[stream] = opened }
        return true
    }

    /// Plays a stream; false if there is none or its engine cannot start.
    public func resume(stream: UInt32) -> Bool {
        guard let engine = lock.withLock({ streams[stream] })?.engine else { return false }
        if engine.isRunning { return true }
        do {
            try engine.start()
            return true
        } catch {
            log.error("the sound cannot start: \(error.localizedDescription)")
            return false
        }
    }

    /// Stops a stream; once this returns no render for it runs.
    public func close(stream: UInt32) {
        guard let closed = lock.withLock({ streams.removeValue(forKey: stream) }) else { return }
        if let observer = closed.observer { NotificationCenter.default.removeObserver(observer) }
        closed.engine.stop()
    }

    /// `frames` frames of an offline stream, as its engine gives them.
    public func renderOffline(stream: UInt32, frames: AVAudioFrameCount) throws -> AVAudioPCMBuffer {
        guard let open = lock.withLock({ streams[stream] }), open.engine.isInManualRenderingMode else {
            throw Failure.noStream
        }
        if !open.engine.isRunning { try open.engine.start() }
        guard let buffer = AVAudioPCMBuffer(pcmFormat: open.engine.manualRenderingFormat, frameCapacity: frames)
        else { throw Failure.rendering }
        while buffer.frameLength < frames {
            let rest = min(frames - buffer.frameLength, Self.offlineFrames)
            guard let part = AVAudioPCMBuffer(pcmFormat: buffer.format, frameCapacity: rest) else {
                throw Failure.rendering
            }
            guard try open.engine.renderOffline(rest, to: part) == .success else { throw Failure.rendering }
            for channel in 0..<Int(buffer.format.channelCount) {
                guard let from = part.floatChannelData?[channel], let to = buffer.floatChannelData?[channel]
                else { throw Failure.rendering }
                (to + Int(buffer.frameLength)).update(from: from, count: Int(part.frameLength))
            }
            buffer.frameLength += part.frameLength
        }
        return buffer
    }
}
