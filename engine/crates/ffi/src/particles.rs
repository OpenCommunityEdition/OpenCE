//! What the renderer draws as sprites beside the level and its objects:
//! the particles effects emit, the contrails objects trail, the weather
//! that fills a box about the camera where its cluster has some, and the
//! detail objects (grass and the like) a BSP scatters over its ground.

use std::collections::{BTreeMap, HashMap, HashSet};

use opence_content::{
    EffectParticles, Level, Particle, PointPhysics, StructureBsp, TagRef, WeatherParticleType,
};
use opence_sim::effects::{FiredPart, Running};
use opence_sim::objects::ObjectHandle;
use opence_sim::particle_collision::PointHit;
use opence_sim::point_physics;
use opence_sim::seats::default_pose;
use opence_sim::{Simulation, Vec3};

use crate::lights::marker_frames;

mod systems;
use crate::snapshot::{
    ParticleSnapshot, MAX_PARTICLES, PARTICLE_FACING_AXIS, PARTICLE_FACING_SCREEN,
    PARTICLE_FACING_SEGMENT, PARTICLE_NO_PICTURE,
};

/// A tick, in seconds.
const TICK: f32 = 1.0 / 30.0;
/// The squared speed below which a settled particle is at rest.
const AT_REST_SPEED_SQUARED: f32 = 0.0625;
/// The speeds at which a particle's collision effect is played at none
/// of its scale, and at all of it.
const COLLISION_SCALE_SPEEDS: [f32; 2] = [0.5, 1.5];
/// The least a surface a particle meets faces up (its normal's z) for it
/// to settle on it.
const SETTLES_ON: f32 = 0.8;
/// The most particles the effects keep alive.
const MAX_LIVE: usize = 1024;
/// The most weather particles of one kind.
const MAX_WEATHER: usize = 384;
/// The most points one contrail keeps.
const MAX_TRAIL_POINTS: usize = 64;

/// A small random sequence (xorshift), the particles' own, so that they
/// take nothing from the simulation's.
#[derive(Clone, Debug)]
struct Random(u32);

impl Default for Random {
    fn default() -> Self {
        Self(0x9E37_79B9)
    }
}

impl Random {
    /// From 0 up to 1.
    fn next(&mut self) -> f32 {
        let mut x = self.0;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        self.0 = x;
        (x >> 8) as f32 / (1u32 << 24) as f32
    }

    /// Between `bounds`' least and most.
    fn between(&mut self, [least, most]: [f32; 2]) -> f32 {
        least + (most - least) * self.next()
    }

    /// One of `0..count` (0 if there are none).
    fn below(&mut self, count: i32) -> i32 {
        ((self.next() * count.max(1) as f32) as i32).min(count.max(1) - 1)
    }

    /// A direction, any way.
    fn direction(&mut self) -> Vec3 {
        let z = 2.0 * self.next() - 1.0;
        let angle = std::f32::consts::TAU * self.next();
        let across = (1.0 - z * z).max(0.0).sqrt();
        Vec3::new(across * libm::cosf(angle), across * libm::sinf(angle), z)
    }

    /// A direction within `angle` (the cone's whole angle) of `axis`.
    fn within(&mut self, axis: Vec3, angle: f32) -> Vec3 {
        let axis = axis.normalize_or(Vec3::X);
        if angle <= 0.0 {
            return axis;
        }
        let cos = 1.0 - self.next() * (1.0 - libm::cosf((angle * 0.5).min(std::f32::consts::PI)));
        let sin = (1.0 - cos * cos).max(0.0).sqrt();
        let around = std::f32::consts::TAU * self.next();
        let side = axis.any_orthonormal_vector();
        let other = axis.cross(side);
        axis * cos + (side * libm::cosf(around) + other * libm::sinf(around)) * sin
    }

    /// `axis` turned by `angle` about a random axis, as the original turns
    /// an emitter's way within its cone.
    fn tilted(&mut self, axis: Vec3, angle: f32) -> Vec3 {
        let axis = axis.normalize_or(Vec3::X);
        if angle == 0.0 {
            return axis;
        }
        let about = self.direction();
        let (sin, cos) = (libm::sinf(angle), libm::cosf(angle));
        axis * cos + about.cross(axis) * sin + about * about.dot(axis) * (1.0 - cos)
    }
}

/// An emitter's particles may be made: in air (no effect is under water)
/// and by a violent effect (an effect is nonviolent only on a corpse,
/// which effects here are not told of).
fn made_here(emitter: &EffectParticles) -> bool {
    !matches!(
        emitter.environment,
        EffectParticles::IN_WATER | EffectParticles::IN_VACUUM
    ) && emitter.disposition != EffectParticles::NONVIOLENT
}

/// An effect's scales A and B and the values an emitter scales by them,
/// as the original's `effect_scale`.
#[derive(Clone, Copy)]
struct Scaled {
    /// The values scale A and scale B scale, each a mask of the emitter's
    /// bits.
    scales: [u32; 2],
    /// The effect's scales A and B.
    by: [f32; 2],
}

impl Scaled {
    /// Some scale scales the value `bit`.
    fn scales(self, bit: u32) -> bool {
        self.scales.iter().any(|mask| mask & bit != 0)
    }

    /// `value` scaled by the scales that scale `bit`.
    fn value(self, value: f32, bit: u32) -> f32 {
        self.scales
            .iter()
            .zip(self.by)
            .filter(|(mask, _)| *mask & bit != 0)
            .fold(value, |value, (_, by)| value * by)
    }

    /// A value between `bounds`: the least scaled by `bit`'s scales and
    /// what is above it by the next bit's.
    fn between(self, random: &mut Random, [least, most]: [f32; 2], bit: u32) -> f32 {
        self.value(least, bit) + random.next() * self.value(most - least, bit << 1)
    }
}

/// The way a yaw and a pitch face, from forward (x).
fn facing([yaw, pitch]: [f32; 2]) -> Vec3 {
    Vec3::new(
        libm::cosf(pitch) * libm::cosf(yaw),
        libm::cosf(pitch) * libm::sinf(yaw),
        libm::sinf(pitch),
    )
}

/// The share of an event's particles emitted by `fraction` of it, by its
/// distribution.
pub(crate) fn emitted_share(distribution: i16, fraction: f32) -> f32 {
    let f = fraction.clamp(0.0, 1.0);
    match distribution {
        EffectParticles::END => {
            if f >= 1.0 {
                1.0
            } else {
                0.0
            }
        }
        EffectParticles::CONSTANT => f,
        EffectParticles::BUILDUP => f * f,
        EffectParticles::FALLOFF => (2.0 - f) * f,
        EffectParticles::BUILDUP_AND_FALLOFF => (3.0 - 2.0 * f) * f * f,
        // (START, and any other)
        _ => 1.0,
    }
}

/// An alpha, red, green, blue colour as red, green, blue and alpha; a
/// colour left all zero is white.
fn rgba([a, r, g, b]: [f32; 4]) -> [f32; 4] {
    if a == 0.0 && r == 0.0 && g == 0.0 && b == 0.0 {
        [1.0; 4]
    } else {
        [r, g, b, a]
    }
}

fn mix(from: [f32; 4], to: [f32; 4], t: f32) -> [f32; 4] {
    [0, 1, 2, 3].map(|i| from[i] + (to[i] - from[i]) * t)
}

/// A point's velocity after `dt`, as `physics` moves it in `wind`,
/// meeting nothing.
fn pushed(physics: &PointPhysics, radius: f32, velocity: Vec3, wind: Vec3, dt: f32) -> Vec3 {
    let (mut at, mut velocity) = (Vec3::ZERO, velocity);
    point_physics::update(
        physics,
        &mut at,
        &mut velocity,
        radius,
        dt,
        (false, wind),
        |_, _| None,
    );
    velocity
}

/// An effect (or sound) a particle made: where it met the world, or where
/// it died.
#[derive(Clone, Debug, PartialEq)]
pub(crate) struct Made {
    /// The effect or sound.
    pub tag: TagRef,
    /// Where.
    pub point: Vec3,
    /// The way it faces: the surface's normal, or the particle's way.
    pub normal: Vec3,
    /// How much of it the original plays, 0 to 1, by the particle's speed
    /// (0 for a death).
    pub scale: f32,
}

/// A particle that met the world, for its material effects' impact event
/// on the material there.
#[derive(Clone, Debug, PartialEq)]
pub(crate) struct ParticleImpact {
    /// Its material effects.
    pub material_effects: TagRef,
    /// Where it met the world.
    pub point: Vec3,
    /// The surface's normal there.
    pub normal: Vec3,
    /// How much of its effect the original plays, 0 to 1.
    pub scale: f32,
}

/// What the particles made in a tick.
#[derive(Clone, Debug, Default)]
pub(crate) struct ParticleEvents {
    /// Their collision and death effects and sounds.
    pub made: Vec<Made>,
    /// Their impacts, for their material effects.
    pub impacts: Vec<ParticleImpact>,
}

impl ParticleEvents {
    fn clear(&mut self) {
        self.made.clear();
        self.impacts.clear();
    }

