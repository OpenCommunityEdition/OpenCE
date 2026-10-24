//! The particle systems (`pctl`): each of a system's kinds of particle has
//! an emitter that passes through its states, making particles at its
//! rate, and each particle passes through states of its own, its look
//! drawn between each state's bounds and cross-faded into the next's.
//! Effects make the systems that go free; objects carry the systems their
//! definitions attach, at a marker, for as long as they are there.

use std::collections::{BTreeMap, HashMap};

use opence_content::{
    Level, ParticleState, ParticleSystem, ParticleSystemType, ParticleTypeVariables, TagRef,
};
use opence_sim::effects::FiredPart;
use opence_sim::objects::ObjectHandle;
use opence_sim::particle_collision::PointHit;
use opence_sim::point_physics;
use opence_sim::seats::default_pose;
use opence_sim::{Simulation, Vec3};

use super::{blend, mix, rgba, Pictures, Random, TICK};
use crate::lights::marker_frames;
use crate::snapshot::{
    ParticleSnapshot, MAX_PARTICLES, PARTICLE_FACING_AXIS, PARTICLE_FACING_SCREEN,
};

/// The most systems running at once.
const MAX_SYSTEMS: usize = 64;
/// The most particles all the systems keep alive.
const MAX_SYSTEM_PARTICLES: usize = 1024;
/// The most particles one emitter makes in a tick.
const MAX_MADE_A_TICK: f32 = 128.0;
/// How much of its state an emitter with fewer than its fewest particles
/// keeps each tick (it runs the state out faster).
const SHORT_OF_PARTICLES: f32 = 0.3;
/// The least a state lasts, so that a run of empty states ends.
const LEAST_SPAN: f32 = 1e-3;
/// The most states a run passes in a tick.
const MOST_STATES_A_TICK: usize = 16;
/// How a type makes its particles: in an explosion.
const CREATION_EXPLOSION: i16 = 1;
/// ...in a jet.
const CREATION_JET: i16 = 2;
/// The least weight of the state a particle is becoming for it to be
/// drawn too.
const LEAST_CROSS_FADE: f32 = 0.01;

/// Where a run of states stands.
#[derive(Clone, Copy, Debug, PartialEq)]
struct Run {
    /// The state it is in.
    state: usize,
    /// It goes backwards (a forwards-and-backwards loop).
    backward: bool,
    /// The state it is becoming, and the way it will go then.
    next: Option<(usize, bool)>,
    /// The seconds left of the state, or of the becoming.
    left: f32,
    /// The seconds the state, or the becoming, lasts.
    length: f32,
}

/// How a run of states goes on past its ends.
#[derive(Clone, Copy, Debug, PartialEq)]
struct Looping {
    /// It goes round again.
    loops: bool,
    /// ...forwards and backwards.
    back_and_forth: bool,
}

/// What a run of states is made of: each state's seconds, and its seconds
/// becoming the next.
type Spans<'a> = &'a dyn Fn(usize) -> ([f32; 2], [f32; 2]);

impl Run {
    fn new(random: &mut Random, spans: Spans<'_>) -> Self {
        let length = random.between(spans(0).0).max(LEAST_SPAN);
        Self {
            state: 0,
            backward: false,
            next: None,
            left: length,
            length,
        }
    }

    /// The state after `self`'s, and the way the run goes then, if there
    /// is one.
    fn following(&self, count: usize, looping: Looping) -> Option<(usize, bool)> {
        if !self.backward && self.state + 1 < count {
            return Some((self.state + 1, false));
        }
        if self.backward && self.state > 0 {
            return Some((self.state - 1, true));
        }
        if !looping.loops || count == 0 {
            return None;
        }
        if looping.back_and_forth && count > 1 {
            return Some(if self.backward {
                (1, false)
            } else {
                (count - 2, true)
            });
        }
        Some((0, false))
    }

    /// `dt` seconds on: false once the run is over. `entered` is told of
    /// each state it begins becoming, and each it enters.
    fn advance(
        &mut self,
        dt: f32,
        random: &mut Random,
        (count, looping): (usize, Looping),
        spans: Spans<'_>,
        mut entered: impl FnMut(&mut Random, usize),
    ) -> bool {
        self.left -= dt;
        let mut passed = 0;
        while self.left <= 0.0 {
            passed += 1;
            if passed > MOST_STATES_A_TICK {
                self.left = self.length;
                break;
            }
            let length = match self.next.take() {
                Some((state, backward)) => {
                    self.state = state;
                    self.backward = backward;
                    random.between(spans(state).0)
                }
                None => {
                    let Some((state, backward)) = self.following(count, looping) else {
                        return false;
                    };
                    let becoming = random.between(spans(self.state).1);
                    entered(random, state);
                    if becoming > 0.0 {
                        self.next = Some((state, backward));
                        becoming
                    } else {
                        self.state = state;
                        self.backward = backward;
                        random.between(spans(state).0)
                    }
                }
            };
            self.length = length.max(LEAST_SPAN);
            self.left += self.length;
        }
        true
    }

