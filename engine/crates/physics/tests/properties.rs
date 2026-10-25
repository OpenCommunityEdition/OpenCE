//! Properties the physics keeps over the speeds and heights the game
//! reaches: a body thrown at a wall stays on its side of it (no tunnelling
//! through it in one step), and a body falling and bouncing on the floor
//! never gains energy.

use glam::Vec3;
use opence_content::level::{CollisionEdge, CollisionSurface, CollisionVertex};
use opence_content::CollisionBsp;
use opence_physics::{LevelMesh, Material, Motion, Pose, Shape, World, WorldSettings};
use proptest::prelude::*;
use proptest::test_runner::{Config, RngSeed};

const TICK: f32 = 1.0 / 60.0;

/// The fastest a body is thrown: 2 units a tick at 30 Hz, past a
/// Banshee's boost and a grenade's throw.
const FASTEST: f32 = 60.0;

/// Adds a one-sided surface whose ring is `points`, in order.
fn add_surface(bsp: &mut CollisionBsp, points: &[[f32; 3]], plane: [f32; 4]) {
    let surface = bsp.surfaces.len() as u32;
    let first_vertex = bsp.vertices.len() as u32;
    let first_edge = bsp.edges.len() as u32;
    let count = points.len() as u32;
    bsp.planes.push(plane);
    for (index, &point) in points.iter().enumerate() {
        let index = index as u32;
        bsp.vertices.push(CollisionVertex {
            point,
            first_edge: first_edge + index,
        });
        bsp.edges.push(CollisionEdge {
            vertices: [first_vertex + index, first_vertex + (index + 1) % count],
            edges: [first_edge + (index + 1) % count, 0],
            surfaces: [Some(surface), None],
        });
    }
    bsp.surfaces.push(CollisionSurface {
        plane: (bsp.planes.len() - 1) as i32,
        first_edge,
        flags: 0,
        breakable_surface: 0,
        material: None,
    });
}

/// A floor 200 units square at z = 0 and a wall across it at x = 5,
/// facing back toward the origin.
fn room() -> CollisionBsp {
    let mut bsp = CollisionBsp::default();
    add_surface(
        &mut bsp,
        &[
            [-100.0, -100.0, 0.0],
            [100.0, -100.0, 0.0],
            [100.0, 100.0, 0.0],
            [-100.0, 100.0, 0.0],
        ],
        [0.0, 0.0, 1.0, 0.0],
    );
    add_surface(
        &mut bsp,
        &[
            [5.0, -100.0, 0.0],
            [5.0, 100.0, 0.0],
            [5.0, 100.0, 50.0],
            [5.0, -100.0, 50.0],
        ],
        [-1.0, 0.0, 0.0, -5.0],
    );
    bsp
}

fn world() -> Result<World, TestCaseError> {
    let fail = |error: opence_physics::PhysicsError| TestCaseError::fail(error.to_string());
    let mut world = World::new(WorldSettings::default()).map_err(fail)?;
    world
        .set_level(&LevelMesh::from_collision(&room()))
        .map_err(fail)?;
    Ok(world)
}

/// A fixed seed, so that each run tries the same cases.
fn config(cases: u32) -> Config {
    Config {
        cases,
        rng_seed: RngSeed::Fixed(0x0CE0_0CE0),
        failure_persistence: None,
        ..Config::default()
    }
}

fn shape(sphere: bool, size: f32) -> Shape {
    if sphere {
        Shape::Sphere { radius: size }
    } else {
        Shape::Box {
            half_extents: Vec3::splat(size),
        }
    }
}

proptest! {
    #![proptest_config(config(48))]

    #[test]
    fn a_body_thrown_at_the_wall_stays_on_its_side(
        sphere in any::<bool>(),
        size in 0.05f32..0.5,
        speed in 1.0f32..FASTEST,
        across in -0.5f32..0.5,
        up in -0.2f32..0.4,
    ) {
        let fail = |error: opence_physics::PhysicsError| TestCaseError::fail(error.to_string());
        let mut world = world()?;
        let body = world
            .add_body(shape(sphere, size), Pose::at(Vec3::new(0.0, 0.0, 2.0)), Motion::Dynamic, 1.0,
                      Material::default())
            .map_err(fail)?;
        let direction = Vec3::new(1.0, across, up).normalize();
        world.set_body_velocity(body, direction * speed, Vec3::ZERO).map_err(fail)?;
        for tick in 0..120 {
            world.step(TICK).map_err(fail)?;
            let pose = world.body_pose(body).map_err(fail)?;
            prop_assert!(pose.position.x < 5.0, "through the wall at tick {tick}: {}", pose.position);
            prop_assert!(pose.position.z > 0.0, "through the floor at tick {tick}: {}", pose.position);
        }
    }

    #[test]
    fn a_body_falling_and_bouncing_never_gains_energy(
        sphere in any::<bool>(),
        size in 0.05f32..0.5,
        height in 0.5f32..20.0,
        restitution in 0.0f32..1.0,
        thrown in -3.0f32..3.0,
    ) {
        let fail = |error: opence_physics::PhysicsError| TestCaseError::fail(error.to_string());
        let mut world = world()?;
        let gravity = -WorldSettings::default().gravity.z;
        let material = Material { friction: 0.5, restitution };
        let body = world
            .add_body(shape(sphere, size), Pose::at(Vec3::new(-20.0, 0.0, height)), Motion::Dynamic, 1.0,
                      material)
            .map_err(fail)?;
        world.set_body_velocity(body, Vec3::new(0.0, 0.0, thrown), Vec3::ZERO).map_err(fail)?;
        // (per unit of mass; the spin's energy left out, so a body only
        // loses what it trades into spinning)
        let energy = |world: &World| -> Result<f32, TestCaseError> {
            let pose = world.body_pose(body).map_err(fail)?;
            let (velocity, _) = world.body_velocity(body).map_err(fail)?;
            Ok(0.5 * velocity.length_squared() + gravity * pose.position.z)
        };
        let start = energy(&world)?;
        let mut most = start;
        for _ in 0..240 {
            world.step(TICK).map_err(fail)?;
            most = most.max(energy(&world)?);
        }
        // (a step's integration may add a hair: 1% and a little more)
        prop_assert!(most <= start * 1.01 + 0.05, "from {start} to {most}");
    }
}
