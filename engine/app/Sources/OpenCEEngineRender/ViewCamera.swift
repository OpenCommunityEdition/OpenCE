// The camera the level is drawn from, and its projection: Hor+ (the
// vertical field of view fixed, the horizontal one growing with the
// aspect ratio) with an infinite far plane and reversed depth (1 at the
// near plane, 0 at infinity), for even precision across Halo's distances.

import simd

/// Where the camera is and which way it faces, in Halo's world: Z up, the
/// camera looking along its rotated +X, its +Y to the left.
public struct ViewCamera: Equatable, Sendable {
    public var position: SIMD3<Float>
    public var rotation: simd_quatf
    /// The vertical field of view, in radians.
    public var verticalFov: Float

    public init(position: SIMD3<Float>, rotation: simd_quatf, verticalFov: Float) {
        self.position = position
        self.rotation = rotation
        self.verticalFov = verticalFov
    }

    /// A camera at `position` turned `yaw` left from +X and `pitch` up, as
    /// the simulation's free camera turns.
    public init(position: SIMD3<Float>, yaw: Float, pitch: Float, verticalFov: Float) {
        let turn = simd_quatf(angle: yaw, axis: [0, 0, 1])
        let tilt = simd_quatf(angle: -pitch, axis: [0, 1, 0])
        self.init(position: position, rotation: turn * tilt, verticalFov: verticalFov)
    }

    /// The camera `alpha` of the way from `from` to `to` (a tick's
    /// snapshots, drawn between them).
    public static func interpolated(from: ViewCamera, to: ViewCamera, alpha: Float) -> ViewCamera {
        let alpha = min(max(alpha.isFinite ? alpha : 1, 0), 1)
        return ViewCamera(position: simd_mix(from.position, to.position, SIMD3(repeating: alpha)),
                          rotation: simd_slerp(from.rotation, to.rotation, alpha),
                          verticalFov: to.verticalFov)
    }

    /// The nearest distance drawn, in world units (a world unit is about
    /// 3 m).
    public static let near: Float = 0.01

    /// World to clip space, for a target `aspect` (width over height).
    public func viewProjection(aspect: Float) -> simd_float4x4 {
        let forward = rotation.act([1, 0, 0])
        let left = rotation.act([0, 1, 0])
        let up = rotation.act([0, 0, 1])
        let fov = min(max(verticalFov, 0.01), Float.pi - 0.01)
        let focalY = 1 / tan(fov / 2)
        let focalX = focalY / max(aspect, 0.01)
        let near = Self.near
        // (view space: x right, y up, w the distance ahead; z the near
        // plane's, so that depth is near over distance)
        return simd_float4x4(rows: [
            SIMD4(-left * focalX, dot(left, position) * focalX),
            SIMD4(up * focalY, -dot(up, position) * focalY),
            SIMD4(0, 0, 0, near),
            SIMD4(forward, -dot(forward, position)),
        ])
    }
}
