//! How an item moves lying about (`items.c`'s `item_update` and
//! `item_accelerate`): a point that falls, bounces off what it meets with
//! 1.4 times the speed into it, and comes to rest on ground flatter than
//! 45° once it meets it slower than 0.05 world units a tick; spinning as it
//! flies, and turning about the ground's normal as it slows lying still.
//! Speeds are in world units a game tick.
//!
//! The item's "ground point" marker is taken to be at its origin, turned as
//! it is (the marker's offset is not imported), and the random direction a
//! push straight up or down spins it about is not the game's geosphere
//! table's, though it draws the same numbers from the seed.

use bevy_ecs::prelude::{Component, World as Ecs};
use glam::{Mat3, Quat, Vec3};
use opence_content::Item;
use opence_physics::{BodyId, Layers, Struck, World};

use crate::random::RandomSeed;
use crate::vehicle::GRAVITY;
use crate::ObjectId;
use crate::Transform;

/// How far off a surface an item is put back, in world units.
const SURFACE_OFFSET: f32 = 0.05;
/// The speed into a surface at which an impact is at its loudest
/// (`item_maximum_impact_velocity`).
const MAXIMUM_IMPACT_VELOCITY: f32 = 0.1;
/// Slower than this into a surface, an item rests on it.
const REST_SPEED: f32 = 0.05;
/// The least normal's z an item rests on (45°: the game's figure, a
/// little under the square root of a half).
#[allow(clippy::approx_constant)]
const REST_SLOPE: f32 = 0.7071;
/// How much of its speed into a surface an item bounces back with, and
/// more.
const BOUNCE: f32 = 1.4;
/// The most a body bounces an item back by.
const BODY_BOUNCE_MAX: f32 = 1.5;
/// What is left of a resting item's spin each tick.
const REST_SPIN_DAMPING: f32 = 0.9;
/// Below this squared size a push only frees an item.
const PUSH_EPSILON: f32 = 0.0001;
const HALF_TURN: f32 = 1.5707964;

/// What an item lies on.
#[derive(Clone, Copy, Debug, PartialEq)]
pub enum Rest {
    /// The level, its surface the plane through `point` with `normal`.
    Structure {
        /// A point on the plane.
        point: Vec3,
        /// The plane's normal.
        normal: Vec3,
    },
    /// A body, at `offset` in the body's frame.
    Body {
        /// The body.
        body: BodyId,
        /// Where on it, in its frame.
        offset: Vec3,
    },
}

/// An item's motion.
#[derive(Clone, Debug, PartialEq)]
pub struct ItemMotion {
    /// Where it is.
    pub position: Vec3,
    /// How fast it moves.
    pub velocity: Vec3,
    /// The way it faces.
    pub forward: Vec3,
    /// Its up.
    pub up: Vec3,
    /// Its spin: its axis, its length the radians a tick.
    pub angular_velocity: Vec3,
    /// Whether it lies still.
    pub at_rest: bool,
    /// What it lies on, if anything.
    pub rest: Option<Rest>,
    /// Whether pushes leave it be (`_item_does_not_accelerate_bit`).
    pub does_not_accelerate: bool,
    /// Ticks until it detonates (0: it is not set to).
    pub detonation_ticks: u32,
    rotation_axis: Vec3,
    rotation_sine: f32,
    rotation_cosine: f32,
    spinning: bool,
}

/// Where an item met something.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Impact {
    /// Where.
    pub point: Vec3,
    /// The surface's normal.
    pub normal: Vec3,
    /// What it met.
    pub struck: Struck,
    /// How hard, 0 to 1 (the material effect's and the sound's scale).
    pub scale: f32,
    /// Where its sound plays: off the surface past where the item would
    /// have gone.
    pub sound_position: Vec3,
}

/// What a tick did to an item.
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct ItemStep {
    /// What it met.
    pub impact: Option<Impact>,
    /// Whether it came to rest (where the game marks an unowned item as
    /// garbage).
    pub came_to_rest: bool,
    /// Whether it detonated (its effect plays and it goes).
    pub detonated: bool,
}

/// A hit on what items meet.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct ItemHit {
    /// Where.
    pub point: Vec3,
    /// The surface's normal, toward where the line began.
    pub normal: Vec3,
    /// What.
    pub struck: Struck,
}

