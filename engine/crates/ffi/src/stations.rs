//! The test maps' stations (`station <name> [filter]`): tests the engine
//! runs by itself on the playground, devgulch
//! (`opence_content::devgulch::STATIONS`), or on the test map, devmap
//! (`opence_content::devmap::STATIONS`). A station drives the player as a
//! person at the keyboard would, for as many ticks as it needs, and writes
//! what it measured into the world (debug text) and into its results,
//! which `station` alone shows, with the set it placed or tests.
//!
//! Each places what it tests about its spot: so far ahead of the way it
//! faces and to its left ([`spot`]), on the ground found there.
//!
//! - `weapons`: fires each weapon at three targets for a few seconds: its
//!   shots, the damage they did, its heat; the equipment lies beside it.
//! - `vehicles`: drives each vehicle round the course (and devmap's
//!   ramps): its lap time, top speed and how high it went; each vehicle is
//!   also parked in a row beside the course.
//! - `animation`: plays each animation of a biped's graph in turn, named.
//! - `doors` (devmap): moves every device through its positions, back and
//!   forth.
//! - `physics`: drops a pile of crates onto the slope (devmap) or the
//!   ground, until they lie still.
//! - `particles` (devmap) and `effects` (devgulch): starts each effect of
//!   the level that makes particles, in turn, before the player: the most
//!   particles it kept alive.
//! - `ai` (devgulch): a squad of each biped that is not the player's,
//!   stood facing the player.
//! - `sounds` (devgulch): a sound of each sound class, each played once
//!   from a row ahead, and every sound scenery stood within hearing:
//!   whether each played, and was counted under its class.
//! - `shaders` (devgulch): an object of each shader class its model uses.
//! - `hud` (devgulch): gives each weapon that has a HUD in turn.
//!
//! The filter keeps the weapons, vehicles, animations, devices, effects,
//! bipeds or sounds whose name has it in it (and the shaders whose class
//! or path has).

use std::sync::Arc;

use opence_audio::{SoundPlayer, VoiceId};
use opence_content::devmap::{self, Station};
use opence_content::{devgulch, EffectPart, Level, ObjectKind, ShaderClass, TagRef};
use opence_dev::draw::{Color, DebugDraw, Layer};
use opence_dev::{Args, CommandError, Data, Output};
use opence_sim::animation::{AnimationPlayer, AnimationState};
use opence_sim::clock::TICKS_PER_SECOND;
use opence_sim::input::{Buttons, TickInput};
use opence_sim::objects::{ObjectHandle, ObjectSpawn};
use opence_sim::units::Radians;
use opence_sim::{Quat, Simulation, Transform, Vec3};
use serde_json::json;

use crate::effects::LevelEffects;
use crate::particles::{Census, LevelParticles};
use crate::{State, Wardrobe};

/// The most the player turns in a tick, in radians.
const TURN_MAX: f32 = 0.25;
/// The ticks a weapon is given to be ready before it fires.
const READY_TICKS: u32 = 45;
/// The ticks a weapon fires for.
const FIRE_TICKS: u32 = 3 * TICKS_PER_SECOND;
/// The ticks after a weapon stops, for its last shots to land.
const PAUSE_TICKS: u32 = TICKS_PER_SECOND;
/// The ticks the trigger is held for before it is let go for one (a
/// weapon that fires once a pull fires again).
const HOLD_TICKS: u32 = 8;
/// Where the weapons' targets stand: ahead of the station and to its left.
const TARGETS: [[f32; 2]; 3] = [[12.0, 3.0], [12.0, 0.0], [12.0, -3.0]];
/// How far to the left of the weapons station the equipment lies.
const EQUIPMENT_LEFT: f32 = 6.0;
/// How high on a target the weapons aim.
const CHEST: f32 = 0.45;
/// The longest a lap may take.
const LAP_TICKS: u32 = 40 * TICKS_PER_SECOND;
/// How near a corner of the course a vehicle has reached it.
const ARRIVED: f32 = 6.0;
/// The longest the player may take to leave a vehicle.
const LEAVE_TICKS: u32 = 3 * TICKS_PER_SECOND;
/// How far to the left of the vehicles station the vehicles are parked
/// (inside the course).
const PARKED_LEFT: f32 = 12.0;
/// How far ahead of the animation station its biped stands.
const DUMMY: f32 = 6.0;
/// The fewest and the most ticks an animation is shown for.
const ANIMATION_TICKS: [u32; 2] = [TICKS_PER_SECOND / 2, 10 * TICKS_PER_SECOND];
/// The animations' frames a second.
const FRAMES_PER_SECOND: u32 = 30;
/// How far ahead of the doors station the devices stand, in a row across.
const DEVICES_AHEAD: f32 = 10.0;
/// The ticks a device takes to go from closed to open and back.
const DEVICE_CYCLE_TICKS: u32 = 4 * TICKS_PER_SECOND;
/// How many times the devices go back and forth.
const DEVICE_CYCLES: u32 = 2;
/// The crates dropped.
const CRATES: usize = 12;
/// Half a crate's size.
const CRATE_HALF: f32 = 0.5;
/// The longest the crates may take to lie still.
const SETTLE_TICKS: u32 = 30 * TICKS_PER_SECOND;
/// The ticks each effect of the particle station is watched for.
const EFFECT_TICKS: u32 = 3 * TICKS_PER_SECOND;
/// How far ahead of the particle station its effects start, and how high.
const EFFECT_AT: [f32; 2] = [6.0, 1.0];
/// The places across the row the effects start at, in turn, and between
/// them.
const EFFECT_SLOTS: usize = 5;
const EFFECT_SPACING: f32 = 2.0;
/// How long each weapon's HUD is shown.
const HUD_TICKS: u32 = 2 * TICKS_PER_SECOND;
/// The bipeds of a squad.
const SQUAD: usize = 3;
/// The squads of a row.
const ROW: usize = 4;
/// The sounds of a row, how far apart they play, how far ahead the first
/// row is, and how high.
const SOUND_ROW: usize = 8;
const SOUND_SPACING: f32 = 1.0;
const SOUNDS_AHEAD: f32 = 3.0;
const SOUND_UP: f32 = 1.0;
/// How far ahead the sound scenery stands, in a row, and how far apart
/// (devgulch's teleporter loop is heard only within 2.5 world units).
const SCENERY_AHEAD: f32 = 1.0;
const SCENERY_SPACING: f32 = 1.0;
/// How near a voice plays to a sound scenery to be its.
const SCENERY_NEAR: f32 = 0.5;
/// The most times a sound that may be skipped is asked to play.
const SOUND_TRIES: u32 = 32;
/// How long the sound station listens.
const LISTEN_TICKS: u32 = 2 * TICKS_PER_SECOND;
/// How long what a station placed is labelled.
const LABEL_TICKS: u32 = 10 * TICKS_PER_SECOND;

/// A station running.
#[derive(Debug)]
pub(crate) struct Run {
    station: &'static Station,
    /// The level it runs on: another level stops it.
    level: Arc<Level>,
    drive: Drive,
    /// What it placed or tests: their tags' paths.
    set: Vec<String>,
    /// The ticks it has run.
    ticks: u32,
    /// What it measured, one entry for each thing it tested.
    results: Vec<Data>,
    /// What it is doing, or how it ended.
    status: String,
    done: bool,
}

#[derive(Debug)]
enum Drive {
    Weapons(Weapons),
    Vehicles(Vehicles),
    Animation(Animations),
    Doors(Doors),
    Physics(Physics),
    Particles(Particles),
    Huds(Huds),
    Sounds(Sounds),
    /// Placed and labelled, measured as they were placed.
    Placed(Placed),
}

impl Run {
    /// The input for the next tick: the station's, while it runs.
    pub(crate) fn input(&mut self, simulation: &Simulation) -> Option<TickInput> {
        if self.done {
            return None;
        }
        Some(match &mut self.drive {
            Drive::Weapons(weapons) => weapons.input(simulation),
            Drive::Vehicles(vehicles) => vehicles.input(simulation),
            Drive::Animation(_)
            | Drive::Doors(_)
            | Drive::Physics(_)
            | Drive::Particles(_)
            | Drive::Huds(_)
            | Drive::Sounds(_)
            | Drive::Placed(_) => face(simulation, self.station.facing),
        })
    }

