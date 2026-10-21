// A map loaded for the app: imported (or read from the cache), its BSP's
// buffers and textures made, and its start view, whether the map came
// from the command line or the picker.
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

    public init(map: String, bsp: UInt32 = 0, cacheFolder: String, device: MTLDevice,
                settings: VideoSettings) throws {
        self.map = map
        self.bsp = bsp
        geometry = try LevelGeometry.load(map: map, cacheFolder: cacheFolder, bsp: bsp)
        renderer = try LevelRenderer(device: device, geometry: geometry, settings: settings)
        start = geometry.startView()
    }
}
