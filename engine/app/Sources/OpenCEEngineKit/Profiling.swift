// The app's profiling marks: its scopes go into the engine's profiler, with
// the engine's own, while `stat frame` or `trace` measures.
import OpenCEEngineCore

/// What times named scopes: the engine's profiler.
public protocol ScopeTimer: AnyObject {
    /// Opens a scope, if frames are measured (false if not).
    func profileBegin(name: String) -> Bool
    /// Closes the innermost scope opened.
    func profileEnd()
}

extension Engine: ScopeTimer {}

/// Runs `body`, timed as the scope `name` when `timer` measures.
@inlinable
public func profiled<T>(_ name: String, _ timer: ScopeTimer?, _ body: () throws -> T) rethrows -> T {
    guard let timer, timer.profileBegin(name: name) else {
        return try body()
    }
    defer { timer.profileEnd() }
    return try body()
}
