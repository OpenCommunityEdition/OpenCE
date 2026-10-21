//! Debris: the particles a shot's effect throws out that are heavy enough
//! to be bodies (spent casings), as the C port's `physics_debris.c` makes
//! them. Each is a capsule along the way it flies, as long as the particle
//! is wide, tumbling end over end; once it has lain still for a moment its
//! body goes and it stays where it came to rest, until the cap (a setting)
//! takes the oldest.

use std::collections::BTreeMap;

use glam::{Quat, Vec3};
use opence_content::{EffectDebris, Level, TagRef};
use opence_physics::{BodyId, Material, Motion, Pose, Shape, World as PhysicsWorld};

use crate::random::RandomSeed;

/// The most pieces at once, unless set otherwise (the C port's
/// `game.physics_debris_max`).
pub const DEBRIS_DEFAULT: usize = 256;
/// The fastest a piece leaves, in world units a second.
pub const SPEED_MAX: f32 = 250.0;
/// Slower than this, in world units a second, a piece is still.
pub const REST_SPEED: f32 = 0.25;
/// The ticks a piece is still for before it rests (the C port's 15 at 30
/// ticks a second).
pub const REST_TICKS: u32 = 30;
/// The seed debris draws from: its own, so that casings do not change the
/// game's numbers (the effects' local seed, in the game).
const SEED: u32 = 0x0DEB_0215;

/// Where a shot's effect is made: its frame in the world.
#[derive(Clone, Debug, PartialEq)]
pub struct Ejection {
    /// The effect.
    pub effect: TagRef,
    /// Where it is.
    pub position: Vec3,
    /// Its turn: its x axis the way it faces.
    pub rotation: Quat,
    /// How fast what made it moves, in world units a second.
    pub velocity: Vec3,
}

/// A piece of debris.
#[derive(Clone, Debug, PartialEq)]
pub struct Piece {
    /// Its body, until it rests.
    body: Option<BodyId>,
    /// Where it is.
    pub pose: Pose,
    /// Its capsule's radius, in world units.
    pub radius: f32,
    /// Half its capsule's length, in world units (along its x axis).
    pub half_length: f32,
    /// The ticks it has been still for.
    still: u32,
}

impl Piece {
    /// Whether it has come to rest (and lost its body).
    pub fn resting(&self) -> bool {
        self.body.is_none()
    }
}

/// The debris of a game.
#[derive(Clone, Debug)]
pub struct Debris {
    /// Each effect's particles that are bodies, by the effect's tag.
    effects: BTreeMap<TagRef, Vec<EffectDebris>>,
    /// The pieces, oldest first.
    pieces: Vec<Piece>,
    /// The most pieces at once.
    cap: usize,
    seed: RandomSeed,
}

impl Default for Debris {
    fn default() -> Self {
        Debris {
            effects: BTreeMap::new(),
            pieces: Vec::new(),
            cap: DEBRIS_DEFAULT,
            seed: RandomSeed(SEED),
        }
    }
}

impl Debris {
    /// No pieces, for `level`'s effects, keeping the cap.
    pub fn for_level(&self, level: &Level) -> Debris {
        Debris {
            effects: level
                .effects
                .iter()
                .filter(|effect| !effect.debris.is_empty())
                .map(|effect| (effect.tag.clone(), effect.debris.clone()))
                .collect(),
            pieces: Vec::new(),
            cap: self.cap,
            seed: RandomSeed(SEED),
        }
    }

    /// The pieces, oldest first.
    pub fn pieces(&self) -> &[Piece] {
        &self.pieces
    }

    /// Whether any piece still moves (its body steps).
    pub fn moving(&self) -> bool {
        self.pieces.iter().any(|piece| !piece.resting())
    }

    /// Sets the most pieces at once (at least one); the oldest past it go
    /// when the next is thrown.
    pub fn set_cap(&mut self, cap: usize) {
        self.cap = cap.max(1);
    }

    /// Removes every piece.
    pub fn clear(&mut self, physics: &mut PhysicsWorld) {
        self.trim(physics, 0);
    }

    fn trim(&mut self, physics: &mut PhysicsWorld, keep: usize) {
        let excess = self.pieces.len().saturating_sub(keep);
        for piece in self.pieces.drain(..excess) {
            if let Some(body) = piece.body {
                physics.remove_body(body);
            }
        }
    }

    /// Throws out each of `ejection`'s effect's pieces.
    pub fn eject(&mut self, physics: &mut PhysicsWorld, ejection: &Ejection) {
        let Some(kinds) = self.effects.get(&ejection.effect) else {
            return;
        };
        let mut thrown = Vec::new();
        for kind in kinds {
            let [lower, upper] = kind.count;
            let count = if upper > lower {
                self.seed.next_range(lower, upper.saturating_add(1))
            } else {
                lower
            };
            for _ in 0..count.max(0) {
                let speed = lerp(kind.velocity[0], kind.velocity[1], self.seed.next_real());
                let direction = cone(
                    direction(kind.direction),
                    kind.velocity_cone_angle,
                    &mut self.seed,
                );
                let velocity = ejection.rotation * direction * speed + ejection.velocity;
                let position =
                    ejection.position + ejection.rotation * Vec3::from_array(kind.offset);
                thrown.push((kind.clone(), position, velocity));
            }
        }
        for (kind, position, velocity) in thrown {
            // (one out for each in, the oldest first)
            self.trim(physics, self.cap.saturating_sub(1));
            if let Some(piece) = throw(physics, &kind, position, velocity) {
                self.pieces.push(piece);
            }
        }
    }

