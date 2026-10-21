// The free camera's controls: the keys held and the mouse's raw motion
// since the last frame, as the engine's input. W, S, A and D move, Space
// rises, Control sinks, Shift is faster, and the mouse turns.
import OpenCEEngineCore

/// The keys and mouse as the engine's input.
public struct FlyControls: Equatable, Sendable {
    /// AppKit's key codes (by the key's place, whatever the layout).
    public enum Key {
        public static let w: UInt16 = 13
        public static let a: UInt16 = 0
        public static let s: UInt16 = 1
        public static let d: UInt16 = 2
        public static let space: UInt16 = 49
        public static let leftShift: UInt16 = 56
        public static let rightShift: UInt16 = 60
        public static let leftControl: UInt16 = 59
        public static let rightControl: UInt16 = 62
    }

    /// The turn for a count of the mouse's motion, in radians.
    public var sensitivity: Float
    /// Whether moving the mouse up looks down.
    public var invertPitch: Bool

    public init(sensitivity: Float = 0.0012, invertPitch: Bool = false) {
        self.sensitivity = sensitivity
        self.invertPitch = invertPitch
    }

    /// The input for the keys `held` and the mouse's motion `mouse` (right
    /// and up positive, in counts).
    public func input(held: Set<UInt16>, mouse: (dx: Double, dy: Double)) -> InputState {
        func axis(_ positive: [UInt16], _ negative: [UInt16]) -> Int8 {
            let plus = positive.contains(where: held.contains)
            let minus = negative.contains(where: held.contains)
            // (both held cancel)
            return plus == minus ? 0 : (plus ? 127 : -127)
        }
        func turn(_ counts: Double) -> Float {
            let radians = Float(counts) * sensitivity
            return radians.isFinite ? radians : 0
        }
        let sprint = [Key.leftShift, Key.rightShift].contains(where: held.contains)
        // (the mouse right turns right, which is a negative yaw)
        return InputState(buttons: sprint ? inputButtons().sprint : 0,
                          forward: axis([Key.w], [Key.s]),
                          right: axis([Key.d], [Key.a]),
                          up: axis([Key.space], [Key.leftControl, Key.rightControl]),
                          yaw: -turn(mouse.dx),
                          pitch: invertPitch ? -turn(mouse.dy) : turn(mouse.dy))
    }
}
