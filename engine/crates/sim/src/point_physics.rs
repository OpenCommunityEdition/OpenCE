//! A point's physics (`point_physics_update`): what moves the particles
//! and the flames. Gravity pulls it by its buoyancy in the air or water
//! about it, that air or water drags it toward the wind, and it meets the
//! world up to three times a tick, bouncing by its elasticity and sliding
//! by its contact friction.

use glam::Vec3;
use opence_content::PointPhysics;

use crate::particle_collision::PointHit;

/// The game's gravity in world units a second a second (0.0035651792 a
/// tick a tick).
pub const GRAVITY: f32 = 0.003_565_179_2 * 900.0;
/// The density of air, grams a millilitre.
pub const AIR_DENSITY: f32 = 0.0011;
/// The density of water, grams a millilitre.
pub const WATER_DENSITY: f32 = 1.0;
/// Grams a millilitre in the game's mass a cubic world unit.
pub const DENSITY_SCALE: f32 = 118_613.34;
/// The most surfaces a point meets in one update.
pub const MAXIMUM_COLLISIONS: usize = 3;
/// The most a point that met a surface is put off it (less for a smaller
/// point: its radius).
pub const MAXIMUM_OFFSET: f32 = 0.005;

/// What an update found.
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct Moved {
    /// It was in water (otherwise in air).
    pub in_water: bool,
    /// The last surface it met in the update, if any.
    pub collision: Option<PointHit>,
}

impl Moved {
    /// It met the world's structure.
    pub fn collided(&self) -> bool {
        self.collision.is_some()
    }
}

/// A point of `physics`'s mass for `radius`, in air or water.
pub fn mass(physics: &PointPhysics, radius: f32, in_water: bool) -> f32 {
    let medium = if in_water { WATER_DENSITY } else { AIR_DENSITY };
    let cubed = radius * radius * radius;
    (physics.mass_over_radius_cubed + medium * DENSITY_SCALE) * cubed
}

/// Moves a point of `physics` and `radius` at `position` with `velocity`
/// for `dt` seconds, in water if `in_water` and otherwise in air, blown
/// toward `wind`. If its physics meets the structure, `collide` gives
/// where a path (from, motion) first meets the world. A radius below 0
/// is taken as 0.
pub fn update(
    physics: &PointPhysics,
    position: &mut Vec3,
    velocity: &mut Vec3,
    radius: f32,
    dt: f32,
    medium: (bool, Vec3),
    mut collide: impl FnMut(Vec3, Vec3) -> Option<PointHit>,
) -> Moved {
    let (in_water, wind) = medium;
    let mut moved = Moved {
        in_water,
        collision: None,
    };
    if dt == 0.0 {
        return moved;
    }
    let radius = if radius >= 0.0 { radius } else { 0.0 };
    let squared = radius * radius;
    let mass = mass(physics, radius, in_water);
    let (mut buoyancy, friction) = if in_water {
        (physics.water_buoyancy, physics.water_friction * squared)
    } else {
        (physics.air_buoyancy, physics.air_friction * squared)
    };
    if physics.flags & PointPhysics::NO_GRAVITY != 0 {
        buoyancy = 0.0;
    }
    velocity.z += GRAVITY * buoyancy * dt;
    let t = if mass == 0.0 {
        if friction == 0.0 {
            0.0
        } else {
            1.0
        }
    } else {
        (dt / mass * friction).clamp(0.0, 1.0)
    };
    *velocity += (wind - *velocity) * t;

    let collides = physics.flags & PointPhysics::STRUCTURE_COLLISIONS != 0;
    let offset = radius.min(MAXIMUM_OFFSET);
    let mut dt = dt;
    for _ in 0..MAXIMUM_COLLISIONS {
        if dt == 0.0 {
            break;
        }
        let delta = *velocity * dt;
        let hit = if collides {
            collide(*position, delta)
        } else {
            None
        };
        let Some(hit) = hit else {
            *position += delta;
            break;
        };
        // (the original keeps what it moved into the surface turned back by
        // its elasticity, unclamped, and what it moved along it slowed by
        // its contact friction, whichever way it was moving)
        let into = hit.normal * velocity.dot(hit.normal);
        let along = *velocity - into;
        *velocity = along * (1.0 - physics.contact_friction) - into * physics.elasticity;
        *position = hit.point + hit.normal * offset;
        dt -= hit.fraction * dt;
        moved.collision = Some(hit);
    }
    moved
}

/// An attached point's velocity after `dt`: its air's friction slows it,
/// and nothing else moves it.
pub fn damped(physics: &PointPhysics, radius: f32, velocity: Vec3, dt: f32) -> Vec3 {
    let radius = radius.max(0.0);
    let friction = physics.air_friction * radius * radius;
    let mass = physics.mass_over_radius_cubed * radius * radius * radius;
    let scale = if mass == 0.0 {
        if friction == 0.0 {
            1.0
        } else {
            0.0
        }
    } else {
        (1.0 - friction / mass * dt).clamp(0.0, 1.0)
    };
    velocity * scale
}

