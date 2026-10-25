// The developer shell's overlay: opened and closed with ~ (the key left of
// 1), sliding down over the top of the game; while it is open it has the
// keys and the pointer, runs the lines typed through the engine's shell,
// completes them with Tab, searches the history with Control-R, follows
// the engine's log (as `log filter` filters it) and shows the watched
// expressions' line, which stays at the top of the screen once it is
// closed. It makes the TextPass's items; closed with nothing watched it
// makes none and asks the engine for nothing but the watch list, once a
// second.
import Foundation
import OpenCEEngineCore
import OpenCEEngineRender

/// What the overlay asks of the engine (the Engine, or a test's fake).
public protocol ShellEngine: AnyObject {
    func runLine(_ line: String) -> ShellOutput
    func completions(_ line: String) -> [String]
    func watches() -> [WatchValue]
    func logLines(since sequence: UInt64, max: UInt32) -> [LogEntry]
}

extension Engine: ShellEngine {
    public func runLine(_ line: String) -> ShellOutput { runCommand(line: line) }
    public func completions(_ line: String) -> [String] { completeCommand(line: line) }
    public func watches() -> [WatchValue] { watchValues() }
    public func logLines(since sequence: UInt64, max: UInt32) -> [LogEntry] { logSince(sequence: sequence, max: max) }
}

/// The system's clipboard, or a test's.
public protocol ShellPasteboard: AnyObject {
    func copy(_ text: String)
    func paste() -> String?
}

/// A key as the shell reads it.
public enum ShellKey: Equatable, Sendable {
    case text(String)
    case enter, tab, backTab, backspace, deleteForward, escape
    case left, right, up, down, home, end, pageUp, pageDown
    case wordLeft, wordRight, killWordBack
    /// Control and a letter.
    case control(Character)
    /// Command-C, -V and -K.
    case copy, paste, clear
    /// ~: opens or closes the shell.
    case toggle

    public struct Modifiers: OptionSet, Sendable {
        public let rawValue: Int
        public init(rawValue: Int) { self.rawValue = rawValue }
        public static let shift = Modifiers(rawValue: 1)
        public static let control = Modifiers(rawValue: 2)
        public static let option = Modifiers(rawValue: 4)
        public static let command = Modifiers(rawValue: 8)
    }

    /// The key that ~ is on (ANSI's grave accent; the key left of 1).
    public static let toggleCode: UInt16 = 50

    /// The key a key press gives, by its virtual code, the characters it
    /// typed and the modifiers held; nil for one the shell leaves (the
    /// menus' Command keys).
    public init?(code: UInt16, characters: String, ignoringModifiers: String, modifiers: Modifiers) {
        let control = modifiers.contains(.control), option = modifiers.contains(.option)
        let command = modifiers.contains(.command)
        switch code {
        case Self.toggleCode where !control && !option && !command: self = .toggle
        case 36, 76: self = .enter
        case 48: self = modifiers.contains(.shift) ? .backTab : .tab
        case 51: self = option || control ? .killWordBack : .backspace
        case 117: self = .deleteForward
        case 53: self = .escape
        case 123: self = command ? .home : option ? .wordLeft : .left
        case 124: self = command ? .end : option ? .wordRight : .right
        case 125: self = .down
        case 126: self = .up
        case 115: self = .home
        case 119: self = .end
        case 116: self = .pageUp
        case 121: self = .pageDown
        default:
            let letter = ignoringModifiers.lowercased().first
            if command {
                switch letter {
                case "c": self = .copy
                case "v": self = .paste
                case "k": self = .clear
                default: return nil
                }
            } else if control {
                guard let letter, letter.isLetter, letter.isASCII else { return nil }
                self = .control(letter)
            } else {
                // (the function keys' characters are Apple's private use
                // ones, U+F700 to U+F8FF)
                let typed = LineEditor.printable(characters).filter {
                    !$0.unicodeScalars.contains { (0xF700...0xF8FF).contains($0.value) }
                }
                guard !typed.isEmpty else { return nil }
                self = .text(typed)
            }
        }
    }
}

/// The overlay's measures, in pixels: the glyph atlas's cell, and the
/// scale its padding goes with.
public struct ShellMetrics: Equatable, Sendable {
    public var cellWidth: Int
    public var cellHeight: Int
    public var scale: Int

