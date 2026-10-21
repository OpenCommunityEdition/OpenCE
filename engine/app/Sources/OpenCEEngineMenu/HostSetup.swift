// A game to host: its map, its kind, how many may play and its name, as
// the game settings keep them.
import Foundation
import OpenCEEngineCore

/// The longest a game's or player's name may be, in UTF-8 bytes (the
/// packets' limit).
public let maxNameBytes = 32

/// `text` cut to at most `bytes` UTF-8 bytes, at a character's end.
public func fitted(_ text: String, bytes: Int) -> String {
    var result = ""
    var used = 0
    for character in text {
        let size = character.utf8.count
        guard used + size <= bytes else { break }
        result.append(character)
        used += size
    }
    return result
}

extension GameKind {
    /// The kinds, in the menu's order.
    public static let all: [GameKind] = [.slayer, .teamSlayer, .captureTheFlag]

    /// Its name in the menus.
    public var title: String {
        switch self {
        case .slayer: "Slayer"
        case .teamSlayer: "Team Slayer"
        case .captureTheFlag: "Capture the Flag"
        }
    }

    /// Its name in the settings file.
    public var setting: String {
        switch self {
        case .slayer: "slayer"
        case .teamSlayer: "team_slayer"
        case .captureTheFlag: "ctf"
        }
    }

    /// The kind the settings file names (Slayer for one it does not know).
    public init(setting: String) {
        self = GameKind.all.first { $0.setting == setting } ?? .slayer
    }
}

/// The name of Halo's game type `number` (Team Slayer is Slayer with
/// teams, the same number).
public func gameTypeTitle(_ number: UInt8) -> String {
    switch number {
    case 1: "Capture the Flag"
    case 2: "Slayer"
    case 3: "Oddball"
    case 4: "King of the Hill"
    case 5: "Race"
    default: "Unknown"
    }
}

public struct HostSetup: Equatable, Sendable {
    /// How many may play, the host among them.
    public static let players: ClosedRange<Int> = 2...16

    /// The map, by its file's name without `.map`.
    public var map: String
    public var kind: GameKind
    public var maxPlayers: Int {
        didSet { maxPlayers = min(max(maxPlayers, HostSetup.players.lowerBound), HostSetup.players.upperBound) }
    }
    public var name: String {
        didSet {
            let cut = fitted(name, bytes: maxNameBytes)
            if cut != name { name = cut }
        }
    }

    /// The setup the game settings keep.
    public init(game: GameSettings) {
        map = game.map
        kind = GameKind(setting: game.gameType)
        maxPlayers = min(max(Int(game.maxPlayers), HostSetup.players.lowerBound), HostSetup.players.upperBound)
        name = fitted(game.serverName, bytes: maxNameBytes)
    }

    /// Kept in `game`.
    public func write(into game: inout GameSettings) {
        game.map = map
        game.gameType = kind.setting
        game.maxPlayers = UInt8(maxPlayers)
        game.serverName = name
    }
}
