// The app's sound for the C host (halo_host_set_audio_backend): the game's
// playback stream is an AVAudioEngine (OpenCEAudio), not SDL's, whose
// source node pulls the stream's frames from the host
// (halo_host_audio_render, port/macos/host/host_audio.c).

import CHaloHost
import OpenCEAudio

enum AudioBackend {
    static let audio = GameAudio { stream, channels, count, frames in
        halo_host_audio_render(stream, channels, count, frames)
    }

    /// Installs the sound, before the guest starts. The host calls it on the
    /// guest's threads.
    static func install() {
        var backend = halo_host_audio_backend()
        backend.open = { _, stream, channels, rate in
            AudioBackend.audio.open(stream: stream, channels: Int(channels), rate: Double(rate)) ? 1 : 0
        }
        backend.resume = { _, stream in
            AudioBackend.audio.resume(stream: stream) ? 1 : 0
        }
        backend.close = { _, stream in
            AudioBackend.audio.close(stream: stream)
        }
        halo_host_set_audio_backend(&backend)
    }
}
