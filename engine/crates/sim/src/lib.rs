//! The simulation: a `bevy_ecs` world stepped on a fixed tick.
//!
//! Systems run on one thread, in the order of [`SimSet`], so the same
//! inputs from the same state give the same world on every run; the maths
//! is `libm`'s and glam's (built on `libm`), never the platform's, and Rust
//! never fuses a multiply and an add unless asked. Only the simulation's
//! own types cross into `ffi`, which turns them into the snapshot; Swift
//! never sees the world.

pub mod animation;
pub mod clock;
pub mod hash;
pub mod input;
pub mod objects;
pub mod player;
pub mod random;
pub mod units;

use bevy_ecs::prelude::*;
use bevy_ecs::schedule::SingleThreadedExecutor;
pub use glam::{Quat, Vec3};

use animation::{AnimationPlayer, NodeMatrices};
use hash::WorldHasher;
use input::{Buttons, InputRecord, TickInput};
use objects::{
    kind_index, Difficulty, Object, ObjectHandle, ObjectKinds, ObjectName, ObjectSpawn,
    ObjectTable, Permutation, TableFull, Team,
};
use opence_content::{Level, ObjectKind, Scenario};
use player::{Player, PlayerError, PlayerPose, View};
use units::{Radians, WorldUnits};

/// A position and an orientation in the world, in world units.
#[derive(Component, Clone, Copy, Debug, PartialEq)]
pub struct Transform {
    /// The position, in world units.
    pub position: Vec3,
    /// The orientation.
    pub rotation: Quat,
    /// The uniform scale.
    pub scale: f32,
}

impl Default for Transform {
    fn default() -> Self {
        Self {
            position: Vec3::ZERO,
            rotation: Quat::IDENTITY,
            scale: 1.0,
        }
    }
}

impl Transform {
    fn hash(&self, hasher: &mut WorldHasher) {
        for value in self
            .position
            .to_array()
            .into_iter()
            .chain(self.rotation.to_array())
            .chain([self.scale])
        {
            hasher.f32(value);
        }
    }
}

/// An object's id: the same from tick to tick, and the order the world
/// hash takes objects in.
#[derive(Component, Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct ObjectId(pub u32);

/// The camera the world is seen from.
#[derive(Resource, Clone, Copy, Debug, PartialEq)]
pub struct Camera {
    /// Where it is and where it looks.
    pub transform: Transform,
    /// The vertical field of view; the horizontal one grows with the
    /// aspect ratio (Hor+).
    pub vertical_fov: Radians,
}

impl Default for Camera {
    fn default() -> Self {
        // (70 degrees vertical: about the stock 70 horizontal at 4:3,
        // widened for today's displays)
        Self {
            transform: Transform::default(),
            vertical_fov: Radians(70.0_f32.to_radians()),
        }
    }
}

/// The free camera's state: where it is and which way it faces. Halo's
/// world is Z up, and a yaw of zero looks along +X.
#[derive(Resource, Clone, Copy, Debug, Default, PartialEq)]
pub struct FreeCamera {
    /// Where it is.
    pub position: Vec3,
    /// The turn to the left from +X.
    pub yaw: Radians,
    /// The turn up from level.
    pub pitch: Radians,
}

impl FreeCamera {
    /// Its speed, in world units per second (a world unit is about 3 m).
    pub const SPEED: WorldUnits = WorldUnits(2.0);
    /// Its speed with the sprint button.
    pub const SPRINT_SPEED: WorldUnits = WorldUnits(8.0);
    /// How far it looks up or down: just short of straight, where the yaw
    /// would flip.
    pub const MAX_PITCH: Radians = Radians(std::f32::consts::FRAC_PI_2 - 0.001);

    /// Its orientation: a yaw about Z, then a pitch about the camera's Y.
    pub fn rotation(&self) -> Quat {
        Quat::from_rotation_z(self.yaw.0) * Quat::from_rotation_y(-self.pitch.0)
    }
}

