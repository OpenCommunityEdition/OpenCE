//! The player: a biped moved by Halo's player physics over the level's
//! collision (opence_physics), and the camera that looks out of it or
//! follows it.
//!
//! The physics world is not `Sync`, so it lives beside the bevy world, in
//! [`crate::Simulation`]; each tick moves the biped first, and a system then
//! puts its object and the camera where the step left them.

use bevy_ecs::prelude::*;
use glam::{Quat, Vec2, Vec3};
use opence_content::{Biped, Level, TagRef};
use opence_physics::{
    Layers, LevelMesh, MovementInput, Mover, MoverSettings, PhysicsError, World as PhysicsWorld,
    WorldSettings,
};
use thiserror::Error;

use crate::hash::WorldHasher;
use crate::input::{Buttons, TickInput};
use crate::objects::ObjectHandle;
use crate::units::Radians;
use crate::{FreeCamera, Transform};

/// Where the player sees from.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum View {
    /// Out of the biped's eyes; its own body is not drawn.
    #[default]
    FirstPerson,
    /// Behind and above it, stopped short of walls.
    ThirdPerson,
}

impl View {
    /// The other one.
    pub fn toggled(self) -> View {
        match self {
            View::FirstPerson => View::ThirdPerson,
            View::ThirdPerson => View::FirstPerson,
        }
    }
}

/// How far behind its eyes the third-person camera follows, in world
/// units.
pub const FOLLOW_DISTANCE: f32 = 1.2;
/// How far above them it follows.
pub const FOLLOW_HEIGHT: f32 = 0.25;
/// How far short of a wall the third-person camera stops.
const FOLLOW_MARGIN: f32 = 0.05;

/// Why there is no player.
#[derive(Debug, Error)]
pub enum PlayerError {
    /// The scenario has no player start of that number.
    #[error("the scenario has no player start {0}")]
    NoStart(usize),
    /// The start's structure BSP is not in the level.
    #[error("the level has no structure bsp {0}")]
    NoBsp(usize),
    /// Neither the campaign's nor multiplayer's player unit is a biped in
    /// the level.
    #[error("the level has no player biped")]
    NoBiped,
    /// The world holds no more objects.
    #[error("the world holds no more objects")]
    TableFull,
    /// The physics refused the level or the biped.
    #[error(transparent)]
    Physics(#[from] PhysicsError),
}

/// The player: its biped's movement and where it looks.
pub struct Player {
    physics: PhysicsWorld,
    mover: Mover,
    /// The facing: the turn to the left from +X.
    yaw: f32,
    /// The look up from level.
    pitch: f32,
    view: View,
    /// Its biped's object.
    handle: ObjectHandle,
    /// Its biped's tag.
    unit: TagRef,
}

/// What the tick's systems need of the player: where its object goes and
/// where the camera goes.
#[derive(Resource, Clone, Copy, Debug)]
pub(crate) struct PlayerPose {
    pub handle: ObjectHandle,
    pub body: Transform,
    pub camera: Transform,
}

/// The player's biped tag in `level`: the campaign's player unit, else
/// multiplayer's, whichever the level has as a biped.
fn player_unit(level: &Level) -> Option<(TagRef, Biped)> {
    [&level.globals.player_unit, &level.globals.multiplayer_unit]
        .into_iter()
        .flatten()
        .find_map(|unit| {
            let biped = level.object(unit)?.biped?;
            Some((unit.clone(), biped))
        })
}

impl Player {
    /// A player standing at `level`'s player start `start`, over the
    /// collision of the start's structure BSP; `spawn` makes its biped's
    /// object.
    pub(crate) fn new(
        level: &Level,
        start: usize,
        spawn: impl FnOnce(TagRef, Transform) -> Option<ObjectHandle>,
    ) -> Result<Player, PlayerError> {
        let place = level
            .scenario
            .player_starts
            .get(start)
            .ok_or(PlayerError::NoStart(start))?;
        let bsp = usize::from(place.structure_bsp.unwrap_or(0));
        let structure = level
            .structure_bsps
            .get(bsp)
            .ok_or(PlayerError::NoBsp(bsp))?;
        let (unit, biped) = player_unit(level).ok_or(PlayerError::NoBiped)?;
        let settings = WorldSettings::default();
        let mut physics = PhysicsWorld::new(settings)?;
        physics.set_level(&LevelMesh::from_collision(&structure.collision))?;
        let mover = Mover::new(
            &mut physics,
            MoverSettings {
                biped,
                player: level.globals.player,
                gravity: -settings.gravity.z,
            },
            Vec3::from_array(place.position),
        )?;
        let yaw = libm::remainderf(place.facing, std::f32::consts::TAU);
        let body = body_transform(mover.position, yaw);
        let handle = spawn(unit.clone(), body).ok_or(PlayerError::TableFull)?;
        Ok(Player {
            physics,
            mover,
            yaw,
            pitch: 0.0,
            view: View::default(),
            handle,
            unit,
        })
    }

