//! The contrails (`cont`) objects trail, as the original's `contrails.c`
//! and `render_contrails.c` make and draw them: an object's attachment
//! leaves points at its marker at the contrail's rate, scaled by the
//! attachment's function; each point passes through the contrail's point
//! states, a random time in each and a random time becoming the next,
//! moved by the state's point physics; the trail is drawn as a strip
//! through its points, its picture scrolling and stepping through its
//! sequence's frames.

use std::collections::{BTreeMap, VecDeque};

use opence_content::{Contrail, ContrailState, Level, TagRef};
use opence_sim::damage::transition;
use opence_sim::functions::FunctionValues;
use opence_sim::objects::ObjectHandle;
use opence_sim::particle_collision::PointHit;
use opence_sim::point_physics;
use opence_sim::seats::default_pose;
use opence_sim::{Simulation, Vec3};
use rustc_hash::FxHashMap;

use super::slots::Slots;
use super::wind::Air;
use super::{blend, Pictures, Random, TICK};
use crate::lights::marker_frames;
use crate::snapshot::{ParticleSnapshot, MAX_PARTICLES, PARTICLE_FACING_QUAD};

/// The most points one contrail keeps.
const MAX_TRAIL_POINTS: usize = 64;

/// `value`, scaled by `by` if bit `bit` of `flags` is set, as the
/// original's `contrail_scale_value`.
fn scaled(value: f32, by: f32, flags: u32, bit: u32) -> f32 {
    if flags & (1 << bit) != 0 {
        value * by
    } else {
        value
    }
}

/// A value between `bounds`, as the original's
/// `contrail_scale_random_value`: the least scaled by `bit`, and a random
/// part of what is above it scaled by the next bit.
fn scaled_between(
    random: &mut Random,
    [least, most]: [f32; 2],
    by: f32,
    flags: u32,
    bit: u32,
) -> f32 {
    random.next() * scaled(most - least, by, flags, bit + 1) + scaled(least, by, flags, bit)
}

/// The value of an object's outgoing function `index` (1 to 4; any other
/// is none) and whether it is on, as the original's
/// `object_get_function_value`: none is 1 and on.
fn function_value(functions: Option<&FunctionValues>, index: i16) -> (f32, bool) {
    match usize::try_from(index - 1) {
        Ok(slot) if slot < 4 => functions.map_or((0.0, false), |values| {
            (values.outgoing[slot], values.active[slot])
        }),
        _ => (1.0, true),
    }
}

/// An alpha, red, green, blue colour as red, green, blue and alpha.
fn argb([a, r, g, b]: [f32; 4]) -> [f32; 4] {
    [r, g, b, a]
}

fn mix(from: [f32; 4], to: [f32; 4], t: f32) -> [f32; 4] {
    [0, 1, 2, 3].map(|i| from[i] + (to[i] - from[i]) * t)
}

/// A colour as four bytes, red lowest, as Metal's `unpack_unorm4x8`
/// reads it.
fn packed(color: [f32; 4]) -> u32 {
    color
        .iter()
        .enumerate()
        .map(|(i, value)| ((value.clamp(0.0, 1.0) * 255.0 + 0.5) as u32) << (8 * i))
        .fold(0, |packed, byte| packed | byte)
}

/// A point of a contrail.
#[derive(Clone, Debug, PartialEq)]
struct TrailPoint {
    position: Vec3,
    velocity: Vec3,
    /// The state it is in (-1 before its first).
    state: i32,
    /// How far it is through its state or its becoming the next, 0 to 1.
    time: f32,
    /// How much of it a second passes (0: none, which it leaves at once).
    delta: f32,
    /// It is becoming its next state.
    transitioning: bool,
    /// It has outlived its states.
    expired: bool,
    /// It was made this tick, and has not moved.
    new: bool,
    /// The contrail's scale when it was made.
    density: f32,
}

impl TrailPoint {
    fn new(position: Vec3, velocity: Vec3, density: f32) -> Self {
        Self {
            position,
            velocity,
            state: -1,
            time: 0.0,
            delta: 0.0,
            transitioning: true,
            expired: false,
            new: true,
            density,
        }
    }

    /// Passes `dt` of its life through `states`, as the original's
    /// `contrail_update_points` does.
    fn age(&mut self, states: &[ContrailState], dt: f32, random: &mut Random) {
        if self.expired {
            return;
        }
        self.time += dt * self.delta;
        while self.delta == 0.0 || self.time > 1.0 {
            if self.transitioning {
                self.state += 1;
                self.time = 0.0;
                let Some(state) = usize::try_from(self.state).ok().and_then(|i| states.get(i))
                else {
                    self.expired = true;
                    break;
                };
                let seconds = scaled_between(
                    random,
                    state.duration,
                    self.density,
                    state.scale_flags,
                    ContrailState::SCALE_DURATION,
                );
                self.delta = if seconds > 0.0 { 1.0 / seconds } else { 0.0 };
                self.transitioning = false;
            } else if usize::try_from(self.state + 1).is_ok_and(|next| next < states.len()) {
                let Some(state) = usize::try_from(self.state).ok().and_then(|i| states.get(i))
                else {
                    self.expired = true;
                    break;
                };
                self.time = 0.0;
                let seconds = scaled_between(
                    random,
                    state.transition,
                    self.density,
                    state.scale_flags,
                    ContrailState::SCALE_TRANSITION,
                );
                self.delta = if seconds > 0.0 { 1.0 / seconds } else { 0.0 };
                self.transitioning = true;
            } else {
                self.expired = true;
                break;
            }
        }
    }