    /// The state it is becoming, and how far it has become it, 0 to 1.
    fn becoming(&self) -> Option<(usize, f32)> {
        let (state, _) = self.next?;
        Some((state, (1.0 - self.left / self.length).clamp(0.0, 1.0)))
    }
}

/// A particle's look in a state, drawn between the state's bounds: its
/// size, frames a second, spin (turns a second) and colour (red, green,
/// blue, alpha).
#[derive(Clone, Copy, Debug, Default, PartialEq)]
struct Look {
    scale: f32,
    animation_rate: f32,
    rotation_rate: f32,
    color: [f32; 4],
}

impl Look {
    fn drawn(random: &mut Random, state: &ParticleState) -> Self {
        Self {
            scale: random.between(state.scale),
            animation_rate: random.between(state.animation_rate),
            rotation_rate: random.between(state.rotation_rate),
            color: mix(rgba(state.color[0]), rgba(state.color[1]), random.next()),
        }
    }

    fn mix(&self, other: &Self, t: f32) -> Self {
        let lerp = |a: f32, b: f32| a + (b - a) * t;
        Self {
            scale: lerp(self.scale, other.scale),
            animation_rate: lerp(self.animation_rate, other.animation_rate),
            rotation_rate: lerp(self.rotation_rate, other.rotation_rate),
            color: mix(self.color, other.color, t),
        }
    }
}

/// A particle of a system.
#[derive(Clone, Debug, PartialEq)]
struct Bit {
    position: Vec3,
    velocity: Vec3,
    run: Run,
    /// Its look in its state, and in the state it is becoming.
    look: Look,
    next_look: Look,
    rotation: f32,
    frame: f32,
}

/// One of a system's kinds of particle: its emitter and its particles.
#[derive(Clone, Debug, PartialEq)]
struct Emitter {
    /// Its emitter's run of states, until it is over.
    run: Option<Run>,
    /// The particles it is owed (a part of one carried from tick to tick).
    owed: f32,
    bits: Vec<Bit>,
}

/// A particle system running.
#[derive(Clone, Debug, PartialEq)]
struct Running {
    /// Its index in the level's systems.
    system: usize,
    position: Vec3,
    velocity: Vec3,
    /// The way it faces.
    forward: Vec3,
    /// What its types' scaled values are scaled by, 0 to 1.
    scale: f32,
    /// Its colour (red, green, blue, alpha).
    tint: [f32; 4],
    /// It is carried (by an object still there): its emitters loop.
    carried: bool,
    /// It goes free: it moves by its own physics.
    free: bool,
    emitters: Vec<Emitter>,
}

/// Each type state's seconds and seconds becoming the next.
fn type_spans(kind: &ParticleSystemType) -> impl Fn(usize) -> ([f32; 2], [f32; 2]) + '_ {
    |index| {
        kind.type_states
            .get(index)
            .map_or(([0.0; 2], [0.0; 2]), |state| {
                (state.duration, state.transition)
            })
    }
}

/// Each particle state's seconds and seconds becoming the next.
fn particle_spans(kind: &ParticleSystemType) -> impl Fn(usize) -> ([f32; 2], [f32; 2]) + '_ {
    |index| {
        kind.particle_states
            .get(index)
            .map_or(([0.0; 2], [0.0; 2]), |state| {
                (state.duration, state.transition)
            })
    }
}

/// What an emitter's run of states sets now, between its state's and the
/// next's.
fn variables(kind: &ParticleSystemType, run: &Run) -> ParticleTypeVariables {
    let of = |index: usize| {
        kind.type_states
            .get(index)
            .map(|state| state.variables)
            .unwrap_or_default()
    };
    match run.becoming() {
        Some((next, t)) => of(run.state).mix(&of(next), t),
        None => of(run.state),
    }
}

impl Running {
    fn new(
        level: &Level,
        system: usize,
        at: (Vec3, Vec3, Vec3),
        carried: bool,
        random: &mut Random,
    ) -> Self {
        let (position, velocity, forward) = at;
        let types = level
            .particle_systems
            .get(system)
            .map_or(&[][..], |system| &system.types[..]);
        let emitters = types
            .iter()
            .map(|kind| Emitter {
                run: (kind.flags & ParticleSystemType::DISABLED == 0
                    && !kind.type_states.is_empty())
                .then(|| Run::new(random, &type_spans(kind))),
                owed: 0.0,
                bits: Vec::new(),
            })
            .collect();
        let mut running = Self {
            system,
            position,
            velocity,
            forward,
            scale: 1.0,
            tint: [1.0; 4],
            carried,
            free: !carried,
            emitters,
        };
        // (each type begins with its first count)
        for (kind, emitter) in types.iter().zip(&mut running.emitters) {
            if emitter.run.is_none() {
                continue;
            }
            let mut count = f32::from(kind.initial_count.max(0));
            if kind.flags & ParticleSystemType::INITIAL_COUNT_SCALES_WITH_EFFECT != 0 {
                count *= running.scale;
            }
            emitter.owed = count;
        }
        running
    }

