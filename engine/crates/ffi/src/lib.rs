//! The contract between the engine and the Swift app.
//!
//! Calls (load a map, start a game, input, settings) go through UniFFI;
//! the per-frame data goes through the `#[repr(C)]` types of [`snapshot`],
//! in the header cbindgen writes, which Swift reads in place.
// (the UniFFI scaffolding and the snapshot's reader are unsafe code; this
// crate and physics are the only ones allowed it)
#![allow(unsafe_code)]

pub mod snapshot;

use std::sync::{Arc, Mutex, MutexGuard};

use opence_sim::clock::Clock;
use opence_sim::input::{Buttons, TickInput};
use opence_sim::units::Radians;
use opence_sim::Simulation;
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
