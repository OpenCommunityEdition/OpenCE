// The Halo menus from ui.map: the main menu over its scene and the pause
// menus over a game. The engine's widgets draw and take the keys, the
// pointer and the wheel; this gives them the levels, maps, servers and
// profile they list, and does what they ask (GameActions).
import Foundation
import OpenCEEngineCore

/// What the menus ask the app to do.
@MainActor
public protocol GameActions: AnyObject {
    /// Plays `map`'s mission at `difficulty` (0 easy to 3 legendary).
    func playCampaign(map: URL, difficulty: UInt8) throws
    /// Hosts `setup`'s game on `map`.
    func host(_ setup: HostSetup, map: URL, port: UInt16) throws
    /// Joins the game at `address` on `map`, as `name`.
    func join(address: String, map: URL, name: String) throws
    /// Plays the mission again from its last checkpoint.
    func restartCheckpoint() throws
    /// Plays the mission again from its start.
    func restartLevel() throws
    /// Leaves the mission or the network game for the main menu.
    func returnToMainMenu()
    func quit()
}

@MainActor
public final class HaloMenu {
    /// The campaign's missions, in the order they are played, with their
    /// titles.
    public nonisolated static let missions: [(map: String, title: String)] = [
        ("a10", "The Pillar of Autumn"), ("a30", "Halo"), ("a50", "The Truth and Reconciliation"),
        ("b30", "The Silent Cartographer"), ("b40", "Assault on the Control Room"), ("c10", "343 Guilty Spark"),
        ("c20", "The Library"), ("c40", "Two Betrayals"), ("d20", "Keyes"), ("d40", "The Maw"),
    ]

    /// The mission after `map` among `maps` (the next of the campaign's
    /// whose file is there), or nil after the last.
    public nonisolated static func nextMission(after map: String, in maps: [URL]) -> URL? {
        let name = URL(fileURLWithPath: map).deletingPathExtension().lastPathComponent.lowercased()
        guard let index = missions.firstIndex(where: { $0.map == name }) else { return nil }
        for mission in missions[(index + 1)...] {
            if let file = maps.first(where: { $0.deletingPathExtension().lastPathComponent.lowercased() == mission.map }) {
                return file
            }
        }
        return nil
    }
    /// The maps that are not multiplayer levels: the missions' and the
    /// shell's own.
    public static let notMultiplayer: Set<String> = Set(missions.map(\.map)).union(["ui"])
    /// How often the server list takes its answers, in seconds.
    public static let refreshSeconds = 0.25

    /// The maps that can be played, by file.
    public let maps: [URL]
    public let settings: SettingsModel
    public let servers: ServerListModel
    /// The menu open, if one is.
    public private(set) var menu: (any MenuProtocol)?
    public weak var actions: GameActions?
    /// What went wrong, for the app to show.
    public var onMessage: ((String) -> Void)?
    /// The menu closed itself (a pause menu's resume).
    public var onClosed: (() -> Void)?
    /// The servers as the menu was last given them.
    private var listed: [ServerInfo] = []
    private var lastRefresh = -Double.infinity

    public init(maps: [URL], settings: SettingsModel, servers: ServerListModel) {
        self.maps = maps
        self.settings = settings
        self.servers = servers
    }

    /// The map file called `name` (without `.map`), if there is one.
    public func map(named name: String) -> URL? {
        maps.first { $0.deletingPathExtension().lastPathComponent.lowercased() == name.lowercased() }
    }

    /// The missions whose maps are here, in order.
    public var levels: [MenuEntry] {
        HaloMenu.missions.filter { map(named: $0.map) != nil }.map { MenuEntry(map: $0.map, name: $0.title) }
    }

    /// The multiplayer maps here, by name.
    public var multiplayerMaps: [MenuEntry] {
        maps.map { $0.deletingPathExtension().lastPathComponent }
            .filter { !HaloMenu.notMultiplayer.contains($0.lowercased()) }
            .map { MenuEntry(map: $0, name: $0) }
    }

