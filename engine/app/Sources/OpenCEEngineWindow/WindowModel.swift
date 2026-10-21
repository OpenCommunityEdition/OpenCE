// What the engine's window decides, apart from AppKit: its size in pixels,
// when the mouse is captured, the mouse's raw motion between frames, the
// keys held, the frame times, and the display link's rate.

import AppKit
import CoreGraphics
import QuartzCore

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

    public init() {}

    /// Handles an event; true if `captured` changed.
    public mutating func handle(_ event: Event) -> Bool {
        let before = captured
        switch event {
        case .focused:
            focused = true
            captured = true
        case .unfocused:
            focused = false
            captured = false
        case .escape:
            captured = false
        case .click:
            captured = focused
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
    /// The display link's rate: the display's own, however fast, and no
    /// slower.
    public static func range(maximumFramesPerSecond: Int) -> CAFrameRateRange {
        let rate = Float(max(maximumFramesPerSecond, 1))
        return CAFrameRateRange(minimum: rate, maximum: rate, preferred: rate)
    }
}
