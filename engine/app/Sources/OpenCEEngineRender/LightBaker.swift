// A level's baked light: read from the cache folder when it was baked
// before for the same BSP and settings (the file's name holds the bake's
// key, a hash of what the bake is made from), or traced on the GPU
// (PathTracer) and kept there. A level is baked as it loads (LoadedLevel),
// behind the loading screen, so that it starts lit and its light never
// changes in play.
import Foundation
import Metal
import OpenCEEngineCore

public enum LightBaker {
    /// The cache file of `bake` in `folder` ("": none).
    public static func file(_ bake: LevelBake, folder: String) -> URL? {
        folder.isEmpty ? nil : URL(fileURLWithPath: folder).appendingPathComponent(bake.cacheFileName())
    }

    /// The light kept for `bake` in `folder`, if it is there and whole.
    public static func cached(_ bake: LevelBake, folder: String) -> BakedLight? {
        guard let file = file(bake, folder: folder), let bytes = try? Data(contentsOf: file) else { return nil }
        return bake.read(bytes: bytes)
    }

    /// Keeps `light` in `folder` (nothing without one).
    public static func store(_ light: BakedLight, bake: LevelBake, folder: String) throws {
        guard let file = file(bake, folder: folder) else { return }
        try FileManager.default.createDirectory(at: file.deletingLastPathComponent(), withIntermediateDirectories: true)
        // (written whole, then moved: a bake stopped part way leaves no file)
        try light.encode().write(to: file, options: .atomic)
    }

    /// The light of `bake`, traced by `tracer`; `progress` is told how far
    /// it has come (0 to 1) after each of its three traces.
    public static func bake(_ bake: LevelBake, tracer: PathTracer,
                            progress: ((Float) -> Void)? = nil) throws -> BakedLight {
        let scene = bake.scene()
        let distance = bake.occlusionDistance()
        let lightmapPoints = Array(bake.lightmapPointBytes())
        let probePoints = Array(bake.probePointBytes())
        let reflectionPoints = Array(bake.reflectionPointBytes())
        // (each trace's share of the whole: its points times their samples)
        let work = [Float(lightmapPoints.count) * Float(bake.lightmapSamples()),
                    Float(probePoints.count) * Float(bake.probeSamples()),
                    Float(reflectionPoints.count) * Float(bake.reflectionSamples())]
        let total = max(work.reduce(0, +), 1)
        let (irradiance, directions) = try tracer.run(.irradiance, scene, points: lightmapPoints,
                                                      samples: bake.lightmapSamples(), seed: 1,
                                                      occlusionDistance: distance)
        progress?(work[0] / total)
        // (the probes leave the sun out: the renderer adds it, shadowed)
        let (probes, _) = try tracer.run(.irradiance, scene, points: probePoints,
                                         samples: bake.probeSamples(), seed: 2, occlusionDistance: distance,
                                         sun: scene.sunLight())
        progress?((work[0] + work[1]) / total)
        let (radiance, _) = try tracer.run(.radiance, scene, points: reflectionPoints,
                                           samples: bake.reflectionSamples(), seed: 3, occlusionDistance: 0)
        progress?(1)
        return bake.finish(irradiance: bytes(irradiance), directions: bytes(directions), probes: bytes(probes),
                           radiance: bytes(radiance))
    }

    /// The light of `bake`: the cached file's, or traced and then kept.
    public static func light(_ bake: LevelBake, folder: String, tracer: PathTracer,
                             progress: ((Float) -> Void)? = nil) throws -> BakedLight {
        if let light = cached(bake, folder: folder) {
            progress?(1)
            return light
        }
        let light = try Self.bake(bake, tracer: tracer, progress: progress)
        try store(light, bake: bake, folder: folder)
        return light
    }

    /// Float4s as the engine reads them.
    static func bytes(_ values: [SIMD4<Float>]) -> Data {
        values.withUnsafeBytes { Data($0) }
    }
}