    public init(cellWidth: Int, cellHeight: Int, scale: Int) {
        self.cellWidth = max(1, cellWidth)
        self.cellHeight = max(1, cellHeight)
        self.scale = max(1, scale)
    }

    public init(atlas: GlyphAtlas) {
        self.init(cellWidth: atlas.cellWidth, cellHeight: atlas.cellHeight, scale: Int(atlas.scale.rounded()))
    }
}

/// The shell.
public final class ShellOverlay {
    /// How long sliding open or shut takes.
    public static let slideSeconds = 0.12
    /// The share of the screen's height it takes open.
    public static let heightShare = 0.45
    /// The log's lines taken in a frame.
    public static let logBatch: UInt32 = 256

    public enum Colors {
        public static let background = SIMD4<Float>(0.02, 0.03, 0.05, 0.86)
        public static let rule = SIMD4<Float>(0.35, 0.6, 0.9, 1)
        public static let input = SIMD4<Float>(0.55, 0.8, 1, 1)
        public static let output = SIMD4<Float>(0.9, 0.9, 0.9, 1)
        public static let error = SIMD4<Float>(1, 0.45, 0.4, 1)
        public static let warning = SIMD4<Float>(1, 0.8, 0.35, 1)
        public static let info = SIMD4<Float>(0.72, 0.74, 0.76, 1)
        public static let debug = SIMD4<Float>(0.52, 0.58, 0.64, 1)
        public static let trace = SIMD4<Float>(0.42, 0.44, 0.5, 1)
        public static let selection = SIMD4<Float>(0.2, 0.4, 0.75, 0.7)
        public static let cursor = SIMD4<Float>(0.9, 0.95, 1, 0.9)
        public static let hint = SIMD4<Float>(0.6, 0.65, 0.7, 1)
        public static let watchBackground = SIMD4<Float>(0, 0, 0, 0.6)

        public static func of(_ kind: Scrollback.Kind) -> SIMD4<Float> {
            switch kind {
            case .input: input
            case .output: output
            case .error, .log(.error): error
            case .log(.warn): warning
            case .log(.info): info
            case .log(.debug): debug
            case .log(.trace): trace
            }
        }
    }

    /// Where the last frame put the scrollback's rows, for the pointer.
    struct Layout {
        var rows: [Scrollback.Row]
        var rowsTop: Int
        var left: Int
        var bottom: Int
        var metrics: ShellMetrics
        var visibleRows: Int
        var columns: Int
    }

    let engine: ShellEngine
    let pasteboard: ShellPasteboard?
    let historyFile: URL?

    public private(set) var editor = LineEditor()
    public private(set) var history: ShellHistory
    public private(set) var scrollback = Scrollback()
    /// Tab's candidates while they are shown, and the one chosen.
    public private(set) var completions: [String] = []
    public private(set) var completionIndex: Int?
    /// Control-R's search: what is typed, and the entry it found.
    public private(set) var search: (query: String, match: Int?)?
    public private(set) var watchValues: [WatchValue] = []
    public private(set) var isOpen = false
    /// 0 shut, 1 open.
    public private(set) var openness = 0.0
    /// The log's last line taken.
    public private(set) var logSequence: UInt64 = 0
    private var lastWatchPoll: TimeInterval?
    private var wheel = 0.0
    private var layout: Layout?

    /// Called with each line's output, for the app to do its action.
    public var onOutput: ((ShellOutput) -> Void)?
    /// Called when it opens or closes.
    public var onOpenChanged: ((Bool) -> Void)?

    public init(engine: ShellEngine, pasteboard: ShellPasteboard? = nil, historyFile: URL? = nil) {
        self.engine = engine
        self.pasteboard = pasteboard
        self.historyFile = historyFile
        history = historyFile.map(ShellHistory.load(from:)) ?? ShellHistory()
    }

    /// Whether anything is drawn: open, sliding, or a watch line.
    public var isVisible: Bool { openness > 0 || !watchValues.isEmpty }

    // ---------- opening and closing

    public func toggle() {
        isOpen ? close() : open()
    }

    public func open() {
        guard !isOpen else { return }
        isOpen = true
        onOpenChanged?(true)
    }

    public func close() {
        guard isOpen else { return }
        isOpen = false
        search = nil
        completions = []
        completionIndex = nil
        scrollback.clearSelection()
        onOpenChanged?(false)
    }

