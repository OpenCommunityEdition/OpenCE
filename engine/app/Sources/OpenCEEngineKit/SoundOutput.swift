// The level's sounds (engine/crates/ffi/src/audio.rs): an AVAudioSourceNode
// pulls the engine's mix, interleaved stereo at 48 kHz, on the audio
// thread. Without an output device the game is silent.
import AVFoundation
import Foundation
import OpenCEEngineCore

public final class SoundOutput {
    private let engine = AVAudioEngine()
    private let output: AudioOutput
    private var node: AVAudioSourceNode?

    /// An output of `output`'s mix; it starts playing at once.
    public init(output: AudioOutput) {
        self.output = output
        let rate = Double(output.sampleRate())
        guard let format = AVAudioFormat(standardFormatWithSampleRate: rate, channels: 2) else { return }
        let source = output
        let node = AVAudioSourceNode(format: format) { _, _, frameCount, buffers -> OSStatus in
            let mixed = source.pull(frames: frameCount)
            let channels = UnsafeMutableAudioBufferListPointer(buffers)
            // (the standard format is deinterleaved: one buffer a channel)
            for (channel, buffer) in channels.enumerated() {
                guard let samples = buffer.mData?.assumingMemoryBound(to: Float.self) else { continue }
                for frame in 0..<Int(frameCount) {
                    let index = frame * 2 + min(channel, 1)
                    samples[frame] = index < mixed.count ? mixed[index] : 0
                }
            }
            return noErr
        }
        engine.attach(node)
        engine.connect(node, to: engine.mainMixerNode, format: format)
        self.node = node
        try? engine.start()
    }

    /// Whether it is playing.
    public var isPlaying: Bool { engine.isRunning }

    /// Stops it.
    public func stop() {
        engine.stop()
    }
}