    /// Its width and colour: its state's, between the state's colours by
    /// its density, becoming the next state's as it transitions.
    fn look(&self, states: &[ContrailState]) -> Option<(f32, [f32; 4])> {
        let last = states.len().checked_sub(1)?;
        let index = usize::try_from(self.state).unwrap_or(0).min(last);
        let of = |state: &ContrailState| {
            let shade = if state.scale_flags & (1 << ContrailState::SCALE_COLOR) != 0 {
                self.density
            } else {
                1.0
            };
            let width = scaled(
                state.width,
                self.density,
                state.scale_flags,
                ContrailState::SCALE_WIDTH,
            );
            (
                width,
                mix(argb(state.color[0]), argb(state.color[1]), shade),
            )
        };
        let (width, color) = of(&states[index]);
        match states.get(index + 1).filter(|_| self.transitioning) {
            Some(next) => {
                let (to_width, to_color) = of(next);
                let t = self.time.clamp(0.0, 1.0);
                Some((width + (to_width - width) * t, mix(color, to_color, t)))
            }
            None => Some((width, color)),
        }
    }
}

/// A contrail an object trails.
#[derive(Clone, Debug, PartialEq)]
struct Trail {
    /// Its index in the level's contrails.
    contrail: usize,
    /// Its points, the newest first.
    points: VecDeque<TrailPoint>,
    /// Its attachment's function's value.
    density: f32,
    /// Its attachment's function is on.
    active: bool,
    /// The part of a point owed, which the next ticks add to.
    until_point: f32,
    /// Its bitmap's sequence, and the frame of it.
    sequence: i32,
    frame: i32,
    /// The seconds into its frame.
    frame_time: f32,
    /// How far its picture has slid across and along.
    offset: [f32; 2],
    /// Its object's change colour it is tinted by.
    tint: [f32; 3],
    /// Its object still carries it.
    carried: bool,
}

impl Trail {
    fn new(contrail: usize, definition: &Contrail, level: &Level, random: &mut Random) -> Self {
        let mut trail = Self {
            contrail,
            points: VecDeque::new(),
            density: 1.0,
            active: false,
            until_point: 0.0,
            sequence: -1,
            frame: 0,
            frame_time: 0.0,
            offset: [0.0; 2],
            tint: [1.0; 3],
            carried: true,
        };
        trail.next_frame(definition, level, random);
        trail
    }

    fn flags(contrail: &Contrail) -> u32 {
        u32::from(contrail.scale_flags)
    }

    /// Its picture's next frame, as the original's `contrail_next_frame`:
    /// past its sequence's last, the first of a random one of the
    /// contrail's sequences.
    fn next_frame(&mut self, contrail: &Contrail, level: &Level, random: &mut Random) {
        self.frame += 1;
        let frames = contrail
            .bitmap
            .as_ref()
            .and_then(|tag| level.bitmap(tag))
            .and_then(|bitmap| bitmap.sequences.get(usize::try_from(self.sequence).ok()?))
            .map_or(0, |sequence| {
                if sequence.sprites.is_empty() {
                    i32::from(sequence.image_count)
                } else {
                    i32::try_from(sequence.sprites.len()).unwrap_or(i32::MAX)
                }
            });
        if self.sequence < 0 || self.frame >= frames {
            let [first, count] = contrail.sequences.map(i32::from);
            self.sequence = first + random.below(count);
            self.frame = 0;
        }
    }

    /// The points owed for `dt`, by the contrail's rate.
    fn owed(&mut self, contrail: &Contrail, dt: f32) -> usize {
        let rate = scaled(
            contrail.point_generation_rate,
            self.density,
            Self::flags(contrail),
            Contrail::SCALE_RATE,
        );
        if rate <= 0.0 {
            return 0;
        }
        // (a hair over, so that a tick's worth at the rate makes its point
        // although the product rounds just under one)
        self.until_point += rate * dt;
        let whole = libm::floorf(self.until_point + 1e-4).max(0.0);
        self.until_point = (self.until_point - whole).max(0.0);
        (whole as usize).min(MAX_TRAIL_POINTS)
    }

    /// Leaves `count` points at `marker` (its position and forward), made
    /// at `density`, the earlier between the newest point and the marker;
    /// none if the marker has not moved and `force` is not set, as the
    /// original's `contrail_add_points`.
    fn add(
        &mut self,
        contrail: &Contrail,
        (count, force, density): (usize, bool, f32),
        (at, forward, moving): (Vec3, Vec3, Vec3),
        random: &mut Random,
    ) {
        let flags = Self::flags(contrail);
        let speed = scaled_between(
            random,
            contrail.velocity,
            self.density,
            flags,
            Contrail::SCALE_VELOCITY,
        );
        let cone = scaled(
            contrail.velocity_cone_angle,
            self.density,
            flags,
            Contrail::SCALE_CONE_ANGLE,
        );
        let inherited = scaled(
            contrail.inherited_velocity_fraction,
            self.density,
            flags,
            Contrail::SCALE_INHERITED_VELOCITY,
        );
        let previous = self.points.front().cloned();
        if !force && previous.as_ref().is_some_and(|point| point.position == at) {
            return;
        }
        let count = if previous.is_none() { 1 } else { count };
        for index in 0..count {
            let direction = random.within(forward, cone);
            let mut point = TrailPoint::new(at, moving * inherited + direction * speed, density);
            if let Some(previous) = &previous {
                let t = (index + 1) as f32 / count as f32;
                point.position = previous.position.lerp(at, t);
                point.velocity = previous.velocity.lerp(point.velocity, t);
                point.density = previous.density + (density - previous.density) * t;
            }
            self.points.push_front(point);
            if self.points.len() > MAX_TRAIL_POINTS {
                self.points.pop_back();
            }
        }
    }

