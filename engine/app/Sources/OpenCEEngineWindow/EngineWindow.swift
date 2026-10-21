// The engine's window: resizable to any size and shape, full screen in a
// Space of its own, with the mouse captured while it has the focus
// (WindowModel.swift's MouseCapture) and the mouse's raw motion read from
// GameController's GCMouse, without the system's acceleration.

import AppKit
import CoreGraphics
import GameController

/// The system's pointer, captured for a window or let go.
@MainActor
public protocol PointerControl: AnyObject {
    func setCaptured(_ captured: Bool, in window: NSWindow)
}

/// Captured, the pointer is hidden and held at the window's middle, and
/// only the mouse's motion is read.
@MainActor
public final class SystemPointer: PointerControl {
    private var hidden = false

    public init() {}

    public func setCaptured(_ captured: Bool, in window: NSWindow) {
        if captured {
            if let view = window.contentView {
                let frame = window.convertToScreen(view.convert(view.bounds, to: nil))
                // (CoreGraphics's coordinates are from the main display's top left)
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

/// A mouse's raw motion: right and up positive, in the device's counts.
@MainActor
public protocol RawMouse: AnyObject {
    /// Whether a mouse is there to be read.
    var available: Bool { get }
    var onMotion: ((Double, Double) -> Void)? { get set }
}

/// The current GCMouse's motion, as its HID reports give it.
@MainActor
public final class GameControllerMouse: RawMouse {
    public var onMotion: ((Double, Double) -> Void)?
    private var observers: [NSObjectProtocol] = []

    public init() {
        for name in [NSNotification.Name.GCMouseDidBecomeCurrent, .GCMouseDidStopBeingCurrent] {
            observers.append(NotificationCenter.default.addObserver(forName: name, object: nil, queue: .main) {
                [weak self] _ in
                MainActor.assumeIsolated { self?.follow(GCMouse.current) }
            })
        }
        follow(GCMouse.current)
    }

    public var available: Bool { GCMouse.current != nil }

    private func follow(_ mouse: GCMouse?) {
        mouse?.handlerQueue = .main
        mouse?.mouseInput?.mouseMovedHandler = { [weak self] _, dx, dy in
            MainActor.assumeIsolated { self?.onMotion?(Double(dx), Double(dy)) }
        }
    }
}

/// A window of any size and shape that can be full screen.
public final class EngineWindow: NSWindow {
    public let engineView: EngineView

    public init(title: String, contentSize: CGSize) {
        let frame = NSRect(origin: .zero, size: contentSize)
        engineView = EngineView(frame: frame)
        super.init(contentRect: frame, styleMask: [.titled, .closable, .miniaturizable, .resizable],
                   backing: .buffered, defer: false)
        self.title = title
        isReleasedWhenClosed = false
        collectionBehavior = [.fullScreenPrimary, .managed]
        contentMinSize = NSSize(width: 320, height: 200)
        backgroundColor = .black
        contentView = engineView
        acceptsMouseMovedEvents = true
        initialFirstResponder = engineView
        makeFirstResponder(engineView)
    }
}

/// The window and its input: the mouse captured with the focus, its raw
/// motion while captured, and the keys held.
@MainActor
public final class EngineWindowController {
    public let window: EngineWindow
    public private(set) var capture = MouseCapture()
    public private(set) var keys = HeldKeys()
    /// Whether the frame-time overlay is shown (F3 shows or hides it).
    public var showsFrameTimes = false
    private var motion = MouseMotion()
    private let pointer: PointerControl
    private let rawMouse: RawMouse
    private var observers: [NSObjectProtocol] = []

    public init(title: String = "OpenCE Engine", contentSize: CGSize = CGSize(width: 1280, height: 720),
                pointer: PointerControl = SystemPointer(), rawMouse: RawMouse = GameControllerMouse()) {
        window = EngineWindow(title: title, contentSize: contentSize)
        self.pointer = pointer
        self.rawMouse = rawMouse
        let view = window.engineView
        view.onKey = { [weak self] code, down in self?.key(code, down) }
        view.onMouseButton = { [weak self] code, down in self?.mouseButton(code, down) }
        view.onMotion = { [weak self] dx, dy in
            // (AppKit's motion only without a raw mouse, and down positive)
            guard let self, !self.rawMouse.available else { return }
            self.moved(dx: dx, dy: -dy)
        }
        rawMouse.onMotion = { [weak self] dx, dy in self?.moved(dx: dx, dy: dy) }
        for (name, event) in [(NSWindow.didBecomeKeyNotification, MouseCapture.Event.focused),
                              (NSWindow.didResignKeyNotification, .unfocused)] {
            observers.append(NotificationCenter.default.addObserver(forName: name, object: window, queue: nil) {
                [weak self] _ in
                MainActor.assumeIsolated { self?.handle(event) }
            })
        }
    }

    isolated deinit {
        observers.forEach(NotificationCenter.default.removeObserver)
    }

    /// Puts the window in front, centred, and full screen with `fullScreen`.
    public func show(fullScreen: Bool) {
        window.center()
        window.makeKeyAndOrderFront(nil)
        if fullScreen, !window.styleMask.contains(.fullScreen) {
            window.toggleFullScreen(nil)
        }
        window.engineView.clear()
    }

    /// The mouse's raw motion since the last call (nothing while it is not
    /// captured).
    public func takeMouseMotion() -> (dx: Double, dy: Double) {
        motion.take()
    }

    private func moved(dx: Double, dy: Double) {
        if capture.captured {
            motion.add(dx: dx, dy: dy)
        }
    }

    private func key(_ code: UInt16, _ down: Bool) {
        if code == KeyCode.escape {
            if down { handle(.escape) }
            return
        }
        if code == KeyCode.frameTimes {
            if down { showsFrameTimes.toggle() }
            return
        }
        if down { keys.press(code) } else { keys.release(code) }
    }

    /// A mouse button is held as a key while the mouse is captured; a
    /// press while it is not captures it, and is not the player's.
    func mouseButton(_ code: UInt16, _ down: Bool) {
        if down {
            if capture.captured {
                keys.press(code)
            } else {
                handle(.click)
            }
        } else {
            keys.release(code)
        }
    }

    func handle(_ event: MouseCapture.Event) {
        if event == .unfocused {
            keys.releaseAll()
        }
        guard capture.handle(event) else { return }
        if !capture.captured {
            // (a button let go after the mouse is is not seen)
            MouseButton.all.forEach { keys.release($0) }
        }
        // (motion from before the capture changed is not the player's look)
        _ = motion.take()
        pointer.setCaptured(capture.captured, in: window)
    }
}