    /// `particle` dying at `point`, going `way`: its death effect.
    fn died(&mut self, particle: &Particle, point: Vec3, way: Vec3) {
        if let Some(tag) = &particle.death_effect {
            self.made.push(Made {
                tag: tag.clone(),
                point,
                normal: way.normalize_or(Vec3::Z),
                scale: 0.0,
            });
        }
    }
}

/// The level's sprites' pictures: each bitmap's images, one after another.
#[derive(Clone, Debug, Default)]
struct Pictures(HashMap<TagRef, u32>);

/// The bitmaps the level's sprites draw, each once, in order.
fn sprite_bitmaps(level: &Level) -> Vec<&TagRef> {
    let mut bitmaps: Vec<&TagRef> = Vec::new();
    let all = level
        .particles
        .iter()
        .filter_map(|particle| particle.bitmap.as_ref())
        .chain(level.contrails.iter().filter_map(|c| c.bitmap.as_ref()))
        .chain(systems::Systems::bitmaps(level))
        .chain(
            level
                .weather
                .iter()
                .flat_map(|system| &system.types)
                .filter_map(|kind| kind.bitmap.as_ref()),
        )
        .chain(
            level
                .detail_object_collections
                .iter()
                .flatten()
                .filter_map(|collection| collection.bitmap.as_ref()),
        );
    for bitmap in all {
        if level.bitmap(bitmap).is_some() && !bitmaps.contains(&bitmap) {
            bitmaps.push(bitmap);
        }
    }
    bitmaps
}

/// The images the level's sprites draw, in the order the snapshot's
/// particle pictures number them.
pub(crate) fn particle_images(level: &Level) -> Vec<(TagRef, u16)> {
    let mut images = Vec::new();
    for tag in sprite_bitmaps(level) {
        let Some(bitmap) = level.bitmap(tag) else {
            continue;
        };
        let count = u16::try_from(bitmap.images.len()).unwrap_or(u16::MAX);
        images.extend((0..count).map(|image| (tag.clone(), image)));
    }
    images
}

/// A sprite of a bitmap: its picture, its texture coordinates and its
/// size in texels.
#[derive(Clone, Copy, Debug, PartialEq)]
struct Frame {
    picture: u32,
    uv: [f32; 4],
    texels: [f32; 2],
}

impl Pictures {
    fn new(level: &Level) -> Self {
        let mut first = 0u32;
        let mut out = HashMap::new();
        for tag in sprite_bitmaps(level) {
            let count = level.bitmap(tag).map_or(0, |bitmap| bitmap.images.len());
            out.insert(tag.clone(), first);
            first = first.saturating_add(u32::try_from(count).unwrap_or(u32::MAX));
        }
        Self(out)
    }

    /// Frame `frame` of sequence `sequence` of `bitmap`: the sequence's
    /// sprites in turn, or its images.
    fn frame(&self, level: &Level, bitmap: Option<&TagRef>, sequence: i32, frame: usize) -> Frame {
        let whole = [0.0, 0.0, 1.0, 1.0];
        let none = Frame {
            picture: PARTICLE_NO_PICTURE,
            uv: whole,
            texels: [1.0, 1.0],
        };
        let Some((tag, bitmap)) = bitmap.and_then(|tag| Some((tag, level.bitmap(tag)?))) else {
            return none;
        };
        let sequence = usize::try_from(sequence)
            .ok()
            .and_then(|index| bitmap.sequences.get(index));
        let (image, uv) = match sequence {
            Some(sequence) if !sequence.sprites.is_empty() => {
                let sprite = sequence.sprites[frame % sequence.sprites.len()];
                let [left, right, top, bottom] = sprite.bounds;
                (sprite.image, [left, top, right, bottom])
            }
            Some(sequence) if sequence.image_count > 0 => {
                let step = (frame % usize::from(sequence.image_count)) as u16;
                (
                    sequence.first_image.unwrap_or(0).saturating_add(step),
                    whole,
                )
            }
            _ => (0, whole),
        };
        let Some(size) = bitmap.images.get(usize::from(image)) else {
            return none;
        };
        let Some(&first) = self.0.get(tag) else {
            return none;
        };
        Frame {
            picture: first + u32::from(image),
            uv,
            texels: [
                (uv[2] - uv[0]).abs() * f32::from(size.width),
                (uv[3] - uv[1]).abs() * f32::from(size.height),
            ],
        }
    }
}

/// A particle an effect emitted.
#[derive(Clone, Debug, PartialEq)]
struct Live {
    /// Its index in the level's particles.
    particle: usize,
    position: Vec3,
    velocity: Vec3,
    age: f32,
    life: f32,
    /// The radius it was emitted at.
    radius: f32,
    rotation: f32,
    spin: f32,
    color: [f32; 4],
    sequence: i32,
    /// Seconds a frame of its animation shows (worn longer each time it
    /// meets the world), or none if it does not animate.
    frame_span: Option<f32>,
    /// The frames it has shown.
    frame: f32,
    /// It settled and stopped: it moves no more.
    at_rest: bool,
    /// The effect it stays with, and where that was at the last tick.
    anchor: Option<(u32, Vec3)>,
}

impl Live {
    fn emit(
        random: &mut Random,
        index: usize,
        particle: &Particle,
        emitter: &EffectParticles,
        effect: &Running,
    ) -> Self {
        let origin = &effect.origin;
        let scaled = Scaled {
            scales: emitter.scales,
            by: origin.scale,
        };
        let forward = origin.rotation * facing(emitter.direction);
        let speed = scaled.between(random, emitter.velocity, EffectParticles::SCALES_VELOCITY);
        let cone = scaled.value(
            emitter.velocity_cone_angle,
            EffectParticles::SCALES_VELOCITY_CONE,
        );
        let tilt = cone * random.next();
        let direction = random.tilted(forward, tilt);
        let reach = scaled.between(
            random,
            emitter.distribution_radius,
            EffectParticles::SCALES_DISTRIBUTION_RADIUS,
        );
        let spread = random.direction() * reach;
        let position =
            origin.position + origin.rotation * Vec3::from_array(emitter.offset) + spread;
        let attached = emitter.flags & EffectParticles::STAY_ATTACHED != 0;
        let mut velocity = direction * speed;
        if !attached {
            velocity += origin.velocity;
        }
        let radius = scaled.between(random, emitter.radius, EffectParticles::SCALES_RADIUS);
        let spin = scaled.between(
            random,
            emitter.angular_velocity,
            EffectParticles::SCALES_ANGULAR_VELOCITY,
        );
        let rotation = if emitter.flags & EffectParticles::RANDOM_ROTATION != 0 {
            std::f32::consts::TAU * random.next()
        } else {
            0.0
        };
        let along = if scaled.scales(EffectParticles::SCALES_TINT) {
            scaled.value(1.0, EffectParticles::SCALES_TINT)
        } else {
            random.next()
        };
        let mut color = mix(rgba(emitter.tint[0]), rgba(emitter.tint[1]), along);
        if emitter.flags & EffectParticles::TINT_FROM_CHANGE_COLOR != 0 {
            for (channel, by) in color.iter_mut().zip(origin.color) {
                *channel *= by;
            }
        }
        Self {
            particle: index,
            position,
            velocity,
            age: 0.0,
            life: random.between(particle.lifespan).max(TICK),
            radius: radius.max(0.0),
            rotation,
            spin,
            color,
            sequence: i32::from(particle.first_sequence)
                + random.below(i32::from(particle.initial_sequences)),
            frame_span: (particle.fps[1] > 0.0)
                .then(|| random.between(particle.fps))
                .filter(|&fps| fps > 0.0)
                .map(|fps| 1.0 / fps),
            frame: 0.0,
            at_rest: false,
            anchor: attached.then_some((effect.number, origin.position)),
        }
    }

    /// Its radius now, by its particle's radius over its life.
    fn radius_now(&self, particle: &Particle) -> f32 {
        self.radius * particle.radius_at(self.age / self.life)
    }

