//! The contract between the engine and the Swift app.
//!
//! Calls (load a map, start a game, input, settings) go through UniFFI;
//! the per-frame data goes through the `#[repr(C)]` types of [`snapshot`],
//! in the header cbindgen writes, which Swift reads in place.
// (the UniFFI scaffolding and the snapshot's reader are unsafe code; this
// crate and physics are the only ones allowed it)
#![allow(unsafe_code)]

pub mod render_level;
pub mod snapshot;

use std::path::Path;
use std::sync::{Arc, Mutex, MutexGuard};

use opence_sim::clock::Clock;
use opence_sim::input::{Buttons, TickInput};
use opence_sim::units::Radians;
use opence_sim::{FreeCamera, Simulation};
use render_level::{RenderDraw, RenderLevel, RenderMaterial, RenderTexture, VERTEX_STRIDE};
use snapshot::{CameraSnapshot, SnapshotBuffers, SnapshotWriter};

uniffi::setup_scaffolding!();

/// The engine's version, as the app shows it.
#[uniffi::export]
pub fn engine_version() -> String {
    env!("CARGO_PKG_VERSION").to_owned()
}

/// What a call to the engine can fail with.
#[derive(Debug, PartialEq, Eq, thiserror::Error, uniffi::Error)]
pub enum EngineError {
    /// A call that needs a world came before one was started.
    #[error("no world is running")]
    NoWorld,
    /// A map could not be loaded.
    #[error("{message}")]
    Load {
        /// Why.
        message: String,
    },
    /// The level has no structure BSP of that index.
    #[error("the level has no BSP {index}")]
    NoBsp {
        /// The index asked for.
        index: u32,
    },
}

/// Where the free camera is put and which way it faces (Z up; a yaw of
/// zero looks along +X; radians).
#[derive(Clone, Copy, Debug, Default, PartialEq, uniffi::Record)]
pub struct CameraPlacement {
    /// Where, in world units.
    pub x: f32,
    /// Where, in world units.
    pub y: f32,
    /// Where, in world units.
    pub z: f32,
    /// The turn to the left from +X.
    pub yaw: f32,
    /// The turn up from level.
    pub pitch: f32,
}

/// A level's structure BSP, ready for the GPU: loaded once, through the
/// cache folder, and read by the app into its buffers and textures.
#[derive(uniffi::Object)]
pub struct LevelGeometry {
    level: RenderLevel,
}

#[uniffi::export]
impl LevelGeometry {
    /// The BSP `bsp` of the map at `map`, its pack kept in `cache_folder`.
    #[uniffi::constructor]
    pub fn load(map: String, cache_folder: String, bsp: u32) -> Result<Arc<Self>, EngineError> {
        let loaded = opence_import::load_level(Path::new(&map), Path::new(&cache_folder)).map_err(
            |error| EngineError::Load {
                message: error.to_string(),
            },
        )?;
        let level = RenderLevel::build(&loaded.level, bsp as usize)
            .ok_or(EngineError::NoBsp { index: bsp })?;
        Ok(Arc::new(Self { level }))
    }

    /// The bytes of one vertex.
    pub fn vertex_stride(&self) -> u32 {
        VERTEX_STRIDE as u32
    }

    /// The vertices: position, normal, texture and lightmap coordinates,
    /// as little-endian floats.
    pub fn vertices(&self) -> Vec<u8> {
        self.level.vertices.clone()
    }

    /// The indices, as little-endian 32-bit integers.
    pub fn indices(&self) -> Vec<u8> {
        self.level
            .indices
            .iter()
            .flat_map(|index| index.to_le_bytes())
            .collect()
    }

    /// The draws: opaque and alpha-tested, then blended.
    pub fn draws(&self) -> Vec<RenderDraw> {
        self.level.draws.clone()
    }

    /// The materials the draws use.
    pub fn materials(&self) -> Vec<RenderMaterial> {
        self.level.materials.clone()
    }

    /// The textures the materials and lightmaps use.
    pub fn textures(&self) -> Vec<RenderTexture> {
        self.level
            .textures
            .iter()
            .map(|(texture, _)| texture.clone())
            .collect()
    }

    /// A texture's level's bytes (none for one it does not have).
    pub fn texture_level(&self, texture: u32, level: u32) -> Vec<u8> {
        self.level
            .textures
            .get(texture as usize)
            .and_then(|(_, levels)| levels.get(level as usize))
            .cloned()
            .unwrap_or_default()
    }

    /// The BSP's bounds: least and greatest x, then y, then z.
    pub fn bounds(&self) -> Vec<f32> {
        self.level.bounds.iter().flatten().copied().collect()
    }

