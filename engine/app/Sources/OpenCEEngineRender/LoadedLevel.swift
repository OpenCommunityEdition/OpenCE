// A map loaded for the app: imported (or read from the cache), its BSP's
// buffers and textures made, and its start view, whether the map came
// from the command line or the picker; drawn with its baked light when
// the cache folder holds it (else with the stock lightmaps, until the
// app's background bake is done).
import Metal
import OpenCEEngineCore

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

    public init(map: String, bsp: UInt32 = 0, cacheFolder: String, device: MTLDevice,
                settings: VideoSettings, progress: ((Float) -> Void)? = nil) throws {
        self.map = map
        self.bsp = bsp
        progress?(0)
        geometry = try LevelGeometry.load(map: map, cacheFolder: cacheFolder, bsp: bsp)
        progress?(Self.importedProgress)
        renderer = try LevelRenderer(device: device, geometry: geometry, settings: settings)
        progress?(Self.drawnProgress)
        start = geometry.startView()
        bake = try? geometry.bakePlan(quick: false)
        if let bake, let light = LightBaker.cached(bake, folder: cacheFolder) {
            try renderer.useBakedLighting(light)
        }
        progress?(1)
    }

    /// How far a load has come once the map is imported (the longest part
    /// the first time), and once its buffers and textures are made.
    public static let importedProgress: Float = 0.5
    public static let drawnProgress: Float = 0.9
}