    /// Slides it `seconds` on, and takes the log's new lines (while open)
    /// and the watch line's values (four times a second while open or
    /// watching, else once a second, `now` the time in seconds).
    public func update(seconds: Double, now: TimeInterval) {
        let step = max(0, seconds) / Self.slideSeconds
        openness = isOpen ? min(1, openness + step) : max(0, openness - step)
        if isOpen {
            takeLog()
        }
        let interval = isOpen || !watchValues.isEmpty ? 0.25 : 1
        if lastWatchPoll.map({ now - $0 >= interval || now < $0 }) ?? true {
            pollWatches(now: now)
        }
    }

    func takeLog() {
        let entries = engine.logLines(since: logSequence, max: Self.logBatch)
        for entry in entries {
            let level = Scrollback.LogLevel(rawValue: entry.level) ?? .info
            scrollback.append("\(entry.target): \(entry.text)", kind: .log(level))
            logSequence = max(logSequence, entry.sequence)
        }
    }

    func pollWatches(now: TimeInterval) {
        lastWatchPoll = now
        let values = engine.watches()
        if values != watchValues {
            watchValues = values
        }
    }

    // ---------- keys

    /// Handles a key while it is open (true: taken).
    @discardableResult
    public func handle(_ key: ShellKey) -> Bool {
        guard isOpen else {
            if key == .toggle {
                open()
                return true
            }
            return false
        }
        if search != nil, searchKey(key) {
            return true
        }
        if key != .tab, key != .backTab {
            completions = []
            completionIndex = nil
        }
        switch key {
        case let .text(text): editor.insert(text)
        case .enter: run()
        case .tab: complete(forward: true)
        case .backTab: complete(forward: false)
        case .backspace: editor.backspace()
        case .deleteForward: editor.deleteForward()
        case .killWordBack: editor.killWordBack()
        case .left: editor.left()
        case .right: editor.right()
        case .home: editor.home()
        case .end: editor.end()
        case .wordLeft: editor.wordLeft()
        case .wordRight: editor.wordRight()
        case .up: historyUp()
        case .down: historyDown()
        case .pageUp: scrollPage(1)
        case .pageDown: scrollPage(-1)
        case .copy: copySelection()
        case .paste:
            if let text = pasteboard?.paste() {
                editor.insert(text)
            }
        case .clear: scrollback.clear()
        case .escape, .toggle: close()
        case let .control(letter): control(letter)
        }
        return true
    }

    private func control(_ letter: Character) {
        switch letter {
        case "a": editor.home()
        case "e": editor.end()
        case "b": editor.left()
        case "f": editor.right()
        case "d": editor.deleteForward()
        case "h": editor.backspace()
        case "k": editor.killToEnd()
        case "u": editor.killToStart()
        case "w": editor.killWordBack()
        case "y": editor.yank()
        case "p": historyUp()
        case "n": historyDown()
        case "l": scrollback.clear()
        case "r": search = (query: "", match: nil)
        case "c":
            // (the line given up, as a terminal shows it)
            scrollback.append("] \(editor.text)^C", kind: .input)
            editor.set("")
            history.reset()
        default: break
        }
    }

    /// A key during Control-R's search; false for one that ends it and is
    /// then handled as usual.
    private func searchKey(_ key: ShellKey) -> Bool {
        guard var (query, match) = search else { return false }
        switch key {
        case let .text(text):
            query += text
            match = history.search(query)
        case .backspace:
            if !query.isEmpty { query.removeLast() }
            match = history.search(query)
        case .control("r"):
            match = history.search(query, before: match) ?? match
        case .escape, .control("g"), .control("c"):
            search = nil
            return true
        case .enter:
            accept(match)
            run()
            return true
        default:
            accept(match)
            return false
        }
        search = (query, match)
        return true
    }

    private func accept(_ match: Int?) {
        if let match, history.entries.indices.contains(match) {
            editor.set(history.entries[match])
        }
        search = nil
    }

    private func historyUp() {
        if let line = history.previous(current: editor.text) {
            editor.set(line)
        }
    }

    private func historyDown() {
        if let line = history.next() {
            editor.set(line)
        }
    }

    /// Runs the line typed.
    func run() {
        let line = editor.text
        editor.set("")
        history.add(line)
        scrollback.scrollToBottom()
        scrollback.append("] \(line)", kind: .input)
        guard !line.trimmingCharacters(in: .whitespaces).isEmpty else { return }
        if let historyFile {
            try? history.save(to: historyFile)
        }
        let output = engine.runLine(line)
        // (the command's own log lines before its output)
        takeLog()
        if !output.text.isEmpty {
            scrollback.append(output.text, kind: output.ok ? .output : .error)
        }
        if Self.clears(output.json) {
            scrollback.clear()
        }
        pollWatches(now: lastWatchPoll ?? 0)
        onOutput?(output)
    }

