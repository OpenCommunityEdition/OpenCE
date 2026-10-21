//! Projectiles in flight: launched along a trigger's error cone, slowed
//! from their initial speed to their final one over their range, pulled
//! down by their share of gravity, and stopped by the first thing a tick's
//! path crosses (`projectiles.c`'s flight, without its guidance, water or
//! material responses).

use glam::{Quat, Vec3};
use opence_content::{Projectile, TagRef, Trigger};
use opence_physics::{Hit, Struck};

use crate::clock;
use crate::hash::WorldHasher;
use crate::objects::ObjectHandle;
use crate::random::RandomSeed;
use crate::Transform;

/// The longest a projectile flies, in ticks, when its range does not stop
/// it first (half a minute: past any level's edge).
pub const MAXIMUM_FLIGHT_TICKS: u32 = 30 * clock::TICKS_PER_SECOND;

/// A projectile in flight.
#[derive(Clone, Debug, PartialEq)]
pub struct Flight {
    /// Its object.
    pub handle: ObjectHandle,
    /// Its tag.
    pub tag: TagRef,
    /// The object that fired it.
    pub owner: ObjectHandle,
    /// Where it is.
    pub position: Vec3,
    /// Its velocity, in world units per second.
    pub velocity: Vec3,
    /// How far it has flown, in world units.
    pub travelled: f32,
    /// The ticks it has flown.
    pub age: u32,
    /// Its speed at the muzzle, in world units per second.
    pub initial_speed: f32,
    /// Its speed at the end of its range.
    pub final_speed: f32,
    /// How far it flies (0: until it hits something).
    pub range: f32,
    /// Its fall, in world units per second per second.
    pub gravity: f32,
    /// The damage it does where it hits.
    pub damage: Option<TagRef>,
}

/// Where a projectile hit.
#[derive(Clone, Debug, PartialEq)]
pub struct Impact {
    /// The projectile's tag.
    pub projectile: TagRef,
    /// The object that fired it.
    pub owner: ObjectHandle,
    /// The damage it does.
    pub damage: Option<TagRef>,
    /// Where it hit.
    pub point: Vec3,
    /// The surface's normal there.
    pub normal: Vec3,
    /// The way it was going.
    pub direction: Vec3,
    /// What it hit.
    pub struck: Struck,
    /// The object it hit, if it hit one.
    pub object: Option<ObjectHandle>,
}

/// What a tick of flight came to.
#[derive(Clone, Debug, PartialEq)]
pub enum Flew {
    /// It flies on.
    On,
    /// It hit something, and is gone.
    Hit(Impact),
    /// It flew its range, or its time, and is gone.
    Spent,
}

impl Flight {
    /// A projectile of `definition` leaving `origin` along `direction` (a
    /// unit vector); `gravity` is the world's, in world units per second
    /// per second.
    pub fn new(
        handle: ObjectHandle,
        tag: TagRef,
        owner: ObjectHandle,
        definition: &Projectile,
        origin: Vec3,
        direction: Vec3,
        gravity: f32,
    ) -> Flight {
        Flight {
            handle,
            tag,
            owner,
            position: origin,
            velocity: direction * definition.initial_velocity,
            travelled: 0.0,
            age: 0,
            initial_speed: definition.initial_velocity,
            final_speed: definition.final_velocity,
            range: definition.maximum_range.max(0.0),
            gravity: definition.air_gravity_scale * gravity,
            damage: definition.impact_damage.clone(),
        }
    }

    /// Its speed now: from its initial to its final over its range.
    fn speed(&self) -> f32 {
        if self.range > 0.0 {
            let along = (self.travelled / self.range).min(1.0);
            self.initial_speed + (self.final_speed - self.initial_speed) * along
        } else {
            self.initial_speed
        }
    }