    /// One tick of its physics (`particle_update_physics`), in `wind`,
    /// meeting the world where `collide` says: false if it died (its death
    /// effect into `events`) or is gone. What it makes meeting the world
    /// goes into `events` too.
    fn step(
        &mut self,
        particle: &Particle,
        wind: Vec3,
        mut collide: impl FnMut(Vec3, Vec3) -> Option<PointHit>,
        events: &mut ParticleEvents,
    ) -> bool {
        if self.at_rest {
            return true;
        }
        let mut settled = false;
        let radius = self.radius_now(particle);
        match (&particle.physics, self.anchor.is_some()) {
            (Some(physics), true) => {
                self.velocity = point_physics::damped(physics, radius, self.velocity, TICK);
                self.position += self.velocity * TICK;
                settled = true;
            }
            (Some(physics), false) => {
                let moved = point_physics::update(
                    physics,
                    &mut self.position,
                    &mut self.velocity,
                    radius,
                    TICK,
                    (false, wind),
                    &mut collide,
                );
                if let Some(hit) = moved.collision {
                    let [none, all] = COLLISION_SCALE_SPEEDS;
                    let scale = ((self.velocity.length() - none) / (all - none)).clamp(0.0, 1.0);
                    if let Some(tag) = &particle.collision_effect {
                        events.made.push(Made {
                            tag: tag.clone(),
                            point: self.position,
                            normal: hit.normal,
                            scale,
                        });
                    }
                    if let Some(effects) = &particle.collision_material_effects {
                        events.impacts.push(ParticleImpact {
                            material_effects: effects.clone(),
                            point: self.position,
                            normal: hit.normal,
                            scale,
                        });
                    }
                    if particle.flags & Particle::DIES_ON_CONTACT_WITH_STRUCTURE != 0 {
                        // (with a collision effect it is only gone)
                        if particle.collision_effect.is_none() {
                            events.died(particle, self.position, self.velocity);
                        }
                        return false;
                    }
                }
                let dies = if moved.in_water {
                    Particle::DIES_ON_CONTACT_WITH_WATER
                } else {
                    Particle::DIES_ON_CONTACT_WITH_AIR
                };
                if particle.flags & dies != 0 {
                    events.died(particle, self.position, self.velocity);
                    return false;
                }
                if let Some(hit) = moved.collision {
                    settled = hit.normal.z > SETTLES_ON;
                    if let Some(span) = &mut self.frame_span {
                        *span += particle.contact_deterioration;
                    }
                }
            }
            (None, _) => self.position += self.velocity * TICK,
        }
        if settled && self.velocity.length_squared() < AT_REST_SPEED_SQUARED {
            if particle.flags & Particle::DIES_AT_REST != 0 {
                events.died(particle, self.position, self.velocity);
                return false;
            }
            self.at_rest = true;
        }
        self.rotation += self.spin * TICK;
        true
    }

    /// One tick of its animation, unless it stopped at rest.
    fn animate(&mut self, particle: &Particle) {
        if self.at_rest && particle.flags & Particle::ANIMATION_STOPS_AT_REST != 0 {
            return;
        }
        if let Some(span) = self.frame_span.filter(|&span| span > 0.0) {
            self.frame += TICK / span;
        }
    }

    fn snapshot(&self, level: &Level, pictures: &Pictures) -> Option<ParticleSnapshot> {
        let particle = level.particles.get(self.particle)?;
        let radius = self.radius_now(particle);
        let fade_in = if particle.fade_in > 0.0 {
            (self.age / particle.fade_in).min(1.0)
        } else {
            1.0
        };
        let fade_out = if particle.fade_out > 0.0 {
            ((self.life - self.age) / particle.fade_out).clamp(0.0, 1.0)
        } else {
            1.0
        };
        let frame = pictures.frame(
            level,
            particle.bitmap.as_ref(),
            self.sequence,
            self.frame as usize,
        );
        let along = self.velocity.normalize_or_zero();
        let facing = match particle.orientation {
            1 | 3 if along != Vec3::ZERO => PARTICLE_FACING_AXIS,
            _ => PARTICLE_FACING_SCREEN,
        };
        let [r, g, b, a] = self.color;
        Some(ParticleSnapshot {
            position: [self.position.x, self.position.y, self.position.z, radius],
            axis: [along.x, along.y, along.z, self.rotation],
            color: [r, g, b, a * fade_in * fade_out],
            uv: frame.uv,
            shape: [radius, 0.0, 0.0, 0.0],
            modes: [
                frame.picture,
                blend(particle.shader.framebuffer_blend_function),
                facing,
                0,
            ],
        })
    }
}

/// A framebuffer blend function as the snapshot gives it.
fn blend(function: i16) -> u32 {
    u32::try_from(function).unwrap_or(0).min(7)
}

/// A weather particle, drifting in the box about the camera.
#[derive(Clone, Debug, PartialEq)]
struct Drop {
    position: Vec3,
    velocity: Vec3,
    acceleration: Vec3,
    radius: f32,
    rotation: f32,
    spin: f32,
    color: [f32; 4],
    fps: f32,
    age: f32,
}

/// The weather about the camera.
#[derive(Clone, Debug, Default)]
struct Weather {
    /// The system falling (its index in the level's weather).
    system: Option<usize>,
    /// Each of its kinds' particles.
    drops: Vec<Vec<Drop>>,
    wind: Vec3,
}

/// The size of a weather kind's box.
fn box_size(kind: &WeatherParticleType) -> f32 {
    if kind.distance_fade[3] > 0.0 {
        kind.distance_fade[3].min(64.0)
    } else {
        10.0
    }
}

/// The weather, and its wind, of the cluster `point` is in.
fn weather_at(level: &Level, bsp: &StructureBsp, point: Vec3) -> (Option<usize>, Vec3) {
    let entry = bsp
        .collision
        .leaf_at(point.to_array())
        .and_then(|leaf| *bsp.leaf_clusters.get(leaf as usize)?)
        .and_then(|cluster| *bsp.cluster_weather.get(usize::from(cluster))?)
        .and_then(|index| bsp.weather_palette.get(usize::from(index)));
    let Some(entry) = entry else {
        return (None, Vec3::ZERO);
    };
    let [x, y, z, speed] = entry.wind;
    let wind = Vec3::new(x, y, z).normalize_or_zero() * speed;
    let system = entry
        .system
        .as_ref()
        .and_then(|tag| level.weather.iter().position(|system| &system.tag == tag));
    (system, wind)
}

/// `offset` brought into the box of side `side` about 0.
fn wrapped(offset: Vec3, side: f32) -> Vec3 {
    let wrap = |value: f32| {
        let mut value = libm::fmodf(value + side * 0.5, side);
        if value < 0.0 {
            value += side;
        }
        value - side * 0.5
    };
    Vec3::new(wrap(offset.x), wrap(offset.y), wrap(offset.z))
}

impl Weather {
    fn tick(&mut self, level: &Level, bsp: &StructureBsp, camera: Vec3, random: &mut Random) {
        let (system, wind) = weather_at(level, bsp, camera);
        self.wind = wind;
        if system != self.system {
            self.system = system;
            self.drops.clear();
            for kind in system
                .and_then(|index| level.weather.get(index))
                .map_or(&[][..], |system| system.types.as_slice())
            {
                let side = box_size(kind);
                let density = (kind.density[0] + kind.density[1]) * 0.5;
                let count = ((density * side * side * side).max(0.0) as usize).min(MAX_WEATHER);
                let drops = (0..count)
                    .map(|_| {
                        let place = Vec3::new(random.next(), random.next(), random.next());
                        let magnitude =
                            random.between([kind.acceleration[0], kind.acceleration[1]]);
                        Drop {
                            position: camera + (place - Vec3::splat(0.5)) * side,
                            velocity: Vec3::ZERO,
                            acceleration: random.direction() * magnitude,
                            radius: random.between(kind.radius),
                            rotation: if kind.flags & WeatherParticleType::RANDOM_ROTATION != 0 {
                                std::f32::consts::TAU * random.next()
                            } else {
                                0.0
                            },
                            spin: random.between(kind.rotation_rate),
                            color: mix(rgba(kind.color[0]), rgba(kind.color[1]), random.next()),
                            fps: random.between(kind.animation_rate),
                            age: 0.0,
                        }
                    })
                    .collect();
                self.drops.push(drops);
            }
        }
        let Some(system) = self.system.and_then(|index| level.weather.get(index)) else {
            return;
        };
        for (kind, drops) in system.types.iter().zip(&mut self.drops) {
            let side = box_size(kind);
            for drop in drops {
                // (its acceleration turns at random)
                let magnitude = drop.acceleration.length();
                if magnitude > 0.0 {
                    let turned = drop.acceleration
                        + random.direction() * magnitude * kind.acceleration[2] * TICK;
                    drop.acceleration = turned.normalize_or_zero() * magnitude;
                }
                drop.velocity += drop.acceleration * TICK;
                if let Some(physics) = &kind.physics {
                    drop.velocity = pushed(physics, drop.radius, drop.velocity, wind, TICK);
                }
                drop.position =
                    camera + wrapped(drop.position + drop.velocity * TICK - camera, side);
                drop.rotation += drop.spin * TICK;
                drop.age += TICK;
            }
        }
    }

    fn gather(
        &self,
        level: &Level,
        pictures: &Pictures,
        camera: Vec3,
        out: &mut Vec<ParticleSnapshot>,
    ) {
        let Some(system) = self.system.and_then(|index| level.weather.get(index)) else {
            return;
        };
        for (kind, drops) in system.types.iter().zip(&self.drops) {
            let side = box_size(kind);
            let [near, full, start, far] = kind.distance_fade;
            for drop in drops {
                if out.len() >= MAX_PARTICLES {
                    return;
                }
                let position = camera + wrapped(drop.position - camera, side);
                let distance = position.distance(camera);
                let fade_in = if full > near {
                    ((distance - near) / (full - near)).clamp(0.0, 1.0)
                } else {
                    1.0
                };
                let fade_out = if far > start {
                    ((far - distance) / (far - start)).clamp(0.0, 1.0)
                } else {
                    1.0
                };
                let frame = pictures.frame(
                    level,
                    kind.bitmap.as_ref(),
                    0,
                    (drop.age * drop.fps.max(0.0)) as usize,
                );
                let along = drop.velocity.normalize_or_zero();
                let (facing, axis) = match kind.render_mode {
                    1 if along != Vec3::ZERO => (PARTICLE_FACING_AXIS, along),
                    1 => (PARTICLE_FACING_AXIS, Vec3::Z),
                    _ => (PARTICLE_FACING_SCREEN, along),
                };
                let [r, g, b, a] = drop.color;
                out.push(ParticleSnapshot {
                    position: [position.x, position.y, position.z, drop.radius],
                    axis: [axis.x, axis.y, axis.z, drop.rotation],
                    color: [r, g, b, a * fade_in * fade_out],
                    uv: frame.uv,
                    shape: [drop.radius, 0.0, 0.0, 0.0],
                    modes: [
                        frame.picture,
                        blend(kind.shader.framebuffer_blend_function),
                        facing,
                        0,
                    ],
                });
            }
        }
    }
}

