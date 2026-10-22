// A level's baked light: read from the cache folder when it was baked
// before for the same BSP and settings (the file's name holds the bake's
// key), or traced on the GPU (PathTracer) and kept there. The bake takes
// a while on a large map, so the app runs it in the background and draws
// the stock lightmaps until it is done.
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

    /// The light of `bake`, traced by `tracer`.
    public static func bake(_ bake: LevelBake, tracer: PathTracer) throws -> BakedLight {
        let scene = bake.scene()
        let distance = bake.occlusionDistance()
        let (irradiance, directions) = try tracer.run(.irradiance, scene, points: Array(bake.lightmapPointBytes()),
                                                      samples: bake.lightmapSamples(), seed: 1,
                                                      occlusionDistance: distance)
        // (the probes leave the sun out: the renderer adds it, shadowed)
        let (probes, _) = try tracer.run(.irradiance, scene, points: Array(bake.probePointBytes()),
                                         samples: bake.probeSamples(), seed: 2, occlusionDistance: distance,
                                         sun: scene.sunLight())
        let (radiance, _) = try tracer.run(.radiance, scene, points: Array(bake.reflectionPointBytes()),
                                           samples: bake.reflectionSamples(), seed: 3, occlusionDistance: 0)
        return bake.finish(irradiance: bytes(irradiance), directions: bytes(directions), probes: bytes(probes),
                           radiance: bytes(radiance))
    }

    /// The light of `bake`: the cached file's, or traced and then kept.
    public static func light(_ bake: LevelBake, folder: String, tracer: PathTracer) throws -> BakedLight {
        if let light = cached(bake, folder: folder) {
            return light
        }
        let light = try Self.bake(bake, tracer: tracer)
        try store(light, bake: bake, folder: folder)
        return light
    }

    /// Bakes `bake`'s light away from the main thread (on its own Metal
    /// queue), keeps it in `folder`, and gives it to `done` on the main
    /// thread.
    public static func bakeInBackground(_ bake: LevelBake, folder: String, device: MTLDevice,
                                        done: @escaping @MainActor @Sendable (Result<BakedLight, Error>) -> Void) {
        Task.detached(priority: .utility) {
            let result = Result { try light(bake, folder: folder, tracer: try PathTracer(device: device)) }
            await MainActor.run { done(result) }
        }
    }

    /// Float4s as the engine reads them.
    static func bytes(_ values: [SIMD4<Float>]) -> Data {
        values.withUnsafeBytes { Data($0) }
    }
}
