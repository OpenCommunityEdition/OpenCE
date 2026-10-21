// The names of keys (AppKit's key codes, by the key's place on a US
// keyboard) and mouse buttons, as the controls screen shows them.
import Foundation

public enum KeyNames {
    private static let names: [UInt16: String] = [
        0: "A", 1: "S", 2: "D", 3: "F", 4: "H", 5: "G", 6: "Z", 7: "X", 8: "C", 9: "V", 11: "B", 12: "Q",
        13: "W", 14: "E", 15: "R", 16: "Y", 17: "T", 18: "1", 19: "2", 20: "3", 21: "4", 22: "6", 23: "5",
        24: "=", 25: "9", 26: "7", 27: "-", 28: "8", 29: "0", 30: "]", 31: "O", 32: "U", 33: "[", 34: "I",
        35: "P", 36: "Return", 37: "L", 38: "J", 39: "'", 40: "K", 41: ";", 42: "\\", 43: ",", 44: "/",
        45: "N", 46: "M", 47: ".", 48: "Tab", 49: "Space", 50: "`", 51: "Delete", 53: "Escape",
        54: "Right Command", 55: "Command", 56: "Shift", 57: "Caps Lock", 58: "Option", 59: "Control",
        60: "Right Shift", 61: "Right Option", 62: "Right Control", 96: "F5", 97: "F6", 98: "F7", 99: "F3",
        100: "F8", 101: "F9", 103: "F11", 109: "F10", 111: "F12", 118: "F4", 120: "F2", 122: "F1",
        123: "Left", 124: "Right", 125: "Down", 126: "Up",
        0x100: "Left Button", 0x101: "Right Button", 0x102: "Middle Button", 0x103: "Button 4", 0x104: "Button 5",
    ]

    /// The name of key or button `code`.
    public static func name(_ code: UInt16) -> String {
        names[code] ?? "Key \(code)"
    }

    /// The names of `codes`, joined.
    public static func names(_ codes: [UInt16]) -> String {
        codes.isEmpty ? "None" : codes.map(name).joined(separator: ", ")
    }

    /// The code of AppKit's mouse `buttonNumber` (0 left, 1 right, 2
    /// middle), as the window holds it.
    public static func button(_ buttonNumber: Int) -> UInt16 {
        0x100 + UInt16(min(max(buttonNumber, 0), 4))
    }
}
