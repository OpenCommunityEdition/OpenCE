// OpenCE.app: AppKit on the main thread, the game in its VM on threads of
// its own (docs/macos_host.md).

import AppKit
import os

@MainActor
final class AppDelegate: NSObject, NSApplicationDelegate {
    private let log = Logger(subsystem: "org.opence.OpenCE", category: "app")

    func applicationDidFinishLaunching(_ notification: Notification) {
        guard let image = Bundle.main.url(forResource: "halo_guest", withExtension: "elf") else {
            fail("The game's image, halo_guest.elf, is missing from the app.")
            return
        }
        log.info("starting the guest image \(image.path)")
        // (the guest's program name, then any arguments the app was given)
        HaloHost.start(image: image, arguments: ["OpenCE"] + CommandLine.arguments.dropFirst()) { [weak self] in
            self?.fail("OpenCE could not start its virtual machine.")
        }
    }

    func applicationWillTerminate(_ notification: Notification) {
        HaloHost.shutDown()
    }

    private func fail(_ message: String) {
        let alert = NSAlert()
        alert.messageText = message
        alert.alertStyle = .critical
        alert.runModal()
        NSApp.terminate(nil)
    }
}

let application = NSApplication.shared
let delegate = AppDelegate()
application.delegate = delegate
application.setActivationPolicy(.regular)
application.run()
