//! The engine's physics: Jolt Physics (port/third_party/jolt) through the
//! C interface the macOS host uses (port/macos/host/physics/physics.h),
//! built into the crate by build.rs.
//!
//! A [`World`] holds a level's collision mesh, made from its collision BSP
//! ([`LevelMesh`]), and bodies: kinematic ones the simulation moves and
//! dynamic ones Jolt moves. It answers rays and shape casts. Everything here
//! is in world units (the game's: z up, 1 unit is 3.048 m), converted to
//! Jolt's metres at the boundary. Jolt runs in its cross-platform
//! deterministic mode: the same calls give the same results on every run.

#![allow(unsafe_code)]

mod ground;
mod mesh;
pub mod movement;
mod ragdoll;
mod sys;

use std::ptr::NonNull;

use glam::{Quat, Vec3};

pub use ground::{SurfaceMaterials, SURFACE_REACH};
pub use mesh::LevelMesh;
pub use movement::{Landing, MovementInput, Mover, MoverSettings};
pub use ragdoll::{shape_volume, RagdollId, RagdollPart, METRES_PER_UNIT_CUBED, RAGDOLL_PARTS_MAX};

/// Metres in a world unit (port/macos/include/host_physics.h's).
pub const METRES_PER_UNIT: f32 = 3.048;

/// What the physics refused.
#[derive(Clone, Copy, Debug, PartialEq, Eq, thiserror::Error)]
pub enum PhysicsError {
    /// The world could not be made (settings out of range).
    #[error("the physics world could not be made")]
    World,
    /// The level's mesh was refused (an index out of range, a vertex not
    /// finite, or no triangle left).
    #[error("the level's collision mesh was refused")]
    Level,
    /// A body's shape, pose, mass or material was refused, or the world is
    /// full.
    #[error("the body was refused")]
    Body,
    /// A handle that names no body, or a number that is not finite.
    #[error("the call was refused")]
    Refused,
}

/// A world's settings.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct WorldSettings {
    /// Gravity, in world units a second squared.
    pub gravity: Vec3,
    /// The most bodies at once.
    pub bodies_max: u32,
    /// Collision steps a [`World::step`] takes.
    pub collision_steps: u32,
}

impl Default for WorldSettings {
    fn default() -> Self {
        // (Halo's: 0.003565 units a 30 Hz tick squared, measured in the
        // reference game's traces)
        WorldSettings {
            gravity: Vec3::new(0.0, 0.0, -0.003565 * 900.0),
            bodies_max: 4096,
            collision_steps: 1,
        }
    }
}

/// A body's shape, in world units, in its body's frame.
#[derive(Clone, Copy, Debug, PartialEq)]
pub enum Shape {
    /// A sphere, by its radius.
    Sphere {
        /// Its radius.
        radius: f32,
    },
    /// A capsule along the z axis (a biped standing), by its radius and
    /// its cylinder's height.
    Capsule {
        /// Its radius.
        radius: f32,
        /// Its cylinder's height (the whole capsule's, less the two caps).
        height: f32,
    },
    /// A box, by its half extents.
    Box {
        /// Half its size on each axis.
        half_extents: Vec3,
    },
}

impl Shape {
    fn raw(&self) -> sys::Shape {
        let identity = sys::Pose {
            position: [0.0; 3],
            rotation: [0.0, 0.0, 0.0, 1.0],
        };
        match *self {
            Shape::Sphere { radius } => sys::Shape {
                kind: sys::SHAPE_SPHERE,
                size: [radius * METRES_PER_UNIT, 0.0, 0.0],
                offset: identity,
            },
            Shape::Capsule { radius, height } => {
                // (the wrapper's capsules lie along x: turned a quarter about y)
                let turn = Quat::from_rotation_y(std::f32::consts::FRAC_PI_2);
                sys::Shape {
                    kind: sys::SHAPE_CAPSULE,
                    size: [
                        radius * METRES_PER_UNIT,
                        0.5 * height * METRES_PER_UNIT,
                        0.0,
                    ],
                    offset: sys::Pose {
                        position: [0.0; 3],
                        rotation: turn.to_array(),
                    },
                }
            }
            Shape::Box { half_extents } => sys::Shape {
                kind: sys::SHAPE_BOX,
                size: (half_extents * METRES_PER_UNIT).to_array(),
                offset: identity,
            },
        }
    }
}

