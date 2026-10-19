// The game's windows: what the host's window imports
// (port/macos/host/host_window.c) make and measure, through the app's
// backend (Sources/OpenCE/WindowBackend.swift).

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
    /// Called when the player closes one of the windows.
    public var onClose: ((UInt32) -> Void)?

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
        window.closed = { [weak self] in
            self?.windows[handle] = nil
            self?.onClose?(handle)
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
}