    public var isOpen: Bool { menu?.isOpen() ?? false }

    /// Opens `widget` on `menu`, its lists filled; false when the menu
    /// has no such widget.
    @discardableResult
    public func show(_ menu: any MenuProtocol, widget: String) -> Bool {
        menu.setLevels(levels: levels)
        menu.setMaps(maps: multiplayerMaps)
        menu.setProfile(profile: settings.settings.game.playerName)
        servers.search()
        listed = []
        menu.setServers(servers: [])
        lastRefresh = -.infinity
        guard menu.open(widget: widget) else {
            self.menu = nil
            return false
        }
        self.menu = menu
        return true
    }

    /// Puts the menu away.
    public func close() {
        menu = nil
        settings.save()
    }

    /// A frame at `now` seconds: the server list takes its answers, the
    /// menu animates, and what it asked for is done.
    public func update(now: Double) {
        guard let menu else { return }
        if now - lastRefresh >= HaloMenu.refreshSeconds {
            lastRefresh = now
            servers.refresh()
            if servers.servers != listed {
                listed = servers.servers
                menu.setServers(servers: listed.map(HaloMenu.row))
            }
        }
        menu.update(nowMs: UInt64(max(now, 0) * 1000))
        // (UI sounds are not played yet)
        _ = menu.takeSounds()
        for action in menu.takeActions() {
            perform(action)
        }
        if self.menu === menu, !menu.isOpen() {
            close()
            onClosed?()
        }
    }

    /// A server's row in the list: its name, map and players.
    public static func row(_ server: ServerInfo) -> String {
        "\(server.name)  \(server.mapName)  \(server.players)/\(server.capacity)"
    }

    /// The menu's key for AppKit's key `code`, if it takes it.
    public static func key(_ code: UInt16) -> MenuKey? {
        switch code {
        case 126: .up
        case 125: .down
        case 123: .left
        case 124: .right
        case 49: .space
        case 36: .return
        case 76: .keypadEnter
        case 53: .escape
        case 51: .backspace
        case 117: .delete
        case 14: .e
        case 48: .tab
        case 122: .f1
        case 13: .w
        case 0: .a
        case 1: .s
        case 2: .d
        default: nil
        }
    }

    /// Key `code` pressed: true when the menu took it.
    public func key(code: UInt16) -> Bool {
        guard let menu, let key = HaloMenu.key(code) else { return false }
        menu.key(key: key)
        return true
    }

    /// Does what the menu asked.
    public func perform(_ action: MenuAction) {
        guard let actions else { return }
        do {
            switch action {
            case let .playCampaign(name, difficulty):
                guard let map = map(named: name) else { return message("There is no mission called \(name).") }
                try actions.playCampaign(map: map, difficulty: min(difficulty, 3))
            case let .hostGame(name):
                guard let map = map(named: name) else { return message("There is no map called \(name).") }
                var setup = HostSetup(game: settings.settings.game)
                setup.map = name
                setup.write(into: &settings.settings.game)
                settings.save()
                try actions.host(setup, map: map, port: settings.settings.game.port)
            case let .joinServer(index):
                guard listed.indices.contains(Int(index)) else { return }
                try join(listed[Int(index)])
            case .restartCheckpoint:
                try actions.restartCheckpoint()
            case .restartLevel:
                try actions.restartLevel()
            case .returnToMainMenu, .leaveGame:
                actions.returnToMainMenu()
            case .quit:
                settings.save()
                actions.quit()
            }
        } catch {
            message("\(error)")
        }
    }

    private func join(_ server: ServerInfo) throws {
        guard server.hasRoom else { return message("\(server.name) is full.") }
        guard let map = map(named: server.mapName) else {
            return message("\(server.name) plays \(server.mapName), which is not in your maps.")
        }
        try actions?.join(address: server.address, map: map, name: settings.settings.game.playerName)
    }

    private func message(_ text: String) {
        onMessage?(text)
    }
}
