// A map loaded for the app: imported (or read from the cache), its BSP's
// buffers and textures made, its light baked (or read from the cache),
// and its start view, whether the map came from the command line or the
// picker. The bake is part of the load, behind the loading screen, so that
// a level starts lit and its light never changes in play; the first load
// of a map bakes every structure of it, so that a mission's switch to
// another structure finds its light kept.
import Metal
import OpenCEEngineCore

/// Where a loaded level's light came from.
public enum LevelLight: Sendable, Equatable {
    /// The map's own lightmaps (its bake failed, or was not asked for).
    case stock
    /// The cache's bake.
    case cached
    /// Baked as it loaded.
    case traced
}

/// How a load has its level's light.
public enum BakeAtLoad: Sendable {
    /// The cache's, or baked now at the bake's settings.
    case full
    /// As full, at few samples (tests).
    case quick
    /// The cache's only, else the stock lightmaps (tests that do not look
    /// at the light).
    case cachedOnly
}

public struct LoadedLevel {
    public let map: String
    /// The structure drawn.
    public let bsp: UInt32
    public let geometry: LevelGeometry
    public let renderer: LevelRenderer
    /// Where the camera starts: the scenario's first player start.
    public let start: CameraPlacement
    /// The bake of the structure drawn (nil when it cannot be baked).
    public let bake: LevelBake?
    /// Where its light came from.
    public let light: LevelLight
    /// Why it could not be baked, when it could not.
    public let bakeFailure: String?

    public init(map: String, bsp: UInt32 = 0, cacheFolder: String, device: MTLDevice,
                settings: VideoSettings, bakeAtLoad: BakeAtLoad = .full,
                prepare size: (width: Int, height: Int)? = nil, progress: ((Float) -> Void)? = nil) throws {
        progress?(0)
        let geometry = try LevelGeometry.load(map: map, cacheFolder: cacheFolder, bsp: bsp)
        try self.init(geometry: geometry, map: map, bsp: bsp, cacheFolder: cacheFolder, device: device,
                      settings: settings, bakeAtLoad: bakeAtLoad, prepare: size, progress: progress)
    }

    /// The load from `geometry`, already imported, of `map`'s structure `bsp`.
    public init(geometry: LevelGeometry, map: String, bsp: UInt32 = 0, cacheFolder: String, device: MTLDevice,
                settings: VideoSettings, bakeAtLoad: BakeAtLoad = .full,
                prepare size: (width: Int, height: Int)? = nil, progress: ((Float) -> Void)? = nil) throws {
        self.map = map
        self.bsp = bsp
        self.geometry = geometry
        progress?(Self.importedProgress)
        renderer = try LevelRenderer(device: device, geometry: geometry, settings: settings)
        progress?(Self.drawnProgress)
        start = geometry.startView()
        let quick = bakeAtLoad == .quick
        bake = try? geometry.bakePlan(quick: quick)
        var light = LevelLight.stock
        var failure: String?
        if let bake {
            if let kept = LightBaker.cached(bake, folder: cacheFolder) {
                try renderer.useBakedLighting(kept)
                light = .cached
            } else if bakeAtLoad != .cachedOnly {
                let span = Self.bakedProgress - Self.drawnProgress
                do {
                    let tracer = try PathTracer(device: device)
                    // (the other structures first, kept for later: their
                    // share of the progress is the drawn one's)
                    let others = geometry.bakePlans(quick: quick).filter { $0.bsp() != bsp }
                    let parts = Float(others.count + 1)
                    for (index, other) in others.enumerated() {
                        _ = try LightBaker.light(other, folder: cacheFolder, tracer: tracer) { done in
                            progress?(Self.drawnProgress + span * (Float(index) + done) / parts)
                        }
                    }
                    let traced = try LightBaker.light(bake, folder: cacheFolder, tracer: tracer) { done in
                        progress?(Self.drawnProgress + span * (parts - 1 + done) / parts)
                    }
                    try renderer.useBakedLighting(traced)
                    light = .traced
                } catch {
                    failure = "\(error)"
                }
            }
        }
        self.light = light
        bakeFailure = failure
        progress?(Self.bakedProgress)
        if let size {
            try Self.prepare(renderer, width: size.width, height: size.height, from: start)
        }
        progress?(1)
    }

    /// How far a load has come once the map is imported (the longest part
    /// the first time, with the bake), once its buffers and textures are
    /// made, and once its light is baked.
    public static let importedProgress: Float = 0.4
    public static let drawnProgress: Float = 0.5
    public static let bakedProgress: Float = 0.95

    /// Draws a frame from the start view at the size the level will be
    /// shown at, offscreen, and waits for it: the size's targets and its
    /// temporal scaler are made, and the GPU has taken the level's
    /// buffers and textures, before its first shown frame (a level loaded
    /// behind the loading screen is prepared on the loader's thread, so
    /// that its first frame is no longer than the rest).
    static func prepare(_ renderer: LevelRenderer, width: Int, height: Int, from start: CameraPlacement) throws {
        guard width > 0, height > 0 else { return }
        let descriptor = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: renderer.outputFormat, width: width,
                                                                  height: height, mipmapped: false)
        descriptor.usage = [.renderTarget]
        descriptor.storageMode = .private
        guard let queue = renderer.device.makeCommandQueue(),
              let target = renderer.device.makeTexture(descriptor: descriptor),
              let commands = queue.makeCommandBuffer()
        else {
            throw LevelRendererError.resources("the first frame's target")
        }
        let camera = ViewCamera(position: [start.x, start.y, start.z], yaw: start.yaw, pitch: start.pitch,
                                verticalFov: 70 * .pi / 180)
        renderer.encode(into: commands, target: target, camera: camera)
        commands.commit()
        commands.waitUntilCompleted()
    }
}
