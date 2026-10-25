// The menus' controller: the Halo menu's lists filled from the maps and
// the servers, its keys, and what it asks done; the host's setup; and the
// settings, which go through the engine's TOML and back.
import Foundation
import OpenCEEngineCore
import OpenCEEngineKit
@testable import OpenCEEngineMenu
import XCTest

/// Games a test has found.
final class FakeSource: ServerSource {
    var answers: [ServerInfo] = []
    func servers() -> [ServerInfo] { answers }
}

/// What the menus asked of the app.
@MainActor
final class FakeActions: GameActions {
    var done: [String] = []
    var fails = false
    func playCampaign(map: URL, difficulty: UInt8) throws {
        if fails { throw EngineError.Network(message: "no") }
        done.append("campaign \(map.lastPathComponent) \(difficulty)")
    }
    func host(_ setup: HostSetup, map: URL, port: UInt16) throws {
        done.append("host \(map.lastPathComponent) \(setup.kind.setting) \(setup.maxPlayers) \(setup.name) \(port)")
    }
    func join(address: String, map: URL, name: String) throws {
        done.append("join \(address) \(map.lastPathComponent) \(name)")
    }
    func restartCheckpoint() throws { done.append("checkpoint") }
    func restartLevel() throws { done.append("restart") }
    func returnToMainMenu() { done.append("main menu") }
    func quit() { done.append("quit") }
}

/// A menu that records what it is given and asks what a test says.
final class FakeMenu: MenuProtocol, @unchecked Sendable {
    var widgets: Set<String> = ["main"]
    var opened: [String] = []
    var open = false
    var levels: [MenuEntry] = []
    var maps: [MenuEntry] = []
    var servers: [String] = []
    var profile = ""
    var keys: [MenuKey] = []
    var asks: [MenuAction] = []
    var now: UInt64 = 0

    func click(x: Float, y: Float, width: UInt32, height: UInt32, right: Bool) {}
    func draw(width: UInt32, height: UInt32) -> [UiQuad] { [] }
    func glyphAtlas() -> UiGlyphAtlas { UiGlyphAtlas(width: 0, height: 0, texels: Data()) }
    func isOpen() -> Bool { open }
    func key(key: MenuKey) { keys.append(key) }
    func `open`(widget: String) -> Bool {
        opened.append(widget)
        open = widgets.contains(widget)
        return open
    }
    func pointerMoved(x: Float, y: Float, width: UInt32, height: UInt32) {}
    func screen() -> String? { open ? opened.last : nil }
    func setLevels(levels: [MenuEntry]) { self.levels = levels }
    func setMaps(maps: [MenuEntry]) { self.maps = maps }
    func setProfile(profile: String) { self.profile = profile }
    func setServers(servers: [String]) { self.servers = servers }
    func takeActions() -> [MenuAction] {
        defer { asks = [] }
        return asks
    }
    func takeSounds() -> [String] { [] }
    func targets(width: UInt32, height: UInt32) -> [MenuPoint] { [] }
    func update(nowMs: UInt64) { now = nowMs }
    func wheel(steps: Int32) {}
}

func server(
    _ name: String, _ address: String, level: String = "bloodgulch", players: UInt8 = 1,
    capacity: UInt8 = 16
) -> ServerInfo {
    ServerInfo(address: address, name: name, level: level, gameType: 2, players: players, capacity: capacity)
}

@MainActor
final class MenuTests: XCTestCase {
    let maps = ["a30", "bloodgulch", "a10", "boardingaction", "ui"].map { URL(fileURLWithPath: "/maps/\($0).map") }
    var folder: URL!

    override func setUp() async throws {
        folder = FileManager.default.temporaryDirectory.appendingPathComponent("menu-\(UUID().uuidString)")
    }

    override func tearDown() async throws {
        try? FileManager.default.removeItem(at: folder)
    }

