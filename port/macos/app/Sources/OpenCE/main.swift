// OpenCE.app: AppKit on the main thread, the game in its VM on threads of
// its own (docs/macos_host.md).

import AppKit
import OpenCEShell
import os

@MainActor
final class AppDelegate: NSObject, NSApplicationDelegate {
    private let log = Logger(subsystem: "org.opence.OpenCE", category: "app")

    func applicationDidFinishLaunching(_ notification: Notification) {
        guard let image = Bundle.main.url(forResource: "halo_guest", withExtension: "elf") else {
            fail("The game's image, halo_guest.elf, is missing from the app.")
            return
        }
        let support = FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask)[0]
        let root = DataRoot.resolve(environment: ProcessInfo.processInfo.environment, applicationSupport: support)
        // (made, so that the player can open it and put the maps in it)
        try? FileManager.default.createDirectory(at: DataRoot.maps(in: root), withIntermediateDirectories: true)
        if let message = DataRoot.missingMapsMessage(root: root) {
            missingMaps(message, folder: DataRoot.maps(in: root))
            return
        }
        // (the host's: the app's own working folder is /)
        setenv("HALO_DATA_ROOT", root.path, 1)
        log.info("starting the guest image \(image.path) with the data in \(root.path)")
        // (the guest's program name, then any arguments the app was given)
        HaloHost.start(image: image, arguments: ["OpenCE"] + CommandLine.arguments.dropFirst()) { [weak self] in
            self?.fail("OpenCE could not start its virtual machine.")
        }
    }

    func applicationWillTerminate(_ notification: Notification) {
        HaloHost.shutDown()
    }

    /// Says where the maps go, offers to show the folder, and quits.
    private func missingMaps(_ message: String, folder: URL) {
        let alert = NSAlert()
        alert.messageText = "OpenCE needs Halo's maps."
        alert.informativeText = message
        alert.addButton(withTitle: "Show the Folder")
        alert.addButton(withTitle: "Quit")
        if alert.runModal() == .alertFirstButtonReturn {
            NSWorkspace.shared.activateFileViewerSelecting([folder])
        }
        NSApp.terminate(nil)
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
