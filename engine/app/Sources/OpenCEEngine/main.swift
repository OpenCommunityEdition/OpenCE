// The app's entry: the engine's window, full screen with --full-screen,
// stepping the world on the display's frames, and drawing the map --map
// PATH names (and --bsp N), or the one chosen from the menus (Campaign,
// which plays a mission at a difficulty with its scripts, Play Alone,
// Join a Game, Join by Address, Host a Game; Game > Main Menu, Command-
// Escape, shows them again) or the Map menu, smoothed as --anti-aliasing
// (temporal, the default, or native) and --render-scale (0.5 to 1) say, or
// as the Video settings do. The player spawns at the scenario's first
// start and moves with W, A, S and D, jumps with Space and crouches with
// Control (or the keys of --bindings FILE, of bindings.json in the app's
// support folder, or of the Controls settings); V changes the camera
// between first and third person, and the mouse looks at
// --mouse-sensitivity radians a count (or settings.toml's). With --fly, or
// on a map without a player start, the free camera flies instead: W, A, S,
// D, Space and Control (Shift faster). F3 or --frame-times shows the frame
// times. --benchmark SECONDS flies the map offscreen at --size (3840x2160
// by default), with no window, and prints the frame times. --headless runs
// the world with no window, answering the control socket (Control.swift);
// --control (or --socket PATH) opens the socket beside the window. ~ (the
// key left of 1) opens the developer shell over the game.
import AppKit
import Metal
import OpenCEEngineCore
import OpenCEEngineKit
import OpenCEEngineMenu
import OpenCEEngineRender
import OpenCEEngineWindow
import opence_ffiFFI
import simd
import SwiftUI
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

