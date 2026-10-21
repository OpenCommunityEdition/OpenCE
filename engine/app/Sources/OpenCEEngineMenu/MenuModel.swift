// The menus' model: which screen shows, the way back, and what each
// screen does (play alone, host, join from the list or by address, the
// settings). The app does the playing (MenuActions).
import Foundation
import OpenCEEngineCore

/// A settings screen.
public enum SettingsPage: String, CaseIterable, Sendable {
    case game, controls, video, audio

    public var title: String { rawValue.capitalized }
}

/// A menu screen.
public enum MenuScreen: Hashable, Sendable {
    case main, solo, servers, join, host
    case settings(SettingsPage)

    public var title: String {
        switch self {
        case .main: "OpenCE"
        case .solo: "Play Alone"
        case .servers: "Join a Game"
        case .join: "Join by Address"
        case .host: "Host a Game"
        case let .settings(page): "\(page.title) Settings"
        }
    }
}

/// What the menus ask the app to do.
@MainActor
public protocol MenuActions: AnyObject {
    /// Plays `map` alone.
    func playAlone(map: URL) throws
    /// Hosts `setup`'s game on `map`.
    func host(_ setup: HostSetup, map: URL, port: UInt16) throws
    /// Joins the game at `address` on `map`, as `name`.
    func join(address: String, map: URL, name: String) throws
    func quit()
}

@MainActor
@Observable
public final class MenuModel {
    /// How long an address is asked before it is given up on, in ticks.
    public static let askTicks = 12

    /// The screens shown, the first the main menu (never empty).
    public private(set) var path: [MenuScreen] = [.main]
    /// The maps that can be played, by file.
    public let maps: [URL]
    public let settings: SettingsModel
    public let servers: ServerListModel
    public var host: HostSetup
    public var address = ""
    /// What went wrong last, shown until the next screen.
    public var message: String?
    /// The address being asked, and for how many more ticks.
    public private(set) var asking: (address: String, ticks: Int)?
    public weak var actions: MenuActions?

    public init(maps: [URL], settings: SettingsModel, servers: ServerListModel) {
        self.maps = maps
        self.settings = settings
        self.servers = servers
        host = HostSetup(game: settings.settings.game)
        if !maps.isEmpty, map(named: host.map) == nil, let first = maps.first {
            host.map = first.deletingPathExtension().lastPathComponent
        }
    }

    public var screen: MenuScreen { path.last ?? .main }

    /// The map file called `name` (without `.map`), if there is one.
    public func map(named name: String) -> URL? {
        maps.first { $0.deletingPathExtension().lastPathComponent.lowercased() == name.lowercased() }
    }

    /// The maps' names, in order.
    public var mapNames: [String] {
        maps.map { $0.deletingPathExtension().lastPathComponent }
    }

    public func show(_ screen: MenuScreen) {
        message = nil
        asking = nil
        if screen == .main {
            path = [.main]
            return
        }
        if case .settings = screen, case .settings = self.screen {
            // (one settings page in place of another)
            path[path.count - 1] = screen
        } else {
            path.append(screen)
        }
        if screen == .servers {
            servers.search()
        }
    }

    /// Back a screen (the settings are saved on the way out).
    public func back() {
        if case .settings = screen {
            settings.save()
        }
        message = nil
        asking = nil
        if path.count > 1 {
            path.removeLast()
        }
    }

    public func playAlone(map name: String) {
        guard let map = map(named: name) else {
            message = "There is no map called \(name)."
            return
        }
        perform { try $0.playAlone(map: map) }
    }

    public func startHosting() {
        guard let map = map(named: host.map) else {
            message = "Choose a map to host."
            return
        }
        host.write(into: &settings.settings.game)
        settings.save()
        let port = settings.settings.game.port
        perform { [host] in try $0.host(host, map: map, port: port) }
    }

    /// Joins a game the list found.
    public func join(_ server: ServerInfo) {
        guard server.hasRoom else {
            message = "\(server.name) is full."
            return
        }
        guard let map = map(named: server.mapName) else {
            message = "\(server.name) plays \(server.mapName), which is not in your maps."
            return
        }
        let name = settings.settings.game.playerName
        perform { try $0.join(address: server.address, map: map, name: name) }
    }

    /// Asks the typed address for its game, to join it when it answers.
    public func joinAddress() {
        guard let parsed = JoinAddress.parse(address) else {
            message = "\(address) is not an address."
            return
        }
        message = nil
        servers.search(addresses: [parsed.text])
        if let problem = servers.problem {
            message = problem
            return
        }
        asking = (parsed.text, MenuModel.askTicks)
    }

    /// A quarter second passed: the list takes its answers, and an asked
    /// address is joined when it answers (or given up on).
    public func tick() {
        servers.refresh()
        guard let asked = asking else { return }
        if let server = servers.servers.first {
            asking = nil
            join(server)
        } else if asked.ticks <= 1 {
            asking = nil
            message = "No game answered at \(asked.address)."
        } else {
            asking = (asked.address, asked.ticks - 1)
        }
    }

    public func quit() {
        settings.save()
        actions?.quit()
    }

    private func perform(_ action: (MenuActions) throws -> Void) {
        guard let actions else { return }
        do {
            try action(actions)
            message = nil
        } catch {
            message = "\(error)"
        }
    }
}