/// The number of ticks run since the world started.
#[derive(Resource, Clone, Copy, Debug, Default, PartialEq, Eq, PartialOrd, Ord)]
pub struct Tick(pub u64);

/// This tick's input.
#[derive(Resource, Clone, Copy, Debug, Default, PartialEq)]
pub struct CurrentInput(pub TickInput);

/// The stages of a tick, run in this order. A new system goes in its stage
/// with `in_set`; nothing else needs to know of it.
#[derive(SystemSet, Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum SimSet {
    /// What the tick's input sets directly (the camera's look).
    Input,
    /// The world moving.
    Simulate,
    /// What follows from the world's new state (the camera's transform,
    /// the tick counter).
    Finish,
}

/// A running world.
pub struct Simulation {
    world: World,
    schedule: Schedule,
    /// The player, if there is one (its physics world is not `Sync`, so it
    /// is kept beside the bevy world, not in it).
    player: Option<Player>,
}

/// An object as the renderer draws it.
#[derive(Clone, Copy, Debug)]
pub struct Drawn<'a> {
    /// Its id.
    pub id: ObjectId,
    /// Where it was at the tick before the last.
    pub previous: &'a Transform,
    /// Where it was at the last tick.
    pub current: &'a Transform,
    /// Its nodes, if it is animated.
    pub nodes: Option<&'a NodeMatrices>,
    /// How it is drawn, if it has been told.
    pub appearance: Option<&'a objects::Appearance>,
}

impl Simulation {
    /// A world with nothing in it but the free camera.
    pub fn empty() -> Self {
        let mut world = World::new();
        world.insert_resource(Tick::default());
        world.insert_resource(Camera::default());
        world.insert_resource(FreeCamera::default());
        world.insert_resource(CurrentInput::default());
        world.insert_resource(ObjectTable::default());
        world.insert_resource(ObjectKinds::default());
        world.insert_resource(random::RandomSeed::default());
        let mut schedule = Schedule::default();
        // (one thread: systems in parallel would be ordered by chance)
        schedule.set_executor(SingleThreadedExecutor::new());
        schedule.configure_sets((SimSet::Input, SimSet::Simulate, SimSet::Finish).chain());
        schedule.add_systems((
            (look_free_camera, remember_transforms).in_set(SimSet::Input),
            (
                move_free_camera,
                player::move_player.before(animation::play_animations),
                animation::play_animations,
            )
                .in_set(SimSet::Simulate),
            (place_camera, advance_tick).chain().in_set(SimSet::Finish),
        ));
        Self {
            world,
            schedule,
            player: None,
        }
    }

    /// Runs one tick with `input`: the player's biped moves first, then
    /// the world's systems run.
    pub fn step(&mut self, input: TickInput) {
        self.world.insert_resource(CurrentInput(input));
        if let Some(player) = self.player.as_mut() {
            let pose = player.step(&input);
            self.world.insert_resource(pose);
        }
        self.schedule.run(&mut self.world);
    }

    /// Makes the player: its biped standing at `level`'s player start
    /// `start`, moving over that start's structure BSP, the camera at its
    /// eyes. A player there was before is removed.
    pub fn spawn_player(
        &mut self,
        level: &Level,
        start: usize,
    ) -> Result<ObjectHandle, PlayerError> {
        if let Some(earlier) = self.player.take() {
            self.remove(earlier.handle());
            self.world.remove_resource::<PlayerPose>();
        }
        let world = &mut self.world;
        let player = Player::new(level, start, |tag, transform| {
            objects::spawn(
                world,
                ObjectSpawn {
                    kind: ObjectKind::Biped,
                    tag,
                    transform,
                    name: None,
                    permutation: None,
                    team: None,
                },
            )
            .ok()
        })?;
        let handle = player.handle();
        self.player = Some(player);
        self.place_player_camera();
        Ok(handle)
    }

    /// The player, if there is one.
    pub fn player(&self) -> Option<&Player> {
        self.player.as_ref()
    }

