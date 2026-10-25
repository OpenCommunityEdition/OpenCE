// The developer shell's history: the lines run, oldest first, without
// blanks or a line repeating the one before; Up and Down walk it keeping
// the line being typed, Control-R searches it, and it is kept in a file
// between runs (shell_history.txt in the app's support folder).
import Foundation

/// The lines run in the shell.
public struct ShellHistory: Equatable, Sendable {
    /// The most lines kept.
    public static let capacity = 500

    public private(set) var entries: [String] = []
    /// The entry Up and Down are at, or nil at the line being typed.
    public private(set) var position: Int?
    /// The line being typed when Up was first pressed.
    private var draft = ""

    public init(_ entries: [String] = []) {
        entries.forEach { add($0) }
    }

    /// Adds a line run, and goes back to the line being typed.
    public mutating func add(_ line: String) {
        reset()
        let trimmed = line.trimmingCharacters(in: .whitespaces)
        guard !trimmed.isEmpty, !trimmed.contains(where: \.isNewline), entries.last != trimmed else { return }
        entries.append(trimmed)
        if entries.count > Self.capacity {
            entries.removeFirst(entries.count - Self.capacity)
        }
    }

    /// Back at the line being typed.
    public mutating func reset() {
        position = nil
        draft = ""
    }

    /// Up: the entry before, `current` kept as the line being typed when
    /// leaving it; nil at the oldest.
    public mutating func previous(current: String) -> String? {
        let index: Int
        if let position {
            guard position > 0 else { return nil }
            index = position - 1
        } else {
            guard !entries.isEmpty else { return nil }
            draft = current
            index = entries.count - 1
        }
        position = index
        return entries[index]
    }

    /// Down: the entry after, else the line that was being typed; nil
    /// there already.
    public mutating func next() -> String? {
        guard let position else { return nil }
        if position + 1 < entries.count {
            self.position = position + 1
            return entries[position + 1]
        }
        self.position = nil
        return draft
    }

    /// The newest entry before `before` (before the end for nil) that
    /// contains `query`, ignoring case.
    public func search(_ query: String, before: Int? = nil) -> Int? {
        guard !query.isEmpty else { return nil }
        var index = min(before ?? entries.count, entries.count) - 1
        while index >= 0 {
            if entries[index].range(of: query, options: .caseInsensitive) != nil {
                return index
            }
            index -= 1
        }
        return nil
    }

    /// Writes the entries to `url`, a line each.
    public func save(to url: URL) throws {
        try FileManager.default.createDirectory(at: url.deletingLastPathComponent(), withIntermediateDirectories: true)
        let text = entries.isEmpty ? "" : entries.joined(separator: "\n") + "\n"
        try Data(text.utf8).write(to: url, options: .atomic)
    }

    /// The history saved at `url`, or an empty one (the file missing or
    /// unreadable, or larger than a history is).
    public static func load(from url: URL) -> ShellHistory {
        guard let attributes = try? FileManager.default.attributesOfItem(atPath: url.path),
            let size = attributes[.size] as? Int, size <= capacity * (LineEditor.maxLength + 1),
            let data = try? Data(contentsOf: url)
        else { return ShellHistory() }
        let text = String(decoding: data, as: UTF8.self)
        return ShellHistory(text.split(whereSeparator: \.isNewline).map { LineEditor.printable(String($0)) })
    }

    /// Where the app keeps it.
    public static var file: URL? {
        FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask).first?
            .appendingPathComponent("OpenCE Engine/shell_history.txt")
    }
}
