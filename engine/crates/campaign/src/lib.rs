//! A campaign mission: a scenario's scripts run over the simulation, with
//! the functions they call given by the mission (`source/hs/hs_library`'s
//! functions as the reference).
//!
//! The [`Mission`] is the scripts' world. Its objects are the simulation's;
//! a script holds an object as an index the mission gives it, which names
//! nothing once the object is gone. Devices, the cinematic camera, the HUD's
//! timer, recordings and custom animations keep their state here and move
//! on each tick. What only the player would see or hear (a fade, a sound, a
//! title, a nav point) is kept as an [`Event`] for the presentation to show.
//! The AI's functions keep what they are told for the AI to act on.

mod actors;
mod ai;
mod checkpoint;
mod cinematic;
mod devices;
mod game;
mod objects;
mod presented;
mod signatures;
mod units;

use std::collections::{BTreeMap, BTreeSet};
use std::sync::Arc;

use opence_content::{AnimationGraph, Level, ObjectDefinition, ValueType};
use opence_script::{Native, Registry, Runtime, Value, World};
use opence_sim::damage::Vitality;
use opence_sim::input::{Buttons, TickInput};
use opence_sim::objects::{is_placed, placement_spawn, Difficulty, ObjectHandle};
use opence_sim::{Simulation, Transform, Vec3};

pub use actors::{Actor, ActorState};
pub use ai::{AiOrders, AiReference};
pub use checkpoint::{Checkpoint, CheckpointError, CHECKPOINT_VERSION};
pub use cinematic::{CameraShot, Cinematic};
pub use devices::DeviceState;
pub use game::Flow;

/// The object lists a mission keeps at once: a list's index is reused once
/// this many more are made (as Halo's object list table is).
pub const OBJECT_LISTS: usize = 256;

/// The value a native's missing argument is taken as.
static VOID: Value = Value::Void;

/// Something a script did that only the player sees or hears: the
/// function it called and its arguments.
#[derive(Clone, Debug, PartialEq)]
pub struct Event {
    /// The function.
    pub function: &'static str,
    /// Its arguments, each of its parameter's type.
    pub arguments: Vec<Value>,
}

/// A recorded animation or a custom animation playing on a unit: the tick
/// it ends on, and whether the unit is removed then.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
struct Playing {
    ends: u64,
    delete: bool,
}

/// A campaign mission: the simulation, the scenario it plays and what the
/// scripts have told it.
pub struct Mission {
    sim: Simulation,
    level: Arc<Level>,
    difficulty: Difficulty,
    // (an object's index is its place here)
    handles: Vec<ObjectHandle>,
    indices: BTreeMap<ObjectHandle, i32>,
    lists: Vec<Vec<i32>>,
    next_list: usize,
    named: Vec<Option<ObjectHandle>>,
    // (each object name's placement, by the name's index)
    placements: Vec<Option<usize>>,
    devices: BTreeMap<ObjectHandle, DeviceState>,
    groups: Vec<f32>,
    graphs: BTreeMap<usize, Arc<AnimationGraph>>,
    recordings: BTreeMap<ObjectHandle, Playing>,
    animations: BTreeMap<ObjectHandle, Playing>,
    maximums: BTreeMap<ObjectHandle, [f32; 2]>,
    invulnerable: BTreeSet<ObjectHandle>,
    attachments: Vec<(ObjectHandle, ObjectHandle)>,
    ai: AiOrders,
    actors: Vec<Actor>,
    cinematic: Cinematic,
    flow: Flow,
    // (the buttons pressed since the scripts last reset them, and the look's
    // turns)
    actions: Buttons,
    looked: [bool; 4],
    input_enabled: bool,
    deathless: bool,
    random: u32,
    events: Vec<Event>,
    printed: Vec<String>,
    // (each tick's input from the start; the ticks the save being written
    // and the last save written cover)
    inputs: Vec<TickInput>,
    saving: Option<usize>,
    saved: Option<usize>,
    reverted: bool,
}

