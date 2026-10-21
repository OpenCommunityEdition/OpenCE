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

struct Running {
    simulation: Simulation,
    previous_camera: snapshot::Transform,
}

struct State {
    running: Option<Running>,
    writer: SnapshotWriter,
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
        });
        publish(&mut state, &self.snapshots, 0.0);
    }

    /// Runs one tick and publishes its snapshot.
    pub fn step(&self) -> Result<u64, EngineError> {
        let mut state = self.lock();
        let running = state.running.as_mut().ok_or(EngineError::NoWorld)?;
        running.previous_camera = transform(&running.simulation.camera().transform);
        running.simulation.step();
        let tick = running.simulation.tick().0;
        publish(&mut state, &self.snapshots, 1.0);
        Ok(tick)
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

fn publish(state: &mut State, buffers: &SnapshotBuffers, alpha: f32) {
    let State { running, writer } = state;
    let Some(running) = running else { return };
    let camera = running.simulation.camera();
    let tick = running.simulation.tick().0;
    let previous = running.previous_camera;
    writer.publish(buffers, tick, alpha, |frame| {
        frame.camera(CameraSnapshot {
            previous,
            current: transform(&camera.transform),
            vertical_fov_radians: camera.vertical_fov_radians,
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
}
