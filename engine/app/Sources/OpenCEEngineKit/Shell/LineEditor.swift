// The developer shell's line as it is typed: its characters, the cursor,
// and the text last killed, with Emacs's keys as the macOS text fields
// have them (Control-A, -E, -K, -U, -W and -Y; Option and the arrows by
// words).

/// The shell's line being typed.
public struct LineEditor: Equatable, Sendable {
    /// The longest line.
    public static let maxLength = 1024

    public private(set) var characters: [Character] = []
    /// Where the next character goes: 0 is before the first.
    public private(set) var cursor = 0
    /// What the last kill took, which yank puts back.
    public private(set) var killed = ""

    public init(_ text: String = "") {
        set(text)
    }

    public var text: String { String(characters) }

    /// Replaces the line, the cursor at its end.
    public mutating func set(_ text: String) {
        characters = Array(Self.printable(text).prefix(Self.maxLength))
        cursor = characters.count
    }

    /// Types `text` at the cursor: only its printable characters, a line
    /// break as a space, and no more than the line has room for.
    public mutating func insert(_ text: String) {
        let room = Self.maxLength - characters.count
        let typed = Array(Self.printable(text).prefix(max(0, room)))
        characters.insert(contentsOf: typed, at: cursor)
        cursor += typed.count
    }

    public mutating func backspace() {
        guard cursor > 0 else { return }
        cursor -= 1
        characters.remove(at: cursor)
    }

    public mutating func deleteForward() {
        guard cursor < characters.count else { return }
        characters.remove(at: cursor)
    }

    public mutating func left() { cursor = max(0, cursor - 1) }
    public mutating func right() { cursor = min(characters.count, cursor + 1) }
    public mutating func home() { cursor = 0 }
    public mutating func end() { cursor = characters.count }

    public mutating func wordLeft() { cursor = wordStart(before: cursor) }

    public mutating func wordRight() {
        var index = cursor
        while index < characters.count, !Self.inWord(characters[index]) { index += 1 }
        while index < characters.count, Self.inWord(characters[index]) { index += 1 }
        cursor = index
    }

    /// Control-K: kills from the cursor to the end.
    public mutating func killToEnd() {
        kill(cursor..<characters.count)
    }

    /// Control-U: kills from the start to the cursor.
    public mutating func killToStart() {
        kill(0..<cursor)
    }

    /// Control-W: kills the word before the cursor.
    public mutating func killWordBack() {
        kill(wordStart(before: cursor)..<cursor)
    }

    /// Control-Y: puts back what was last killed.
    public mutating func yank() {
        insert(killed)
    }

    private mutating func kill(_ range: Range<Int>) {
        guard !range.isEmpty else { return }
        killed = String(characters[range])
        characters.removeSubrange(range)
        cursor = range.lowerBound
    }

    private func wordStart(before position: Int) -> Int {
        var index = position
        while index > 0, !Self.inWord(characters[index - 1]) { index -= 1 }
        while index > 0, Self.inWord(characters[index - 1]) { index -= 1 }
        return index
    }

    /// A word's characters: letters, digits, `_`, `-` and `.` (HaloScript's
    /// names and numbers are words).
    static func inWord(_ character: Character) -> Bool {
        character.isLetter || character.isNumber || "_-.".contains(character)
    }

    /// `text` with line breaks and tabs as spaces, and other control
    /// characters left out.
    static func printable(_ text: String) -> String {
        var result = ""
        for character in text {
            if character.isNewline || character == "\t" {
                result.append(" ")
            } else if character.unicodeScalars.allSatisfy({ $0.value >= 0x20 && !(0x7F...0x9F).contains($0.value) }) {
                result.append(character)
            }
        }
        return result
    }
}