    /// Its picture's frames and slide over `dt`.
    fn animate(&mut self, contrail: &Contrail, level: &Level, dt: f32, random: &mut Random) {
        let flags = Self::flags(contrail);
        let rate = scaled(
            contrail.frames_per_second,
            self.density,
            flags,
            Contrail::SCALE_ANIMATION_RATE,
        );
        if rate > 0.0 {
            let period = 1.0 / rate;
            self.frame_time += dt;
            let mut steps = 0;
            while self.frame_time >= period && steps < MAX_TRAIL_POINTS {
                self.frame_time -= period;
                self.next_frame(contrail, level, random);
                steps += 1;
            }
        }
        let [u, v] = contrail.texture_animation;
        self.offset[0] -= scaled(u, self.density, flags, Contrail::SCALE_ANIMATION_U) * dt;
        self.offset[1] += scaled(v, self.density, flags, Contrail::SCALE_ANIMATION_V) * dt;
    }

    /// Its points' lives and motions over `dt`, then its expired tail
    /// dropped, as the original's `contrail_update_points`.
    fn age(
        &mut self,
        contrail: &Contrail,
        dt: f32,
        air: &Air<'_>,
        random: &mut Random,
        mut collide: impl FnMut(Vec3, Vec3) -> Option<PointHit>,
    ) {
        for point in &mut self.points {
            point.age(&contrail.states, dt, random);
            if point.new {
                point.new = false;
                continue;
            }
            if point.expired {
                continue;
            }
            let Some(state) = usize::try_from(point.state)
                .ok()
                .and_then(|i| contrail.states.get(i))
            else {
                continue;
            };
            if let Some(physics) = &state.physics {
                let wind = air.at(point.position, physics.flags);
                point_physics::update(
                    physics,
                    &mut point.position,
                    &mut point.velocity,
                    state.width * 0.5,
                    dt,
                    (false, wind),
                    &mut collide,
                );
            }
        }
        while self.points.len() >= 2 && self.points.iter().rev().take(2).all(|point| point.expired)
        {
            self.points.pop_back();
        }
        if self.points.len() == 1 && self.points.iter().all(|point| point.expired) {
            self.points.clear();
        }
    }

    /// Its strip's edge at each point, newest first: its two vertices, its
    /// colour and its picture's u.
    fn edges(&self, contrail: &Contrail, camera: Vec3) -> Vec<Edge> {
        let flags = Self::flags(contrail);
        let repeats_u = scaled(
            contrail.texture_repeats[0],
            self.density,
            flags,
            Contrail::SCALE_REPEATS_U,
        );
        let last = self.points.len().saturating_sub(1);
        let mut edges = Vec::with_capacity(self.points.len());
        for (index, point) in self.points.iter().enumerate() {
            let Some((width, color)) = point.look(&contrail.states) else {
                return Vec::new();
            };
            let at = point.position;
            let (tangent, eye) = match index.checked_sub(1).and_then(|i| self.points.get(i)) {
                Some(newer) => (at - newer.position, camera - newer.position),
                None => (
                    self.points
                        .get(1)
                        .map_or(Vec3::ZERO, |older| older.position - at),
                    camera - at,
                ),
            };
            let half = width * 0.5;
            let across = Vec3::new(-tangent.y, tangent.x, 0.0).normalize_or_zero();
            let (side, normal) = match contrail.render_type {
                Contrail::VERTICAL => (Vec3::Z, across),
                Contrail::VIEWER_FACING => {
                    let facing = tangent.cross(eye).normalize_or_zero();
                    (facing, facing.cross(tangent).normalize_or_zero())
                }
                // (horizontal, and the media's, drawn as it)
                _ => (across, Vec3::Z),
            };
            let mut alpha = color[3] * fade(contrail, at, normal, camera);
            let unfaded = match index {
                0 => contrail.flags & Contrail::FIRST_POINT_UNFADED != 0,
                i if i == last => contrail.flags & Contrail::LAST_POINT_UNFADED != 0,
                _ => true,
            };
            if !unfaded {
                alpha = 0.0;
            }
            let [r, g, b] = self.tint;
            edges.push(Edge {
                vertices: [at - side * half, at + side * half],
                color: [
                    color[0] * r,
                    color[1] * g,
                    color[2] * b,
                    alpha.clamp(0.0, 1.0),
                ],
                u: self.offset[0] - repeats_u * index as f32,
            });
        }
        edges
    }
}

