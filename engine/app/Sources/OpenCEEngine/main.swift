// The app's entry: the engine's window, full screen with --full-screen,
// stepping an empty world on the display's frames, and with --map PATH
// (and --bsp N) drawing that map's structure from the camera.
import AppKit
import Metal
import OpenCEEngineCore
import OpenCEEngineKit
import OpenCEEngineRender
import OpenCEEngineWindow
import opence_ffiFFI
import simd

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

/// The value after `name` among the arguments, or nil.
func argument(_ name: String) -> String? {
    let arguments = CommandLine.arguments
    guard let index = arguments.firstIndex(of: name), index + 1 < arguments.count else { return nil }
    return arguments[index + 1]
}

/// Where imported maps are kept between runs.
func cacheFolder() -> String {
    let caches = FileManager.default.urls(for: .cachesDirectory, in: .userDomainMask).first
        ?? URL(fileURLWithPath: NSTemporaryDirectory())
    let folder = caches.appendingPathComponent("OpenCE Engine", isDirectory: true)
    try? FileManager.default.createDirectory(at: folder, withIntermediateDirectories: true)
    return folder.path
}

func fail(_ message: String) -> Never {
    FileHandle.standardError.write(Data("OpenCE Engine: \(message)\n".utf8))
    exit(1)
}

extension ViewCamera {
    /// The camera between a snapshot's last two ticks.
    init(_ snapshot: OCECameraSnapshot, alpha: Float) {
        func camera(_ transform: OCETransform) -> ViewCamera {
            let position = transform.position
            let rotation = transform.rotation
            return ViewCamera(position: SIMD3(position.x, position.y, position.z),
                              rotation: simd_quatf(ix: rotation.x, iy: rotation.y, iz: rotation.z, r: rotation.w),
                              verticalFov: snapshot.vertical_fov_radians)
        }
        self = .interpolated(from: camera(snapshot.previous), to: camera(snapshot.current), alpha: alpha)
    }
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
    lazy var snapshots = SnapshotReader(engine: engine)
    var controller: EngineWindowController?
    var renderer: LevelRenderer?
    var lastFrame: CFTimeInterval?

    func applicationDidFinishLaunching(_ notification: Notification) {
        NSApp.mainMenu = mainMenu()
        engine.startEmptyWorld()
        let controller = EngineWindowController()
        if let map = argument("--map") {
            let bsp = argument("--bsp").flatMap { UInt32($0) } ?? 0
            guard let device = controller.window.engineView.device else { fail("no Metal device") }
            do {
                let geometry = try LevelGeometry.load(map: map, cacheFolder: cacheFolder(), bsp: bsp)
                renderer = try LevelRenderer(device: device, geometry: geometry)
                try engine.placeCamera(placement: geometry.startView())
            } catch {
                fail("\(map): \(error)")
            }
        }
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
        guard let renderer else {
            controller.window.engineView.clear()
            return
        }
        let frame = snapshots.acquire()
        let camera = ViewCamera(frame.camera, alpha: frame.alpha)
        controller.window.engineView.render { commands, target in
            renderer.encode(into: commands, target: target, camera: camera)
        }
    }
}

let app = NSApplication.shared
let delegate = AppDelegate()
app.delegate = delegate
app.setActivationPolicy(.regular)
app.run()
