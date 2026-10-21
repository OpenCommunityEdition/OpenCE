// The menus, drawn over the engine's view: a panel on the left with the
// screen's choices, worked with the mouse (and Return and Escape), in
// Overpass, the titles in the OpenCE font.
import AppKit
import OpenCEEngineCore
import OpenCEEngineKit
import SwiftUI

/// The menus' colours: Halo's blue on a dark glass.
enum MenuStyle {
    static let accent = Color(red: 0.42, green: 0.71, blue: 0.95)
    static let text = Color(white: 0.93)
    static let faint = Color(white: 0.62)
    static let panel = Color(red: 0.03, green: 0.06, blue: 0.10).opacity(0.86)
    static let row = Color.white.opacity(0.06)
    static let chosen = accent.opacity(0.28)
}

public struct MenuView: View {
    @Bindable var model: MenuModel

    public init(model: MenuModel) {
        self.model = model
    }

    public var body: some View {
        HStack(spacing: 0) {
            VStack(alignment: .leading, spacing: 18) {
                Text(model.screen == .main ? "OpenCE" : model.screen.title.uppercased())
                    .font(model.screen == .main ? MenuFonts.title(64) : MenuFonts.title(34))
                    .foregroundStyle(MenuStyle.accent)
                screen
                    .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
                if let message = model.message {
                    Text(message)
                        .font(MenuFonts.body(14))
                        .foregroundStyle(Color(red: 1, green: 0.55, blue: 0.45))
                        .fixedSize(horizontal: false, vertical: true)
                }
                if model.screen != .main {
                    MenuButton("Back", shortcut: .cancelAction) { model.back() }
                }
            }
            .padding(36)
            .frame(width: 560)
            .frame(maxHeight: .infinity)
            .background(MenuStyle.panel)
            Spacer(minLength: 0)
        }
        .foregroundStyle(MenuStyle.text)
        .font(MenuFonts.body(16))
        .task {
            // (four times a second: the server list's answers)
            while !Task.isCancelled {
                model.tick()
                try? await Task.sleep(for: .milliseconds(250))
            }
        }
    }

    @ViewBuilder
    private var screen: some View {
        switch model.screen {
        case .main: MainScreen(model: model)
        case .campaign: CampaignScreen(model: model)
        case .solo: SoloScreen(model: model)
        case .servers: ServersScreen(model: model, list: model.servers)
        case .join: JoinScreen(model: model)
        case .host: HostScreen(model: model)
        case let .settings(page): SettingsScreen(model: model, settings: model.settings, page: page)
        }
    }
}

/// A menu's button: its words large, lit under the mouse.
struct MenuButton: View {
    let title: String
    var shortcut: KeyboardShortcut?
    var enabled = true
    let action: () -> Void
    @State private var hovered = false

    init(_ title: String, shortcut: KeyboardShortcut? = nil, enabled: Bool = true, action: @escaping () -> Void) {
        self.title = title
        self.shortcut = shortcut
        self.enabled = enabled
        self.action = action
    }

    var body: some View {
        Button(action: action) {
            Text(title.uppercased())
                .font(MenuFonts.heading(22))
                .foregroundStyle(enabled ? (hovered ? MenuStyle.accent : MenuStyle.text) : MenuStyle.faint)
                .frame(maxWidth: .infinity, alignment: .leading)
                .contentShape(Rectangle())
        }
        .buttonStyle(.plain)
        .disabled(!enabled)
        .onHover { hovered = $0 }
        .keyboardShortcut(shortcut)
    }
}

/// A setting's row: its name, then its control.
struct SettingRow<Control: View>: View {
    let name: String
    @ViewBuilder let control: Control

    var body: some View {
        HStack {
            Text(name).foregroundStyle(MenuStyle.faint)
            Spacer()
            control.frame(width: 260, alignment: .trailing)
        }
    }
}

struct MainScreen: View {
    let model: MenuModel

