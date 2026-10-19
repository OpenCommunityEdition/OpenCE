import AVFoundation
import OpenCEAudio
import Testing

/// The game's sound (Sources/OpenCEAudio): an engine's source node renders,
/// a channel each, what the host's render function gives for its stream.
/// The engines render offline here, with no device.
struct GameAudioTests {
    /// A render function that gives channel c of frame f as stream + c + f / 1000,
    /// and records the streams and frame counts it was asked for.
    final class Host: @unchecked Sendable {
        var asked: [(UInt32, Int32, Int32)] = []

        func render(_ stream: UInt32, _ channels: UnsafePointer<UnsafeMutablePointer<Float>?>, _ count: Int32,
                    _ frames: Int32) {
            asked.append((stream, count, frames))
            for channel in 0..<Int(count) {
                guard let samples = channels[channel] else { continue }
                for frame in 0..<Int(frames) {
                    samples[frame] = Float(stream) / 100 + Float(channel) / 10 + Float(frame) / 10_000
                }
            }
        }
    }

    @Test func aStreamRendersWhatTheHostGives() throws {
        let host = Host()
        let audio = GameAudio(offline: true) { host.render($0, $1, $2, $3) }
        #expect(audio.open(stream: 7, channels: 2, rate: 48_000))
        #expect(audio.resume(stream: 7))
        let buffer = try #require(try audio.renderOffline(stream: 7, frames: 256))
        #expect(buffer.frameLength == 256)
        #expect(buffer.format.channelCount == 2)
        let left = try #require(buffer.floatChannelData?[0])
        let right = try #require(buffer.floatChannelData?[1])
        #expect(abs(left[0] - 0.07) < 1e-5)
        #expect(abs(right[0] - 0.17) < 1e-5)
        #expect(abs(left[100] - (0.07 + 0.01)) < 1e-5)
        #expect(host.asked.allSatisfy { $0.0 == 7 && $0.1 == 2 })
        #expect(host.asked.reduce(0) { $0 + Int($1.2) } >= 256)
        audio.close(stream: 7)
        #expect(throws: GameAudio.Failure.self) { try audio.renderOffline(stream: 7, frames: 16) }
    }

    @Test func aStreamPlaysAtItsOwnRateAndChannels() throws {
        let host = Host()
        let audio = GameAudio(offline: true) { host.render($0, $1, $2, $3) }
        #expect(audio.open(stream: 3, channels: 1, rate: 22_050))
        let buffer = try #require(try audio.renderOffline(stream: 3, frames: 64))
        #expect(buffer.format.sampleRate == 22_050 && buffer.format.channelCount == 1)
        #expect(abs((buffer.floatChannelData?[0][1] ?? 0) - (0.03 + 0.0001)) < 1e-5)
        audio.close(stream: 3)
    }

    @Test func badStreamsAreRefused() {
        let audio = GameAudio(offline: true) { _, _, _, _ in }
        #expect(!audio.open(stream: 1, channels: 0, rate: 48_000))
        #expect(!audio.open(stream: 1, channels: 2, rate: 0))
        #expect(!audio.resume(stream: 1))
        #expect(audio.open(stream: 1, channels: 2, rate: 48_000))
        // (one stream a handle)
        #expect(!audio.open(stream: 1, channels: 2, rate: 48_000))
        audio.close(stream: 1)
        audio.close(stream: 1)
    }
}
