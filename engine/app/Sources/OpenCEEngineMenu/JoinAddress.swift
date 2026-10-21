// An address typed to join a game: a host name or IP address, with a
// port or Halo's (2302); an IPv6 address with a port goes in brackets.
import Foundation

public struct JoinAddress: Equatable, Sendable {
    /// Halo's port.
    public static let defaultPort: UInt16 = 2302

    public let host: String
    public let port: UInt16

    public init(host: String, port: UInt16 = JoinAddress.defaultPort) {
        self.host = host
        self.port = port
    }

    /// The address `text` names, or nil if it names none.
    public static func parse(_ text: String) -> JoinAddress? {
        let text = text.trimmingCharacters(in: .whitespaces)
        let host: Substring
        let port: Substring?
        if text.hasPrefix("[") {
            // ([::1] or [::1]:2302)
            guard let close = text.firstIndex(of: "]") else { return nil }
            host = text[text.index(after: text.startIndex)..<close]
            let rest = text[text.index(after: close)...]
            guard rest.isEmpty || rest.hasPrefix(":") else { return nil }
            port = rest.isEmpty ? nil : rest.dropFirst()
        } else if text.filter({ $0 == ":" }).count > 1 {
            // (an IPv6 address without a port)
            host = Substring(text)
            port = nil
        } else if let colon = text.firstIndex(of: ":") {
            host = text[..<colon]
            port = text[text.index(after: colon)...]
        } else {
            host = Substring(text)
            port = nil
        }
        guard !host.isEmpty, !host.contains(where: { $0.isWhitespace || $0 == "/" }) else { return nil }
        guard let port else { return JoinAddress(host: String(host)) }
        guard port.allSatisfy(\.isASCII), let number = UInt16(port), number > 0 else { return nil }
        return JoinAddress(host: String(host), port: number)
    }

    /// As the engine takes it: `host:port`, an IPv6 host in brackets.
    public var text: String {
        host.contains(":") ? "[\(host)]:\(port)" : "\(host):\(port)"
    }
}