    var body: some View {
        VStack(alignment: .leading, spacing: 14) {
            MenuButton("Campaign") { model.show(.campaign) }
            MenuButton("Play Alone") { model.show(.solo) }
            MenuButton("Join a Game") { model.show(.servers) }
            MenuButton("Join by Address") { model.show(.join) }
            MenuButton("Host a Game") { model.show(.host) }
            MenuButton("Settings") { model.show(.settings(.game)) }
            MenuButton("Quit") { model.quit() }
        }
    }
}

struct CampaignScreen: View {
    @Bindable var model: MenuModel

    var body: some View {
        if model.missionNames.isEmpty {
            Text("No missions: put a10.map and the others in assets/maps.").foregroundStyle(MenuStyle.faint)
        } else {
            VStack(alignment: .leading, spacing: 14) {
                SettingRow(name: "Difficulty") {
                    Picker("", selection: $model.difficulty) {
                        ForEach(Array(MenuModel.difficulties.enumerated()), id: \.offset) { index, name in
                            Text(name).tag(UInt8(index))
                        }
                    }
                    .labelsHidden()
                }
                ScrollView {
                    VStack(alignment: .leading, spacing: 10) {
                        ForEach(model.missionNames, id: \.self) { name in
                            MenuButton(name) { model.playCampaign(mission: name) }
                        }
                    }
                }
            }
        }
    }
}

struct SoloScreen: View {
    let model: MenuModel

    var body: some View {
        if model.maps.isEmpty {
            Text("No maps: put them in assets/maps or assets/custom_maps.").foregroundStyle(MenuStyle.faint)
        } else {
            ScrollView {
                VStack(alignment: .leading, spacing: 10) {
                    ForEach(model.mapNames, id: \.self) { name in
                        MenuButton(name) { model.playAlone(map: name) }
                    }
                }
            }
        }
    }
}

struct ServersScreen: View {
    let model: MenuModel
    let list: ServerListModel

    var body: some View {
        VStack(alignment: .leading, spacing: 10) {
            HStack {
                Text("Name").frame(maxWidth: .infinity, alignment: .leading)
                Text("Map").frame(width: 110, alignment: .leading)
                Text("Game").frame(width: 120, alignment: .leading)
                Text("Players").frame(width: 60, alignment: .trailing)
            }
            .font(MenuFonts.heading(13))
            .foregroundStyle(MenuStyle.faint)
            ScrollView {
                VStack(spacing: 4) {
                    ForEach(list.servers, id: \.address) { server in
                        row(server)
                    }
                }
            }
            if list.servers.isEmpty {
                Text(list.problem ?? "Looking for games on this network…").foregroundStyle(MenuStyle.faint)
            }
            HStack(spacing: 24) {
                MenuButton("Join", shortcut: .defaultAction, enabled: list.chosen != nil) {
                    if let chosen = list.chosen { model.join(chosen) }
                }
                MenuButton("Refresh") { list.search() }
            }
        }
    }

    private func row(_ server: ServerInfo) -> some View {
        HStack {
            Text(server.name).frame(maxWidth: .infinity, alignment: .leading)
            Text(server.mapName).frame(width: 110, alignment: .leading)
            Text(gameTypeTitle(server.gameType)).frame(width: 120, alignment: .leading)
            Text("\(server.players)/\(server.capacity)").frame(width: 60, alignment: .trailing)
        }
        .lineLimit(1)
        .padding(.vertical, 6)
        .padding(.horizontal, 8)
        .foregroundStyle(server.hasRoom ? MenuStyle.text : MenuStyle.faint)
        .background(list.selected == server.address ? MenuStyle.chosen : MenuStyle.row)
        .contentShape(Rectangle())
        .onTapGesture(count: 2) { model.join(server) }
        .onTapGesture { list.selected = server.address }
    }
}

struct JoinScreen: View {
    @Bindable var model: MenuModel

