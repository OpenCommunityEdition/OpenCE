//! Ragdolls over a floor: two parts joined at a joint fall and come to
//! rest on it, the joint holds within its limits, a limp ragdoll still
//! rests, and a broken joint lets the parts part. Bad parts are refused.

use std::error::Error;

use glam::Vec3;
use opence_content::level::{CollisionEdge, CollisionSurface, CollisionVertex};
use opence_content::CollisionBsp;
use opence_physics::{
    shape_volume, LevelMesh, Material, PhysicsError, Pose, RagdollId, RagdollPart, Shape, World,
    WorldSettings, METRES_PER_UNIT, METRES_PER_UNIT_CUBED,
};

const TICK: f32 = 1.0 / 30.0;

/// A floor 20 units square at z = 0.
fn floor() -> CollisionBsp {
    let mut bsp = CollisionBsp::default();
    let points = [
        [-10.0, -10.0, 0.0],
        [10.0, -10.0, 0.0],
        [10.0, 10.0, 0.0],
        [-10.0, 10.0, 0.0],
    ];
    bsp.planes.push([0.0, 0.0, 1.0, 0.0]);
    for (index, &point) in points.iter().enumerate() {
        let index = index as u32;
        bsp.vertices.push(CollisionVertex {
            point,
            first_edge: index,
        });
        bsp.edges.push(CollisionEdge {
            vertices: [index, (index + 1) % 4],
            edges: [(index + 1) % 4, 0],
            surfaces: [Some(0), None],
        });
    }
    bsp.surfaces.push(CollisionSurface {
        plane: 0,
        first_edge: 0,
        flags: 0,
        breakable_surface: 0,
        material: None,
    });
    bsp
}

fn part(parent: Option<usize>) -> RagdollPart {
    RagdollPart {
        parent,
        shape: Shape::Box {
            half_extents: Vec3::new(0.15, 0.05, 0.05),
        },
        // (the box reaches along the part's x from its joint)
        offset: Pose::at(Vec3::new(0.15, 0.0, 0.0)),
        mass: 10.0,
        twist_axis: Vec3::X,
        plane_axis: Vec3::Z,
        twist: [-0.2, 0.2],
        swing_normal: 0.5,
        swing_plane: 0.5,
    }
}

/// Two parts end to end along x, `height` above the floor.
fn two_parts(world: &mut World, height: f32) -> Result<(RagdollId, Vec<Pose>), PhysicsError> {
    let parts = [part(None), part(Some(0))];
    let poses = vec![
        Pose::at(Vec3::new(0.0, 0.0, height)),
        Pose::at(Vec3::new(0.3, 0.0, height)),
    ];
    let ragdoll = world.add_ragdoll(&parts, &poses, &poses, Material::default())?;
    Ok((ragdoll, poses))
}

fn world() -> Result<World, PhysicsError> {
    let mut world = World::new(WorldSettings::default())?;
    world.set_level(&LevelMesh::from_collision(&floor()))?;
    Ok(world)
}

#[test]
fn a_ragdoll_falls_and_rests_within_its_limits() -> Result<(), Box<dyn Error>> {
    let mut world = world()?;
    let (ragdoll, poses) = two_parts(&mut world, 2.0)?;
    world.drive_ragdoll(ragdoll, &poses, 4.0)?;
    for _ in 0..30 {
        world.step(TICK)?;
    }
    world.ragdoll_go_limp(ragdoll)?;
    for _ in 0..300 {
        world.step(TICK)?;
    }
    let now = world.ragdoll_poses(ragdoll, 2)?;
    for pose in &now {
        assert!(
            pose.position.z > 0.0 && pose.position.z < 0.2,
            "rests at {}",
            pose.position
        );
    }
    // (still joined: the second part's origin is the first's box's end)
    let joint = now[0].position + now[0].rotation * Vec3::new(0.3, 0.0, 0.0);
    assert!(joint.distance(now[1].position) < 0.03);
    assert!(world.ragdoll_limit_error(ragdoll)? < 0.1);
    assert!(!world.ragdoll_awake(ragdoll)?);
    assert!(!world.ragdoll_joint_broken(ragdoll, 1)?);
    Ok(())
}

