// The app's renderer for the C host (halo_host_set_render_backend): the
// guest's render batches (port/macos/include/opence_render.h) are drawn with
// Metal (OpenCERender) in the game window's layer, in place of the host's
// own check, which only counts them.

import CHaloHost
import Foundation
import Metal
import OpenCERender
import OpenCERuntime
import os

enum RenderBackend {
    /// The game's window, once the guest opens it (WindowBackend).
    static let window = LayerWindow()
    private static let made = Made()

    /// The renderer, made at the first batch (the guest's memory is mapped by
    /// then), or nil when it cannot be.
    private final class Made: @unchecked Sendable {
        private let lock = NSLock()
        private var tried = false
        private var made: MetalRenderer?

        var renderer: MetalRenderer? {
            lock.lock()
            defer { lock.unlock() }
            if !tried {
                tried = true
                let log = Logger(subsystem: "org.opence.OpenCE", category: "render")
                guard let device = MTLCreateSystemDefaultDevice(), let memory = runningGuestMemory(),
                      let renderer = MetalRenderer(device: device, memory: memory, window: RenderBackend.window)
                else {
                    log.error("no Metal renderer: the guest's batches are refused")
                    return nil
                }
                renderer.log = { message in log.error("\(message, privacy: .public)") }
                made = renderer
            }
            return made
        }
    }

    /// Installs the renderer, before the guest starts. The host calls it on
    /// the guest's thread that sends each batch.
    static func install() {
        var backend = halo_host_render_backend()
        backend.submit = { _, batch, size in
            RenderBackend.made.renderer?.submit(address: batch, size: size) ?? -1
        }
        halo_host_set_render_backend(&backend)
    }
}
