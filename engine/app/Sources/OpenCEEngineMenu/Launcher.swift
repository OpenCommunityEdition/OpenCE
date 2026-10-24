// The launcher: the window before the game, with Play, Settings,
// Developer and Exit only. Its settings are what must be set before the
// game starts: the data folders, the display and the window mode; the rest
// is the game's menus. Developer starts a test map (DeveloperTab.swift).
import AppKit
import Foundation
import OpenCEEngineCore
import OpenCEEngineKit
import SwiftUI

/// What the launcher offers.
public enum LauncherChoice: String, CaseIterable, Sendable {
    case play, settings, developer, exit

    public var title: String { rawValue.capitalized }
}

/// How the game's window shows.
public enum WindowMode: String, CaseIterable, Sendable {
    case windowed, fullScreen

    public var title: String {
        switch self {
        case .windowed: "Windowed"
        case .fullScreen: "Full Screen"
        }
    }
}

@MainActor
@Observable
public final class LauncherModel {
    public let settings: SettingsModel
    /// The screens' names, the main one first.
    public let displays: [String]
    /// The checkout's folder, whose assets/maps the stock folders are.
    public let root: URL
    /// Whether the settings show.
    public var showsSettings = false
    /// Whether the Developer tab shows.
    public var showsDeveloper = false
    /// The Developer tab's launch.
    public let developer: DeveloperModel
    public var onPlay: (() -> Void)?
    /// Starts the game with the Developer tab's arguments.
    public var onLaunch: (([String]) -> Void)?
    public var onExit: (() -> Void)?

    /// `variables` are the shell's, whose drawing switches the Developer
    /// tab sets, and `tuning` its lighting controls.
    public init(settings: SettingsModel, displays: [String], root: URL, variables: [ShellVariable] = [],
                tuning: [TuneControl] = []) {
        self.settings = settings
        self.displays = displays.isEmpty ? ["Main Display"] : displays
        self.root = root
        developer = DeveloperModel(variables: variables, tuning: tuning, root: root)
    }

    public func choose(_ choice: LauncherChoice) {
        switch choice {
        case .play:
            settings.save()
            onPlay?()
        case .settings:
            showsSettings = true
        case .developer:
            showsDeveloper = true
        case .exit:
            settings.save()
            onExit?()
        }
    }

    /// Starts the game with the Developer tab's `arguments`.
    public func launch(_ arguments: [String]) {
        settings.save()
        onLaunch?(arguments)
    }

    /// The folders the game reads its maps from.
    public var folders: [URL] {
        MapLibrary.folders(data: settings.settings.data, root: root)
    }

    /// The Xbox maps' folder; set only to an absolute path (empty for the
    /// stock one).
    public var mapsFolder: String {
        get { settings.settings.data.mapsFolder }
        set { settings.settings.data.mapsFolder = LauncherModel.folder(newValue) }
    }

    public var customMapsFolder: String {
        get { settings.settings.data.customMapsFolder }
        set { settings.settings.data.customMapsFolder = LauncherModel.folder(newValue) }
    }

    /// The screen the game opens on; one no longer there is the main one.
    public var display: Int {
        get {
            let index = Int(settings.settings.video.display)
            return displays.indices.contains(index) ? index : 0
        }
        set { settings.settings.video.display = UInt32(displays.indices.contains(newValue) ? newValue : 0) }
    }

    public var windowMode: WindowMode {
        get { settings.settings.video.fullScreen ? .fullScreen : .windowed }
        set { settings.settings.video.fullScreen = newValue == .fullScreen }
    }

    /// What is missing for the game to start: its main menu's ui.map.
    public var problem: String? {
        guard let maps = folders.first else { return nil }
        let ui = maps.appendingPathComponent("ui.map")
        return FileManager.default.fileExists(atPath: ui.path) ? nil : "There is no ui.map in \(maps.path)."
    }

    /// The stock settings of the launcher's own: the folders, the display
    /// and the window mode.
    public func reset() {
        let stock = stockSettings()
        settings.settings.data = stock.data
        settings.settings.video.display = stock.video.display
        settings.settings.video.fullScreen = stock.video.fullScreen
    }

    static func folder(_ path: String) -> String {
        path.hasPrefix("/") ? (path as NSString).standardizingPath : ""
    }
}

/// The launcher's window: the four choices, and the settings or the
/// Developer tab in their place when chosen.
public struct LauncherView: View {
    @Bindable var model: LauncherModel

    public init(model: LauncherModel) {
        self.model = model
    }

    public var body: some View {
        VStack(spacing: 18) {
            Text("OpenCE").font(MenuFonts.title(44))
            if model.showsSettings {
                LauncherSettings(model: model)
            } else if model.showsDeveloper {
                DeveloperTab(model: model.developer, onLaunch: { model.launch($0) },
                             onDone: { model.showsDeveloper = false })
            } else {
                ForEach(LauncherChoice.allCases, id: \.self) { choice in
                    Button(choice.title) { model.choose(choice) }
                        .font(MenuFonts.heading(18))
                        .frame(width: 220)
                        .keyboardShortcut(choice == .play ? .defaultAction : nil)
                }
                if let problem = model.problem {
                    Text(problem).font(MenuFonts.body(12)).foregroundStyle(.secondary)
                }
            }
        }
        .padding(32)
        .frame(minWidth: 460, maxWidth: .infinity, maxHeight: .infinity)
    }
}

struct LauncherSettings: View {
    @Bindable var model: LauncherModel

    var body: some View {
        Form {
            FolderRow(title: "Maps", path: $model.mapsFolder, stock: "assets/maps")
            FolderRow(title: "Custom maps", path: $model.customMapsFolder, stock: "assets/custom_maps")
            Picker("Display", selection: $model.display) {
                ForEach(model.displays.indices, id: \.self) { index in
                    Text(model.displays[index]).tag(index)
                }
            }
            Picker("Window", selection: $model.windowMode) {
                ForEach(WindowMode.allCases, id: \.self) { mode in
                    Text(mode.title).tag(mode)
                }
            }
            HStack {
                Button("Reset") { model.reset() }
                Spacer()
                Button("Done") {
                    model.settings.save()
                    model.showsSettings = false
                }
                .keyboardShortcut(.defaultAction)
            }
        }
        .font(MenuFonts.body(13))
    }
}

/// A data folder: its path (or the stock one), chosen with the open panel.
struct FolderRow: View {
    let title: String
    @Binding var path: String
    let stock: String

    var body: some View {
        LabeledContent(title) {
            HStack {
                Text(path.isEmpty ? stock : path).lineLimit(1).truncationMode(.middle)
                Button("Choose…") {
                    let panel = NSOpenPanel()
                    panel.canChooseDirectories = true
                    panel.canChooseFiles = false
                    panel.allowsMultipleSelection = false
                    if panel.runModal() == .OK, let url = panel.url {
                        path = url.path
                    }
                }
            }
        }
    }
}