    /// After a tick: what the tick did, and the station's next step.
    pub(crate) fn after_tick(
        &mut self,
        simulation: &mut Simulation,
        level: &Arc<Level>,
        draw: &mut DebugDraw,
    ) {
        if self.done {
            return;
        }
        if !Arc::ptr_eq(level, &self.level) || simulation.player().is_none() {
            self.finish("stopped: the world changed");
            return;
        }
        self.ticks = self.ticks.saturating_add(1);
        let mut report = Report {
            results: &mut self.results,
            draw,
            status: None,
        };
        let finished = match &mut self.drive {
            Drive::Weapons(weapons) => weapons.after_tick(simulation, level, &mut report),
            Drive::Vehicles(vehicles) => vehicles.after_tick(simulation, level, &mut report),
            Drive::Animation(animations) => animations.after_tick(simulation, &mut report),
            Drive::Doors(doors) => doors.after_tick(simulation, &mut report),
            Drive::Physics(physics) => physics.after_tick(simulation, &mut report),
            Drive::Particles(particles) => particles.after_tick(&mut report),
            Drive::Huds(huds) => huds.after_tick(simulation, &mut report),
            Drive::Sounds(sounds) => sounds.after_tick(&mut report),
            Drive::Placed(placed) => placed.after_tick(&mut report),
        };
        if let Some(status) = report.status {
            self.status = status;
        }
        if finished {
            let tested = self.results.len();
            self.finish(&format!("done: {tested} tested"));
        }
    }

    /// After the tick's effects and particles: the particle station starts
    /// its next effect, and counts the particles alive.
    pub(crate) fn after_effects(
        &mut self,
        level: &Level,
        effects: &mut LevelEffects,
        particles: &mut LevelParticles,
    ) {
        if let (false, Drive::Particles(station)) = (self.done, &mut self.drive) {
            station.after_effects(level, effects, particles);
        }
    }

    /// After the frame's sounds: the sound station plays its sounds, and
    /// sees what the player made of them.
    pub(crate) fn after_sounds(&mut self, player: &mut SoundPlayer) {
        if let (false, Drive::Sounds(station)) = (self.done, &mut self.drive) {
            station.after_sounds(player);
        }
    }

    fn finish(&mut self, status: &str) {
        self.done = true;
        self.status = format!("{} {status}", self.station.name);
        opence_dev::logbook::record(opence_dev::logbook::Level::Info, "station", &self.status);
    }

    /// What it measured, and whether it is done.
    fn summary(&self) -> Data {
        json!({
            "station": self.station.name,
            "done": self.done,
            "ticks": self.ticks,
            "status": self.status,
            "set": self.set,
            "results": self.results,
        })
    }
}

/// What a tick's step reports: results, shapes, its status.
struct Report<'a> {
    results: &'a mut Vec<Data>,
    draw: &'a mut DebugDraw,
    status: Option<String>,
}

impl Report<'_> {
    fn label(&mut self, at: Vec3, text: &str, ticks: u32) {
        self.draw
            .text(Layer::General, at.to_array(), text, Color::YELLOW, ticks);
    }
}

/// `station [name] [filter]`: alone, the stations of the test map
/// running (devgulch's if none is) and the one running; `station stop`
/// stops it.
pub(crate) fn command(state: &mut State, args: &Args) -> Result<Output, CommandError> {
    let name = args.string_or("name", "")?.to_owned();
    let filter = args.string_or("filter", "")?.to_lowercase();
    if name.is_empty() {
        let level = state
            .running
            .as_ref()
            .and_then(|running| running.level.as_deref());
        let running = state.shell.station.as_ref().map(Run::summary);
        let mut text = stations(level)
            .iter()
            .map(|station| format!("{:<10} {}", station.name, station.help))
            .collect::<Vec<_>>()
            .join("\n");
        if let Some(run) = &state.shell.station {
            text.push_str(&format!("\n{} ({} ticks)", run.status, run.ticks));
        }
        return Ok(Output::with_data(
            text,
            json!({ "stations": names(level), "run": running }),
        ));
    }
    if name == "stop" {
        return match state.shell.station.take() {
            Some(run) => Ok(Output::with_data(
                format!("stopped {}", run.station.name),
                run.summary(),
            )),
            None => Err(failed("no station is running")),
        };
    }
    let running = state
        .running
        .as_mut()
        .ok_or_else(|| failed("no world is running"))?;
    let level = running
        .level
        .clone()
        .ok_or_else(|| failed("the world has no level"))?;
    if ![devgulch::NAME, devmap::NAME].contains(&level.scenario.path.as_str()) {
        return Err(failed("the stations are on the test maps: `map devgulch`"));
    }
    let station = stations(Some(&level))
        .iter()
        .find(|station| station.name == name)
        .ok_or_else(|| {
            failed(format!(
                "no station `{name}`: there are {}",
                names(Some(&level)).join(", ")
            ))
        })?;
    if running.network.is_some() || running.campaign.is_some() {
        return Err(failed("a station runs without a network game or a mission"));
    }
    let simulation = &mut running.simulation;
    let player = simulation
        .player()
        .ok_or_else(|| failed("there is no player"))?;
    if player.seated().is_some() {
        return Err(failed("the player is seated: leave the vehicle first"));
    }
    let ground = devgulch::ground(&level, station.at).unwrap_or(0.0);
    let [x, y] = station.at;
    if !simulation.teleport_player(0, Vec3::new(x, y, ground + 0.1)) {
        return Err(failed("the player could not be moved to the station"));
    }
    let drive = match station.name {
        "weapons" => Drive::Weapons(Weapons::new(simulation, &level, station, &filter)),
        "vehicles" => Drive::Vehicles(Vehicles::new(simulation, &level, station, &filter)),
        "animation" => Drive::Animation(Animations::new(simulation, &level, station, &filter)),
        "doors" => Drive::Doors(Doors::new(simulation, &level, station, &filter)),
        "particles" | "effects" => Drive::Particles(Particles::new(&level, station, &filter)),
        "hud" => Drive::Huds(Huds::new(&level, &filter)),
        "ai" => Drive::Placed(Placed::squads(simulation, &level, station, &filter)),
        "sounds" => Drive::Sounds(Sounds::new(simulation, &level, station, &filter)),
        "shaders" => Drive::Placed(Placed::shaders(simulation, &level, station, &filter)),
        _ => Drive::Physics(Physics::new(simulation, &level, station)),
    };
    let count = drive.count();
    let run = Run {
        station,
        level,
        set: drive.set(),
        drive,
        ticks: 0,
        results: Vec::new(),
        status: format!("{} running: {count} to test", station.name),
        done: false,
    };
    let text = run.status.clone();
    let data = run.summary();
    state.shell.station = Some(run);
    Ok(Output::with_data(text, data))
}

/// The stations of `level`: devmap's on devmap, otherwise devgulch's.
fn stations(level: Option<&Level>) -> &'static [Station] {
    of_map(level.map_or(devgulch::NAME, |level| level.scenario.path.as_str()))
}

/// The stations of the map named `map`: devmap's on devmap, otherwise
/// devgulch's.
pub(crate) fn of_map(map: &str) -> &'static [Station] {
    if map == devmap::NAME {
        &devmap::STATIONS
    } else {
        &devgulch::STATIONS
    }
}

fn names(level: Option<&Level>) -> Vec<&'static str> {
    stations(level).iter().map(|station| station.name).collect()
}

/// The vehicles' course on `level`.
fn course(level: &Level) -> &'static [[f32; 2]] {
    if level.scenario.path == devmap::NAME {
        &devmap::COURSE
    } else {
        &devgulch::COURSE
    }
}

/// The point `ahead` of `station`, the way it faces, and `left` of it,
/// `up` over the ground of `level` there.
fn spot(station: &Station, level: &Level, [ahead, left]: [f32; 2], up: f32) -> Vec3 {
    let (sin, cos) = (libm::sinf(station.facing), libm::cosf(station.facing));
    let x = station.at[0] + ahead * cos - left * sin;
    let y = station.at[1] + ahead * sin + left * cos;
    let ground = devgulch::ground(level, [x, y]).unwrap_or(0.0);
    Vec3::new(x, y, ground + up)
}

/// The room an object of `tag` is given in a row.
fn width(level: &Level, tag: &TagRef) -> f32 {
    level.object(tag).map_or(2.0, |object| {
        (2.0 * object.bounding_radius).clamp(2.0, 12.0)
    })
}

