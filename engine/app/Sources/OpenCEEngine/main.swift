// The app's entry: the engine's window, full screen with --full-screen,
// stepping an empty world on the display's frames. Later tasks draw the
// level in it and fly the camera.
import AppKit
import OpenCEEngineCore
import OpenCEEngineKit
import OpenCEEngineWindow

if CommandLine.arguments.contains("--version") {
    print("OpenCE Engine \(engineVersion())")
    exit(0)
}

do {
    try checkSnapshotLayout()
} catch {
    FileHandle.standardError.write(Data("OpenCE Engine: \(error)\n".utf8))
    exit(1)
}

/// The app's menu: quit, and full screen (Escape lets the mouse go first).
@MainActor
func mainMenu() -> NSMenu {
    let menu = NSMenu()
    let appItem = NSMenuItem()
    appItem.submenu = NSMenu()
    appItem.submenu?.addItem(withTitle: "Quit OpenCE Engine", action: #selector(NSApplication.terminate(_:)),
                             keyEquivalent: "q")
    menu.addItem(appItem)
    let viewItem = NSMenuItem()
    let view = NSMenu(title: "View")
    let fullScreen = view.addItem(withTitle: "Enter Full Screen", action: #selector(NSWindow.toggleFullScreen(_:)),
                                  keyEquivalent: "f")
    fullScreen.keyEquivalentModifierMask = [.command, .control]
    viewItem.submenu = view
    menu.addItem(viewItem)
    return menu
}

@MainActor
final class AppDelegate: NSObject, NSApplicationDelegate {
    let engine = Engine()
    var controller: EngineWindowController?
    var lastFrame: CFTimeInterval?

    func applicationDidFinishLaunching(_ notification: Notification) {
        NSApp.mainMenu = mainMenu()
        engine.startEmptyWorld()
        let controller = EngineWindowController()
        controller.window.engineView.onFrame = { [weak self] link in self?.frame(link) }
        controller.show(fullScreen: CommandLine.arguments.contains("--full-screen"))
        controller.window.engineView.startDisplayLink()
        self.controller = controller
        NSApp.activate()
    }

    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool { true }

    private func frame(_ link: CADisplayLink) {
        guard let controller else { return }
        let seconds = lastFrame.map { link.timestamp - $0 } ?? 0
        lastFrame = link.timestamp
        _ = controller.takeMouseMotion()
        _ = try? engine.advance(frameSeconds: seconds)
        controller.window.engineView.clear()
    }
}

let app = NSApplication.shared
let delegate = AppDelegate()
app.delegate = delegate
app.setActivationPolicy(.regular)
app.run()