    func menu(source: FakeSource = FakeSource()) -> (HaloMenu, FakeMenu, FakeActions, [String]) {
        let settings = SettingsModel(path: folder.appendingPathComponent("settings.toml").path)
        let halo = HaloMenu(maps: maps, settings: settings, servers: ServerListModel { _ in source })
        let actions = FakeActions()
        halo.actions = actions
        let fake = FakeMenu()
        XCTAssertTrue(halo.show(fake, widget: "main"))
        return (halo, fake, actions, [])
    }

    func testTheListsAreTheMissionsInOrderAndTheOtherMaps() {
        let (halo, fake, _, _) = menu()
        XCTAssertEqual(
            fake.levels, [MenuEntry(map: "a10", name: "The Pillar of Autumn"), MenuEntry(map: "a30", name: "Halo")])
        XCTAssertEqual(fake.maps.map(\.map), ["bloodgulch", "boardingaction"], "no missions, no ui")
        XCTAssertEqual(fake.profile, "Spartan")
        XCTAssertTrue(halo.isOpen)
        XCTAssertFalse(halo.show(FakeMenu(), widget: "nowhere"))
        XCTAssertNil(halo.menu, "a widget not there opens nothing")
    }

    func testTheKeysAreTheMenusOnly() {
        let (halo, fake, _, _) = menu()
        for code: UInt16 in [126, 125, 36, 53, 0, 122, 12] {
            _ = halo.key(code: code)
        }
        XCTAssertEqual(fake.keys, [.up, .down, .return, .escape, .a, .f1], "Q (12) is not the menu's")
        XCTAssertNil(HaloMenu.key(50))
    }

    func testTheMissionsPlayAndAMissingOneIsSaid() {
        let (halo, fake, actions, _) = menu()
        var said: [String] = []
        halo.onMessage = { said.append($0) }
        fake.asks = [.playCampaign(map: "a30", difficulty: 9), .playCampaign(map: "d40", difficulty: 1)]
        halo.update(now: 0)
        XCTAssertEqual(actions.done, ["campaign a30.map 3"], "the difficulty kept to legendary")
        XCTAssertEqual(said, ["There is no mission called d40."])
        actions.fails = true
        fake.asks = [.playCampaign(map: "a10", difficulty: 0)]
        halo.update(now: 1)
        XCTAssertEqual(said.count, 2, "the app's error is said")
        XCTAssertEqual(fake.now, 1000)
    }

    func testTheServerListIsSortedAndJoinsGamesWithRoomOnMapsItHas() {
        let source = FakeSource()
        let (halo, fake, actions, _) = menu(source: source)
        var said: [String] = []
        halo.onMessage = { said.append($0) }
        source.answers = [
            server("zeta", "10.0.0.2:2302"), server("Alpha", "10.0.0.3:2302", players: 16),
            server("beta", "10.0.0.4:2302", level: "levels\\test\\icefields\\icefields"),
        ]
        halo.update(now: 0)
        XCTAssertEqual(fake.servers, ["Alpha  bloodgulch  16/16", "beta  icefields  1/16", "zeta  bloodgulch  1/16"])
        fake.asks = [.joinServer(index: 0), .joinServer(index: 1), .joinServer(index: 2), .joinServer(index: 7)]
        halo.update(now: 0.1)
        XCTAssertEqual(said, ["Alpha is full.", "beta plays icefields, which is not in your maps."])
        XCTAssertEqual(actions.done, ["join 10.0.0.2:2302 bloodgulch.map Spartan"])
        source.answers = []
        halo.update(now: 0.2)
        XCTAssertEqual(fake.servers.count, 3, "taken only each quarter second")
        halo.update(now: 0.4)
        XCTAssertEqual(fake.servers, [])
    }

    func testHostingSavesTheMapAndThePauseMenusChoicesAreDone() throws {
        let (halo, fake, actions, _) = menu()
        fake.asks = [
            .hostGame(map: "boardingaction"), .restartCheckpoint, .restartLevel, .returnToMainMenu,
            .leaveGame, .quit,
        ]
        halo.update(now: 0)
        let setup = HostSetup(game: halo.settings.settings.game)
        XCTAssertEqual(
            actions.done,
            [
                "host boardingaction.map \(setup.kind.setting) \(setup.maxPlayers) \(setup.name) 2302",
                "checkpoint", "restart", "main menu", "main menu", "quit",
            ])
        XCTAssertEqual(loadSettings(path: halo.settings.path).game.map, "boardingaction")
    }