impl Mission {
    /// The mission `level` plays at `difficulty`, over `sim` (its players
    /// already made): each object its scenario places at the difficulty and
    /// that is made when the mission begins, made.
    pub fn new(mut sim: Simulation, level: Arc<Level>, difficulty: Difficulty) -> Self {
        let campaign = &level.scenario.campaign;
        let placements: Vec<Option<usize>> = campaign
            .object_names
            .iter()
            .map(|name| {
                level
                    .scenario
                    .placements
                    .iter()
                    .position(|p| p.name.as_deref() == Some(name.as_str()))
            })
            .collect();
        let groups: Vec<f32> = campaign
            .device_groups
            .iter()
            .map(|group| group.initial_value)
            .collect();
        let mut named = vec![None; placements.len()];
        let mut devices = BTreeMap::new();
        for (index, placement) in level.scenario.placements.iter().enumerate() {
            if !is_placed(placement, difficulty) || !placement.is_automatic() {
                continue;
            }
            let Ok(handle) = sim.spawn(placement_spawn(placement)) else {
                break; // (the table is full; Halo stops placing there too)
            };
            if let Some(name) = placements.iter().position(|&p| p == Some(index)) {
                named[name] = Some(handle);
            }
            if let Some(device) = &placement.device {
                devices.insert(handle, DeviceState::of(device, &groups));
            }
        }
        let mut mission = Self {
            sim,
            level,
            difficulty,
            handles: Vec::new(),
            indices: BTreeMap::new(),
            lists: Vec::new(),
            next_list: 0,
            named,
            placements,
            devices,
            groups,
            graphs: BTreeMap::new(),
            recordings: BTreeMap::new(),
            animations: BTreeMap::new(),
            maximums: BTreeMap::new(),
            invulnerable: BTreeSet::new(),
            attachments: Vec::new(),
            ai: AiOrders::default(),
            actors: Vec::new(),
            cinematic: Cinematic::default(),
            flow: Flow::default(),
            actions: Buttons::default(),
            looked: [false; 4],
            input_enabled: true,
            deathless: false,
            random: 0x2545_F491,
            events: Vec::new(),
            printed: Vec::new(),
            inputs: Vec::new(),
            saving: None,
            saved: None,
            reverted: false,
        };
        let difficulty = match difficulty {
            Difficulty::Easy => 0,
            Difficulty::Normal => 1,
            Difficulty::Hard => 2,
            Difficulty::Legendary => 3,
        };
        mission
            .sim
            .play_campaign(&mission.level.globals.difficulty, difficulty);
        // (the structure the first player starts over)
        mission.flow.bsp = mission
            .level
            .scenario
            .player_starts
            .first()
            .and_then(|start| start.structure_bsp)
            .and_then(|bsp| i16::try_from(bsp).ok())
            .unwrap_or(0);
        actors::begin(&mut mission);
        mission
    }

    /// The simulation.
    pub fn sim(&self) -> &Simulation {
        &self.sim
    }

    /// The simulation, to change.
    pub fn sim_mut(&mut self) -> &mut Simulation {
        &mut self.sim
    }

    /// The level played.
    pub fn level(&self) -> &Level {
        &self.level
    }

    /// The difficulty played at.
    pub fn difficulty(&self) -> Difficulty {
        self.difficulty
    }

    /// The object of scenario object name `name`, if it is in the world.
    pub fn named(&self, name: usize) -> Option<ObjectHandle> {
        self.named
            .get(name)
            .copied()
            .flatten()
            .filter(|&h| self.sim.object(h).is_some())
    }

    /// A device's state.
    pub fn device(&self, handle: ObjectHandle) -> Option<&DeviceState> {
        self.devices.get(&handle)
    }

    /// A device group's value.
    pub fn group(&self, group: usize) -> Option<f32> {
        self.groups.get(group).copied()
    }

    /// What the scripts have told the AI.
    pub fn ai(&self) -> &AiOrders {
        &self.ai
    }

    /// What the scripts have told the AI, to act on.
    pub fn ai_mut(&mut self) -> &mut AiOrders {
        &mut self.ai
    }

    /// The cinematic and its camera.
    pub fn cinematic(&self) -> &Cinematic {
        &self.cinematic
    }

    /// Whether the game is won or lost, saved, and where.
    pub fn flow(&self) -> &Flow {
        &self.flow
    }

