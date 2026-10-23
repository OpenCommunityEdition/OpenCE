// The loading screen's chant (engine/crates/ffi/src/loading.rs): four
// voices of the one tone, each at its own rate and gain, looped while the
// screen shows. The tone is the player's own, made from their default.xbe
// into the cache; without it the screen is silent.
import AVFoundation
import Foundation
import OpenCEEngineCore

public final class ChantPlayer {
    private let engine = AVAudioEngine()
    private var voices: [(player: AVAudioPlayerNode, speed: AVAudioUnitVarispeed)] = []
    private var buffer: AVAudioPCMBuffer?
    private var playing = false

    /// Where the player's default.xbe may be: beside each data folder,
    /// in it, and in the checkout's Haloce_dump.
    public static func executables(folders: [URL], root: URL) -> [URL] {
        folders.flatMap { [$0.deletingLastPathComponent(), $0] }.map { $0.appendingPathComponent("default.xbe") }
            + [root.appendingPathComponent("Haloce_dump/default.xbe")]
    }

    /// The chant's file in `cacheFolder`, made from the first executable
    /// of `executables` that holds the tone; nil when none does.
    public static func wave(executables: [URL], cacheFolder: String) -> URL? {
        // (the cache's file is taken first, whichever path is given)
        for xbe in executables.isEmpty ? [URL(fileURLWithPath: "/nonexistent")] : executables {
            if let made = chantWave(xbePath: xbe.path, cacheFolder: cacheFolder) {
                return URL(fileURLWithPath: made)
            }
        }
        return nil
    }

    /// A player of the tone in `wave` (silent without one).
    public init(wave: URL?) {
        guard let wave, let file = try? AVAudioFile(forReading: wave),
              let buffer = AVAudioPCMBuffer(pcmFormat: file.processingFormat,
                                            frameCapacity: AVAudioFrameCount(file.length)),
              (try? file.read(into: buffer)) != nil else {
            return
        }
        self.buffer = buffer
        for _ in 0..<4 {
            let voice = (player: AVAudioPlayerNode(), speed: AVAudioUnitVarispeed())
            engine.attach(voice.player)
            engine.attach(voice.speed)
            engine.connect(voice.player, to: voice.speed, format: buffer.format)
            engine.connect(voice.speed, to: engine.mainMixerNode, format: buffer.format)
            voice.player.volume = 0
            voices.append(voice)
        }
    }

    /// Whether there is a tone to play.
    public var hasTone: Bool { buffer != nil }

    /// The voices as `view` has them; they start with the first that is
    /// heard and stop once the screen is done.
    public func apply(_ view: LoadingView) {
        guard let buffer else { return }
        if view.stage == .done {
            stop()
            return
        }
        for (index, voice) in voices.enumerated() {
            voice.player.volume = index < view.gains.count ? view.gains[index] : 0
            voice.speed.rate = index < view.rates.count ? max(0.25, min(4, view.rates[index])) : 1
        }
        guard !playing, view.gains.contains(where: { $0 > 0 }) else { return }
        do {
            try engine.start()
        } catch {
            return // (no output device: the screen is silent)
        }
        for voice in voices {
            voice.player.scheduleBuffer(buffer, at: nil, options: .loops)
            voice.player.play()
        }
        playing = true
    }

    /// Silences the voices.
    public func stop() {
        guard playing else { return }
        for voice in voices {
            voice.player.stop()
        }
        engine.stop()
        playing = false
    }

    /// Whether the voices are playing.
    public var isPlaying: Bool { playing }
}