    /// Sees from `view`; false without a player.
    pub fn set_player_view(&mut self, view: View) -> bool {
        let Some(player) = self.player.as_mut() else {
            return false;
        };
        player.set_view(view);
        self.place_player_camera();
        true
    }

    /// The camera at the player's view now, as if it had always been there.
    fn place_player_camera(&mut self) {
        let Some(player) = self.player.as_ref() else {
            return;
        };
        let pose = player.pose();
        let mut camera = self.camera();
        camera.transform = pose.camera;
        self.world.insert_resource(camera);
        self.world.insert_resource(pose);
    }

    /// Runs every tick of a recording.
    pub fn replay(&mut self, record: &InputRecord) {
        for &input in &record.ticks {
            self.step(input);
        }
    }

    /// The ticks run so far.
    pub fn tick(&self) -> Tick {
        self.world
            .get_resource::<Tick>()
            .copied()
            .unwrap_or_default()
    }

    /// The camera.
    pub fn camera(&self) -> Camera {
        self.world
            .get_resource::<Camera>()
            .copied()
            .unwrap_or_default()
    }

    /// Puts the free camera at `free` (a level's start), its pitch kept
    /// within the limit, and the camera with it.
    pub fn place_free_camera(&mut self, free: FreeCamera) {
        let max = FreeCamera::MAX_PITCH.0;
        let free = FreeCamera {
            pitch: Radians(free.pitch.0.clamp(-max, max)),
            ..free
        };
        self.world.insert_resource(free);
        let mut camera = self.camera();
        camera.transform.position = free.position;
        camera.transform.rotation = free.rotation();
        self.world.insert_resource(camera);
    }

    /// Makes an object.
    pub fn spawn(&mut self, spawn: ObjectSpawn) -> Result<ObjectHandle, TableFull> {
        objects::spawn(&mut self.world, spawn)
    }

    /// Makes every object `scenario` places at `difficulty`.
    pub fn spawn_scenario(
        &mut self,
        scenario: &Scenario,
        difficulty: Difficulty,
    ) -> Result<Vec<ObjectHandle>, TableFull> {
        objects::spawn_scenario(&mut self.world, scenario, difficulty)
    }

    /// Plays `player` on an object, posing its nodes now; false if `handle`
    /// is not in the world.
    pub fn animate(&mut self, handle: ObjectHandle, player: AnimationPlayer) -> bool {
        let Some(entity) = self
            .world
            .get_resource::<ObjectTable>()
            .and_then(|table| table.entity(handle))
        else {
            return false;
        };
        let Ok(mut object) = self.world.get_entity_mut(entity) else {
            return false;
        };
        let transform = object.get::<Transform>().copied().unwrap_or_default();
        let current = player.matrices(&transform);
        object.insert((
            player,
            NodeMatrices {
                previous: current.clone(),
                current,
            },
        ));
        true
    }

    /// Each object as the renderer draws it, in the order of their ids.
    pub fn visit_drawn(&mut self, mut visit: impl FnMut(Drawn<'_>)) {
        let mut objects: Vec<_> = self
            .world
            .query::<(
                &ObjectId,
                &Transform,
                Option<&PreviousTransform>,
                Option<&NodeMatrices>,
                Option<&objects::Appearance>,
            )>()
            .iter(&self.world)
            .collect();
        objects.sort_by_key(|(id, ..)| **id);
        // (the player's own body is not seen from its eyes)
        let hidden = self
            .player
            .as_ref()
            .filter(|player| player.view() == View::FirstPerson)
            .map(|player| ObjectId(player.handle().index));
        for (id, transform, previous, nodes, appearance) in objects {
            if Some(*id) == hidden {
                continue;
            }
            visit(Drawn {
                id: *id,
                previous: previous.map_or(transform, |p| &p.0),
                current: transform,
                nodes,
                appearance,
            });
        }
    }

    /// Sets how an object is drawn; false if `handle` is not in the world.
    pub fn set_appearance(
        &mut self,
        handle: ObjectHandle,
        appearance: objects::Appearance,
    ) -> bool {
        let Some(entity) = self
            .world
            .get_resource::<ObjectTable>()
            .and_then(|table| table.entity(handle))
        else {
            return false;
        };
        let Ok(mut object) = self.world.get_entity_mut(entity) else {
            return false;
        };
        object.insert(appearance);
        true
    }

    /// Removes an object; false if `handle` is not in the world.
    pub fn remove(&mut self, handle: ObjectHandle) -> bool {
        objects::remove(&mut self.world, handle)
    }

    /// The hooks each kind's objects are made with.
    pub fn kinds_mut(&mut self) -> Mut<'_, ObjectKinds> {
        self.world.resource_mut::<ObjectKinds>()
    }