/// How much of a contrail's strip shows at `at`, its surface facing
/// `normal`, by its shader's fade mode, as the original's `contrail_fade`.
fn fade(contrail: &Contrail, at: Vec3, normal: Vec3, camera: Vec3) -> f32 {
    let mode = contrail.shader.fade_mode;
    if mode == 0 {
        return 1.0;
    }
    let to = camera - at;
    let length = to.length();
    let mut facing = if length > 0.0 {
        (normal.dot(to) / length).abs()
    } else {
        1.0
    };
    if contrail.flags & Contrail::FADES_SLOWLY != 0 {
        facing = transition(2, facing);
    }
    if mode == 2 {
        1.0 - facing
    } else {
        facing
    }
}

/// A contrail's strip across one of its points.
#[derive(Clone, Copy, Debug, PartialEq)]
struct Edge {
    vertices: [Vec3; 2],
    color: [f32; 4],
    u: f32,
}

/// A contrail an object definition carries.
#[derive(Clone, Debug, PartialEq)]
struct Carried {
    /// Its attachment's slot.
    slot: usize,
    /// Its index in the level's contrails.
    contrail: usize,
    /// The marker it leaves from.
    marker: String,
    /// The outgoing function its density is (1 to 4; none is 1).
    primary_scale: i16,
    /// The change colour it is tinted by (1 to 4; none is white).
    change_color: i16,
}

/// Each object definition's contrails, and the model their markers are
/// on.
type CarriedBy = BTreeMap<TagRef, (Vec<Carried>, Option<usize>)>;

/// The contrails objects carry, and those they trail.
#[derive(Clone, Debug, Default)]
pub(super) struct Contrails {
    carried: CarriedBy,
    /// Each object's trails, by its slot and the attachment's, in the
    /// slots of the original's contrail data array.
    trails: Slots<(ObjectHandle, usize), Trail>,
}

impl Contrails {
    pub(super) fn new(level: &Level) -> Self {
        let mut carried = BTreeMap::new();
        for object in &level.objects {
            let contrails: Vec<_> = object
                .attachments
                .iter()
                .enumerate()
                .filter_map(|(slot, attachment)| {
                    let tag = attachment.tag.as_ref()?;
                    let contrail = level.contrails.iter().position(|c| &c.tag == tag)?;
                    Some(Carried {
                        slot,
                        contrail,
                        marker: attachment.marker.clone(),
                        primary_scale: attachment.primary_scale,
                        change_color: attachment.change_color,
                    })
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
            trails: Slots::default(),
        }
    }

    /// One tick: the objects leave their contrails' points, then every
    /// trail's points age and move in `air`, meeting the world.
    pub(super) fn tick(
        &mut self,
        level: &Level,
        simulation: &mut Simulation,
        air: &Air<'_>,
        random: &mut Random,
    ) {
        if self.carried.is_empty() && self.trails.is_empty() {
            return;
        }
        self.carry(level, simulation, random);
        let world: &Simulation = simulation;
        self.trails.retain(|trail| {
            let Some(contrail) = level.contrails.get(trail.contrail) else {
                return false;
            };
            if !trail.carried {
                trail.animate(contrail, level, TICK, random);
            }
            trail.age(contrail, TICK, air, random, |from, motion| {
                world.collide_point(from, motion)
            });
            trail.carried || !trail.points.is_empty()
        });
    }

    /// The objects carrying contrails leave their points.
    fn carry(&mut self, level: &Level, simulation: &mut Simulation, random: &mut Random) {
        for trail in self.trails.values_mut() {
            trail.carried = false;
        }
        if self.carried.is_empty() {
            return;
        }
        let handles: FxHashMap<u32, ObjectHandle> = simulation
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
            let moving = (object.current.position - object.previous.position) / TICK;
            for carried in contrails {
                let Some(contrail) = level.contrails.get(carried.contrail) else {
                    continue;
                };
                let (current, _) = marker_frames(model, &object, &rest, &carried.marker);
                let marker = (
                    Vec3::from_array(current.position),
                    Vec3::from_array(current.forward),
                    moving,
                );
                let (density, active) = function_value(object.functions, carried.primary_scale);
                let tint = usize::try_from(carried.change_color - 1)
                    .ok()
                    .and_then(|i| object.appearance?.change_colors.get(i).copied())
                    .map_or([1.0; 3], |color| {
                        [16, 8, 0].map(|shift| ((color >> shift) & 0xFF) as f32 / 255.0)
                    });
                let mut made = false;
                let trail = trails.get_or_insert_with((handle, carried.slot), || {
                    made = true;
                    Trail::new(carried.contrail, contrail, level, random)
                });
                trail.carried = true;
                trail.density = density;
                trail.tint = tint;
                if made {
                    if active {
                        trail.active = true;
                        trail.add(contrail, (1, true, density), marker, random);
                    }
                } else {
                    if active != trail.active {
                        // (a point at nothing of it where it starts or stops)
                        trail.add(contrail, (1, true, 0.0), marker, random);
                        trail.active = active;
                    }
                    if active {
                        let owed = trail.owed(contrail, TICK);
                        trail.add(contrail, (owed, true, density), marker, random);
                    }
                }
                trail.animate(contrail, level, TICK, random);
            }
        });
    }