/// What items meet.
pub trait ItemWorld {
    /// The first surface on the line from `from` to `to`.
    fn line(&self, from: Vec3, to: Vec3) -> Option<ItemHit>;
    /// Whether `point` is inside something.
    fn inside(&self, point: Vec3) -> bool;
    /// Where `offset`, in `body`'s frame, is in the world (none: the body
    /// is gone).
    fn body_point(&self, body: BodyId, offset: Vec3) -> Option<Vec3>;
    /// Where `point` is in `body`'s frame.
    fn body_offset(&self, body: BodyId, point: Vec3) -> Option<Vec3>;
}

impl ItemWorld for World {
    fn line(&self, from: Vec3, to: Vec3) -> Option<ItemHit> {
        let hit = self.cast_ray(from, to - from, Layers::ALL).ok()??;
        Some(ItemHit {
            point: hit.point,
            normal: hit.normal,
            struck: hit.struck,
        })
    }

    // (a mesh has no inside, and a point on the surface it is cast to is
    // not taken to be in it)
    fn inside(&self, _point: Vec3) -> bool {
        false
    }

    fn body_point(&self, body: BodyId, offset: Vec3) -> Option<Vec3> {
        let pose = self.body_pose(body).ok()?;
        Some(pose.position + pose.rotation * offset)
    }

    fn body_offset(&self, body: BodyId, point: Vec3) -> Option<Vec3> {
        let pose = self.body_pose(body).ok()?;
        Some(pose.rotation.inverse() * (point - pose.position))
    }
}

impl ItemMotion {
    /// An item placed at `position`, facing `forward` with `up`, still.
    pub fn placed(position: Vec3, forward: Vec3, up: Vec3) -> ItemMotion {
        ItemMotion {
            position,
            velocity: Vec3::ZERO,
            forward,
            up,
            angular_velocity: Vec3::ZERO,
            at_rest: false,
            rest: None,
            does_not_accelerate: false,
            detonation_ticks: 0,
            rotation_axis: Vec3::Z,
            rotation_sine: 0.0,
            rotation_cosine: 1.0,
            spinning: false,
        }
    }

    /// Sets it spinning by `angular_velocity`, in radians a tick about the
    /// vector's direction.
    pub fn spin(&mut self, angular_velocity: Vec3) {
        self.angular_velocity = angular_velocity;
        self.spin_changed();
    }

    /// The axis it turns about each tick, and the sine and cosine of the
    /// turn.
    pub fn rotation(&self) -> (Vec3, f32, f32) {
        (self.rotation_axis, self.rotation_sine, self.rotation_cosine)
    }

    /// Pushes it by `acceleration` (`item_accelerate`), freeing it from
    /// what it lies on, and spins it about the push's side.
    pub fn accelerate(&mut self, acceleration: Vec3, seed: &mut RandomSeed) {
        if self.does_not_accelerate {
            return;
        }
        let pushed = acceleration.dot(acceleration) >= PUSH_EPSILON;
        let on_structure = matches!(self.rest, Some(Rest::Structure { .. }));
        if on_structure {
            if pushed {
                // (lifted off the surface first)
                if let Some(Rest::Structure { point, normal }) = self.rest {
                    let above = SURFACE_OFFSET - normal.dot(self.position - point);
                    self.position += normal * above;
                }
                self.at_rest = false;
                self.rest = None;
            }
        } else {
            self.at_rest = false;
        }
        self.velocity += acceleration;

        if matches!(self.rest, Some(Rest::Structure { .. })) && !pushed {
            // (a nudge lying on the ground only turns it about its up)
            let turn = -HALF_TURN + 2.0 * HALF_TURN * seed.next_real();
            self.angular_velocity += self.up * turn;
        } else {
            let mut magnitude = libm::sqrtf(acceleration.dot(acceleration));
            if magnitude < PUSH_EPSILON {
                magnitude = seed.next_real();
            }
            let (length, mut axis) = normalize(Vec3::Z.cross(acceleration));
            if length <= 0.0 {
                axis = random_direction(seed);
            }
            let turn = seed.next_real() * magnitude * HALF_TURN;
            self.angular_velocity += axis * turn;
        }
        self.spin_changed();
    }

