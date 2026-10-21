//! The simulation: a `bevy_ecs` world stepped on a fixed tick.
//!
//! Systems run on one thread, in the order of [`SimSet`], so the same
//! inputs from the same state give the same world on every run; the maths
//! is `libm`'s and glam's (built on `libm`), never the platform's, and Rust
//! never fuses a multiply and an add unless asked. Only the simulation's
//! own types cross into `ffi`, which turns them into the snapshot; Swift
//! never sees the world.

pub mod clock;
pub mod hash;
pub mod input;
pub mod units;

use bevy_ecs::prelude::*;
use bevy_ecs::schedule::SingleThreadedExecutor;
pub use glam::{Quat, Vec3};

use hash::WorldHasher;
use input::{Buttons, InputRecord, TickInput};
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
}

impl Simulation {
    /// A world with nothing in it but the free camera.
    pub fn empty() -> Self {
        let mut world = World::new();
        world.insert_resource(Tick::default());
        world.insert_resource(Camera::default());
        world.insert_resource(FreeCamera::default());
        world.insert_resource(CurrentInput::default());
        let mut schedule = Schedule::default();
        // (one thread: systems in parallel would be ordered by chance)
        schedule.set_executor(SingleThreadedExecutor::new());
        schedule.configure_sets((SimSet::Input, SimSet::Simulate, SimSet::Finish).chain());
        schedule.add_systems((
            look_free_camera.in_set(SimSet::Input),
            move_free_camera.in_set(SimSet::Simulate),
            (place_camera, advance_tick).chain().in_set(SimSet::Finish),
        ));
        Self { world, schedule }
    }

    /// Runs one tick with `input`.
    pub fn step(&mut self, input: TickInput) {
        self.world.insert_resource(CurrentInput(input));
        self.schedule.run(&mut self.world);
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

    /// Adds an object (for tests, until maps spawn them).
    pub fn spawn_object(&mut self, id: ObjectId, transform: Transform) {
        self.world.spawn((id, transform));
    }

    /// A hash of the world's state: the tick, the camera, and every
    /// object's transform in the order of their ids.
    pub fn world_hash(&mut self) -> u64 {
        let mut hasher = WorldHasher::default();
        hasher.u64(self.tick().0);
        self.camera().transform.hash(&mut hasher);
        let mut objects: Vec<(ObjectId, Transform)> = self
            .world
            .query::<(&ObjectId, &Transform)>()
            .iter(&self.world)
            .map(|(id, t)| (*id, *t))
            .collect();
        objects.sort_by_key(|(id, _)| *id);
        for (id, transform) in objects {
            hasher.u64(u64::from(id.0));
            transform.hash(&mut hasher);
        }
        hasher.finish()
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

fn place_camera(free: Res<FreeCamera>, mut camera: ResMut<Camera>) {
    camera.transform.position = free.position;
    camera.transform.rotation = free.rotation();
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
        for id in [3, 1, 2] {
            simulation.spawn_object(
                ObjectId(id),
                Transform {
                    position: Vec3::splat(id as f32),
                    ..Default::default()
                },
            );
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
    const PINNED_HASH: u64 = 0xa426_4986_4ed8_b740;

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

    #[test]
    fn the_hash_takes_objects_by_id_not_by_spawn_order() {
        let mut a = Simulation::empty();
        let mut b = Simulation::empty();
        a.spawn_object(ObjectId(1), Transform::default());
        a.spawn_object(
            ObjectId(2),
            Transform {
                scale: 2.0,
                ..Default::default()
            },
        );
        b.spawn_object(
            ObjectId(2),
            Transform {
                scale: 2.0,
                ..Default::default()
            },
        );
        b.spawn_object(ObjectId(1), Transform::default());
        assert_eq!(a.world_hash(), b.world_hash());
    }
}
