//! The particles' collision with the world: where a point moving through
//! the level meets it (the level and the machines, not the bodies lying
//! about), and how it goes on from there. The particles ask it each tick
//! for the path they would move along.

use glam::Vec3;

/// Where a point's path met the world.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct PointHit {
    /// Where.
    pub point: Vec3,
    /// The surface's normal there, toward the point.
    pub normal: Vec3,
    /// How far along the path, 0 to 1.
    pub fraction: f32,
}

/// A point's velocity after it met a surface of `normal`: what it moved
/// into the surface turned back by `elasticity` (0 stops it, 1 bounces it
/// as fast as it came), and what it moved along it slowed by `friction`
/// (0 slides freely, 1 stops it).
pub fn bounce(velocity: Vec3, normal: Vec3, elasticity: f32, friction: f32) -> Vec3 {
    let into = velocity.dot(normal);
    if into >= 0.0 {
        // (moving away from it already)
        return velocity;
    }
    let along = velocity - normal * into;
    along * (1.0 - friction.clamp(0.0, 1.0)) - normal * into * elasticity.clamp(0.0, 1.0)
}

/// Where a point moving at `velocity` that met the world at `hit` is put,
/// and how fast it goes on: at the hit, a little off the surface, its
/// velocity bounced. The rest of its move that tick is left off.
pub fn rebound(hit: &PointHit, velocity: Vec3, elasticity: f32, friction: f32) -> (Vec3, Vec3) {
    (
        hit.point + hit.normal * OFF_SURFACE,
        bounce(velocity, hit.normal, elasticity, friction),
    )
}

/// How far off a surface a point that met it is put, so that its next
/// path starts in front of it.
pub const OFF_SURFACE: f32 = 1e-3;

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn a_point_bounces_by_its_elasticity_and_slides_by_its_friction() {
        let velocity = Vec3::new(2.0, 0.0, -4.0);
        let up = Vec3::Z;
        assert!(bounce(velocity, up, 1.0, 0.0).abs_diff_eq(Vec3::new(2.0, 0.0, 4.0), 1e-6));
        assert!(bounce(velocity, up, 0.5, 0.0).abs_diff_eq(Vec3::new(2.0, 0.0, 2.0), 1e-6));
        // (it stops dead, slides on, or slows along the surface)
        assert!(bounce(velocity, up, 0.0, 1.0).abs_diff_eq(Vec3::ZERO, 1e-6));
        assert!(bounce(velocity, up, 0.0, 0.0).abs_diff_eq(Vec3::new(2.0, 0.0, 0.0), 1e-6));
        assert!(bounce(velocity, up, 0.0, 0.25).abs_diff_eq(Vec3::new(1.5, 0.0, 0.0), 1e-6));
        // (out of range, the factors are held to 0 to 1)
        assert!(bounce(velocity, up, 3.0, -1.0).abs_diff_eq(Vec3::new(2.0, 0.0, 4.0), 1e-6));
        // a point leaving the surface is left alone
        let leaving = Vec3::new(1.0, 0.0, 3.0);
        assert_eq!(bounce(leaving, up, 0.5, 0.5), leaving);
    }

    #[test]
    fn a_rebound_puts_the_point_just_off_the_surface() {
        let hit = PointHit {
            point: Vec3::new(1.0, 2.0, 0.0),
            normal: Vec3::NEG_X,
            fraction: 0.5,
        };
        let (at, velocity) = rebound(&hit, Vec3::new(3.0, 0.0, 0.0), 1.0, 0.0);
        assert!(at.abs_diff_eq(Vec3::new(1.0 - OFF_SURFACE, 2.0, 0.0), 1e-6));
        assert!(velocity.abs_diff_eq(Vec3::new(-3.0, 0.0, 0.0), 1e-6));
    }
}
