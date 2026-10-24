// The launcher's Developer tab: it lists devgulch first, its stations, the
// views and the drawing switches, and Launch hands the game the arguments
// it reads back.
import Foundation
import OpenCEEngineCore
import OpenCEEngineKit
@testable import OpenCEEngineMenu
import OpenCEEngineRender
import SwiftUI
import XCTest

@MainActor
final class DeveloperTests: XCTestCase {
    var folder: URL!

    override func setUp() async throws {
        folder = FileManager.default.temporaryDirectory.appendingPathComponent("developer-\(UUID().uuidString)")
    }

    override func tearDown() async throws {
        try? FileManager.default.removeItem(at: folder)
    }

    func launcher() -> LauncherModel {
        let settings = SettingsModel(path: folder.appendingPathComponent("settings.toml").path)
        return LauncherModel(settings: settings, displays: ["Built-in"], root: folder,
                             variables: Engine().shellVariables())
    }

    func testTheTabListsDevgulchItsStationsTheViewsAndTheSwitches() {
        let model = launcher().developer
        XCTAssertEqual(model.levels, ["devgulch", "devmap"])
        XCTAssertEqual(model.level, "devgulch", "devgulch by default")
        XCTAssertEqual(model.stations.map(\.name),
                       ["weapons", "vehicles", "ai", "effects", "sounds", "shaders", "animation", "hud", "physics"])
        XCTAssertTrue(model.stations.allSatisfy { !$0.help.isEmpty })
        XCTAssertEqual(model.views.first, .frame)
        XCTAssertTrue(model.views.contains(.normals))
        let names = model.switches.map(\.name)
        XCTAssertTrue(names.contains("render.gtao"))
        XCTAssertTrue(names.contains("render.wireframe"))
        XCTAssertFalse(names.contains("render.view"), "the view is its own picker")
        XCTAssertTrue(names.allSatisfy { $0.hasPrefix("render.") })
        model.level = "devmap"
        XCTAssertTrue(model.stations.map(\.name).contains("doors"))
    }

    func testAStationTheLevelHasNotIsLetGo() {
        let model = launcher().developer
        model.station = "ai"
        XCTAssertEqual(model.station, "ai")
        model.level = "devmap"
        XCTAssertNil(model.station, "devmap has no ai station")
        model.station = "nowhere"
        XCTAssertNil(model.station)
    }

    func testOnlyTheSwitchesChangedAreLaunched() throws {
        let model = launcher().developer
        let gtao = try XCTUnwrap(model.switches.first { $0.name == "render.gtao" })
        let wireframe = try XCTUnwrap(model.switches.first { $0.name == "render.wireframe" })
        XCTAssertTrue(model.isOn(gtao))
        XCTAssertFalse(model.isOn(wireframe))
        model.set(gtao, on: false)
        model.set(wireframe, on: true)
        XCTAssertFalse(model.isOn(gtao))
        XCTAssertEqual(model.launch.variables, ["render.gtao": "false", "render.wireframe": "true"])
        model.set(wireframe, on: false)
        XCTAssertEqual(model.launch.variables, ["render.gtao": "false"], "as the shell has it: left out")
    }

    func testLaunchingPassesTheLevelStationViewSwitchesAndCaptureToTheGame() throws {
        let launcher = launcher()
        var launched: [[String]] = []
        launcher.onLaunch = { launched.append($0) }
        let model = launcher.developer
        model.launch.view = .normals
        let gtao = try XCTUnwrap(model.switches.first { $0.name == "render.gtao" })
        model.set(gtao, on: false)
        model.capturesOnLaunch = true
        // (a station's own Launch starts straight at it)
        launcher.launch(model.arguments(at: "vehicles"))
        let capture = folder.appendingPathComponent("assets/captures/devgulch-vehicles.png").path
        XCTAssertEqual(launched, [["--map", "devgulch", "--station", "vehicles", "--view", "normals",
                                   "--set", "render.gtao=false", "--capture", capture]])
        XCTAssertTrue(FileManager.default.fileExists(atPath: launcher.settings.path), "saved before the game starts")
        // the game reads it back
        let read = try XCTUnwrap(DeveloperLaunch(arguments: ["OpenCE Engine", "--frame-times"] + launched[0]))
        XCTAssertEqual(read, model.launch)
        XCTAssertEqual(read.settings.map(\.name), ["render.gtao", "render.view"])
        XCTAssertEqual(read.settings.map(\.value), ["false", "normals"])
        XCTAssertEqual(read.commands, ["station vehicles", "frame \"\(capture)\""])
    }

    func testThePlainLaunchIsDevgulchAtItsStart() {
        let launch = DeveloperLaunch()
        XCTAssertEqual(launch.arguments, ["--map", "devgulch"])
        XCTAssertTrue(launch.settings.isEmpty)
        XCTAssertTrue(launch.commands.isEmpty)
        XCTAssertNil(DeveloperLaunch(arguments: ["OpenCE Engine", "--fly"]), "no --map: not a launch")
        XCTAssertEqual(DeveloperLaunch(arguments: ["--map", "bloodgulch", "--bsp", "0"])?.level, "bloodgulch")
    }

    func testTheTabDrawsAndBackReturnsToTheChoices() {
        let model = launcher()
        model.choose(.developer)
        XCTAssertTrue(model.showsDeveloper)
        let view = NSHostingView(rootView: LauncherView(model: model))
        view.frame = NSRect(x: 0, y: 0, width: 620, height: 640)
        view.layoutSubtreeIfNeeded()
        XCTAssertGreaterThan(view.fittingSize.height, 0)
    }
}