    /// One tick: its emitters make particles and its particles move,
    /// meeting the world where `collide` says. False once it is over.
    fn tick(
        &mut self,
        level: &Level,
        random: &mut Random,
        room: &mut usize,
        mut collide: impl FnMut(Vec3, Vec3) -> Option<PointHit>,
    ) -> bool {
        let Some(system) = level.particle_systems.get(self.system) else {
            return false;
        };
        if self.free {
            self.drift(system, &mut collide);
        }
        let (position, velocity, forward, scale) =
            (self.position, self.velocity, self.forward, self.scale);
        let carried = self.carried;
        let mut live = false;
        for (kind, emitter) in system.types.iter().zip(&mut self.emitters) {
            emitter.make(
                kind,
                (position, velocity, forward, scale),
                carried,
                random,
                room,
            );
            emitter
                .bits
                .retain_mut(|bit| bit.tick(kind, scale, random, &mut collide));
            live |= emitter.run.is_some() || !emitter.bits.is_empty();
        }
        live
    }

    /// One tick of its own physics, as it goes free.
    fn drift(
        &mut self,
        system: &ParticleSystem,
        collide: &mut impl FnMut(Vec3, Vec3) -> Option<PointHit>,
    ) {
        match &system.point_physics {
            Some(physics) if system.update_physics == 0 => {
                point_physics::update(
                    physics,
                    &mut self.position,
                    &mut self.velocity,
                    1.0,
                    TICK,
                    (false, Vec3::ZERO),
                    collide,
                );
            }
            _ => self.position += self.velocity * TICK,
        }
    }

    fn gather(&self, level: &Level, pictures: &Pictures, out: &mut Vec<ParticleSnapshot>) {
        let Some(system) = level.particle_systems.get(self.system) else {
            return;
        };
        for (kind, emitter) in system.types.iter().zip(&self.emitters) {
            let multiplier = emitter.run.as_ref().map_or_else(
                || {
                    kind.type_states
                        .last()
                        .map(|s| s.variables)
                        .unwrap_or_default()
                },
                |run| variables(kind, run),
            );
            let tint = if kind.flags & ParticleSystemType::TINT_BY_EFFECT_COLOR != 0 {
                self.tint
            } else {
                [1.0; 4]
            };
            for bit in &emitter.bits {
                bit.gather(level, pictures, kind, (&multiplier, tint, self.scale), out);
                if out.len() >= MAX_PARTICLES {
                    return;
                }
            }
        }
    }
}

impl Emitter {
    /// Its run of states goes on, and it makes the particles it is owed.
    fn make(
        &mut self,
        kind: &ParticleSystemType,
        (position, velocity, forward, scale): (Vec3, Vec3, Vec3, f32),
        carried: bool,
        random: &mut Random,
        room: &mut usize,
    ) {
        let Some(run) = &mut self.run else {
            return;
        };
        let now = variables(kind, run);
        let mut least = now.minimum_count;
        let mut rate = now.creation_rate;
        if kind.flags & ParticleSystemType::MINIMUM_COUNT_SCALES_WITH_EFFECT != 0 {
            least *= scale;
        }
        if kind.flags & ParticleSystemType::CREATION_RATE_SCALES_WITH_EFFECT != 0 {
            rate *= scale;
        }
        if (self.bits.len() as f32) < least {
            run.left *= libm::powf(SHORT_OF_PARTICLES, TICK * 30.0);
        }
        // (a free system's emitters do not loop)
        let looping = Looping {
            loops: carried && kind.flags & ParticleSystemType::TYPE_STATES_LOOP != 0,
            back_and_forth: kind.flags & ParticleSystemType::TYPE_STATES_LOOP_FORWARD_BACKWARD != 0,
        };
        let going = run.advance(
            TICK,
            random,
            (kind.type_states.len(), looping),
            &type_spans(kind),
            |_, _| {},
        );
        let state = run.state;
        if !going {
            self.run = None;
        }
        if kind.particle_states.is_empty() {
            self.owed = 0.0;
            return;
        }
        self.owed += rate.max(0.0) * TICK;
        let mut made = 0.0;
        while self.owed >= 1.0 && made < MAX_MADE_A_TICK && *room > 0 {
            self.owed -= 1.0;
            made += 1.0;
            *room -= 1;
            let creation = kind
                .type_states
                .get(state)
                .map(|state| state.creation_physics)
                .filter(|&physics| physics >= 0)
                .unwrap_or(kind.creation_physics);
            let constants = &kind.physics_constants;
            let speed = constants.first().copied().unwrap_or(0.0);
            let own = match creation {
                CREATION_EXPLOSION => random.direction() * speed,
                CREATION_JET => {
                    random.within(forward, constants.get(1).copied().unwrap_or(0.0)) * speed
                }
                _ => Vec3::ZERO,
            };
            self.bits
                .push(Bit::new(kind, position, velocity + own, random));
        }
        self.owed = self.owed.min(MAX_MADE_A_TICK);
    }
}

