// The menus' models: the screens and the way back, the server list, an
// address typed to join, the host's setup, and the settings, which go
// through the engine's TOML and back.
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
final class FakeActions: MenuActions {
    var done: [String] = []
    var fails = false
    func playCampaign(map: URL, difficulty: UInt8) throws {
        done.append("campaign \(map.lastPathComponent) \(difficulty)")
    }
    func playAlone(map: URL) throws {
        if fails { throw EngineError.Network(message: "no") }
        done.append("alone \(map.lastPathComponent)")
    }
    func host(_ setup: HostSetup, map: URL, port: UInt16) throws {
        done.append("host \(map.lastPathComponent) \(setup.kind.setting) \(setup.maxPlayers) \(setup.name) \(port)")
    }
    func join(address: String, map: URL, name: String) throws {
        done.append("join \(address) \(map.lastPathComponent) \(name)")
    }
    func quit() { done.append("quit") }
}

func server(_ name: String, _ address: String, level: String = "bloodgulch", players: UInt8 = 1,
            capacity: UInt8 = 16) -> ServerInfo {
    ServerInfo(address: address, name: name, level: level, gameType: 2, players: players, capacity: capacity)
}

@MainActor
final class MenuTests: XCTestCase {
    let maps = ["bloodgulch", "boardingaction", "hangemhigh"].map { URL(fileURLWithPath: "/maps/\($0).map") }
    var folder: URL!

    override func setUp() async throws {
        folder = FileManager.default.temporaryDirectory.appendingPathComponent("menu-\(UUID().uuidString)")
    }

    override func tearDown() async throws {
        try? FileManager.default.removeItem(at: folder)
    }

    func menu(source: FakeSource = FakeSource()) -> (MenuModel, FakeActions) {
        let settings = SettingsModel(path: folder.appendingPathComponent("settings.toml").path)
        let model = MenuModel(maps: maps, settings: settings, servers: ServerListModel { _ in source })
        let actions = FakeActions()
        model.actions = actions
        return (model, actions)
    }

    func testTheScreensGoForwardAndBack() {
        let (model, _) = menu()
        XCTAssertEqual(model.screen, .main)
        model.show(.host)
        XCTAssertEqual(model.path, [.main, .host])
        model.back()
        model.show(.settings(.game))
        model.show(.settings(.video))
        XCTAssertEqual(model.path, [.main, .settings(.video)], "one settings page in place of another")
        model.back()
        model.back()
        XCTAssertEqual(model.path, [.main], "never past the main menu")
        model.show(.solo)
        model.show(.main)
        XCTAssertEqual(model.path, [.main])
    }

    func testTheCampaignPlaysOnlyItsMissionsAtTheChosenDifficulty() {
        let settings = SettingsModel(path: folder.appendingPathComponent("settings.toml").path)
        let missions = ["a30", "bloodgulch", "a10"].map { URL(fileURLWithPath: "/maps/\($0).map") }
        let model = MenuModel(maps: missions, settings: settings, servers: ServerListModel { _ in FakeSource() })
        let actions = FakeActions()
        model.actions = actions
        XCTAssertEqual(model.missionNames, ["a10", "a30"], "in the campaign's order, multiplayer maps left out")
        model.difficulty = 3
        model.playCampaign(mission: "a30")
        model.playCampaign(mission: "bloodgulch")
        XCTAssertEqual(actions.done, ["campaign a30.map 3"])
        XCTAssertEqual(model.message, "There is no mission called bloodgulch.")
    }

    func testPlayingAloneLoadsTheMap() {
        let (model, actions) = menu()
        model.playAlone(map: "HangEmHigh")
        model.playAlone(map: "nowhere")
        XCTAssertEqual(actions.done, ["alone hangemhigh.map"])
        XCTAssertEqual(model.message, "There is no map called nowhere.")
        actions.fails = true
        model.playAlone(map: "bloodgulch")
        XCTAssertNotNil(model.message, "the app's error is shown")
    }

    func testTheServerListIsSortedAndJoinsGamesWithRoomOnMapsItHas() {
        let source = FakeSource()
        let (model, actions) = menu(source: source)
        model.show(.servers)
        source.answers = [server("zeta", "10.0.0.2:2302"), server("Alpha", "10.0.0.3:2302", players: 16),
                          server("beta", "10.0.0.4:2302", level: "levels\\test\\icefields\\icefields")]
        model.tick()
        XCTAssertEqual(model.servers.servers.map(\.name), ["Alpha", "beta", "zeta"])
        XCTAssertEqual(gameTypeTitle(model.servers.servers[0].gameType), "Slayer")
        model.servers.selected = "10.0.0.2:2302"
        XCTAssertEqual(model.servers.chosen?.name, "zeta")
        model.join(model.servers.servers[0])
        XCTAssertEqual(model.message, "Alpha is full.")
        model.join(model.servers.servers[1])
        XCTAssertEqual(model.message, "beta plays icefields, which is not in your maps.")
        model.join(model.servers.servers[2])
        XCTAssertEqual(actions.done, ["join 10.0.0.2:2302 bloodgulch.map Spartan"])
        source.answers = []
        model.tick()
        XCTAssertNil(model.servers.selected, "a game gone is not chosen")
    }