    /// Whether the scripts let the player's input through.
    pub fn input_enabled(&self) -> bool {
        self.input_enabled
    }

    /// The events since they were last taken.
    pub fn take_events(&mut self) -> Vec<Event> {
        std::mem::take(&mut self.events)
    }

    /// The lines the scripts printed since they were last taken.
    pub fn take_printed(&mut self) -> Vec<String> {
        std::mem::take(&mut self.printed)
    }

    /// Runs one tick of the simulation with `input` and then moves on what
    /// the scripts started: devices toward their positions, recordings and
    /// animations to their ends, attached objects with their parents, the
    /// camera and the timers. Objects that cannot be hurt keep their
    /// vitality.
    pub fn step(&mut self, input: TickInput) {
        self.inputs.push(input);
        self.latch(input);
        let input = if self.input_enabled {
            input
        } else {
            TickInput::default()
        };
        let mut kept: Vec<(ObjectHandle, Vitality)> = self
            .invulnerable
            .iter()
            .filter_map(|&h| Some((h, self.sim.component::<Vitality>(h)?)))
            .collect();
        if self.deathless {
            for player in self.sim.players() {
                if let Some(vitality) = self.sim.component::<Vitality>(player.handle()) {
                    kept.push((player.handle(), vitality));
                }
            }
        }
        actors::think(self);
        self.sim.step(input);
        for (handle, vitality) in kept {
            self.sim.insert(handle, vitality);
        }
        let tick = self.sim.tick().0;
        devices::advance(self);
        for playing in [&mut self.recordings, &mut self.animations] {
            let ended: Vec<(ObjectHandle, bool)> = playing
                .iter()
                .filter(|(_, p)| p.ends <= tick)
                .map(|(&h, p)| (h, p.delete))
                .collect();
            for (handle, delete) in ended {
                playing.remove(&handle);
                if delete {
                    self.sim.remove(handle);
                }
            }
        }
        self.follow_attachments();
        self.cinematic.advance();
        self.flow.advance();
        if self.flow.saving == 0 {
            if let Some(ticks) = self.saving.take() {
                self.saved = Some(ticks);
            }
        }
    }

    /// The saved game of the last save written: the mission's start if
    /// none has been.
    pub fn checkpoint(&self) -> Checkpoint {
        let ticks = self.saved.unwrap_or(0).min(self.inputs.len());
        Checkpoint {
            scenario: self.level.scenario.path.clone(),
            difficulty: self.difficulty,
            inputs: self.inputs[..ticks].to_vec(),
        }
    }

    /// The saved game of every tick played so far.
    pub fn played(&self) -> Checkpoint {
        Checkpoint {
            scenario: self.level.scenario.path.clone(),
            difficulty: self.difficulty,
            inputs: self.inputs.clone(),
        }
    }

    /// Whether this mission was loaded from a save in the tick before.
    pub fn reverted(&self) -> bool {
        self.reverted
    }

    fn latch(&mut self, input: TickInput) {
        self.actions = Buttons(self.actions.0 | input.buttons.0);
        let (yaw, pitch) = (input.yaw.0, input.pitch.0);
        // (left, right, up, down)
        for (looked, turned) in
            self.looked
                .iter_mut()
                .zip([yaw > 0.0, yaw < 0.0, pitch > 0.0, pitch < 0.0])
        {
            *looked |= turned;
        }
    }

    fn follow_attachments(&mut self) {
        self.attachments.retain(|&(parent, child)| {
            self.sim.object(parent).is_some() && self.sim.object(child).is_some()
        });
        for &(parent, child) in &self.attachments {
            if let (Some(at), Some(mut transform)) =
                (self.sim.transform(parent), self.sim.transform(child))
            {
                transform.position = at.position;
                self.sim.insert(child, transform);
            }
        }
    }

    // (objects and lists, as the scripts hold them)

    /// The index a script holds `handle` by.
    fn index_of(&mut self, handle: ObjectHandle) -> i32 {
        if let Some(&index) = self.indices.get(&handle) {
            return index;
        }
        let index = i32::try_from(self.handles.len()).unwrap_or(-1);
        self.handles.push(handle);
        self.indices.insert(handle, index);
        index
    }