/// Where a body is, in world units.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Pose {
    /// Its position.
    pub position: Vec3,
    /// Its rotation.
    pub rotation: Quat,
}

impl Pose {
    /// A pose at a position, unturned.
    pub fn at(position: Vec3) -> Pose {
        Pose {
            position,
            rotation: Quat::IDENTITY,
        }
    }

    fn raw(&self) -> sys::Pose {
        sys::Pose {
            position: (self.position * METRES_PER_UNIT).to_array(),
            rotation: self.rotation.to_array(),
        }
    }

    fn from_raw(raw: &sys::Pose) -> Pose {
        Pose {
            position: Vec3::from_array(raw.position) / METRES_PER_UNIT,
            rotation: Quat::from_array(raw.rotation),
        }
    }
}

/// How a body moves.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Motion {
    /// Moved by the simulation each step ([`World::move_body`]), pushing
    /// what it meets and carrying the characters that stand on it: a
    /// platform ([`Layers::PLATFORMS`]).
    Kinematic,
    /// Moved by the world.
    Dynamic,
}

/// A body's surface.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Material {
    /// Its friction.
    pub friction: f32,
    /// Its bounce, 0 to 1.
    pub restitution: f32,
}

impl Default for Material {
    fn default() -> Self {
        Material {
            friction: 0.5,
            restitution: 0.0,
        }
    }
}

/// A body, by its handle.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub struct BodyId(u32);

/// What a cast may hit.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Layers(u32);

impl Layers {
    /// The level's mesh.
    pub const LEVEL: Layers = Layers(sys::CAST_LEVEL);
    /// Dynamic bodies: ragdoll parts and debris.
    pub const BODIES: Layers = Layers(sys::CAST_BODIES);
    /// Kinematic bodies: the lifts and doors characters stand on.
    pub const PLATFORMS: Layers = Layers(sys::CAST_PLATFORMS);
    /// The level and its platforms, what a character stands on.
    pub const GROUND: Layers = Layers(sys::CAST_LEVEL | sys::CAST_PLATFORMS);
    /// Everything.
    pub const ALL: Layers = Layers(sys::CAST_LEVEL | sys::CAST_BODIES | sys::CAST_PLATFORMS);
}

/// What a body met.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Struck {
    /// The level's mesh.
    Level,
    /// A body.
    Body(BodyId),
    /// Something else (a ragdoll's part).
    Other,
}

/// A cast's first hit.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Hit {
    /// How far along the cast, 0 to 1.
    pub fraction: f32,
    /// Where, in world units.
    pub point: Vec3,
    /// The surface's normal, toward the cast's start.
    pub normal: Vec3,
    /// What was hit.
    pub struck: Struck,
}

impl Hit {
    fn from_raw(raw: &sys::Hit) -> Hit {
        let struck = if raw.level != 0 {
            Struck::Level
        } else if raw.body != sys::NONE {
            Struck::Body(BodyId(raw.body))
        } else {
            Struck::Other
        };
        Hit {
            fraction: raw.fraction,
            point: Vec3::from_array(raw.point) / METRES_PER_UNIT,
            normal: Vec3::from_array(raw.normal),
            struck,
        }
    }
}

/// A sphere's contact with a surface it sinks into.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Contact {
    /// How far it sinks, along the normal.
    pub depth: f32,
    /// The deepest point it meets, in world units.
    pub point: Vec3,
    /// The surface's normal, out of the surface.
    pub normal: Vec3,
    /// What it touches.
    pub struck: Struck,
}

/// A physics world.
pub struct World {
    raw: NonNull<sys::World>,
}