    /// Moves it a tick (`item_update`) through `world`.
    pub fn update(
        &mut self,
        item: &Item,
        world: &dyn ItemWorld,
        seed: &mut RandomSeed,
    ) -> ItemStep {
        let mut step = ItemStep::default();
        let antigravity = item.flags & Item::UNAFFECTED_BY_GRAVITY != 0;
        if item.flags & Item::ALWAYS_MAINTAINS_Z_UP != 0 && (self.up.z - 1.0).abs() >= PUSH_EPSILON
        {
            self.up = Vec3::Z;
            let left = self.up.cross(self.forward);
            let (length, forward) = normalize(left.cross(self.up));
            self.forward = if length == 0.0 { Vec3::X } else { forward };
        }

        if !self.at_rest {
            self.fly(antigravity, world, &mut step);
        } else if !antigravity {
            self.lie(world, seed);
        }

        if self.spinning {
            let (axis, sine, cosine) = self.rotation();
            self.forward = rotate_about(self.forward, axis, sine, cosine);
            self.up = rotate_about(self.up, axis, sine, cosine);
            if self.at_rest {
                // (the ground point's frame squared up: the marker is the
                // origin)
                let left = self.up.cross(self.forward);
                self.forward = normalize(left.cross(self.up)).1;
            }
            self.up = normalize(self.up).1;
            let left = self.up.cross(self.forward);
            self.forward = normalize(left.cross(self.up)).1;
        }

        if self.detonation_ticks > 0 {
            self.detonation_ticks -= 1;
            step.detonated = self.detonation_ticks == 0;
        }
        step
    }

    fn fly(&mut self, antigravity: bool, world: &dyn ItemWorld, step: &mut ItemStep) {
        let mut velocity = self.velocity;
        if !antigravity {
            velocity.z -= GRAVITY;
        }
        let mut position = self.position + velocity;
        if let Some(hit) = world.line(self.position, position) {
            let scale =
                (libm::sqrtf(velocity.dot(velocity)) / MAXIMUM_IMPACT_VELOCITY).clamp(0.0, 1.0);
            step.impact = Some(Impact {
                point: hit.point,
                normal: hit.normal,
                struck: hit.struck,
                scale,
                sound_position: position + hit.normal * SURFACE_OFFSET,
            });
            if !matches!(hit.struck, Struck::Other)
                && hit.normal.z > REST_SLOPE
                && -hit.normal.dot(velocity) < REST_SPEED
            {
                position = hit.point;
                self.align(hit.normal);
                velocity = Vec3::ZERO;
                self.angular_velocity = hit.normal * self.angular_velocity.dot(hit.normal);
                self.at_rest = true;
                step.came_to_rest = true;
                self.rest = match hit.struck {
                    Struck::Body(body) => world
                        .body_offset(body, hit.point)
                        .map(|offset| Rest::Body { body, offset }),
                    _ => Some(Rest::Structure {
                        point: hit.point,
                        normal: hit.normal,
                    }),
                };
                self.rotation_axis = hit.normal;
                self.spin_changed();
            } else {
                let mut reflection = -BOUNCE * hit.normal.dot(velocity);
                if hit.struck != Struck::Level {
                    reflection = reflection.min(BODY_BOUNCE_MAX);
                }
                position = hit.point;
                velocity += hit.normal * reflection;
                if world.inside(position) {
                    position = hit.point + hit.normal * SURFACE_OFFSET;
                }
            }
        }
        self.velocity = velocity;
        self.position = position;
    }

    fn lie(&mut self, world: &dyn ItemWorld, seed: &mut RandomSeed) {
        if let Some(Rest::Body { body, offset }) = self.rest {
            match world.body_point(body, offset) {
                Some(point) => {
                    // (carried with what it lies on)
                    self.position = point;
                    self.align(self.rotation_axis);
                }
                None => {
                    self.rest = None;
                    self.accelerate(Vec3::new(0.0, 0.0, -GRAVITY), seed);
                }
            }
        }
        self.angular_velocity *= REST_SPIN_DAMPING;
        self.spin_changed();
    }

