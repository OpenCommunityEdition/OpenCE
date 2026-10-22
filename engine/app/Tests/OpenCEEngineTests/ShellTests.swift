import Foundation
import OpenCEEngineCore
import OpenCEEngineKit
import OpenCEEngineRender
import XCTest

/// The developer shell's terminal: line editing, the history and its
/// search, completion, the scrollback with its selection and copying, the
/// log and watch lines, and what the overlay draws.
final class ShellTests: XCTestCase {
    final class FakeEngine: ShellEngine {
        var ran: [String] = []
        var replies: [String: ShellOutput] = [:]
        var candidates: [String: [String]] = [:]
        var watched: [WatchValue] = []
        var log: [LogEntry] = []
        var watchPolls = 0
        var logPolls = 0

        func runLine(_ line: String) -> ShellOutput {
            ran.append(line)
            return replies[line] ?? ShellOutput(ok: true, text: "ran \(line)", json: "null", action: .none)
        }

        func completions(_ line: String) -> [String] { candidates[line] ?? [] }

        func watches() -> [WatchValue] {
            watchPolls += 1
            return watched
        }

        func logLines(since sequence: UInt64, max: UInt32) -> [LogEntry] {
            logPolls += 1
            return Array(log.filter { $0.sequence > sequence }.prefix(Int(max)))
        }
    }

    final class FakePasteboard: ShellPasteboard {
        var text: String?
        func copy(_ text: String) { self.text = text }
        func paste() -> String? { text }
    }

    let metrics = ShellMetrics(cellWidth: 8, cellHeight: 16, scale: 1)

    func type(_ text: String, into shell: ShellOverlay) {
        shell.handle(.text(text))
    }

    // ---------- the line

    func testLineEditing() {
        var line = LineEditor("spawn warthog")
        XCTAssertEqual(line.cursor, 13)
        line.wordLeft()
        XCTAssertEqual(line.cursor, 6)
        line.insert("big ")
        XCTAssertEqual(line.text, "spawn big warthog")
        line.home()
        line.deleteForward()
        XCTAssertEqual(line.text, "pawn big warthog")
        line.right()
        line.backspace()
        XCTAssertEqual(line.text, "awn big warthog")
        line.wordRight()
        XCTAssertEqual(line.cursor, 3)
        line.killToEnd()
        XCTAssertEqual(line.text, "awn")
        XCTAssertEqual(line.killed, " big warthog")
        line.home()
        line.yank()
        XCTAssertEqual(line.text, " big warthogawn")
        line.end()
        line.killWordBack()
        XCTAssertEqual(line.text, " big ")
        XCTAssertEqual(line.killed, "warthogawn")
        line.left()
        line.killToStart()
        XCTAssertEqual(line.text, " ")
        XCTAssertEqual(line.cursor, 0)
        // (the edges hold)
        line.left()
        line.backspace()
        XCTAssertEqual(line.cursor, 0)
        line.end()
        line.right()
        line.deleteForward()
        XCTAssertEqual(line.text, " ")
        // (control characters left out, line breaks as spaces, and no
        // more than the longest line)
        var typed = LineEditor()
        typed.insert("a\u{7}b\nc\td")
        XCTAssertEqual(typed.text, "ab c d")
        typed.insert(String(repeating: "x", count: LineEditor.maxLength * 2))
        XCTAssertEqual(typed.characters.count, LineEditor.maxLength)
        // (HaloScript's names are words: a dot or dash does not split one)
        var words = LineEditor("(ai_place jail.guards-1)")
        words.wordLeft()
        XCTAssertEqual(words.cursor, 10)
        words.killWordBack()
        XCTAssertEqual(words.text, "(jail.guards-1)")
    }

    func testTheKeysAreRead() {
        func key(_ code: UInt16, _ characters: String = "", _ modifiers: ShellKey.Modifiers = []) -> ShellKey? {
            ShellKey(code: code, characters: characters, ignoringModifiers: characters, modifiers: modifiers)
        }
        XCTAssertEqual(key(ShellKey.toggleCode, "`"), .toggle)
        XCTAssertEqual(key(ShellKey.toggleCode, "~", .shift), .toggle)
        XCTAssertEqual(key(36), .enter)
        XCTAssertEqual(key(48), .tab)
        XCTAssertEqual(key(48, "", .shift), .backTab)
        XCTAssertEqual(key(51), .backspace)
        XCTAssertEqual(key(51, "", .option), .killWordBack)
        XCTAssertEqual(key(123, "", .option), .wordLeft)
        XCTAssertEqual(key(124, "", .command), .end)
        XCTAssertEqual(key(126), .up)
        XCTAssertEqual(key(0, "a", .control), .control("a"))
        XCTAssertEqual(key(8, "c", .command), .copy)
        XCTAssertEqual(key(9, "v", .command), .paste)
        XCTAssertNil(key(12, "q", .command), "the menus' keys")
        XCTAssertEqual(key(0, "a"), .text("a"))
        XCTAssertEqual(key(0, "A", .shift), .text("A"))
        XCTAssertNil(key(200, "\u{f704}"))
    }