    /// The trails' strips, a quad between each two of their points (none
    /// for the ground's, which the original never draws), lit and faded
    /// for `camera`.
    pub(super) fn gather(
        &self,
        level: &Level,
        pictures: &Pictures,
        camera: Vec3,
        out: &mut Vec<ParticleSnapshot>,
    ) {
        // (in slot order, as the original draws them)
        for trail in self.trails.values() {
            let Some(contrail) = level.contrails.get(trail.contrail) else {
                continue;
            };
            if trail.points.len() < 2 || contrail.render_type == Contrail::GROUND {
                continue;
            }
            let frame = pictures.frame(
                level,
                contrail.bitmap.as_ref(),
                trail.sequence,
                usize::try_from(trail.frame).unwrap_or(0),
            );
            let flags = Trail::flags(contrail);
            let far = scaled(
                contrail.texture_repeats[1],
                trail.density,
                flags,
                Contrail::SCALE_REPEATS_V,
            ) + trail.offset[1];
            let near = trail.offset[1];
            let edges = trail.edges(contrail, camera);
            for pair in edges.windows(2) {
                if out.len() >= MAX_PARTICLES {
                    return;
                }
                let [from, to] = [pair[0], pair[1]];
                let [a, b] = from.vertices;
                let [c, d] = to.vertices;
                out.push(ParticleSnapshot {
                    position: [a.x, a.y, a.z, d.x],
                    axis: [b.x, b.y, b.z, d.y],
                    color: from.color,
                    uv: [from.u, far, to.u, near],
                    shape: [c.x, c.y, c.z, d.z],
                    modes: [
                        frame.picture,
                        blend(contrail.shader.framebuffer_blend_function),
                        PARTICLE_FACING_QUAD,
                        packed(to.color),
                    ],
                });
            }
        }
    }

    /// The trails running.
    #[cfg(test)]
    pub(super) fn len(&self) -> usize {
        self.trails.len()
    }