    /// Where a player would first see the BSP from: its first player
    /// start, at a standing player's eyes, facing its way; or, if it has
    /// none, a view over it from its bounds.
    pub fn start_view(&self) -> CameraPlacement {
        match self.level.player_start {
            Some((position, facing)) => player_view(position, facing),
            None => start_view(self.level.bounds),
        }
    }
}

/// How far a standing player's eyes are above their feet, in world units
/// (the stock cyborg's camera).
const EYE_HEIGHT: f32 = 0.62;

/// The view from a player start at `position` facing `facing`.
fn player_view(position: [f32; 3], facing: f32) -> CameraPlacement {
    let [x, y, z] = position;
    CameraPlacement {
        x,
        y,
        z: z + EYE_HEIGHT,
        yaw: facing,
        pitch: 0.0,
    }
}

/// A view over a BSP of `bounds`: high over one corner, looking at its
/// middle, a little above its floor.
fn start_view(bounds: [[f32; 2]; 3]) -> CameraPlacement {
    let at = |axis: usize, part: f32| {
        let [least, greatest] = bounds[axis];
        least + (greatest - least) * part
    };
    let (x, y, z) = (at(0, 0.2), at(1, 0.2), at(2, 0.7));
    let (dx, dy, dz) = (at(0, 0.5) - x, at(1, 0.5) - y, at(2, 0.3) - z);
    // (libm's, as the simulation's: the camera's start is part of its state)
    let across = libm::sqrtf(dx * dx + dy * dy);
    let finite = |value: f32| if value.is_finite() { value } else { 0.0 };
    CameraPlacement {
        x: finite(x),
        y: finite(y),
        z: finite(z),
        yaw: finite(libm::atan2f(dy, dx)),
        pitch: finite(libm::atan2f(dz, across)),
    }
}

/// The player's input as the app reads it from the keyboard and mouse.
#[derive(Clone, Copy, Debug, Default, PartialEq, uniffi::Record)]
pub struct InputState {
    /// The buttons held (`Buttons` of the simulation).
    pub buttons: u32,
    /// Forward (127) or back (-127).
    pub forward: i8,
    /// Right (127) or left (-127).
    pub right: i8,
    /// Up (127) or down (-127), for the free camera.
    pub up: i8,
    /// The turn to the left since the last input, in radians.
    pub yaw: f32,
    /// The turn up since the last input, in radians.
    pub pitch: f32,
}

impl InputState {
    /// The input as the simulation takes it: unknown buttons dropped, the
    /// axes kept within ±127 and the turns finite and within a half turn.
    fn tick_input(self) -> TickInput {
        let axis = |value: i8| value.max(-127);
        let angle = |value: f32| {
            if value.is_finite() {
                Radians(value.clamp(-std::f32::consts::PI, std::f32::consts::PI))
            } else {
                Radians(0.0)
            }
        };
        TickInput {
            buttons: Buttons(self.buttons & Buttons::ALL.0),
            forward: axis(self.forward),
            right: axis(self.right),
            up: axis(self.up),
            yaw: angle(self.yaw),
            pitch: angle(self.pitch),
        }
    }
}

struct Running {
    simulation: Simulation,
    previous_camera: snapshot::Transform,
    clock: Clock,
}

struct State {
    running: Option<Running>,
    writer: SnapshotWriter,
    input: InputState,
}

/// The engine: one per app.
#[derive(uniffi::Object)]
pub struct Engine {
    state: Mutex<State>,
    snapshots: Arc<SnapshotBuffers>,
}

#[uniffi::export]
impl Engine {
    /// An engine with no world.
    #[uniffi::constructor]
    pub fn new() -> Arc<Self> {
        let (buffers, writer) = SnapshotBuffers::new();
        Arc::new(Self {
            state: Mutex::new(State {
                running: None,
                writer,
                input: InputState::default(),
            }),
            snapshots: Arc::new(buffers),
        })
    }

    /// Starts a world with nothing in it, and publishes its first snapshot.
    pub fn start_empty_world(&self) {
        let mut state = self.lock();
        let simulation = Simulation::empty();
        let previous_camera = transform(&simulation.camera().transform);
        state.running = Some(Running {
            simulation,
            previous_camera,
            clock: Clock::default(),
        });
        publish(&mut state, &self.snapshots, 0.0);
    }