// SAFETY: the world is Jolt's and its own threads'; it is used from one
// thread at a time (`&mut` to change it).
unsafe impl Send for World {}

fn units_to_metres(vector: Vec3) -> [f32; 3] {
    (vector * METRES_PER_UNIT).to_array()
}

impl World {
    /// A world, empty.
    pub fn new(settings: WorldSettings) -> Result<World, PhysicsError> {
        let raw = sys::WorldSettings {
            gravity: units_to_metres(settings.gravity),
            bodies_max: settings.bodies_max,
            collision_steps: settings.collision_steps,
        };
        // SAFETY: the settings are a valid pointer for the call.
        let world = unsafe { sys::physics_world_create(&raw) };
        NonNull::new(world)
            .map(|raw| World { raw })
            .ok_or(PhysicsError::World)
    }

    /// Replaces the level's collision mesh.
    pub fn set_level(&mut self, mesh: &LevelMesh) -> Result<(), PhysicsError> {
        let vertices: Vec<f32> = mesh
            .vertices
            .iter()
            .flat_map(|vertex| vertex.map(|v| v * METRES_PER_UNIT))
            .collect();
        let (Ok(vertex_count), Ok(triangle_count)) = (
            u32::try_from(mesh.vertices.len()),
            u32::try_from(mesh.triangles.len()),
        ) else {
            return Err(PhysicsError::Level);
        };
        // SAFETY: the arrays hold the counts given, and the call copies them.
        let result = unsafe {
            sys::physics_world_set_level(
                self.raw.as_ptr(),
                vertices.as_ptr(),
                vertex_count,
                mesh.triangles.as_ptr().cast(),
                triangle_count,
            )
        };
        if result == 0 {
            Ok(())
        } else {
            Err(PhysicsError::Level)
        }
    }

    /// Takes the level's collision mesh away.
    pub fn clear_level(&mut self) {
        // SAFETY: the world is live.
        unsafe { sys::physics_world_clear_level(self.raw.as_ptr()) }
    }

    /// Steps the world by `seconds`.
    pub fn step(&mut self, seconds: f32) -> Result<(), PhysicsError> {
        // SAFETY: the world is live.
        let result = unsafe { sys::physics_world_step(self.raw.as_ptr(), seconds) };
        if result == 0 {
            Ok(())
        } else {
            Err(PhysicsError::Refused)
        }
    }

    /// The bodies, and those awake.
    pub fn bodies(&self) -> (u32, u32) {
        // SAFETY: the world is live.
        unsafe {
            (
                sys::physics_world_bodies(self.raw.as_ptr()),
                sys::physics_world_active_bodies(self.raw.as_ptr()),
            )
        }
    }

    /// Adds a body. `mass` is in kilograms, positive (a kinematic body's
    /// too: what it pushes).
    pub fn add_body(
        &mut self,
        shape: Shape,
        pose: Pose,
        motion: Motion,
        mass: f32,
        material: Material,
    ) -> Result<BodyId, PhysicsError> {
        let shape = shape.raw();
        let pose = pose.raw();
        let material = sys::Material {
            friction: material.friction,
            restitution: material.restitution,
        };
        let motion = match motion {
            Motion::Kinematic => sys::MOTION_KINEMATIC,
            Motion::Dynamic => sys::MOTION_DYNAMIC,
        };
        // SAFETY: the pointers are valid for the call, which copies them.
        let body = unsafe {
            sys::physics_body_add(self.raw.as_ptr(), &shape, &pose, motion, mass, &material)
        };
        if body == sys::NONE {
            Err(PhysicsError::Body)
        } else {
            Ok(BodyId(body))
        }
    }

    /// Removes a body (nothing, if it names none).
    pub fn remove_body(&mut self, body: BodyId) {
        // SAFETY: the world is live; the handle is checked.
        unsafe { sys::physics_body_remove(self.raw.as_ptr(), body.0) }
    }