    /// One tick of flight; `cast` finds the first thing along a path (from
    /// a point, the path's whole length).
    pub fn step(&mut self, mut cast: impl FnMut(Vec3, Vec3) -> Option<Hit>) -> Flew {
        let dt = clock::TICK.0;
        // (its speed is its range's; gravity bends its path)
        let speed = self.speed();
        if speed > 0.0 {
            self.velocity = self.velocity.normalize_or_zero() * speed;
        }
        self.velocity.z -= self.gravity * dt;
        let mut path = self.velocity * dt;
        let mut spent = false;
        if self.range > 0.0 {
            let left = self.range - self.travelled;
            let length = path.length();
            if length >= left {
                path *= (left / length).max(0.0);
                spent = true;
            }
        }
        self.age += 1;
        if path.length_squared() > 0.0 {
            if let Some(hit) = cast(self.position, path) {
                return Flew::Hit(Impact {
                    projectile: self.tag.clone(),
                    owner: self.owner,
                    damage: self.damage.clone(),
                    point: hit.point,
                    normal: hit.normal,
                    direction: path.normalize_or_zero(),
                    struck: hit.struck,
                    object: None,
                });
            }
        }
        self.position += path;
        self.travelled += path.length();
        if spent || self.age >= MAXIMUM_FLIGHT_TICKS {
            Flew::Spent
        } else {
            Flew::On
        }
    }

    /// Its object's transform: at it, turned along its path.
    pub fn transform(&self) -> Transform {
        let direction = self.velocity.normalize_or_zero();
        let rotation = if direction == Vec3::ZERO {
            Quat::IDENTITY
        } else {
            Quat::from_rotation_arc(Vec3::X, direction)
        };
        Transform {
            position: self.position,
            rotation,
            scale: 1.0,
        }
    }

    pub(crate) fn hash(&self, hasher: &mut WorldHasher) {
        hasher.u64(u64::from(self.handle.index));
        for value in self
            .position
            .to_array()
            .into_iter()
            .chain(self.velocity.to_array())
            .chain([self.travelled])
        {
            hasher.f32(value);
        }
        hasher.u64(u64::from(self.age));
    }
}

/// The ways a shot's projectiles leave a weapon looking along `aim` (its
/// X forward): each in a random cone of the trigger's error at `error`,
/// fanned across its distribution angle when it fans them.
pub fn shot_directions(
    trigger: &Trigger,
    error: f32,
    aim: Quat,
    seed: &mut RandomSeed,
) -> Vec<Vec3> {
    /// The trigger's distribution function that fans its projectiles.
    const HORIZONTAL_FAN: i16 = 1;
    let count = trigger.projectiles_per_shot.max(1);
    let [least, most] = trigger.projectile_error_angle;
    let cone = least + (most - least) * error.clamp(0.0, 1.0);
    let identical = trigger.flags & Trigger::PROJECTILES_HAVE_IDENTICAL_ERROR != 0;
    let mut shared = None;
    (0..count)
        .map(|index| {
            let fan = if trigger.projectile_distribution_function == HORIZONTAL_FAN && count > 1 {
                let along = f32::from(index) / f32::from(count - 1) - 0.5;
                trigger.projectile_distribution_angle * along
            } else {
                0.0
            };
            let (theta, phi) = match shared {
                Some(angles) if identical => angles,
                _ => {
                    let angles = (
                        cone * seed.next_real(),
                        std::f32::consts::TAU * seed.next_real(),
                    );
                    shared = Some(angles);
                    angles
                }
            };
            let (sin, cos) = (libm::sinf(theta), libm::cosf(theta));
            let local = Vec3::new(cos, sin * libm::cosf(phi), sin * libm::sinf(phi));
            (aim * Quat::from_rotation_z(fan) * local).normalize_or_zero()
        })
        .collect()
}

#[cfg(test)]
mod tests {
    use super::*;
    use opence_content::Group;

    fn tag() -> TagRef {
        TagRef {
            group: Group(*b"proj"),
            path: "weapons\\assault rifle\\bullet".into(),
        }
    }

    fn handle(index: u32) -> ObjectHandle {
        ObjectHandle { index, salt: 1 }
    }

    fn bullet() -> Projectile {
        Projectile {
            initial_velocity: 300.0,
            final_velocity: 150.0,
            maximum_range: 40.0,
            air_gravity_scale: 1.0,
            impact_damage: Some(tag()),
            ..Projectile::default()
        }
    }

    fn flight(definition: &Projectile) -> Flight {
        Flight::new(
            handle(1),
            tag(),
            handle(0),
            definition,
            Vec3::ZERO,
            Vec3::X,
            3.0,
        )
    }