    var body: some View {
        VStack(alignment: .leading, spacing: 16) {
            Text("The host's address, and its port if not 2302.").foregroundStyle(MenuStyle.faint)
            TextField("192.168.1.20:2302", text: $model.address)
                .textFieldStyle(.roundedBorder)
                .font(MenuFonts.body(18))
                .onSubmit { model.joinAddress() }
            if let asking = model.asking {
                Text("Asking \(asking.address)…").foregroundStyle(MenuStyle.faint)
            }
            MenuButton("Join", shortcut: .defaultAction, enabled: model.asking == nil) { model.joinAddress() }
        }
    }
}

struct HostScreen: View {
    @Bindable var model: MenuModel

    var body: some View {
        VStack(alignment: .leading, spacing: 16) {
            SettingRow(name: "Map") {
                Picker("", selection: $model.host.map) {
                    ForEach(model.mapNames, id: \.self) { Text($0).tag($0) }
                }
                .labelsHidden()
            }
            SettingRow(name: "Game") {
                Picker("", selection: $model.host.kind) {
                    ForEach(GameKind.all, id: \.self) { Text($0.title).tag($0) }
                }
                .labelsHidden()
            }
            SettingRow(name: "Players") {
                Stepper("\(model.host.maxPlayers)", value: $model.host.maxPlayers, in: HostSetup.players)
            }
            SettingRow(name: "Name") {
                TextField("Halo", text: $model.host.name).textFieldStyle(.roundedBorder)
            }
            MenuButton("Host", shortcut: .defaultAction, enabled: !model.maps.isEmpty) { model.startHosting() }
        }
    }
}

struct SettingsScreen: View {
    let model: MenuModel
    @Bindable var settings: SettingsModel
    let page: SettingsPage

    var body: some View {
        VStack(alignment: .leading, spacing: 18) {
            HStack(spacing: 18) {
                ForEach(SettingsPage.allCases, id: \.self) { other in
                    Button(other.title.uppercased()) { model.show(.settings(other)) }
                        .buttonStyle(.plain)
                        .font(MenuFonts.heading(15))
                        .foregroundStyle(other == page ? MenuStyle.accent : MenuStyle.faint)
                }
            }
            ScrollView {
                VStack(alignment: .leading, spacing: 14) {
                    switch page {
                    case .game: game
                    case .controls: ControlsPage(settings: settings)
                    case .video: video
                    case .audio: audio
                    }
                }
            }
            MenuButton("Stock Settings") { settings.reset(page) }
            if let problem = settings.problem {
                Text(problem).foregroundStyle(MenuStyle.faint)
            }
        }
    }

    @ViewBuilder private var game: some View {
        SettingRow(name: "Player name") {
            TextField("Spartan", text: $settings.playerName).textFieldStyle(.roundedBorder)
        }
        SettingRow(name: "Port to host on") {
            TextField("2302", value: $settings.settings.game.port, format: .number.grouping(.never))
                .textFieldStyle(.roundedBorder)
        }
    }

    @ViewBuilder private var video: some View {
        SettingRow(name: "Smoothing") {
            Picker("", selection: $settings.antiAliasing) {
                Text("Temporal").tag("temporal")
                Text("Native").tag("native")
            }
            .labelsHidden()
        }
        SettingRow(name: "Render scale") {
            Slider(value: $settings.renderScale, in: SettingsModel.renderScale, step: 0.05)
        }
        SettingRow(name: "Full screen") { Toggle("", isOn: $settings.settings.video.fullScreen).labelsHidden() }
        SettingRow(name: "Frame times") {
            Toggle("", isOn: $settings.settings.video.showFrameTimes).labelsHidden()
        }
        Text("Smoothing and the render scale take effect on the next map.").foregroundStyle(MenuStyle.faint)
    }

    @ViewBuilder private var audio: some View {
        SettingRow(name: "Master") { Slider(value: $settings.master, in: SettingsModel.volume) }
        SettingRow(name: "Effects") { Slider(value: $settings.effects, in: SettingsModel.volume) }
        SettingRow(name: "Music") { Slider(value: $settings.music, in: SettingsModel.volume) }
    }
}

