// The control socket's work in the app: the maps and structures the shell
// asks to load, the frames it asks to be drawn, and --headless, which runs
// the world with no window, its frames drawn offscreen when asked
// (tools/test_engine_headless.py and the opence tool drive it).
import Foundation
import Metal
import OpenCEEngineCore
import OpenCEEngineKit
import OpenCEEngineRender
import opence_ffiFFI

/// Writes a line to the engine's log (the shell's `log`, `subscribe log`),
/// and to standard error.
func appLog(_ level: String, _ text: String) {
    engineLog(level: level, target: "swift", text: text)
    FileHandle.standardError.write(Data("OpenCE Engine: \(text)\n".utf8))
}

/// Does a command's `action` (the socket's or the shell's) with the app's
/// loading; false for quit.
func performAction(_ action: ShellAction, loadMap: (String, UInt32) throws -> Void,
                   reloadBsp: (UInt32) throws -> Void) -> Bool {
    switch action {
    case .none:
        break
    case let .loadMap(path, bsp):
        do {
            try loadMap(path, bsp)
            appLog("info", "loaded \(path) at structure \(bsp)")
        } catch {
            appLog("error", "\(path): \(error)")
        }
    case let .reloadBsp(bsp):
        do {
            try reloadBsp(bsp)
        } catch {
            appLog("error", "structure \(bsp): \(error)")
        }
    case .quit:
        return false
    }
    return true
}

/// Answers the control socket for the app: loads what the shell's `map`
/// and `bsp` ask for, and draws the frames asked for.
final class ControlServant {
    let engine: Engine
    let device: MTLDevice
    let queue: MTLCommandQueue
    let snapshots: SnapshotReader
    /// Loads a map at a structure (the app's own loading, which starts
    /// the level and places the player).
    var loadMap: (String, UInt32) throws -> Void
    /// Draws a structure of the level already loaded.
    var reloadBsp: (UInt32) throws -> Void
    /// The level drawn, if any.
    var renderer: () -> LevelRenderer?
    /// The developer state, read again only when the shell's variables change.
    private let dev = DevStateReader()

    init(engine: Engine, device: MTLDevice, queue: MTLCommandQueue, snapshots: SnapshotReader,
         loadMap: @escaping (String, UInt32) throws -> Void, reloadBsp: @escaping (UInt32) throws -> Void,
         renderer: @escaping () -> LevelRenderer?) {
        self.engine = engine
        self.device = device
        self.queue = queue
        self.snapshots = snapshots
        self.loadMap = loadMap
        self.reloadBsp = reloadBsp
        self.renderer = renderer
    }

    /// Does what the socket asked since the last call; false once it asked
    /// to quit.
    func serve() -> Bool {
        for action in engine.takeControlActions() where !performAction(action, loadMap: loadMap, reloadBsp: reloadBsp) {
            return false
        }
        for request in engine.controlFrameRequests() {
            engine.finishFrame(id: request.id, error: draw(request))
        }
        return true
    }

    /// Draws `request`'s frame from the world's camera into its PNG; the
    /// error, or nil.
    private func draw(_ request: FrameRequest) -> String? {
        guard let renderer = renderer() else { return "no level is loaded" }
        let frame = snapshots.acquire()
        let camera = ViewCamera(frame.camera, alpha: frame.alpha)
        // (the game's time, at its 60 ticks a second, for the shaders' animations)
        renderer.time = Float((Double(frame.tick) + Double(frame.alpha)) / 60)
        renderer.lights = frame.drawnLights()
        renderer.laidDecals = Array(frame.decals)
        renderer.laidParticles = Array(frame.particles)
        let devState = dev.update(generation: engine.devGeneration(), flag: { engine.shellFlag(name: $0) },
                                  value: { engine.shellValue(name: $0) })
        renderer.switches = devState.switches
        renderer.tuning = devState.tuning
        renderer.sprites = engine.cameraSprites(camera: camera.spriteCamera, flares: renderer.switches.lensFlares,
                                                widgets: renderer.switches.glows)
        let width = Int(request.width), height = Int(request.height)
        do {
            let captured = try renderer.capture(width: width, height: height, camera: camera,
                                                objects: frame.drawnObjects(),
                                                hudQuads: engine.hud(width: request.width, height: request.height),
                                                debugLines: frame.debugSegments(), queue: queue)
            try captured.writePNG(to: URL(fileURLWithPath: request.path))
            return nil
        } catch {
            return "\(error)"
        }
    }
}