/// A point of a contrail.
#[derive(Clone, Debug, PartialEq)]
struct TrailPoint {
    position: Vec3,
    velocity: Vec3,
    age: f32,
    /// Where it lies between its states' two colours.
    shade: f32,
}

/// A contrail an object trails.
#[derive(Clone, Debug, PartialEq)]
struct Trail {
    /// Its index in the level's contrails.
    contrail: usize,
    points: Vec<TrailPoint>,
    /// The points owed, by its rate, not yet made.
    owed: f32,
    /// Its object still carries it.
    carried: bool,
}

/// What a contrail's point of `age` is: its width and its colour (none:
/// it has outlived its states).
fn trail_state(
    states: &[opence_content::ContrailState],
    age: f32,
    shade: f32,
) -> Option<(f32, [f32; 4])> {
    let mut start = 0.0;
    for (index, state) in states.iter().enumerate() {
        let duration = (state.duration[0] + state.duration[1]) * 0.5;
        let transition = (state.transition[0] + state.transition[1]) * 0.5;
        let color = mix(rgba(state.color[0]), rgba(state.color[1]), shade);
        if age < start + duration {
            return Some((state.width, color));
        }
        if age < start + duration + transition {
            let t = (age - start - duration) / transition.max(1e-6);
            let Some(next) = states.get(index + 1) else {
                let mut faded = color;
                faded[3] *= 1.0 - t;
                return Some((state.width, faded));
            };
            let to = mix(rgba(next.color[0]), rgba(next.color[1]), shade);
            return Some((
                state.width + (next.width - state.width) * t,
                mix(color, to, t),
            ));
        }
        start += duration + transition;
    }
    None
}

/// A contrail an object definition carries: its attachment's slot, its
/// index in the level's contrails, and the marker it leaves from.
type CarriedContrail = (usize, usize, String);

/// The contrails objects carry, and those they trail.
#[derive(Clone, Debug, Default)]
struct Contrails {
    /// Each object definition's contrails, and the model their markers
    /// are on.
    carried: BTreeMap<TagRef, (Vec<CarriedContrail>, Option<usize>)>,
    /// Each object's trails, by its slot and the attachment's.
    trails: HashMap<(ObjectHandle, usize), Trail>,
}

impl Contrails {
    fn new(level: &Level) -> Self {
        let mut carried = BTreeMap::new();
        for object in &level.objects {
            let contrails: Vec<_> = object
                .attachments
                .iter()
                .enumerate()
                .filter_map(|(slot, attachment)| {
                    let tag = attachment.tag.as_ref()?;
                    let index = level.contrails.iter().position(|c| &c.tag == tag)?;
                    Some((slot, index, attachment.marker.clone()))
                })
                .collect();
            if contrails.is_empty() {
                continue;
            }
            let model = object
                .model
                .as_ref()
                .and_then(|model| level.model_index(model));
            carried.insert(object.tag.clone(), (contrails, model));
        }
        Self {
            carried,
            trails: HashMap::new(),
        }
    }

    fn tick(&mut self, level: &Level, simulation: &mut Simulation, random: &mut Random) {
        if self.carried.is_empty() {
            return;
        }
        for trail in self.trails.values_mut() {
            trail.carried = false;
        }
        let handles: HashMap<u32, ObjectHandle> = simulation
            .objects()
            .into_iter()
            .map(|handle| (handle.index, handle))
            .collect();
        let carried = &self.carried;
        let trails = &mut self.trails;
        simulation.visit_lit(|object| {
            let Some((contrails, model)) = carried.get(object.tag) else {
                return;
            };
            let Some(&handle) = handles.get(&object.id.0) else {
                return;
            };
            let model = model.and_then(|index| level.models.get(index));
            let rest = match (object.nodes, model) {
                (None, Some(model)) => default_pose(model),
                _ => Vec::new(),
            };
            for (slot, index, marker) in contrails {
                let Some(contrail) = level.contrails.get(*index) else {
                    continue;
                };
                let (current, _) = marker_frames(model, &object, &rest, marker);
                let trail = trails.entry((handle, *slot)).or_insert_with(|| Trail {
                    contrail: *index,
                    points: Vec::new(),
                    owed: 0.0,
                    carried: true,
                });
                trail.carried = true;
                trail.owed += contrail.point_generation_rate.max(0.0) * TICK;
                let forward = Vec3::from_array(current.forward);
                while trail.owed >= 1.0 {
                    trail.owed -= 1.0;
                    let direction = random.within(forward, contrail.velocity_cone_angle);
                    trail.points.push(TrailPoint {
                        position: Vec3::from_array(current.position),
                        velocity: direction * random.between(contrail.velocity),
                        age: 0.0,
                        shade: random.next(),
                    });
                    if trail.points.len() > MAX_TRAIL_POINTS {
                        trail.points.remove(0);
                    }
                }
            }
        });
        self.trails.retain(|_, trail| {
            let Some(contrail) = level.contrails.get(trail.contrail) else {
                return false;
            };
            for point in &mut trail.points {
                point.position += point.velocity * TICK;
                point.age += TICK;
            }
            trail
                .points
                .retain(|point| trail_state(&contrail.states, point.age, point.shade).is_some());
            trail.carried || !trail.points.is_empty()
        });
    }

    fn gather(
        &self,
        level: &Level,
        pictures: &Pictures,
        seconds: f32,
        out: &mut Vec<ParticleSnapshot>,
    ) {
        let mut trails: Vec<_> = self.trails.iter().collect();
        trails.sort_by_key(|((handle, slot), _)| (handle.index, *slot));
        for (_, trail) in trails {
            let Some(contrail) = level.contrails.get(trail.contrail) else {
                continue;
            };
            let frame = pictures.frame(
                level,
                contrail.bitmap.as_ref(),
                i32::from(contrail.sequences[0]),
                0,
            );
            let count = trail.points.len();
            for (index, pair) in trail.points.windows(2).enumerate() {
                if out.len() >= MAX_PARTICLES {
                    return;
                }
                let [from, to] = [&pair[0], &pair[1]];
                let Some((width, color)) = trail_state(&contrail.states, from.age, from.shade)
                else {
                    continue;
                };
                // (its picture repeats along the trail, and slides)
                let along = contrail.texture_repeats[1].max(1.0) / count.max(1) as f32;
                let slide = contrail.texture_animation[1] * seconds;
                let v0 = index as f32 * along + slide;
                let [u0, _, u1, _] = frame.uv;
                out.push(ParticleSnapshot {
                    position: [
                        from.position.x,
                        from.position.y,
                        from.position.z,
                        width * 0.5,
                    ],
                    axis: [to.position.x, to.position.y, to.position.z, 0.0],
                    color,
                    uv: [u0, v0, u1, v0 + along],
                    shape: [width * 0.5, 0.0, 0.0, 0.0],
                    modes: [
                        frame.picture,
                        blend(contrail.shader.framebuffer_blend_function),
                        PARTICLE_FACING_SEGMENT,
                        0,
                    ],
                });
            }
        }
    }
}

/// The particles, contrails and weather while the game runs.
#[derive(Clone, Debug, Default)]
pub(crate) struct LevelParticles {
    pictures: Pictures,
    /// Each particle's index in the level's.
    particles: HashMap<TagRef, usize>,
    live: Vec<Live>,
    /// What they made in the last tick.
    events: ParticleEvents,
    /// Each running effect's event's emitters: how many they emit, and how
    /// many they have.
    emitted: HashMap<(u32, usize, usize), (u16, u16)>,
    weather: Weather,
    contrails: Contrails,
    /// The particle systems running.
    systems: systems::Systems,
    random: Random,
    /// The camera at the last frame, which the weather is about.
    pub(crate) camera: Vec3,
    /// The seconds run.
    seconds: f32,
}

impl LevelParticles {
    pub(crate) fn new(level: &Level) -> Self {
        Self {
            pictures: Pictures::new(level),
            particles: level
                .particles
                .iter()
                .enumerate()
                .map(|(index, particle)| (particle.tag.clone(), index))
                .collect(),
            contrails: Contrails::new(level),
            systems: systems::Systems::new(level),
            ..Self::default()
        }
    }

