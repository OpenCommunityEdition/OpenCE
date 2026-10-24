//! Platforms, kinematic bodies the simulation moves: a rising and sliding
//! lift carries a box and a character standing on it, a door sliding
//! across the floor pushes a box ahead of it, and casts find platforms only
//! when they ask for them.

use glam::{Quat, Vec3};
use opence_content::level::{CollisionEdge, CollisionSurface, CollisionVertex};
use opence_content::CollisionBsp;
use opence_physics::{
    BodyId, CharacterSettings, Ground, Layers, LevelMesh, Material, Motion, PhysicsError, Pose,
    Shape, Struck, World, WorldSettings,
};

const TICK: f32 = 1.0 / 60.0;
/// The lift's speed, in units a second, as it rises and slides.
const LIFT: Vec3 = Vec3::new(0.5, 0.0, 1.0);
/// Half the lift's size.
const LIFT_HALF: Vec3 = Vec3::new(1.5, 1.5, 0.1);

/// A floor 40 units square at z = 0.
fn floor() -> Result<World, PhysicsError> {
    let mut bsp = CollisionBsp::default();
    let points = [
        [-20.0, -20.0, 0.0],
        [20.0, -20.0, 0.0],
        [20.0, 20.0, 0.0],
        [-20.0, 20.0, 0.0],
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
    let mut world = World::new(WorldSettings::default())?;
    world.set_level(&LevelMesh::from_collision(&bsp))?;
    Ok(world)
}

fn platform(world: &mut World, half_extents: Vec3, at: Vec3) -> Result<BodyId, PhysicsError> {
    world.add_body(
        Shape::Box { half_extents },
        Pose::at(at),
        Motion::Kinematic,
        1000.0,
        Material {
            friction: 0.8,
            restitution: 0.0,
        },
    )
}

fn crate_(world: &mut World, at: Vec3) -> Result<BodyId, PhysicsError> {
    world.add_body(
        Shape::Box {
            half_extents: Vec3::splat(0.2),
        },
        Pose::at(at),
        Motion::Dynamic,
        20.0,
        Material {
            friction: 0.8,
            restitution: 0.0,
        },
    )
}

#[test]
fn a_lift_carries_a_box_and_a_character_standing_on_it() -> Result<(), PhysicsError> {
    let mut world = floor()?;
    let start = Vec3::new(0.0, 0.0, 0.5);
    let lift = platform(&mut world, LIFT_HALF, start)?;
    let top = |centre: Vec3| centre.z + LIFT_HALF.z;
    let box_ = crate_(&mut world, Vec3::new(0.7, 0.0, top(start) + 0.21))?;
    let character = world.add_character(
        &CharacterSettings {
            radius: 0.2,
            height: 0.7,
            maximum_slope: 0.8,
            step_up: 0.1,
            stick_down: 0.1,
            layers: Layers::GROUND,
        },
        Vec3::new(-0.7, 0.0, top(start) + 0.01),
    )?;
    // (a second for them to settle on it, then two of it moving)
    let mut centre = start;
    let mut state = world.character_state(character)?;
    for tick in 0..180 {
        if tick >= 60 {
            centre += LIFT * TICK;
        }
        world.move_body(lift, Pose::at(centre), TICK)?;
        // (the character stands still, falling only off its ground)
        let fall = if state.ground == Ground::Standing {
            Vec3::ZERO
        } else {
            Vec3::new(0.0, 0.0, -3.0)
        };
        state = world.move_character(character, fall, TICK)?;
        world.step(TICK)?;
    }
    let moved = LIFT * 2.0;
    let lift_pose = world.body_pose(lift)?;
    assert!(
        lift_pose.position.abs_diff_eq(start + moved, 1e-3),
        "{}",
        lift_pose.position
    );

    let box_pose = world.body_pose(box_)?;
    let expected = Vec3::new(0.7, 0.0, top(start) + 0.2) + moved;
    assert!(
        box_pose.position.distance(expected) < 0.1,
        "the box is at {} on a lift that carried it to {expected}",
        box_pose.position
    );

    let expected = Vec3::new(-0.7, 0.0, top(start)) + moved;
    assert_eq!(state.ground, Ground::Standing);
    assert!(
        state.position.distance(expected) < 0.1,
        "the character is at {} on a lift that carried it to {expected}",
        state.position
    );
    assert!(
        state.ground_velocity.abs_diff_eq(LIFT, 0.05),
        "it was carried at {}",
        state.ground_velocity
    );
    Ok(())
}

#[test]
fn a_door_pushes_a_box_ahead_of_it() -> Result<(), PhysicsError> {
    let mut world = floor()?;
    let half = Vec3::new(0.1, 1.0, 1.0);
    let mut centre = Vec3::new(-1.0, 0.0, 1.02);
    let door = platform(&mut world, half, centre)?;
    let box_ = crate_(&mut world, Vec3::new(0.0, 0.0, 0.2))?;
    for tick in 0..120 {
        if tick >= 30 {
            centre.x += 2.0 * TICK;
        }
        world.move_body(door, Pose::at(centre), TICK)?;
        world.step(TICK)?;
    }
    let pose = world.body_pose(box_)?;
    // (the door's face is at its centre plus its half, the box's back a
    // fifth of a unit behind its centre)
    assert!(
        pose.position.x >= centre.x + half.x + 0.15,
        "the box at {} was not pushed ahead of the door at {}",
        pose.position,
        centre.x
    );
    assert!(
        pose.position.z > 0.1 && pose.position.z < 0.5,
        "{}",
        pose.position
    );
    Ok(())
}

#[test]
fn casts_find_platforms_only_when_they_ask() -> Result<(), PhysicsError> {
    let mut world = floor()?;
    let lift = platform(&mut world, LIFT_HALF, Vec3::new(0.0, 0.0, 1.0))?;
    let from = Vec3::new(0.0, 0.0, 3.0);
    let down = Vec3::new(0.0, 0.0, -5.0);
    let struck = |world: &World, layers| -> Result<Option<Struck>, PhysicsError> {
        Ok(world.cast_ray(from, down, layers)?.map(|hit| hit.struck))
    };
    assert_eq!(struck(&world, Layers::PLATFORMS)?, Some(Struck::Body(lift)));
    assert_eq!(struck(&world, Layers::GROUND)?, Some(Struck::Body(lift)));
    assert_eq!(struck(&world, Layers::ALL)?, Some(Struck::Body(lift)));
    assert_eq!(struck(&world, Layers::LEVEL)?, Some(Struck::Level));
    assert_eq!(struck(&world, Layers::BODIES)?, None);
    // (and a platform turned on its side is still one)
    world.move_body(
        lift,
        Pose {
            position: Vec3::new(0.0, 0.0, 1.0),
            rotation: Quat::from_rotation_x(std::f32::consts::FRAC_PI_2),
        },
        TICK,
    )?;
    world.step(TICK)?;
    assert_eq!(struck(&world, Layers::PLATFORMS)?, Some(Struck::Body(lift)));
    Ok(())
}