    /// The object a script's index names, if it is still in the world.
    fn handle(&self, index: i32) -> Option<ObjectHandle> {
        let handle = *self.handles.get(usize::try_from(index).ok()?)?;
        self.sim.object(handle).map(|_| handle)
    }

    /// A new object list of these objects' indices.
    fn new_list(&mut self, objects: Vec<i32>) -> i32 {
        let index = self.next_list;
        if index < self.lists.len() {
            self.lists[index] = objects;
        } else {
            self.lists.push(objects);
        }
        self.next_list = (index + 1) % OBJECT_LISTS;
        i32::try_from(index).unwrap_or(-1)
    }

    /// A new list of these objects.
    fn list_of(&mut self, handles: Vec<ObjectHandle>) -> i32 {
        let objects = handles.into_iter().map(|h| self.index_of(h)).collect();
        self.new_list(objects)
    }

    /// The objects of a script's list still in the world (none for no
    /// list).
    fn list(&self, index: i32) -> Vec<ObjectHandle> {
        usize::try_from(index)
            .ok()
            .and_then(|index| self.lists.get(index))
            .map(|list| list.iter().filter_map(|&o| self.handle(o)).collect())
            .unwrap_or_default()
    }

    /// The players' units.
    fn player_units(&self) -> Vec<ObjectHandle> {
        self.sim.players().iter().map(|p| p.handle()).collect()
    }

    /// The player whose unit `handle` is.
    fn player_of(&self, handle: ObjectHandle) -> Option<usize> {
        self.sim.players().iter().position(|p| p.handle() == handle)
    }

    /// Where an object is.
    fn position(&self, handle: ObjectHandle) -> Option<Vec3> {
        self.sim.transform(handle).map(|t| t.position)
    }

    /// Moves an object: a player's unit is stood there, any other is put
    /// there.
    fn place(&mut self, handle: ObjectHandle, transform: Transform) {
        match self.player_of(handle) {
            Some(player) => {
                self.sim.teleport_player(player, transform.position);
            }
            None => {
                self.sim.insert(handle, transform);
            }
        }
    }

    /// An object's definition.
    fn definition(&self, handle: ObjectHandle) -> Option<&ObjectDefinition> {
        let object = self.sim.object(handle)?;
        self.level.objects.iter().find(|d| d.tag == object.tag)
    }

    /// An object's most body and shield vitality.
    fn maximum_vitality(&self, handle: ObjectHandle) -> [f32; 2] {
        if let Some(&maximums) = self.maximums.get(&handle) {
            return maximums;
        }
        self.definition(handle)
            .and_then(|d| d.collision_model.as_ref())
            .and_then(|tag| self.level.collision_models.iter().find(|m| &m.tag == tag))
            .map_or([0.0, 0.0], |m| {
                [m.maximum_body_vitality, m.maximum_shield_vitality]
            })
    }

    /// A shared animation graph of the level's, by its place.
    fn graph(&mut self, index: usize) -> Option<Arc<AnimationGraph>> {
        if let Some(graph) = self.graphs.get(&index) {
            return Some(graph.clone());
        }
        let graph = Arc::new(self.level.animation_graphs.get(index)?.clone());
        self.graphs.insert(index, graph.clone());
        Some(graph)
    }

    /// The mission's next random number (a linear congruential generator,
    /// so a mission plays the same each time).
    fn next_random(&mut self) -> u32 {
        self.random = self
            .random
            .wrapping_mul(1_664_525)
            .wrapping_add(1_013_904_223);
        self.random >> 8
    }

    /// Keeps what a script did for the presentation.
    fn present(&mut self, function: &'static str, arguments: &[Value]) {
        self.events.push(Event {
            function,
            arguments: arguments.to_vec(),
        });
    }
}

impl World for Mission {
    fn object_of_name(&mut self, name: i32) -> i32 {
        match usize::try_from(name).ok().and_then(|n| self.named(n)) {
            Some(handle) => self.index_of(handle),
            None => -1,
        }
    }

    fn object_list_of(&mut self, objects: &[i32]) -> i32 {
        let objects = objects.iter().copied().filter(|&o| o >= 0).collect();
        self.new_list(objects)
    }