    /// Puts the free camera at `placement`, as if it had always been there
    /// (no interpolation from where it was).
    pub fn place_camera(&self, placement: CameraPlacement) -> Result<(), EngineError> {
        let mut state = self.lock();
        let running = state.running.as_mut().ok_or(EngineError::NoWorld)?;
        let finite = |value: f32| if value.is_finite() { value } else { 0.0 };
        running.simulation.place_free_camera(FreeCamera {
            position: opence_sim::Vec3::new(
                finite(placement.x),
                finite(placement.y),
                finite(placement.z),
            ),
            yaw: Radians(finite(placement.yaw)),
            pitch: Radians(finite(placement.pitch)),
        });
        running.previous_camera = transform(&running.simulation.camera().transform);
        publish(&mut state, &self.snapshots, 0.0);
        Ok(())
    }

    /// Sets the input the next ticks run with. The turns are used once,
    /// by the next tick; the axes and buttons stay until the next call.
    pub fn set_input(&self, input: InputState) {
        let mut state = self.lock();
        let turn = |earlier: f32, later: f32| {
            if (earlier + later).is_finite() {
                earlier + later
            } else {
                earlier
            }
        };
        let yaw = turn(state.input.yaw, input.yaw);
        let pitch = turn(state.input.pitch, input.pitch);
        state.input = InputState {
            yaw,
            pitch,
            ..input
        };
    }

    /// Runs one tick and publishes its snapshot.
    pub fn step(&self) -> Result<u64, EngineError> {
        let mut state = self.lock();
        run_ticks(&mut state, 1)?;
        let tick = state
            .running
            .as_ref()
            .map_or(0, |running| running.simulation.tick().0);
        publish(&mut state, &self.snapshots, 1.0);
        Ok(tick)
    }

    /// Adds a displayed frame's time: runs the ticks it covers and
    /// publishes a snapshot with the interpolation factor between the last
    /// two. Returns the ticks run.
    pub fn advance(&self, frame_seconds: f64) -> Result<u32, EngineError> {
        let mut state = self.lock();
        let running = state.running.as_mut().ok_or(EngineError::NoWorld)?;
        let frame = running.clock.advance(frame_seconds);
        run_ticks(&mut state, frame.ticks)?;
        publish(&mut state, &self.snapshots, frame.alpha);
        Ok(frame.ticks)
    }

    /// The address to pass to `opence_snapshot_acquire`, valid while this
    /// engine is alive.
    pub fn snapshot_buffers(&self) -> u64 {
        Arc::as_ptr(&self.snapshots) as u64
    }
}

impl Engine {
    fn lock(&self) -> MutexGuard<'_, State> {
        // (a panic aborts, so the lock is never poisoned by one)
        self.state
            .lock()
            .unwrap_or_else(std::sync::PoisonError::into_inner)
    }
}

fn transform(from: &opence_sim::Transform) -> snapshot::Transform {
    snapshot::Transform {
        position: snapshot::Vec3 {
            x: from.position.x,
            y: from.position.y,
            z: from.position.z,
        },
        scale: from.scale,
        rotation: snapshot::Quat {
            x: from.rotation.x,
            y: from.rotation.y,
            z: from.rotation.z,
            w: from.rotation.w,
        },
    }
}

fn run_ticks(state: &mut State, ticks: u32) -> Result<(), EngineError> {
    let State { running, input, .. } = state;
    let running = running.as_mut().ok_or(EngineError::NoWorld)?;
    for _ in 0..ticks {
        running.previous_camera = transform(&running.simulation.camera().transform);
        running.simulation.step(input.tick_input());
        // (the mouse's turn is used once)
        input.yaw = 0.0;
        input.pitch = 0.0;
    }
    Ok(())
}

