// What the launcher's Developer tab starts: a test map, the station to
// start at, the buffer to view, the drawing switches as they start and a
// frame to capture, as the arguments the game reads (--map, --station,
// --view, --set, --capture), so that the same launch can be typed.
import Foundation
import OpenCEEngineRender

public struct DeveloperLaunch: Equatable, Sendable {
    /// The test map (MapLibrary.developerMaps, or any of testMaps).
    public var level: String
    /// The station run once the level is loaded (`station <name>`).
    public var station: String?
    /// What `render.view` shows.
    public var view: RenderView
    /// The shell variables set before the first frame, by name.
    public var variables: [String: String]
    /// Where the first frame is drawn to (`frame <path>`).
    public var capture: String?

    public init(
        level: String = MapLibrary.developerMaps[0], station: String? = nil, view: RenderView = .frame,
        variables: [String: String] = [:], capture: String? = nil
    ) {
        self.level = level
        self.station = station
        self.view = view
        self.variables = variables
        self.capture = capture
    }

    /// The game's arguments for it.
    public var arguments: [String] {
        var arguments = ["--map", level]
        if let station {
            arguments += ["--station", station]
        }
        if view != .frame {
            arguments += ["--view", view.rawValue]
        }
        for (name, value) in variables.sorted(by: { $0.key < $1.key }) {
            arguments += ["--set", "\(name)=\(value)"]
        }
        if let capture {
            arguments += ["--capture", capture]
        }
        return arguments
    }

    /// The launch `arguments` ask for; none without --map. (The last of
    /// an argument given twice wins, as the launcher's are added after the
    /// command line's.)
    public init?(arguments: [String]) {
        var level: String?
        self.init()
        var index = arguments.startIndex
        while index + 1 < arguments.endIndex {
            let value = arguments[index + 1]
            switch arguments[index] {
            case "--map": level = value
            case "--station": station = value
            case "--view": view = RenderView(rawValue: value) ?? view
            case "--set":
                let parts = value.split(separator: "=", maxSplits: 1).map(String.init)
                if parts.count == 2 {
                    variables[parts[0]] = parts[1]
                }
            case "--capture": capture = value
            default:
                index += 1
                continue
            }
            index += 2
        }
        guard let level else { return nil }
        self.level = level
    }

    /// The shell variables to set once the level is loaded, in order: the
    /// switches, then the view.
    public var settings: [(name: String, value: String)] {
        let switches = variables.sorted(by: { $0.key < $1.key }).map { (name: $0.key, value: $0.value) }
        return view == .frame ? switches : switches + [(name: "render.view", value: view.rawValue)]
    }

    /// The shell lines to run after them: the station, then the capture.
    public var commands: [String] {
        var commands: [String] = []
        if let station {
            commands.append("station \(station)")
        }
        if let capture {
            commands.append("frame \"\(capture)\"")
        }
        return commands
    }
}
