// The developer state the dev panel, the shell, the control socket and the
// launcher's Developer tab share, as the renderer reads it: the drawing
// switches (`render.*`), the lighting controls (`tune.*`) and the panel's
// switch (`dev.panel`). The engine counts each change of its variables;
// the state is read again only when the count moves, so a frame whose
// state nobody touched reads nothing.
import Foundation

/// The lighting controls (the shell's `tune.*`), at the stock frame's
/// values by default.
public struct LightingTuning: Equatable, Sendable {
    /// Stops added before the tone map.
    public var exposure: Float = 0
    /// The sun's light, and its colour's channels, scaled.
    public var sun: Float = 1
    public var sunColor = SIMD3<Float>(repeating: 1)
    /// The baked bounced light, scaled.
    public var bounce: Float = 1
    /// The probes' light on the objects, scaled.
    public var probes: Float = 1
    /// How much the ambient occlusion darkens, scaled.
    public var ambientOcclusion: Float = 1
    /// The shaders' own light, scaled.
    public var emissive: Float = 1
    /// The bloom's strength, scaled.
    public var bloom: Float = 1
    /// The tone curve's contrast.
    public var toneCurve: Float = 1

    public init() {}

    /// The shell's names of the controls.
    public static let names = [
        "tune.exposure", "tune.sun", "tune.sun_red", "tune.sun_green", "tune.sun_blue", "tune.bounce",
        "tune.probes", "tune.ao", "tune.emissive", "tune.bloom", "tune.tone_curve",
    ]

    /// The controls as the shell's variables hold them, read as text
    /// through `value` (one that is not a number keeps its default).
    public init(value: (String) -> String) {
        func read(_ name: String, _ otherwise: Float) -> Float {
            Float(value(name)).flatMap { $0.isFinite ? $0 : nil } ?? otherwise
        }
        exposure = read("tune.exposure", exposure)
        sun = read("tune.sun", sun)
        sunColor = SIMD3(read("tune.sun_red", 1), read("tune.sun_green", 1), read("tune.sun_blue", 1))
        bounce = read("tune.bounce", bounce)
        probes = read("tune.probes", probes)
        ambientOcclusion = read("tune.ao", ambientOcclusion)
        emissive = read("tune.emissive", emissive)
        bloom = read("tune.bloom", bloom)
        toneCurve = read("tune.tone_curve", toneCurve)
    }

    /// Whether every control is at the stock frame's value.
    public var isStock: Bool { self == LightingTuning() }
}

/// What the developer tools have set.
public struct DevState: Equatable, Sendable {
    public var switches = RenderSwitches()
    public var tuning = LightingTuning()
    /// Whether the dev panel shows.
    public var panel = false

    public init() {}

    /// The state as the shell's variables hold it, read through `flag` and
    /// `value`.
    public init(flag: (String) -> Bool, value: (String) -> String) {
        switches = RenderSwitches(flag: flag, value: value)
        tuning = LightingTuning(value: value)
        panel = flag("dev.panel")
    }
}

/// Reads the developer state again only when the engine's count of its
/// changes moved.
public final class DevStateReader {
    public private(set) var state = DevState()
    /// The count the state was read at (none before the first read).
    private var generation: UInt64?
    /// How many times the state was read, for the tests.
    public private(set) var reads = 0

    public init() {}

    /// The state at `generation`: read through `flag` and `value` if it
    /// moved, else the one held.
    @discardableResult
    public func update(generation: UInt64, flag: (String) -> Bool, value: (String) -> String) -> DevState {
        if generation != self.generation {
            state = DevState(flag: flag, value: value)
            self.generation = generation
            reads += 1
        }
        return state
    }

    /// Forgets the count: the next update reads the state.
    public func invalidate() {
        generation = nil
    }
}
