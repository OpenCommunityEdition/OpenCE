// The maps the app offers: the .map files of the data folders (assets/maps
// and assets/custom_maps, or the folder --maps names), without the shared
// resource maps, which are not levels.
import Foundation

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

    /// The levels of all `folders`, in order.
    public static func maps(in folders: [URL]) -> [URL] {
        folders.flatMap { maps(in: $0) }
    }
}