    /// One tick, after the effects': the running effects emit, the
    /// particles move, meeting the world of `simulation`, the weather about
    /// the camera on BSP `bsp` drifts, the objects trail their contrails,
    /// and the particle systems the effects' parts `fired` made, and those
    /// the objects carry, run. What the particles made is in
    /// [`LevelParticles::events`].
    pub(crate) fn tick(
        &mut self,
        level: &Level,
        simulation: &mut Simulation,
        (running, fired): (&[Running], &[FiredPart]),
        bsp: usize,
    ) {
        self.seconds += TICK;
        self.emit(level, running);
        let origins: HashMap<u32, Vec3> = running
            .iter()
            .map(|effect| (effect.number, effect.origin.position))
            .collect();
        let wind = self.weather.wind;
        let events = &mut self.events;
        events.clear();
        let world: &Simulation = simulation;
        self.live.retain_mut(|live| {
            let Some(particle) = level.particles.get(live.particle) else {
                return false;
            };
            if let Some((number, at)) = &mut live.anchor {
                if let Some(&now) = origins.get(number) {
                    live.position += now - *at;
                    *at = now;
                }
            }
            live.age += TICK;
            if !live.step(
                particle,
                wind,
                |from, motion| world.collide_point(from, motion),
                events,
            ) {
                return false;
            }
            live.animate(particle);
            if live.age >= live.life {
                events.died(particle, live.position, live.velocity);
                return false;
            }
            true
        });
        if let Some(structure) = level.structure_bsps.get(bsp) {
            self.weather
                .tick(level, structure, self.camera, &mut self.random);
        }
        self.contrails.tick(level, simulation, &mut self.random);
        self.systems
            .tick(level, simulation, fired, &mut self.random);
    }

    /// What the particles made in the last tick.
    pub(crate) fn events(&self) -> &ParticleEvents {
        &self.events
    }

    fn emit(&mut self, level: &Level, running: &[Running]) {
        let mut seen = HashSet::new();
        for effect in running {
            let Some(event) = level
                .effect(&effect.tag)
                .and_then(|definition| definition.events.get(effect.event))
            else {
                continue;
            };
            if effect.skipped || effect.clock < effect.delay {
                continue;
            }
            let fraction = if effect.duration == 0 {
                1.0
            } else {
                (effect.clock + 1 - effect.delay) as f32 / effect.duration as f32
            };
            for (index, emitter) in event.particles.iter().enumerate() {
                let Some(&particle) = self.particles.get(&emitter.particle) else {
                    continue;
                };
                if !made_here(emitter) {
                    continue;
                }
                let key = (effect.number, effect.event, index);
                seen.insert(key);
                let random = &mut self.random;
                let (total, done) = self.emitted.entry(key).or_insert_with(|| {
                    let scaled = Scaled {
                        scales: emitter.scales,
                        by: effect.origin.scale,
                    };
                    let bounds = emitter.count.map(f32::from);
                    let count = scaled.between(random, bounds, EffectParticles::SCALES_COUNT);
                    (count.clamp(0.0, f32::from(u8::MAX)) as u16, 0)
                });
                let target = ((f32::from(*total) * emitted_share(emitter.distribution, fraction))
                    as u16)
                    .min(*total);
                while *done < target {
                    *done += 1;
                    if self.live.len() >= MAX_LIVE {
                        self.live.remove(0);
                    }
                    self.live.push(Live::emit(
                        random,
                        particle,
                        &level.particles[particle],
                        emitter,
                        effect,
                    ));
                }
            }
        }
        self.emitted.retain(|key, _| seen.contains(key));
    }

    /// The frame's particles: the effects', the particle systems', the
    /// contrails', then the weather's about `camera`.
    pub(crate) fn gather(&self, level: &Level, camera: Vec3, out: &mut Vec<ParticleSnapshot>) {
        out.clear();
        for live in &self.live {
            if out.len() >= MAX_PARTICLES {
                return;
            }
            out.extend(live.snapshot(level, &self.pictures));
        }
        self.systems.gather(level, &self.pictures, out);
        self.contrails
            .gather(level, &self.pictures, self.seconds, out);
        self.weather.gather(level, &self.pictures, camera, out);
    }
}

/// The detail objects BSP `structure` scatters, as sprites standing on its
/// ground, each faded out between its type's distances.
pub(crate) fn detail_objects(level: &Level, structure: &StructureBsp) -> Vec<ParticleSnapshot> {
    let pictures = Pictures::new(level);
    let mut out = Vec::with_capacity(structure.detail_objects.len());
    for object in &structure.detail_objects {
        let Some(Some(collection)) = level
            .detail_object_collections
            .get(usize::from(object.collection))
        else {
            continue;
        };
        if collection.types.is_empty() {
            continue;
        }
        let kind = &collection.types[usize::from(object.data >> 4) % collection.types.len()];
        let sprite = usize::from(kind.sprites[0])
            + usize::from(object.data & 0xF) % usize::from(kind.sprites[1].max(1));
        let frame = pictures.frame(
            level,
            collection.bitmap.as_ref(),
            i32::from(kind.sequence),
            sprite,
        );
        let scale = (kind.size[0] + kind.size[1]) * 0.5;
        let [width, height] = frame.texels.map(|texels| texels * scale * 0.5);
        let own = object.color.map(|value| f32::from(value) / 255.0);
        let tint = own.map(|value| 1.0 + (value - 1.0) * kind.color_override.clamp(0.0, 1.0));
        let [x, y, z] = object.position;
        let facing = if collection.kind == 0 {
            PARTICLE_FACING_AXIS
        } else {
            PARTICLE_FACING_SCREEN
        };
        out.push(ParticleSnapshot {
            position: [x, y, z + height, width],
            axis: [0.0, 0.0, 1.0, 0.0],
            color: [tint[0], tint[1], tint[2], 1.0],
            uv: frame.uv,
            shape: [height, kind.fade[0], kind.fade[1], 0.0],
            modes: [frame.picture, 0, facing, 0],
        });
    }
    out
}

#[cfg(test)]
mod tests {
    use super::*;
    use opence_content::{
        Contrail, ContrailState, DetailObject, DetailObjectCollection, DetailObjectType, Effect,
        EffectEvent, Group, Sequence, Sprite, SpriteShader, WeatherPalette, WeatherSystem,
    };
    use opence_sim::effects::Origin;
    use opence_sim::Quat;

    fn tag(group: &[u8; 4], path: &str) -> TagRef {
        TagRef {
            group: Group(*group),
            path: path.into(),
        }
    }

    fn emitter(distribution: i16, count: i16) -> EffectParticles {
        EffectParticles {
            particle: tag(b"part", "spark"),
            location: 0,
            flags: 0,
            distribution,
            count: [count, count],
            distribution_radius: [0.0, 0.0],
            direction: [0.0, 0.0],
            offset: [0.0, 0.0, 0.0],
            velocity: [2.0, 2.0],
            velocity_cone_angle: 0.0,
            angular_velocity: [0.0, 0.0],
            radius: [0.5, 0.5],
            tint: [[0.0; 4], [0.0; 4]],
            environment: 0,
            disposition: 0,
            camera_mode: 0,
            scales: [0, 0],
        }
    }

    /// A level whose effect `sparks` emits `count` sparks by
    /// `distribution`, drawn from a sheet of two images.
    fn level(distribution: i16, count: i16) -> Level {
        let mut level = opence_content::devmap::level(None);
        let mut sheet = crate::render_level::tests::bitmap("sheet", 2, 1);
        sheet.sequences = vec![Sequence {
            name: String::new(),
            first_image: Some(0),
            image_count: 2,
            sprites: Vec::new(),
        }];
        level.bitmaps.push(sheet);
        level.particles.push(Particle {
            bitmap: Some(tag(b"bitm", "sheet")),
            fade_out: 0.5,
            radius: [1.0, 2.0],
            ..Particle::new(tag(b"part", "spark"))
        });
        level.effects.push(Effect {
            events: vec![EffectEvent {
                skip_fraction: 0.0,
                delay: [0.0, 0.0],
                duration: [1.0, 1.0],
                parts: Vec::new(),
                particles: vec![emitter(distribution, count)],
            }],
            locations: vec![String::new()],
            ..Effect::new(tag(b"effe", "sparks"))
        });
        level
    }

    fn running(clock: u32, duration: u32) -> Running {
        Running {
            number: 1,
            tag: tag(b"effe", "sparks"),
            origin: Origin::at(Vec3::new(1.0, 2.0, 3.0), Quat::IDENTITY),
            event: 0,
            clock,
            delay: 0,
            duration,
            skipped: false,
            fired: true,
            age: clock,
        }
    }

    /// A heavy, unbouncing pebble that meets the world.
    fn pebble(flags: u32) -> Particle {
        Particle {
            flags,
            radius: [1.0, 1.0],
            physics: Some(PointPhysics {
                flags: PointPhysics::STRUCTURE_COLLISIONS,
                mass_over_radius_cubed: 1.0,
                air_buoyancy: -1.0,
                density: 1.0,
                contact_friction: 1.0,
                ..PointPhysics::default()
            }),
            death_effect: Some(tag(b"effe", "dust")),
            contact_deterioration: 0.5,
            ..Particle::new(tag(b"part", "pebble"))
        }
    }

    /// A particle at `height` going `velocity`, with frames of 0.1s.
    fn live(height: f32, velocity: Vec3) -> Live {
        Live {
            particle: 0,
            position: Vec3::new(0.0, 0.0, height),
            velocity,
            age: 0.0,
            life: 10.0,
            radius: 0.01,
            rotation: 0.0,
            spin: 1.0,
            color: [1.0; 4],
            sequence: 0,
            frame_span: Some(0.1),
            frame: 0.0,
            at_rest: false,
            anchor: None,
        }
    }

