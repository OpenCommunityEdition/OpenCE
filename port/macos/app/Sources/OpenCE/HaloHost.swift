// The C host (port/macos/host/halo_host.h): the one place the app calls it.

import CHaloHost
import Foundation
import os

enum HaloHost {
    static let log = Logger(subsystem: "org.opence.OpenCE", category: "host")

    /// Runs the guest image's main with `arguments` (the first is its
    /// program name) on a thread of its own, which becomes the guest's
    /// main thread. The host ends the process when the game quits; if the
    /// VM cannot be created, `failed` is called on the main actor.
    static func start(image: URL, arguments: [String], failed: @escaping @MainActor @Sendable () -> Void) {
        let thread = Thread {
            // The C strings are copied once and never freed: halo_host_run
            // keeps argv for the guest's lifetime, which is the process's.
            let argv = arguments.map { strdup($0) }
            var table = argv + [nil]
            let result = table.withUnsafeMutableBufferPointer { buffer in
                // sound: the buffer outlives the call, and halo_host_run
                // reads at most argc entries from it
                halo_host_run(image.path, Int32(arguments.count), buffer.baseAddress)
            }
            log.error("the host could not create the guest's VM (\(result))")
            Task { @MainActor in failed() }
        }
        thread.name = "guest main"
        // (the guest's own stacks are in guest memory; this is the host's
        // side of its main thread, as large as a process's main thread)
        thread.stackSize = 8 << 20
        thread.start()
    }
}