    /// Its biped's object.
    pub fn handle(&self) -> ObjectHandle {
        self.handle
    }

    /// Its biped's tag.
    pub fn unit(&self) -> &TagRef {
        &self.unit
    }

    /// Its movement: where its feet are, its velocity, its ground.
    pub fn mover(&self) -> &Mover {
        &self.mover
    }

    /// Where it sees from.
    pub fn view(&self) -> View {
        self.view
    }

    pub(crate) fn set_view(&mut self, view: View) {
        self.view = view;
    }

    /// Its facing and look.
    pub fn look(&self) -> (Radians, Radians) {
        (Radians(self.yaw), Radians(self.pitch))
    }

    /// One tick of `input`: the mouse's turn, then the move.
    pub(crate) fn step(&mut self, input: &TickInput) -> PlayerPose {
        let max = FreeCamera::MAX_PITCH.0;
        self.yaw = libm::remainderf(self.yaw + input.yaw.0, std::f32::consts::TAU);
        self.pitch = (self.pitch + input.pitch.0).clamp(-max, max);
        let axis = |value: i8| f32::from(value) / 127.0;
        // (forward and left; no faster on a diagonal)
        let throttle = Vec2::new(axis(input.forward), -axis(input.right));
        let throttle = if throttle.length_squared() > 1.0 {
            throttle.normalize()
        } else {
            throttle
        };
        let movement = MovementInput {
            throttle,
            yaw: self.yaw,
            jump: input.buttons.contains(Buttons::JUMP),
            crouch: input.buttons.contains(Buttons::CROUCH),
        };
        // (the physics refuses only numbers that are not finite, which the
        // input never holds: a refused step leaves the biped where it was)
        let _ = self.mover.step(&mut self.physics, &movement);
        self.pose()
    }

    /// Where its object and the camera are now.
    pub(crate) fn pose(&self) -> PlayerPose {
        PlayerPose {
            handle: self.handle,
            body: body_transform(self.mover.position, self.yaw),
            camera: self.camera(),
        }
    }

    /// The camera: at its eyes, or following behind them.
    fn camera(&self) -> Transform {
        let rotation = look_rotation(self.yaw, self.pitch);
        let eyes = self.mover.position + Vec3::Z * self.mover.camera_height();
        let position = match self.view {
            View::FirstPerson => eyes,
            View::ThirdPerson => {
                let back =
                    rotation * Vec3::new(-FOLLOW_DISTANCE, 0.0, 0.0) + Vec3::Z * FOLLOW_HEIGHT;
                let reach = match self.physics.cast_ray(eyes, back, Layers::LEVEL) {
                    Ok(Some(hit)) => (hit.fraction - FOLLOW_MARGIN / back.length()).max(0.0),
                    _ => 1.0,
                };
                eyes + back * reach
            }
        };
        Transform {
            position,
            rotation,
            scale: 1.0,
        }
    }

    /// Its state, for the world's hash.
    pub(crate) fn hash(&self, hasher: &mut WorldHasher) {
        let mover = &self.mover;
        for value in mover
            .position
            .to_array()
            .into_iter()
            .chain(mover.velocity.to_array())
            .chain([mover.crouch, self.yaw, self.pitch])
        {
            hasher.f32(value);
        }
        hasher.u64(u64::from(mover.airborne));
        hasher.u64(match self.view {
            View::FirstPerson => 0,
            View::ThirdPerson => 1,
        });
    }
}

/// The biped's object: its feet, turned to its facing.
fn body_transform(position: Vec3, yaw: f32) -> Transform {
    Transform {
        position,
        rotation: Quat::from_rotation_z(yaw),
        scale: 1.0,
    }
}

/// A look's orientation: a yaw about Z, then a pitch about the camera's Y
/// (the free camera's).
fn look_rotation(yaw: f32, pitch: f32) -> Quat {
    Quat::from_rotation_z(yaw) * Quat::from_rotation_y(-pitch)
}

/// Puts the player's object where its step left it, before the animations
/// pose its nodes from there.
pub(crate) fn move_player(
    pose: Option<Res<PlayerPose>>,
    table: Res<crate::objects::ObjectTable>,
    mut transforms: Query<&mut Transform>,
) {
    let Some(pose) = pose else { return };
    let Some(entity) = table.entity(pose.handle) else {
        return;
    };
    if let Ok(mut transform) = transforms.get_mut(entity) {
        *transform = pose.body;
    }
}