    /// A body's pose.
    pub fn body_pose(&self, body: BodyId) -> Result<Pose, PhysicsError> {
        let mut pose = sys::Pose::default();
        // SAFETY: the pose is valid for the call.
        if unsafe { sys::physics_body_get_pose(self.raw.as_ptr(), body.0, &mut pose) } == 0 {
            Ok(Pose::from_raw(&pose))
        } else {
            Err(PhysicsError::Refused)
        }
    }

    /// A body's velocity, in world units a second, and its turning, in
    /// radians a second.
    pub fn body_velocity(&self, body: BodyId) -> Result<(Vec3, Vec3), PhysicsError> {
        let mut linear = [0.0f32; 3];
        let mut angular = [0.0f32; 3];
        // SAFETY: the arrays hold three each.
        let result = unsafe {
            sys::physics_body_get_velocity(
                self.raw.as_ptr(),
                body.0,
                linear.as_mut_ptr(),
                angular.as_mut_ptr(),
            )
        };
        if result == 0 {
            Ok((
                Vec3::from_array(linear) / METRES_PER_UNIT,
                Vec3::from_array(angular),
            ))
        } else {
            Err(PhysicsError::Refused)
        }
    }

    /// Moves a kinematic body to reach `pose` by the end of a step of
    /// `seconds`.
    pub fn move_body(
        &mut self,
        body: BodyId,
        pose: Pose,
        seconds: f32,
    ) -> Result<(), PhysicsError> {
        let pose = pose.raw();
        // SAFETY: the pose is valid for the call.
        let result = unsafe { sys::physics_body_move(self.raw.as_ptr(), body.0, &pose, seconds) };
        if result == 0 {
            Ok(())
        } else {
            Err(PhysicsError::Refused)
        }
    }

    /// Sets a body's velocity (world units a second) and turning (radians a
    /// second).
    pub fn set_body_velocity(
        &mut self,
        body: BodyId,
        linear: Vec3,
        angular: Vec3,
    ) -> Result<(), PhysicsError> {
        let linear = units_to_metres(linear);
        let angular = angular.to_array();
        // SAFETY: the arrays hold three each.
        let result = unsafe {
            sys::physics_body_set_velocity(
                self.raw.as_ptr(),
                body.0,
                linear.as_ptr(),
                angular.as_ptr(),
            )
        };
        if result == 0 {
            Ok(())
        } else {
            Err(PhysicsError::Refused)
        }
    }

    /// Pushes a body: an impulse in kilogram world units a second, at a
    /// point in the world.
    pub fn push_body(
        &mut self,
        body: BodyId,
        impulse: Vec3,
        point: Vec3,
    ) -> Result<(), PhysicsError> {
        let impulse = units_to_metres(impulse);
        let point = units_to_metres(point);
        // SAFETY: the arrays hold three each.
        let result = unsafe {
            sys::physics_body_add_impulse(
                self.raw.as_ptr(),
                body.0,
                impulse.as_ptr(),
                point.as_ptr(),
            )
        };
        if result == 0 {
            Ok(())
        } else {
            Err(PhysicsError::Refused)
        }
    }

    /// Whether a body is awake.
    pub fn body_awake(&self, body: BodyId) -> Result<bool, PhysicsError> {
        // SAFETY: the world is live; the handle is checked.
        match unsafe { sys::physics_body_active(self.raw.as_ptr(), body.0) } {
            1 => Ok(true),
            0 => Ok(false),
            _ => Err(PhysicsError::Refused),
        }
    }

    /// The first surface on the segment from `origin` to `origin +
    /// direction`.
    pub fn cast_ray(
        &self,
        origin: Vec3,
        direction: Vec3,
        layers: Layers,
    ) -> Result<Option<Hit>, PhysicsError> {
        let origin = units_to_metres(origin);
        let direction = units_to_metres(direction);
        let mut hit = sys::Hit::default();
        // SAFETY: the arrays hold three each, and the hit is valid for the call.
        let result = unsafe {
            sys::physics_world_cast_ray(
                self.raw.as_ptr(),
                origin.as_ptr(),
                direction.as_ptr(),
                layers.0,
                &mut hit,
            )
        };
        Self::cast_result(result, &hit)
    }

