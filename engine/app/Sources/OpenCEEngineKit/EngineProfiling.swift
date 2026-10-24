// The engine's profiler times the app's scopes (OpenCEEngineRender's
// `profiled`), with its own, while `stat frame` or `trace` measures.
import OpenCEEngineCore
import OpenCEEngineRender

extension Engine: ScopeTimer {}