    /// The ground at z = 0, facing up.
    fn floor(from: Vec3, motion: Vec3) -> Option<PointHit> {
        let to = from + motion;
        (from.z >= 0.0 && to.z < 0.0).then(|| {
            let fraction = from.z / (from.z - to.z);
            PointHit {
                point: from + motion * fraction,
                normal: Vec3::Z,
                fraction,
            }
        })
    }

    #[test]
    fn a_particle_meets_the_ground_settles_and_rests() {
        let stops = pebble(Particle::ANIMATION_STOPS_AT_REST);
        let mut one = live(0.01, Vec3::new(0.0, 0.0, -1.0));
        let mut events = ParticleEvents::default();
        assert!(one.step(&stops, Vec3::ZERO, floor, &mut events));
        // (its frames wear longer for meeting the ground; it slid to a stop)
        assert_eq!(one.frame_span, Some(0.6));
        assert!(one.at_rest, "{:?}", one.velocity);
        assert!(one.position.z >= 0.0, "{}", one.position);
        assert!(events.made.is_empty() && events.impacts.is_empty());
        // at rest it moves, turns and animates no more
        let still = one.clone();
        one.animate(&stops);
        assert!(one.step(&stops, Vec3::ONE, floor, &mut events));
        assert_eq!(one, still);
        // unless its animation goes on at rest
        one.animate(&pebble(0));
        assert!(one.frame > 0.0);
        // in the air, far from the ground, it falls and turns
        let mut falling = live(10.0, Vec3::ZERO);
        assert!(falling.step(&stops, Vec3::ZERO, floor, &mut events));
        assert!(falling.velocity.z < 0.0 && !falling.at_rest);
        assert_eq!(falling.rotation, TICK);
        assert_eq!(falling.frame_span, Some(0.1));
    }

    #[test]
    fn a_particle_dies_at_rest_or_on_contact_by_its_flags() {
        let down = Vec3::new(0.0, 0.0, -1.0);
        let mut events = ParticleEvents::default();
        let mut one = live(0.01, down);
        assert!(!one.step(
            &pebble(Particle::DIES_AT_REST),
            Vec3::ZERO,
            floor,
            &mut events
        ));
        let made: Vec<&str> = events
            .made
            .iter()
            .map(|one| one.tag.path.as_str())
            .collect();
        assert_eq!(made, ["dust"]);
        assert_eq!(events.made.first().map(|one| one.scale), Some(0.0));

        // meeting the world, with no collision effect it dies
        events.clear();
        let mut one = live(0.01, down * 2.0);
        let dies = pebble(Particle::DIES_ON_CONTACT_WITH_STRUCTURE);
        assert!(!one.step(&dies, Vec3::ZERO, floor, &mut events));
        assert_eq!(events.made.len(), 1);
        // and with one it plays that, scaled by its speed, and is only gone
        events.clear();
        let mut one = live(0.01, down * 2.0);
        let chipped = Particle {
            collision_effect: Some(tag(b"snd!", "tink")),
            collision_material_effects: Some(tag(b"foot", "pebble")),
            ..dies
        };
        assert!(!one.step(&chipped, Vec3::ZERO, floor, &mut events));
        let made: Vec<(&str, f32)> = events
            .made
            .iter()
            .map(|one| (one.tag.path.as_str(), one.scale))
            .collect();
        assert_eq!(made, [("tink", 0.0)]);
        assert_eq!(events.impacts.len(), 1);
        assert_eq!(events.impacts.first().map(|one| one.normal), Some(Vec3::Z));

        // one that dies in air dies on its first tick
        events.clear();
        let mut one = live(10.0, Vec3::ZERO);
        assert!(!one.step(
            &pebble(Particle::DIES_ON_CONTACT_WITH_AIR),
            Vec3::ZERO,
            floor,
            &mut events
        ));
        assert_eq!(events.made.len(), 1);
    }

    #[test]
    fn an_attached_particle_is_slowed_by_its_air_and_one_without_physics_coasts() {
        let mut events = ParticleEvents::default();
        let draggy = Particle {
            physics: Some(PointPhysics {
                mass_over_radius_cubed: 1.0,
                air_friction: 1.0,
                ..PointPhysics::default()
            }),
            ..pebble(Particle::DIES_AT_REST)
        };
        let mut one = Live {
            anchor: Some((1, Vec3::ZERO)),
            radius: 1.0,
            ..live(5.0, Vec3::X)
        };
        assert!(one.step(&draggy, Vec3::ZERO, floor, &mut events));
        assert!(
            one.velocity.x < 1.0 && one.velocity.x > 0.0,
            "{}",
            one.velocity
        );
        assert_eq!(one.velocity.z, 0.0);
        // (settled, but too fast to rest)
        assert!(!one.at_rest && events.made.is_empty());
        let mut coasting = live(5.0, Vec3::X);
        let ghost = Particle {
            physics: None,
            ..pebble(0)
        };
        assert!(coasting.step(&ghost, Vec3::ZERO, floor, &mut events));
        assert_eq!(coasting.position, Vec3::new(TICK, 0.0, 5.0));
    }

    #[test]
    fn the_shares_emitted_follow_their_distributions() {
        let share = |distribution, fraction| emitted_share(distribution, fraction);
        assert_eq!(share(EffectParticles::START, 0.0), 1.0);
        assert_eq!(share(EffectParticles::END, 0.5), 0.0);
        assert_eq!(share(EffectParticles::END, 1.0), 1.0);
        assert_eq!(share(EffectParticles::CONSTANT, 0.25), 0.25);
        assert_eq!(share(EffectParticles::BUILDUP, 0.5), 0.25);
        assert_eq!(share(EffectParticles::FALLOFF, 0.5), 0.75);
        assert_eq!(share(EffectParticles::BUILDUP_AND_FALLOFF, 0.5), 0.5);
        for distribution in 0..6 {
            assert_eq!(share(distribution, 2.0), 1.0, "{distribution}");
        }
    }

    #[test]
    fn an_effect_emits_its_particles_which_move_grow_fade_and_die() {
        let level = level(EffectParticles::START, 3);
        let mut particles = LevelParticles::new(&level);
        let mut simulation = Simulation::empty();
        let effect = [running(0, 30)];
        particles.tick(&level, &mut simulation, (&effect, &[]), 0);
        assert_eq!(particles.live.len(), 3);
        // (emitted once, though the effect runs on)
        particles.tick(&level, &mut simulation, (&[running(1, 30)], &[]), 0);
        assert_eq!(particles.live.len(), 3);
        let mut out = Vec::new();
        particles.gather(&level, Vec3::ZERO, &mut out);
        assert_eq!(out.len(), 3);
        let first = out[0];
        // (it left forward at 2 a second, for two ticks)
        assert!(
            (first.position[0] - (1.0 + 4.0 / 30.0)).abs() < 1e-4,
            "{first:?}"
        );
        assert!(first.position[3] > 0.5, "it grows: {first:?}");
        assert_eq!(first.color, [1.0, 1.0, 1.0, 1.0]);
        assert_eq!(first.modes[2], PARTICLE_FACING_SCREEN);
        assert!(first.modes[0] < 2, "{first:?}");
        for _ in 0..27 {
            particles.tick(&level, &mut simulation, (&[], &[]), 0);
        }
        particles.gather(&level, Vec3::ZERO, &mut out);
        assert!(out[0].color[3] < 0.5, "it fades out: {:?}", out[0]);
        particles.tick(&level, &mut simulation, (&[], &[]), 0);
        assert!(particles.live.is_empty(), "it dies at a second");
        assert!(particles.emitted.is_empty());
    }

    #[test]
    fn a_constant_effect_emits_over_its_duration_and_a_skipped_one_not_at_all() {
        let level = level(EffectParticles::CONSTANT, 10);
        let mut particles = LevelParticles::new(&level);
        let mut simulation = Simulation::empty();
        particles.tick(&level, &mut simulation, (&[running(4, 10)], &[]), 0);
        assert_eq!(particles.live.len(), 5);
        particles.tick(&level, &mut simulation, (&[running(9, 10)], &[]), 0);
        assert_eq!(particles.live.len(), 10);
        let mut skipped = running(0, 10);
        skipped.number = 2;
        skipped.skipped = true;
        particles.tick(&level, &mut simulation, (&[skipped], &[]), 0);
        assert_eq!(particles.live.len(), 10);
    }

    #[test]
    fn attached_particles_go_with_their_effect() {
        let mut level = level(EffectParticles::START, 1);
        let event = &mut level.effects.last_mut().unwrap_or_else(|| panic!()).events[0];
        event.particles[0].flags = EffectParticles::STAY_ATTACHED;
        event.particles[0].velocity = [0.0, 0.0];
        let mut particles = LevelParticles::new(&level);
        let mut simulation = Simulation::empty();
        particles.tick(&level, &mut simulation, (&[running(0, 30)], &[]), 0);
        let mut moved = running(1, 30);
        moved.origin.position += Vec3::new(0.0, 0.0, 5.0);
        particles.tick(&level, &mut simulation, (&[moved], &[]), 0);
        assert_eq!(particles.live[0].position, Vec3::new(1.0, 2.0, 8.0));
    }

