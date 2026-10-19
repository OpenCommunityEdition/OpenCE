// The app's windows for the C host (halo_host_set_window_backend): the
// guest's window is an AppKit window with a Metal layer (OpenCEWindow), not
// SDL's.

import AppKit
import CHaloHost
import OpenCEWindow

@MainActor
enum WindowBackend {
    static let windows = GameWindows()

    /// Installs the windows, before the guest starts. The host calls them on
    /// the main thread (they are main-thread imports); closing the game's
    /// window quits.
    static func install() {
        windows.onClose = { _ in NSApp.terminate(nil) }
        var backend = halo_host_window_backend()
        backend.create = { _, title, width, height, _ in
            // (the guest's flags are Android's, fullscreen: the window opens
            // as a window, which the player can make fullscreen)
            let title = title.map { String(cString: $0) } ?? ""
            return MainActor.assumeIsolated {
                WindowBackend.windows.create(title: title, width: Int(width), height: Int(height))
            }
        }
        backend.size_in_pixels = { _, window, width, height in
            let size = MainActor.assumeIsolated { WindowBackend.windows.sizeInPixels(window) }
            width?.pointee = Int32(clamping: size?.width ?? 0)
            height?.pointee = Int32(clamping: size?.height ?? 0)
        }
        halo_host_set_window_backend(&backend)
    }
}