    // ---------- the history

    func testHistoryAndItsSearch() throws {
        var history = ShellHistory(["god", "god", "  ", "spawn warthog", "noclip"])
        XCTAssertEqual(history.entries, ["god", "spawn warthog", "noclip"], "blanks and repeats left out")
        XCTAssertEqual(history.previous(current: "tele"), "noclip")
        XCTAssertEqual(history.previous(current: "ignored"), "spawn warthog")
        XCTAssertEqual(history.previous(current: ""), "god")
        XCTAssertNil(history.previous(current: ""))
        XCTAssertEqual(history.next(), "spawn warthog")
        XCTAssertEqual(history.next(), "noclip")
        XCTAssertEqual(history.next(), "tele", "the line typed kept")
        XCTAssertNil(history.next())
        XCTAssertEqual(history.search("O"), 2)
        XCTAssertEqual(history.search("o", before: 2), 1)
        XCTAssertEqual(history.search("o", before: 1), 0)
        XCTAssertNil(history.search("o", before: 0))
        XCTAssertNil(history.search(""))

        var full = ShellHistory()
        for index in 0..<(ShellHistory.capacity + 10) {
            full.add("line \(index)")
        }
        XCTAssertEqual(full.entries.count, ShellHistory.capacity)
        XCTAssertEqual(full.entries.first, "line 10")

        let folder = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString)
        defer { try? FileManager.default.removeItem(at: folder) }
        let file = folder.appendingPathComponent("shell_history.txt")
        try history.save(to: file)
        XCTAssertEqual(ShellHistory.load(from: file).entries, history.entries)
        XCTAssertEqual(ShellHistory.load(from: folder.appendingPathComponent("none")).entries, [])
    }

    func testTheOverlayRunsLinesAndKeepsTheirHistory() throws {
        let engine = FakeEngine()
        let folder = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString)
        defer { try? FileManager.default.removeItem(at: folder) }
        let file = folder.appendingPathComponent("history.txt")
        let shell = ShellOverlay(engine: engine, historyFile: file)
        var outputs: [ShellOutput] = []
        shell.onOutput = { outputs.append($0) }
        var opened: [Bool] = []
        shell.onOpenChanged = { opened.append($0) }

        XCTAssertFalse(shell.handle(.text("x")), "shut, it takes only ~")
        XCTAssertTrue(shell.handle(.toggle))
        type("god", into: shell)
        shell.handle(.enter)
        engine.replies["bad"] = ShellOutput(ok: false, text: "no command bad", json: "null", action: .none)
        type("bad", into: shell)
        shell.handle(.enter)
        XCTAssertEqual(engine.ran, ["god", "bad"])
        XCTAssertEqual(outputs.count, 2)
        XCTAssertEqual(shell.scrollback.lines.map { String($0.text) }, ["] god", "ran god", "] bad", "no command bad"])
        XCTAssertEqual(shell.scrollback.lines.map(\.kind), [.input, .output, .input, .error])
        XCTAssertEqual(shell.editor.text, "")
        XCTAssertEqual(ShellHistory.load(from: file).entries, ["god", "bad"], "saved as it runs")

        // Up and Down, the line typed kept
        type("ti", into: shell)
        shell.handle(.up)
        XCTAssertEqual(shell.editor.text, "bad")
        shell.handle(.control("p"))
        XCTAssertEqual(shell.editor.text, "god")
        shell.handle(.down)
        shell.handle(.control("n"))
        XCTAssertEqual(shell.editor.text, "ti")

        // Control-R, again for older, Enter runs what it found
        shell.handle(.control("u"))
        shell.handle(.control("r"))
        type("d", into: shell)
        XCTAssertEqual(shell.search?.match, 1)
        shell.handle(.control("r"))
        XCTAssertEqual(shell.search?.match, 0)
        shell.handle(.enter)
        XCTAssertNil(shell.search)
        XCTAssertEqual(engine.ran.last, "god")
        // (a failed search, Escape giving up; an arrow takes what it found)
        shell.handle(.control("r"))
        type("zz", into: shell)
        XCTAssertNil(shell.search?.match)
        shell.handle(.escape)
        XCTAssertNil(shell.search)
        XCTAssertTrue(shell.isOpen, "Escape ends the search, not the shell")
        shell.handle(.control("r"))
        type("ba", into: shell)
        shell.handle(.left)
        XCTAssertEqual(shell.editor.text, "bad")
        XCTAssertEqual(shell.editor.cursor, 2)

        // Control-C gives up the line
        shell.handle(.control("c"))
        XCTAssertEqual(shell.editor.text, "")
        XCTAssertEqual(String(shell.scrollback.lines.last?.text ?? []), "] bad^C")

        // a blank line is shown, not run
        let runs = engine.ran.count
        shell.handle(.enter)
        XCTAssertEqual(engine.ran.count, runs)

        // `clear` clears; Escape shuts it
        engine.replies["clear"] = ShellOutput(ok: true, text: "", json: "{\"clear\": true}", action: .none)
        type("clear", into: shell)
        shell.handle(.enter)
        XCTAssertTrue(shell.scrollback.lines.isEmpty)
        shell.handle(.escape)
        XCTAssertFalse(shell.isOpen)
        XCTAssertEqual(opened, [true, false])
    }

    func testCompletion() {
        let engine = FakeEngine()
        let shell = ShellOverlay(engine: engine)
        shell.open()
        shell.update(seconds: 1, now: 0)
        engine.candidates["no"] = ["noclip"]
        engine.candidates["s"] = ["set", "spawn", "step"]
        engine.candidates["sp"] = ["spawn"]
        type("no", into: shell)
        shell.handle(.tab)
        XCTAssertEqual(shell.editor.text, "noclip", "one candidate replaces the line")
        XCTAssertTrue(shell.completions.isEmpty)

        shell.handle(.control("u"))
        type("s", into: shell)
        shell.handle(.tab)
        XCTAssertEqual(shell.completions, ["set", "spawn", "step"], "several are listed")
        XCTAssertEqual(shell.editor.text, "s")
        shell.handle(.tab)
        XCTAssertEqual(shell.editor.text, "set")
        shell.handle(.tab)
        shell.handle(.tab)
        XCTAssertEqual(shell.editor.text, "step")
        shell.handle(.tab)
        XCTAssertEqual(shell.editor.text, "set", "and round again")
        shell.handle(.backTab)
        XCTAssertEqual(shell.editor.text, "step")
        let listed = shell.items(width: 640, height: 480, metrics: metrics)
        XCTAssertTrue(listed.contains { if case let .text(text, _, _, _) = $0 { text.contains("▸step") } else { false } })
        shell.handle(.text(" "))
        XCTAssertTrue(shell.completions.isEmpty, "another key ends it")

        // what they share is taken
        engine.candidates["tele"] = ["teleport_to", "teleport_player"]
        shell.handle(.control("u"))
        type("tele", into: shell)
        shell.handle(.tab)
        XCTAssertEqual(shell.editor.text, "teleport_")
        shell.handle(.control("u"))
        shell.handle(.tab)
        XCTAssertEqual(shell.editor.text, "", "nothing to complete")
    }

    // ---------- the scrollback

    func testScrollbackWrapsScrollsAndKeepsItsCapacity() {
        var lines = Scrollback()
        lines.append("one\ntwo\n", kind: .output)
        lines.append("abcdefghij", kind: .error)
        lines.append("", kind: .output)
        lines.append("\tx", kind: .output)
        XCTAssertEqual(lines.lines.count, 5)
        XCTAssertEqual(String(lines.lines[4].text), "    x")
        XCTAssertEqual(lines.rowCount(columns: 4), 1 + 1 + 3 + 1 + 2)
        let rows = lines.visibleRows(columns: 4, count: 4)
        XCTAssertEqual(rows.map(\.text), ["ij", "", "    ", "x"])
        XCTAssertEqual(rows.first?.kind, .error)
        XCTAssertEqual(rows.first?.start, 8)
        lines.scroll(by: 3, columns: 4, visible: 4)
        XCTAssertEqual(lines.visibleRows(columns: 4, count: 4).map(\.text), ["two", "abcd", "efgh", "ij"])
        lines.scroll(by: 100, columns: 4, visible: 4)
        XCTAssertEqual(lines.scroll, 4, "no further than the top")
        lines.scroll(by: -100, columns: 4, visible: 4)
        XCTAssertEqual(lines.scroll, 0)

        var full = Scrollback()
        for index in 0..<(Scrollback.capacity + 5) {
            full.append("\(index)", kind: .output)
        }
        XCTAssertEqual(full.lines.count, Scrollback.capacity)
        XCTAssertEqual(full.dropped, 5)
        XCTAssertEqual(String(full.lines[0].text), "5")
        XCTAssertEqual(String(full.line(5)?.text ?? []), "5")
        XCTAssertNil(full.line(4))
        full.append(String(repeating: "y", count: Scrollback.maxLineLength * 2), kind: .output)
        XCTAssertEqual(full.lines.last?.text.count, Scrollback.maxLineLength)
        XCTAssertEqual(full.lines.last?.text.last, "…")
        full.clear()
        XCTAssertTrue(full.lines.isEmpty)
        XCTAssertEqual(full.dropped, Scrollback.capacity + 6)
    }

    func testSelectionAndCopy() {
        var lines = Scrollback()
        lines.append("first line\nsecond\nthird", kind: .output)
        lines.select(at: .init(line: 0, offset: 6))
        lines.extendSelection(to: .init(line: 2, offset: 3))
        XCTAssertEqual(lines.selectedText, "line\nsecond\nthi")
        // (backwards is the same)
        lines.select(at: .init(line: 2, offset: 3))
        lines.extendSelection(to: .init(line: 0, offset: 6))
        XCTAssertEqual(lines.selectedText, "line\nsecond\nthi")
        let rows = lines.visibleRows(columns: 20, count: 3)
        XCTAssertEqual(lines.selectedColumns(rows[0]), 6..<11, "with the line break")
        XCTAssertEqual(lines.selectedColumns(rows[1]), 0..<7)
        XCTAssertEqual(lines.selectedColumns(rows[2]), 0..<3)
        // (a click alone selects nothing; positions past the ends are held)
        lines.select(at: .init(line: 9, offset: 99))
        XCTAssertNil(lines.selectedText)
        lines.extendSelection(to: .init(line: 2, offset: 0))
        XCTAssertEqual(lines.selectedText, "third")
        // (a selection of lines dropped goes)
        var dropping = Scrollback()
        dropping.append("a\nb", kind: .output)
        dropping.select(at: .init(line: 0, offset: 0))
        dropping.extendSelection(to: .init(line: 1, offset: 1))
        for _ in 0..<Scrollback.capacity {
            dropping.append("z", kind: .output)
        }
        XCTAssertNil(dropping.selection)
    }

    func testTheOverlaySelectsWithThePointerAndCopies() throws {
        let engine = FakeEngine()
        let pasteboard = FakePasteboard()
        let shell = ShellOverlay(engine: engine, pasteboard: pasteboard)
        shell.open()
        shell.update(seconds: 1, now: 0)
        type("echo", into: shell)
        shell.handle(.enter)
        let items = shell.items(width: 640, height: 480, metrics: metrics)
        // the rows' places, from what was drawn
        let rows = items.compactMap { item -> (String, Int, Int)? in
            if case let .text(text, x, y, _) = item, text.hasPrefix("ran") || text.hasPrefix("] echo") {
                return (text, x, y)
            }
            return nil
        }
        let output = try XCTUnwrap(rows.first { $0.0 == "ran echo" })
        let input = try XCTUnwrap(rows.first { $0.0 == "] echo" })
        XCTAssertEqual(output.2 - input.2, metrics.cellHeight, "a row a line, the newest lowest")
        XCTAssertTrue(shell.pointer(.down, x: Double(output.1), y: Double(output.2 + 4)))
        XCTAssertTrue(shell.pointer(.dragged, x: Double(output.1 + 3 * metrics.cellWidth), y: Double(output.2 + 4)))
        shell.pointer(.up, x: 0, y: 0)
        shell.handle(.copy)
        XCTAssertEqual(pasteboard.text, "ran")
        let drawn = shell.items(width: 640, height: 480, metrics: metrics)
        XCTAssertTrue(drawn.contains { item in
            if case let .box(x, y, width, _, color) = item {
                return color == ShellOverlay.Colors.selection && x == output.1 && y == output.2
                    && width == 3 * metrics.cellWidth
            }
            return false
        })
        // Command-V types the clipboard's text
        pasteboard.text = "pasted\ntwo"
        shell.handle(.paste)
        XCTAssertEqual(shell.editor.text, "pasted two")
        // (the pointer below the panel is the game's)
        XCTAssertFalse(shell.pointer(.down, x: 10, y: 470))
        shell.close()
        XCTAssertFalse(shell.pointer(.down, x: Double(output.1), y: Double(output.2)))
    }

    func testTheWheelAndPagesScroll() {
        let shell = ShellOverlay(engine: FakeEngine())
        shell.open()
        shell.update(seconds: 1, now: 0)
        for index in 0..<100 {
            type("line \(index)", into: shell)
            shell.handle(.enter)
        }
        _ = shell.items(width: 640, height: 480, metrics: metrics)
        XCTAssertTrue(shell.scrollWheel(pixels: Double(metrics.cellHeight) * 2.5))
        XCTAssertEqual(shell.scrollback.scroll, 2)
        shell.scrollWheel(pixels: Double(metrics.cellHeight) * 0.5)
        XCTAssertEqual(shell.scrollback.scroll, 3, "the parts of a row add up")
        shell.handle(.pageUp)
        XCTAssertGreaterThan(shell.scrollback.scroll, 3)
        let items = shell.items(width: 640, height: 480, metrics: metrics)
        XCTAssertTrue(items.contains { if case let .text(text, _, _, _) = $0 { text.hasPrefix("↓") } else { false } })
        shell.handle(.pageDown)
        shell.handle(.pageDown)
        XCTAssertEqual(shell.scrollback.scroll, 0)
        type("x", into: shell)
        shell.scrollWheel(pixels: 100)
        shell.handle(.enter)
        XCTAssertEqual(shell.scrollback.scroll, 0, "a line run shows the newest")
    }

    // ---------- the log and the watch line

    func testTheLogIsFollowedWhileOpen() {
        let engine = FakeEngine()
        let shell = ShellOverlay(engine: engine)
        engine.log = [LogEntry(sequence: 1, level: "info", target: "sim", text: "spawned"),
                      LogEntry(sequence: 2, level: "warn", target: "ai", text: "no path"),
                      LogEntry(sequence: 3, level: "bogus", target: "x", text: "y")]
        shell.update(seconds: 0.016, now: 0)
        XCTAssertEqual(engine.logPolls, 0, "not while shut")
        shell.open()
        shell.update(seconds: 0.016, now: 0.016)
        XCTAssertEqual(shell.scrollback.lines.map { String($0.text) }, ["sim: spawned", "ai: no path", "x: y"])
        XCTAssertEqual(shell.scrollback.lines.map(\.kind), [.log(.info), .log(.warn), .log(.info)])
        XCTAssertEqual(shell.logSequence, 3)
        shell.update(seconds: 0.016, now: 0.032)
        XCTAssertEqual(shell.scrollback.lines.count, 3, "each line once")
        XCTAssertEqual(ShellOverlay.Colors.of(.log(.warn)), ShellOverlay.Colors.warning)
        XCTAssertEqual(ShellOverlay.Colors.of(.log(.error)), ShellOverlay.Colors.error)
    }

    func testTheWatchLine() {
        let engine = FakeEngine()
        let shell = ShellOverlay(engine: engine)
        // shut and unwatched: a poll a second, nothing drawn
        for frame in 0..<120 {
            shell.update(seconds: 1.0 / 60, now: Double(frame) / 60)
        }
        XCTAssertEqual(engine.watchPolls, 2)
        XCTAssertEqual(shell.items(width: 640, height: 480, metrics: metrics), [])
        XCTAssertFalse(shell.isVisible)

        engine.watched = [WatchValue(expression: "(game_difficulty_get)", value: "normal", ok: true),
                          WatchValue(expression: "(nope)", value: "no such function", ok: false)]
        shell.update(seconds: 1.0 / 60, now: 3)
        XCTAssertTrue(shell.isVisible)
        let polls = engine.watchPolls
        for frame in 0..<60 {
            shell.update(seconds: 1.0 / 60, now: 3 + Double(frame + 1) / 60)
        }
        XCTAssertEqual(engine.watchPolls - polls, 4, "four times a second while watching")
        let items = shell.items(width: 640, height: 480, metrics: metrics)
        let texts = items.compactMap { item -> (String, Int, SIMD4<Float>)? in
            if case let .text(text, _, y, color) = item { return (text, y, color) }
            return nil
        }
        XCTAssertEqual(texts.map(\.0), ["(game_difficulty_get) = normal", "  │  (nope) = no such function"])
        XCTAssertEqual(texts.map(\.1), [1, 1], "at the top of the screen, shut")
        XCTAssertEqual(texts.last?.2, ShellOverlay.Colors.error)
        // (cut to the width)
        let narrow = shell.items(width: 200, height: 480, metrics: metrics)
        for item in narrow {
            if case let .text(text, x, _, _) = item {
                XCTAssertLessThanOrEqual(x + text.count * metrics.cellWidth, 200)
            }
        }
        // a line run asks again at once
        shell.open()
        engine.watched = []
        type("unwatch", into: shell)
        shell.handle(.enter)
        XCTAssertTrue(shell.watchValues.isEmpty)
    }

    // ---------- drawing

    func testTheOverlaySlidesAndDrawsTheLine() throws {
        let shell = ShellOverlay(engine: FakeEngine())
        XCTAssertEqual(shell.items(width: 640, height: 480, metrics: metrics), [], "nothing while shut")
        shell.open()
        XCTAssertEqual(shell.openness, 0)
        shell.update(seconds: ShellOverlay.slideSeconds / 2, now: 0)
        XCTAssertEqual(shell.openness, 0.5, accuracy: 1e-9)
        let half = shell.items(width: 640, height: 480, metrics: metrics)
        guard case let .box(_, halfTop, _, panel, _) = try XCTUnwrap(half.first) else { return XCTFail("background") }
        XCTAssertEqual(panel, Int(480 * ShellOverlay.heightShare))
        XCTAssertEqual(halfTop, -panel / 2)
        shell.update(seconds: 1, now: 0.1)
        XCTAssertEqual(shell.openness, 1)
        type("spawn warthog", into: shell)
        shell.handle(.wordLeft)
        let items = shell.items(width: 640, height: 480, metrics: metrics)
        guard case .box(_, 0, 640, _, ShellOverlay.Colors.background) = try XCTUnwrap(items.first) else {
            return XCTFail("the background at the top")
        }
        XCTAssertTrue(items.contains(.text("] ", x: 8, y: panel - 1 - 16 - 4, color: ShellOverlay.Colors.input)))
        XCTAssertTrue(items.contains(.text("spawn warthog", x: 8 + 16, y: panel - 1 - 16 - 4,
                                           color: ShellOverlay.Colors.output)))
        XCTAssertTrue(items.contains(.box(x: 8 + 16 + 6 * 8, y: panel - 1 - 16 - 4, width: 2, height: 16,
                                          color: ShellOverlay.Colors.cursor)), "the cursor before warthog")
        // (a line longer than the width scrolls to the cursor)
        shell.handle(.end)
        type(String(repeating: "z", count: 200), into: shell)
        for item in shell.items(width: 640, height: 480, metrics: metrics) {
            if case let .text(text, x, _, _) = item {
                XCTAssertLessThanOrEqual(x + text.count * 8, 640 - 8, text)
            }
        }
        shell.close()
        shell.update(seconds: 1, now: 1)
        XCTAssertEqual(shell.openness, 0)
        XCTAssertEqual(shell.items(width: 640, height: 480, metrics: metrics), [])
    }

    func testTheOverlayIsDrawnByTheTextPass() throws {
        let device = try XCTUnwrap(MTLCreateSystemDefaultDevice())
        let atlas = try GlyphAtlas(url: try XCTUnwrap(GlyphAtlas.url()), scale: 1, device: device)
        let pass = try TextPass(device: device, atlas: atlas, outputFormat: .bgra8Unorm)
        let shell = ShellOverlay(engine: FakeEngine())
        shell.open()
        shell.update(seconds: 1, now: 0)
        type("help", into: shell)
        shell.handle(.enter)
        let descriptor = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: .bgra8Unorm, width: 320, height: 240,
                                                                  mipmapped: false)
        descriptor.usage = [.renderTarget]
        let target = try XCTUnwrap(device.makeTexture(descriptor: descriptor))
        let queue = try XCTUnwrap(device.makeCommandQueue())
        let commands = try XCTUnwrap(queue.makeCommandBuffer())
        pass.encode(shell.items(width: 320, height: 240, metrics: ShellMetrics(atlas: atlas)), into: commands,
                    target: target)
        commands.commit()
        commands.waitUntilCompleted()
        XCTAssertNil(commands.error)
        XCTAssertGreaterThan(pass.lastDrawn, "] help".count + "ran help".count)
    }
}