#[cfg(test)]
mod tests {
    use super::*;

    const TICK: f32 = 1.0 / 30.0;

    /// A point as heavy as rock, falling in air with no drag.
    fn rock() -> PointPhysics {
        PointPhysics {
            flags: PointPhysics::STRUCTURE_COLLISIONS,
            mass_over_radius_cubed: 2.5 * DENSITY_SCALE,
            water_buoyancy: -0.6,
            air_buoyancy: -1.0,
            density: 2.5,
            air_friction: 0.0,
            water_friction: 0.0,
            contact_friction: 0.0,
            elasticity: 1.0,
        }
    }

    fn floor(from: Vec3, motion: Vec3) -> Option<PointHit> {
        // (the plane z = 0, from above)
        let to = from + motion;
        if from.z < 0.0 || to.z >= 0.0 {
            return None;
        }
        let fraction = from.z / (from.z - to.z);
        Some(PointHit {
            point: from + motion * fraction,
            normal: Vec3::Z,
            fraction,
        })
    }

    #[test]
    fn gravity_pulls_by_the_buoyancy_and_not_without_it() {
        let physics = rock();
        let (mut at, mut velocity) = (Vec3::new(0.0, 0.0, 10.0), Vec3::ZERO);
        let moved = update(
            &physics,
            &mut at,
            &mut velocity,
            0.1,
            TICK,
            (false, Vec3::ZERO),
            floor,
        );
        assert!(!moved.in_water && !moved.collided());
        assert!((velocity.z + GRAVITY * TICK).abs() < 1e-6, "{velocity}");
        assert!((at.z - (10.0 + velocity.z * TICK)).abs() < 1e-6);
        // (in water, by its water buoyancy)
        let mut velocity = Vec3::ZERO;
        let moved = update(
            &physics,
            &mut at,
            &mut velocity,
            0.1,
            TICK,
            (true, Vec3::ZERO),
            floor,
        );
        assert!(moved.in_water);
        assert!((velocity.z + 0.6 * GRAVITY * TICK).abs() < 1e-6);
        // (and none at all with no gravity)
        let floating = PointPhysics {
            flags: PointPhysics::NO_GRAVITY,
            ..physics
        };
        let mut velocity = Vec3::ZERO;
        update(
            &floating,
            &mut at,
            &mut velocity,
            0.1,
            TICK,
            (false, Vec3::ZERO),
            floor,
        );
        assert_eq!(velocity, Vec3::ZERO);
        // (and no time moves nothing)
        let before = at;
        update(
            &physics,
            &mut at,
            &mut velocity,
            0.1,
            0.0,
            (false, Vec3::ZERO),
            floor,
        );
        assert_eq!(at, before);
    }

    #[test]
    fn drag_brings_it_toward_the_wind() {
        let physics = PointPhysics {
            flags: PointPhysics::NO_GRAVITY,
            mass_over_radius_cubed: 0.0,
            air_friction: 0.5,
            ..rock()
        };
        let wind = Vec3::new(2.0, 0.0, 0.0);
        let mut velocity = Vec3::ZERO;
        let mut at = Vec3::ZERO;
        let radius = 0.05;
        update(
            &physics,
            &mut at,
            &mut velocity,
            radius,
            TICK,
            (false, wind),
            floor,
        );
        let mass = AIR_DENSITY * DENSITY_SCALE * radius * radius * radius;
        let t = (TICK / mass * 0.5 * radius * radius).clamp(0.0, 1.0);
        assert!((velocity.x - 2.0 * t).abs() < 1e-5, "{velocity} {t}");
        assert!(velocity.x > 0.0 && velocity.x <= 2.0);
        // (a point with no mass but some friction goes with the wind at
        // once, with neither it stays as it was)
        let mut velocity = Vec3::ZERO;
        update(
            &physics,
            &mut at,
            &mut velocity,
            0.0,
            TICK,
            (false, wind),
            floor,
        );
        assert_eq!(velocity, Vec3::ZERO);
        let weightless = PointPhysics {
            mass_over_radius_cubed: -AIR_DENSITY * DENSITY_SCALE,
            ..physics
        };
        let mut velocity = Vec3::ZERO;
        update(
            &weightless,
            &mut at,
            &mut velocity,
            radius,
            TICK,
            (false, wind),
            floor,
        );
        assert!(velocity.abs_diff_eq(wind, 1e-6), "{velocity}");
    }