    /// The object definitions that carry contrails.
    #[cfg(test)]
    pub(super) fn carriers(&self) -> usize {
        self.carried.len()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use opence_content::object::Attachment;
    use opence_content::{Group, ObjectDefinition, ObjectKind, PointPhysics, SpriteShader};
    use opence_sim::objects::ObjectSpawn;
    use opence_sim::Transform;

    fn tag(group: &[u8; 4], path: &str) -> TagRef {
        TagRef {
            group: Group(*group),
            path: path.into(),
        }
    }

    fn state(duration: f32, transition: f32, width: f32, color: [[f32; 4]; 2]) -> ContrailState {
        ContrailState {
            duration: [duration; 2],
            transition: [transition; 2],
            width,
            color,
            physics: None,
            scale_flags: 0,
        }
    }

    fn contrail(render_type: i16, states: Vec<ContrailState>) -> Contrail {
        Contrail {
            tag: tag(b"cont", "trail"),
            flags: Contrail::FIRST_POINT_UNFADED | Contrail::LAST_POINT_UNFADED,
            scale_flags: 0,
            point_generation_rate: 30.0,
            velocity: [0.0, 0.0],
            velocity_cone_angle: 0.0,
            inherited_velocity_fraction: 0.0,
            render_type,
            texture_repeats: [1.0, 1.0],
            texture_animation: [0.0, 0.0],
            frames_per_second: 0.0,
            bitmap: None,
            sequences: [0, 1],
            shader: SpriteShader::default(),
            states,
        }
    }

    fn red() -> [[f32; 4]; 2] {
        [[1.0, 1.0, 0.0, 0.0]; 2]
    }

    /// A trail of points at `at`, newest first, each in state 0.
    fn trail(at: &[Vec3]) -> Trail {
        let mut trail = Trail::new(
            0,
            &contrail(0, Vec::new()),
            &level(),
            &mut Random::default(),
        );
        for &position in at {
            let mut point = TrailPoint::new(position, Vec3::ZERO, 1.0);
            point.state = 0;
            point.delta = 0.1;
            point.new = false;
            point.transitioning = false;
            trail.points.push_back(point);
        }
        trail
    }

    fn level() -> Level {
        opence_content::devmap::level(None)
    }

    #[test]
    fn scaling_applies_only_where_its_bit_is_set() {
        assert_eq!(scaled(2.0, 0.5, 0b10, 1), 1.0);
        assert_eq!(scaled(2.0, 0.5, 0b10, 0), 2.0);
        let mut random = Random::default();
        // (the least scaled by bit 0; the rest, here none, by bit 1)
        assert_eq!(scaled_between(&mut random, [2.0, 2.0], 0.5, 0b01, 0), 1.0);
        assert_eq!(function_value(None, 0), (1.0, true));
        assert_eq!(function_value(None, 2), (0.0, false));
        let values = FunctionValues {
            incoming: [0.0; 4],
            outgoing: [0.0, 0.25, 0.0, 0.0],
            active: [false, true, false, false],
        };
        assert_eq!(function_value(Some(&values), 2), (0.25, true));
        assert_eq!(packed([1.0, 0.0, 0.0, 1.0]), 0xFF00_00FF);
    }

    #[test]
    fn a_point_lives_through_each_state_then_becomes_the_next() {
        let states = [state(1.0, 0.5, 1.0, red()), state(1.0, 0.0, 3.0, red())];
        let mut random = Random::default();
        let mut point = TrailPoint::new(Vec3::ZERO, Vec3::ZERO, 1.0);
        point.age(&states, 0.0, &mut random);
        assert_eq!((point.state, point.transitioning), (0, false));
        point.age(&states, 0.5, &mut random);
        assert_eq!((point.state, point.transitioning), (0, false));
        point.age(&states, 0.6, &mut random);
        assert_eq!(
            (point.state, point.transitioning),
            (0, true),
            "becoming the next"
        );
        point.age(&states, 0.25, &mut random);
        let Some((width, _)) = point.look(&states) else {
            panic!("it has states");
        };
        assert!(width > 1.0 && width < 3.0, "{width}");
        point.age(&states, 0.3, &mut random);
        assert_eq!((point.state, point.transitioning), (1, false));
        assert_eq!(point.look(&states).map(|(width, _)| width), Some(3.0));
        point.age(&states, 1.1, &mut random);
        assert!(point.expired, "it outlived its last state");
        // (a state of no time is left at once)
        let mut fleeting = TrailPoint::new(Vec3::ZERO, Vec3::ZERO, 1.0);
        fleeting.age(&[state(0.0, 0.0, 1.0, red())], 0.0, &mut random);
        assert!(fleeting.expired);
    }

    #[test]
    fn a_states_lifetime_and_look_scale_by_the_points_density() {
        let mut scaled = state(2.0, 0.0, 2.0, [[0.0, 0.0, 0.0, 0.0], [1.0, 1.0, 1.0, 1.0]]);
        scaled.scale_flags = (1 << ContrailState::SCALE_DURATION)
            | (1 << ContrailState::SCALE_WIDTH)
            | (1 << ContrailState::SCALE_COLOR);
        let states = [scaled];
        let mut random = Random::default();
        let mut point = TrailPoint::new(Vec3::ZERO, Vec3::ZERO, 0.5);
        point.age(&states, 0.0, &mut random);
        assert!((point.delta - 1.0).abs() < 1e-6, "a second, not two");
        assert_eq!(point.look(&states), Some((1.0, [0.5; 4])));
        // (unscaled, it is its upper colour)
        let plain = [state(2.0, 0.0, 2.0, scaled.color)];
        assert_eq!(point.look(&plain), Some((2.0, [1.0; 4])));
    }

    #[test]
    fn the_expired_tail_is_dropped_and_a_lone_expired_point_too() {
        let mut trail = trail(&[Vec3::ZERO, Vec3::X, Vec3::Y, Vec3::Z]);
        for point in trail.points.iter_mut().skip(1) {
            point.expired = true;
        }
        let definition = contrail(0, vec![state(10.0, 0.0, 1.0, red())]);
        let mut random = Random::default();
        trail.age(&definition, 0.0, &Air::still(), &mut random, |_, _| None);
        assert_eq!(trail.points.len(), 2, "the newest and one expired end");
        let mut lone = self::trail(&[Vec3::ZERO]);
        lone.points[0].expired = true;
        lone.age(&definition, 0.0, &Air::still(), &mut random, |_, _| None);
        assert!(lone.points.is_empty());
    }

    #[test]
    fn points_move_by_their_states_physics_but_not_when_new() {
        let mut moving = state(10.0, 0.0, 1.0, red());
        moving.physics = Some(PointPhysics {
            flags: PointPhysics::NO_GRAVITY,
            ..PointPhysics::default()
        });
        let definition = contrail(0, vec![moving]);
        let mut trail = trail(&[]);
        let mut random = Random::default();
        trail
            .points
            .push_front(TrailPoint::new(Vec3::ZERO, Vec3::X, 1.0));
        trail.age(&definition, 0.5, &Air::still(), &mut random, |_, _| None);
        assert_eq!(trail.points[0].position, Vec3::ZERO, "made this tick");
        trail.age(&definition, 0.5, &Air::still(), &mut random, |_, _| None);
        assert!((trail.points[0].position.x - 0.5).abs() < 1e-4);
        // (without physics, a point stays where it was left)
        let still = contrail(0, vec![state(10.0, 0.0, 1.0, red())]);
        let mut trail = self::trail(&[Vec3::ZERO]);
        trail.points[0].velocity = Vec3::X;
        trail.age(&still, 0.5, &Air::still(), &mut random, |_, _| None);
        assert_eq!(trail.points[0].position, Vec3::ZERO);
    }

    #[test]
    fn points_are_left_between_the_newest_and_the_marker() {
        let definition = contrail(0, vec![state(10.0, 0.0, 1.0, red())]);
        let mut trail = trail(&[]);
        let mut random = Random::default();
        let marker = |x: f32| (Vec3::new(x, 0.0, 0.0), Vec3::X, Vec3::ZERO);
        trail.add(&definition, (3, true, 1.0), marker(0.0), &mut random);
        assert_eq!(trail.points.len(), 1, "the first point is one");
        trail.add(&definition, (2, false, 0.0), marker(0.0), &mut random);
        assert_eq!(trail.points.len(), 1, "the marker has not moved");
        trail.add(&definition, (2, false, 0.0), marker(2.0), &mut random);
        let xs: Vec<_> = trail.points.iter().map(|point| point.position.x).collect();
        assert_eq!(xs, [2.0, 1.0, 0.0], "newest first");
        assert_eq!(trail.points[1].density, 0.5);
        assert_eq!(trail.owed(&definition, 0.1), 3);
        assert_eq!(trail.owed(&definition, 0.01), 0);
    }

    #[test]
    fn its_picture_slides_and_steps_through_its_frames() {
        let mut level = level();
        let mut sheet = crate::render_level::tests::bitmap("sheet", 2, 1);
        sheet.sequences = vec![opence_content::Sequence {
            name: String::new(),
            first_image: Some(0),
            image_count: 2,
            sprites: Vec::new(),
        }];
        level.bitmaps.push(sheet);
        let mut definition = contrail(0, Vec::new());
        definition.bitmap = Some(tag(b"bitm", "sheet"));
        definition.frames_per_second = 10.0;
        definition.texture_animation = [2.0, 4.0];
        let mut random = Random::default();
        let mut trail = Trail::new(0, &definition, &level, &mut random);
        assert_eq!((trail.sequence, trail.frame), (0, 0));
        trail.animate(&definition, &level, 0.1, &mut random);
        assert_eq!(trail.frame, 1);
        trail.animate(&definition, &level, 0.1, &mut random);
        assert_eq!(trail.frame, 0, "past the last, a sequence's first");
        assert!((trail.offset[0] + 0.4).abs() < 1e-5);
        assert!((trail.offset[1] - 0.8).abs() < 1e-5);
    }

    #[test]
    fn its_strip_lies_by_its_render_type() {
        let points = [Vec3::ZERO, Vec3::new(-1.0, 0.0, 0.0)];
        let camera = Vec3::new(0.0, 0.0, 10.0);
        let vertical = contrail(Contrail::VERTICAL, vec![state(1.0, 0.0, 2.0, red())]);
        let edges = trail(&points).edges(&vertical, camera);
        assert_eq!(
            edges[0].vertices,
            [Vec3::new(0.0, 0.0, -1.0), Vec3::new(0.0, 0.0, 1.0)]
        );
        let horizontal = Contrail {
            render_type: Contrail::HORIZONTAL,
            ..vertical.clone()
        };
        let edges = trail(&points).edges(&horizontal, camera);
        assert_eq!(
            edges[1].vertices,
            [Vec3::new(-1.0, 1.0, 0.0), Vec3::new(-1.0, -1.0, 0.0)]
        );
        let viewer = Contrail {
            render_type: Contrail::VIEWER_FACING,
            ..vertical.clone()
        };
        let edges = trail(&points).edges(&viewer, Vec3::new(0.0, -10.0, 0.0));
        let [a, b] = edges[0].vertices;
        assert!(
            (a - b).normalize().dot(Vec3::Z).abs() > 0.999,
            "across the view"
        );
        // (its picture repeats along it)
        assert_eq!(
            edges.iter().map(|edge| edge.u).collect::<Vec<_>>(),
            [0.0, -1.0]
        );
    }

    #[test]
    fn its_ends_and_its_edge_on_strips_fade() {
        let points = [
            Vec3::ZERO,
            Vec3::new(-1.0, 0.0, 0.0),
            Vec3::new(-2.0, 0.0, 0.0),
        ];
        let mut definition = contrail(Contrail::HORIZONTAL, vec![state(1.0, 0.0, 2.0, red())]);
        definition.flags = 0;
        let alphas = |definition: &Contrail, camera: Vec3| -> Vec<f32> {
            trail(&points)
                .edges(definition, camera)
                .iter()
                .map(|edge| edge.color[3])
                .collect()
        };
        let above = Vec3::new(0.0, 0.0, 10.0);
        assert_eq!(alphas(&definition, above), [0.0, 1.0, 0.0]);
        definition.flags = Contrail::FIRST_POINT_UNFADED | Contrail::LAST_POINT_UNFADED;
        definition.shader.fade_mode = 1;
        let faced = alphas(&definition, above);
        assert!(
            faced.iter().all(|&alpha| alpha > 0.95),
            "faced, it shows: {faced:?}"
        );
        let beside = alphas(&definition, Vec3::new(-1.0, 10.0, 0.0));
        assert!(
            beside.iter().all(|&alpha| alpha < 1e-3),
            "edge on: {beside:?}"
        );
        definition.shader.fade_mode = 2;
        let inverted = alphas(&definition, above);
        assert!(
            inverted.iter().all(|&alpha| alpha < 0.05),
            "faced, the inverted fade hides it: {inverted:?}"
        );
    }

    fn carrier(level: &mut Level, attachment: Attachment) {
        level.objects.push(ObjectDefinition {
            widgets: Vec::new(),
            tag: tag(b"proj", "rocket"),
            kind: ObjectKind::Scenery,
            flags: 0,
            bounding_radius: 1.0,
            bounding_offset: [0.0, 0.0, 0.0],
            acceleration_scale: 1.0,
            render_bounding_radius: 1.0,
            icon_text_index: -1,
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
            attachments: vec![attachment],
        });
    }

    fn spawn(simulation: &mut Simulation) -> ObjectHandle {
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
        handle
    }

    #[test]
    fn an_object_trails_its_contrail_as_quads_between_its_points() {
        let mut level = level();
        let mut definition = contrail(Contrail::HORIZONTAL, vec![state(1.0, 0.0, 0.5, red())]);
        definition.velocity = [3.0, 3.0];
        definition.states[0].physics = Some(PointPhysics {
            flags: PointPhysics::NO_GRAVITY,
            ..PointPhysics::default()
        });
        level.contrails.push(definition);
        carrier(
            &mut level,
            Attachment {
                tag: Some(tag(b"cont", "trail")),
                marker: String::new(),
                primary_scale: 0,
                change_color: 0,
            },
        );
        let mut contrails = Contrails::new(&level);
        assert_eq!(contrails.carriers(), 1);
        let mut simulation = Simulation::empty();
        let handle = spawn(&mut simulation);
        let mut random = Random::default();
        for _ in 0..3 {
            contrails.tick(&level, &mut simulation, &Air::still(), &mut random);
        }
        let mut out = Vec::new();
        contrails.gather(
            &level,
            &Pictures::default(),
            Vec3::new(0.0, 0.0, 10.0),
            &mut out,
        );
        assert_eq!(out.len(), 2, "three points, two quads: {out:?}");
        assert_eq!(out[0].modes[2], PARTICLE_FACING_QUAD);
        assert_eq!(out[0].color, [1.0, 0.0, 0.0, 1.0]);
        assert_eq!(out[0].modes[3], packed([1.0, 0.0, 0.0, 1.0]));
        // (the newest is at the marker; the older have flown forward)
        assert_eq!([out[0].position[0], out[0].axis[0]], [0.0, 0.0]);
        assert!(out[0].shape[0] > 0.0, "{:?}", out[0]);
        assert!(simulation.remove(handle));
        for _ in 0..31 {
            contrails.tick(&level, &mut simulation, &Air::still(), &mut random);
        }
        assert_eq!(contrails.len(), 0, "its points died");
    }

    /// `carriers` rockets trailing a contrail whose points' lifetimes and
    /// speeds are drawn at random, stepped `ticks` ticks: each tick's strips.
    fn trailed(carriers: usize, ticks: usize) -> Vec<Vec<ParticleSnapshot>> {
        let mut level = level();
        let mut aging = state(1.0, 0.0, 0.5, red());
        aging.duration = [0.2, 1.5];
        let mut definition = contrail(Contrail::HORIZONTAL, vec![aging]);
        definition.velocity = [1.0, 5.0];
        definition.velocity_cone_angle = 0.5;
        level.contrails.push(definition);
        carrier(
            &mut level,
            Attachment {
                tag: Some(tag(b"cont", "trail")),
                marker: String::new(),
                primary_scale: 0,
                change_color: 0,
            },
        );
        let mut contrails = Contrails::new(&level);
        let mut simulation = Simulation::empty();
        for _ in 0..carriers {
            spawn(&mut simulation);
        }
        let mut random = Random::default();
        (0..ticks)
            .map(|_| {
                contrails.tick(&level, &mut simulation, &Air::still(), &mut random);
                let mut out = Vec::new();
                contrails.gather(
                    &level,
                    &Pictures::default(),
                    Vec3::new(0.0, 0.0, 10.0),
                    &mut out,
                );
                out
            })
            .collect()
    }

    #[test]
    fn the_same_start_trails_the_same_each_tick() {
        // (each run's maps hash by their own seed; the draws must not follow it)
        let first = trailed(8, 40);
        assert!(
            first.iter().any(|strips| strips.len() > 8),
            "the rockets trail"
        );
        for run in 1..4 {
            assert!(trailed(8, 40) == first, "run {run} trailed otherwise");
        }
    }

    #[test]
    fn a_trail_takes_the_lowest_free_slot_and_they_age_in_slot_order() {
        let mut slots = Slots::default();
        for key in [30, 10, 20] {
            slots.get_or_insert_with(key, || key * 2);
        }
        assert_eq!(slots.values().copied().collect::<Vec<_>>(), [60, 20, 40]);
        slots.retain(|value| *value != 20);
        slots.get_or_insert_with(5, || 5);
        assert_eq!(slots.values().copied().collect::<Vec<_>>(), [60, 5, 40]);
        assert_eq!(
            *slots.get_or_insert_with(30, || 0),
            60,
            "a held key is not made again"
        );
        assert_eq!(slots.len(), 3);
    }

    #[test]
    fn a_contrail_off_by_its_function_leaves_nothing_and_the_ground_is_not_drawn() {
        let mut level = level();
        level.contrails.push(contrail(
            Contrail::GROUND,
            vec![state(1.0, 0.0, 0.5, red())],
        ));
        carrier(
            &mut level,
            Attachment {
                tag: Some(tag(b"cont", "trail")),
                marker: String::new(),
                primary_scale: 0,
                change_color: 0,
            },
        );
        let mut contrails = Contrails::new(&level);
        let mut simulation = Simulation::empty();
        spawn(&mut simulation);
        let mut random = Random::default();
        for _ in 0..3 {
            contrails.tick(&level, &mut simulation, &Air::still(), &mut random);
        }
        let mut out = Vec::new();
        contrails.gather(&level, &Pictures::default(), Vec3::ZERO, &mut out);
        assert!(out.is_empty(), "the ground's are never drawn");
        // (by function A, which an object without functions has off)
        level.objects[0].attachments[0].primary_scale = 1;
        let mut contrails = Contrails::new(&level);
        for _ in 0..3 {
            contrails.tick(&level, &mut simulation, &Air::still(), &mut random);
        }
        assert_eq!(contrails.len(), 1);
        assert!(contrails
            .trails
            .values()
            .all(|trail| trail.points.is_empty()));
    }
}