    func testAMenuThatClosesItselfIsPutAway() {
        let (halo, fake, _, _) = menu()
        var closed = 0
        halo.onClosed = { closed += 1 }
        halo.update(now: 0)
        XCTAssertEqual(closed, 0)
        fake.open = false
        halo.update(now: 0.016)
        XCTAssertEqual(closed, 1)
        XCTAssertNil(halo.menu)
        halo.update(now: 0.032)
        XCTAssertEqual(closed, 1)
    }

    func testTheHostSetupIsKeptInRange() {
        var setup = HostSetup(game: stockSettings().game)
        setup.maxPlayers = 40
        XCTAssertEqual(setup.maxPlayers, 16)
        setup.maxPlayers = 1
        XCTAssertEqual(setup.maxPlayers, 2)
        setup.name = String(repeating: "é", count: 20)
        XCTAssertEqual(setup.name.utf8.count, 32, "cut at a character")
        var game = stockSettings().game
        setup.write(into: &game)
        XCTAssertEqual(HostSetup(game: game), setup)
    }

    func testSettingsAreClampedAndBindingsMove() {
        let settings = SettingsModel(path: folder.appendingPathComponent("settings.toml").path)
        settings.mouseSensitivity = 5
        XCTAssertEqual(settings.mouseSensitivity, SettingsModel.mouseSensitivity.upperBound)
        settings.renderScale = 0.1
        XCTAssertEqual(settings.renderScale, 0.5)
        settings.master = .nan
        XCTAssertEqual(settings.master, 0)
        settings.antiAliasing = "fxaa"
        XCTAssertEqual(settings.antiAliasing, "temporal")
        XCTAssertEqual(settings.settings.controls.bindings, [], "the stock keys are not written")
        settings.bind(.jump, to: 13)  // (W, forward's)
        XCTAssertEqual(settings.bindings.keys[.jump], [13])
        XCTAssertEqual(settings.bindings.keys[.forward], [])
        XCTAssertEqual(Set(settings.settings.controls.bindings.map(\.action)), ["jump", "forward"])
        settings.reset(.controls)
        XCTAssertEqual(settings.bindings, KeyBindings.stock)
        XCTAssertEqual(KeyNames.names([59, 62]), "Control, Right Control")
        XCTAssertEqual(KeyNames.name(KeyNames.button(1)), "Right Button")
    }

    func testTheSettingsGoThroughTomlAndBack() throws {
        let settings = SettingsModel(path: folder.appendingPathComponent("settings.toml").path)
        settings.playerName = "Chief"
        settings.mouseSensitivity = 0.003
        settings.settings.controls.invertMouse = true
        settings.bind(.melee, to: 8)
        settings.antiAliasing = "native"
        settings.renderScale = 0.75
        settings.settings.video.fullScreen = true
        settings.music = 0.25
        let text = try settingsToToml(settings: settings.settings)
        XCTAssertTrue(text.contains("player_name = \"Chief\""), text)
        XCTAssertEqual(try settingsFromToml(text: text), settings.settings)

        settings.save()
        let loaded = SettingsModel(path: settings.path)
        XCTAssertEqual(loaded.settings, settings.settings)
        XCTAssertEqual(loaded.bindings.keys[.melee], [8])
        XCTAssertEqual(loadSettings(path: folder.appendingPathComponent("none.toml").path), stockSettings())
    }

    func testTheFontsAreFound() {
        XCTAssertNotNil(MenuFonts.folder(bundle: Bundle(for: MenuTests.self)))
        XCTAssertTrue(MenuFonts.register())
        XCTAssertEqual(MenuFonts.names.count, 3)
        XCTAssertTrue(MenuFonts.names.values.contains { $0.hasPrefix("Overpass") }, "\(MenuFonts.names)")
    }
}
