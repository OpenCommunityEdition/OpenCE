// The dev panel (the shell's `dev.panel`), over the game's right side with
// the mouse free: the object clicked in the frame, inspected as the
// shell's `inspect` gives it (its tag, shader, animation, physics, light
// and sounds); the frame's timings (the GPU's passes, the CPU's scopes,
// the hitches since the level started); and the voices playing by sound
// class. It is read again four times a second, so that it can be read.
import Foundation
import OpenCEEngineCore
import OpenCEEngineRender
import SwiftUI

@MainActor
@Observable
public final class DevPanelModel {
    /// Where the panel reads what it shows.
    public struct Sources {
        /// The shell's `inspect object=<id>`.
        public var inspect: (UInt32) -> ShellOutput
        public var gpu: () -> [PassTime]
        public var cpu: () -> [ScopeTime]
        public var hitches: () -> [HitchLog.Entry]
        public var sounds: () -> [SoundClassCount]

        public init(
            inspect: @escaping (UInt32) -> ShellOutput, gpu: @escaping () -> [PassTime],
            cpu: @escaping () -> [ScopeTime], hitches: @escaping () -> [HitchLog.Entry],
            sounds: @escaping () -> [SoundClassCount]
        ) {
            self.inspect = inspect
            self.gpu = gpu
            self.cpu = cpu
            self.hitches = hitches
            self.sounds = sounds
        }

        /// The engine's, the GPU timings' and the hitch log's.
        public static func engine(_ engine: Engine) -> Sources {
            Sources(
                inspect: { engine.runCommand(line: "inspect object=\($0)") },
                gpu: { GpuTimings.shared.lastFrame }, cpu: { engine.frameScopes() },
                hitches: { HitchLog.shared.entries }, sounds: { engine.audioCensus() })
        }
    }

    /// What the inspector says with nothing picked.
    public static let unpicked = "Click an object to inspect it."
    /// What it says for a click on no object.
    public static let nothing = "Nothing there: the level or the sky."
    /// How often it is read again, in seconds.
    public static let interval = 0.25

    /// The object inspected, by its id in the snapshot (nil for none).
    public private(set) var inspected: UInt32?
    public private(set) var inspection = DevPanelModel.unpicked
    public private(set) var gpu: [PassTime] = []
    public private(set) var cpu: [ScopeTime] = []
    public private(set) var hitches: [HitchLog.Entry] = []
    public private(set) var sounds: [SoundClassCount] = []
    private let sources: Sources
    private var lastRead: Double?

    public init(sources: Sources) {
        self.sources = sources
    }

    /// The GPU's time of the frame, in milliseconds.
    public var gpuMilliseconds: Double { gpu.reduce(0) { $0 + $1.milliseconds } }

    /// Inspects the object `id` picked under the cursor (nil for none).
    public func pick(_ id: UInt32?) {
        inspected = id
        guard id != nil else {
            inspection = Self.nothing
            return
        }
        inspect()
    }

    /// Reads it all again, if `interval` has passed since the last time.
    public func update(now: Double) {
        if let lastRead, now - lastRead < Self.interval { return }
        lastRead = now
        gpu = sources.gpu()
        cpu = sources.cpu()
        hitches = sources.hitches()
        sounds = sources.sounds()
        // (its motion and light change as it plays)
        if inspected != nil { inspect() }
    }

    /// Forgets the object picked, as a new level has none of the last's.
    public func reset() {
        inspected = nil
        inspection = Self.unpicked
        lastRead = nil
    }

    private func inspect() {
        guard let id = inspected else { return }
        let output = sources.inspect(id)
        inspection = output.text
        // (one gone, as a grenade that went off, is no longer read)
        if !output.ok { inspected = nil }
    }
}

public struct DevPanelView: View {
    let model: DevPanelModel

    public init(model: DevPanelModel) {
        self.model = model
    }

    public var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 12) {
                section("Object") {
                    Text(model.inspection).textSelection(.enabled)
                }
                section("Frame") {
                    row("GPU", String(format: "%.2f ms", model.gpuMilliseconds))
                    ForEach(Array(model.gpu.enumerated()), id: \.offset) { _, pass in
                        row("  " + pass.name, String(format: "%.2f", pass.milliseconds))
                    }
                    Text("CPU").padding(.top, 4)
                    ForEach(Array(model.cpu.enumerated()), id: \.offset) { _, scope in
                        row(
                            String(repeating: "  ", count: Int(min(scope.depth, 8)) + 1) + scope.name,
                            String(format: "%.2f", scope.milliseconds))
                    }
                    Text(model.hitches.isEmpty ? "No hitches" : "Hitches").padding(.top, 4)
                    ForEach(Array(model.hitches.enumerated()), id: \.offset) { _, hitch in
                        row(
                            "  \(hitch.kind.rawValue) \(hitch.cause) ×\(hitch.count)",
                            String(format: "%.1f", hitch.longestMilliseconds))
                    }
                }
                section("Sounds") {
                    if model.sounds.isEmpty {
                        Text("None playing")
                    }
                    ForEach(model.sounds, id: \.class) { count in
                        row(count.class, "\(count.playing)  culled \(count.culled)  failed \(count.failed)")
                    }
                }
            }
            .padding(12)
            .frame(maxWidth: .infinity, alignment: .leading)
        }
        .font(MenuFonts.body(11))
        .foregroundStyle(.white)
        .background(Color.black.opacity(0.7))
    }

    private func section(_ title: String, @ViewBuilder _ content: () -> some View) -> some View {
        VStack(alignment: .leading, spacing: 2) {
            Text(title).font(MenuFonts.heading(13))
            content()
        }
    }

    private func row(_ name: String, _ value: String) -> some View {
        HStack {
            Text(name).lineLimit(1)
            Spacer()
            Text(value).monospacedDigit()
        }
    }
}