/// Where each of `tags` stands in a row across, `ahead` of `station`,
/// centred on it, each given its [`width`].
fn row(station: &Station, level: &Level, tags: &[TagRef], ahead: f32, up: f32) -> Vec<Vec3> {
    let length: f32 = tags.iter().map(|tag| width(level, tag)).sum();
    let mut left = 0.5 * length;
    tags.iter()
        .map(|tag| {
            let room = width(level, tag);
            let at = spot(station, level, [ahead, left - 0.5 * room], up);
            left -= room;
            at
        })
        .collect()
}

fn failed(text: impl Into<String>) -> CommandError {
    CommandError::Failed(text.into())
}

impl Drive {
    /// How many things it tests.
    fn count(&self) -> usize {
        match self {
            Drive::Weapons(weapons) => weapons.weapons.len(),
            Drive::Vehicles(vehicles) => vehicles.vehicles.len(),
            Drive::Animation(animations) => animations.animations.len(),
            Drive::Doors(doors) => doors.devices.len(),
            Drive::Physics(_) => CRATES,
            Drive::Particles(particles) => particles.effects.len(),
            Drive::Huds(huds) => huds.weapons.len(),
            Drive::Sounds(sounds) => sounds.emitters.len() + sounds.scenery.len(),
            Drive::Placed(placed) => placed.results.len(),
        }
    }

    /// What it placed or tests: their tags' paths.
    fn set(&self) -> Vec<String> {
        let paths = |tags: &[TagRef]| tags.iter().map(|tag| tag.path.clone()).collect::<Vec<_>>();
        match self {
            Drive::Weapons(weapons) => {
                [paths(&weapons.weapons), paths(&weapons.equipment)].concat()
            }
            Drive::Vehicles(vehicles) => {
                [paths(&vehicles.vehicles), paths(&vehicles.parked)].concat()
            }
            Drive::Animation(animations) => paths(animations.tag.as_slice()),
            Drive::Doors(doors) => doors
                .devices
                .iter()
                .map(|device| device.tag.path.clone())
                .collect(),
            Drive::Physics(_) => vec!["crate".into(); CRATES],
            Drive::Particles(particles) => paths(&particles.effects),
            Drive::Huds(huds) => huds
                .weapons
                .iter()
                .map(|(weapon, _)| weapon.path.clone())
                .collect(),
            Drive::Sounds(sounds) => sounds
                .emitters
                .iter()
                .map(|emitter| &emitter.sound)
                .chain(sounds.scenery.iter().map(|scenery| &scenery.tag))
                .map(|tag| tag.path.clone())
                .collect(),
            Drive::Placed(placed) => placed.set.clone(),
        }
    }
}

/// The last part of a tag's path: its name.
fn short(tag: &TagRef) -> &str {
    tag.path.rsplit('\\').next().unwrap_or(&tag.path)
}

/// The level's definitions of `kinds` whose path has `filter` in it.
fn definitions(level: &Level, kinds: &[ObjectKind], filter: &str) -> Vec<TagRef> {
    level
        .objects
        .iter()
        .filter(|object| kinds.contains(&object.kind))
        .filter(|object| object.tag.path.to_lowercase().contains(filter))
        .map(|object| object.tag.clone())
        .collect()
}

/// Makes an object of `tag` at `position` facing `yaw`, dressed.
fn place(
    simulation: &mut Simulation,
    level: &Level,
    tag: &TagRef,
    position: Vec3,
    yaw: f32,
) -> Option<ObjectHandle> {
    let kind = level.object(tag)?.kind;
    let handle = simulation
        .spawn(ObjectSpawn {
            kind,
            tag: tag.clone(),
            transform: Transform {
                position,
                rotation: Quat::from_rotation_z(yaw),
                scale: 1.0,
            },
            name: None,
            permutation: None,
            team: None,
        })
        .ok()?;
    Wardrobe::default().dress(simulation, level, handle);
    Some(handle)
}

/// The player's own biped's tag.
fn player_biped(simulation: &Simulation) -> Option<TagRef> {
    let handle = simulation.player()?.handle();
    Some(simulation.object(handle)?.tag.clone())
}

/// The turn toward `yaw` and `pitch` from where the player looks, at most
/// [`TURN_MAX`] a tick.
fn turn_toward(simulation: &Simulation, yaw: f32, pitch: f32) -> (Radians, Radians) {
    let Some(player) = simulation.player() else {
        return (Radians(0.0), Radians(0.0));
    };
    let (Radians(now_yaw), Radians(now_pitch)) = player.look();
    let yaw = libm::remainderf(yaw - now_yaw, std::f32::consts::TAU).clamp(-TURN_MAX, TURN_MAX);
    let pitch = (pitch - now_pitch).clamp(-TURN_MAX, TURN_MAX);
    (Radians(yaw), Radians(pitch))
}

/// Standing still, turning to face `yaw`, looking level.
fn face(simulation: &Simulation, yaw: f32) -> TickInput {
    let (yaw, pitch) = turn_toward(simulation, yaw, 0.0);
    TickInput {
        yaw,
        pitch,
        ..TickInput::default()
    }
}

/// The yaw and pitch from `from` to `to`.
fn angles_to(from: Vec3, to: Vec3) -> (f32, f32) {
    let way = to - from;
    let flat = libm::hypotf(way.x, way.y);
    (libm::atan2f(way.y, way.x), libm::atan2f(way.z, flat))
}

/// The weapons station: each weapon given, aimed at the targets and fired.
#[derive(Debug)]
struct Weapons {
    weapons: Vec<TagRef>,
    /// The equipment laid beside the station.
    equipment: Vec<TagRef>,
    at: usize,
    phase: WeaponPhase,
    phase_ticks: u32,
    target: Option<TagRef>,
    targets: Vec<ObjectHandle>,
    /// Where the targets stand, the way they face, where the weapons aim
    /// and where what each did is written.
    targets_at: [Vec3; 3],
    target_facing: f32,
    aim: Vec3,
    label_at: Vec3,
    tally: Tally,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
enum WeaponPhase {
    Arm,
    Ready,
    Fire,
    Pause,
}

/// What a weapon did.
#[derive(Clone, Copy, Debug, Default)]
struct Tally {
    /// The ticks it fired in.
    shots: u32,
    damage: f32,
    heat: f32,
    kills: u32,
}

impl Weapons {
    fn new(simulation: &mut Simulation, level: &Level, station: &Station, filter: &str) -> Weapons {
        // (the equipment in a line ahead, to the left of the targets)
        let mut equipment = Vec::new();
        for tag in definitions(level, &[ObjectKind::Equipment], filter) {
            let ahead = 2.0 + 2.0 * equipment.len() as f32;
            let at = spot(station, level, [ahead, EQUIPMENT_LEFT], 0.2);
            if place(simulation, level, &tag, at, station.facing).is_some() {
                equipment.push(tag);
            }
        }
        Weapons {
            weapons: definitions(level, &[ObjectKind::Weapon], filter),
            equipment,
            at: 0,
            phase: WeaponPhase::Arm,
            phase_ticks: 0,
            target: player_biped(simulation),
            targets: Vec::new(),
            targets_at: TARGETS.map(|target| spot(station, level, target, 0.0)),
            target_facing: station.facing + std::f32::consts::PI,
            aim: spot(station, level, TARGETS[1], CHEST),
            label_at: spot(station, level, TARGETS[1], 1.2),
            tally: Tally::default(),
        }
    }

    fn input(&self, simulation: &Simulation) -> TickInput {
        let eyes = simulation
            .player()
            .map_or(Vec3::ZERO, |player| player.eyes());
        let (yaw, pitch) = angles_to(eyes, self.aim);
        let (yaw, pitch) = turn_toward(simulation, yaw, pitch);
        let firing =
            self.phase == WeaponPhase::Fire && !self.phase_ticks.is_multiple_of(HOLD_TICKS + 1);
        TickInput {
            buttons: if firing { Buttons::FIRE } else { Buttons(0) },
            yaw,
            pitch,
            ..TickInput::default()
        }
    }