    /// Whether a reply's data is `{"clear": true}`.
    static func clears(_ json: String) -> Bool {
        guard json.contains("clear"),
            let object = try? JSONSerialization.jsonObject(with: Data(json.utf8)) as? [String: Any]
        else { return false }
        return object["clear"] as? Bool == true
    }

    /// Tab: one candidate replaces the line; several are listed, the line
    /// taking what they share, and further Tabs go through them.
    private func complete(forward: Bool) {
        if !completions.isEmpty {
            let count = completions.count
            let index = completionIndex.map { ($0 + (forward ? 1 : count - 1)) % count } ?? (forward ? 0 : count - 1)
            completionIndex = index
            editor.set(completions[index])
            return
        }
        let candidates = engine.completions(editor.text)
        guard let first = candidates.first else { return }
        if candidates.count == 1 {
            editor.set(first)
            return
        }
        let shared = candidates.dropFirst().reduce(first) { String($0.commonPrefix(with: $1)) }
        if shared.count > editor.text.count {
            editor.set(shared)
        }
        completions = candidates
        completionIndex = nil
    }

    private func copySelection() {
        if let text = scrollback.selectedText {
            pasteboard?.copy(text)
        }
    }

    private func scrollPage(_ direction: Int) {
        guard let layout else { return }
        scrollback.scroll(
            by: direction * max(1, layout.visibleRows - 1), columns: layout.columns,
            visible: layout.visibleRows)
    }

    // ---------- the pointer

    public enum PointerPhase: Sendable {
        case down, dragged, up
    }

    /// A button or drag at `x`, `y` pixels from the top left; true when
    /// it is the shell's (open, and over it).
    @discardableResult
    public func pointer(_ phase: PointerPhase, x: Double, y: Double) -> Bool {
        guard isOpen, let layout, let position = position(x: x, y: y, layout: layout) else { return false }
        switch phase {
        case .down:
            guard y < Double(layout.bottom) else { return false }
            scrollback.select(at: position)
        case .dragged: scrollback.extendSelection(to: position)
        case .up: break
        }
        return true
    }

    /// The wheel: `pixels` up positive; true when it is the shell's.
    @discardableResult
    public func scrollWheel(pixels: Double) -> Bool {
        guard isOpen, let layout else { return false }
        wheel += pixels / Double(layout.metrics.cellHeight)
        let rows = Int(wheel.rounded(.towardZero))
        guard rows != 0 else { return true }
        wheel -= Double(rows)
        scrollback.scroll(by: rows, columns: layout.columns, visible: layout.visibleRows)
        return true
    }

    private func position(x: Double, y: Double, layout: Layout) -> Scrollback.Position? {
        guard let last = layout.rows.last, let first = layout.rows.first else { return nil }
        let index = Int(floor((y - Double(layout.rowsTop)) / Double(layout.metrics.cellHeight)))
        if index < 0 {
            return Scrollback.Position(line: first.line, offset: first.start)
        }
        guard index < layout.rows.count else {
            return Scrollback.Position(line: last.line, offset: last.start + last.length)
        }
        let row = layout.rows[index]
        let column = Int(((x - Double(layout.left)) / Double(layout.metrics.cellWidth)).rounded())
        return Scrollback.Position(line: row.line, offset: row.start + min(max(0, column), row.length))
    }

    // ---------- drawing