impl Bit {
    fn new(kind: &ParticleSystemType, position: Vec3, velocity: Vec3, random: &mut Random) -> Self {
        let run = Run::new(random, &particle_spans(kind));
        let look = kind
            .particle_states
            .first()
            .map(|state| Look::drawn(random, state))
            .unwrap_or_default();
        let rotation = if kind.flags & ParticleSystemType::ROTATIONAL_SPRITES_ANIMATE_SIDEWAYS != 0
        {
            std::f32::consts::TAU * random.next()
        } else {
            0.0
        };
        Self {
            position,
            velocity,
            run,
            look,
            next_look: look,
            rotation,
            frame: 0.0,
        }
    }

    /// Its look now, between its state's and the next's.
    fn look_now(&self) -> Look {
        match self.run.becoming() {
            Some((_, t)) => self.look.mix(&self.next_look, t),
            None => self.look,
        }
    }

    /// One tick: false once it is gone.
    fn tick(
        &mut self,
        kind: &ParticleSystemType,
        scale: f32,
        random: &mut Random,
        collide: &mut impl FnMut(Vec3, Vec3) -> Option<PointHit>,
    ) -> bool {
        let Some(state) = kind.particle_states.get(self.run.state) else {
            return false;
        };
        let radius = kind.radius * state.radius;
        let mut grounded = false;
        let mut in_water = false;
        match &state.point_physics {
            Some(physics) => {
                let moved = point_physics::update(
                    physics,
                    &mut self.position,
                    &mut self.velocity,
                    radius,
                    TICK,
                    (false, Vec3::ZERO),
                    &mut *collide,
                );
                grounded = moved.collided();
                in_water = moved.in_water;
            }
            None => self.position += self.velocity * TICK,
        }
        let dies = (in_water && kind.flags & ParticleSystemType::PARTICLES_DIE_IN_WATER != 0)
            || (!in_water && kind.flags & ParticleSystemType::PARTICLES_DIE_IN_AIR != 0)
            || (grounded && kind.flags & ParticleSystemType::PARTICLES_DIE_ON_GROUND != 0);
        if dies {
            return false;
        }
        let mut look = self.look_now();
        if kind.flags & ParticleSystemType::ANIMATION_RATE_SCALES_WITH_EFFECT != 0 {
            look.animation_rate *= scale;
        }
        if kind.flags & ParticleSystemType::ROTATION_RATE_SCALES_WITH_EFFECT != 0 {
            look.rotation_rate *= scale;
        }
        self.frame += look.animation_rate.max(0.0) * TICK;
        self.rotation += std::f32::consts::TAU * look.rotation_rate * TICK;
        let looping = Looping {
            loops: kind.flags & ParticleSystemType::PARTICLE_STATES_LOOP != 0,
            back_and_forth: kind.flags & ParticleSystemType::PARTICLE_STATES_LOOP_FORWARD_BACKWARD
                != 0,
        };
        let mut drawn = None;
        let going = self.run.advance(
            TICK,
            random,
            (kind.particle_states.len(), looping),
            &particle_spans(kind),
            |random, state| {
                drawn = kind
                    .particle_states
                    .get(state)
                    .map(|state| Look::drawn(random, state));
            },
        );
        if let Some(look) = drawn {
            self.next_look = look;
        }
        // (out of a becoming, the look it became is its look)
        if self.run.next.is_none() {
            self.look = self.next_look;
        }
        going
    }