    #[test]
    fn it_bounces_slides_and_stops_by_its_surface_factors() {
        let start = Vec3::new(0.0, 0.0, 0.05);
        let falling = Vec3::new(1.5, 0.0, -3.0);
        let run = |physics: &PointPhysics| {
            let (mut at, mut velocity) = (start, falling);
            let moved = update(
                physics,
                &mut at,
                &mut velocity,
                0.002,
                TICK,
                (false, Vec3::ZERO),
                floor,
            );
            (at, velocity, moved)
        };
        let no_gravity = PointPhysics::NO_GRAVITY | PointPhysics::STRUCTURE_COLLISIONS;
        let bouncy = PointPhysics {
            flags: no_gravity,
            ..rock()
        };
        let (at, velocity, moved) = run(&bouncy);
        assert_eq!(moved.collision.map(|hit| hit.normal), Some(Vec3::Z));
        assert!(
            velocity.abs_diff_eq(Vec3::new(1.5, 0.0, 3.0), 1e-5),
            "{velocity}"
        );
        // (it went on from the bounce with the rest of the tick)
        assert!(at.z > 0.002, "{at}");
        // a dead point stops into the surface and slides along it
        let dead = PointPhysics {
            elasticity: 0.0,
            ..bouncy
        };
        let (at, velocity, _) = run(&dead);
        assert!(velocity.abs_diff_eq(Vec3::new(1.5, 0.0, 0.0), 1e-5));
        // (put off the surface by its radius, below the most)
        assert!((at.z - 0.002).abs() < 1e-6, "{at}");
        // and with contact friction it stops dead
        let stopped = PointPhysics {
            contact_friction: 1.0,
            ..dead
        };
        let (_, velocity, _) = run(&stopped);
        assert!(velocity.abs_diff_eq(Vec3::ZERO, 1e-6));
        // (elasticity is not held to 1: it can come back faster)
        let lively = PointPhysics {
            elasticity: 2.0,
            ..bouncy
        };
        let (_, velocity, _) = run(&lively);
        assert!((velocity.z - 6.0).abs() < 1e-4);
        // a point whose physics meets no structure passes through it
        let ghost = PointPhysics {
            flags: PointPhysics::NO_GRAVITY,
            ..rock()
        };
        let (at, _, moved) = run(&ghost);
        assert!(!moved.collided() && at.z < 0.0);
    }

    #[test]
    fn it_meets_the_world_at_most_three_times_a_tick() {
        let physics = PointPhysics {
            flags: PointPhysics::NO_GRAVITY | PointPhysics::STRUCTURE_COLLISIONS,
            ..rock()
        };
        let mut casts = 0;
        let (mut at, mut velocity) = (Vec3::ZERO, Vec3::X);
        update(
            &physics,
            &mut at,
            &mut velocity,
            0.1,
            TICK,
            (false, Vec3::ZERO),
            |from, motion| {
                casts += 1;
                // (a wall at once, every way it turns)
                Some(PointHit {
                    point: from,
                    normal: -motion.normalize_or_zero(),
                    fraction: 0.0,
                })
            },
        );
        assert_eq!(casts, MAXIMUM_COLLISIONS);
        assert!(at.is_finite());
        // (a point below 0 in radius is a point of none)
        let physics = PointPhysics {
            elasticity: 0.0,
            ..physics
        };
        let (mut at, mut velocity) = (Vec3::new(0.0, 0.0, 0.01), Vec3::NEG_Z);
        let moved = update(
            &physics,
            &mut at,
            &mut velocity,
            -1.0,
            TICK,
            (false, Vec3::ZERO),
            floor,
        );
        assert!(moved.collided());
        assert_eq!(at.z, 0.0);
    }

    #[test]
    fn an_attached_point_is_only_slowed_by_its_air() {
        let physics = PointPhysics {
            air_friction: 0.25,
            mass_over_radius_cubed: 1.0,
            ..rock()
        };
        let slowed = damped(&physics, 1.0, Vec3::X * 2.0, 1.0);
        assert!(slowed.abs_diff_eq(Vec3::X * 1.5, 1e-6));
        // (held to stopping, never turned back)
        assert_eq!(damped(&physics, 1.0, Vec3::X, 10.0), Vec3::ZERO);
        let massless = PointPhysics {
            mass_over_radius_cubed: 0.0,
            ..physics
        };
        assert_eq!(damped(&massless, 1.0, Vec3::X, 1.0), Vec3::ZERO);
        let free = PointPhysics {
            air_friction: 0.0,
            ..massless
        };
        assert_eq!(damped(&free, 1.0, Vec3::X, 1.0), Vec3::X);
    }

    #[test]
    fn mass_counts_the_air_or_water_it_displaces() {
        let physics = rock();
        let air = mass(&physics, 1.0, false);
        let water = mass(&physics, 1.0, true);
        assert!((air - (2.5 + AIR_DENSITY) * DENSITY_SCALE).abs() < 1.0);
        assert!(water > air);
    }
}
