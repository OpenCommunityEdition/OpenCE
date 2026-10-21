// The app's entry: the engine's window, full screen with --full-screen,
// stepping the world on the display's frames, and drawing the map --map
// PATH names (and --bsp N), or the one chosen from the Map menu or the
// picker, smoothed as --anti-aliasing (temporal, the default, or native)
// and --render-scale (0.5 to 1) say. The free camera flies with W, A, S,
// D, Space and Control (Shift faster), turned by the mouse at
// --mouse-sensitivity radians a count; F3 or --frame-times shows the
// frame times.
import AppKit
import Metal
import OpenCEEngineCore
import OpenCEEngineKit
import OpenCEEngineRender
import OpenCEEngineWindow
import opence_ffiFFI
import simd
import UniformTypeIdentifiers

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

/// The video settings the arguments give.
func videoSettings() -> VideoSettings {
    var settings = VideoSettings()
    if let mode = argument("--anti-aliasing") {
        guard let antiAliasing = AntiAliasing(rawValue: mode) else {
            fail("--anti-aliasing is temporal or native, not \(mode)")
        }
        settings.antiAliasing = antiAliasing
    }
    if let scale = argument("--render-scale") {
        guard let value = Float(scale), VideoSettings.renderScales.contains(value) else {
            fail("--render-scale is 0.5 to 1, not \(scale)")
        }
        settings.renderScale = value
    }
    return settings
}

/// The mouse's turn per count the arguments give.
func flyControls() -> FlyControls {
    var controls = FlyControls()
    if let value = argument("--mouse-sensitivity") {
        guard let sensitivity = Float(value), sensitivity.isFinite, sensitivity > 0, sensitivity < 0.1 else {
            fail("--mouse-sensitivity is radians a count, above 0 and below 0.1, not \(value)")
        }
        controls.sensitivity = sensitivity
    }
    controls.invertPitch = CommandLine.arguments.contains("--invert-mouse")
    return controls
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
    let controls = flyControls()
    let settings = videoSettings()
    var controller: EngineWindowController?
    var level: LoadedLevel?
    var lastFrame: CFTimeInterval?
    var frameTimes = FrameTimes()
    var lastOverlay: CFTimeInterval = 0
    let maps = MapLibrary.maps(in: MapLibrary.folders(
        named: argument("--maps"), root: URL(fileURLWithPath: FileManager.default.currentDirectoryPath)))

    func applicationDidFinishLaunching(_ notification: Notification) {
        NSApp.mainMenu = mainMenu()
        NSApp.mainMenu?.addItem(mapMenu())
        engine.startEmptyWorld()
        let controller = EngineWindowController()
        controller.showsFrameTimes = CommandLine.arguments.contains("--frame-times")
        self.controller = controller
        if let map = argument("--map") {
            do {
                try load(map: map, bsp: argument("--bsp").flatMap { UInt32($0) } ?? 0)
            } catch {
                fail("\(map): \(error)")
            }
        }
        controller.window.engineView.onFrame = { [weak self] link in self?.frame(link) }
        controller.show(fullScreen: CommandLine.arguments.contains("--full-screen"))
        controller.window.engineView.startDisplayLink()
        NSApp.activate()
        if level == nil {
            pick(nil)
        }
    }

    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool { true }

    /// Loads `map` and puts the camera at its start.
    func load(map: String, bsp: UInt32 = 0) throws {
        guard let controller else { return }
        guard let device = controller.window.engineView.device else { fail("no Metal device") }
        let loaded = try LoadedLevel(map: map, bsp: bsp, cacheFolder: cacheFolder(), device: device,
                                     settings: settings)
        // (the scenario's objects, at Normal, standing still)
        _ = try engine.startLevel(geometry: loaded.geometry, difficulty: 1)
        try engine.placeCamera(placement: loaded.start)
        level = loaded
        controller.window.title = "OpenCE Engine — \(URL(fileURLWithPath: map).deletingPathExtension().lastPathComponent)"
    }

    /// The Map menu: the maps of the data folders, and Open Map… for any.
    func mapMenu() -> NSMenuItem {
        let item = NSMenuItem()
        let menu = NSMenu(title: "Map")
        let open = menu.addItem(withTitle: "Open Map…", action: #selector(pick(_:)), keyEquivalent: "o")
        open.target = self
        if !maps.isEmpty {
            menu.addItem(.separator())
        }
        for (index, map) in maps.enumerated() {
            let entry = menu.addItem(withTitle: map.deletingPathExtension().lastPathComponent,
                                     action: #selector(choose(_:)), keyEquivalent: "")
            entry.target = self
            entry.tag = index
        }
        item.submenu = menu
        return item
    }

    @objc func choose(_ sender: NSMenuItem) {
        guard maps.indices.contains(sender.tag) else { return }
        open(maps[sender.tag].path)
    }

    /// The picker: any .map file, starting in the first data folder.
    @objc func pick(_ sender: Any?) {
        let panel = NSOpenPanel()
        panel.title = "Choose a map"
        panel.allowedContentTypes = [UTType(filenameExtension: "map") ?? .data]
        panel.canChooseDirectories = false
        panel.allowsMultipleSelection = false
        panel.directoryURL = maps.first?.deletingLastPathComponent()
        guard let window = controller?.window else { return }
        panel.beginSheetModal(for: window) { [weak self] response in
            guard response == .OK, let url = panel.url else { return }
            MainActor.assumeIsolated { self?.open(url.path) }
        }
    }

    func open(_ map: String) {
        do {
            try load(map: map)
        } catch {
            let alert = NSAlert()
            alert.messageText = "\(URL(fileURLWithPath: map).lastPathComponent) could not be loaded"
            alert.informativeText = "\(error)"
            alert.runModal()
        }
    }

    private func frame(_ link: CADisplayLink) {
        guard let controller else { return }
        let seconds = lastFrame.map { link.timestamp - $0 } ?? 0
        lastFrame = link.timestamp
        frameTimes.add(seconds)
        // (the keys and mouse fly the camera only while the mouse is captured)
        let mouse = controller.takeMouseMotion()
        let held = controller.capture.captured ? controller.keys.codes : []
        engine.setInput(input: controls.input(held: held, mouse: mouse))
        _ = try? engine.advance(frameSeconds: seconds)
        if controller.showsFrameTimes {
            // (four times a second, so that it can be read)
            if link.timestamp - lastOverlay >= 0.25 {
                controller.window.engineView.overlayText = frameTimes.text
                lastOverlay = link.timestamp
            }
        } else {
            controller.window.engineView.overlayText = nil
        }
        guard let renderer = level?.renderer else {
            controller.window.engineView.clear()
            return
        }
        let frame = snapshots.acquire()
        let camera = ViewCamera(frame.camera, alpha: frame.alpha)
        let objects = frame.drawnObjects()
        controller.window.engineView.render { commands, target in
            renderer.encode(into: commands, target: target, camera: camera, objects: objects)
        }
    }
}

let app = NSApplication.shared
let delegate = AppDelegate()
app.delegate = delegate
app.setActivationPolicy(.regular)
app.run()