    fn object_list_of_ai(&mut self, ai: i32) -> i32 {
        let units = self.ai.units(AiReference::of(ai));
        self.list_of(units)
    }

    fn print(&mut self, text: &str) {
        self.printed.push(text.to_owned());
    }
}

/// The ticks a dead player lies before the mission reverts to its last
/// save (Halo's few seconds).
pub const REVERT_TICKS: u32 = 150;

/// A mission and its scripts, run together.
pub struct MissionScripts {
    mission: Mission,
    runtime: Runtime<Mission>,
    dead: u32,
}

impl MissionScripts {
    /// The mission with its scenario's scripts, none run yet.
    pub fn new(mut mission: Mission) -> Self {
        let scripts = mission.level.scenario.scripts.clone();
        let runtime = Runtime::new(scripts, registry(), &mut mission);
        Self {
            mission,
            runtime,
            dead: 0,
        }
    }

    /// `level`'s mission at `difficulty` from its start: the player at the
    /// first player start, the scripts not yet run.
    pub fn start(level: Arc<Level>, difficulty: Difficulty) -> Result<Self, CheckpointError> {
        let mut sim = Simulation::empty();
        sim.spawn_player(&level, 0)
            .map_err(|error| CheckpointError::Start(error.to_string()))?;
        Ok(Self::new(Mission::new(sim, level, difficulty)))
    }

    /// `level`'s mission as `checkpoint` saved it: started, and its inputs
    /// played again. Fails for a save of another scenario.
    pub fn load(level: Arc<Level>, checkpoint: &Checkpoint) -> Result<Self, CheckpointError> {
        if checkpoint.scenario != level.scenario.path {
            return Err(CheckpointError::Scenario(
                checkpoint.scenario.clone(),
                level.scenario.path.clone(),
            ));
        }
        let mut loaded = Self::start(level, checkpoint.difficulty)?;
        for &input in &checkpoint.inputs {
            loaded.play(input);
        }
        Ok(loaded)
    }

    /// Runs one tick: the scripts, then the mission with `input`. A revert
    /// the scripts asked for, or a player dead for [`REVERT_TICKS`], loads
    /// the last save (a mission whose save cannot load goes on).
    pub fn tick(&mut self, input: TickInput) {
        let reverted = self.mission.reverted;
        self.play(input);
        if reverted {
            self.mission.reverted = false;
        }
        let dead = self
            .mission
            .sim
            .players()
            .first()
            .is_some_and(|player| !self.mission.living(player.handle()));
        self.dead = if dead { self.dead + 1 } else { 0 };
        if self.mission.flow.revert || self.dead >= REVERT_TICKS {
            self.mission.flow.revert = false;
            self.dead = 0;
            let checkpoint = self.mission.checkpoint();
            if let Ok(loaded) = Self::load(Arc::clone(&self.mission.level), &checkpoint) {
                *self = loaded;
                self.mission.reverted = true;
            }
        }
    }

    fn play(&mut self, input: TickInput) {
        self.runtime.tick(&mut self.mission);
        self.mission.deathless = self
            .runtime
            .engine_global("cheat_deathless_player")
            .is_some_and(Value::as_boolean);
        self.mission.step(input);
    }

    /// The mission.
    pub fn mission(&self) -> &Mission {
        &self.mission
    }

    /// The mission, to change.
    pub fn mission_mut(&mut self) -> &mut Mission {
        &mut self.mission
    }

    /// The scripts running.
    pub fn runtime(&self) -> &Runtime<Mission> {
        &self.runtime
    }

    /// The scripts running, to change.
    pub fn runtime_mut(&mut self) -> &mut Runtime<Mission> {
        &mut self.runtime
    }
}

/// Every native, by its name: each module's.
fn natives() -> impl Iterator<Item = (&'static str, Native<Mission>)> {
    [
        ai::NATIVES,
        ai::PRESENTED,
        cinematic::NATIVES,
        devices::NATIVES,
        game::NATIVES,
        objects::NATIVES,
        presented::PRESENTED,
        units::NATIVES,
    ]
    .into_iter()
    .flatten()
    .copied()
}