    /// Turns it so that its up is `normal`, turning its forward with it
    /// (`item_align_to_normal_and_point`, the marker at its origin).
    fn align(&mut self, normal: Vec3) {
        let scale = libm::sqrtf(2.0 * (self.up.dot(normal) + 1.0));
        let forward = if scale > 0.01 {
            let v = self.up.cross(normal) / scale;
            let w = scale * 0.5;
            let p = self.forward;
            // (the quaternion (v, w) applied to p)
            let t = v.cross(p) * 2.0;
            p + t * w + v.cross(t)
        } else {
            normal.cross(self.forward).cross(normal)
        };
        self.forward = normalize(forward).1;
        self.up = normal;
    }

    /// `item_adjust_for_angular_velocity_change`.
    fn spin_changed(&mut self) {
        let magnitude = libm::sqrtf(self.angular_velocity.dot(self.angular_velocity));
        if magnitude != 0.0 {
            self.spinning = true;
            if !self.at_rest {
                self.rotation_axis = self.angular_velocity / magnitude;
            }
            self.rotation_sine = libm::sinf(magnitude);
            self.rotation_cosine = libm::cosf(magnitude);
        } else {
            self.spinning = false;
            self.rotation_sine = 0.0;
            self.rotation_cosine = 1.0;
        }
    }
}

/// `v` turned about the unit `n` by the right hand
/// (`rotate_vector_about_axis`).
fn rotate_about(v: Vec3, n: Vec3, sine: f32, cosine: f32) -> Vec3 {
    let along = n.dot(v) * (1.0 - cosine);
    let cross = Vec3::new(
        n.z * v.y - v.z * n.y,
        n.x * v.z - v.x * n.z,
        n.y * v.x - v.y * n.x,
    );
    v * cosine + n * along - cross * sine
}

/// A vector's length and direction (unchanged below the game's epsilon).
fn normalize(v: Vec3) -> (f32, Vec3) {
    let length = libm::sqrtf(v.dot(v));
    if length < PUSH_EPSILON {
        (0.0, v)
    } else {
        (length, v / length)
    }
}

/// A direction drawn from the seed: one draw, as the game's table takes.
fn random_direction(seed: &mut RandomSeed) -> Vec3 {
    // (the golden-angle spiral over the sphere's 642 points: a stand-in for
    // the geosphere's)
    let index = f32::from(seed.next_range(0, 642));
    let z = 1.0 - 2.0 * (index + 0.5) / 642.0;
    let radius = libm::sqrtf((1.0 - z * z).max(0.0));
    let angle = index * 2.399_963_1;
    Vec3::new(radius * libm::cosf(angle), radius * libm::sinf(angle), z)
}

/// An item lying about in the world: its definition's item part and how it
/// moves.
#[derive(Component, Clone, Debug, PartialEq)]
pub struct Lying {
    /// Its definition's item part.
    pub item: Item,
    /// How it moves.
    pub motion: ItemMotion,
}

impl Lying {
    /// An item at `transform`, moving at `velocity` world units a tick.
    pub fn new(item: Item, transform: &Transform, velocity: Vec3) -> Lying {
        let mut motion = ItemMotion::placed(
            transform.position,
            transform.rotation * Vec3::X,
            transform.rotation * Vec3::Z,
        );
        motion.velocity = velocity;
        Lying { item, motion }
    }
}

/// The orientation of an item facing `forward` with `up` (the two square,
/// as the game keeps them).
pub fn orientation(forward: Vec3, up: Vec3) -> Quat {
    let left = up.cross(forward);
    Quat::from_mat3(&Mat3::from_cols(forward, left, up)).normalize()
}

/// Moves every item lying about a tick through `physics`, its transform
/// following; what they met.
pub fn step_lying(world: &mut Ecs, physics: &World) -> Vec<Impact> {
    let mut seed = world.remove_resource::<RandomSeed>().unwrap_or_default();
    let mut impacts = Vec::new();
    let mut lying: Vec<_> = world
        .query::<(&ObjectId, &mut Lying, &mut Transform)>()
        .iter_mut(world)
        .collect();
    // (in the order the objects were made, so every machine draws the
    // seed alike)
    lying.sort_by_key(|(id, ..)| **id);
    for (_, mut lying, mut transform) in lying {
        if lying.motion.at_rest && !lying.motion.spinning {
            continue;
        }
        let Lying { item, motion } = &mut *lying;
        let step = motion.update(item, physics, &mut seed);
        impacts.extend(step.impact);
        transform.position = motion.position;
        transform.rotation = orientation(motion.forward, motion.up);
    }
    world.insert_resource(seed);
    impacts
}

