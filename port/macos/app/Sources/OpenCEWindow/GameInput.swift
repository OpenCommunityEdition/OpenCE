// The game windows' keyboard and mouse: what their views pass on of
// AppKit's events, for the host's event queue (port/macos/host/host_input.c,
// which makes SDL's events of them), and the mouse captured for the game.

import AppKit
import CoreGraphics

/// One of AppKit's events, as the host's queue takes it: key codes are
/// NSEvent's keyCode, positions are in the view's points from its top left,
/// and button numbers are AppKit's (0 left, 1 right, 2 middle).
public enum GameInputEvent: Equatable, Sendable {
    case key(code: UInt16, down: Bool, repeating: Bool)
    /// A modifier key pressed or let go (flagsChanged), with the flags after it.
    case modifiers(code: UInt16, flags: UInt64)
    case mouseMotion(x: Float, y: Float, dx: Float, dy: Float)
    case mouseButton(number: Int, down: Bool, x: Float, y: Float)
    /// The wheel, or a trackpad's scrolling (`precise`, in points).
    case wheel(dx: Float, dy: Float, precise: Bool)
    case focus(Bool)
}

/// Where the windows' events go.
@MainActor
public protocol GameInputSink: AnyObject {
    func gameInput(window: UInt32, _ event: GameInputEvent)
}

/// The mouse pointer, captured for a window or let go.
@MainActor
public protocol PointerControl: AnyObject {
    func setCaptured(_ captured: Bool, in window: NSWindow)
}

/// The system's pointer: captured, it is hidden and held at the window's
/// middle, and the mouse's moves are only relative (as SDL's relative mode
/// has it on the Mac).
@MainActor
public final class SystemPointer: PointerControl {
    private var hidden = false

    public init() {}

    public func setCaptured(_ captured: Bool, in window: NSWindow) {
        if captured {
            if let view = window.contentView {
                let frame = window.convertToScreen(view.convert(view.bounds, to: nil))
                // (CoreGraphics's global coordinates are from the top left
                // of the main display)
                let top = NSScreen.screens.first?.frame.maxY ?? frame.maxY
                CGWarpMouseCursorPosition(CGPoint(x: frame.midX, y: top - frame.midY))
            }
            CGAssociateMouseAndMouseCursorPosition(0)
            if !hidden {
                NSCursor.hide()
                hidden = true
            }
        } else {
            CGAssociateMouseAndMouseCursorPosition(1)
            if hidden {
                NSCursor.unhide()
                hidden = false
            }
        }
    }
}