    fn after_tick(
        &mut self,
        simulation: &mut Simulation,
        level: &Level,
        report: &mut Report<'_>,
    ) -> bool {
        self.phase_ticks += 1;
        if matches!(self.phase, WeaponPhase::Fire | WeaponPhase::Pause) {
            self.count(simulation);
        }
        match self.phase {
            WeaponPhase::Arm => return self.arm(simulation, level, report),
            WeaponPhase::Ready if self.phase_ticks >= READY_TICKS => {
                self.next(WeaponPhase::Fire);
            }
            WeaponPhase::Fire if self.phase_ticks >= FIRE_TICKS => {
                self.next(WeaponPhase::Pause);
            }
            WeaponPhase::Pause if self.phase_ticks >= PAUSE_TICKS => {
                self.record(report);
                self.at += 1;
                self.next(WeaponPhase::Arm);
            }
            _ => {}
        }
        false
    }

    fn next(&mut self, phase: WeaponPhase) {
        self.phase = phase;
        self.phase_ticks = 0;
    }

    /// Arms the next weapon that can be carried, with fresh targets; true
    /// when there are none left.
    fn arm(&mut self, simulation: &mut Simulation, level: &Level, report: &mut Report<'_>) -> bool {
        for target in self.targets.drain(..) {
            simulation.remove(target);
        }
        while let Some(weapon) = self.weapons.get(self.at) {
            if simulation.give_weapon(weapon) {
                if let Some(tag) = &self.target {
                    for at in self.targets_at {
                        if let Some(handle) = place(simulation, level, tag, at, self.target_facing)
                        {
                            self.targets.push(handle);
                        }
                    }
                }
                self.tally = Tally::default();
                report.status = Some(format!(
                    "weapons: {} ({}/{})",
                    short(weapon),
                    self.at + 1,
                    self.weapons.len()
                ));
                self.next(WeaponPhase::Ready);
                return false;
            }
            report.results.push(json!({
                "weapon": weapon.path,
                "error": "the player cannot carry it",
            }));
            self.at += 1;
        }
        true
    }

    /// The tick's shot, damage and heat.
    fn count(&mut self, simulation: &Simulation) {
        let Some(player) = simulation.player() else {
            return;
        };
        let own = player.handle();
        if let Some((_, state)) = player.weapon() {
            if self.phase == WeaponPhase::Fire
                && state.triggers.iter().any(|trigger| trigger.idle_ticks == 0)
            {
                self.tally.shots += 1;
            }
            self.tally.heat = self.tally.heat.max(state.heat);
        }
        for event in simulation.damage_events() {
            if event.owner == Some(own) && event.object != own {
                self.tally.damage += event.outcome.shield + event.outcome.body;
                self.tally.kills += u32::from(event.outcome.killed);
            }
        }
    }

    fn record(&mut self, report: &mut Report<'_>) {
        let Some(weapon) = self.weapons.get(self.at) else {
            return;
        };
        let Tally {
            shots,
            damage,
            heat,
            kills,
        } = self.tally;
        let seconds = FIRE_TICKS as f32 / TICKS_PER_SECOND as f32;
        report.label(
            self.label_at,
            &format!(
                "{}: {shots} shots, {damage:.2} damage, heat {heat:.2}, {kills} killed",
                short(weapon)
            ),
            3 * TICKS_PER_SECOND,
        );
        report.results.push(json!({
            "weapon": weapon.path,
            "shots": shots,
            "shots_a_second": shots as f32 / seconds,
            "damage": damage,
            "heat": heat,
            "kills": kills,
        }));
    }
}

/// The vehicles station: each vehicle driven round the course.
#[derive(Debug)]
struct Vehicles {
    vehicles: Vec<TagRef>,
    /// The vehicles parked in a row beside the course.
    parked: Vec<TagRef>,
    /// The course's corners, the first its start, and the way it starts.
    course: Vec<Vec3>,
    facing: f32,
    at: usize,
    phase: VehiclePhase,
    phase_ticks: u32,
    vehicle: Option<ObjectHandle>,
    /// The corner of the course it drives to next, from the second.
    corner: usize,
    last: Option<Vec3>,
    top_speed: f32,
    top_height: f32,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
enum VehiclePhase {
    Board,
    Drive,
    Leave,
}

impl Vehicles {
    fn new(
        simulation: &mut Simulation,
        level: &Level,
        station: &Station,
        filter: &str,
    ) -> Vehicles {
        let vehicles = definitions(level, &[ObjectKind::Vehicle], filter);
        // (parked in a line ahead, to the left of the station)
        let mut parked = Vec::new();
        let mut ahead = 4.0;
        for tag in &vehicles {
            let room = width(level, tag).max(3.0);
            let at = spot(station, level, [ahead + 0.5 * room, PARKED_LEFT], 0.3);
            ahead += room;
            if place(simulation, level, tag, at, station.facing).is_some() {
                parked.push(tag.clone());
            }
        }
        let course = course(level)
            .iter()
            .map(|&[x, y]| Vec3::new(x, y, devgulch::ground(level, [x, y]).unwrap_or(0.0)))
            .collect();
        Vehicles {
            vehicles,
            parked,
            course,
            facing: station.facing,
            at: 0,
            phase: VehiclePhase::Board,
            phase_ticks: 0,
            vehicle: None,
            corner: 1,
            last: None,
            top_speed: 0.0,
            top_height: 0.0,
        }
    }

    /// The corner driven to: the course's next, then its first again.
    fn goal(&self) -> Vec3 {
        self.course
            .get(self.corner % self.course.len().max(1))
            .copied()
            .unwrap_or(Vec3::ZERO)
    }

    fn input(&self, simulation: &Simulation) -> TickInput {
        let at = self
            .vehicle
            .and_then(|vehicle| simulation.transform(vehicle))
            .map_or(Vec3::ZERO, |transform| transform.position);
        match self.phase {
            VehiclePhase::Drive => {
                let (yaw, _) = angles_to(at, self.goal());
                let (yaw, pitch) = turn_toward(simulation, yaw, -0.1);
                TickInput {
                    forward: 127,
                    yaw,
                    pitch,
                    ..TickInput::default()
                }
            }
            VehiclePhase::Leave => TickInput {
                buttons: Buttons::ACTION,
                ..TickInput::default()
            },
            VehiclePhase::Board => TickInput::default(),
        }
    }

    fn next(&mut self, phase: VehiclePhase) {
        self.phase = phase;
        self.phase_ticks = 0;
    }

    fn after_tick(
        &mut self,
        simulation: &mut Simulation,
        level: &Level,
        report: &mut Report<'_>,
    ) -> bool {
        self.phase_ticks += 1;
        match self.phase {
            VehiclePhase::Board => return self.board(simulation, level, report),
            VehiclePhase::Drive => self.drive(simulation, report),
            VehiclePhase::Leave => {
                let seated = simulation
                    .player()
                    .is_some_and(|player| player.seated().is_some());
                if !seated {
                    if let Some(vehicle) = self.vehicle.take() {
                        simulation.remove(vehicle);
                    }
                    self.at += 1;
                    self.next(VehiclePhase::Board);
                } else if self.phase_ticks >= LEAVE_TICKS {
                    report.status = Some("vehicles: the player could not leave".into());
                    return true;
                }
            }
        }
        false
    }

    /// Places the next vehicle at the course's start, and on the tick
    /// after (when the world has taken it as a vehicle) seats the player
    /// in it; true when there are none left.
    fn board(
        &mut self,
        simulation: &mut Simulation,
        level: &Level,
        report: &mut Report<'_>,
    ) -> bool {
        let start = self.course.first().copied().unwrap_or(Vec3::ZERO) + Vec3::Z * 0.3;
        let facing = self.facing;
        while let Some(tag) = self.vehicles.get(self.at) {
            match self.vehicle.take() {
                None => {
                    if let Some(vehicle) = place(simulation, level, tag, start, facing) {
                        self.vehicle = Some(vehicle);
                        return false;
                    }
                }
                Some(vehicle) if simulation.enter_vehicle(vehicle, 0) => {
                    self.vehicle = Some(vehicle);
                    self.corner = 1;
                    self.last = None;
                    self.top_speed = 0.0;
                    self.top_height = 0.0;
                    report.status = Some(format!(
                        "vehicles: {} ({}/{})",
                        short(tag),
                        self.at + 1,
                        self.vehicles.len()
                    ));
                    self.next(VehiclePhase::Drive);
                    return false;
                }
                Some(vehicle) => {
                    simulation.remove(vehicle);
                }
            }
            report.results.push(json!({
                "vehicle": tag.path,
                "error": "the player cannot drive it",
            }));
            self.at += 1;
        }
        true
    }