#[cfg(test)]
mod tests {
    use super::*;
    use opence_physics::PhysicsError;

    /// The ground: the plane z = 0, and a body's frame a shift.
    struct Floor {
        body: Option<(BodyId, Vec3)>,
    }

    impl ItemWorld for Floor {
        fn line(&self, from: Vec3, to: Vec3) -> Option<ItemHit> {
            if from.z >= 0.0 && to.z < 0.0 {
                let t = from.z / (from.z - to.z);
                Some(ItemHit {
                    point: from + (to - from) * t,
                    normal: Vec3::Z,
                    struck: self
                        .body
                        .map_or(Struck::Level, |(body, _)| Struck::Body(body)),
                })
            } else {
                None
            }
        }

        fn inside(&self, point: Vec3) -> bool {
            point.z < 0.0
        }

        fn body_point(&self, body: BodyId, offset: Vec3) -> Option<Vec3> {
            let (id, shift) = self.body?;
            (id == body).then_some(offset + shift)
        }

        fn body_offset(&self, body: BodyId, point: Vec3) -> Option<Vec3> {
            let (id, shift) = self.body?;
            (id == body).then_some(point - shift)
        }
    }

    fn level() -> Floor {
        Floor { body: None }
    }

    fn dropped(height: f32) -> ItemMotion {
        ItemMotion::placed(Vec3::new(0.0, 0.0, height), Vec3::X, Vec3::Z)
    }

    #[test]
    fn a_dropped_item_falls_and_rests_on_the_ground() {
        let mut item = dropped(1.0);
        let mut seed = RandomSeed::default();
        let mut came_to_rest = 0;
        let mut impacts = 0;
        for _ in 0..200 {
            let step = item.update(&Item::default(), &level(), &mut seed);
            came_to_rest += usize::from(step.came_to_rest);
            impacts += usize::from(step.impact.is_some());
        }
        assert!(item.at_rest);
        assert_eq!(came_to_rest, 1);
        assert!(impacts >= 2, "it bounces before it rests: {impacts}");
        assert!(item.position.z.abs() < 1e-5);
        assert_eq!(item.velocity, Vec3::ZERO);
        assert!(matches!(item.rest, Some(Rest::Structure { .. })));
    }

    #[test]
    fn a_fast_fall_bounces_back_with_more_than_it_met() {
        let mut item = dropped(0.05);
        item.velocity = Vec3::new(0.0, 0.0, -0.2);
        let step = item.update(&Item::default(), &level(), &mut RandomSeed::default());
        assert!(step.impact.is_some_and(|impact| impact.scale == 1.0));
        assert!(!item.at_rest);
        let into = 0.2 + GRAVITY;
        assert!((item.velocity.z - (-into + BOUNCE * into)).abs() < 1e-5);
    }

    #[test]
    fn a_steep_slope_never_holds_it() {
        struct Slope;
        impl ItemWorld for Slope {
            fn line(&self, from: Vec3, to: Vec3) -> Option<ItemHit> {
                (to.z < 0.0 && from.z >= 0.0).then(|| ItemHit {
                    point: Vec3::new(to.x, to.y, 0.0),
                    normal: Vec3::new(0.8, 0.0, 0.6),
                    struck: Struck::Other,
                })
            }
            fn inside(&self, _point: Vec3) -> bool {
                true
            }
            fn body_point(&self, _body: BodyId, _offset: Vec3) -> Option<Vec3> {
                None
            }
            fn body_offset(&self, _body: BodyId, _point: Vec3) -> Option<Vec3> {
                None
            }
        }
        let mut item = dropped(0.001);
        item.velocity = Vec3::new(0.0, 0.0, -10.0);
        item.update(&Item::default(), &Slope, &mut RandomSeed::default());
        assert!(!item.at_rest);
        // (a body's bounce is held to 1.5, and the item put off the surface)
        let bounced = item.velocity - Vec3::new(0.0, 0.0, -10.0 - GRAVITY);
        assert!((bounced.length() - BODY_BOUNCE_MAX).abs() < 1e-4);
        assert!((item.position.z - 0.6 * SURFACE_OFFSET).abs() < 1e-5);
    }