/// The functions and globals the mission gives its scripts: each function
/// the stock scenarios call, with Halo's types.
pub fn registry() -> Registry<Mission> {
    let natives: BTreeMap<&str, Native<Mission>> = natives().collect();
    let mut registry = Registry::new();
    for (name, returns, parameters) in signatures::SIGNATURES {
        if let Some(&native) = natives.get(name) {
            registry.function(name, returns, parameters, native);
        }
    }
    registry.global(
        "cheat_deathless_player",
        ValueType::Boolean,
        Value::Boolean(false),
    );
    registry.global(
        "rasterizer_near_clip_distance",
        ValueType::Real,
        Value::Real(0.0625),
    );
    registry.global("weather", ValueType::Boolean, Value::Boolean(true));
    registry
}

/// The names of the functions the stock scenarios call that the mission
/// gives no native, and of the natives no stock scenario calls.
pub fn unmatched() -> (Vec<&'static str>, Vec<&'static str>) {
    let natives: BTreeSet<&str> = natives().map(|(name, _)| name).collect();
    let signed: BTreeSet<&str> = signatures::SIGNATURES.iter().map(|s| s.0).collect();
    (
        signed.difference(&natives).copied().collect(),
        natives.difference(&signed).copied().collect(),
    )
}

// (a native's arguments: each is already of its parameter's type)

fn arg(arguments: &[Value], at: usize) -> &Value {
    arguments.get(at).unwrap_or(&VOID)
}

fn real(arguments: &[Value], at: usize) -> f32 {
    arg(arguments, at).as_real()
}

fn boolean(arguments: &[Value], at: usize) -> bool {
    arg(arguments, at).as_boolean()
}

fn index(arguments: &[Value], at: usize) -> i32 {
    arg(arguments, at).as_index()
}

fn short(arguments: &[Value], at: usize) -> i16 {
    match arg(arguments, at) {
        Value::Short(value) => *value,
        other => i16::try_from(other.as_index()).unwrap_or(0),
    }
}

fn text(arguments: &[Value], at: usize) -> &str {
    match arg(arguments, at) {
        Value::String(text) => text,
        _ => "",
    }
}

/// A scenario list's element a script names by its index.
fn element<T>(list: &[T], index: i32) -> Option<&T> {
    list.get(usize::try_from(index).ok()?)
}

/// Natives that only keep what was done for the presentation, each
/// returning nothing, and a table of them, `PRESENTED`.
macro_rules! presents {
    ($($name:ident),* $(,)?) => {
        $(
            fn $name(mission: &mut $crate::Mission, arguments: &[opence_script::Value]) -> opence_script::Value {
                mission.present(stringify!($name), arguments);
                opence_script::Value::Void
            }
        )*
        pub(crate) const PRESENTED: &[(&str, opence_script::Native<$crate::Mission>)] =
            &[$((stringify!($name), $name)),*];
    };
}
pub(crate) use presents;

#[cfg(test)]
mod tests {
    use opence_content::PlacementDevice;

    use super::*;

    #[test]
    fn every_function_the_scenarios_call_has_a_native() {
        assert_eq!(unmatched(), (Vec::new(), Vec::new()));
    }

    #[test]
    fn an_ai_reference_is_decoded() {
        assert_eq!(AiReference::of(-1), AiReference::None);
        assert_eq!(AiReference::of(7), AiReference::Encounter(7));
        assert_eq!(
            AiReference::of((1 << 30) | (3 << 16) | 7),
            AiReference::Platoon(7, 3)
        );
        assert_eq!(
            AiReference::of(i32::from_ne_bytes(
                ((2u32 << 30) | (5 << 16) | 2).to_ne_bytes()
            )),
            AiReference::Squad(2, 5)
        );
    }

    #[test]
    fn a_device_begins_at_its_groups_values() {
        let device = PlacementDevice {
            power_group: 1,
            position_group: 0,
            flags: 0,
            kind_flags: 0,
        };
        let state = DeviceState::of(&device, &[0.5, 0.0]);
        assert_eq!((state.position, state.power), (0.5, 0.0));
        let open = PlacementDevice {
            power_group: -1,
            position_group: -1,
            flags: 1,
            kind_flags: 0,
        };
        let state = DeviceState::of(&open, &[]);
        assert_eq!((state.position, state.power), (1.0, 1.0));
    }
}