    /// The items to draw over a `width` by `height` frame (none while it
    /// is shut with nothing watched).
    public func items(width: Int, height: Int, metrics: ShellMetrics) -> [TextItem] {
        guard isVisible, width > 0, height > 0 else {
            layout = nil
            return []
        }
        var items: [TextItem] = []
        let cell = metrics.cellHeight
        let pad = metrics.cellWidth
        let columns = max(1, (width - 2 * pad) / metrics.cellWidth)
        let panel = max(cell * 6, Int(Double(height) * Self.heightShare))
        let top = Int((Double(-panel) * (1 - openness)).rounded())
        if openness > 0 {
            items.append(.box(x: 0, y: top, width: width, height: panel, color: Colors.background))
            items.append(
                .box(
                    x: 0, y: top + panel - metrics.scale, width: width, height: metrics.scale,
                    color: Colors.rule))
            let inputTop = top + panel - metrics.scale - cell - pad / 2
            var rowsBottom = inputTop - pad / 2
            if !completions.isEmpty {
                rowsBottom -= cell
                items.append(contentsOf: completionRow(y: rowsBottom, left: pad, columns: columns))
            }
            let rowsTop = top + pad / 2
            let visible = max(1, (rowsBottom - rowsTop) / cell)
            let rows = scrollback.visibleRows(columns: columns, count: visible)
            // (the rows sit on the input, the oldest at the top)
            let firstTop = rowsBottom - rows.count * cell
            for (index, row) in rows.enumerated() {
                let y = firstTop + index * cell
                if let selected = scrollback.selectedColumns(row) {
                    items.append(
                        .box(
                            x: pad + selected.lowerBound * metrics.cellWidth, y: y,
                            width: selected.count * metrics.cellWidth, height: cell,
                            color: Colors.selection))
                }
                if !row.text.isEmpty {
                    items.append(.text(row.text, x: pad, y: y, color: Colors.of(row.kind)))
                }
            }
            if scrollback.scroll > 0 {
                let mark = "↓ \(scrollback.scroll)"
                items.append(
                    .text(
                        mark, x: width - pad - mark.count * metrics.cellWidth, y: inputTop,
                        color: Colors.hint))
            }
            items.append(contentsOf: inputRow(y: inputTop, left: pad, columns: columns, metrics: metrics))
            layout = Layout(
                rows: rows, rowsTop: firstTop, left: pad, bottom: top + panel, metrics: metrics,
                visibleRows: visible, columns: columns)
        } else {
            layout = nil
        }
        if !watchValues.isEmpty {
            items.append(
                contentsOf: watchRow(
                    y: max(0, top + panel), width: width, left: pad, columns: columns,
                    metrics: metrics))
        }
        return items
    }

    private func inputRow(y: Int, left: Int, columns: Int, metrics: ShellMetrics) -> [TextItem] {
        var prompt = "] "
        var characters = editor.characters
        var cursor = editor.cursor
        if let (query, match) = search {
            prompt = match == nil && !query.isEmpty ? "(failed search)'\(query)': " : "(search)'\(query)': "
            characters = match.map { Array(history.entries[$0]) } ?? []
            cursor = characters.count
        }
        let room = max(1, columns - prompt.count)
        // (a long line scrolls to keep the cursor in view)
        let first = max(0, cursor - room + 1)
        let shown = String(characters[first..<min(characters.count, first + room)])
        var items: [TextItem] = [.text(prompt, x: left, y: y, color: search == nil ? Colors.input : Colors.hint)]
        if !shown.isEmpty {
            items.append(.text(shown, x: left + prompt.count * metrics.cellWidth, y: y, color: Colors.output))
        }
        let cursorX = left + (prompt.count + cursor - first) * metrics.cellWidth
        items.append(
            .box(
                x: cursorX, y: y, width: 2 * metrics.scale, height: metrics.cellHeight,
                color: Colors.cursor))
        return items
    }

    private func completionRow(y: Int, left: Int, columns: Int) -> [TextItem] {
        var text = ""
        for (index, candidate) in completions.enumerated() {
            let entry = index == completionIndex ? "▸\(candidate)" : " \(candidate)"
            guard text.count + entry.count + 1 <= columns else {
                text += " …"
                break
            }
            text += entry + " "
        }
        return [.text(text, x: left, y: y, color: Colors.hint)]
    }

    private func watchRow(y: Int, width: Int, left: Int, columns: Int, metrics: ShellMetrics) -> [TextItem] {
        var items: [TextItem] = [
            .box(
                x: 0, y: y, width: width, height: metrics.cellHeight + metrics.scale * 2,
                color: Colors.watchBackground)
        ]
        var column = 0
        for (index, watch) in watchValues.enumerated() {
            let entry = (index > 0 ? "  │  " : "") + "\(watch.expression) = \(watch.value)"
            let room = columns - column
            guard room > 1 else { break }
            let shown = entry.count > room ? String(entry.prefix(room - 1)) + "…" : entry
            items.append(
                .text(
                    shown, x: left + column * metrics.cellWidth, y: y + metrics.scale,
                    color: watch.ok ? Colors.output : Colors.error))
            column += shown.count
        }
        return items
    }
}