    /// After the physics steps: each piece's pose, and the bodies of those
    /// still for long enough gone.
    pub fn settle(&mut self, physics: &mut PhysicsWorld) {
        for piece in &mut self.pieces {
            let Some(body) = piece.body else {
                continue;
            };
            if let Ok(pose) = physics.body_pose(body) {
                piece.pose = Pose {
                    position: pose.position,
                    rotation: pose.rotation * lying().inverse(),
                };
            }
            let slow = physics
                .body_velocity(body)
                .map(|(linear, _)| linear.length() < REST_SPEED)
                .unwrap_or(true);
            let awake = physics.body_awake(body).unwrap_or(false);
            piece.still = if slow { piece.still + 1 } else { 0 };
            if !awake || piece.still >= REST_TICKS {
                physics.remove_body(body);
                piece.body = None;
            }
        }
    }
}

/// A capsule shape stands along its body's z axis: a piece's body is
/// turned by this so that it lies along the piece's x.
fn lying() -> Quat {
    Quat::from_rotation_y(std::f32::consts::FRAC_PI_2)
}

fn lerp(a: f32, b: f32, t: f32) -> f32 {
    a + (b - a) * t
}

/// A way, from a yaw and a pitch.
fn direction([yaw, pitch]: [f32; 2]) -> Vec3 {
    Vec3::new(
        libm::cosf(pitch) * libm::cosf(yaw),
        libm::cosf(pitch) * libm::sinf(yaw),
        libm::sinf(pitch),
    )
}

/// A way within `angle` of `axis`, at random.
fn cone(axis: Vec3, angle: f32, seed: &mut RandomSeed) -> Vec3 {
    if angle <= 0.0 {
        return axis;
    }
    let tilt = angle * libm::sqrtf(seed.next_real());
    let around = std::f32::consts::TAU * seed.next_real();
    let across = axis.any_orthonormal_vector();
    let spun = Quat::from_axis_angle(axis, around) * across;
    Quat::from_axis_angle(spun, tilt) * axis
}

/// A piece's body: a capsule along the way it flies, tumbling end over end
/// (`physics_debris_particle_body`).
fn throw(
    physics: &mut PhysicsWorld,
    kind: &EffectDebris,
    position: Vec3,
    velocity: Vec3,
) -> Option<Piece> {
    if kind.radius.is_nan() || kind.radius <= 0.0 {
        return None;
    }
    let radius = (0.35 * kind.radius).clamp(0.003, 10.0);
    let half_length = (0.65 * kind.radius).clamp(0.003, 10.0);
    let metres = opence_physics::METRES_PER_UNIT;
    let (a, h) = (radius * metres, half_length * metres);
    let pi = std::f32::consts::PI;
    // (the density is in grams a millilitre)
    let mass = (kind.density * 1000.0 * (pi * a * a * 2.0 * h + 4.0 / 3.0 * pi * a * a * a))
        .clamp(0.01, 50.0);
    let speed = velocity.length();
    let forward = if speed > f32::EPSILON {
        velocity / speed
    } else {
        Vec3::X
    };
    let velocity = if speed > SPEED_MAX {
        velocity * (SPEED_MAX / speed)
    } else {
        velocity
    };
    // (left: across the way it flies and up, or y flying straight up or
    // down)
    let left = if forward.z.abs() > 0.95 {
        Vec3::new(forward.z, 0.0, -forward.x)
    } else {
        Vec3::new(-forward.y, forward.x, 0.0)
    }
    .normalize_or(Vec3::Y);
    let up = forward.cross(left);
    let rotation = Quat::from_mat3(&glam::Mat3::from_cols(forward, left, up)).normalize();
    let pose = Pose { position, rotation };
    let body = physics
        .add_body(
            Shape::Capsule {
                radius,
                height: 2.0 * half_length,
            },
            Pose {
                position,
                rotation: rotation * lying(),
            },
            Motion::Dynamic,
            mass,
            Material {
                friction: kind.friction.clamp(0.0, 1.0),
                restitution: kind.elasticity.clamp(0.0, 1.0),
            },
        )
        .ok()?;
    let spin = (speed / (4.0 * kind.radius)).clamp(0.0, 50.0);
    // (a body the physics will not set moving lies where it was made)
    let _ = physics.set_body_velocity(body, velocity, left * spin);
    Some(Piece {
        body: Some(body),
        pose,
        radius,
        half_length,
        still: 0,
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn a_way_within_a_cone_stays_within_it() {
        let mut seed = RandomSeed(1);
        let axis = direction([0.3, 0.2]);
        for _ in 0..100 {
            let way = cone(axis, 0.25, &mut seed);
            assert!((way.length() - 1.0).abs() < 1e-4);
            assert!(libm::acosf(way.dot(axis).min(1.0)) <= 0.2501);
        }
        assert_eq!(cone(axis, 0.0, &mut seed), axis);
    }

    #[test]
    fn a_direction_of_no_yaw_or_pitch_is_forward() {
        assert!(direction([0.0, 0.0]).distance(Vec3::X) < 1e-6);
        assert!(direction([0.0, std::f32::consts::FRAC_PI_2]).distance(Vec3::Z) < 1e-6);
    }
}