    fn gather(
        &self,
        level: &Level,
        pictures: &Pictures,
        kind: &ParticleSystemType,
        (multiplier, tint, scale): (&ParticleTypeVariables, [f32; 4], f32),
        out: &mut Vec<ParticleSnapshot>,
    ) {
        let along = self.velocity.normalize_or_zero();
        let facing = match kind.sprite_render_mode {
            1 | 2 if along != Vec3::ZERO => PARTICLE_FACING_AXIS,
            _ => PARTICLE_FACING_SCREEN,
        };
        let by = &multiplier.randomized;
        let tinted = rgba(by.color);
        let mut draw = |state: &ParticleState, look: &Look, weight: f32| {
            let mut size = look.scale * by.scale;
            if kind.flags & ParticleSystemType::SCALE_SCALES_WITH_EFFECT != 0 {
                size *= scale;
            }
            let radius = kind.radius * multiplier.radius * state.radius * size;
            let frame = pictures.frame(
                level,
                state.bitmap.as_ref(),
                i32::from(state.sequence),
                self.frame as usize,
            );
            let color = [0, 1, 2, 3].map(|i| look.color[i] * tinted[i] * tint[i]);
            out.push(ParticleSnapshot {
                position: [self.position.x, self.position.y, self.position.z, radius],
                axis: [along.x, along.y, along.z, self.rotation],
                color: [color[0], color[1], color[2], color[3] * weight],
                uv: frame.uv,
                shape: [radius, 0.0, 0.0, 0.0],
                modes: [
                    frame.picture,
                    blend(state.shader.framebuffer_blend_function),
                    facing,
                    0,
                ],
            });
        };
        let Some(state) = kind.particle_states.get(self.run.state) else {
            return;
        };
        match self.run.becoming() {
            Some((next, t)) if t > LEAST_CROSS_FADE => {
                if t < 1.0 - LEAST_CROSS_FADE {
                    draw(state, &self.look, 1.0 - t);
                }
                if let Some(next) = kind.particle_states.get(next) {
                    draw(next, &self.next_look, t);
                }
            }
            _ => draw(state, &self.look, 1.0),
        }
    }
}

/// A system an object definition carries: its attachment's slot, its
/// index in the level's systems, the marker it is at and the function
/// that scales it (1 to 4, or none).
type CarriedSystem = (usize, usize, String, i16);

/// The particle systems running: those effects made, and those objects
/// carry.
#[derive(Clone, Debug, Default)]
pub(super) struct Systems {
    /// Each object definition's systems, and the model their markers are
    /// on.
    carried: BTreeMap<TagRef, (Vec<CarriedSystem>, Option<usize>)>,
    /// Each system's index in the level's.
    index: HashMap<TagRef, usize>,
    /// The systems effects made, oldest first.
    free: Vec<Running>,
    /// The systems objects carry, by the object and the attachment's slot.
    attached: BTreeMap<(ObjectHandle, usize), Running>,
}

impl Systems {
    pub(super) fn new(level: &Level) -> Self {
        let index: HashMap<TagRef, usize> = level
            .particle_systems
            .iter()
            .enumerate()
            .map(|(at, system)| (system.tag.clone(), at))
            .collect();
        let mut carried = BTreeMap::new();
        for object in &level.objects {
            let systems: Vec<_> = object
                .attachments
                .iter()
                .enumerate()
                .filter_map(|(slot, attachment)| {
                    let system = *index.get(attachment.tag.as_ref()?)?;
                    Some((
                        slot,
                        system,
                        attachment.marker.clone(),
                        attachment.primary_scale,
                    ))
                })
                .collect();
            if systems.is_empty() {
                continue;
            }
            let model = object
                .model
                .as_ref()
                .and_then(|model| level.model_index(model));
            carried.insert(object.tag.clone(), (systems, model));
        }
        Self {
            carried,
            index,
            free: Vec::new(),
            attached: BTreeMap::new(),
        }
    }

    /// The bitmaps the level's systems' particle states draw.
    pub(super) fn bitmaps(level: &Level) -> impl Iterator<Item = &TagRef> {
        level
            .particle_systems
            .iter()
            .flat_map(|system| &system.types)
            .flat_map(|kind| &kind.particle_states)
            .filter_map(|state| state.bitmap.as_ref())
    }

    /// One tick: the systems the effects' parts `fired` made begin, the
    /// objects of `simulation` carry theirs, and all of them run, meeting
    /// its world.
    pub(super) fn tick(
        &mut self,
        level: &Level,
        simulation: &mut Simulation,
        fired: &[FiredPart],
        random: &mut Random,
    ) {
        if self.index.is_empty() {
            return;
        }
        for part in fired {
            let Some(&system) = self.index.get(&part.tag) else {
                continue;
            };
            if self.free.len() >= MAX_SYSTEMS {
                self.free.remove(0);
            }
            let forward = part.rotation * Vec3::X;
            self.free.push(Running::new(
                level,
                system,
                (part.position, part.velocity, forward),
                false,
                random,
            ));
        }
        self.carry(level, simulation, random);
        let mut room = MAX_SYSTEM_PARTICLES.saturating_sub(self.count());
        let world: &Simulation = simulation;
        let collide = |from, motion| world.collide_point(from, motion);
        self.free
            .retain_mut(|running| running.tick(level, random, &mut room, collide));
        self.attached.retain(|_, running| {
            let live = running.tick(level, random, &mut room, collide);
            running.carried || live
        });
    }

