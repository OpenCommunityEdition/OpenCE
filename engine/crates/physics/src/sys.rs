//! The C interface to Jolt (port/macos/host/physics/physics.h), declared
//! as it is there. Units are metres; [`crate::World`] converts.

#![allow(unsafe_code)]

/// No body.
pub const NONE: u32 = 0xffff_ffff;

pub const SHAPE_SPHERE: i32 = 0;
pub const SHAPE_CAPSULE: i32 = 1;
pub const SHAPE_BOX: i32 = 2;

pub const MOTION_KINEMATIC: i32 = 0;
pub const MOTION_DYNAMIC: i32 = 1;

pub const CAST_LEVEL: u32 = 1;
pub const CAST_BODIES: u32 = 2;

/// The world, opaque.
#[repr(C)]
pub struct World {
    _private: [u8; 0],
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct Pose {
    pub position: [f32; 3],
    pub rotation: [f32; 4],
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct Shape {
    pub kind: i32,
    pub size: [f32; 3],
    pub offset: Pose,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct Material {
    pub friction: f32,
    pub restitution: f32,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct WorldSettings {
    pub gravity: [f32; 3],
    pub bodies_max: u32,
    pub collision_steps: u32,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct Hit {
    pub fraction: f32,
    pub point: [f32; 3],
    pub normal: [f32; 3],
    pub body: u32,
    pub level: i32,
}

extern "C" {
    pub fn physics_world_create(settings: *const WorldSettings) -> *mut World;
    pub fn physics_world_destroy(world: *mut World);
    pub fn physics_world_set_level(
        world: *mut World,
        vertices: *const f32,
        vertex_count: u32,
        indices: *const u32,
        triangle_count: u32,
    ) -> i32;
    pub fn physics_world_clear_level(world: *mut World);
    pub fn physics_world_step(world: *mut World, seconds: f32) -> i32;
    pub fn physics_world_active_bodies(world: *const World) -> u32;
    pub fn physics_world_bodies(world: *const World) -> u32;

    pub fn physics_body_add(
        world: *mut World,
        shape: *const Shape,
        pose: *const Pose,
        motion: i32,
        mass: f32,
        material: *const Material,
    ) -> u32;
    pub fn physics_body_remove(world: *mut World, body: u32);
    pub fn physics_body_get_pose(world: *const World, body: u32, pose: *mut Pose) -> i32;
    pub fn physics_body_get_velocity(
        world: *const World,
        body: u32,
        linear: *mut f32,
        angular: *mut f32,
    ) -> i32;
    pub fn physics_body_move(world: *mut World, body: u32, pose: *const Pose, seconds: f32) -> i32;
    pub fn physics_body_add_impulse(
        world: *mut World,
        body: u32,
        impulse: *const f32,
        point: *const f32,
    ) -> i32;
    pub fn physics_body_set_velocity(
        world: *mut World,
        body: u32,
        linear: *const f32,
        angular: *const f32,
    ) -> i32;
    pub fn physics_body_active(world: *const World, body: u32) -> i32;

    pub fn physics_world_cast_ray(
        world: *const World,
        origin: *const f32,
        direction: *const f32,
        layers: u32,
        hit: *mut Hit,
    ) -> i32;
    pub fn physics_world_cast_shape(
        world: *const World,
        shape: *const Shape,
        pose: *const Pose,
        direction: *const f32,
        layers: u32,
        hit: *mut Hit,
    ) -> i32;
}