    #[test]
    fn physics_pulls_a_point_down_and_drags_it_toward_the_wind() {
        let physics = PointPhysics {
            air_buoyancy: -1.0,
            air_friction: 1.0,
            ..Default::default()
        };
        let still = pushed(&physics, 0.1, Vec3::ZERO, Vec3::ZERO, TICK);
        assert!(still.z < 0.0, "{still}");
        let floating = PointPhysics {
            flags: PointPhysics::NO_GRAVITY,
            ..physics
        };
        let blown = pushed(&floating, 0.1, Vec3::ZERO, Vec3::X, TICK);
        assert_eq!(blown.z, 0.0);
        assert!(blown.x > 0.0 && blown.x <= 1.0, "{blown}");
    }

    #[test]
    fn a_frame_is_a_sequences_sprite_or_image_and_none_without_a_bitmap() {
        let mut level = level(EffectParticles::START, 1);
        let mut sprites = crate::render_level::tests::bitmap("sprites", 1, 1);
        sprites.sequences = vec![Sequence {
            name: String::new(),
            first_image: None,
            image_count: 0,
            sprites: vec![Sprite {
                image: 0,
                bounds: [0.0, 0.5, 0.25, 1.0],
                registration_point: [0.0, 0.0],
            }],
        }];
        level.bitmaps.push(sprites);
        level.particles.push(Particle {
            bitmap: Some(tag(b"bitm", "sprites")),
            ..Particle::new(tag(b"part", "dust"))
        });
        let pictures = Pictures::new(&level);
        let sheet = pictures.frame(&level, Some(&tag(b"bitm", "sheet")), 0, 3);
        assert_eq!(sheet.picture, 1);
        assert_eq!(sheet.uv, [0.0, 0.0, 1.0, 1.0]);
        assert_eq!(sheet.texels, [4.0, 4.0]);
        let sprite = pictures.frame(&level, Some(&tag(b"bitm", "sprites")), 0, 0);
        assert_eq!(sprite.picture, 2);
        assert_eq!(sprite.uv, [0.0, 0.25, 0.5, 1.0]);
        assert_eq!(sprite.texels, [2.0, 3.0]);
        let none = pictures.frame(&level, None, 0, 0);
        assert_eq!(none.picture, PARTICLE_NO_PICTURE);
        let images = particle_images(&level);
        assert_eq!(images.len(), 3);
        assert_eq!(images[2], (tag(b"bitm", "sprites"), 0));
    }

    /// The devmap with snow falling in every cluster, in a wind along y.
    fn snowing() -> (Level, Vec3) {
        let mut level = level(EffectParticles::START, 1);
        level.weather.push(WeatherSystem {
            tag: tag(b"rain", "snow"),
            types: vec![WeatherParticleType {
                flags: 0,
                distance_fade: [0.0, 1.0, 3.0, 4.0],
                density: [1.0, 1.0],
                physics: None,
                acceleration: [0.0, 0.0, 0.0, 0.0],
                radius: [0.1, 0.1],
                animation_rate: [0.0, 0.0],
                rotation_rate: [0.0, 0.0],
                color: [[1.0, 1.0, 1.0, 1.0]; 2],
                bitmap: Some(tag(b"bitm", "sheet")),
                render_mode: 1,
                shader: SpriteShader::default(),
            }],
        });
        let bsp = &mut level.structure_bsps[0];
        bsp.weather_palette = vec![WeatherPalette {
            system: Some(tag(b"rain", "snow")),
            wind: [0.0, 2.0, 0.0, 3.0],
        }];
        bsp.cluster_weather = vec![Some(0)];
        bsp.leaf_clusters = vec![Some(0); bsp.collision.leaves.len()];
        let camera = [1.0, -1.0, 10.0, -10.0]
            .into_iter()
            .map(|z| Vec3::new(0.5, 0.5, z))
            .find(|point| bsp.collision.leaf_at(point.to_array()).is_some());
        let Some(camera) = camera else {
            panic!("the devmap has no leaf");
        };
        (level, camera)
    }

    #[test]
    fn weather_fills_a_box_about_the_camera_where_its_cluster_has_some() {
        let (level, camera) = snowing();
        let (system, wind) = weather_at(&level, &level.structure_bsps[0], camera);
        assert_eq!(system, Some(0));
        assert_eq!(wind, Vec3::new(0.0, 3.0, 0.0));
        let mut particles = LevelParticles::new(&level);
        particles.camera = camera;
        let mut simulation = Simulation::empty();
        particles.tick(&level, &mut simulation, (&[], &[]), 0);
        assert_eq!(particles.weather.drops[0].len(), 64);
        let mut out = Vec::new();
        particles.gather(&level, camera, &mut out);
        assert_eq!(out.len(), 64);
        for drop in &out {
            let at = Vec3::new(drop.position[0], drop.position[1], drop.position[2]);
            assert!((at - camera).abs().max_element() <= 2.0 + 1e-4, "{at}");
            assert!((0.0..=1.0).contains(&drop.color[3]));
            assert_eq!(drop.modes[2], PARTICLE_FACING_AXIS);
        }
        // (somewhere with none, it stops)
        let mut dry = level.clone();
        dry.structure_bsps[0].cluster_weather = vec![None];
        particles.tick(&dry, &mut simulation, (&[], &[]), 0);
        particles.gather(&dry, camera, &mut out);
        assert!(out.is_empty());
    }

    #[test]
    fn wrapping_brings_an_offset_into_the_box() {
        let wrapped = wrapped(Vec3::new(2.5, -2.5, 0.25), 4.0);
        assert!(
            (wrapped - Vec3::new(-1.5, 1.5, 0.25)).length() < 1e-5,
            "{wrapped}"
        );
    }

    #[test]
    fn a_contrails_points_age_through_its_states_and_fade() {
        let white = [[1.0, 1.0, 1.0, 1.0]; 2];
        let states = [
            ContrailState {
                duration: [1.0, 1.0],
                transition: [1.0, 1.0],
                width: 1.0,
                color: white,
            },
            ContrailState {
                duration: [1.0, 1.0],
                transition: [1.0, 1.0],
                width: 3.0,
                color: white,
            },
        ];
        assert_eq!(trail_state(&states, 0.5, 0.0), Some((1.0, [1.0; 4])));
        assert_eq!(trail_state(&states, 1.5, 0.0), Some((2.0, [1.0; 4])));
        let Some((width, color)) = trail_state(&states, 3.5, 0.0) else {
            panic!("it is fading");
        };
        assert_eq!(width, 3.0);
        assert!((color[3] - 0.5).abs() < 1e-5);
        assert_eq!(trail_state(&states, 4.5, 0.0), None);
    }

    #[test]
    fn an_object_trails_its_contrail_as_strips_between_its_points() {
        use opence_content::object::Attachment;
        use opence_content::ObjectKind;
        use opence_sim::objects::ObjectSpawn;
        use opence_sim::Transform;
        let mut level = level(EffectParticles::START, 1);
        level.contrails.push(Contrail {
            tag: tag(b"cont", "trail"),
            flags: 0,
            point_generation_rate: 30.0,
            velocity: [0.0, 0.0],
            velocity_cone_angle: 0.0,
            inherited_velocity_fraction: 0.0,
            render_type: 0,
            texture_repeats: [1.0, 1.0],
            texture_animation: [0.0, 0.0],
            bitmap: Some(tag(b"bitm", "sheet")),
            sequences: [0, 1],
            shader: SpriteShader::default(),
            states: vec![ContrailState {
                duration: [1.0, 1.0],
                transition: [0.0, 0.0],
                width: 0.5,
                color: [[1.0, 1.0, 0.0, 0.0]; 2],
            }],
        });
        level.objects.push(opence_content::ObjectDefinition {
            widgets: Vec::new(),
            tag: tag(b"proj", "rocket"),
            kind: ObjectKind::Scenery,
            flags: 0,
            bounding_radius: 1.0,
            bounding_offset: [0.0, 0.0, 0.0],
            acceleration_scale: 1.0,
            render_bounding_radius: 1.0,
            model: None,
            animation_graph: None,
            collision_model: None,
            physics: None,
            modifier_shader: None,
            creation_effect: None,
            function_modes: Default::default(),
            functions: Vec::new(),
            change_colors: Vec::new(),
            unit_hud: None,
            material_effects: None,
            unit_melee_damage: None,
            biped: None,
            weapon: None,
            projectile: None,
            equipment: None,
            vehicle: None,
            device: None,
            attachments: vec![Attachment {
                tag: Some(tag(b"cont", "trail")),
                marker: String::new(),
                primary_scale: 0,
                change_color: 0,
            }],
        });
        let mut particles = LevelParticles::new(&level);
        assert_eq!(particles.contrails.carried.len(), 1);
        let mut simulation = Simulation::empty();
        let spawned = simulation.spawn(ObjectSpawn {
            kind: ObjectKind::Scenery,
            tag: tag(b"proj", "rocket"),
            transform: Transform::default(),
            name: None,
            permutation: None,
            team: None,
        });
        let Ok(handle) = spawned else {
            panic!("the rocket was not made");
        };
        for _ in 0..3 {
            particles.tick(&level, &mut simulation, (&[], &[]), 0);
        }
        let mut out = Vec::new();
        particles.gather(&level, Vec3::ZERO, &mut out);
        assert_eq!(out.len(), 2, "three points, two strips: {out:?}");
        assert_eq!(out[0].modes[2], PARTICLE_FACING_SEGMENT);
        assert_eq!(out[0].color, [1.0, 0.0, 0.0, 1.0]);
        assert_eq!(out[0].position[3], 0.25);
        assert!(simulation.remove(handle));
        for _ in 0..31 {
            particles.tick(&level, &mut simulation, (&[], &[]), 0);
        }
        assert!(particles.contrails.trails.is_empty(), "its points died");
    }

