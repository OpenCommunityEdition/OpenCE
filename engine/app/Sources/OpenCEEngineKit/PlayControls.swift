// The player's controls: the keys held and the mouse's raw motion since the
// last frame, as the engine's input, through key bindings the player may
// change (a JSON file of action names to AppKit key codes). The stock
// bindings are Halo PC's: W, S, A and D move, Space jumps, Control
// crouches, the mouse looks, its left button fires and its right throws a
// grenade; R reloads, Tab changes weapons, E picks up, F strikes, Z zooms
// G changes grenades and Q the flashlight. V changes the camera between first and third
// person. The mouse's buttons are bound by codes past the keys' (from 256:
// left, right, middle, fourth, fifth).
import Foundation
import OpenCEEngineCore

/// What a key may be bound to.
public enum PlayAction: String, CaseIterable, Codable, Sendable {
    case forward, back, left, right, jump, crouch, fire, toggleView
    case reload, swap, action, melee, grenade, zoom, switchGrenade, flashlight
}

/// The codes of the mouse's buttons (the window's `MouseButton`).
public enum MouseButtonCode {
    public static let left: UInt16 = 0x100
    public static let right: UInt16 = 0x101
}

/// Each action's keys (AppKit's key codes, by the key's place).
public struct KeyBindings: Equatable, Sendable {
    public var keys: [PlayAction: [UInt16]]

    public init(keys: [PlayAction: [UInt16]]) {
        self.keys = keys
    }

    /// Halo PC's.
    public static let stock = KeyBindings(keys: [
        .forward: [13],  // W
        .back: [1],  // S
        .left: [0],  // A
        .right: [2],  // D
        .jump: [49],  // Space
        .crouch: [59, 62],  // the Controls
        .fire: [MouseButtonCode.left],
        .toggleView: [9],  // V
        .reload: [15],  // R
        .swap: [48],  // Tab
        .action: [14],  // E
        .melee: [3],  // F
        .grenade: [MouseButtonCode.right],
        .zoom: [6],  // Z
        .switchGrenade: [5],  // G
        .flashlight: [12],  // Q
    ])

    /// Bindings read from JSON (`{"jump": [49], ...}`): the actions it
    /// names take its keys, the rest keep the stock ones. Unknown actions
    /// are an error.
    public static func read(_ data: Data) throws -> KeyBindings {
        let named = try JSONDecoder().decode([String: [UInt16]].self, from: data)
        var bindings = KeyBindings.stock
        for (name, keys) in named {
            guard let action = PlayAction(rawValue: name) else {
                throw BindingError.unknownAction(name)
            }
            bindings.keys[action] = keys
        }
        return bindings
    }

    /// The JSON `read` takes.
    public func written() throws -> Data {
        let named = Dictionary(uniqueKeysWithValues: keys.map { ($0.key.rawValue, $0.value) })
        let encoder = JSONEncoder()
        encoder.outputFormatting = [.prettyPrinted, .sortedKeys]
        return try encoder.encode(named)
    }

    /// Whether any of `action`'s keys is among `held`.
    public func holds(_ action: PlayAction, _ held: Set<UInt16>) -> Bool {
        (keys[action] ?? []).contains(where: held.contains)
    }
}

public enum BindingError: Error, Equatable {
    case unknownAction(String)
}

/// The keys and mouse as the player's input.
public struct PlayControls: Equatable, Sendable {
    public var bindings: KeyBindings
    /// The turn for a count of the mouse's motion, in radians.
    public var sensitivity: Float
    /// Whether moving the mouse up looks down.
    public var invertPitch: Bool

    public init(bindings: KeyBindings = .stock, sensitivity: Float = 0.0012, invertPitch: Bool = false) {
        self.bindings = bindings
        self.sensitivity = sensitivity
        self.invertPitch = invertPitch
    }

    /// The input for the keys `held` and the mouse's motion `mouse` (right
    /// and up positive, in counts).
    public func input(held: Set<UInt16>, mouse: (dx: Double, dy: Double)) -> InputState {
        func axis(_ positive: PlayAction, _ negative: PlayAction) -> Int8 {
            let plus = bindings.holds(positive, held)
            let minus = bindings.holds(negative, held)
            // (both held cancel)
            return plus == minus ? 0 : (plus ? 127 : -127)
        }
        func turn(_ counts: Double) -> Float {
            let radians = Float(counts) * sensitivity
            return radians.isFinite ? radians : 0
        }
        let names = inputButtons()
        var buttons: UInt32 = 0
        let actions: [(PlayAction, UInt32)] = [
            (.jump, names.jump), (.crouch, names.crouch), (.fire, names.fire),
            (.reload, names.reload), (.swap, names.swap), (.action, names.action),
            (.melee, names.melee), (.grenade, names.grenade), (.zoom, names.zoom),
            (.switchGrenade, names.switchGrenade), (.flashlight, names.flashlight),
        ]
        for (action, button) in actions where bindings.holds(action, held) {
            buttons |= button
        }
        // (the mouse right turns right, which is a negative yaw)
        return InputState(
            buttons: buttons,
            forward: axis(.forward, .back),
            right: axis(.right, .left),
            up: 0,
            yaw: -turn(mouse.dx),
            pitch: invertPitch ? -turn(mouse.dy) : turn(mouse.dy))
    }

    /// Whether the view changes this frame: a toggle key pressed since the
    /// last frame's `earlier` keys.
    public func togglesView(held: Set<UInt16>, earlier: Set<UInt16>) -> Bool {
        bindings.holds(.toggleView, held) && !bindings.holds(.toggleView, earlier)
    }
}
