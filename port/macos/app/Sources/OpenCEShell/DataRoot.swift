// The game's data folder (HALO_DATA_ROOT for the host): where its maps,
// config.toml and debug.txt are. The app chooses it before it starts the
// guest, and says so in a dialog if the maps are not there.

import Foundation

public enum DataRoot {
    /// HALO_DATA_ROOT if it is set, else OpenCE's folder in Application
    /// Support (where the host keeps saves too).
    public static func resolve(environment: [String: String], applicationSupport: URL) -> URL {
        if let root = environment["HALO_DATA_ROOT"], !root.isEmpty {
            return URL(fileURLWithPath: root, isDirectory: true)
        }
        return applicationSupport.appendingPathComponent("OpenCE", isDirectory: true)
    }

    /// The maps folder of `root`.
    public static func maps(in root: URL) -> URL {
        root.appendingPathComponent("maps", isDirectory: true)
    }

    /// What the dialog says when `root` has no maps the game can start
    /// with; nil when it has them. ui.map is the one the game cannot start
    /// without: its menus are in it.
    public static func missingMapsMessage(root: URL, fileManager: FileManager = .default) -> String? {
        let maps = maps(in: root)
        if fileManager.fileExists(atPath: maps.appendingPathComponent("ui.map").path) {
            return nil
        }
        return "Copy the maps folder of Halo for PC or Xbox (with ui.map, bloodgulch.map and the rest) into "
            + "\(maps.path), then open OpenCE again."
    }
}
