//! The physics world over a collision BSP: a falling box comes to rest on
//! the floor, rays and shape casts hit the surfaces they should, and two
//! runs of the same calls are identical. The last test reads
//! bloodgulch's collision BSP, and passes without assets/maps/bloodgulch.map.

use std::error::Error;

use glam::{Quat, Vec3};
use opence_content::level::{CollisionEdge, CollisionSurface, CollisionVertex};
use opence_content::CollisionBsp;
use opence_import::testing::stock_level;
use opence_physics::{
    Layers, LevelMesh, Material, Motion, PhysicsError, Pose, Shape, Struck, World, WorldSettings,
};

const TICK: f32 = 1.0 / 60.0;

/// Adds a surface whose ring is `points`, in order, with no neighbours.
fn add_surface(bsp: &mut CollisionBsp, points: &[[f32; 3]], plane: [f32; 4], flags: u8) {
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
        flags,
        breakable_surface: 0,
        material: None,
    });
}

/// A floor 20 units square at z = 0 and a wall across it at x = 5,
/// facing back toward the origin.
fn room() -> CollisionBsp {
    let mut bsp = CollisionBsp::default();
    add_surface(
        &mut bsp,
        &[
            [-10.0, -10.0, 0.0],
            [10.0, -10.0, 0.0],
            [10.0, 10.0, 0.0],
            [-10.0, 10.0, 0.0],
        ],
        [0.0, 0.0, 1.0, 0.0],
        0,
    );
    // (its ring wound the other way: the plane says which side is its front)
    add_surface(
        &mut bsp,
        &[
            [5.0, -10.0, 0.0],
            [5.0, 10.0, 0.0],
            [5.0, 10.0, 10.0],
            [5.0, -10.0, 10.0],
        ],
        [-1.0, 0.0, 0.0, -5.0],
        0,
    );
    bsp
}

fn world() -> Result<World, PhysicsError> {
    let mut world = World::new(WorldSettings::default())?;
    world.set_level(&LevelMesh::from_collision(&room()))?;
    Ok(world)
}

#[test]
fn surfaces_become_fans_facing_as_their_planes() {
    let mut bsp = room();
    let mesh = LevelMesh::from_collision(&bsp);
    assert_eq!(mesh.triangles.len(), 4);
    for triangle in &mesh.triangles {
        let [a, b, c] = triangle.map(|index| Vec3::from_array(mesh.vertices[index as usize]));
        let normal = (b - a).cross(c - a).normalize();
        assert!(
            normal.abs_diff_eq(Vec3::Z, 1e-6) || normal.abs_diff_eq(Vec3::NEG_X, 1e-6),
            "{normal}"
        );
    }
    // a two-sided surface has both windings
    bsp.surfaces[1].flags = 1;
    assert_eq!(LevelMesh::from_collision(&bsp).triangles.len(), 6);
    // a broken ring is left out
    bsp.edges[0].edges[0] = 99;
    assert_eq!(LevelMesh::from_collision(&bsp).triangles.len(), 4);
}

#[test]
fn a_falling_box_comes_to_rest_on_the_floor() -> Result<(), PhysicsError> {
    let mut world = world()?;
    let half = 0.25;
    let shape = Shape::Box {
        half_extents: Vec3::splat(half),
    };
    let body = world.add_body(
        shape,
        Pose::at(Vec3::new(0.0, 0.0, 3.0)),
        Motion::Dynamic,
        10.0,
        Material::default(),
    )?;
    for _ in 0..(4.0 / TICK) as usize {
        world.step(TICK)?;
    }
    let pose = world.body_pose(body)?;
    assert!(
        (pose.position.z - half).abs() < 0.02,
        "rests at {}",
        pose.position
    );
    assert!(
        pose.position.truncate().length() < 0.05,
        "slid to {}",
        pose.position
    );
    let (velocity, _) = world.body_velocity(body)?;
    assert!(velocity.length() < 0.01, "still moving at {velocity}");
    Ok(())
}