    fn drive(&mut self, simulation: &Simulation, report: &mut Report<'_>) {
        let Some(at) = self
            .vehicle
            .and_then(|vehicle| simulation.transform(vehicle))
            .map(|transform| transform.position)
        else {
            self.lap(report, "the vehicle went");
            return;
        };
        if let Some(last) = self.last {
            self.top_speed = self
                .top_speed
                .max(at.distance(last) * TICKS_PER_SECOND as f32);
        }
        self.last = Some(at);
        self.top_height = self.top_height.max(at.z);
        let goal = self.goal();
        report.draw.line(
            Layer::General,
            at.to_array(),
            goal.to_array(),
            Color::GREEN,
            1,
        );
        if libm::hypotf(goal.x - at.x, goal.y - at.y) < ARRIVED {
            self.corner += 1;
            if self.corner > self.course.len() {
                self.lap(report, "");
                return;
            }
        }
        if self.phase_ticks >= LAP_TICKS {
            self.lap(report, "the lap took too long");
        }
    }

    fn lap(&mut self, report: &mut Report<'_>, error: &str) {
        let Some(tag) = self.vehicles.get(self.at) else {
            return;
        };
        let seconds = self.phase_ticks as f32 / TICKS_PER_SECOND as f32;
        let mut result = json!({
            "vehicle": tag.path,
            "lap_seconds": seconds,
            "corners": self.corner - 1,
            "top_speed": self.top_speed,
            "top_height": self.top_height,
        });
        let text = if error.is_empty() {
            format!(
                "{}: lap {seconds:.1} s, top speed {:.1}, {:.2} high",
                short(tag),
                self.top_speed,
                self.top_height
            )
        } else {
            result["error"] = json!(error);
            format!("{}: {error}", short(tag))
        };
        let start = self.course.first().copied().unwrap_or(Vec3::ZERO);
        report.label(start + Vec3::Z * 2.0, &text, 3 * TICKS_PER_SECOND);
        report.results.push(result);
        self.next(VehiclePhase::Leave);
    }
}

/// The animation station: a biped's animations, each in turn.
#[derive(Debug)]
struct Animations {
    /// The biped's tag, where it stands, and its handle.
    tag: Option<TagRef>,
    stand: Vec3,
    biped: Option<ObjectHandle>,
    player: Option<AnimationPlayer>,
    /// The animations shown, by their index in the graph.
    animations: Vec<u16>,
    at: usize,
    shown: u32,
}

impl Animations {
    fn new(
        simulation: &mut Simulation,
        level: &Level,
        station: &Station,
        filter: &str,
    ) -> Animations {
        let stand = spot(station, level, [DUMMY, 0.0], 0.0);
        let tag = player_biped(simulation);
        let biped = tag.as_ref().and_then(|tag| {
            place(
                simulation,
                level,
                tag,
                stand,
                station.facing + std::f32::consts::PI,
            )
        });
        let player = biped.and_then(|biped| simulation.component::<AnimationPlayer>(biped));
        let animations = player
            .as_ref()
            .map(|player| {
                (0..player.graph.animations.len())
                    .filter_map(|index| u16::try_from(index).ok())
                    .filter(|&index| {
                        player.graph.animations[usize::from(index)]
                            .name
                            .to_lowercase()
                            .contains(filter)
                    })
                    .collect()
            })
            .unwrap_or_default();
        Animations {
            tag: tag.filter(|_| biped.is_some()),
            stand,
            biped,
            player,
            animations,
            at: 0,
            shown: 0,
        }
    }

    fn after_tick(&mut self, simulation: &mut Simulation, report: &mut Report<'_>) -> bool {
        let (Some(biped), Some(player)) = (self.biped, &mut self.player) else {
            return true;
        };
        let Some(&index) = self.animations.get(self.at) else {
            return true;
        };
        let Some(animation) = player.graph.animations.get(usize::from(index)) else {
            return true;
        };
        let frames = u32::from(animation.frame_count.max(1));
        let ticks = (frames * TICKS_PER_SECOND / FRAMES_PER_SECOND)
            .clamp(ANIMATION_TICKS[0], ANIMATION_TICKS[1]);
        let frame = (self.shown * FRAMES_PER_SECOND / TICKS_PER_SECOND).min(frames - 1);
        player.state = AnimationState {
            animation: index,
            frame: u16::try_from(frame).unwrap_or(0),
        };
        let name = animation.name.clone();
        simulation.animate(biped, player.clone());
        if self.shown == 0 {
            let text = format!("{}/{} {name}", self.at + 1, self.animations.len());
            report.label(self.stand + Vec3::Z, &text, ticks);
            report.status = Some(format!("animation: {text}"));
        }
        self.shown += 1;
        if self.shown >= ticks {
            report.results.push(json!({
                "index": index,
                "name": name,
                "frames": frames,
            }));
            self.at += 1;
            self.shown = 0;
        }
        self.at >= self.animations.len()
    }
}

/// The doors station: every device, moved through its positions.
#[derive(Debug)]
struct Doors {
    devices: Vec<Device>,
    ticks: u32,
}

#[derive(Debug)]
struct Device {
    tag: TagRef,
    handle: ObjectHandle,
    player: AnimationPlayer,
    animation: u16,
    frames: u16,
}

impl Doors {
    fn new(simulation: &mut Simulation, level: &Level, station: &Station, filter: &str) -> Doors {
        let tags = definitions(level, &[ObjectKind::Machine, ObjectKind::Control], filter);
        let places = row(station, level, &tags, DEVICES_AHEAD, 0.0);
        let mut devices = Vec::new();
        for (tag, at) in tags.into_iter().zip(places) {
            let Some(handle) = place(simulation, level, &tag, at, 0.0) else {
                continue;
            };
            let Some(player) = simulation.component::<AnimationPlayer>(handle) else {
                continue;
            };
            let animations = &player.graph.animations;
            let animation = animations
                .iter()
                .position(|animation| animation.name.contains("position"))
                .unwrap_or(0);
            let Some(frames) = animations
                .get(animation)
                .map(|animation| animation.frame_count)
            else {
                continue;
            };
            devices.push(Device {
                tag,
                handle,
                animation: u16::try_from(animation).unwrap_or(0),
                frames,
                player,
            });
        }
        Doors { devices, ticks: 0 }
    }

    fn after_tick(&mut self, simulation: &mut Simulation, report: &mut Report<'_>) -> bool {
        // (closed to open and back: 0 to 1 to 0)
        let along = (self.ticks % DEVICE_CYCLE_TICKS) as f32 / DEVICE_CYCLE_TICKS as f32;
        let position = 1.0 - (2.0 * along - 1.0).abs();
        for device in &mut self.devices {
            let last = f32::from(device.frames.saturating_sub(1));
            device.player.state = AnimationState {
                animation: device.animation,
                frame: libm::roundf(position * last) as u16,
            };
            simulation.animate(device.handle, device.player.clone());
            if self.ticks.is_multiple_of(TICKS_PER_SECOND) {
                let at = simulation
                    .transform(device.handle)
                    .map_or(Vec3::ZERO, |transform| transform.position);
                let text = format!("{} {position:.2}", short(&device.tag));
                report.label(at + Vec3::Z * 2.0, &text, TICKS_PER_SECOND);
            }
        }
        self.ticks += 1;
        if self.ticks < DEVICE_CYCLES * DEVICE_CYCLE_TICKS && !self.devices.is_empty() {
            return false;
        }
        for device in &self.devices {
            let name = device
                .player
                .graph
                .animations
                .get(usize::from(device.animation))
                .map_or("", |animation| animation.name.as_str());
            report.results.push(json!({
                "device": device.tag.path,
                "animation": name,
                "frames": device.frames,
            }));
        }
        true
    }
}

/// The physics station: crates dropped onto the slope.
#[derive(Debug)]
struct Physics {
    ticks: u32,
    /// Where how they lay is written.
    label_at: Vec3,
}

impl Physics {
    fn new(simulation: &mut Simulation, level: &Level, station: &Station) -> Physics {
        simulation.clear_crates();
        let slope = devmap::SLOPE;
        let on_slope = level.scenario.path == devmap::NAME;
        let half = Vec3::splat(CRATE_HALF);
        for at in 0..CRATES {
            // (a loose pile over the slope's upper half, or ahead of the
            // station, each turned)
            let row = (at / 4) as f32;
            let column = (at % 4) as f32;
            let across = 2.5 * (column - 1.5);
            let above = 3.0 + 1.5 * row + 0.5 * column;
            let position = if on_slope {
                let along = 0.55 * slope.length + 2.0 * row;
                Vec3::new(
                    slope.foot[0] + slope.rising[0] * along - slope.rising[1] * across,
                    slope.foot[1] + slope.rising[1] * along + slope.rising[0] * across,
                    slope.height + above,
                )
            } else {
                spot(station, level, [8.0 + 2.0 * row, across], above)
            };
            let turn = Quat::from_rotation_z(0.4 * at as f32) * Quat::from_rotation_x(0.3 * row);
            simulation.drop_crate(position, turn, half);
        }
        let label_at = if on_slope {
            let [x, y] = slope.foot;
            Vec3::new(x, y, 3.0)
        } else {
            spot(station, level, [8.0, 0.0], 3.0)
        };
        Physics { ticks: 0, label_at }
    }