    func testAddressesAreParsed() {
        XCTAssertEqual(JoinAddress.parse("192.168.1.20"), JoinAddress(host: "192.168.1.20", port: 2302))
        XCTAssertEqual(JoinAddress.parse(" halo.example:2303 "), JoinAddress(host: "halo.example", port: 2303))
        XCTAssertEqual(JoinAddress.parse("[::1]:2400"), JoinAddress(host: "::1", port: 2400))
        XCTAssertEqual(JoinAddress.parse("fe80::1"), JoinAddress(host: "fe80::1", port: 2302))
        XCTAssertEqual(JoinAddress.parse("[::1]")?.text, "[::1]:2302")
        XCTAssertEqual(JoinAddress.parse("10.0.0.1")?.text, "10.0.0.1:2302")
        for bad in ["", ":2302", "host:0", "host:65536", "host:x", "a b", "[::1", "[::1]2302", "host:２３"] {
            XCTAssertNil(JoinAddress.parse(bad), bad)
        }
    }

    func testJoiningByAddressAsksAndJoinsWhenItAnswers() {
        let source = FakeSource()
        var asked: [[String]] = []
        let settings = SettingsModel(path: folder.appendingPathComponent("settings.toml").path)
        let model = MenuModel(maps: maps, settings: settings, servers: ServerListModel { asked.append($0); return source })
        let actions = FakeActions()
        model.actions = actions
        model.address = "nonsense:"
        model.joinAddress()
        XCTAssertEqual(model.message, "nonsense: is not an address.")
        model.address = "10.0.0.9"
        model.joinAddress()
        XCTAssertEqual(asked, [["10.0.0.9:2302"]])
        model.tick()
        XCTAssertNotNil(model.asking)
        source.answers = [server("Gulch", "10.0.0.9:2302")]
        model.tick()
        XCTAssertNil(model.asking)
        XCTAssertEqual(actions.done, ["join 10.0.0.9:2302 bloodgulch.map Spartan"])

        source.answers = []
        model.joinAddress()
        for _ in 0..<MenuModel.askTicks {
            model.tick()
        }
        XCTAssertEqual(model.message, "No game answered at 10.0.0.9:2302.")
    }

    func testTheHostSetupIsKeptInRangeAndSaved() throws {
        let (model, actions) = menu()
        XCTAssertEqual(model.host.map, "bloodgulch")
        model.host.maxPlayers = 40
        XCTAssertEqual(model.host.maxPlayers, 16)
        model.host.maxPlayers = 1
        XCTAssertEqual(model.host.maxPlayers, 2)
        model.host.name = String(repeating: "é", count: 20)
        XCTAssertEqual(model.host.name.utf8.count, 32, "cut at a character")
        model.host.kind = .captureTheFlag
        model.host.map = "boardingaction"
        model.host.maxPlayers = 8
        model.startHosting()
        XCTAssertEqual(actions.done, ["host boardingaction.map ctf 8 \(model.host.name) 2302"])
        let saved = loadSettings(path: model.settings.path)
        XCTAssertEqual(saved.game.gameType, "ctf")
        XCTAssertEqual(saved.game.map, "boardingaction")
        XCTAssertEqual(saved.game.maxPlayers, 8)
        XCTAssertEqual(HostSetup(game: saved.game), model.host)
    }

    func testAMapNotThereIsReplacedByTheFirst() {
        var game = stockSettings().game
        game.map = "gone"
        XCTAssertEqual(HostSetup(game: game).map, "gone")
        let settings = SettingsModel(settings: Settings(game: game, controls: stockSettings().controls,
                                                       video: stockSettings().video, audio: stockSettings().audio),
                                     path: folder.appendingPathComponent("s.toml").path)
        let model = MenuModel(maps: maps, settings: settings, servers: ServerListModel { _ in FakeSource() })
        XCTAssertEqual(model.host.map, "bloodgulch")
    }

    func testSettingsAreClampedAndBindingsMove() {
        let (model, _) = menu()
        let settings = model.settings
        settings.mouseSensitivity = 5
        XCTAssertEqual(settings.mouseSensitivity, SettingsModel.mouseSensitivity.upperBound)
        settings.renderScale = 0.1
        XCTAssertEqual(settings.renderScale, 0.5)
        settings.master = .nan
        XCTAssertEqual(settings.master, 0)
        settings.antiAliasing = "fxaa"
        XCTAssertEqual(settings.antiAliasing, "temporal")
        XCTAssertEqual(settings.settings.controls.bindings, [], "the stock keys are not written")
        settings.bind(.jump, to: 13) // (W, forward's)
        XCTAssertEqual(settings.bindings.keys[.jump], [13])
        XCTAssertEqual(settings.bindings.keys[.forward], [])
        XCTAssertEqual(Set(settings.settings.controls.bindings.map(\.action)), ["jump", "forward"])
        settings.reset(.controls)
        XCTAssertEqual(settings.bindings, KeyBindings.stock)
        XCTAssertEqual(KeyNames.names([59, 62]), "Control, Right Control")
        XCTAssertEqual(KeyNames.name(KeyNames.button(1)), "Right Button")
    }

    func testTheSettingsGoThroughTomlAndBack() throws {
        let (model, _) = menu()
        let settings = model.settings
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

        model.show(.settings(.controls))
        model.back()
        XCTAssertTrue(FileManager.default.fileExists(atPath: settings.path), "saved on the way out")
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

    func testQuitSavesAndQuits() {
        let (model, actions) = menu()
        model.quit()
        XCTAssertEqual(actions.done, ["quit"])
        XCTAssertTrue(FileManager.default.fileExists(atPath: model.settings.path))
    }
}