    /// The systems the objects carry move with them; those of objects gone
    /// run out.
    fn carry(&mut self, level: &Level, simulation: &mut Simulation, random: &mut Random) {
        for running in self.attached.values_mut() {
            running.carried = false;
        }
        if self.carried.is_empty() {
            return;
        }
        let handles: HashMap<u32, ObjectHandle> = simulation
            .objects()
            .into_iter()
            .map(|handle| (handle.index, handle))
            .collect();
        let carried = &self.carried;
        let attached = &mut self.attached;
        simulation.visit_lit(|object| {
            let Some((systems, model)) = carried.get(object.tag) else {
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
            for (slot, system, marker, primary_scale) in systems {
                let (current, _) = marker_frames(model, &object, &rest, marker);
                let at = Vec3::from_array(current.position);
                let forward = Vec3::from_array(current.forward);
                let scale = match usize::try_from(primary_scale - 1) {
                    Ok(index) if index < 4 => object.functions.map_or(0.0, |values| {
                        if values.active[index] {
                            values.outgoing[index]
                        } else {
                            0.0
                        }
                    }),
                    _ => 1.0,
                };
                let running = attached.entry((handle, *slot)).or_insert_with(|| {
                    Running::new(level, *system, (at, moving, forward), true, random)
                });
                running.position = at;
                running.velocity = moving;
                running.forward = forward;
                running.scale = scale.clamp(0.0, 1.0);
                // (a system scaled to nothing makes nothing)
                running.carried = scale > 0.0;
            }
        });
    }

    /// The particles alive.
    fn count(&self) -> usize {
        self.free
            .iter()
            .chain(self.attached.values())
            .flat_map(|running| &running.emitters)
            .map(|emitter| emitter.bits.len())
            .sum()
    }

    pub(super) fn gather(
        &self,
        level: &Level,
        pictures: &Pictures,
        out: &mut Vec<ParticleSnapshot>,
    ) {
        for running in self.free.iter().chain(self.attached.values()) {
            if out.len() >= MAX_PARTICLES {
                return;
            }
            running.gather(level, pictures, out);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use opence_content::object::Attachment;
    use opence_content::{
        Group, ObjectDefinition, ObjectKind, ParticleTypeState, PointPhysics, RandomizedVariables,
        SpriteShader,
    };
    use opence_sim::objects::ObjectSpawn;
    use opence_sim::{Quat, Transform};

    fn tag(group: &[u8; 4], path: &str) -> TagRef {
        TagRef {
            group: Group(*group),
            path: path.into(),
        }
    }

    fn type_state(duration: f32, transition: f32, rate: f32) -> ParticleTypeState {
        ParticleTypeState {
            name: String::new(),
            duration: [duration; 2],
            transition: [transition; 2],
            flags: 0,
            variables: ParticleTypeVariables {
                randomized: RandomizedVariables {
                    scale: 1.0,
                    animation_rate: 1.0,
                    rotation_rate: 1.0,
                    color: [0.0; 4],
                },
                radius: 1.0,
                minimum_count: 0.0,
                creation_rate: rate,
            },
            creation_physics: -1,
            update_physics: -1,
            physics_constants: Vec::new(),
        }
    }

    fn particle_state(duration: f32, transition: f32, alpha: f32) -> ParticleState {
        ParticleState {
            name: String::new(),
            duration: [duration; 2],
            transition: [transition; 2],
            bitmap: None,
            sequence: 0,
            scale: [1.0; 2],
            animation_rate: [0.0; 2],
            rotation_rate: [0.0; 2],
            color: [[alpha, 1.0, 1.0, 1.0]; 2],
            radius: 1.0,
            point_physics: None,
            shader: SpriteShader::default(),
            primary_map_flags: 0,
            secondary_map: None,
        }
    }

    /// A kind of particle made at `rate` a second for a second, each
    /// lasting half a second and fading out over a quarter.
    fn kind(rate: f32, flags: u32) -> ParticleSystemType {
        ParticleSystemType {
            name: "puff".into(),
            flags,
            initial_count: 0,
            complex_sprite_render_mode: 0,
            sprite_render_mode: 0,
            radius: 0.5,
            creation_physics: 0,
            physics_flags: 0,
            physics_constants: vec![2.0],
            type_states: vec![type_state(1.0, 0.0, rate)],
            particle_states: vec![
                particle_state(0.5, 0.25, 1.0),
                particle_state(0.25, 0.0, 0.0),
            ],
        }
    }

    fn level(kind: ParticleSystemType) -> Level {
        let mut level = opence_content::devmap::level(None);
        level.particle_systems.push(ParticleSystem {
            tag: tag(b"pctl", "smoke"),
            point_physics: None,
            update_physics: 0,
            physics_flags: 0,
            physics_constants: Vec::new(),
            types: vec![kind],
        });
        level
    }

    fn fired(at: Vec3) -> FiredPart {
        FiredPart {
            effect: 1,
            event: 0,
            part: 0,
            tag: tag(b"pctl", "smoke"),
            position: at,
            rotation: Quat::IDENTITY,
            velocity: Vec3::ZERO,
            on: None,
            surface: None,
        }
    }

    fn live(systems: &Systems) -> usize {
        systems.count()
    }

    #[test]
    fn a_run_of_states_ends_loops_or_goes_back_and_forth() {
        let spans = |_: usize| ([1.0; 2], [0.0; 2]);
        let mut random = Random::default();
        let once = Looping {
            loops: false,
            back_and_forth: false,
        };
        let mut run = Run::new(&mut random, &spans);
        assert!(run.advance(1.5, &mut random, (2, once), &spans, |_, _| {}));
        assert_eq!(run.state, 1);
        assert!(!run.advance(1.0, &mut random, (2, once), &spans, |_, _| {}));

        let round = Looping {
            loops: true,
            back_and_forth: false,
        };
        let mut run = Run::new(&mut random, &spans);
        let seen: Vec<usize> = (0..4)
            .map(|_| {
                assert!(run.advance(1.0, &mut random, (2, round), &spans, |_, _| {}));
                run.state
            })
            .collect();
        assert_eq!(seen, [1, 0, 1, 0]);

        let back = Looping {
            loops: true,
            back_and_forth: true,
        };
        let mut run = Run::new(&mut random, &spans);
        let seen: Vec<usize> = (0..5)
            .map(|_| {
                assert!(run.advance(1.0, &mut random, (3, back), &spans, |_, _| {}));
                run.state
            })
            .collect();
        assert_eq!(seen, [1, 2, 1, 0, 1]);
    }

    #[test]
    fn a_run_becomes_its_next_state_over_the_transition() {
        let spans = |_: usize| ([1.0; 2], [0.5; 2]);
        let mut random = Random::default();
        let once = Looping {
            loops: false,
            back_and_forth: false,
        };
        let mut run = Run::new(&mut random, &spans);
        let mut entered = Vec::new();
        assert!(
            run.advance(1.25, &mut random, (2, once), &spans, |_, state| entered
                .push(state))
        );
        assert_eq!(entered, [1]);
        assert_eq!(run.state, 0);
        let Some((next, t)) = run.becoming() else {
            panic!("it is not becoming its next state");
        };
        assert_eq!(next, 1);
        assert!((t - 0.5).abs() < 1e-4, "{t}");
        assert!(run.advance(0.25, &mut random, (2, once), &spans, |_, _| {}));
        assert_eq!((run.state, run.next), (1, None));
    }

    #[test]
    fn an_effects_system_makes_particles_at_its_rate_and_runs_out() {
        let level = level(kind(45.0, 0));
        let mut systems = Systems::new(&level);
        let mut simulation = Simulation::empty();
        let mut random = Random::default();
        systems.tick(&level, &mut simulation, &[fired(Vec3::Z)], &mut random);
        // (1.5 a tick: one, then two, carrying the half)
        assert_eq!(live(&systems), 1);
        systems.tick(&level, &mut simulation, &[], &mut random);
        assert_eq!(live(&systems), 3);
        let mut out = Vec::new();
        systems.gather(&level, &Pictures::default(), &mut out);
        assert_eq!(out.len(), 3);
        assert_eq!(out[0].position, [0.0, 0.0, 1.0, 0.5]);
        assert_eq!(out[0].color, [1.0; 4]);
        // its emitter's one state over, it makes no more; its particles
        // fade into their second state and are gone
        for _ in 0..60 {
            systems.tick(&level, &mut simulation, &[], &mut random);
        }
        assert!(systems.free.is_empty());
    }

    #[test]
    fn a_particle_cross_fades_into_its_next_state() {
        let level = level(kind(30.0, 0));
        let mut systems = Systems::new(&level);
        let mut simulation = Simulation::empty();
        let mut random = Random::default();
        systems.tick(&level, &mut simulation, &[fired(Vec3::ZERO)], &mut random);
        // the first particle is two thirds of a second old: two thirds of
        // the way through its quarter becoming the second (alpha 0)
        for _ in 0..19 {
            systems.tick(&level, &mut simulation, &[], &mut random);
        }
        let mut out = Vec::new();
        systems.gather(&level, &Pictures::default(), &mut out);
        let first: Vec<f32> = out.iter().take(2).map(|one| one.color[3]).collect();
        assert_eq!(first.len(), 2);
        assert!((first[0] - 1.0 / 3.0).abs() < 0.01, "{first:?}");
        assert_eq!(first[1], 0.0);
    }

    #[test]
    fn particles_are_made_in_an_explosion_or_a_jet() {
        let mut random = Random::default();
        let made = |creation: i16, random: &mut Random| {
            let mut kind = kind(30.0, 0);
            kind.creation_physics = creation;
            kind.physics_constants = vec![2.0, 0.0];
            let mut emitter = Emitter {
                run: Some(Run::new(random, &type_spans(&kind))),
                owed: 0.0,
                bits: Vec::new(),
            };
            let mut room = 8;
            emitter.make(
                &kind,
                (Vec3::ZERO, Vec3::Z, Vec3::Y, 1.0),
                false,
                random,
                &mut room,
            );
            assert_eq!(room, 7);
            emitter.bits.first().map(|bit| bit.velocity)
        };
        // at the marker, with the system's velocity
        assert_eq!(made(0, &mut random), Some(Vec3::Z));
        // in a jet, along the way it faces at its speed
        assert_eq!(
            made(CREATION_JET, &mut random),
            Some(Vec3::Z + Vec3::Y * 2.0)
        );
        // in an explosion, any way at its speed
        let Some(velocity) = made(CREATION_EXPLOSION, &mut random) else {
            panic!("the explosion made nothing");
        };
        assert!(((velocity - Vec3::Z).length() - 2.0).abs() < 1e-4);
    }

    #[test]
    fn particles_die_on_the_ground_by_their_flags() {
        let mut kind = kind(30.0, ParticleSystemType::PARTICLES_DIE_ON_GROUND);
        kind.particle_states[0].point_physics = Some(PointPhysics {
            flags: PointPhysics::STRUCTURE_COLLISIONS,
            mass_over_radius_cubed: 1.0,
            density: 1.0,
            air_buoyancy: -1.0,
            ..PointPhysics::default()
        });
        let mut random = Random::default();
        let floor = |from: Vec3, motion: Vec3| {
            let to = from + motion;
            (from.z >= 0.0 && to.z < 0.0).then(|| PointHit {
                point: from + motion * (from.z / (from.z - to.z)),
                normal: Vec3::Z,
                fraction: from.z / (from.z - to.z),
            })
        };
        let mut falling = Bit::new(&kind, Vec3::new(0.0, 0.0, 10.0), Vec3::ZERO, &mut random);
        assert!(falling.tick(&kind, 1.0, &mut random, &mut { floor }));
        let mut landing = Bit::new(
            &kind,
            Vec3::new(0.0, 0.0, 0.01),
            Vec3::new(0.0, 0.0, -1.0),
            &mut random,
        );
        assert!(!landing.tick(&kind, 1.0, &mut random, &mut { floor }));
        // in the air, those that die in the air are gone at once
        let airy = ParticleSystemType {
            flags: ParticleSystemType::PARTICLES_DIE_IN_AIR,
            ..kind
        };
        assert!(!falling.tick(&airy, 1.0, &mut random, &mut { floor }));
    }

    #[test]
    fn an_object_carries_its_system_and_it_runs_out_when_it_is_gone() {
        let mut level = level(kind(30.0, ParticleSystemType::TYPE_STATES_LOOP));
        level.objects.push(ObjectDefinition {
            widgets: Vec::new(),
            tag: tag(b"scen", "torch"),
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
                tag: Some(tag(b"pctl", "smoke")),
                marker: String::new(),
                primary_scale: 0,
                change_color: 0,
            }],
        });
        let mut systems = Systems::new(&level);
        assert_eq!(systems.carried.len(), 1);
        let mut simulation = Simulation::empty();
        let spawned = simulation.spawn(ObjectSpawn {
            kind: ObjectKind::Scenery,
            tag: tag(b"scen", "torch"),
            transform: Transform {
                position: Vec3::new(3.0, 0.0, 0.0),
                ..Transform::default()
            },
            name: None,
            permutation: None,
            team: None,
        });
        let Ok(handle) = spawned else {
            panic!("the torch was not made");
        };
        let mut random = Random::default();
        // (its emitter loops: it goes on making them past its state)
        for _ in 0..45 {
            systems.tick(&level, &mut simulation, &[], &mut random);
        }
        assert_eq!(systems.attached.len(), 1);
        let mut out = Vec::new();
        systems.gather(&level, &Pictures::default(), &mut out);
        assert!(!out.is_empty());
        assert!(out.iter().all(|one| one.position[0] == 3.0));
        assert!(simulation.remove(handle));
        for _ in 0..60 {
            systems.tick(&level, &mut simulation, &[], &mut random);
        }
        assert!(systems.attached.is_empty());
    }
}
