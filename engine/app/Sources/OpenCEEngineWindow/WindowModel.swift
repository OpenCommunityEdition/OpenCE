// What the engine's window decides, apart from AppKit: its size in pixels,
// when the mouse is captured, the mouse's raw motion between frames, the
// keys held, the frame times, and the display link's rate: the frame cap
// in front, a few frames a second behind other windows, none when the
// window cannot be seen, and at most a few frames in flight on the GPU.

import AppKit
import CoreGraphics
import QuartzCore
import Synchronization

/// The drawable's size: the view's points at the screen's scale.
public enum Viewport {
    /// The pixels of `points` at `scale`, rounded as AppKit's backing is.
    public static func pixelSize(points: CGSize, scale: CGFloat) -> (width: Int, height: Int) {
        (Int((points.width * scale).rounded()), Int((points.height * scale).rounded()))
    }
}

/// When the mouse is captured: while the window has the focus, until
/// Escape lets it go; a click in the window takes it again.
public struct MouseCapture: Equatable, Sendable {
    public enum Event: Sendable {
        case focused, unfocused, escape, click
    }

    public private(set) var focused = false
    public private(set) var captured = false
    /// Whether it may capture at all (not while a menu is shown).
    public private(set) var allowed = true

    public init() {}

    /// Allows capturing or not (not: the mouse is let go); true if
    /// `captured` changed.
    public mutating func allow(_ allowed: Bool) -> Bool {
        let before = captured
        self.allowed = allowed
        captured = captured && allowed
        return captured != before
    }

    /// Handles an event; true if `captured` changed.
    public mutating func handle(_ event: Event) -> Bool {
        let before = captured
        switch event {
        case .focused:
            focused = true
            captured = allowed
        case .unfocused:
            focused = false
            captured = false
        case .escape:
            captured = false
        case .click:
            captured = focused && allowed
        }
        return captured != before
    }
}

/// The mouse's motion since the last frame, in the device's own counts
/// (no acceleration, no scaling): right and up positive.
public struct MouseMotion: Equatable, Sendable {
    public private(set) var dx: Double = 0
    public private(set) var dy: Double = 0

    public init() {}

    public mutating func add(dx: Double, dy: Double) {
        guard dx.isFinite, dy.isFinite else { return }
        self.dx += dx
        self.dy += dy
    }

    /// The motion so far, which then starts again from nothing.
    public mutating func take() -> (dx: Double, dy: Double) {
        defer { self = MouseMotion() }
        return (dx, dy)
    }
}

/// The keys held down, by AppKit's key codes. All are let go when the
/// window loses the focus, so that none stays held while it is away.
public struct HeldKeys: Equatable, Sendable {
    public private(set) var codes: Set<UInt16> = []

    public init() {}

    public mutating func press(_ code: UInt16) { codes.insert(code) }
    public mutating func release(_ code: UInt16) { codes.remove(code) }
    public mutating func releaseAll() { codes.removeAll() }
    public func isHeld(_ code: UInt16) -> Bool { codes.contains(code) }
}

/// AppKit's key codes the window itself handles.
public enum KeyCode {
    public static let escape: UInt16 = 53
    /// F3 shows or hides the frame times.
    public static let frameTimes: UInt16 = 99
}

/// The mouse's buttons, held as keys: codes past AppKit's key codes
/// (which are below 128).
public enum MouseButton {
    public static let left: UInt16 = 0x100
    public static let right: UInt16 = 0x101
    public static let middle: UInt16 = 0x102
    /// The buttons past the middle one (the fourth and fifth).
    public static let fourth: UInt16 = 0x103
    public static let fifth: UInt16 = 0x104
    public static let all: [UInt16] = [left, right, middle, fourth, fifth]

    /// The code of AppKit's `buttonNumber` (0 left, 1 right, 2 middle).
    public static func code(_ buttonNumber: Int) -> UInt16 {
        0x100 + UInt16(clamping: max(0, min(buttonNumber, 4)))
    }
}