    fn after_tick(&mut self, simulation: &Simulation, report: &mut Report<'_>) -> bool {
        self.ticks += 1;
        let crates = simulation.crates();
        for one in crates.all() {
            let rotation = one.pose.rotation;
            let axes = [Vec3::X, Vec3::Y, Vec3::Z].map(|axis| (rotation * axis).to_array());
            let color = if one.resting() {
                Color::GREEN
            } else {
                Color::YELLOW
            };
            report.draw.oriented_box(
                Layer::General,
                one.pose.position.to_array(),
                axes,
                one.half_extents.to_array(),
                color,
                1,
            );
        }
        let settled = !crates.moving();
        if !settled && self.ticks < SETTLE_TICKS {
            return false;
        }
        let seconds = self.ticks as f32 / TICKS_PER_SECOND as f32;
        for one in crates.all() {
            report.results.push(json!({
                "position": one.pose.position.to_array(),
                "resting": one.resting(),
            }));
        }
        let text = if settled {
            format!("{} crates still after {seconds:.1} s", crates.all().len())
        } else {
            format!("the crates still move after {seconds:.1} s")
        };
        report.label(self.label_at, &text, 5 * TICKS_PER_SECOND);
        report.status = Some(format!("physics: {text}"));
        true
    }
}

/// The particle station: each effect that makes particles, started in
/// turn and watched.
#[derive(Debug)]
struct Particles {
    /// The station's name, which its status starts with.
    name: &'static str,
    effects: Vec<TagRef>,
    /// The particle systems each effect surely begins, and its emitters
    /// that surely emit.
    expected: Vec<[usize; 2]>,
    /// The places across the row the effects start at, in turn.
    slots: Vec<Vec3>,
    /// The effect watched.
    at: usize,
    /// The ticks it has been watched.
    ticks: u32,
    /// The effect to start after this tick's effects, and where.
    start: Option<(TagRef, Vec3)>,
    /// The particles alive after the last tick.
    alive: usize,
    /// The most alive while the effect has been watched.
    peak: usize,
    /// What the effect asked of the particles and made, so far.
    census: Census,
}

/// The particle systems an effect's parts surely begin: those of its
/// events that are never skipped, made in the air and violently, as the
/// simulation makes parts.
fn systems_expected(level: &Level, effect: &TagRef) -> usize {
    level.effect(effect).map_or(0, |effect| {
        effect
            .events
            .iter()
            .filter(|event| event.skip_fraction <= 0.0)
            .flat_map(|event| &event.parts)
            .filter(|part| {
                !matches!(part.create_in, EffectPart::IN_WATER | EffectPart::IN_SPACE)
                    && part.create_in_mode != EffectPart::NOT_VIOLENT
                    && level.particle_system(&part.tag).is_some()
            })
            .count()
    })
}

/// The emitters of an effect's events that are never skipped that surely
/// emit: made here, of a particle the level has, asking for some.
fn emitters_expected(level: &Level, effect: &TagRef) -> usize {
    level.effect(effect).map_or(0, |effect| {
        effect
            .events
            .iter()
            .filter(|event| event.skip_fraction <= 0.0)
            .flat_map(|event| &event.particles)
            .filter(|emitter| {
                crate::particles::made_here(emitter)
                    && emitter.count.iter().any(|count| *count > 0)
                    && level
                        .particles
                        .iter()
                        .any(|particle| particle.tag == emitter.particle)
            })
            .count()
    })
}

impl Particles {
    /// The level's effects whose path has `filter` in it and whose events
    /// emit particles or make particle systems.
    fn new(level: &Level, station: &'static Station, filter: &str) -> Particles {
        let effects = level
            .effects
            .iter()
            .filter(|effect| effect.tag.path.to_lowercase().contains(filter))
            .filter(|effect| {
                effect.events.iter().any(|event| {
                    !event.particles.is_empty()
                        || event
                            .parts
                            .iter()
                            .any(|part| level.particle_system(&part.tag).is_some())
                })
            })
            .map(|effect| effect.tag.clone())
            .collect::<Vec<_>>();
        let expected = effects
            .iter()
            .map(|effect| {
                [
                    systems_expected(level, effect),
                    emitters_expected(level, effect),
                ]
            })
            .collect();
        let slots = (0..EFFECT_SLOTS)
            .map(|slot| {
                let left = EFFECT_SPACING * (slot as f32 - 0.5 * (EFFECT_SLOTS - 1) as f32);
                spot(station, level, [EFFECT_AT[0], left], EFFECT_AT[1])
            })
            .collect();
        Particles {
            name: station.name,
            effects,
            expected,
            slots,
            at: 0,
            ticks: 0,
            start: None,
            alive: 0,
            peak: 0,
            census: Census::default(),
        }
    }

    fn after_tick(&mut self, report: &mut Report<'_>) -> bool {
        let Some(effect) = self.effects.get(self.at) else {
            return true;
        };
        if self.ticks == 0 {
            let at = self
                .slots
                .get(self.at % self.slots.len().max(1))
                .copied()
                .unwrap_or(Vec3::ZERO);
            self.start = Some((effect.clone(), at));
            self.peak = 0;
            report.label(at + Vec3::Z * 2.0, short(effect), EFFECT_TICKS);
            report.status = Some(format!(
                "{}: {}/{} {}",
                self.name,
                self.at + 1,
                self.effects.len(),
                short(effect)
            ));
        }
        self.peak = self.peak.max(self.alive);
        self.ticks += 1;
        if self.ticks < EFFECT_TICKS {
            return false;
        }
        let census = &self.census;
        let [systems, emitters] = self.expected.get(self.at).copied().unwrap_or_default();
        report.results.push(json!({
            "effect": effect.path,
            "particles": self.peak,
            "planned": census.planned,
            "made": census.made,
            "missing": census.missing,
            "systems": census.systems,
            "systems expected": systems,
            "emitters expected": emitters,
        }));
        self.ticks = 0;
        self.at += 1;
        self.at >= self.effects.len()
    }

    fn after_effects(
        &mut self,
        level: &Level,
        effects: &mut LevelEffects,
        particles: &mut LevelParticles,
    ) {
        if let Some((effect, at)) = self.start.take() {
            // (the last effect's particles go, so that each is counted alone)
            particles.clear();
            effects.start(level, &effect, at, Vec3::Z);
        }
        self.alive = particles.alive();
        self.census = particles.census().clone();
    }
}

/// The HUD station: each weapon that has a HUD, given in turn and held.
#[derive(Debug)]
struct Huds {
    /// Each weapon, and its HUD.
    weapons: Vec<(TagRef, TagRef)>,
    at: usize,
    ticks: u32,
}

impl Huds {
    fn new(level: &Level, filter: &str) -> Huds {
        let weapons = definitions(level, &[ObjectKind::Weapon], filter)
            .into_iter()
            .filter_map(|tag| {
                let hud = level.object(&tag)?.weapon.as_ref()?.hud_interface.clone()?;
                Some((tag, hud))
            })
            .collect();
        Huds {
            weapons,
            at: 0,
            ticks: 0,
        }
    }

