// The launcher: its four choices, and its settings (the data folders, the
// display and the window mode), kept in range and saved.
import AppKit
import Foundation
import OpenCEEngineCore
import OpenCEEngineKit
@testable import OpenCEEngineMenu
import SwiftUI
import XCTest

@MainActor
final class LauncherTests: XCTestCase {
    var folder: URL!

    override func setUp() async throws {
        folder = FileManager.default.temporaryDirectory.appendingPathComponent("launcher-\(UUID().uuidString)")
    }

    override func tearDown() async throws {
        try? FileManager.default.removeItem(at: folder)
    }

    func launcher(displays: [String] = ["Built-in", "Studio Display"]) -> LauncherModel {
        let settings = SettingsModel(path: folder.appendingPathComponent("settings.toml").path)
        return LauncherModel(settings: settings, displays: displays, root: folder)
    }

    func testThereArePlaySettingsDeveloperAndExitOnly() {
        XCTAssertEqual(LauncherChoice.allCases.map(\.title), ["Play", "Settings", "Developer", "Exit"])
    }

    func testPlayAndExitSaveAndSettingsShowsThem() {
        let model = launcher()
        var done: [String] = []
        model.onPlay = { done.append("play") }
        model.onExit = { done.append("exit") }
        model.choose(.settings)
        XCTAssertTrue(model.showsSettings)
        XCTAssertEqual(done, [])
        model.windowMode = .fullScreen
        model.choose(.play)
        XCTAssertEqual(done, ["play"])
        XCTAssertTrue(loadSettings(path: model.settings.path).video.fullScreen, "saved before the game starts")
        model.choose(.exit)
        XCTAssertEqual(done, ["play", "exit"])
    }

    func testTheFoldersAreAbsoluteOrTheStockOnes() throws {
        let model = launcher()
        XCTAssertEqual(model.folders.map(\.lastPathComponent), ["maps", "custom_maps"])
        XCTAssertEqual(model.folders.first?.deletingLastPathComponent().lastPathComponent, "assets")
        model.mapsFolder = "relative/maps"
        XCTAssertEqual(model.mapsFolder, "", "not absolute: the stock one")
        model.mapsFolder = "/Volumes/Halo/maps/../maps"
        model.customMapsFolder = "/Volumes/Halo/custom"
        XCTAssertEqual(model.folders.map(\.path), ["/Volumes/Halo/maps", "/Volumes/Halo/custom"])
        XCTAssertEqual(model.problem, "There is no ui.map in /Volumes/Halo/maps.")
        let maps = folder.appendingPathComponent("maps")
        try FileManager.default.createDirectory(at: maps, withIntermediateDirectories: true)
        try Data().write(to: maps.appendingPathComponent("ui.map"))
        model.mapsFolder = maps.path
        XCTAssertNil(model.problem)
    }

    func testTheDisplayIsOneThereAndResetIsTheStockOnes() {
        let model = launcher()
        XCTAssertEqual(model.display, 0)
        model.display = 1
        XCTAssertEqual(model.settings.settings.video.display, 1)
        model.display = 5
        XCTAssertEqual(model.display, 0, "a display not there is the main one")
        model.settings.settings.video.display = 1
        XCTAssertEqual(launcherWith(model.settings, displays: ["Built-in"]).display, 0, "unplugged since")
        model.windowMode = .fullScreen
        model.mapsFolder = "/maps"
        model.settings.settings.game.playerName = "Chief"
        model.reset()
        XCTAssertEqual(model.windowMode, .windowed)
        XCTAssertEqual(model.mapsFolder, "")
        XCTAssertEqual(model.display, 0)
        XCTAssertEqual(model.settings.settings.game.playerName, "Chief", "the game's own settings are the menus'")
        XCTAssertEqual(LauncherModel(settings: model.settings, displays: [], root: folder).displays, ["Main Display"])
    }

    func testTheViewDraws() {
        let model = launcher()
        let view = NSHostingView(rootView: LauncherView(model: model))
        view.frame = NSRect(x: 0, y: 0, width: 460, height: 380)
        view.layoutSubtreeIfNeeded()
        XCTAssertGreaterThan(view.fittingSize.height, 0)
        model.choose(.settings)
        view.layoutSubtreeIfNeeded()
        XCTAssertGreaterThan(view.fittingSize.height, 0)
    }

    func launcherWith(_ settings: SettingsModel, displays: [String]) -> LauncherModel {
        LauncherModel(settings: settings, displays: displays, root: folder)
    }
}
