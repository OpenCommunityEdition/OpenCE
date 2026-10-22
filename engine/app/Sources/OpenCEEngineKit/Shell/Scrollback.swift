// The developer shell's scrollback: the lines typed, their output and the
// log's, each of a kind its colour comes from, wrapped at the overlay's
// width, scrolled by rows from the bottom, and selected with the mouse
// for copying.

/// The shell's lines.
public struct Scrollback: Equatable, Sendable {
    /// The most lines kept.
    public static let capacity = 2000
    /// The longest line kept (longer ones are cut, marked with …).
    public static let maxLineLength = 4096

    public enum Kind: Equatable, Sendable {
        /// A line typed.
        case input
        /// A command's output.
        case output
        /// A command's error.
        case error
        /// The log's lines, by level.
        case log(LogLevel)
    }

    public enum LogLevel: String, Equatable, Sendable, CaseIterable {
        case error, warn, info, debug, trace
    }

    public struct Line: Equatable, Sendable {
        public var text: [Character]
        public var kind: Kind
    }

    /// A line's row at a width: its line's number, where in it the row
    /// starts, and its characters.
    public struct Row: Equatable, Sendable {
        public var line: Int
        public var start: Int
        public var text: String
        public var length: Int
        public var kind: Kind
    }

    /// A place between two characters of a line, by its line's number
    /// (which counts lines dropped from the top too).
    public struct Position: Comparable, Sendable {
        public var line: Int
        public var offset: Int

        public init(line: Int, offset: Int) {
            self.line = line
            self.offset = offset
        }

        public static func < (a: Position, b: Position) -> Bool {
            (a.line, a.offset) < (b.line, b.offset)
        }
    }

    public private(set) var lines: [Line] = []
    /// How many lines have been dropped from the top (the first kept
    /// line's number).
    public private(set) var dropped = 0
    /// How many rows up from the bottom the view is.
    public private(set) var scroll = 0
    /// The selection's two ends, as the mouse made it.
    public private(set) var selection: (anchor: Position, head: Position)?

    public init() {}

    public static func == (a: Scrollback, b: Scrollback) -> Bool {
        a.lines == b.lines && a.dropped == b.dropped && a.scroll == b.scroll
            && a.selection?.anchor == b.selection?.anchor && a.selection?.head == b.selection?.head
    }

    /// Adds `text`'s lines (a line a line break), with tabs as four
    /// spaces.
    public mutating func append(_ text: String, kind: Kind) {
        var pieces = text.split(separator: "\n", omittingEmptySubsequences: false)
        if pieces.count > 1, pieces.last?.isEmpty == true {
            pieces.removeLast()
        }
        for piece in pieces {
            var characters = Array(LineEditor.printable(piece.replacingOccurrences(of: "\t", with: "    ")))
            if characters.count > Self.maxLineLength {
                characters = Array(characters.prefix(Self.maxLineLength - 1)) + ["…"]
            }
            lines.append(Line(text: characters, kind: kind))
        }
        if lines.count > Self.capacity {
            let extra = lines.count - Self.capacity
            lines.removeFirst(extra)
            dropped += extra
            if let selection, min(selection.anchor, selection.head).line < dropped {
                self.selection = nil
            }
        }
    }

    /// Empties it (`clear`, Control-L).
    public mutating func clear() {
        dropped += lines.count
        lines.removeAll()
        scroll = 0
        selection = nil
    }

    /// The number of rows the lines take at `columns`.
    public func rowCount(columns: Int) -> Int {
        lines.reduce(0) { $0 + Self.rows(of: $1.text.count, columns: columns) }
    }

    static func rows(of length: Int, columns: Int) -> Int {
        max(1, (length + max(1, columns) - 1) / max(1, columns))
    }

    /// The `count` rows shown at `columns`, top first, `scroll` rows up
    /// from the bottom (found from the bottom, so that only those shown
    /// are made).
    public func visibleRows(columns: Int, count: Int) -> [Row] {
        let columns = max(1, columns)
        var rows: [Row] = []
        var skip = scroll
        var index = lines.count - 1
        while index >= 0, rows.count < count {
            let line = lines[index]
            var row = Self.rows(of: line.text.count, columns: columns) - 1
            while row >= 0, rows.count < count {
                if skip > 0 {
                    skip -= 1
                } else {
                    let start = row * columns
                    let end = min(line.text.count, start + columns)
                    rows.append(Row(line: dropped + index, start: start, text: String(line.text[start..<end]),
                                    length: end - start, kind: line.kind))
                }
                row -= 1
            }
            index -= 1
        }
        return rows.reversed()
    }

    /// Scrolls by `rows` (up positive), no further than the top of the
    /// lines with `visible` rows shown, nor below the bottom.
    public mutating func scroll(by rows: Int, columns: Int, visible: Int) {
        let most = max(0, rowCount(columns: columns) - visible)
        scroll = min(most, max(0, scroll + rows))
    }

    /// Back at the bottom (a line typed shows the newest).
    public mutating func scrollToBottom() {
        scroll = 0
    }

    /// Starts a selection at `position` (a click).
    public mutating func select(at position: Position) {
        selection = (clamped(position), clamped(position))
    }

    /// Moves the selection's end to `position` (a drag).
    public mutating func extendSelection(to position: Position) {
        guard let selection else { return }
        self.selection = (selection.anchor, clamped(position))
    }

    public mutating func clearSelection() {
        selection = nil
    }

    /// The selection's two ends in order, or nil without one or when it
    /// is empty.
    public var selectedRange: (start: Position, end: Position)? {
        guard let selection, selection.anchor != selection.head else { return nil }
        return (min(selection.anchor, selection.head), max(selection.anchor, selection.head))
    }

    /// The selected text, its lines joined by line breaks.
    public var selectedText: String? {
        guard let (start, end) = selectedRange else { return nil }
        var pieces: [String] = []
        for number in start.line...end.line {
            guard let line = line(number) else { continue }
            let from = number == start.line ? start.offset : 0
            let to = number == end.line ? end.offset : line.text.count
            pieces.append(String(line.text[min(from, to)..<to]))
        }
        return pieces.joined(separator: "\n")
    }

    /// The columns of `row` that are selected, or nil.
    public func selectedColumns(_ row: Row) -> Range<Int>? {
        guard let (start, end) = selectedRange,
              Position(line: row.line, offset: row.start + row.length) >= start,
              Position(line: row.line, offset: row.start) <= end else { return nil }
        let from = start.line == row.line ? max(0, start.offset - row.start) : 0
        let to = end.line == row.line ? min(row.length, end.offset - row.start) : row.length
        // (a line break selected shows as a cell past the end)
        let past = end.line > row.line && row.start + row.length == line(row.line)?.text.count ? 1 : 0
        return from < to + past ? from..<(to + past) : nil
    }

    /// The line numbered `number`, if it is kept.
    public func line(_ number: Int) -> Line? {
        let index = number - dropped
        return lines.indices.contains(index) ? lines[index] : nil
    }

    private func clamped(_ position: Position) -> Position {
        guard !lines.isEmpty else { return Position(line: dropped, offset: 0) }
        let number = min(max(position.line, dropped), dropped + lines.count - 1)
        let length = lines[number - dropped].text.count
        return Position(line: number, offset: min(max(0, position.offset), length))
    }
}
