// The games found on the network, or the one game asked by its address,
// as the server list shows them.
import Foundation
import OpenCEEngineCore

/// Where the list's games come from: the engine's search, or a test's.
public protocol ServerSource: AnyObject {
    func servers() -> [ServerInfo]
}

extension ServerSearch: ServerSource {}

extension ServerInfo {
    /// Whether there is room for one more.
    public var hasRoom: Bool { players < capacity }

    /// Its map's name: the level's last part (`levels\test\bloodgulch\bloodgulch`
    /// or `bloodgulch` is `bloodgulch`).
    public var mapName: String {
        String(level.split(whereSeparator: { $0 == "\\" || $0 == "/" }).last ?? Substring(level))
    }
}

@MainActor
@Observable
public final class ServerListModel {
    /// The games found, by name.
    public private(set) var servers: [ServerInfo] = []
    /// The address of the game chosen, if any.
    public var selected: String?
    /// Why the search could not start, if it could not.
    public private(set) var problem: String?
    private var source: ServerSource?
    private let search: ([String]) throws -> ServerSource

    /// A list whose searches `search` makes: with no addresses the whole
    /// network's, otherwise those addresses'.
    public init(search: @escaping ([String]) throws -> ServerSource) {
        self.search = search
    }

    /// The engine's searches.
    public convenience init() {
        self.init { addresses in
            addresses.isEmpty ? try ServerSearch.start() : try ServerSearch.asking(addresses: addresses)
        }
    }

    /// Starts a new search (of `addresses`, or of the network), forgetting
    /// the games found.
    public func search(addresses: [String] = []) {
        servers = []
        selected = nil
        do {
            source = try search(addresses)
            problem = nil
        } catch {
            source = nil
            problem = "\(error)"
        }
    }

    /// Takes the answers that have come.
    public func refresh() {
        guard let source else { return }
        servers = source.servers().sorted {
            ($0.name.localizedLowercase, $0.address) < ($1.name.localizedLowercase, $1.address)
        }
        if let selected, !servers.contains(where: { $0.address == selected }) {
            self.selected = nil
        }
    }

    /// The game chosen.
    public var chosen: ServerInfo? {
        servers.first { $0.address == selected }
    }
}