    #[test]
    fn it_flies_its_range_and_slows() {
        let mut flight = flight(&bullet());
        let first = flight.step(|_, _| None);
        assert_eq!(first, Flew::On);
        assert!(
            (flight.position.x - 5.0).abs() < 0.01,
            "{}",
            flight.position
        );
        let mut ticks = 1;
        while flight.step(|_, _| None) == Flew::On {
            ticks += 1;
            assert!(ticks < 100, "it flew on");
        }
        // (40 units from 300 a second down to 150: about ten ticks)
        assert!((9..=12).contains(&ticks), "{ticks} ticks");
        assert!((flight.travelled - 40.0).abs() < 1e-3);
        assert!(flight.velocity.x < 160.0 && flight.velocity.x > 140.0);
        // (and fell, a little)
        assert!(flight.position.z < 0.0);
    }

    #[test]
    fn it_stops_where_it_hits() {
        let mut flight = flight(&bullet());
        let wall = |from: Vec3, path: Vec3| {
            let to = from + path;
            (to.x >= 12.0).then(|| Hit {
                fraction: (12.0 - from.x) / path.x,
                point: Vec3::new(12.0, from.y, from.z),
                normal: -Vec3::X,
                struck: Struck::Level,
            })
        };
        let mut flew = Flew::On;
        for _ in 0..10 {
            flew = flight.step(wall);
            if flew != Flew::On {
                break;
            }
        }
        let Flew::Hit(impact) = flew else {
            panic!("{flew:?}");
        };
        assert_eq!(impact.point.x, 12.0);
        assert_eq!(impact.normal, -Vec3::X);
        assert_eq!(impact.damage, Some(tag()));
        assert_eq!(impact.owner, handle(0));
        assert!((impact.direction - Vec3::X).length() < 0.01);
    }

    #[test]
    fn without_a_range_it_flies_until_its_time_is_up() {
        let definition = Projectile {
            maximum_range: 0.0,
            air_gravity_scale: 0.0,
            ..bullet()
        };
        let mut flight = flight(&definition);
        let ticks = (1..).find(|_| flight.step(|_, _| None) != Flew::On);
        assert_eq!(ticks, Some(MAXIMUM_FLIGHT_TICKS));
        // (no slowing without a range)
        assert!((flight.velocity.length() - 300.0).abs() < 1e-3);
    }

    #[test]
    fn shots_spread_within_their_cone() {
        let trigger = Trigger {
            projectile_error_angle: [0.0, 0.2],
            projectiles_per_shot: 15,
            ..Trigger::default()
        };
        let mut seed = RandomSeed::default();
        let aim = Quat::from_rotation_z(1.0);
        let forward = aim * Vec3::X;
        let at_rest = shot_directions(&trigger, 0.0, aim, &mut seed);
        assert_eq!(at_rest.len(), 15);
        assert!(at_rest.iter().all(|way| (*way - forward).length() < 1e-5));
        let spread = shot_directions(&trigger, 1.0, aim, &mut seed);
        let angles: Vec<f32> = spread
            .iter()
            .map(|way| libm::acosf(way.dot(forward).min(1.0)))
            .collect();
        assert!(
            angles.iter().all(|&angle| angle <= 0.2 + 1e-4),
            "{angles:?}"
        );
        assert!(angles.iter().any(|&angle| angle > 0.05), "{angles:?}");
        // (the same seed, the same spread)
        let mut again = RandomSeed::default();
        shot_directions(&trigger, 0.0, aim, &mut again);
        assert_eq!(shot_directions(&trigger, 1.0, aim, &mut again), spread);
    }

    #[test]
    fn a_fan_spreads_across_its_angle() {
        let trigger = Trigger {
            projectiles_per_shot: 3,
            projectile_distribution_function: 1,
            projectile_distribution_angle: 0.5,
            ..Trigger::default()
        };
        let ways = shot_directions(&trigger, 0.0, Quat::IDENTITY, &mut RandomSeed::default());
        let yaws: Vec<f32> = ways.iter().map(|way| libm::atan2f(way.y, way.x)).collect();
        for (yaw, expected) in yaws.iter().zip([-0.25, 0.0, 0.25]) {
            assert!((yaw - expected).abs() < 1e-5, "{yaws:?}");
        }
    }
}