#[test]
fn rays_hit_the_floor_and_the_wall() -> Result<(), PhysicsError> {
    let world = world()?;
    let down = world.cast_ray(
        Vec3::new(1.0, 2.0, 5.0),
        Vec3::new(0.0, 0.0, -10.0),
        Layers::ALL,
    )?;
    let down = down.ok_or(PhysicsError::Refused)?;
    assert_eq!(down.struck, Struck::Level);
    assert!((down.fraction - 0.5).abs() < 1e-4, "{}", down.fraction);
    assert!(
        down.point.abs_diff_eq(Vec3::new(1.0, 2.0, 0.0), 1e-3),
        "{}",
        down.point
    );
    assert!(down.normal.abs_diff_eq(Vec3::Z, 1e-4), "{}", down.normal);

    let ahead = world.cast_ray(
        Vec3::new(0.0, 0.0, 1.0),
        Vec3::new(10.0, 0.0, 0.0),
        Layers::ALL,
    )?;
    let ahead = ahead.ok_or(PhysicsError::Refused)?;
    assert!(
        ahead.point.abs_diff_eq(Vec3::new(5.0, 0.0, 1.0), 1e-3),
        "{}",
        ahead.point
    );
    assert!(
        ahead.normal.abs_diff_eq(Vec3::NEG_X, 1e-4),
        "{}",
        ahead.normal
    );

    // short of the wall, up into the sky, and with the level left out
    assert_eq!(
        world.cast_ray(
            Vec3::new(0.0, 0.0, 1.0),
            Vec3::new(4.0, 0.0, 0.0),
            Layers::ALL
        )?,
        None
    );
    assert_eq!(
        world.cast_ray(
            Vec3::new(0.0, 0.0, 1.0),
            Vec3::new(0.0, 0.0, 50.0),
            Layers::ALL
        )?,
        None
    );
    assert_eq!(
        world.cast_ray(
            Vec3::new(0.0, 0.0, 5.0),
            Vec3::new(0.0, 0.0, -10.0),
            Layers::BODIES
        )?,
        None
    );
    // a ray of no length is refused
    assert_eq!(
        world.cast_ray(Vec3::ZERO, Vec3::ZERO, Layers::ALL),
        Err(PhysicsError::Refused)
    );
    Ok(())
}

#[test]
fn shape_casts_stop_short_by_their_size() -> Result<(), PhysicsError> {
    let mut world = world()?;
    let sphere = Shape::Sphere { radius: 0.5 };
    let hit = world.cast_shape(
        sphere,
        Pose::at(Vec3::new(0.0, 0.0, 1.0)),
        Vec3::new(10.0, 0.0, 0.0),
        Layers::ALL,
    )?;
    let hit = hit.ok_or(PhysicsError::Refused)?;
    assert_eq!(hit.struck, Struck::Level);
    assert!((hit.fraction - 0.45).abs() < 2e-3, "{}", hit.fraction);
    assert!(hit.normal.abs_diff_eq(Vec3::NEG_X, 1e-3), "{}", hit.normal);
    assert!((hit.point.x - 5.0).abs() < 1e-2, "{}", hit.point);

    // a standing capsule dropped onto the floor lands on its lower cap
    let capsule = Shape::Capsule {
        radius: 0.2,
        height: 0.3,
    };
    let hit = world.cast_shape(
        capsule,
        Pose::at(Vec3::new(-2.0, 0.0, 2.0)),
        Vec3::new(0.0, 0.0, -4.0),
        Layers::ALL,
    )?;
    let hit = hit.ok_or(PhysicsError::Refused)?;
    // (it falls 2 - 0.35 units of the 4)
    assert!((hit.fraction - 1.65 / 4.0).abs() < 2e-3, "{}", hit.fraction);
    assert!(hit.normal.abs_diff_eq(Vec3::Z, 1e-3), "{}", hit.normal);

    // a body in the way is hit, and only when bodies are asked for
    let crate_ = Shape::Box {
        half_extents: Vec3::splat(0.5),
    };
    let body = world.add_body(
        crate_,
        Pose::at(Vec3::new(2.0, 0.0, 1.0)),
        Motion::Kinematic,
        50.0,
        Material::default(),
    )?;
    let hit = world.cast_shape(
        sphere,
        Pose::at(Vec3::new(0.0, 0.0, 1.0)),
        Vec3::new(10.0, 0.0, 0.0),
        Layers::ALL,
    )?;
    assert_eq!(hit.map(|hit| hit.struck), Some(Struck::Body(body)));
    let hit = world.cast_ray(
        Vec3::new(0.0, 0.0, 1.0),
        Vec3::new(10.0, 0.0, 0.0),
        Layers::LEVEL,
    )?;
    assert_eq!(hit.map(|hit| hit.struck), Some(Struck::Level));
    Ok(())
}