/// The controls: the mouse, and each action's key, changed by clicking it
/// and pressing the new key or mouse button (Escape keeps the old).
struct ControlsPage: View {
    @Bindable var settings: SettingsModel
    @State private var listener = KeyListener()

    var body: some View {
        SettingRow(name: "Mouse sensitivity") {
            Slider(value: $settings.mouseSensitivity, in: SettingsModel.mouseSensitivity)
        }
        SettingRow(name: "Invert the mouse") {
            Toggle("", isOn: $settings.settings.controls.invertMouse).labelsHidden()
        }
        ForEach(PlayAction.allCases, id: \.self) { action in
            SettingRow(name: action.title) {
                Button(listener.action == action ? "Press a key…" : KeyNames.names(settings.bindings.keys[action] ?? [])) {
                    listener.listen(for: action) { settings.bind(action, to: $0) }
                }
                .buttonStyle(.plain)
                .foregroundStyle(listener.action == action ? MenuStyle.accent : MenuStyle.text)
            }
        }
        .onDisappear { listener.stop() }
    }
}

extension PlayAction {
    /// Its name on the controls screen.
    var title: String {
        switch self {
        case .forward: "Move forward"
        case .back: "Move back"
        case .left: "Move left"
        case .right: "Move right"
        case .jump: "Jump"
        case .crouch: "Crouch"
        case .fire: "Fire"
        case .toggleView: "First or third person"
        case .reload: "Reload"
        case .swap: "Swap weapons"
        case .action: "Action"
        case .melee: "Melee"
        case .grenade: "Throw a grenade"
        case .zoom: "Zoom"
        case .switchGrenade: "Switch grenades"
        }
    }
}

/// Reads the next key or mouse button pressed, for one action.
@MainActor
@Observable
final class KeyListener {
    private(set) var action: PlayAction?
    @ObservationIgnored private var monitor: Any?
    @ObservationIgnored private var bound: ((UInt16) -> Void)?
    /// (the click that started it is not the new button)
    @ObservationIgnored private var started = Date.distantPast

    func listen(for action: PlayAction, _ bind: @escaping (UInt16) -> Void) {
        stop()
        self.action = action
        bound = bind
        started = Date()
        monitor = NSEvent.addLocalMonitorForEvents(
            matching: [.keyDown, .flagsChanged, .leftMouseDown, .rightMouseDown, .otherMouseDown]
        ) { [weak self] event in
            // (read here: the event is not to be sent to the main actor)
            let pressed = Pressed(type: event.type, keyCode: event.keyCode, buttonNumber: event.buttonNumber,
                                  modifiers: !event.modifierFlags.intersection(.deviceIndependentFlagsMask).isEmpty)
            let taken = MainActor.assumeIsolated { self?.take(pressed) ?? false }
            return taken ? nil : event
        }
    }

    func stop() {
        if let monitor {
            NSEvent.removeMonitor(monitor)
        }
        monitor = nil
        bound = nil
        action = nil
    }

    /// What of an event it reads.
    struct Pressed: Sendable {
        let type: NSEvent.EventType
        let keyCode: UInt16
        let buttonNumber: Int
        /// Whether a modifier is held (a modifier's key pressed, not let go).
        let modifiers: Bool
    }

    /// Takes the press; true if it was the new key.
    private func take(_ pressed: Pressed) -> Bool {
        switch pressed.type {
        case .keyDown:
            if pressed.keyCode != 53 {
                bound?(pressed.keyCode)
            }
        case .flagsChanged:
            guard pressed.modifiers else { return true }
            bound?(pressed.keyCode)
        default:
            guard Date().timeIntervalSince(started) > 0.2 else { return false }
            bound?(KeyNames.button(pressed.buttonNumber))
        }
        stop()
        return true
    }
}
