// The settings screens' model: the engine's settings (settings.toml in
// the app's support folder), edited within their ranges and saved.
import Foundation
import OpenCEEngineCore
import OpenCEEngineKit

extension KeyBindings {
    /// The bindings the settings keep: the actions they name take their
    /// keys, the rest keep the stock ones (unknown actions are skipped).
    public init(settings: [KeyBinding]) {
        var bindings = KeyBindings.stock
        for binding in settings {
            if let action = PlayAction(rawValue: binding.action) {
                bindings.keys[action] = binding.keys
            }
        }
        self = bindings
    }

    /// As the settings keep them: those that are not the stock ones, in
    /// the actions' order.
    public var settings: [KeyBinding] {
        PlayAction.allCases.compactMap { action in
            let keys = self.keys[action] ?? []
            return keys == KeyBindings.stock.keys[action] ? nil : KeyBinding(action: action.rawValue, keys: keys)
        }
    }
}

@MainActor
@Observable
public final class SettingsModel {
    /// The ranges the screens offer.
    public static let mouseSensitivity: ClosedRange<Float> = 0.0002...0.01
    public static let renderScale: ClosedRange<Float> = 0.5...1
    public static let volume: ClosedRange<Float> = 0...1

    public var settings: Settings
    /// Where they are saved.
    public let path: String
    /// Why the last save failed, if it did.
    public private(set) var problem: String?

    /// The settings saved at `path` (the stock ones if there are none).
    public init(path: String = settingsPath()) {
        self.path = path
        settings = loadSettings(path: path)
    }

    public init(settings: Settings, path: String) {
        self.settings = settings
        self.path = path
    }

    /// Saves them; false (with `problem`) if they could not be.
    @discardableResult
    public func save() -> Bool {
        do {
            try saveSettings(settings: settings, path: path)
            problem = nil
            return true
        } catch {
            problem = "\(error)"
            return false
        }
    }

    /// The stock settings of `page`.
    public func reset(_ page: SettingsPage) {
        let stock = stockSettings()
        switch page {
        case .game: settings.game = stock.game
        case .controls: settings.controls = stock.controls
        case .video: settings.video = stock.video
        case .audio: settings.audio = stock.audio
        }
    }

    public var playerName: String {
        get { settings.game.playerName }
        set { settings.game.playerName = fitted(newValue, bytes: maxNameBytes) }
    }

    public var mouseSensitivity: Float {
        get { settings.controls.mouseSensitivity }
        set { settings.controls.mouseSensitivity = clamped(newValue, SettingsModel.mouseSensitivity) }
    }

    public var bindings: KeyBindings {
        get { KeyBindings(settings: settings.controls.bindings) }
        set { settings.controls.bindings = newValue.settings }
    }

    /// Binds `action` to `key` alone, taking the key from any other
    /// action that had it.
    public func bind(_ action: PlayAction, to key: UInt16) {
        var bindings = self.bindings
        for other in PlayAction.allCases {
            bindings.keys[other]?.removeAll { $0 == key }
        }
        bindings.keys[action] = [key]
        self.bindings = bindings
    }

    public var antiAliasing: String {
        get { settings.video.antiAliasing }
        set { settings.video.antiAliasing = ["temporal", "native"].contains(newValue) ? newValue : "temporal" }
    }

    public var renderScale: Float {
        get { settings.video.renderScale }
        set { settings.video.renderScale = clamped(newValue, SettingsModel.renderScale) }
    }

    public var master: Float {
        get { settings.audio.master }
        set { settings.audio.master = clamped(newValue, SettingsModel.volume) }
    }

    public var effects: Float {
        get { settings.audio.effects }
        set { settings.audio.effects = clamped(newValue, SettingsModel.volume) }
    }

    public var music: Float {
        get { settings.audio.music }
        set { settings.audio.music = clamped(newValue, SettingsModel.volume) }
    }
}

func clamped(_ value: Float, _ range: ClosedRange<Float>) -> Float {
    value.isFinite ? min(max(value, range.lowerBound), range.upperBound) : range.lowerBound
}

/// A page of the settings.
public enum SettingsPage: String, CaseIterable, Sendable {
    case game, controls, video, audio

    public var title: String { rawValue.capitalized }
}
