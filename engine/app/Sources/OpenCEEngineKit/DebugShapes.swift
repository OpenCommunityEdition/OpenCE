// A snapshot's debug lines and labels as the renderer draws them.
import OpenCEEngineRender
import opence_ffiFFI
import simd

extension FrameView {
    /// The debug lines.
    public func debugSegments() -> [DebugSegment] {
        debugLines.map { line in
            DebugSegment(from: SIMD3(line.from.x, line.from.y, line.from.z),
                         to: SIMD3(line.to.x, line.to.y, line.to.z), color: line.color)
        }
    }

    /// The debug labels, their text up to its first zero.
    public func debugMarks() -> [DebugMark] {
        debugLabels.map { label in
            let bytes = withUnsafeBytes(of: label.text) { raw in Array(raw.prefix { $0 != 0 }) }
            return DebugMark(at: SIMD3(label.at.x, label.at.y, label.at.z), color: label.color,
                             text: String(decoding: bytes, as: UTF8.self))
        }
    }
}
