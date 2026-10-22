// The renderer's Metal shaders' sources (engine/app/Shaders), compiled at
// run time (swift build does not compile .metal files). A build run from
// the checkout reads them from there and watches them, so that an edited
// shader is compiled again while the game runs; the app's bundle carries
// a copy in Resources/shaders (tools/engine_build.py).
import Foundation
import Metal

/// One of the renderer's shader files.
public enum Shader: String, CaseIterable, Sendable {
    /// The level's surfaces, the objects, the sky, the tone map.
    case level
    case hud
    case text
    case debug
    /// The screen's passes: occlusion, reflections, their resolve.
    case screen
    /// The bake's path tracer (PathTracer, compiled for each bake).
    case bake

    public var file: String { "\(rawValue).metal" }
}

public enum ShaderSources {
    /// The checkout's engine/app/Shaders, if it is there.
    public static var checkout: URL? {
        var folder = URL(fileURLWithPath: #filePath)
        for _ in 0..<3 {
            folder.deleteLastPathComponent()
        }
        folder.appendPathComponent("Shaders")
        return FileManager.default.fileExists(atPath: folder.appendingPathComponent(Shader.level.file).path)
            ? folder : nil
    }

    /// The folder the shaders are read from: the checkout's (a
    /// development build) or the bundle's Resources/shaders.
    public static func folder(bundle: Bundle = .main) -> URL? {
        if let checkout {
            return checkout
        }
        let bundled = bundle.resourceURL?.appendingPathComponent("shaders")
        return bundled.flatMap { FileManager.default.fileExists(atPath: $0.path) ? $0 : nil }
    }

    /// The source of `shader` in `folder`.
    public static func source(_ shader: Shader, in folder: URL? = folder()) throws -> String {
        guard let folder else {
            throw LevelRendererError.shaders("no shaders folder (engine/app/Shaders, or the bundle's)")
        }
        do {
            return try String(contentsOf: folder.appendingPathComponent(shader.file), encoding: .utf8)
        } catch {
            throw LevelRendererError.shaders("\(shader.file): \(error.localizedDescription)")
        }
    }
}

/// Notices the shader files that change in a folder, by their modification
/// times and sizes.
public final class ShaderWatcher {
    public let folder: URL
    private var seen: [Shader: (Date, Int)] = [:]

    public init(folder: URL) {
        self.folder = folder
        for shader in Shader.allCases {
            seen[shader] = stamp(shader)
        }
    }

    private func stamp(_ shader: Shader) -> (Date, Int)? {
        let path = folder.appendingPathComponent(shader.file).path
        guard let attributes = try? FileManager.default.attributesOfItem(atPath: path),
              let date = attributes[.modificationDate] as? Date,
              let size = attributes[.size] as? Int else { return nil }
        return (date, size)
    }

    /// The shaders whose files changed since the last call (or since the
    /// watcher was made). A file that is removed is not a change.
    public func changed() -> [Shader] {
        var out: [Shader] = []
        for shader in Shader.allCases {
            guard let now = stamp(shader) else { continue }
            if let before = seen[shader], before.0 == now.0, before.1 == now.1 {
                continue
            }
            seen[shader] = now
            out.append(shader)
        }
        return out
    }
}

/// Compiles the shaders that change into the passes using them (the level's
/// into the level and its objects, the debug drawing's with the level's
/// renderer).
public final class ShaderReloader {
    public let watcher: ShaderWatcher

    /// A reloader for a development build (one run from the checkout), or
    /// nil.
    public convenience init?() {
        guard let checkout = ShaderSources.checkout else { return nil }
        self.init(folder: checkout)
    }

    public init(folder: URL) {
        watcher = ShaderWatcher(folder: folder)
    }

    /// One reload: the shader, and why it was not used (its old pipelines
    /// are kept), or nil.
    public struct Reload {
        public var shader: Shader
        public var error: Error?
    }

    /// Compiles each shader that changed into the passes given (one that
    /// is nil is skipped: it reads the new source when it is made).
    public func poll(level: LevelRenderer?, hud: HudPass?, text: TextPass?) -> [Reload] {
        var out: [Reload] = []
        for shader in watcher.changed() {
            let reload: ((String) throws -> Void)?
            switch shader {
            case .level: reload = level.map { pass in { try pass.reloadShaders(source: $0) } }
            case .debug: reload = level.map { pass in { try pass.debug.reloadShaders(source: $0) } }
            case .screen: reload = level.map { pass in { try pass.screen.reloadShaders(source: $0) } }
            case .hud: reload = hud.map { pass in { try pass.reloadShaders(source: $0) } }
            case .text: reload = text.map { pass in { try pass.reloadShaders(source: $0) } }
            case .bake: reload = nil
            }
            guard let reload else { continue }
            do {
                try reload(ShaderSources.source(shader, in: watcher.folder))
                out.append(Reload(shader: shader, error: nil))
            } catch {
                out.append(Reload(shader: shader, error: error))
            }
        }
        return out
    }
}

extension MTLLibrary {
    /// The function `name`: a pipeline made without one (a fragment
    /// function may be left out) would draw nothing, not fail.
    func required(_ name: String) throws -> MTLFunction {
        guard let function = makeFunction(name: name) else {
            throw LevelRendererError.shaders("no function \(name)")
        }
        return function
    }
}