fn publish(state: &mut State, buffers: &SnapshotBuffers, alpha: f32) {
    let State {
        running, writer, ..
    } = state;
    let Some(running) = running else { return };
    let camera = running.simulation.camera();
    let tick = running.simulation.tick().0;
    let previous = running.previous_camera;
    writer.publish(buffers, tick, alpha, |frame| {
        frame.camera(CameraSnapshot {
            previous,
            current: transform(&camera.transform),
            vertical_fov_radians: camera.vertical_fov.0,
            reserved: [0; 3],
        });
    });
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn the_version_is_the_crates() {
        assert_eq!(engine_version(), "0.1.0");
    }

    #[test]
    fn stepping_without_a_world_fails() {
        assert_eq!(Engine::new().step(), Err(EngineError::NoWorld));
    }

    #[test]
    fn an_empty_world_publishes_its_ticks() {
        let engine = Engine::new();
        engine.start_empty_world();
        assert_eq!(engine.step(), Ok(1));
        assert_eq!(engine.step(), Ok(2));
        let buffers = engine.snapshot_buffers() as *const SnapshotBuffers;
        // SAFETY: the engine is alive and this is the only reader.
        let frame = unsafe { &*snapshot::opence_snapshot_acquire(buffers) };
        assert_eq!(frame.tick, 2);
        assert_eq!(frame.object_count, 0);
        assert_eq!(frame.camera.current.scale, 1.0);
        assert!(frame.camera.vertical_fov_radians > 1.0);
    }

    fn acquire(engine: &Engine) -> &FrameSnapshotRef {
        // SAFETY: the engine is alive and each test is its only reader.
        unsafe {
            &*snapshot::opence_snapshot_acquire(engine.snapshot_buffers() as *const SnapshotBuffers)
        }
    }
    type FrameSnapshotRef = snapshot::FrameSnapshot;

    #[test]
    fn advancing_runs_whole_ticks_and_publishes_alpha() {
        let engine = Engine::new();
        assert_eq!(engine.advance(0.1), Err(EngineError::NoWorld));
        engine.start_empty_world();
        // (a 144 Hz display: a tick every 2.4 frames)
        let mut ticks = 0;
        for _ in 0..144 {
            ticks += engine.advance(1.0 / 144.0).unwrap_or(0);
            let frame = acquire(&engine);
            assert!((0.0..1.0).contains(&frame.alpha), "{}", frame.alpha);
            assert_eq!(frame.tick, u64::from(ticks));
        }
        assert!((59..=60).contains(&ticks), "{ticks}");
    }

    #[test]
    fn the_mouse_turns_once_and_the_keys_hold() {
        let engine = Engine::new();
        engine.start_empty_world();
        engine.set_input(InputState {
            forward: 127,
            yaw: 0.25,
            ..Default::default()
        });
        for _ in 0..3 {
            let _ = engine.step();
        }
        let frame = acquire(&engine);
        let turned = opence_sim::FreeCamera {
            yaw: Radians(0.25),
            ..Default::default()
        }
        .rotation();
        assert!((frame.camera.current.rotation.z - turned.z).abs() < 1e-6);
        assert!(frame.camera.current.position.x > 0.0 && frame.camera.current.position.y > 0.0);
    }

    #[test]
    fn a_placed_camera_is_published_without_a_streak() {
        let engine = Engine::new();
        let placement = CameraPlacement {
            x: 10.0,
            y: -4.0,
            z: 2.0,
            yaw: 1.0,
            pitch: 0.5,
        };
        assert_eq!(engine.place_camera(placement), Err(EngineError::NoWorld));
        engine.start_empty_world();
        assert_eq!(engine.place_camera(placement), Ok(()));
        let frame = acquire(&engine);
        assert_eq!(frame.camera.current.position.x, 10.0);
        assert_eq!(frame.camera.previous.position.y, -4.0);
        let _ = engine.step();
        let frame = acquire(&engine);
        assert_eq!(frame.camera.current.position.z, 2.0);
    }

    #[test]
    fn the_start_view_looks_into_the_bounds() {
        let view = start_view([[0.0, 100.0], [0.0, 100.0], [0.0, 10.0]]);
        assert_eq!((view.x, view.y, view.z), (20.0, 20.0, 7.0));
        assert!((view.yaw - std::f32::consts::FRAC_PI_4).abs() < 1e-6);
        assert!(view.pitch < 0.0 && view.pitch > -0.2);
        let nonsense = start_view([[f32::NAN, 1.0], [0.0, f32::INFINITY], [0.0, 0.0]]);
        assert!([nonsense.x, nonsense.y, nonsense.yaw, nonsense.pitch]
            .iter()
            .all(|value| value.is_finite()));
    }

    #[test]
    fn a_player_start_is_seen_from_its_eyes() {
        let view = player_view([1.0, 2.0, 3.0], 0.5);
        assert_eq!((view.x, view.y, view.z), (1.0, 2.0, 3.0 + EYE_HEIGHT));
        assert_eq!((view.yaw, view.pitch), (0.5, 0.0));
    }

    #[test]
    fn a_missing_map_is_a_load_error() {
        let result = LevelGeometry::load("/nonexistent/none.map".into(), "/nonexistent".into(), 0);
        assert!(matches!(result, Err(EngineError::Load { .. })));
    }

    #[test]
    fn nonsense_input_is_made_safe() {
        let input = InputState {
            buttons: u32::MAX,
            forward: i8::MIN,
            yaw: f32::NAN,
            pitch: 100.0,
            ..Default::default()
        };
        let tick = input.tick_input();
        assert_eq!(tick.buttons, Buttons::ALL);
        assert_eq!(tick.forward, -127);
        assert_eq!(tick.yaw, Radians(0.0));
        assert_eq!(tick.pitch, Radians(std::f32::consts::PI));
    }
}