    /// The first surface `shape` meets moved from `pose` by `direction`.
    pub fn cast_shape(
        &self,
        shape: Shape,
        pose: Pose,
        direction: Vec3,
        layers: Layers,
    ) -> Result<Option<Hit>, PhysicsError> {
        let shape = shape.raw();
        let pose = pose.raw();
        let direction = units_to_metres(direction);
        let mut hit = sys::Hit::default();
        // SAFETY: the pointers are valid for the call.
        let result = unsafe {
            sys::physics_world_cast_shape(
                self.raw.as_ptr(),
                &shape,
                &pose,
                direction.as_ptr(),
                layers.0,
                &mut hit,
            )
        };
        Self::cast_result(result, &hit)
    }

    /// How far a sphere at `center` sinks into what `layers` hold, if it
    /// touches at all: the deepest point it meets, the surface's normal
    /// there (out of the surface, toward the sphere), and the depth along
    /// it (a vehicle's mass point on the ground).
    pub fn sphere_contact(
        &self,
        center: Vec3,
        radius: f32,
        layers: Layers,
    ) -> Result<Option<Contact>, PhysicsError> {
        // (a cast too short to move it, so that the hit is what it overlaps
        // where it is, with the deepest point)
        let hit = self.cast_shape(
            Shape::Sphere { radius },
            Pose::at(center),
            Vec3::new(0.0, 0.0, -1e-3),
            layers,
        )?;
        Ok(hit.and_then(|hit| {
            let depth = radius - (center - hit.point).dot(hit.normal);
            (depth > 0.0).then_some(Contact {
                depth,
                point: hit.point,
                normal: hit.normal,
                struck: hit.struck,
            })
        }))
    }

    fn cast_result(result: i32, hit: &sys::Hit) -> Result<Option<Hit>, PhysicsError> {
        match result {
            1 => Ok(Some(Hit::from_raw(hit))),
            0 => Ok(None),
            _ => Err(PhysicsError::Refused),
        }
    }
}

/// A character: a biped's pill as it walks, the level sliding it along.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub struct CharacterId(u32);

/// A character's pill and how it walks, in world units.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct CharacterSettings {
    /// Its radius.
    pub radius: f32,
    /// Its height, from its feet to its top.
    pub height: f32,
    /// The steepest ground it stands on (radians).
    pub maximum_slope: f32,
    /// The highest ledge it steps up onto.
    pub step_up: f32,
    /// How far it follows the floor down as it walks off a step.
    pub stick_down: f32,
    /// What it collides with.
    pub layers: Layers,
}

/// What holds a character up.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Ground {
    /// Ground it stands on.
    Standing,
    /// Ground too steep to stand on: it slides.
    Steep,
    /// Ground that touches it without holding it up.
    Unsupported,
    /// Nothing.
    Airborne,
}

/// A character's state, in world units.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct CharacterState {
    /// Its feet.
    pub position: Vec3,
    /// Its velocity, as the collisions left it (units a second).
    pub velocity: Vec3,
    /// Its ground's normal (up, airborne).
    pub ground_normal: Vec3,
    /// What holds it up.
    pub ground: Ground,
    /// The velocity of the platform it stood on, which its last move added
    /// to the velocity it was given (units a second).
    pub ground_velocity: Vec3,
}

impl CharacterState {
    fn from_raw(raw: &sys::CharacterState) -> CharacterState {
        CharacterState {
            position: Vec3::from_array(raw.position) / METRES_PER_UNIT,
            velocity: Vec3::from_array(raw.velocity) / METRES_PER_UNIT,
            ground_normal: Vec3::from_array(raw.ground_normal),
            ground: match raw.ground {
                sys::GROUND_STANDING => Ground::Standing,
                sys::GROUND_STEEP => Ground::Steep,
                sys::GROUND_UNSUPPORTED => Ground::Unsupported,
                _ => Ground::Airborne,
            },
            ground_velocity: Vec3::from_array(raw.ground_velocity) / METRES_PER_UNIT,
        }
    }
}