    /// Every object, by slot.
    pub fn objects(&mut self) -> Vec<ObjectHandle> {
        let mut handles: Vec<ObjectHandle> = self
            .world
            .query::<&ObjectHandle>()
            .iter(&self.world)
            .copied()
            .collect();
        handles.sort();
        handles
    }

    /// The entity `handle` is, if it is still in the world.
    fn entity(&self, handle: ObjectHandle) -> Option<EntityRef<'_>> {
        let entity = self.world.get_resource::<ObjectTable>()?.entity(handle)?;
        self.world.get_entity(entity).ok()
    }

    /// One of an object's components, if `handle` is still in the world.
    pub fn component<C: Component + Clone>(&self, handle: ObjectHandle) -> Option<C> {
        self.entity(handle)?.get::<C>().cloned()
    }

    /// What an object is.
    pub fn object(&self, handle: ObjectHandle) -> Option<Object> {
        self.component(handle)
    }

    /// Where an object is.
    pub fn transform(&self, handle: ObjectHandle) -> Option<Transform> {
        self.component(handle)
    }

    /// A hash of the world's state: the tick, the camera, and every
    /// object in the order of their ids: its transform, kind, tag, name,
    /// permutation, team and animation, the random seed, and the player's
    /// movement and look.
    pub fn world_hash(&mut self) -> u64 {
        let mut hasher = WorldHasher::default();
        hasher.u64(self.tick().0);
        self.camera().transform.hash(&mut hasher);
        hasher.u64(u64::from(
            self.world
                .get_resource::<random::RandomSeed>()
                .map_or(0, |seed| seed.0),
        ));
        let mut objects: Vec<_> = self
            .world
            .query::<(
                &ObjectId,
                &Transform,
                &Object,
                Option<&ObjectName>,
                Option<&Permutation>,
                Option<&Team>,
                Option<&AnimationPlayer>,
            )>()
            .iter(&self.world)
            .collect();
        objects.sort_by_key(|(id, ..)| **id);
        for (id, transform, object, name, permutation, team, player) in objects {
            hasher.u64(u64::from(id.0));
            transform.hash(&mut hasher);
            hasher.u64(kind_index(object.kind) as u64);
            hasher.bytes(&object.tag.group.0);
            hasher.u64(object.tag.path.len() as u64);
            hasher.bytes(object.tag.path.as_bytes());
            // (each absent part hashed as such, so two never run together)
            match name {
                Some(name) => {
                    hasher.u64(1 + name.0.len() as u64);
                    hasher.bytes(name.0.as_bytes());
                }
                None => hasher.u64(0),
            }
            match permutation {
                Some(Permutation(permutation)) => {
                    hasher.u64(1);
                    for color in permutation.change_colors {
                        hasher.u64(u64::from(color));
                    }
                    hasher.bytes(&permutation.region_permutations);
                }
                None => hasher.u64(0),
            }
            hasher.u64(team.map_or(u64::MAX, |team| u64::from(team.0)));
            match player {
                Some(player) => {
                    hasher.u64(1 + u64::from(player.state.animation));
                    hasher.u64(u64::from(player.state.frame));
                }
                None => hasher.u64(0),
            }
        }
        // (nothing for no player, so that a world without one hashes as
        // it did before there were players)
        if let Some(player) = self.player.as_ref() {
            player.hash(&mut hasher);
        }
        hasher.finish()
    }
}

