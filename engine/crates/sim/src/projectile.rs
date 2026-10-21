//! Projectiles in flight: launched along a trigger's error cone, slowed
//! from their initial speed to their final one over their range, pulled
//! down by their share of gravity, and stopped by the first thing a tick's
//! path crosses (`projectiles.c`'s flight, without its guidance or water).
//!
//! A thrown grenade has a fuse: it arms, bounces off what it strikes by its
//! material response, sticks to what it may stick to, counts down once it
//! stops or sticks, and detonates.

use glam::{Quat, Vec3};
use opence_content::{MaterialResponse, Projectile, TagRef, Trigger};
use opence_physics::{Hit, Struck};

use crate::clock;
use crate::hash::WorldHasher;
use crate::objects::ObjectHandle;
use crate::random::RandomSeed;
use crate::Transform;

/// The longest a projectile flies, in ticks, when its range does not stop
/// it first (half a minute: past any level's edge).
pub const MAXIMUM_FLIGHT_TICKS: u32 = 30 * clock::TICKS_PER_SECOND;

/// The most surfaces a fused projectile strikes in a tick before it
/// detonates (the game's `MAXIMUM_COLLISIONS_PER_TICK`, ours smaller).
pub const MAXIMUM_COLLISIONS_PER_TICK: u32 = 8;

/// The squared speed, in world units per second, below which a projectile
/// that has struck something has stopped (the game's per-tick epsilon).
const STOPPED_SPEED_SQUARED: f32 = 0.09;

/// How steep a surface may be and still be ground: its normal's height.
const GROUND_NORMAL: f32 = 0.3;

/// How far off a surface a projectile that struck it is put back, so that
/// its next path does not start inside it.
const SKIN: f32 = 0.001;

/// What a projectile's path struck.
#[derive(Clone, Debug, PartialEq)]
pub struct Contact {
    /// Where, and the surface.
    pub hit: Hit,
    /// The object struck, if one was (a unit: projectiles strike only
    /// bipeds and the level).
    pub object: Option<ObjectHandle>,
    /// That object's transform, to stick to it.
    pub frame: Option<Transform>,
}

/// Where a stuck projectile is held.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Attachment {
    /// The object it is stuck to; none, stuck where it is.
    pub parent: Option<ObjectHandle>,
    /// Where on it, in its frame.
    pub offset: Vec3,
}

/// A thrown projectile's fuse (`projectile_update`'s timers and flags).
#[derive(Clone, Debug, PartialEq)]
pub struct Fuse {
    /// When its timer starts ([`Fuse::IMMEDIATELY`] and the rest).
    pub starts: i16,
    /// Its arming, 0 to 1.
    pub arming: f32,
    /// Its arming each tick (0: armed from the start).
    pub arming_delta: f32,
    /// Its timer, 0 to 1.
    pub timer: f32,
    /// Its timer each tick, once it counts down.
    pub timer_delta: f32,
    /// The timer counts down.
    pub counting_down: bool,
    /// It has struck the ground.
    pub collided_once: bool,
    /// It has slowed to a stop against something.
    pub stopped: bool,
    /// It lies at rest on the ground, or is stuck.
    pub at_rest: bool,
    /// It will detonate once armed.
    pub detonating: bool,
    /// Where it is stuck, if it is.
    pub attached: Option<Attachment>,
    /// Its timer's longest.
    timer_upper: f32,
    /// Stuck, it waits its longest.
    longest_if_attached: bool,
    /// Its least speed.
    minimum_velocity: f32,
    /// Its response to what it strikes.
    response: MaterialResponse,
    /// The draws for its potential responses.
    seed: RandomSeed,
    /// The effect where it detonates.
    effect: Option<TagRef>,
    /// The damage it does to what it is stuck to.
    attached_damage: Option<TagRef>,
}

impl Fuse {
    /// The timer starts at once.
    pub const IMMEDIATELY: i16 = 0;
    /// The timer starts after the first bounce.
    pub const AFTER_FIRST_BOUNCE: i16 = 1;
    /// The timer starts once it is at rest.
    pub const WHEN_AT_REST: i16 = 2;

    /// Armed: it arms at once, or its arming has run.
    pub fn armed(&self) -> bool {
        self.arming_delta == 0.0 || self.arming >= 1.0
    }

