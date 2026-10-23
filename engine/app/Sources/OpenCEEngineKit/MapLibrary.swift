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

    /// The test map's name: the engine makes it, gathering every map of
    /// the first folder.
    public static let testMap = "devmap"

    /// The path `--map` loads: the test map's in the first of `folders`
    /// (no file is there; the engine knows the name), any other as given.
    public static func path(of map: String, folders: [URL]) -> String {
        guard map == testMap, let folder = folders.first else {
            return map
        }
        return folder.appendingPathComponent("\(testMap).map").path
    }

    /// The levels of all `folders`, in order.
    public static func maps(in folders: [URL]) -> [URL] {
        folders.flatMap { maps(in: $0) }
    }
}