/// Where an object was at the tick before: the renderer interpolates from
/// it to its [`Transform`].
#[derive(Component, Clone, Copy, Debug, PartialEq)]
pub struct PreviousTransform(pub Transform);

fn remember_transforms(mut objects: Query<(&Transform, &mut PreviousTransform)>) {
    for (transform, mut previous) in &mut objects {
        previous.0 = *transform;
    }
}

fn look_free_camera(input: Res<CurrentInput>, mut camera: ResMut<FreeCamera>) {
    let max = FreeCamera::MAX_PITCH.0;
    camera.yaw = Radians(libm::remainderf(
        camera.yaw.0 + input.0.yaw.0,
        std::f32::consts::TAU,
    ));
    camera.pitch = Radians((camera.pitch.0 + input.0.pitch.0).clamp(-max, max));
}

fn move_free_camera(input: Res<CurrentInput>, mut camera: ResMut<FreeCamera>) {
    let input = input.0;
    let axis = |value: i8| f32::from(value) / 127.0;
    let speed = if input.buttons.contains(Buttons::SPRINT) {
        FreeCamera::SPRINT_SPEED
    } else {
        FreeCamera::SPEED
    };
    let rotation = camera.rotation();
    let direction = rotation * Vec3::X * axis(input.forward)
        - rotation * Vec3::Y * axis(input.right)
        + Vec3::Z * axis(input.up);
    // (no faster on a diagonal)
    let direction = if direction.length_squared() > 1.0 {
        direction.normalize()
    } else {
        direction
    };
    camera.position += direction * (speed.0 * clock::TICK.0);
}

fn place_camera(
    free: Res<FreeCamera>,
    player: Option<Res<PlayerPose>>,
    mut camera: ResMut<Camera>,
) {
    if let Some(player) = player {
        camera.transform = player.camera;
    } else {
        camera.transform.position = free.position;
        camera.transform.rotation = free.rotation();
    }
}

fn advance_tick(mut tick: ResMut<Tick>) {
    tick.0 += 1;
}

#[cfg(test)]
mod tests {
    use super::*;

    /// Inputs that turn, move and sprint, from a fixed pseudo-random
    /// sequence.
    fn recorded_inputs(ticks: usize) -> InputRecord {
        let mut state = 0x2545_f491_4f6c_dd1du64;
        let mut next = move || {
            state ^= state << 13;
            state ^= state >> 7;
            state ^= state << 17;
            state
        };
        let ticks = (0..ticks)
            .map(|_| {
                let r = next();
                TickInput {
                    buttons: if r & 1 == 0 {
                        Buttons::SPRINT
                    } else {
                        Buttons::default()
                    },
                    forward: (r >> 8) as i8 / 2,
                    right: (r >> 16) as i8 / 2,
                    up: (r >> 24) as i8 / 4,
                    yaw: Radians(((r >> 32) as u16 as f32 / 65535.0 - 0.5) * 0.1),
                    pitch: Radians(((r >> 48) as u16 as f32 / 65535.0 - 0.5) * 0.05),
                }
            })
            .collect();
        InputRecord { ticks }
    }

    fn replayed_hash(record: &InputRecord) -> u64 {
        let mut simulation = Simulation::empty();
        for at in [3, 1, 2] {
            let spawned = simulation.spawn(objects::ObjectSpawn {
                kind: opence_content::ObjectKind::Scenery,
                tag: opence_content::TagRef {
                    group: opence_content::Group(*b"scen"),
                    path: format!("scenery\\rock{at}"),
                },
                transform: Transform {
                    position: Vec3::splat(at as f32),
                    ..Default::default()
                },
                name: None,
                permutation: None,
                team: Some(at),
            });
            assert!(spawned.is_ok());
        }
        simulation.replay(record);
        simulation.world_hash()
    }