#[test]
fn a_broken_joint_lets_the_parts_part() -> Result<(), Box<dyn Error>> {
    let mut world = world()?;
    let (ragdoll, _) = two_parts(&mut world, 0.1)?;
    world.break_ragdoll_joint(ragdoll, 1)?;
    assert!(world.ragdoll_joint_broken(ragdoll, 1)?);
    // (once only, and never the first part)
    assert!(world.break_ragdoll_joint(ragdoll, 1).is_err());
    assert!(world.break_ragdoll_joint(ragdoll, 0).is_err());
    world.push_ragdoll(
        ragdoll,
        1,
        Vec3::new(30.0, 0.0, 0.0),
        Vec3::new(0.45, 0.0, 0.1),
    )?;
    for _ in 0..60 {
        world.step(TICK)?;
    }
    let now = world.ragdoll_poses(ragdoll, 2)?;
    let joint = now[0].position + now[0].rotation * Vec3::new(0.3, 0.0, 0.0);
    assert!(
        joint.distance(now[1].position) > 0.2,
        "{}",
        joint.distance(now[1].position)
    );
    world.remove_ragdoll(ragdoll);
    assert!(world.ragdoll_poses(ragdoll, 2).is_err());
    Ok(())
}

#[test]
fn bad_parts_are_refused() -> Result<(), PhysicsError> {
    let mut world = world()?;
    let poses = [Pose::at(Vec3::Z), Pose::at(Vec3::Z)];
    let refused = |world: &mut World, parts: &[RagdollPart]| {
        world
            .add_ragdoll(
                parts,
                &poses[..parts.len()],
                &poses[..parts.len()],
                Material::default(),
            )
            .is_err()
    };
    // (a first part with a parent, a parent not earlier, a twist out of range)
    assert!(refused(&mut world, &[part(Some(0))]));
    assert!(refused(&mut world, &[part(None), part(Some(1))]));
    let mut twisted = part(Some(0));
    twisted.twist = [0.5, 0.2];
    assert!(refused(&mut world, &[part(None), twisted]));
    let mut parallel = part(Some(0));
    parallel.plane_axis = Vec3::X;
    assert!(refused(&mut world, &[part(None), parallel]));
    // (and poses that do not match the parts)
    assert!(world
        .add_ragdoll(&[part(None)], &poses, &poses[..1], Material::default())
        .is_err());
    Ok(())
}

#[test]
fn a_ragdoll_wakes_and_once_removed_is_refused() -> Result<(), Box<dyn Error>> {
    let mut world = world()?;
    let (ragdoll, poses) = two_parts(&mut world, 3.0)?;
    assert!(world.ragdoll_awake(ragdoll)?);
    assert!(world.ragdoll_limit_error(ragdoll)? >= 0.0);
    world.set_ragdoll_velocity(ragdoll, Vec3::new(0.0, 0.0, 5.0))?;
    world.remove_ragdoll(ragdoll);
    assert_eq!(world.ragdoll_awake(ragdoll), Err(PhysicsError::Refused));
    assert_eq!(
        world.ragdoll_limit_error(ragdoll),
        Err(PhysicsError::Refused)
    );
    assert_eq!(
        world.drive_ragdoll(ragdoll, &poses, 4.0),
        Err(PhysicsError::Refused)
    );
    assert_eq!(world.ragdoll_go_limp(ragdoll), Err(PhysicsError::Refused));
    assert_eq!(
        world.set_ragdoll_velocity(ragdoll, Vec3::ZERO),
        Err(PhysicsError::Refused)
    );
    Ok(())
}

#[test]
fn each_shapes_volume_is_its_solids() {
    let near = |a: f32, b: f32| (a - b).abs() < 1e-4 * b;
    let pi = std::f32::consts::PI;
    assert!(near(
        shape_volume(&Shape::Sphere { radius: 2.0 }),
        32.0 / 3.0 * pi
    ));
    // (a cylinder 3 high and the two halves of a sphere of radius 2)
    assert!(near(
        shape_volume(&Shape::Capsule {
            radius: 2.0,
            height: 3.0
        }),
        pi * 4.0 * 3.0 + 32.0 / 3.0 * pi
    ));
    assert!(near(
        shape_volume(&Shape::Box {
            half_extents: Vec3::new(2.0, 3.0, 4.0)
        }),
        192.0
    ));
    assert!(near(
        METRES_PER_UNIT_CUBED,
        METRES_PER_UNIT * METRES_PER_UNIT * METRES_PER_UNIT
    ));
    assert!(near(METRES_PER_UNIT_CUBED, 28.316_846));
}
