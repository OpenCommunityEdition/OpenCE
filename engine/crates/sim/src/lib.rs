//! The simulation: a `bevy_ecs` world stepped on a fixed tick.
//!
//! Only the simulation's own types cross into `ffi`, which turns them into
//! the snapshot; Swift never sees the world.

use bevy_ecs::prelude::*;
use glam::{Quat, Vec3};

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

/// The camera the world is seen from.
#[derive(Resource, Clone, Copy, Debug, PartialEq)]
pub struct Camera {
    /// Where it is and where it looks.
    pub transform: Transform,
    /// The vertical field of view; the horizontal one grows with the
    /// aspect ratio (Hor+).
    pub vertical_fov_radians: f32,
}

impl Default for Camera {
    fn default() -> Self {
        // (70 degrees vertical: about the stock game's 70 horizontal at 4:3
        // widened for 16:9)
        Self {
            transform: Transform::default(),
            vertical_fov_radians: 70.0_f32.to_radians(),
        }
    }
}

/// The number of ticks run since the world started.
#[derive(Resource, Clone, Copy, Debug, Default, PartialEq, Eq, PartialOrd, Ord)]
pub struct Tick(pub u64);

/// A running world.
pub struct Simulation {
    world: World,
    schedule: Schedule,
}

impl Simulation {
    /// A world with nothing in it but the camera.
    pub fn empty() -> Self {
        let mut world = World::new();
        world.insert_resource(Tick::default());
        world.insert_resource(Camera::default());
        let mut schedule = Schedule::default();
        schedule.add_systems(advance_tick);
        Self { world, schedule }
    }

    /// Runs one tick.
    pub fn step(&mut self) {
        self.schedule.run(&mut self.world);
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
}

fn advance_tick(mut tick: ResMut<Tick>) {
    tick.0 += 1;
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn an_empty_world_counts_its_ticks() {
        let mut simulation = Simulation::empty();
        assert_eq!(simulation.tick(), Tick(0));
        simulation.step();
        simulation.step();
        assert_eq!(simulation.tick(), Tick(2));
    }
}