    fn hash(&self, hasher: &mut WorldHasher) {
        for value in [self.arming, self.timer] {
            hasher.f32(value);
        }
        hasher.u64(
            u64::from(self.counting_down)
                | u64::from(self.collided_once) << 1
                | u64::from(self.stopped) << 2
                | u64::from(self.at_rest) << 3
                | u64::from(self.detonating) << 4,
        );
        hasher.u64(u64::from(self.seed.0));
        if let Some(attached) = self.attached {
            hasher.u64(
                attached
                    .parent
                    .map_or(u64::MAX, |parent| u64::from(parent.index)),
            );
            for value in attached.offset.to_array() {
                hasher.f32(value);
            }
        }
    }
}

/// The ticks' delta that runs a timer of `seconds` (none for no time).
fn delta(seconds: f32) -> f32 {
    let ticks = seconds * clock::TICKS_PER_SECOND as f32;
    if ticks >= 1.0 {
        1.0 / ticks
    } else {
        0.0
    }
}

/// Where a fused projectile detonated.
#[derive(Clone, Debug, PartialEq)]
pub struct Detonation {
    /// The projectile's tag.
    pub projectile: TagRef,
    /// The object that threw it.
    pub owner: ObjectHandle,
    /// Where.
    pub point: Vec3,
    /// Its effect, whose damage is done around the point.
    pub effect: Option<TagRef>,
    /// The object it was stuck to.
    pub attached: Option<ObjectHandle>,
    /// The damage it does to that object.
    pub attached_damage: Option<TagRef>,
}

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
    /// Its fuse, if it was thrown.
    pub fuse: Option<Fuse>,
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
    /// It detonated, and is gone.
    Detonated(Detonation),
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
            fuse: None,
        }
    }

    /// A grenade of `definition` thrown from `origin` at `velocity`, in
    /// world units per second; `seed` draws its timer.
    #[allow(clippy::too_many_arguments)]
    pub fn thrown(
        handle: ObjectHandle,
        tag: TagRef,
        owner: ObjectHandle,
        definition: &Projectile,
        origin: Vec3,
        velocity: Vec3,
        gravity: f32,
        seed: &mut RandomSeed,
    ) -> Flight {
        let [lower, upper] = definition.timer;
        let longest_if_attached =
            definition.flags & Projectile::DETONATION_MAX_TIME_IF_ATTACHED != 0;
        let seconds = if longest_if_attached {
            lower
        } else {
            lower + (upper - lower) * seed.next_real()
        };
        let draws = RandomSeed(u32::from(seed.next_u16()) << 16 | u32::from(seed.next_u16()));
        Flight {
            velocity,
            initial_speed: 0.0,
            final_speed: 0.0,
            range: 0.0,
            damage: None,
            fuse: Some(Fuse {
                starts: definition.detonation_timer_starts,
                arming: 0.0,
                arming_delta: delta(definition.arming_time),
                timer: 0.0,
                timer_delta: delta(seconds),
                counting_down: false,
                collided_once: false,
                stopped: false,
                at_rest: false,
                detonating: false,
                attached: None,
                timer_upper: upper,
                longest_if_attached,
                minimum_velocity: definition.minimum_velocity,
                response: definition
                    .material_responses
                    .first()
                    .cloned()
                    .unwrap_or_default(),
                seed: draws,
                effect: definition.effect.clone(),
                attached_damage: definition.attached_detonation_damage.clone(),
            }),
            ..Flight::new(handle, tag, owner, definition, origin, Vec3::ZERO, gravity)
        }
    }

    /// Where it is stuck, if it is.
    pub fn attachment(&self) -> Option<Attachment> {
        self.fuse.as_ref().and_then(|fuse| fuse.attached)
    }

    /// Follows what it is stuck to, at `frame`; none, and it is gone, so it
    /// stays where it is.
    pub fn follow(&mut self, frame: Option<Transform>) {
        let Some(fuse) = self.fuse.as_mut() else {
            return;
        };
        let Some(attached) = fuse.attached.as_mut() else {
            return;
        };
        if attached.parent.is_none() {
            return;
        }
        match frame {
            Some(frame) => self.position = frame.position + frame.rotation * attached.offset,
            None => {
                attached.parent = None;
                attached.offset = self.position;
            }
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
    pub fn step(&mut self, mut cast: impl FnMut(Vec3, Vec3) -> Option<Contact>) -> Flew {
        if self.fuse.is_some() {
            return self.step_fused(cast);
        }
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
            if let Some(Contact { hit, object, .. }) = cast(self.position, path) {
                return Flew::Hit(Impact {
                    projectile: self.tag.clone(),
                    owner: self.owner,
                    damage: self.damage.clone(),
                    point: hit.point,
                    normal: hit.normal,
                    direction: path.normalize_or_zero(),
                    struck: hit.struck,
                    object,
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

    /// A tick of a fused projectile: its arming and timer run, and it
    /// moves, bouncing, until it stops, sticks or detonates.
    fn step_fused(&mut self, mut cast: impl FnMut(Vec3, Vec3) -> Option<Contact>) -> Flew {
        let dt = clock::TICK.0;
        self.age += 1;
        let Some(mut fuse) = self.fuse.take() else {
            return Flew::Spent;
        };
        // (BUG (original, preserved): its timer's start "after the first
        // bounce" falls through to "when at rest", so both wait for it to
        // stop)
        if fuse.starts == Fuse::IMMEDIATELY || fuse.stopped || fuse.attached.is_some() {
            fuse.counting_down = true;
        }
        if fuse.counting_down {
            fuse.timer = (fuse.timer + fuse.timer_delta).min(1.0);
            if fuse.timer >= 1.0 {
                fuse.detonating = true;
            }
        }
        fuse.arming = (fuse.arming + fuse.arming_delta).min(1.0);
        let mut remaining = 1.0;
        let mut collisions = 0;
        while remaining > 0.0
            && fuse.attached.is_none()
            && !fuse.at_rest
            && !(fuse.detonating && fuse.armed())
        {
            let fall = self.gravity * dt * remaining;
            let start = self.velocity;
            self.velocity.z -= fall;
            let path = (start + self.velocity) * 0.5 * dt * remaining;
            if path.length_squared() == 0.0 {
                break;
            }
            let Some(contact) = cast(self.position, path) else {
                self.position += path;
                self.travelled += path.length();
                break;
            };
            let fraction = contact.hit.fraction.clamp(0.0, 1.0);
            let normal = contact.hit.normal;
            self.travelled += path.length() * fraction;
            self.position = contact.hit.point + normal * SKIN;
            // (the gravity of the time not flown is given back)
            self.velocity.z += fall * (1.0 - fraction);
            remaining *= 1.0 - fraction;
            if normal.z > GROUND_NORMAL {
                fuse.collided_once = true;
            }
            match self.collide(&mut fuse, &contact) {
                Collided::On => {}
                Collided::Gone => return Flew::Spent,
            }
            collisions += 1;
            if collisions >= MAXIMUM_COLLISIONS_PER_TICK {
                fuse.detonating = true;
                break;
            }
        }
        if fuse.detonating && fuse.armed() {
            return Flew::Detonated(Detonation {
                projectile: self.tag.clone(),
                owner: self.owner,
                point: self.position,
                effect: fuse.effect.clone(),
                attached: fuse.attached.and_then(|attached| attached.parent),
                attached_damage: fuse.attached_damage.clone(),
            });
        }
        let spent = self.age >= MAXIMUM_FLIGHT_TICKS;
        self.fuse = Some(fuse);
        if spent {
            Flew::Spent
        } else {
            Flew::On
        }
    }

    /// What it does on striking `contact` (`projectile_collision`): its
    /// material's potential response if its bounds hold, else its default.
    fn collide(&mut self, fuse: &mut Fuse, contact: &Contact) -> Collided {
        let response = fuse.response.clone();
        let normal = contact.hit.normal;
        let speed = self.velocity.length();
        let along = self.velocity.dot(normal);
        let angle = if speed > 0.0 {
            libm::acosf((along / speed).clamp(-1.0, 1.0)) - std::f32::consts::FRAC_PI_2
        } else {
            0.0
        };
        let within =
            |[lower, upper]: [f32; 2], value: f32| upper == 0.0 || (lower..=upper).contains(&value);
        let potential = response.potential_response != 0
            && within(response.potential_angle, angle)
            && within(response.potential_velocity, -along)
            && (response.potential_flags & MaterialResponse::ONLY_AGAINST_UNITS == 0
                || contact.object.is_some())
            && fuse.seed.next_real() >= response.potential_skip_fraction;
        let mut kind = if potential {
            response.potential_response
        } else {
            response.default_response
        };
        if kind == MaterialResponse::OVERPENETRATE {
            kind = if fuse.timer_upper != 0.0 {
                fuse.collided_once = true;
                fuse.stopped = true;
                MaterialResponse::ATTACH
            } else {
                MaterialResponse::DETONATE
            };
        }
        if kind == MaterialResponse::REFLECT {
            let across = normal * along;
            let surface = self.velocity - across;
            self.velocity = surface * (1.0 - response.perpendicular_friction)
                - across * (1.0 - response.parallel_friction);
        } else {
            self.velocity = Vec3::ZERO;
        }
        let minimum = fuse.minimum_velocity;
        if kind != MaterialResponse::ATTACH && self.velocity.length_squared() < minimum * minimum {
            fuse.detonating = true;
        }
        if self.velocity.length_squared() < STOPPED_SPEED_SQUARED {
            fuse.stopped = true;
            if normal.z > GROUND_NORMAL {
                fuse.at_rest = true;
                if fuse.timer_upper == 0.0 {
                    fuse.detonating = true;
                }
            }
        }
        match kind {
            MaterialResponse::DISAPPEAR => return Collided::Gone,
            MaterialResponse::DETONATE => fuse.detonating = true,
            MaterialResponse::ATTACH => {
                self.velocity = Vec3::ZERO;
                self.position = contact.hit.point;
                fuse.at_rest = true;
                fuse.attached = Some(match (contact.object, contact.frame) {
                    (Some(parent), Some(frame)) => Attachment {
                        parent: Some(parent),
                        offset: frame.rotation.inverse() * (contact.hit.point - frame.position),
                    },
                    _ => Attachment {
                        parent: None,
                        offset: contact.hit.point,
                    },
                });
                if fuse.longest_if_attached {
                    fuse.timer_delta = delta(fuse.timer_upper);
                }
            }
            _ => {}
        }
        Collided::On
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
        if let Some(fuse) = &self.fuse {
            fuse.hash(hasher);
        }
    }
}

/// What striking something came to.
enum Collided {
    /// It goes on (bounced, stopped or stuck).
    On,
    /// It disappeared.
    Gone,
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
            (to.x >= 12.0).then(|| Contact {
                hit: Hit {
                    fraction: (12.0 - from.x) / path.x,
                    point: Vec3::new(12.0, from.y, from.z),
                    normal: -Vec3::X,
                    struck: Struck::Level,
                },
                object: None,
                frame: None,
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

    fn grenade(flags: u32, response: MaterialResponse) -> Projectile {
        Projectile {
            flags,
            detonation_timer_starts: Fuse::WHEN_AT_REST,
            arming_time: 0.5,
            timer: [1.0, 1.0],
            air_gravity_scale: 1.0,
            effect: Some(tag()),
            material_responses: vec![response],
            ..Projectile::default()
        }
    }

    fn bouncing() -> MaterialResponse {
        MaterialResponse {
            default_response: MaterialResponse::REFLECT,
            parallel_friction: 0.5,
            perpendicular_friction: 0.5,
            ..MaterialResponse::default()
        }
    }

    /// The ground at z = 0.
    fn ground(from: Vec3, path: Vec3) -> Option<Contact> {
        let to = from + path;
        (to.z < 0.0 && path.z < 0.0).then(|| {
            let fraction = (from.z / -path.z).clamp(0.0, 1.0);
            Contact {
                hit: Hit {
                    fraction,
                    point: from + path * fraction,
                    normal: Vec3::Z,
                    struck: Struck::Level,
                },
                object: None,
                frame: None,
            }
        })
    }

    fn throw(definition: &Projectile, velocity: Vec3) -> Flight {
        Flight::thrown(
            handle(1),
            tag(),
            handle(0),
            definition,
            Vec3::new(0.0, 0.0, 1.0),
            velocity,
            9.8,
            &mut RandomSeed::default(),
        )
    }

    #[test]
    fn a_grenade_bounces_comes_to_rest_and_detonates_on_its_timer() {
        let mut flight = throw(&grenade(0, bouncing()), Vec3::new(6.0, 0.0, 2.0));
        let mut bounced = false;
        let mut rest = None;
        let mut detonation = None;
        for tick in 1..600 {
            let before = flight.velocity.z;
            match flight.step(ground) {
                Flew::On => {}
                Flew::Detonated(detonated) => {
                    detonation = Some((tick, detonated));
                    break;
                }
                other => panic!("{other:?}"),
            }
            assert!(flight.position.z >= 0.0, "{}", flight.position);
            bounced |= before < 0.0 && flight.velocity.z > 0.0;
            if rest.is_none() && flight.fuse.as_ref().is_some_and(|fuse| fuse.at_rest) {
                rest = Some(tick);
            }
        }
        assert!(bounced, "it never bounced");
        let rest = rest.unwrap_or_else(|| panic!("it never came to rest"));
        let (tick, detonation) = detonation.unwrap_or_else(|| panic!("it never detonated"));
        // (its timer of a second runs from when it stopped)
        let second = clock::TICKS_PER_SECOND;
        assert!(
            (second..=second + 2).contains(&(tick - rest)),
            "{rest} {tick}"
        );
        assert_eq!(detonation.effect, Some(tag()));
        assert_eq!(detonation.owner, handle(0));
        assert_eq!(detonation.attached, None);
        assert!(detonation.point.z.abs() < 0.01 && detonation.point.x > 1.0);
    }

    #[test]
    fn a_sticky_grenade_sticks_to_a_unit_and_follows_it() {
        let sticky = MaterialResponse {
            potential_response: MaterialResponse::ATTACH,
            potential_flags: MaterialResponse::ONLY_AGAINST_UNITS,
            ..bouncing()
        };
        let definition = Projectile {
            arming_time: 0.0,
            attached_detonation_damage: Some(tag()),
            ..grenade(Projectile::DETONATION_MAX_TIME_IF_ATTACHED, sticky)
        };
        let frame = Transform {
            position: Vec3::new(3.0, 0.0, 0.0),
            rotation: Quat::IDENTITY,
            scale: 1.0,
        };
        // (a unit's wall at x = 3)
        let unit = |from: Vec3, path: Vec3| {
            let to = from + path;
            (to.x >= 3.0 && from.x < 3.0).then(|| {
                let fraction = (3.0 - from.x) / path.x;
                Contact {
                    hit: Hit {
                        fraction,
                        point: from + path * fraction,
                        normal: -Vec3::X,
                        struck: Struck::Other,
                    },
                    object: Some(handle(7)),
                    frame: Some(frame),
                }
            })
        };
        let mut flight = throw(&definition, Vec3::new(10.0, 0.0, 0.0));
        let mut ticks = 0;
        while flight.attachment().is_none() {
            assert_eq!(flight.step(unit), Flew::On);
            ticks += 1;
            assert!(ticks < 60, "it never stuck");
        }
        let attachment = flight.attachment().unwrap_or_else(|| unreachable!());
        assert_eq!(attachment.parent, Some(handle(7)));
        assert!((attachment.offset.x).abs() < 1e-4);
        // (it follows the unit as it walks)
        let moved = Transform {
            position: Vec3::new(5.0, 1.0, 0.0),
            ..frame
        };
        flight.follow(Some(moved));
        assert!((flight.position.x - 5.0).abs() < 1e-4 && (flight.position.y - 1.0).abs() < 0.1);
        let mut detonated = None;
        for tick in 1..200 {
            flight.follow(Some(moved));
            if let Flew::Detonated(detonation) = flight.step(unit) {
                detonated = Some((tick, detonation));
                break;
            }
        }
        let (tick, detonation) = detonated.unwrap_or_else(|| panic!("it never detonated"));
        let second = clock::TICKS_PER_SECOND;
        assert!((second - 1..=second + 1).contains(&tick), "{tick}");
        assert_eq!(detonation.attached, Some(handle(7)));
        assert_eq!(detonation.attached_damage, Some(tag()));
    }

    #[test]
    fn an_unarmed_grenade_waits_to_arm() {
        let definition = Projectile {
            detonation_timer_starts: Fuse::IMMEDIATELY,
            arming_time: 2.0,
            timer: [0.5, 0.5],
            ..grenade(0, bouncing())
        };
        let mut flight = throw(&definition, Vec3::ZERO);
        let tick = (1..600).find(|_| matches!(flight.step(ground), Flew::Detonated(_)));
        // (its timer ran out at half a second; it detonates once armed)
        let armed = 2 * clock::TICKS_PER_SECOND;
        assert!(
            tick.is_some_and(|tick| (armed - 1..=armed + 1).contains(&tick)),
            "{tick:?}"
        );
    }
}