    #[test]
    fn an_empty_world_counts_its_ticks() {
        let mut simulation = Simulation::empty();
        assert_eq!(simulation.tick(), Tick(0));
        simulation.step(TickInput::default());
        simulation.step(TickInput::default());
        assert_eq!(simulation.tick(), Tick(2));
    }

    #[test]
    fn a_replay_gives_the_same_hash_on_every_run() {
        let record = recorded_inputs(3600);
        let first = replayed_hash(&record);
        for _ in 0..3 {
            assert_eq!(replayed_hash(&record), first);
        }
    }

    #[test]
    fn a_replay_read_from_its_file_gives_the_same_hash() {
        let record = recorded_inputs(600);
        let read = InputRecord::from_bytes(&record.to_bytes()).unwrap_or_default();
        assert_eq!(replayed_hash(&read), replayed_hash(&record));
    }

    #[test]
    fn the_replays_hash_is_pinned() {
        // A change to the simulation's maths or its order changes this;
        // when that is meant, write the new value here. A change nobody
        // meant (another platform's maths, a fused multiply-add, systems
        // out of order) is what this catches.
        assert_eq!(replayed_hash(&recorded_inputs(3600)), PINNED_HASH);
    }
    const PINNED_HASH: u64 = 0x0101_d70d_de3d_1b7f;

    #[test]
    fn one_different_input_changes_the_hash() {
        let record = recorded_inputs(600);
        let mut changed = record.clone();
        changed.ticks[300].yaw = Radians(changed.ticks[300].yaw.0 + 0.001);
        assert_ne!(replayed_hash(&changed), replayed_hash(&record));
    }

    #[test]
    fn the_free_camera_moves_at_its_speed() {
        let mut simulation = Simulation::empty();
        for _ in 0..60 {
            simulation.step(TickInput {
                forward: 127,
                ..Default::default()
            });
        }
        let position = simulation.camera().transform.position;
        assert!(
            (position.x - FreeCamera::SPEED.0).abs() < 1e-4,
            "{position}"
        );
        assert!(position.y.abs() < 1e-6 && position.z.abs() < 1e-6);
    }

    #[test]
    fn the_free_camera_turns_and_stops_short_of_straight_up() {
        let mut simulation = Simulation::empty();
        for _ in 0..100 {
            simulation.step(TickInput {
                yaw: Radians(0.1),
                pitch: Radians(0.1),
                ..Default::default()
            });
        }
        let forward = simulation.camera().transform.rotation * Vec3::X;
        assert!(forward.z > 0.999 && forward.z < 1.0, "{forward}");
    }

    fn rock(scale: f32) -> objects::ObjectSpawn {
        objects::ObjectSpawn {
            kind: opence_content::ObjectKind::Scenery,
            tag: opence_content::TagRef {
                group: opence_content::Group(*b"scen"),
                path: "scenery\\rock".into(),
            },
            transform: Transform {
                scale,
                ..Default::default()
            },
            name: None,
            permutation: None,
            team: None,
        }
    }

    #[test]
    fn the_hash_takes_objects_by_id_not_by_spawn_order() -> Result<(), Box<dyn std::error::Error>> {
        // (a's slot 0 is taken again after its first object goes, by an
        // entity made after slot 1's)
        let mut a = Simulation::empty();
        let first = a.spawn(rock(1.0))?;
        a.spawn(rock(2.0))?;
        assert!(a.remove(first));
        a.spawn(rock(3.0))?;
        let mut b = Simulation::empty();
        b.spawn(rock(3.0))?;
        b.spawn(rock(2.0))?;
        assert_eq!(a.world_hash(), b.world_hash());
        let mut c = Simulation::empty();
        c.spawn(rock(2.0))?;
        c.spawn(rock(3.0))?;
        assert_ne!(a.world_hash(), c.world_hash());
        Ok(())
    }
}