    fn after_tick(&mut self, simulation: &mut Simulation, report: &mut Report<'_>) -> bool {
        let Some((weapon, hud)) = self.weapons.get(self.at) else {
            return true;
        };
        if self.ticks == 0 {
            if !simulation.give_weapon(weapon) {
                report.results.push(json!({
                    "weapon": weapon.path,
                    "hud": hud.path,
                    "error": "the player cannot carry it",
                }));
                self.at += 1;
                return self.at >= self.weapons.len();
            }
            report.status = Some(format!(
                "hud: {} ({}/{}) {}",
                short(weapon),
                self.at + 1,
                self.weapons.len(),
                short(hud)
            ));
        }
        self.ticks += 1;
        if self.ticks < HUD_TICKS {
            return false;
        }
        report.results.push(json!({
            "weapon": weapon.path,
            "hud": hud.path,
        }));
        self.ticks = 0;
        self.at += 1;
        self.at >= self.weapons.len()
    }
}

/// The sound station: a sound of each sound class the level has, each
/// played once from a row ahead (nearer, within half its maximum
/// distance, for one heard only near), and every sound scenery stood within
/// hearing; what the player made of each, by its census.
#[derive(Debug)]
struct Sounds {
    emitters: Vec<Emitter>,
    scenery: Vec<Scenery>,
    /// The classes with no sound that has anything to play.
    unplayed: Vec<u16>,
    labels: Vec<(Vec3, String)>,
    /// The emitters have played.
    played: bool,
    /// The ticks since they played.
    listened: u32,
}

/// A sound class's emitter.
#[derive(Debug)]
struct Emitter {
    class: u16,
    sound: TagRef,
    at: Vec3,
    /// Its sound plays every time it is asked; one that may be skipped is
    /// asked again at full scale, up to [`SOUND_TRIES`] times.
    plain: bool,
    /// The times it was asked.
    tries: u32,
    /// The voice it played on.
    voice: Option<VoiceId>,
    /// Why it did not play: culled, failed, or not in the player's bank.
    dropped: Option<&'static str>,
    /// Its voice was counted under its class.
    counted: bool,
}

/// A sound scenery stood within hearing.
#[derive(Debug)]
struct Scenery {
    tag: TagRef,
    at: Vec3,
    /// A voice played where it stands.
    heard: bool,
}

impl Sounds {
    fn new(simulation: &mut Simulation, level: &Level, station: &Station, filter: &str) -> Sounds {
        let mut classes: Vec<u16> = level.sounds.iter().map(|sound| sound.class).collect();
        classes.sort_unstable();
        classes.dedup();
        let mut sounds = Sounds {
            emitters: Vec::new(),
            scenery: Vec::new(),
            unplayed: Vec::new(),
            labels: Vec::new(),
            played: false,
            listened: 0,
        };
        for class in classes {
            let name = opence_audio::class(i32::from(class)).name;
            let kept: Vec<_> = level
                .sounds
                .iter()
                .filter(|sound| {
                    sound.class == class
                        && (name.contains(filter) || sound.tag.path.to_lowercase().contains(filter))
                })
                .collect();
            if kept.is_empty() {
                continue;
            }
            // (one that plays every time, else the least often skipped)
            let plain = kept.iter().copied().find(|sound| plays_every_time(sound));
            let Some(sound) = plain.or_else(|| {
                kept.iter()
                    .copied()
                    .filter(|sound| has_samples(sound))
                    .min_by(|a, b| a.skip_fraction.total_cmp(&b.skip_fraction))
            }) else {
                sounds.unplayed.push(class);
                continue;
            };
            let at = sounds.emitters.len();
            let ahead = SOUNDS_AHEAD + SOUND_SPACING * (at / SOUND_ROW) as f32;
            let left = SOUND_SPACING * ((at % SOUND_ROW) as f32 - 0.5 * (SOUND_ROW - 1) as f32);
            let row = spot(station, level, [ahead, left], SOUND_UP);
            // (a sound heard only near, a first-person one, plays nearer)
            let near = 0.5 * SoundPlayer::distances(sound)[1];
            let player = spot(station, level, [0.0, 0.0], SOUND_UP);
            let here = if row.distance(player) > near {
                player + (row - player).normalize_or_zero() * near
            } else {
                row
            };
            sounds.labels.push((here, name.to_owned()));
            sounds.emitters.push(Emitter {
                class,
                sound: sound.tag.clone(),
                at: here,
                plain: plain.is_some(),
                tries: 0,
                voice: None,
                dropped: None,
                counted: false,
            });
        }
        let scenery = definitions(level, &[ObjectKind::SoundScenery], filter);
        let count = scenery.len();
        for (at, tag) in scenery.into_iter().enumerate() {
            let left = SCENERY_SPACING * (at as f32 - 0.5 * count.saturating_sub(1) as f32);
            let here = spot(station, level, [SCENERY_AHEAD, left], 0.5);
            if let Some(handle) = place(simulation, level, &tag, here, station.facing) {
                // (the object stands where the ground lets it)
                let at = simulation
                    .transform(handle)
                    .map_or(here, |transform| transform.position);
                sounds.labels.push((at + Vec3::Z, short(&tag).to_owned()));
                sounds.scenery.push(Scenery {
                    tag,
                    at,
                    heard: false,
                });
            }
        }
        sounds
    }

    fn after_sounds(&mut self, player: &mut SoundPlayer) {
        if !self.played {
            self.played = true;
            for emitter in &mut self.emitters {
                let Some(index) = player.sound_index(&emitter.sound.path) else {
                    emitter.dropped = Some("not in the bank");
                    continue;
                };
                // (a sound silent at no scale, an impact's say, is asked at
                // full scale)
                let source = opence_audio::Source {
                    position: Some(emitter.at.to_array()),
                    scale: if emitter.plain { 0.0 } else { 1.0 },
                    ..opence_audio::Source::default()
                };
                let tries = if emitter.plain { 1 } else { SOUND_TRIES };
                let mut before = dropped(player, emitter.class);
                while emitter.voice.is_none() && emitter.tries < tries {
                    before = dropped(player, emitter.class);
                    emitter.tries += 1;
                    emitter.voice = player.impulse(index, &source);
                }
                if emitter.voice.is_none() {
                    let after = dropped(player, emitter.class);
                    emitter.dropped = Some(if after[1] > before[1] {
                        "failed"
                    } else {
                        "culled"
                    });
                }
            }
        }
        let playing = player.census().playing();
        for emitter in &mut self.emitters {
            emitter.counted |= playing
                .iter()
                .any(|one| Some(one.voice) == emitter.voice && one.class == emitter.class);
        }
        for scenery in &mut self.scenery {
            scenery.heard |= playing.iter().any(|one| {
                one.position
                    .is_some_and(|at| Vec3::from_array(at).distance(scenery.at) < SCENERY_NEAR)
            });
        }
    }

    fn after_tick(&mut self, report: &mut Report<'_>) -> bool {
        for (at, text) in self.labels.drain(..) {
            report.label(at, &text, LABEL_TICKS);
        }
        if !self.played {
            return false;
        }
        self.listened = self.listened.saturating_add(1);
        if self.listened < LISTEN_TICKS {
            return false;
        }
        for emitter in &self.emitters {
            report.results.push(json!({
                "class": opence_audio::class(i32::from(emitter.class)).name,
                "sound": emitter.sound.path,
                "played": emitter.voice.is_some(),
                "plain": emitter.plain,
                "tries": emitter.tries,
                "counted": emitter.counted,
                "dropped": emitter.dropped,
            }));
        }
        for &class in &self.unplayed {
            report.results.push(json!({
                "unplayed_class": opence_audio::class(i32::from(class)).name,
            }));
        }
        for scenery in &self.scenery {
            report.results.push(json!({
                "sound_scenery": scenery.tag.path,
                "heard": scenery.heard,
            }));
        }
        true
    }
}

/// A sound that plays whenever it is asked to: no skip fraction, no
/// promotion, each pitch range with permutations, none of them skipped.
fn plays_every_time(sound: &opence_content::Sound) -> bool {
    sound.zero.gain > 0.0
        && sound.skip_fraction == 0.0
        && sound.promotion_count == 0
        && has_samples(sound)
        && sound.pitch_ranges.iter().all(|range| {
            range
                .permutations
                .iter()
                .all(|permutation| permutation.skip_fraction == 0.0)
        })
}

/// A sound with something to play: each pitch range has permutations.
fn has_samples(sound: &opence_content::Sound) -> bool {
    !sound.pitch_ranges.is_empty()
        && sound
            .pitch_ranges
            .iter()
            .all(|range| !range.permutations.is_empty())
}

/// A class's sounds culled and failed so far.
fn dropped(player: &mut SoundPlayer, class: u16) -> [u32; 2] {
    player
        .census()
        .counts()
        .iter()
        .find(|count| count.class == class)
        .map_or([0, 0], |count| [count.culled, count.failed])
}

/// A station whose set is placed when it starts and stands to be looked
/// at: what each is is written over it, and its results are what was
/// placed.
#[derive(Debug, Default)]
struct Placed {
    set: Vec<String>,
    results: Vec<Data>,
    labels: Vec<(Vec3, String)>,
}

impl Placed {
    fn add(&mut self, tag: &TagRef, result: Data, label: (Vec3, String)) {
        self.set.push(tag.path.clone());
        self.results.push(result);
        self.labels.push(label);
    }