    #[test]
    fn detail_objects_stand_on_the_ground_sized_by_their_sprites() {
        let mut level = level(EffectParticles::START, 1);
        level.detail_object_collections = vec![
            None,
            Some(DetailObjectCollection {
                tag: tag(b"dobc", "grass"),
                kind: 0,
                bitmap: Some(tag(b"bitm", "sheet")),
                types: vec![DetailObjectType {
                    sequence: 0,
                    sprites: [0, 2],
                    color_override: 1.0,
                    fade: [5.0, 10.0],
                    size: [0.5, 0.5],
                }],
            }),
        ];
        let mut bsp = level.structure_bsps[0].clone();
        bsp.detail_objects = vec![
            DetailObject {
                position: [1.0, 2.0, 3.0],
                color: [255, 0, 255],
                collection: 1,
                data: 0x11,
            },
            DetailObject {
                collection: 0,
                ..Default::default()
            },
        ];
        let objects = detail_objects(&level, &bsp);
        assert_eq!(objects.len(), 1);
        let grass = objects[0];
        // (4 texels at half a unit each: a unit high, half a unit up)
        assert_eq!(grass.position, [1.0, 2.0, 4.0, 1.0]);
        assert_eq!(grass.shape, [1.0, 5.0, 10.0, 0.0]);
        assert_eq!(grass.color, [1.0, 0.0, 1.0, 1.0]);
        assert_eq!(grass.modes, [1, 0, PARTICLE_FACING_AXIS, 0]);
    }

    #[test]
    fn a_random_direction_stays_within_its_cone() {
        let mut random = Random::default();
        for _ in 0..100 {
            let direction = random.within(Vec3::Z, 0.5);
            assert!((direction.length() - 1.0).abs() < 1e-4);
            assert!(direction.z >= libm::cosf(0.25) - 1e-4, "{direction}");
            assert!(random.below(3) < 3);
        }
        assert_eq!(random.within(Vec3::Y, 0.0), Vec3::Y);
    }

    /// `level`'s effect `sparks` emits `emitter` alone.
    fn emits(level: &mut Level, emitter: EffectParticles) {
        let Some(event) = level
            .effects
            .iter_mut()
            .find(|effect| effect.tag.path == "sparks")
            .and_then(|effect| effect.events.first_mut())
        else {
            panic!("the level has no sparks");
        };
        event.particles = vec![emitter];
    }

    /// The particles an effect at rest emitting `emitter` once makes, at
    /// scales `scale` and moving at `velocity`.
    fn emitted(emitter: EffectParticles, scale: [f32; 2], velocity: Vec3) -> Vec<Live> {
        let mut level = level(EffectParticles::START, 1);
        emits(&mut level, emitter);
        let mut particles = LevelParticles::new(&level);
        let mut effect = running(0, 30);
        effect.origin.scale = scale;
        effect.origin.velocity = velocity;
        particles.tick(&level, &mut Simulation::empty(), (&[effect], &[]), 0);
        particles.live
    }

    #[test]
    fn an_effects_scales_scale_the_counts_its_emitters_say() {
        let mut many = emitter(EffectParticles::START, 0);
        many.count = [8, 8];
        assert_eq!(emitted(many.clone(), [0.5, 0.25], Vec3::ZERO).len(), 8);
        many.scales = [EffectParticles::SCALES_COUNT, 0];
        assert_eq!(emitted(many.clone(), [0.5, 0.25], Vec3::ZERO).len(), 4);
        many.scales = [EffectParticles::SCALES_COUNT, EffectParticles::SCALES_COUNT];
        assert_eq!(emitted(many.clone(), [0.5, 0.25], Vec3::ZERO).len(), 1);
        // (a count is the fewest and a random share of the rest, short of
        // the most)
        many.count = [2, 6];
        many.scales = [0, 0];
        for _ in 0..8 {
            let made = emitted(many.clone(), [1.0; 2], Vec3::ZERO).len();
            assert!((2..6).contains(&made), "{made}");
        }
    }

    #[test]
    fn particles_take_the_effects_velocity_unless_they_stay_with_it() {
        let moving = Vec3::new(0.0, 0.0, 3.0);
        let free = emitted(emitter(EffectParticles::START, 1), [1.0; 2], moving);
        assert_eq!(free[0].velocity, Vec3::new(2.0, 0.0, 3.0));
        let mut stays = emitter(EffectParticles::START, 1);
        stays.flags = EffectParticles::STAY_ATTACHED;
        let held = emitted(stays, [1.0; 2], moving);
        assert_eq!(held[0].velocity, Vec3::new(2.0, 0.0, 0.0));
        // and a scaled speed is the scaled least and the rest scaled by
        // its own bit
        let mut quick = emitter(EffectParticles::START, 1);
        quick.velocity = [2.0, 6.0];
        quick.scales = [
            EffectParticles::SCALES_VELOCITY | EffectParticles::SCALES_VELOCITY_DELTA,
            0,
        ];
        let slow = emitted(quick, [0.0, 1.0], Vec3::ZERO);
        assert_eq!(slow[0].velocity, Vec3::ZERO);
    }

    #[test]
    fn particles_leave_within_their_cone_and_are_sized_and_tinted_by_the_scales() {
        let mut wide = emitter(EffectParticles::START, 16);
        wide.velocity_cone_angle = 0.4;
        wide.radius = [1.0, 3.0];
        wide.tint = [[1.0, 0.0, 0.0, 0.0], [1.0, 1.0, 1.0, 1.0]];
        wide.scales = [
            EffectParticles::SCALES_RADIUS | EffectParticles::SCALES_TINT,
            0,
        ];
        let made = emitted(wide.clone(), [0.25, 1.0], Vec3::ZERO);
        assert_eq!(made.len(), 16);
        for one in &made {
            let way = one.velocity.normalize_or(Vec3::ZERO);
            assert!(way.x >= libm::cosf(0.4) - 1e-4, "{way}");
            // the least radius scaled, the rest not
            assert!((0.25..2.25).contains(&one.radius), "{}", one.radius);
            // a quarter of the way from black to white
            assert!((one.color[0] - 0.25).abs() < 1e-4, "{:?}", one.color);
        }
        wide.velocity_cone_angle = 0.0;
        let straight = emitted(wide, [1.0; 2], Vec3::ZERO);
        assert!(straight
            .iter()
            .all(|one| one.velocity == Vec3::new(2.0, 0.0, 0.0)));
    }

    #[test]
    fn particles_are_tinted_by_the_effects_colour_when_they_say_so() {
        let mut tinted = emitter(EffectParticles::START, 1);
        tinted.flags = EffectParticles::TINT_FROM_CHANGE_COLOR;
        let mut level = level(EffectParticles::START, 1);
        emits(&mut level, tinted);
        let mut particles = LevelParticles::new(&level);
        let mut effect = running(0, 30);
        effect.origin.color = [0.5, 0.25, 1.0];
        particles.tick(&level, &mut Simulation::empty(), (&[effect], &[]), 0);
        assert_eq!(particles.live[0].color, [0.5, 0.25, 1.0, 1.0]);
    }

    #[test]
    fn particles_for_water_or_a_vacuum_or_nonviolence_are_not_made() {
        for (environment, disposition) in [
            (EffectParticles::IN_WATER, 0),
            (EffectParticles::IN_VACUUM, 0),
            (0, EffectParticles::NONVIOLENT),
        ] {
            let mut kept = emitter(EffectParticles::START, 3);
            kept.environment = environment;
            kept.disposition = disposition;
            assert!(emitted(kept, [1.0; 2], Vec3::ZERO).is_empty());
        }
        let mut airy = emitter(EffectParticles::START, 3);
        airy.environment = EffectParticles::IN_AIR;
        airy.disposition = EffectParticles::VIOLENT;
        assert_eq!(emitted(airy, [1.0; 2], Vec3::ZERO).len(), 3);
    }

    #[test]
    fn a_tilted_way_turns_no_further_than_its_angle() {
        let mut random = Random::default();
        for _ in 0..100 {
            let way = random.tilted(Vec3::Z, 0.3);
            assert!((way.length() - 1.0).abs() < 1e-4);
            assert!(way.z >= libm::cosf(0.3) - 1e-4, "{way}");
        }
        assert_eq!(random.tilted(Vec3::X, 0.0), Vec3::X);
    }
}