fn tumble() -> Result<Vec<Pose>, PhysicsError> {
    let mut world = world()?;
    let mut bodies = Vec::new();
    for index in 0..8 {
        let index = index as f32;
        let pose = Pose {
            position: Vec3::new(index * 0.7 - 3.0, 0.3 * index, 2.0 + 0.5 * index),
            rotation: Quat::from_euler(glam::EulerRot::XYZ, 0.3 * index, 0.7, 0.1 * index),
        };
        let shape = if (index as u32).is_multiple_of(2) {
            Shape::Box {
                half_extents: Vec3::new(0.3, 0.2, 0.1),
            }
        } else {
            Shape::Capsule {
                radius: 0.15,
                height: 0.4,
            }
        };
        let body = world.add_body(
            shape,
            pose,
            Motion::Dynamic,
            5.0,
            Material {
                friction: 0.6,
                restitution: 0.3,
            },
        )?;
        world.set_body_velocity(body, Vec3::new(4.0, -1.0, 0.0), Vec3::new(0.0, 2.0, 1.0))?;
        bodies.push(body);
    }
    for _ in 0..(3.0 / TICK) as usize {
        world.step(TICK)?;
    }
    bodies.iter().map(|&body| world.body_pose(body)).collect()
}

#[test]
fn two_runs_are_identical() -> Result<(), PhysicsError> {
    let first = tumble()?;
    let second = tumble()?;
    for (a, b) in first.iter().zip(&second) {
        assert_eq!(
            a.position.to_array().map(f32::to_bits),
            b.position.to_array().map(f32::to_bits)
        );
        assert_eq!(
            a.rotation.to_array().map(f32::to_bits),
            b.rotation.to_array().map(f32::to_bits)
        );
    }
    Ok(())
}

#[test]
fn bloodgulchs_ground_is_under_the_trace_start() -> Result<(), Box<dyn Error>> {
    let Some(stock) = stock_level("bloodgulch")? else {
        return Ok(()); // (no game data here)
    };
    let level = stock;
    let bsp = level.structure_bsps.first().ok_or("no structure bsp")?;
    let mesh = LevelMesh::from_collision(&bsp.collision);
    assert!(
        mesh.triangles.len() > 1000,
        "{} triangles",
        mesh.triangles.len()
    );
    let mut world = World::new(WorldSettings::default())?;
    world.set_level(&mesh)?;
    // (engine/tests/traces: the biped stands at about z 0.1 there)
    let hit = world.cast_ray(
        Vec3::new(87.1, -158.45, 5.0),
        Vec3::new(0.0, 0.0, -10.0),
        Layers::LEVEL,
    )?;
    let hit = hit.ok_or("no ground under the trace start")?;
    assert!(
        hit.point.z > -0.5 && hit.point.z < 0.5,
        "the ground at {}",
        hit.point
    );
    assert!(hit.normal.z > 0.7, "the ground's normal {}", hit.normal);
    Ok(())
}

#[test]
fn the_devmaps_plane_is_solid_to_its_edges() -> Result<(), PhysicsError> {
    let plane = opence_content::devmap::plane(80.0, 32);
    let mesh = LevelMesh::from_collision(&plane.collision);
    assert_eq!(mesh.triangles.len(), 2);
    let mut world = World::new(WorldSettings::default())?;
    world.set_level(&mesh)?;
    for (x, y) in [(0.0, 0.0), (79.5, -79.5), (-40.0, 60.0)] {
        let hit = world
            .cast_ray(Vec3::new(x, y, 2.0), Vec3::new(0.0, 0.0, -4.0), Layers::ALL)?
            .ok_or(PhysicsError::Refused)?;
        assert_eq!(hit.struck, Struck::Level);
        assert!(
            hit.point.abs_diff_eq(Vec3::new(x, y, 0.0), 1e-3),
            "{}",
            hit.point
        );
        assert!(hit.normal.abs_diff_eq(Vec3::Z, 1e-4), "{}", hit.normal);
    }
    // (past its edge, nothing)
    assert_eq!(
        world.cast_ray(
            Vec3::new(81.0, 0.0, 2.0),
            Vec3::new(0.0, 0.0, -4.0),
            Layers::ALL
        )?,
        None
    );
    Ok(())
}
