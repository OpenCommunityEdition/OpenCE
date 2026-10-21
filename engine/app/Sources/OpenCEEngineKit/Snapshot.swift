// The app's side of the contract (engine/crates/ffi): the engine, and the
// per-frame snapshots read in place from its triple buffer.
import OpenCEEngineCore
import opence_ffiFFI

/// The layout this app was compiled against differs from the library's.
public struct SnapshotLayoutMismatch: Error, CustomStringConvertible {
    public let field: String
    public let library: Int
    public let header: Int
    public var description: String { "snapshot layout: \(field) is \(library) in the library, \(header) in the header" }
}

/// Checks the library's snapshot layout against the header's.
public func checkSnapshotLayout() throws {
    let layout = snapshotLayout()
    let checks: [(String, UInt32, Int?)] = [
        ("version", layout.version, Int(OCE_SNAPSHOT_VERSION)),
        ("max_objects", layout.maxObjects, Int(OCE_MAX_OBJECTS)),
        ("max_nodes", layout.maxNodes, Int(OCE_MAX_NODES)),
        ("FrameSnapshot size", layout.frameSize, MemoryLayout<OCEFrameSnapshot>.size),
        ("FrameSnapshot.tick", layout.frameTickOffset, MemoryLayout<OCEFrameSnapshot>.offset(of: \.tick)),
        ("FrameSnapshot.alpha", layout.frameAlphaOffset, MemoryLayout<OCEFrameSnapshot>.offset(of: \.alpha)),
        ("FrameSnapshot.camera", layout.frameCameraOffset, MemoryLayout<OCEFrameSnapshot>.offset(of: \.camera)),
        ("FrameSnapshot.objects", layout.frameObjectsOffset, MemoryLayout<OCEFrameSnapshot>.offset(of: \.objects)),
        ("FrameSnapshot.nodes", layout.frameNodesOffset, MemoryLayout<OCEFrameSnapshot>.offset(of: \.nodes)),
        ("CameraSnapshot size", layout.cameraSize, MemoryLayout<OCECameraSnapshot>.size),
        ("ObjectSnapshot size", layout.objectSize, MemoryLayout<OCEObjectSnapshot>.size),
        ("ObjectSnapshot.current", layout.objectCurrentOffset, MemoryLayout<OCEObjectSnapshot>.offset(of: \.current)),
        ("ObjectSnapshot.first_node", layout.objectFirstNodeOffset, MemoryLayout<OCEObjectSnapshot>.offset(of: \.first_node)),
        ("Transform size", layout.transformSize, MemoryLayout<OCETransform>.size),
        ("NodeSnapshot size", layout.nodeSize, MemoryLayout<OCENodeSnapshot>.size),
    ]
    for (field, library, header) in checks where Int(library) != header {
        throw SnapshotLayoutMismatch(field: field, library: Int(library), header: header ?? -1)
    }
}

/// A snapshot read in place. It is valid until the next `acquire` on the
/// same reader, so it must not be kept past the frame it was taken for.
public struct FrameView {
    public let pointer: UnsafePointer<OCEFrameSnapshot>

    public var tick: UInt64 { pointer.pointee.tick }
    public var alpha: Float { pointer.pointee.alpha }
    public var camera: OCECameraSnapshot { pointer.pointee.camera }
    public var objects: UnsafeBufferPointer<OCEObjectSnapshot> {
        UnsafeBufferPointer(start: pointer.pointee.objects, count: Int(pointer.pointee.object_count))
    }
    /// Every animated object's nodes; an object's are `node_count` from its
    /// `first_node`.
    public var nodes: UnsafeBufferPointer<OCENodeSnapshot> {
        UnsafeBufferPointer(start: pointer.pointee.nodes, count: Int(pointer.pointee.node_count))
    }
    /// One object's nodes.
    public func nodes(of object: OCEObjectSnapshot) -> UnsafeBufferPointer<OCENodeSnapshot> {
        let all = nodes
        let first = min(Int(object.first_node), all.count)
        let count = min(Int(object.node_count), all.count - first)
        return UnsafeBufferPointer(rebasing: all[first..<first + count])
    }
}

/// The one reader of an engine's snapshots (the render thread's).
public final class SnapshotReader {
    // (kept so that the buffers outlive the reader)
    private let engine: Engine
    private let buffers: OpaquePointer

    public init(engine: Engine) {
        self.engine = engine
        guard let buffers = OpaquePointer(bitPattern: UInt(engine.snapshotBuffers())) else {
            preconditionFailure("the engine has no snapshot buffers")
        }
        self.buffers = buffers
    }

    /// The newest snapshot.
    public func acquire() -> FrameView {
        guard let frame = opence_snapshot_acquire(buffers) else {
            preconditionFailure("the engine's buffers are always valid while it lives")
        }
        return FrameView(pointer: frame)
    }
}