    #[test]
    fn an_antigravity_item_hangs_where_it_is() {
        let mut item = dropped(1.0);
        let floating = Item {
            flags: Item::UNAFFECTED_BY_GRAVITY,
            ..Item::default()
        };
        for _ in 0..30 {
            item.update(&floating, &level(), &mut RandomSeed::default());
        }
        assert_eq!(item.position, Vec3::new(0.0, 0.0, 1.0));
    }

    #[test]
    fn a_push_frees_it_and_spins_it_about_the_push_s_side() {
        let mut item = dropped(0.5);
        let mut seed = RandomSeed::default();
        for _ in 0..200 {
            item.update(&Item::default(), &level(), &mut seed);
        }
        assert!(item.at_rest);
        item.accelerate(Vec3::new(0.2, 0.0, 0.1), &mut seed);
        assert!(!item.at_rest && item.rest.is_none());
        assert!((item.position.z - SURFACE_OFFSET).abs() < 1e-5);
        let axis = item.rotation().0;
        assert!(axis.dot(Vec3::Y).abs() > 0.999, "{axis}");
        let before = item.forward;
        item.update(&Item::default(), &level(), &mut seed);
        assert!(item.forward.dot(before) < 1.0);
        assert!((item.forward.length() - 1.0).abs() < 1e-4);
        assert!(item.forward.dot(item.up).abs() < 1e-4);

        // (a push straight up spins it about some direction, from one draw)
        let mut straight = dropped(1.0);
        let mut first = RandomSeed::default();
        straight.accelerate(Vec3::new(0.0, 0.0, 0.3), &mut first);
        let mut counted = RandomSeed::default();
        counted.next_u16();
        counted.next_u16();
        assert_eq!(first, counted);
        assert!((straight.rotation().0.length() - 1.0).abs() < 1e-4);

        // (one that does not accelerate is left be)
        let mut fixed = dropped(1.0);
        fixed.does_not_accelerate = true;
        fixed.accelerate(Vec3::X, &mut seed);
        assert_eq!(fixed.velocity, Vec3::ZERO);
    }

    #[test]
    fn a_nudge_on_the_ground_turns_it_where_it_lies() {
        let mut item = dropped(0.01);
        let mut seed = RandomSeed(7);
        for _ in 0..50 {
            item.update(&Item::default(), &level(), &mut seed);
        }
        assert!(item.at_rest);
        item.accelerate(Vec3::ZERO, &mut seed);
        assert!(item.at_rest, "a nudge leaves it lying");
        assert!(item.angular_velocity.x == 0.0 && item.angular_velocity.y == 0.0);
        let spin = item.angular_velocity.z.abs();
        assert!(spin > 0.0 && spin <= HALF_TURN);
        let before = item.forward;
        item.update(&Item::default(), &level(), &mut seed);
        assert!(item.forward.dot(before) < 1.0, "it turns");
        assert!((item.up - Vec3::Z).length() < 1e-4, "about its up");
        // (and the turn dies away)
        for _ in 0..300 {
            item.update(&Item::default(), &level(), &mut seed);
        }
        assert!(item.angular_velocity.length() < 1e-6);
        assert!(item.position.z.abs() < 1e-5);
    }

    /// A world with a box's top at z = 0.5, and the box.
    fn boxed() -> Result<(World, BodyId), PhysicsError> {
        let mut world = World::new(opence_physics::WorldSettings::default())?;
        let body = world.add_body(
            opence_physics::Shape::Box {
                half_extents: Vec3::new(2.0, 2.0, 0.5),
            },
            opence_physics::Pose {
                position: Vec3::ZERO,
                rotation: glam::Quat::IDENTITY,
            },
            opence_physics::Motion::Kinematic,
            1.0,
            opence_physics::Material {
                friction: 0.5,
                restitution: 0.0,
            },
        )?;
        Ok((world, body))
    }