impl World {
    /// A character with its feet at `position`.
    pub fn add_character(
        &mut self,
        settings: &CharacterSettings,
        position: Vec3,
    ) -> Result<CharacterId, PhysicsError> {
        let raw = sys::CharacterSettings {
            radius: settings.radius * METRES_PER_UNIT,
            height: settings.height * METRES_PER_UNIT,
            maximum_slope: settings.maximum_slope,
            step_up: settings.step_up * METRES_PER_UNIT,
            stick_down: settings.stick_down * METRES_PER_UNIT,
            layers: settings.layers.0,
        };
        let position = units_to_metres(position);
        // SAFETY: the settings and the position are valid for the call.
        let character =
            unsafe { sys::physics_character_add(self.raw.as_ptr(), &raw, position.as_ptr()) };
        if character == sys::NONE {
            Err(PhysicsError::Body)
        } else {
            Ok(CharacterId(character))
        }
    }

    /// Removes a character (nothing, if it names none).
    pub fn remove_character(&mut self, character: CharacterId) {
        // SAFETY: the world is live; the handle is checked.
        unsafe { sys::physics_character_remove(self.raw.as_ptr(), character.0) }
    }

    /// Moves a character at `velocity` (units a second) for `seconds`,
    /// colliding, stepping up and following the floor.
    pub fn move_character(
        &mut self,
        character: CharacterId,
        velocity: Vec3,
        seconds: f32,
    ) -> Result<CharacterState, PhysicsError> {
        let velocity = units_to_metres(velocity);
        let mut state = sys::CharacterState::default();
        // SAFETY: the velocity and the state are valid for the call.
        let result = unsafe {
            sys::physics_character_move(
                self.raw.as_ptr(),
                character.0,
                velocity.as_ptr(),
                seconds,
                &mut state,
            )
        };
        if result == 0 {
            Ok(CharacterState::from_raw(&state))
        } else {
            Err(PhysicsError::Refused)
        }
    }

    /// Puts a character's feet at `position`, standing still.
    pub fn set_character_position(
        &mut self,
        character: CharacterId,
        position: Vec3,
    ) -> Result<(), PhysicsError> {
        let position = units_to_metres(position);
        // SAFETY: the position is valid for the call.
        let result = unsafe {
            sys::physics_character_set_position(self.raw.as_ptr(), character.0, position.as_ptr())
        };
        if result == 0 {
            Ok(())
        } else {
            Err(PhysicsError::Refused)
        }
    }

    /// A character's state.
    pub fn character_state(&self, character: CharacterId) -> Result<CharacterState, PhysicsError> {
        let mut state = sys::CharacterState::default();
        // SAFETY: the state is valid for the call.
        if unsafe { sys::physics_character_get_state(self.raw.as_ptr(), character.0, &mut state) }
            == 0
        {
            Ok(CharacterState::from_raw(&state))
        } else {
            Err(PhysicsError::Refused)
        }
    }

    /// Changes a character's height (crouching or standing): whether it
    /// changed (not, beneath a ceiling with no room).
    pub fn set_character_height(
        &mut self,
        character: CharacterId,
        height: f32,
    ) -> Result<bool, PhysicsError> {
        // SAFETY: the world is live; the handle is checked.
        match unsafe {
            sys::physics_character_set_height(
                self.raw.as_ptr(),
                character.0,
                height * METRES_PER_UNIT,
            )
        } {
            1 => Ok(true),
            0 => Ok(false),
            _ => Err(PhysicsError::Refused),
        }
    }
}

impl Drop for World {
    fn drop(&mut self) {
        // SAFETY: the world is live and not used again.
        unsafe { sys::physics_world_destroy(self.raw.as_ptr()) }
    }
}