/// The modifier keys held as keys: Shift, Control and Option, left and
/// right (Command is the menus').
public enum ModifierKey {
    /// The flag a modifier's key code sets, or nil for other keys.
    public static func flag(_ code: UInt16) -> NSEvent.ModifierFlags? {
        switch code {
        case 56, 60: .shift
        case 59, 62: .control
        case 58, 61: .option
        default: nil
        }
    }
}

/// The frame times of the last few seconds, for the overlay: the mean and
/// the slowest frame, in milliseconds.
public struct FrameTimes: Equatable, Sendable {
    public let capacity: Int
    public private(set) var seconds: [Double] = []
    private var next = 0

    public init(capacity: Int = 240) {
        self.capacity = max(capacity, 1)
    }

    public mutating func add(_ frame: Double) {
        guard frame.isFinite, frame > 0 else { return }
        if seconds.count < capacity {
            seconds.append(frame)
        } else {
            seconds[next] = frame
        }
        next = (next + 1) % capacity
    }

    public var meanMilliseconds: Double {
        seconds.isEmpty ? 0 : seconds.reduce(0, +) / Double(seconds.count) * 1000
    }

    public var slowestMilliseconds: Double { (seconds.max() ?? 0) * 1000 }

    /// The line the overlay shows.
    public var text: String {
        guard !seconds.isEmpty else { return "–" }
        let mean = meanMilliseconds
        return String(format: "%.2f ms  %.0f fps  slowest %.2f ms", mean, 1000 / mean, slowestMilliseconds)
    }
}

public enum FrameRate {
    /// The display link's range for a rate: that rate exactly, no faster
    /// and no slower.
    public static func range(maximumFramesPerSecond: Int) -> CAFrameRateRange {
        let rate = Float(max(maximumFramesPerSecond, 1))
        return CAFrameRateRange(minimum: rate, maximum: rate, preferred: rate)
    }
}

/// How much of the window the player has.
public enum WindowActivity: Sendable, Equatable {
    /// In front, with the keys.
    case focused
    /// Seen, behind another window or app.
    case unfocused
    /// Minimised, covered or on another Space.
    case hidden

    public init(key: Bool, appActive: Bool, visible: Bool, miniaturised: Bool) {
        if miniaturised || !visible {
            self = .hidden
        } else {
            self = key && appActive ? .focused : .unfocused
        }
    }
}

/// How fast the window draws.
public enum FramePacing {
    /// The rate behind another window: enough to show the game going on.
    public static let idleRate = 10

    /// The frames a second for `activity` on a display of `display` Hz
    /// with the setting's `cap` (0 the display's rate), or nil when none
    /// are drawn. `keepsRunning` (a network game, the control socket) keeps
    /// the cap's rate whatever the window, since others wait on its frames.
    public static func rate(display: Int, cap: Int, activity: WindowActivity, keepsRunning: Bool) -> Int? {
        let display = max(display, 1)
        let capped = cap > 0 ? min(cap, display) : display
        if keepsRunning {
            return capped
        }
        switch activity {
        case .focused:
            return capped
        case .unfocused:
            return min(idleRate, capped)
        case .hidden:
            return nil
        }
    }
}

/// The frames the GPU has yet to finish, held to a limit: a frame begun
/// past it is skipped, not waited for, so that the main thread never
/// blocks and the CPU runs no further ahead of the GPU than the limit.
public final class FramesInFlight: Sendable {
    public let limit: Int
    private let frames = Mutex(0)

    public init(limit: Int) {
        self.limit = max(limit, 1)
    }

    public var count: Int { frames.withLock { $0 } }

    /// Takes a place for a frame, false when every place is taken.
    public func begin() -> Bool {
        frames.withLock { count in
            guard count < limit else { return false }
            count += 1
            return true
        }
    }

    /// Gives a frame's place back (from the GPU's completion handler).
    public func end() {
        frames.withLock { count in count = max(count - 1, 0) }
    }
}
