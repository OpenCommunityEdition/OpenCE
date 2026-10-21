// A snapshot's objects as the renderer draws them: each object and each of
// its nodes `alpha` of the way from the tick before the last to the last.
import OpenCEEngineRender
import opence_ffiFFI
import simd

extension FrameView {
    /// The objects with a model, between the last two ticks.
    public func drawnObjects() -> [DrawnObject] {
        let alpha = min(max(alpha.isFinite ? alpha : 1, 0), 1)
        var drawn: [DrawnObject] = []
        drawn.reserveCapacity(objects.count)
        for object in objects where object.model != UInt32.max {
            let transform = Self.between(object.previous, object.current, alpha)
            let regions = withUnsafeBytes(of: object.regions) { Array($0) }
            let nodes = nodes(of: object).map { Self.between($0.previous, $0.current, alpha) }
            let radius = object.bounding_radius.isFinite ? max(object.bounding_radius, 0) : 0
            drawn.append(DrawnObject(model: Int(object.model), transform: transform,
                                     center: SIMD3(transform.columns.3.x, transform.columns.3.y,
                                                   transform.columns.3.z),
                                     radius: radius, regions: regions, nodes: nodes))
        }
        return drawn
    }

    static func vector(_ value: OCEVec3) -> SIMD3<Float> { SIMD3(value.x, value.y, value.z) }

    /// A transform's matrix, `alpha` of the way from `from` to `to`.
    static func between(_ from: OCETransform, _ to: OCETransform, _ alpha: Float) -> simd_float4x4 {
        let quaternion = { (rotation: OCEQuat) in
            simd_quatf(ix: rotation.x, iy: rotation.y, iz: rotation.z, r: rotation.w)
        }
        let rotation = simd_slerp(quaternion(from.rotation), quaternion(to.rotation), alpha)
        let position = simd_mix(vector(from.position), vector(to.position), SIMD3(repeating: alpha))
        let scale = from.scale + (to.scale - from.scale) * alpha
        return DrawnObject.matrix(forward: rotation.act([1, 0, 0]), left: rotation.act([0, 1, 0]),
                                  up: rotation.act([0, 0, 1]), position: position, scale: scale)
    }

    /// A node's matrix, `alpha` of the way from `from` to `to`: its
    /// rotation turned between, its position and scale mixed.
    static func between(_ from: OCENodeMatrix, _ to: OCENodeMatrix, _ alpha: Float) -> simd_float4x4 {
        let rotation = { (matrix: OCENodeMatrix) in
            simd_quatf(simd_float3x3(columns: (vector(matrix.forward), vector(matrix.left), vector(matrix.up))))
        }
        let turned = simd_slerp(rotation(from), rotation(to), alpha)
        let position = simd_mix(vector(from.position), vector(to.position), SIMD3(repeating: alpha))
        let scale = from.scale + (to.scale - from.scale) * alpha
        return DrawnObject.matrix(forward: turned.act([1, 0, 0]), left: turned.act([0, 1, 0]),
                                  up: turned.act([0, 0, 1]), position: position, scale: scale)
    }
}
