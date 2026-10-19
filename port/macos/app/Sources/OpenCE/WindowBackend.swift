// The app's windows for the C host (halo_host_set_window_backend): the
// guest's window is an AppKit window with a Metal layer (OpenCEWindow), not
// SDL's, and its keyboard and mouse go to the host's event queue
// (halo_host_post_*, port/macos/host/host_input.c).

import AppKit
import CHaloHost
import OpenCEWindow

@MainActor
enum WindowBackend {
    static let windows = GameWindows()
    static let input = HostInput()

    /// Installs the windows, before the guest starts. The host calls them on
    /// the main thread (they are main-thread imports); closing the game's
    /// window quits.
    static func install() {
        windows.onClose = { _ in NSApp.terminate(nil) }
        windows.input = input
        var backend = halo_host_window_backend()
        backend.create = { _, title, width, height, _ in
            // (the guest's flags are Android's, fullscreen: the window opens
            // as a window, which the player can make fullscreen)
            let title = title.map { String(cString: $0) } ?? ""
            return MainActor.assumeIsolated {
                let handle = WindowBackend.windows.create(title: title, width: Int(width), height: Int(height))
                // (the first window is the one the game draws in)
                if RenderBackend.window.layer == nil, let view = WindowBackend.windows.window(handle)?.gameView {
                    view.clearsOnResize = false
                    RenderBackend.window.layer = view.metalLayer
                }
                return handle
            }
        }
        backend.size_in_pixels = { _, window, width, height in
            let size = MainActor.assumeIsolated { WindowBackend.windows.sizeInPixels(window) }
            width?.pointee = Int32(clamping: size?.width ?? 0)
            height?.pointee = Int32(clamping: size?.height ?? 0)
        }
        backend.set_relative_mouse = { _, window, enabled in
            MainActor.assumeIsolated { WindowBackend.windows.setRelativeMouse(window, enabled != 0) } ? 1 : 0
        }
        halo_host_set_window_backend(&backend)
    }
}

/// The windows' events, to the host's queue (on the main thread).
@MainActor
final class HostInput: GameInputSink {
    func gameInput(window: UInt32, _ event: GameInputEvent) {
        switch event {
        case let .key(code, down, repeating):
            halo_host_post_key(window, code, down ? 1 : 0, repeating ? 1 : 0)
        case let .modifiers(code, flags):
            halo_host_post_modifiers(window, code, flags)
        case let .mouseMotion(x, y, dx, dy):
            halo_host_post_mouse_motion(window, x, y, dx, dy)
        case let .mouseButton(number, down, x, y):
            halo_host_post_mouse_button(window, Int32(clamping: number), down ? 1 : 0, x, y)
        case let .wheel(dx, dy, precise):
            halo_host_post_wheel(window, dx, dy, precise ? 1 : 0)
        case let .focus(focused):
            halo_host_post_focus(window, focused ? 1 : 0)
        }
    }
}