    #[test]
    fn it_rides_what_it_rests_on_and_falls_when_that_goes() -> Result<(), PhysicsError> {
        let (_world, body) = boxed()?;
        let mut floor = Floor {
            body: Some((body, Vec3::ZERO)),
        };
        let mut item = dropped(0.01);
        let mut seed = RandomSeed::default();
        for _ in 0..50 {
            item.update(&Item::default(), &floor, &mut seed);
        }
        assert!(matches!(item.rest, Some(Rest::Body { .. })));
        floor.body = Some((body, Vec3::new(1.0, 0.0, 0.0)));
        item.update(&Item::default(), &floor, &mut seed);
        assert!((item.position.x - 1.0).abs() < 1e-5);
        floor.body = None;
        item.update(&Item::default(), &floor, &mut seed);
        assert!(!item.at_rest && item.rest.is_none());
        assert!(item.velocity.z < 0.0);
        Ok(())
    }

    #[test]
    fn it_lands_upright_and_keeps_upright_if_it_must() {
        let mut item =
            ItemMotion::placed(Vec3::new(0.0, 0.0, 1.0), Vec3::X, Vec3::new(0.6, 0.0, 0.8));
        item.forward = Vec3::new(0.8, 0.0, -0.6);
        let upright = Item {
            flags: Item::ALWAYS_MAINTAINS_Z_UP,
            ..Item::default()
        };
        item.update(&upright, &level(), &mut RandomSeed::default());
        assert_eq!(item.up, Vec3::Z);
        assert!((item.forward - Vec3::X).length() < 1e-5);

        // (straight up, its forward is the world's)
        let mut flat = ItemMotion::placed(Vec3::ONE, Vec3::Z, Vec3::Y);
        flat.update(&upright, &level(), &mut RandomSeed::default());
        assert_eq!(flat.forward, Vec3::X);

        // (an item on its side, landing, is turned up onto the ground)
        let mut side = ItemMotion::placed(Vec3::new(0.0, 0.0, 0.002), Vec3::X, Vec3::Y);
        side.update(&Item::default(), &level(), &mut RandomSeed::default());
        assert!(side.at_rest);
        assert_eq!(side.up, Vec3::Z);
        assert!((side.forward - Vec3::X).length() < 1e-5);
        // (and upside down, the other way round)
        let mut flipped = ItemMotion::placed(Vec3::new(0.0, 0.0, 0.002), Vec3::X, -Vec3::Z);
        flipped.update(&Item::default(), &level(), &mut RandomSeed::default());
        assert_eq!(flipped.up, Vec3::Z);
        assert!((flipped.forward - Vec3::X).length() < 1e-5);
    }

    #[test]
    fn it_detonates_when_its_ticks_run_out() {
        let mut item = dropped(1.0);
        item.detonation_ticks = 2;
        let mut seed = RandomSeed::default();
        assert!(!item.update(&Item::default(), &level(), &mut seed).detonated);
        assert!(item.update(&Item::default(), &level(), &mut seed).detonated);
        assert!(!item.update(&Item::default(), &level(), &mut seed).detonated);
    }

    #[test]
    fn the_turn_is_the_games() {
        // (rotate_vector_about_axis turns x toward y about z)
        let turned = rotate_about(Vec3::X, Vec3::Z, 1.0, 0.0);
        assert!((turned - Vec3::Y).length() < 1e-6);
        for draw in 0..20 {
            let direction = random_direction(&mut RandomSeed(draw));
            assert!((direction.length() - 1.0).abs() < 1e-4);
        }
    }

    #[test]
    fn the_physics_world_s_items_fall_onto_its_bodies() -> Result<(), PhysicsError> {
        let (mut world, body) = boxed()?;
        assert!(!world.inside(Vec3::ZERO));
        let mut item = dropped(1.0);
        let mut seed = RandomSeed::default();
        for _ in 0..200 {
            item.update(&Item::default(), &world, &mut seed);
        }
        assert!(item.at_rest);
        assert!((item.position.z - 0.5).abs() < 1e-3, "{}", item.position);
        assert!(matches!(item.rest, Some(Rest::Body { body: on, .. }) if on == body));
        let Some(Rest::Body { offset, .. }) = item.rest else {
            return Err(PhysicsError::Refused);
        };
        assert!((offset.z - 0.5).abs() < 1e-3);
        world.remove_body(body);
        assert!(world.body_point(body, offset).is_none());
        assert!(world.body_offset(body, offset).is_none());
        item.update(&Item::default(), &world, &mut seed);
        assert!(!item.at_rest);
        Ok(())
    }
}
