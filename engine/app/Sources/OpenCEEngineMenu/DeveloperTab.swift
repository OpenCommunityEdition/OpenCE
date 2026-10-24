// The launcher's Developer tab: a test map (devgulch first), the station
// to start at, the buffer to view and the drawing switches as the game
// starts, and a frame captured once it has; Launch starts it, and each
// station's own Launch starts straight at that station.
import Foundation
import OpenCEEngineCore
import OpenCEEngineKit
import OpenCEEngineRender
import SwiftUI

@MainActor
@Observable
public final class DeveloperModel {
    /// What Launch starts.
    public var launch = DeveloperLaunch()
    /// The drawing switches (the shell's `render.*` flags the renderer
    /// reads, RenderSwitches.flags), as the shell holds them now.
    public let switches: [ShellVariable]
    /// Where captures are written: the data root's assets/captures.
    public let captures: URL

    /// `variables` are the shell's; those of its `render.*` flags the
    /// renderer reads are the switches.
    public init(variables: [ShellVariable], root: URL) {
        switches = variables.filter { RenderSwitches.flags.contains($0.name) }
        captures = root.appendingPathComponent("assets/captures", isDirectory: true)
    }

    public var levels: [String] { MapLibrary.developerMaps }

    public var views: [RenderView] { RenderView.allCases }

    /// The level's stations.
    public var stations: [TestStation] { testStations(map: launch.level) }

    /// The test map; a station it does not have is let go.
    public var level: String {
        get { launch.level }
        set {
            launch.level = newValue
            if let station = launch.station, !stations.contains(where: { $0.name == station }) {
                launch.station = nil
            }
            if capturesOnLaunch {
                capturesOnLaunch = true
            }
        }
    }

    /// The station started at (none: the level's start).
    public var station: String? {
        get { launch.station }
        set {
            launch.station = newValue.flatMap { name in stations.contains { $0.name == name } ? name : nil }
            if capturesOnLaunch {
                capturesOnLaunch = true
            }
        }
    }

    /// Whether `switch` is on at start.
    public func isOn(_ variable: ShellVariable) -> Bool {
        (launch.variables[variable.name] ?? variable.value) == "true"
    }

    /// Sets `switch` on or off at start; as the shell has it, it is left
    /// out of the launch.
    public func set(_ variable: ShellVariable, on: Bool) {
        let value = on ? "true" : "false"
        launch.variables[variable.name] = value == variable.value ? nil : value
    }

    /// Whether the first frame is captured, to captures/<level>-<station>.png.
    public var capturesOnLaunch: Bool {
        get { launch.capture != nil }
        set {
            let name = "\(launch.level)-\(launch.station ?? "start").png"
            launch.capture = newValue ? captures.appendingPathComponent(name).path : nil
        }
    }

    /// The arguments to launch with, at `station` when one is given.
    public func arguments(at station: String? = nil) -> [String] {
        if let station {
            self.station = station
        }
        return launch.arguments
    }
}

/// The tab: the level, the station, the view, the switches, the capture,
/// and Launch.
struct DeveloperTab: View {
    @Bindable var model: DeveloperModel
    let onLaunch: ([String]) -> Void
    let onDone: () -> Void

    var body: some View {
        VStack(spacing: 12) {
            form
            HStack {
                Button("Back") { onDone() }
                Spacer()
                Button("Launch") { onLaunch(model.arguments()) }
                    .keyboardShortcut(.defaultAction)
            }
        }
        .font(MenuFonts.body(13))
    }

    /// The choices; it scrolls when the window is shorter than they are,
    /// Back and Launch staying below it.
    private var form: some View {
        Form {
            Picker("Level", selection: $model.level) {
                ForEach(model.levels, id: \.self) { Text($0).tag($0) }
            }
            Picker("View", selection: $model.launch.view) {
                ForEach(model.views, id: \.self) { Text($0.rawValue).tag($0) }
            }
            Section("Stations") {
                Picker("Start at", selection: $model.station) {
                    Text("the level's start").tag(String?.none)
                    ForEach(model.stations, id: \.name) { Text($0.name).tag(String?.some($0.name)) }
                }
                ForEach(model.stations, id: \.name) { station in
                    HStack {
                        Text(station.name).frame(width: 80, alignment: .leading)
                        Text(station.help).font(MenuFonts.body(11)).foregroundStyle(.secondary).lineLimit(1)
                        Spacer()
                        Button("Launch") { onLaunch(model.arguments(at: station.name)) }
                    }
                }
            }
            Section("Switches") {
                ForEach(model.switches, id: \.name) { variable in
                    Toggle(variable.name, isOn: Binding(get: { model.isOn(variable) },
                                                        set: { model.set(variable, on: $0) }))
                        .help(variable.help)
                }
            }
            Toggle("Capture the first frame", isOn: $model.capturesOnLaunch)
        }
        .formStyle(.grouped)
        .frame(minWidth: 400, maxWidth: .infinity, minHeight: 200, maxHeight: .infinity)
    }
}
