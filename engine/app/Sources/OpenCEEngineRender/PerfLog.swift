// perf.log: a line a second of the frames' CPU times, the last frame's GPU
// time and its costliest passes, and the hitches of that second with their
// causes. The app writes it beside its settings (Application Support/OpenCE
// Engine/perf.log), started again at each launch.
import Foundation

public final class PerfLog {
    public let url: URL
    private let file: FileHandle
    /// The CPU's milliseconds of the frames since the last line.
    private var frames: [Double] = []
    private var lastLine: Double?
    /// The passes a line names.
    static let passesShown = 5

    /// Starts the log at `url` again (nil if it cannot be written).
    public init?(url: URL) {
        try? FileManager.default.createDirectory(at: url.deletingLastPathComponent(), withIntermediateDirectories: true)
        guard FileManager.default.createFile(atPath: url.path, contents: nil),
              let file = try? FileHandle(forWritingTo: url) else { return nil }
        self.url = url
        self.file = file
    }

    deinit {
        try? file.close()
    }

    /// Records a frame of `cpuMilliseconds`, `now` seconds into the app's
    /// run; writes the line of the second once it has passed.
    public func frame(cpuMilliseconds: Double, now: Double, gpu: [PassTime], hitches: () -> [HitchLog.Entry]) {
        if cpuMilliseconds.isFinite, cpuMilliseconds > 0 {
            frames.append(cpuMilliseconds)
        }
        guard let last = lastLine else {
            lastLine = now
            return
        }
        guard now - last >= 1 else { return }
        lastLine = now
        let line = Self.line(seconds: now, frames: frames, gpu: gpu, hitches: hitches())
        frames = []
        file.write(Data((line + "\n").utf8))
    }

    /// `12.0 s  cpu 8.31 mean 12.40 max (60 frames)  gpu 6.20: level opaque 2.10, …  hitches: allocation light lists 60× 0.02 (longest 0.05)`
    static func line(seconds: Double, frames: [Double], gpu: [PassTime], hitches: [HitchLog.Entry]) -> String {
        let mean = frames.isEmpty ? 0 : frames.reduce(0, +) / Double(frames.count)
        var line = String(format: "%.1f s  cpu %.2f mean %.2f max (%d frames)", seconds, mean, frames.max() ?? 0,
                          frames.count)
        if !gpu.isEmpty {
            // (the passes of one name, each of a light's faces say, added)
            var named: [(name: String, milliseconds: Double)] = []
            for pass in gpu {
                let name = pass.name.split(separator: " ").filter { $0.first?.isNumber != true }.joined(separator: " ")
                if let index = named.firstIndex(where: { $0.name == name }) {
                    named[index].milliseconds += pass.milliseconds
                } else {
                    named.append((name, pass.milliseconds))
                }
            }
            let total = gpu.reduce(0) { $0 + $1.milliseconds }
            let costliest = named.sorted { $0.milliseconds > $1.milliseconds }.prefix(passesShown)
                .map { String(format: "%@ %.2f", $0.name, $0.milliseconds) }
            line += String(format: "  gpu %.2f: ", total) + costliest.joined(separator: ", ")
        }
        if !hitches.isEmpty {
            line += "  hitches: " + hitches.map {
                String(format: "%@ %@ %d× %.2f (longest %.2f)", $0.kind.rawValue, $0.cause, $0.count,
                       $0.totalMilliseconds, $0.longestMilliseconds)
            }.joined(separator: ", ")
        }
        return line
    }
}