    /// A squad of each biped that is not the player's, in rows ahead,
    /// facing the player.
    fn squads(
        simulation: &mut Simulation,
        level: &Level,
        station: &Station,
        filter: &str,
    ) -> Placed {
        let own = player_biped(simulation);
        let facing = station.facing + std::f32::consts::PI;
        let mut placed = Placed::default();
        let bipeds = definitions(level, &[ObjectKind::Biped], filter);
        for tag in bipeds.iter().filter(|&tag| Some(tag) != own.as_ref()) {
            let at = placed.results.len();
            let ahead = 8.0 + 5.0 * (at / ROW) as f32;
            let left = 5.0 * ((at % ROW) as f32 - 0.5 * (ROW - 1) as f32);
            let mut stood = 0;
            for one in 0..SQUAD {
                let beside = 1.2 * (one as f32 - 0.5 * (SQUAD - 1) as f32);
                let here = spot(station, level, [ahead, left + beside], 0.0);
                stood += usize::from(place(simulation, level, tag, here, facing).is_some());
            }
            if stood > 0 {
                let label = spot(station, level, [ahead, left], 2.5);
                let result = json!({ "biped": tag.path, "squad": stood });
                placed.add(tag, result, (label, short(tag).to_owned()));
            }
        }
        placed
    }

    /// For each shader class, an object whose model uses a shader of it
    /// (scenery first), in a row ahead.
    fn shaders(
        simulation: &mut Simulation,
        level: &Level,
        station: &Station,
        filter: &str,
    ) -> Placed {
        let mut objects: Vec<_> = level.objects.iter().collect();
        objects.sort_by_key(|object| object.kind != ObjectKind::Scenery);
        let mut found: Vec<(&'static str, TagRef, TagRef)> = Vec::new();
        for object in objects {
            let Some(model) = object
                .model
                .as_ref()
                .and_then(|model| level.model_index(model))
                .and_then(|index| level.models.get(index))
            else {
                continue;
            };
            for shader in model
                .shaders
                .iter()
                .filter_map(|shader| shader.shader.as_ref())
            {
                let Some(class) = level.shader(shader).map(|shader| class_name(&shader.class))
                else {
                    continue;
                };
                let kept = class.contains(filter) || shader.path.to_lowercase().contains(filter);
                if kept && found.iter().all(|(seen, _, _)| *seen != class) {
                    found.push((class, shader.clone(), object.tag.clone()));
                }
            }
        }
        found.sort_by_key(|(class, _, _)| CLASSES.iter().position(|name| name == class));
        let tags: Vec<TagRef> = found.iter().map(|(_, _, object)| object.clone()).collect();
        let places = row(station, level, &tags, 8.0, 0.0);
        let mut placed = Placed::default();
        for ((class, shader, object), here) in found.into_iter().zip(places) {
            if place(simulation, level, &object, here, station.facing).is_some() {
                let result = json!({
                    "class": class,
                    "shader": shader.path,
                    "object": object.path,
                });
                let label = format!("{class}: {}", short(&shader));
                placed.add(&object, result, (here + Vec3::Z * 3.0, label));
            }
        }
        placed
    }

    fn after_tick(&mut self, report: &mut Report<'_>) -> bool {
        for (at, text) in self.labels.drain(..) {
            report.label(at, &text, LABEL_TICKS);
        }
        report.results.append(&mut self.results);
        true
    }
}

/// The shader classes' names, in their tags' order.
const CLASSES: [&str; 10] = [
    "environment",
    "transparent_chicago",
    "transparent_chicago_extended",
    "model",
    "transparent_generic",
    "glass",
    "meter",
    "plasma",
    "water",
    "other",
];

fn class_name(class: &ShaderClass) -> &'static str {
    match class {
        ShaderClass::Environment(_) => CLASSES[0],
        ShaderClass::TransparentChicago(_) => CLASSES[1],
        ShaderClass::TransparentChicagoExtended(_) => CLASSES[2],
        ShaderClass::Model(_) => CLASSES[3],
        ShaderClass::TransparentGeneric(_) => CLASSES[4],
        ShaderClass::Glass(_) => CLASSES[5],
        ShaderClass::Meter(_) => CLASSES[6],
        ShaderClass::Plasma(_) => CLASSES[7],
        ShaderClass::Water(_) => CLASSES[8],
        ShaderClass::Other(_) => CLASSES[9],
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn every_slot_of_the_effects_row_is_in_view() {
        // (the narrowest window is 4:3; the slots at the row's ends were
        // once out of the view's sides, and their particles drew nothing)
        let vertical = opence_sim::Camera::default().vertical_fov.0;
        let half_across = libm::atanf(libm::tanf(vertical / 2.0) * 4.0 / 3.0);
        let outer = EFFECT_SPACING * 0.5 * (EFFECT_SLOTS - 1) as f32;
        let [ahead, _] = EFFECT_AT;
        let off = libm::atanf(outer / ahead);
        assert!(
            off < 0.8 * half_across,
            "the outer slot is {off} across, the view {half_across}"
        );
    }

    fn near(at: Vec3, [x, y, z]: [f32; 3]) -> bool {
        (at.x - x).abs() < 1e-3 && (at.y - y).abs() < 1e-3 && (at.z - z).abs() < 1e-3
    }

    #[test]
    fn devmaps_spots_are_where_its_stations_placed_things_before() -> Result<(), &'static str> {
        let level = devmap::level(None);
        let weapons = devmap::station("weapons").ok_or("no weapons station")?;
        let before = [[-20.0, -18.0], [-20.0, -15.0], [-20.0, -12.0]];
        for (target, [x, y]) in TARGETS.into_iter().zip(before) {
            let at = spot(weapons, &level, target, 0.0);
            assert!(near(at, [x, y, 0.0]), "{at}");
        }
        let animation = devmap::station("animation").ok_or("no animation station")?;
        let at = spot(animation, &level, [DUMMY, 0.0], 0.0);
        assert!(near(at, [-14.0, -62.0, 0.0]), "{at}");
        let particles = devmap::station("particles").ok_or("no particles station")?;
        let [ahead, up] = EFFECT_AT;
        let at = spot(particles, &level, [ahead, 0.0], up);
        assert!(near(at, [-14.0, 15.0, 1.0]), "{at}");
        Ok(())
    }

    #[test]
    fn the_stations_and_course_are_the_test_maps_running() {
        let devmap = devmap::level(None);
        assert_eq!(stations(Some(&devmap)).len(), devmap::STATIONS.len());
        assert_eq!(course(&devmap), &devmap::COURSE);
        let mut other = devmap;
        other.scenario.path = devgulch::NAME.into();
        assert_eq!(stations(Some(&other)).len(), devgulch::STATIONS.len());
        assert_eq!(course(&other), &devgulch::COURSE);
        assert_eq!(stations(None).len(), devgulch::STATIONS.len());
    }

    #[test]
    fn a_row_is_centred_ahead_of_its_station() -> Result<(), &'static str> {
        let level = devmap::level(None);
        let doors = devmap::station("doors").ok_or("no doors station")?;
        let tag = TagRef {
            group: opence_content::Group(*b"scen"),
            path: "a".into(),
        };
        let tags = [tag.clone(), tag];
        let places = row(doors, &level, &tags, DEVICES_AHEAD, 0.0);
        // (facing -x: ahead is -x, left is -y; two of 2 each, centred)
        assert!(near(places[0], [-18.0, 44.0, 0.0]), "{}", places[0]);
        assert!(near(places[1], [-18.0, 46.0, 0.0]), "{}", places[1]);
        Ok(())
    }
}