/// --headless: the world run with no window at --tick-rate frames a
/// second (60 by default; 0 runs as fast as it can), the control socket at
/// --socket PATH (else the default), and --map PATH loaded first. The
/// socket's path is printed once it listens.
func headless() -> Never {
    guard let device = MTLCreateSystemDefaultDevice(), let queue = device.makeCommandQueue() else {
        fail("no Metal device")
    }
    let rate = argument("--tick-rate").map { Double($0) } ?? 60
    guard let rate, rate.isFinite, rate >= 0, rate <= 1000 else { fail("--tick-rate is 0 to 1000 frames a second") }
    let engine = Engine()
    let snapshots = SnapshotReader(engine: engine)
    let settings = videoSettings()
    let cache = argument("--cache") ?? cacheFolder()
    let hud = hudPass(device: device)
    var level: LoadedLevel?
    engine.startEmptyWorld()
    if let maps = argument("--maps") {
        engine.setMapsFolder(folder: maps)
    }
    let servant = ControlServant(
        engine: engine, device: device, queue: queue, snapshots: snapshots,
        loadMap: { path, bsp in
            let loaded = try LoadedLevel(map: path, bsp: bsp, cacheFolder: cache, device: device, settings: settings)
            if let failure = loaded.bakeFailure {
                appLog("warn", "the bake of \(path): \(failure); drawn with its lightmaps")
            }
            loaded.renderer.hud = hud
            engine.leaveGame()
            _ = try engine.startLevel(geometry: loaded.geometry, difficulty: 1)
            try engine.placeCamera(placement: loaded.start)
            if !launchArguments.contains("--fly") {
                try? engine.spawnPlayer(start: 0)
            }
            level = loaded
        },
        reloadBsp: { bsp in
            guard let current = level else { return }
            let loaded = try LoadedLevel(map: current.map, bsp: bsp, cacheFolder: cache, device: device,
                                         settings: settings)
            loaded.renderer.hud = hud
            level = loaded
        },
        renderer: { level?.renderer })
    if let map = argument("--map") {
        do {
            let folders = MapLibrary.folders(named: argument("--maps"),
                                             root: MapLibrary.dataRoot)
            try servant.loadMap(MapLibrary.path(of: map, folders: folders), argument("--bsp").flatMap { UInt32($0) } ?? 0)
        } catch {
            fail("\(map): \(error)")
        }
    }
    let socket: String
    do {
        socket = try engine.startControl(path: argument("--socket"))
    } catch {
        fail("the control socket: \(error)")
    }
    print("control socket: \(socket)")
    fflush(stdout)
    appLog("info", "headless at \(rate == 0 ? "full speed" : "\(rate) frames a second")")
    // (unthrottled, each turn still counts as a sixtieth of a second)
    let interval = rate == 0 ? 1.0 / 60 : 1 / rate
    var last = Date()
    while true {
        let running = autoreleasepool { () -> Bool in
            guard servant.serve() else { return false }
            let now = Date()
            _ = try? engine.advance(frameSeconds: rate == 0 ? interval : now.timeIntervalSince(last))
            last = now
            return true
        }
        guard running else { break }
        if rate > 0 {
            let wait = interval - Date().timeIntervalSince(last)
            if wait > 0 {
                Thread.sleep(forTimeInterval: wait)
            }
        }
    }
    engine.stopControl()
    exit(0)
}
