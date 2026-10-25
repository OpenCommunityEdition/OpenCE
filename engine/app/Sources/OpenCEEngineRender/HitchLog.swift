// What may stall a frame in play: a pipeline compiled, a texture or buffer
// uploaded, a buffer or target allocated. Each place the renderer does one
// is wrapped in `note`, which times it while a level is played (not while
// it loads); the hitches are kept by kind and cause, for perf.log and the
// tests. None should happen once a level plays: Phase 10 removes those that do.
import Dispatch
import Synchronization

public final class HitchLog: Sendable {
    /// The app's (and the tests').
    public static let shared = HitchLog()

    public enum Kind: String, Sendable {
        case compile
        case upload
        case allocation
    }

    /// The hitches of one kind and cause.
    public struct Entry: Sendable, Equatable {
        public let kind: Kind
        public let cause: String
        public var count: Int
        public var totalMilliseconds: Double
        public var longestMilliseconds: Double

        public init(kind: Kind, cause: String, count: Int, totalMilliseconds: Double, longestMilliseconds: Double) {
            self.kind = kind
            self.cause = cause
            self.count = count
            self.totalMilliseconds = totalMilliseconds
            self.longestMilliseconds = longestMilliseconds
        }
    }

    private struct State {
        var playing = false
        /// Since the level started.
        var entries: [Entry] = []
        /// Since the last `take`.
        var recent: [Entry] = []
    }

    private let state = Mutex(State())

    init() {}

    /// Whether a level is played (the app sets it: no level loading, no
    /// loading screen).
    public var playing: Bool {
        get { state.withLock { $0.playing } }
        set { state.withLock { $0.playing = newValue } }
    }

    /// The hitches since the level started (or the last `reset`).
    public var entries: [Entry] { state.withLock { $0.entries } }

    /// The hitches since the last call, and forgets them.
    public func take() -> [Entry] {
        state.withLock { state in
            defer { state.recent = [] }
            return state.recent
        }
    }

    public func reset() {
        state.withLock { state in
            state.entries = []
            state.recent = []
        }
    }

    /// Runs `body`, a `kind` of hitch for `cause`, timed while a level is played.
    @discardableResult
    public func note<T>(_ kind: Kind, _ cause: @autoclosure () -> String, _ body: () throws -> T) rethrows -> T {
        guard playing else { return try body() }
        let start = DispatchTime.now().uptimeNanoseconds
        defer {
            let milliseconds = Double(DispatchTime.now().uptimeNanoseconds - start) / 1e6
            let cause = cause()
            state.withLock { state in
                Self.add(kind, cause, milliseconds, to: &state.entries)
                Self.add(kind, cause, milliseconds, to: &state.recent)
            }
        }
        return try body()
    }

    private static func add(_ kind: Kind, _ cause: String, _ milliseconds: Double, to entries: inout [Entry]) {
        if let index = entries.firstIndex(where: { $0.kind == kind && $0.cause == cause }) {
            entries[index].count += 1
            entries[index].totalMilliseconds += milliseconds
            entries[index].longestMilliseconds = max(entries[index].longestMilliseconds, milliseconds)
        } else {
            entries.append(
                Entry(
                    kind: kind, cause: cause, count: 1, totalMilliseconds: milliseconds,
                    longestMilliseconds: milliseconds))
        }
    }
}
