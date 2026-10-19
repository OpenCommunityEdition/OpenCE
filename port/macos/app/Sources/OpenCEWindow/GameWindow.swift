// The game's windows: what the host's window imports
// (port/macos/host/host_window.c) make and measure, through the app's
// backend (Sources/OpenCE/WindowBackend.swift), and their keyboard, mouse
// and focus (GameInput.swift).

import AppKit
import OpenCEShell

/// A resizable window that can be fullscreen in a Space of its own, whose
/// content is the game's view.
public final class GameWindow: NSWindow {
    public let gameView: GameView
    /// Called when the window closes (the player's close button, or close()).
    var closed: (() -> Void)?

    init(title: String, contentSize: CGSize) {
        let frame = NSRect(origin: .zero, size: contentSize)
        gameView = GameView(frame: frame)
        super.init(contentRect: frame, styleMask: [.titled, .closable, .miniaturizable, .resizable],
                   backing: .buffered, defer: false)
        self.title = title
        isReleasedWhenClosed = false
        collectionBehavior = [.fullScreenPrimary, .managed]
        contentMinSize = NSSize(width: 320, height: 240)
        backgroundColor = .black
        contentView = gameView
        acceptsMouseMovedEvents = true
        initialFirstResponder = gameView
        makeFirstResponder(gameView)
    }

    public override func close() {
        super.close()
        let closed = self.closed
        self.closed = nil
        closed?()
    }
}

/// The windows the guest has made, by the handles it holds (nonzero).
@MainActor
public final class GameWindows {
    private var windows: [UInt32: GameWindow] = [:]
    private var nextHandle: UInt32 = 1
    private var focusObservers: [UInt32: [NSObjectProtocol]] = [:]
    /// The window the mouse is captured for.
    private var captured: UInt32?
    /// Called when the player closes one of the windows.
    public var onClose: ((UInt32) -> Void)?
    /// Where the windows' keyboard, mouse and focus go.
    public var input: GameInputSink?
    /// The pointer the windows capture.
    public var pointer: PointerControl = SystemPointer()

    public init() {}

    /// Makes a window of about `width` by `height` points (fitted to the
    /// screen) and, with `show`, puts it in front; its handle, or 0.
    public func create(title: String, width: Int, height: Int, show: Bool = true) -> UInt32 {
        guard width > 0, height > 0, nextHandle != 0 else { return 0 }
        let requested = CGSize(width: width, height: height)
        let visible = NSScreen.main?.visibleFrame.size ?? requested
        let window = GameWindow(title: title, contentSize: WindowLayout.initialContentSize(requested: requested,
                                                                                          visible: visible))
        let handle = nextHandle
        nextHandle &+= 1
        windows[handle] = window
        window.gameView.onInput = { [weak self] event in
            self?.input?.gameInput(window: handle, event)
        }
        focusObservers[handle] = [NSWindow.didBecomeKeyNotification, NSWindow.didResignKeyNotification].map { name in
            NotificationCenter.default.addObserver(forName: name, object: window, queue: nil) { [weak self] _ in
                MainActor.assumeIsolated {
                    self?.input?.gameInput(window: handle, .focus(name == NSWindow.didBecomeKeyNotification))
                }
            }
        }
        window.closed = { [weak self] in
            self?.closed(handle)
        }
        if show {
            window.center()
            window.makeKeyAndOrderFront(nil)
            window.gameView.clear()
        }
        return handle
    }

    public func window(_ handle: UInt32) -> GameWindow? {
        windows[handle]
    }

    /// The window's drawable size in pixels now; nil for a handle it does
    /// not have.
    public func sizeInPixels(_ handle: UInt32) -> (width: Int, height: Int)? {
        windows[handle]?.gameView.sizeInPixels
    }

    public func close(_ handle: UInt32) {
        windows[handle]?.close()
    }

    private func closed(_ handle: UInt32) {
        if captured == handle, let window = windows[handle] {
            captured = nil
            pointer.setCaptured(false, in: window)
        }
        focusObservers[handle]?.forEach(NotificationCenter.default.removeObserver)
        focusObservers[handle] = nil
        windows[handle] = nil
        onClose?(handle)
    }

    /// Captures the mouse for the window (hidden, held in it, moving only
    /// relatively), or lets it go; false for a handle it does not have. The
    /// host lets it go while the window has not the focus (host_input.c).
    public func setRelativeMouse(_ handle: UInt32, _ enabled: Bool) -> Bool {
        guard let window = windows[handle] else { return false }
        if enabled, captured != handle {
            if let other = captured, let otherWindow = windows[other] {
                pointer.setCaptured(false, in: otherWindow)
            }
            captured = handle
            pointer.setCaptured(true, in: window)
        } else if !enabled, captured == handle {
            captured = nil
            pointer.setCaptured(false, in: window)
        }
        return true
    }
}
