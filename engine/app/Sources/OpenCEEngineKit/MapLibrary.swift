// The maps the app offers: the .map files of the data folders (assets/maps
// and assets/custom_maps, the settings' folders, or the folder --maps
// names), without the shared resource maps, which are not levels.
import Foundation
import OpenCEEngineCore

public enum MapLibrary {
    /// The shared resource maps of Custom Edition's folders.
    public static let resourceMaps: Set<String> = ["bitmaps", "sounds", "loc"]

    /// The levels in `folder`, by name.
    public static func maps(in folder: URL) -> [URL] {
        let files = (try? FileManager.default.contentsOfDirectory(at: folder, includingPropertiesForKeys: [.isRegularFileKey],
                                                                 options: [.skipsHiddenFiles])) ?? []
        return files
            .filter { $0.pathExtension.lowercased() == "map" }
            .filter { !resourceMaps.contains($0.deletingPathExtension().lastPathComponent.lowercased()) }
            .filter { (try? $0.resourceValues(forKeys: [.isRegularFileKey]).isRegularFile) == true }
            .sorted { $0.lastPathComponent.localizedStandardCompare($1.lastPathComponent) == .orderedAscending }
    }

    /// The data root, the folder holding assets/maps: OPENCE_ROOT if set,
    /// else the first of `current` and the folders above `bundle` that
    /// has assets/maps (so an app opened from build/engine finds the
    /// checkout's), else `current`.
    public static func dataRoot(environment: [String: String], current: URL, bundle: URL) -> URL {
        if let set = environment["OPENCE_ROOT"], !set.isEmpty {
            return URL(fileURLWithPath: set, isDirectory: true)
        }
        var above: [URL] = []
        var folder = bundle.standardizedFileURL
        while folder.path != "/" {
            folder = folder.deletingLastPathComponent()
            above.append(folder)
        }
        let found = ([current] + above).first { candidate in
            var directory: ObjCBool = false
            return FileManager.default.fileExists(atPath: candidate.appendingPathComponent("assets/maps").path,
                                                  isDirectory: &directory) && directory.boolValue
        }
        return found ?? current
    }

    /// The data root of this process (`dataRoot` with its environment,
    /// working folder and bundle).
    public static var dataRoot: URL {
        dataRoot(environment: ProcessInfo.processInfo.environment,
                 current: URL(fileURLWithPath: FileManager.default.currentDirectoryPath, isDirectory: true),
                 bundle: Bundle.main.bundleURL)
    }

    /// The folders searched: `named` alone if given, otherwise the data
    /// root's maps and custom_maps under `root`.
    public static func folders(named: String?, root: URL) -> [URL] {
        if let named {
            return [URL(fileURLWithPath: named, isDirectory: true)]
        }
        return ["maps", "custom_maps"].map { root.appendingPathComponent("assets/\($0)", isDirectory: true) }
    }

    /// The folders the settings name, each empty one the data root's
    /// under `root`.
    public static func folders(data: DataSettings, root: URL) -> [URL] {
        zip([data.mapsFolder, data.customMapsFolder], folders(named: nil, root: root)).map { set, stock in
            set.isEmpty ? stock : URL(fileURLWithPath: set, isDirectory: true)
        }
    }

    /// The test maps' names, which the engine makes when they are loaded:
    /// devgulch from the first folder's bloodgulch.map, holding every
    /// definition of its other maps, and devmap's galleries of them.
    public static let testMaps = ["devgulch", "devmap"]

    /// The path `--map` loads: a test map's in the first of `folders` (no
    /// file is there; the engine knows the name), any other as given.
    public static func path(of map: String, folders: [URL]) -> String {
        guard testMaps.contains(map), let folder = folders.first else {
            return map
        }
        return folder.appendingPathComponent("\(map).map").path
    }

    /// The levels of all `folders`, in order.
    public static func maps(in folders: [URL]) -> [URL] {
        folders.flatMap { maps(in: $0) }
    }
}