/// The HUD's redraws: in the bundle's resources, or (run from the
/// package) the repository's port/assets/hud.
func hudFolder() -> URL? {
    let bundled = Bundle.main.resourceURL?.appendingPathComponent("hud")
    let repository = URL(fileURLWithPath: #filePath).deletingLastPathComponent().deletingLastPathComponent()
        .deletingLastPathComponent().deletingLastPathComponent().deletingLastPathComponent()
        .appendingPathComponent("port/assets/hud")
    return [bundled, repository].compactMap { $0 }.first {
        FileManager.default.fileExists(atPath: $0.appendingPathComponent("layout.json").path)
    }
}

/// The HUD's pass for `device`, or nil (logged) without its redraws.
func hudPass(device: MTLDevice) -> HudPass? {
    do {
        guard let folder = hudFolder(), let queue = device.makeCommandQueue() else {
            FileHandle.standardError.write(Data("OpenCE Engine: no HUD art\n".utf8))
            return nil
        }
        return try HudPass(device: device, art: HudArt.load(folder: folder, device: device, queue: queue),
                           outputFormat: .bgra8Unorm)
    } catch {
        FileHandle.standardError.write(Data("OpenCE Engine: the HUD: \(error)\n".utf8))
        return nil
    }
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

/// The video settings the arguments give, else the saved `options`.
func videoSettings(_ options: VideoOptions = stockSettings().video) -> VideoSettings {
    var settings = VideoSettings()
    settings.antiAliasing = AntiAliasing(rawValue: options.antiAliasing) ?? settings.antiAliasing
    if VideoSettings.renderScales.contains(options.renderScale) {
        settings.renderScale = options.renderScale
    }
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

/// The mouse's turn per count: the argument's, else the saved setting's.
func mouseSensitivity(_ controls: ControlSettings) -> Float {
    func valid(_ value: Float) -> Bool { value.isFinite && value > 0 && value < 0.1 }
    if let value = argument("--mouse-sensitivity") {
        guard let sensitivity = Float(value), valid(sensitivity) else {
            fail("--mouse-sensitivity is radians a count, above 0 and below 0.1, not \(value)")
        }
        return sensitivity
    }
    return valid(controls.mouseSensitivity) ? controls.mouseSensitivity : FlyControls().sensitivity
}

/// Whether the mouse's up looks down: --invert-mouse, else the setting.
func invertsMouse(_ controls: ControlSettings) -> Bool {
    CommandLine.arguments.contains("--invert-mouse") || controls.invertMouse
}

/// The free camera's controls the arguments give, else the settings.
func flyControls(_ controls: ControlSettings) -> FlyControls {
    FlyControls(sensitivity: mouseSensitivity(controls), invertPitch: invertsMouse(controls))
}

/// The player's controls: --bindings FILE's keys, else those of
/// bindings.json in the app's support folder, else the settings' keys.
func playControls(_ controls: ControlSettings) -> PlayControls {
    var bindings = KeyBindings(settings: controls.bindings)
    let support = FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask).first
    let saved = support?.appendingPathComponent("OpenCE Engine/bindings.json")
    if let path = argument("--bindings") ?? saved.flatMap({ FileManager.default.fileExists(atPath: $0.path) ? $0.path : nil }) {
        do {
            bindings = try KeyBindings.read(Data(contentsOf: URL(fileURLWithPath: path)))
        } catch {
            fail("\(path): \(error)")
        }
    }
    return PlayControls(bindings: bindings, sensitivity: mouseSensitivity(controls),
                        invertPitch: invertsMouse(controls))
}

func fail(_ message: String) -> Never {
    FileHandle.standardError.write(Data("OpenCE Engine: \(message)\n".utf8))
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
    let gameItem = NSMenuItem()
    let game = NSMenu(title: "Game")
    let main = game.addItem(withTitle: "Main Menu", action: #selector(AppDelegate.showMenu(_:)), keyEquivalent: "\u{1b}")
    main.keyEquivalentModifierMask = [.command]
    let leave = game.addItem(withTitle: "Leave the Game", action: #selector(AppDelegate.leave(_:)), keyEquivalent: "")
    leave.keyEquivalentModifierMask = []
    gameItem.submenu = game
    menu.addItem(gameItem)
    return menu
}

@MainActor
final class AppDelegate: NSObject, NSApplicationDelegate {
    let engine = Engine()
    lazy var snapshots = SnapshotReader(engine: engine)
    /// The saved settings (settings.toml), which the menus change.
    let saved = SettingsModel()
    lazy var controls = flyControls(saved.settings.controls)
    lazy var play = playControls(saved.settings.controls)
    var earlierKeys: Set<UInt16> = []
    var thirdPerson = false
    lazy var settings = videoSettings(saved.settings.video)
    var controller: EngineWindowController?
    var level: LoadedLevel?
    /// The HUD's pass, made with the first level.
    var hud: HudPass?
    var lastFrame: CFTimeInterval?
    var frameTimes = FrameTimes()
    var lastOverlay: CFTimeInterval = 0
    let folders = MapLibrary.folders(
        named: argument("--maps"), root: URL(fileURLWithPath: FileManager.default.currentDirectoryPath))
    lazy var maps = MapLibrary.maps(in: folders)
    lazy var menu = MenuModel(maps: maps, settings: saved, servers: ServerListModel())
    /// The menus' view, while they show.
    var menuView: NSView?
    /// The network's state last shown in the title.
    var shownStatus: NetworkStatus = .alone
    /// Draws the frames asked for (the shell's `frame`, a window script's,
    /// the control socket's) and acts on the socket's actions.
    var servant: ControlServant?
    /// The developer shell (~), and the pass that draws its text.
    var shell: ShellOverlay?
    var textPass: TextPass?
    /// The shaders' watcher, in a build run from the checkout.
    let shaders = ShaderReloader()
    var lastShaderPoll: CFTimeInterval = 0

    func applicationDidFinishLaunching(_ notification: Notification) {
        NSApp.mainMenu = mainMenu()
        NSApp.mainMenu?.addItem(mapMenu())
        engine.startEmptyWorld()
        MenuFonts.register()
        menu.actions = self
        let controller = EngineWindowController()
        controller.showsFrameTimes = CommandLine.arguments.contains("--frame-times")
            || saved.settings.video.showFrameTimes
        self.controller = controller
        if let folder = folders.first {
            engine.setMapsFolder(folder: folder.path)
        }
        if let map = argument("--map") {
            do {
                try load(map: MapLibrary.path(of: map, folders: folders), bsp: argument("--bsp").flatMap { UInt32($0) } ?? 0)
            } catch {
                fail("\(map): \(error)")
            }
        }
        startShell(view: controller.window.engineView)
        startServant(device: controller.window.engineView.device)
        if CommandLine.arguments.contains("--control") || argument("--socket") != nil {
            startControl()
        }
        controller.window.engineView.onFrame = { [weak self] link in self?.frame(link) }
        controller.show(fullScreen: CommandLine.arguments.contains("--full-screen") || saved.settings.video.fullScreen)
        controller.window.engineView.startDisplayLink()
        NSApp.activate()
        if level == nil {
            showMenu(nil)
        }
    }

    /// Shows the menus over the game (which plays on behind them).
    @objc func showMenu(_ sender: Any?) {
        guard let controller, menuView == nil else { return }
        let view = NSHostingView(rootView: MenuView(model: menu))
        view.frame = controller.window.engineView.bounds
        view.autoresizingMask = [.width, .height]
        controller.window.engineView.addSubview(view)
        controller.capturesMouse = false
        menuView = view
        menu.show(.main)
    }

    /// Takes the menus away, the settings they changed taken up.
    func hideMenu() {
        menu.settings.save()
        menuView?.removeFromSuperview()
        menuView = nil
        controls = flyControls(saved.settings.controls)
        play = playControls(saved.settings.controls)
        settings = videoSettings(saved.settings.video)
        controller?.showsFrameTimes = CommandLine.arguments.contains("--frame-times")
            || saved.settings.video.showFrameTimes
        controller?.capturesMouse = true
    }

    /// Makes the servant that draws the frames asked for here.
    func startServant(device: MTLDevice?) {
        guard let device, let queue = device.makeCommandQueue() else { fail("no Metal device") }
        servant = ControlServant(
            engine: engine, device: device, queue: queue, snapshots: snapshots,
            loadMap: { [weak self] path, bsp in try self?.commandLoad(map: path, bsp: bsp) },
            reloadBsp: { [weak self] bsp in try self?.reloadStructure(bsp) },
            renderer: { [weak self] in self?.level?.renderer })
    }

    /// Opens the control socket (with --control or --socket).
    func startControl() {
        do {
            appLog("info", "control socket: \(try engine.startControl(path: argument("--socket")))")
        } catch {
            fail("the control socket: \(error)")
        }
    }

    /// A command's `map`: leaves the game and loads it, the menus put away.
    func commandLoad(map: String, bsp: UInt32) throws {
        engine.leaveGame()
        try load(map: map, bsp: bsp)
        if menuView != nil {
            hideMenu()
        }
    }

    /// A command's `bsp`: draws another structure of the level loaded.
    func reloadStructure(_ bsp: UInt32) throws {
        guard let level, let device = controller?.window.engineView.device else { return }
        let switched = try LoadedLevel(map: level.map, bsp: bsp, cacheFolder: cacheFolder(), device: device,
                                       settings: settings)
        switched.renderer.hud = hud
        self.level = switched
    }

    /// The developer shell: ~ opens it over the game, which keeps playing;
    /// while open it has the keys and the pointer, not the player.
    func startShell(view: EngineView) {
        guard let device = view.device, let font = GlyphAtlas.url() else {
            appLog("warn", "the shell has no font")
            return
        }
        do {
            let atlas = try GlyphAtlas(url: font, scale: Double(view.window?.backingScaleFactor ?? 2), device: device)
            textPass = try TextPass(device: device, atlas: atlas, outputFormat: .bgra8Unorm)
        } catch {
            appLog("error", "the shell's text: \(error)")
            return
        }
        let shell = ShellOverlay(engine: engine, pasteboard: SystemPasteboard(), historyFile: ShellHistory.file)
        shell.onOutput = { [weak self] output in
            guard let self else { return }
            if !performAction(output.action, loadMap: commandLoad, reloadBsp: reloadStructure) {
                NSApp.terminate(nil)
            }
        }
        shell.onOpenChanged = { [weak self] open in
            guard let self, let controller else { return }
            if open {
                controller.releaseKeys()
                controller.capturesMouse = false
            } else if menuView == nil {
                controller.capturesMouse = true
            }
        }
        view.keyFilter = { [weak shell] event in
            guard let shell else { return false }
            if event.type == .keyUp || (event.isARepeat && !shell.isOpen) {
                return shell.isOpen
            }
            guard let key = ShellKey(event) else { return shell.isOpen }
            return shell.handle(key) || shell.isOpen
        }
        view.pointerFilter = { [weak shell] phase, x, y in
            guard let shell else { return false }
            switch phase {
            case .down: return shell.pointer(.down, x: x, y: y)
            case .dragged: return shell.pointer(.dragged, x: x, y: y)
            case .up: return shell.pointer(.up, x: x, y: y)
            }
        }
        view.scrollFilter = { [weak self, weak shell] delta, precise in
            guard let self, let shell, let textPass else { return false }
            // (a wheel's lines as three of the shell's rows)
            let cell = Double(ShellMetrics(atlas: textPass.atlas).cellHeight)
            return shell.scrollWheel(pixels: precise ? delta : delta * 3 * cell)
        }
        self.shell = shell
    }

    /// What the shell draws over a frame `width` by `height` pixels.
    func shellItems(width: Int, height: Int) -> [TextItem] {
        guard let shell, shell.isVisible, let textPass else { return [] }
        return shell.items(width: width, height: height, metrics: ShellMetrics(atlas: textPass.atlas))
    }

    @objc func leave(_ sender: Any?) {
        engine.leaveGame()
    }

    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool { true }

    /// Loads `map` and plays its mission at `difficulty`: its scripts run,
    /// its cinematics hold the camera.
    func loadCampaign(map: String, difficulty: UInt8) throws {
        guard let controller else { return }
        guard let device = controller.window.engineView.device else { fail("no Metal device") }
        let loaded = try LoadedLevel(map: map, bsp: 0, cacheFolder: cacheFolder(), device: device,
                                     settings: settings)
        if hud == nil {
            hud = hudPass(device: device)
        }
        loaded.renderer.hud = hud
        try engine.startCampaign(geometry: loaded.geometry, difficultyLevel: difficulty)
        thirdPerson = false
        level = loaded
        followStructure()
        controller.window.title = "OpenCE Engine — \(URL(fileURLWithPath: map).deletingPathExtension().lastPathComponent)"
    }

    /// Draws the structure the mission plays in, once its scripts switch
    /// to another (Halo pauses for it too).
    func followStructure() {
        guard let state = engine.campaignState(), let level, UInt32(state.bsp) != level.bsp,
              let device = controller?.window.engineView.device else { return }
        do {
            let switched = try LoadedLevel(map: level.map, bsp: UInt32(state.bsp), cacheFolder: cacheFolder(),
                                           device: device, settings: settings)
            switched.renderer.hud = hud
            self.level = switched
        } catch {
            fail("\(level.map) structure \(state.bsp): \(error)")
        }
    }

    /// Loads `map` and puts the camera at its start.
    func load(map: String, bsp: UInt32 = 0) throws {
        guard let controller else { return }
        guard let device = controller.window.engineView.device else { fail("no Metal device") }
        let loaded = try LoadedLevel(map: map, bsp: bsp, cacheFolder: cacheFolder(), device: device,
                                     settings: settings)
        if hud == nil {
            hud = hudPass(device: device)
        }
        loaded.renderer.hud = hud
        // (the scenario's objects, at Normal, standing still)
        _ = try engine.startLevel(geometry: loaded.geometry, difficulty: 1)
        try engine.placeCamera(placement: loaded.start)
        // (the player at the first start, unless flying; a map without one
        // keeps the free camera)
        if !CommandLine.arguments.contains("--fly") {
            try? engine.spawnPlayer(start: 0)
            thirdPerson = false
        }
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
            engine.leaveGame()
            try load(map: map)
            if menuView != nil {
                hideMenu()
            }
        } catch {
            let alert = NSAlert()
            alert.messageText = "\(URL(fileURLWithPath: map).lastPathComponent) could not be loaded"
            alert.informativeText = "\(error)"
            alert.runModal()
        }
    }

    /// Compiles the shaders edited in the checkout again, once a second;
    /// the shell shows what was reloaded and why a shader was not.
    private func reloadShaders(now: CFTimeInterval) {
        guard let shaders, now - lastShaderPoll >= 1 else { return }
        lastShaderPoll = now
        for reload in shaders.poll(level: level?.renderer, hud: hud, text: textPass) {
            if let error = reload.error {
                engineLog(level: "error", target: "shaders", text: "\(reload.shader.file) kept its old pipelines: \(error)")
            } else {
                engineLog(level: "info", target: "shaders", text: "\(reload.shader.file) reloaded")
            }
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
        if engine.playing() {
            if play.togglesView(held: held, earlier: earlierKeys) {
                thirdPerson.toggle()
                _ = engine.setThirdPerson(thirdPerson: thirdPerson)
            }
            engine.setInput(input: play.input(held: held, mouse: mouse))
        } else {
            engine.setInput(input: controls.input(held: held, mouse: mouse))
        }
        earlierKeys = held
        if let servant, !servant.serve() {
            NSApp.terminate(nil)
            return
        }
        _ = try? engine.advance(frameSeconds: seconds)
        followStructure()
        showStatus()
        profiled("shell", engine) { shell?.update(seconds: seconds, now: link.timestamp) }
        reloadShaders(now: link.timestamp)
        if controller.showsFrameTimes {
            // (four times a second, so that it can be read)
            if link.timestamp - lastOverlay >= 0.25 {
                controller.window.engineView.overlayText = frameTimes.text
                lastOverlay = link.timestamp
            }
        } else {
            controller.window.engineView.overlayText = nil
        }
        let (width, height) = controller.window.engineView.sizeInPixels
        let text = shellItems(width: width, height: height)
        guard let renderer = level?.renderer else {
            guard let textPass, !text.isEmpty else {
                controller.window.engineView.clear()
                return
            }
            controller.window.engineView.render { commands, target in
                let pass = MTLRenderPassDescriptor()
                pass.colorAttachments[0].texture = target
                pass.colorAttachments[0].loadAction = .clear
                pass.colorAttachments[0].storeAction = .store
                pass.colorAttachments[0].clearColor = MTLClearColor(red: 0, green: 0, blue: 0, alpha: 1)
                commands.makeRenderCommandEncoder(descriptor: pass)?.endEncoding()
                textPass.encode(text, into: commands, target: target)
            }
            return
        }
        let frame = profiled("snapshot", engine) { snapshots.acquire() }
        let camera = ViewCamera(frame.camera, alpha: frame.alpha)
        let objects = profiled("objects", engine) { frame.drawnObjects() }
        let hudQuads = profiled("hud", engine) { engine.hud(width: UInt32(width), height: UInt32(height)) }
        let debugLines = frame.debugSegments()
        // (the labels under the shell)
        var labels: [TextItem] = []
        if let textPass, frame.debugLabels.count > 0 {
            labels = DebugPass.labels(frame.debugMarks(),
                                      viewProjection: camera.viewProjection(aspect: Float(width) / Float(max(height, 1))),
                                      width: width, height: height, cellWidth: textPass.atlas.cellWidth,
                                      cellHeight: textPass.atlas.cellHeight)
        }
        controller.window.engineView.render { commands, target in
            profiled("encode", self.engine) {
                renderer.encode(into: commands, target: target, camera: camera, objects: objects,
                                hudQuads: hudQuads, debugLines: debugLines)
            }
            profiled("text", self.engine) { self.textPass?.encode(labels + text, into: commands, target: target) }
        }
    }
}

/// The shell's copying and pasting, with the system's clipboard.
final class SystemPasteboard: ShellPasteboard {
    func copy(_ text: String) {
        NSPasteboard.general.clearContents()
        NSPasteboard.general.setString(text, forType: .string)
    }

    func paste() -> String? {
        NSPasteboard.general.string(forType: .string)
    }
}

extension ShellKey {
    /// The shell's key for an AppKit key press.
    init?(_ event: NSEvent) {
        let flags = event.modifierFlags
        var modifiers: Modifiers = []
        if flags.contains(.shift) { modifiers.insert(.shift) }
        if flags.contains(.control) { modifiers.insert(.control) }
        if flags.contains(.option) { modifiers.insert(.option) }
        if flags.contains(.command) { modifiers.insert(.command) }
        self.init(code: event.keyCode, characters: event.characters ?? "",
                  ignoringModifiers: event.charactersIgnoringModifiers ?? "", modifiers: modifiers)
    }
}

extension AppDelegate: MenuActions {
    func playCampaign(map: URL, difficulty: UInt8) throws {
        engine.leaveGame()
        try loadCampaign(map: map.path, difficulty: difficulty)
        hideMenu()
    }

    func playAlone(map: URL) throws {
        engine.leaveGame()
        try load(map: map.path)
        hideMenu()
    }

    func host(_ setup: HostSetup, map: URL, port: UInt16) throws {
        engine.leaveGame()
        hideMenu()
        try load(map: map.path)
        guard let level else { return }
        _ = try engine.hostGame(geometry: level.geometry, kind: setup.kind, port: port, name: setup.name,
                                maxPlayers: UInt8(setup.maxPlayers))
    }

    func join(address: String, map: URL, name: String) throws {
        engine.leaveGame()
        hideMenu()
        try load(map: map.path)
        guard let level else { return }
        try engine.joinGame(geometry: level.geometry, address: address, name: name)
    }

    func quit() {
        NSApp.terminate(nil)
    }

    /// The network's state in the title; a refusal brings the menus back
    /// with its reason.
    func showStatus() {
        let status = engine.networkStatus()
        guard status != shownStatus, let controller else { return }
        shownStatus = status
        let map = level.map { " — " + URL(fileURLWithPath: $0.map).deletingPathExtension().lastPathComponent } ?? ""
        let state: String
        switch status {
        case .alone: state = ""
        case let .hosting(port, players): state = " — hosting on port \(port), \(players) playing"
        case .waiting: state = " — joining…"
        case let .playing(player): state = " — playing as player \(player + 1)"
        case let .refused(reason):
            state = ""
            engine.leaveGame()
            showMenu(nil)
            menu.message = "The host refused: \(reason)."
        }
        controller.window.title = "OpenCE Engine\(map)\(state)"
    }
}

/// The benchmark (tools/test_engine_benchmark.py): --map drawn offscreen
/// along the benchmark's path, its frame times printed as JSON.
func benchmark(seconds: String) -> Never {
    guard let time = Double(seconds), time.isFinite, time > 0 else { fail("--benchmark is seconds, not \(seconds)") }
    guard let map = argument("--map") else { fail("--benchmark needs --map") }
    let size = (argument("--size") ?? "3840x2160").split(separator: "x").compactMap { Int($0) }
    guard size.count == 2, size.allSatisfy({ (1...16384).contains($0) }) else { fail("--size is WIDTHxHEIGHT") }
    guard let device = MTLCreateSystemDefaultDevice(), let queue = device.makeCommandQueue() else {
        fail("no Metal device")
    }
    do {
        let level = try LoadedLevel(map: map, bsp: argument("--bsp").flatMap { UInt32($0) } ?? 0,
                                    cacheFolder: argument("--cache") ?? cacheFolder(), device: device,
                                    settings: videoSettings())
        let engine = Engine()
        let snapshots = SnapshotReader(engine: engine)
        _ = try engine.startLevel(geometry: level.geometry, difficulty: 1)
        _ = try engine.step()
        let objects = snapshots.acquire().drawnObjects()
        let start = level.start
        let path = BenchmarkPath(centre: SIMD3(start.x, start.y, start.z))
        let result = try level.renderer.benchmark(width: size[0], height: size[1], path: path, seconds: time,
                                                  objects: objects, queue: queue)
        print(result.json)
        exit(0)
    } catch {
        fail("\(map): \(error)")
    }
}

if let seconds = argument("--benchmark") {
    benchmark(seconds: seconds)
}

if CommandLine.arguments.contains("--headless") {
    headless()
}

let app = NSApplication.shared
let delegate = AppDelegate()
app.delegate = delegate
app.setActivationPolicy(.regular)
app.run()
