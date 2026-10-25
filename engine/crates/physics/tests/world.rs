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
    CharacterSettings, Ground, Layers, LevelMesh, Material, Motion, PhysicsError, Pose, Shape,
    Struck, World, WorldSettings,
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

/// The six faces of the box from `low` to `high`, each facing in, and a
/// slanted triangle: some rings wound the other way and some planes named
/// through their reverse (a negative index), so that each of the ring's
/// normal's components decides a face's winding. Each face's triangles
/// follow it, with its normal.
fn closed_box(low: [f32; 3], high: [f32; 3]) -> (CollisionBsp, Vec<Vec3>) {
    let mut bsp = CollisionBsp::default();
    let mut normals = Vec::new();
    for axis in 0..3 {
        for (side, at) in [low[axis], high[axis]].into_iter().enumerate() {
            let (u, v) = ((axis + 1) % 3, (axis + 2) % 3);
            let corner = |a: f32, b: f32| {
                let mut point = [0.0; 3];
                point[axis] = at;
                point[u] = a;
                point[v] = b;
                point
            };
            let mut points = vec![
                corner(low[u], low[v]),
                corner(high[u], low[v]),
                corner(high[u], high[v]),
                corner(low[u], high[v]),
            ];
            let face = bsp.surfaces.len();
            if face % 2 == 1 {
                points.reverse();
            }
            let mut inward = Vec3::ZERO;
            inward[axis] = if side == 0 { 1.0 } else { -1.0 };
            add_face(&mut bsp, &points, inward, at * inward[axis], face % 3 == 0);
            normals.extend([inward; 2]);
        }
    }
    let slant = [[1.5, 2.5, 3.5], [3.5, 3.0, 4.0], [2.0, 5.0, 8.0]];
    let [a, b, c] = slant.map(Vec3::from_array);
    let facing = (b - a).cross(c - a).normalize();
    add_face(&mut bsp, &slant, facing, facing.dot(a), true);
    normals.push(facing);
    (bsp, normals)
}

/// Adds a surface facing `normal`, its plane stored as is or (`reversed`)
/// as the opposite plane named through a negative index.
fn add_face(
    bsp: &mut CollisionBsp,
    points: &[[f32; 3]],
    normal: Vec3,
    distance: f32,
    reversed: bool,
) {
    let sign = if reversed { -1.0 } else { 1.0 };
    let plane = (normal * sign).extend(distance * sign).to_array();
    add_surface(bsp, points, plane, 0);
    if let Some(surface) = bsp.surfaces.last_mut() {
        if reversed {
            surface.plane |= i32::MIN;
        }
    }
}

#[test]
fn every_surface_faces_as_its_plane_whichever_way_it_is_wound() {
    // (one box beside the origin and one across it, below it: Newell's sums
    // meet coordinates of either sign)
    for (low, high) in [
        ([1.0, 2.0, 3.0], [4.0, 6.0, 9.0]),
        ([-3.0, -2.0, -5.0], [2.0, 4.0, -1.0]),
    ] {
        let (bsp, normals) = closed_box(low, high);
        let mesh = LevelMesh::from_collision(&bsp);
        assert_eq!(mesh.triangles.len(), normals.len());
        for (triangle, expected) in mesh.triangles.iter().zip(&normals) {
            let [a, b, c] = triangle.map(|index| Vec3::from_array(mesh.vertices[index as usize]));
            let normal = (b - a).cross(c - a).normalize();
            assert!(
                normal.abs_diff_eq(*expected, 1e-5),
                "{normal} against {expected}"
            );
        }
    }
    let (bsp, _) = closed_box([1.0, 2.0, 3.0], [4.0, 6.0, 9.0]);
    let mesh = LevelMesh::from_collision(&bsp);
    assert!(!mesh.is_empty());
    // appended again, the second copy's triangles name its own vertices
    let mut twice = mesh.clone();
    twice.append(&bsp);
    assert_eq!(twice.triangles.len(), 2 * mesh.triangles.len());
    let offset = mesh.vertices.len() as u32;
    for (first, second) in mesh
        .triangles
        .iter()
        .zip(&twice.triangles[mesh.triangles.len()..])
    {
        assert_eq!(first.map(|index| index + offset), *second);
    }
}

#[test]
fn a_ring_broken_past_its_third_vertex_is_left_out() {
    let (mut bsp, _) = closed_box([1.0, 2.0, 3.0], [4.0, 6.0, 9.0]);
    // (the first face's third edge leads nowhere: three vertices read, then
    // the ring breaks)
    bsp.edges[2].edges[0] = 999;
    assert_eq!(LevelMesh::from_collision(&bsp).triangles.len(), 11);
    assert!(LevelMesh::from_collision(&CollisionBsp::default()).is_empty());
}

#[test]
fn bodies_are_counted_given_velocities_and_removed() -> Result<(), PhysicsError> {
    let mut world = world()?;
    assert_eq!(world.bodies(), (0, 0));
    let shape = Shape::Box {
        half_extents: Vec3::splat(0.25),
    };
    let add = |world: &mut World, x: f32| {
        world.add_body(
            shape,
            Pose::at(Vec3::new(x, 0.0, 3.0)),
            Motion::Dynamic,
            10.0,
            Material::default(),
        )
    };
    let first = add(&mut world, -2.0)?;
    let second = add(&mut world, 2.0)?;
    assert_eq!(world.bodies(), (2, 2));
    world.set_body_velocity(first, Vec3::new(1.0, -2.0, 3.0), Vec3::new(0.0, 0.0, 0.5))?;
    let (linear, angular) = world.body_velocity(first)?;
    assert!(
        linear.abs_diff_eq(Vec3::new(1.0, -2.0, 3.0), 1e-4),
        "{linear}"
    );
    assert!(
        angular.abs_diff_eq(Vec3::new(0.0, 0.0, 0.5), 1e-4),
        "{angular}"
    );
    world.remove_body(second);
    assert_eq!(world.bodies(), (1, 1));
    assert!(world.body_pose(second).is_err());
    assert!(world
        .set_body_velocity(second, Vec3::X, Vec3::ZERO)
        .is_err());
    Ok(())
}

#[test]
fn a_sphere_sinking_into_the_floor_meets_it_and_one_above_does_not() -> Result<(), PhysicsError> {
    let mut world = world()?;
    let contact = world
        .sphere_contact(Vec3::new(1.0, 2.0, 0.75), 1.0, Layers::LEVEL)?
        .ok_or(PhysicsError::Refused)?;
    assert!((contact.depth - 0.25).abs() < 1e-3, "{}", contact.depth);
    assert!(
        contact.normal.abs_diff_eq(Vec3::Z, 1e-4),
        "{}",
        contact.normal
    );
    assert!(
        contact.point.abs_diff_eq(Vec3::new(1.0, 2.0, 0.0), 1e-3),
        "{}",
        contact.point
    );
    assert_eq!(contact.struck, Struck::Level);
    // the wall faces back along x: the depth is along its normal
    let wall = world
        .sphere_contact(Vec3::new(4.25, 0.0, 3.0), 1.0, Layers::LEVEL)?
        .ok_or(PhysicsError::Refused)?;
    assert!((wall.depth - 0.25).abs() < 1e-3, "{}", wall.depth);
    assert!(
        wall.normal.abs_diff_eq(Vec3::NEG_X, 1e-4),
        "{}",
        wall.normal
    );
    // (just clear of it: the cast reaches the floor, but the sphere is not in it)
    assert_eq!(
        world.sphere_contact(Vec3::new(1.0, 2.0, 1.0005), 1.0, Layers::LEVEL)?,
        None
    );
    assert_eq!(
        world.sphere_contact(Vec3::new(1.0, 2.0, 3.0), 1.0, Layers::LEVEL)?,
        None
    );
    // with the level taken away there is nothing to meet or hit
    world.clear_level();
    assert_eq!(
        world.sphere_contact(Vec3::new(1.0, 2.0, 0.75), 1.0, Layers::LEVEL)?,
        None
    );
    let down = world.cast_ray(
        Vec3::new(1.0, 2.0, 5.0),
        Vec3::new(0.0, 0.0, -10.0),
        Layers::ALL,
    )?;
    assert_eq!(down, None);
    Ok(())
}

/// The room, a ceiling a unit up over its far side (x < -5) and a ramp
/// too steep to stand on (60 degrees) rising along y from y = 6.
fn rooms_with_a_ceiling_and_a_ramp() -> CollisionBsp {
    let mut bsp = room();
    add_surface(
        &mut bsp,
        &[
            [-10.0, -10.0, 1.0],
            [-10.0, 10.0, 1.0],
            [-5.0, 10.0, 1.0],
            [-5.0, -10.0, 1.0],
        ],
        [0.0, 0.0, -1.0, -1.0],
        0,
    );
    let rise = 2.0 * libm::sqrtf(3.0);
    let (sin, cos) = (libm::sqrtf(3.0) / 2.0, 0.5);
    add_surface(
        &mut bsp,
        &[
            [-2.0, 6.0, 0.0],
            [2.0, 6.0, 0.0],
            [2.0, 8.0, rise],
            [-2.0, 8.0, rise],
        ],
        [0.0, -sin, cos, -sin * 6.0],
        0,
    );
    bsp
}

fn pill() -> CharacterSettings {
    CharacterSettings {
        radius: 0.2,
        height: 0.7,
        maximum_slope: 0.8,
        step_up: 0.1,
        stick_down: 0.1,
        layers: Layers::GROUND,
    }
}

#[test]
fn a_character_walks_to_a_wall_and_stands_off_it_by_its_radius() -> Result<(), PhysicsError> {
    let mut world = World::new(WorldSettings::default())?;
    world.set_level(&LevelMesh::from_collision(
        &rooms_with_a_ceiling_and_a_ramp(),
    ))?;
    let character = world.add_character(&pill(), Vec3::new(2.0, -3.0, 0.0))?;
    let mut state = world.character_state(character)?;
    assert!(
        state.position.abs_diff_eq(Vec3::new(2.0, -3.0, 0.0), 1e-3),
        "{}",
        state.position
    );
    state = world.move_character(character, Vec3::new(4.0, 0.0, -1.0), TICK)?;
    assert!((state.velocity.x - 4.0).abs() < 0.05, "{}", state.velocity);
    for _ in 0..120 {
        state = world.move_character(character, Vec3::new(4.0, 0.0, -1.0), TICK)?;
    }
    assert!((state.position.x - 4.8).abs() < 0.02, "{}", state.position);
    assert!((state.position.y + 3.0).abs() < 0.02, "{}", state.position);
    assert_eq!(state.ground, Ground::Standing);
    assert_eq!(world.character_state(character)?, state);
    world.remove_character(character);
    assert_eq!(world.character_state(character), Err(PhysicsError::Refused));
    Ok(())
}

/// A floor at z = 0 for x < 0 and one `rise` higher for x > 0, the step
/// between them facing back along x.
fn ledge(rise: f32) -> LevelMesh {
    let quads = [
        [
            [-10.0, -10.0, 0.0],
            [0.0, -10.0, 0.0],
            [0.0, 10.0, 0.0],
            [-10.0, 10.0, 0.0],
        ],
        [
            [0.0, 10.0, 0.0],
            [0.0, -10.0, 0.0],
            [0.0, -10.0, rise],
            [0.0, 10.0, rise],
        ],
        [
            [0.0, -10.0, rise],
            [10.0, -10.0, rise],
            [10.0, 10.0, rise],
            [0.0, 10.0, rise],
        ],
    ];
    let mut mesh = LevelMesh::default();
    for quad in quads {
        let first = mesh.vertices.len() as u32;
        mesh.vertices.extend(quad);
        mesh.triangles.push([first, first + 1, first + 2]);
        mesh.triangles.push([first, first + 2, first + 3]);
        mesh.materials.extend([None, None]);
    }
    mesh
}

/// Where a character walking at `velocity` for a second from `start`
/// over a ledge `rise` high ends, and whether it ever left the ground.
fn over_a_ledge(rise: f32, start: Vec3, velocity: Vec3) -> Result<(Vec3, bool), PhysicsError> {
    let mut world = World::new(WorldSettings::default())?;
    world.set_level(&ledge(rise))?;
    let character = world.add_character(&pill(), start)?;
    world.move_character(character, Vec3::new(0.0, 0.0, -1.0), TICK)?;
    let mut left = false;
    let mut state = world.character_state(character)?;
    for _ in 0..60 {
        state = world.move_character(character, velocity, TICK)?;
        left |= state.ground == Ground::Airborne;
    }
    Ok((state.position, left))
}

#[test]
fn a_character_steps_up_and_follows_down_only_as_far_as_it_reaches() -> Result<(), PhysicsError> {
    // (its step up and its stick down are a tenth of a unit)
    let forward = Vec3::new(3.0, 0.0, 0.0);
    let (low, _) = over_a_ledge(0.05, Vec3::new(-1.0, 0.0, 0.0), forward)?;
    assert!(low.x > 1.0 && (low.z - 0.05).abs() < 0.01, "{low}");
    let (high, _) = over_a_ledge(0.5, Vec3::new(-1.0, 0.0, 0.0), forward)?;
    assert!(high.x < 0.0, "{high}");
    let back = Vec3::new(-3.0, 0.0, 0.0);
    let (down, left) = over_a_ledge(0.05, Vec3::new(1.0, 0.0, 0.05), back)?;
    assert!(down.x < -1.0 && !left, "{down}");
    let (_, left) = over_a_ledge(0.5, Vec3::new(1.0, 0.0, 0.5), back)?;
    assert!(left);
    Ok(())
}

#[test]
fn a_character_grows_only_where_there_is_room() -> Result<(), PhysicsError> {
    let mut world = World::new(WorldSettings::default())?;
    world.set_level(&LevelMesh::from_collision(
        &rooms_with_a_ceiling_and_a_ramp(),
    ))?;
    let character = world.add_character(&pill(), Vec3::new(-7.0, 0.0, 0.0))?;
    for _ in 0..10 {
        world.move_character(character, Vec3::new(0.0, 0.0, -1.0), TICK)?;
    }
    assert_eq!(world.set_character_height(character, 0.9), Ok(true));
    assert_eq!(world.set_character_height(character, 1.5), Ok(false));
    // (in the open it grows as tall as asked)
    let open = world.add_character(&pill(), Vec3::new(0.0, -3.0, 0.0))?;
    assert_eq!(world.set_character_height(open, 1.5), Ok(true));
    world.remove_character(open);
    assert_eq!(
        world.set_character_height(open, 0.7),
        Err(PhysicsError::Refused)
    );
    Ok(())
}

#[test]
fn a_character_on_a_ramp_too_steep_for_it_slides() -> Result<(), PhysicsError> {
    let mut world = World::new(WorldSettings::default())?;
    world.set_level(&LevelMesh::from_collision(
        &rooms_with_a_ceiling_and_a_ramp(),
    ))?;
    let character = world.add_character(&pill(), Vec3::new(0.0, 7.0, 2.0))?;
    let mut state = world.character_state(character)?;
    for _ in 0..20 {
        state = world.move_character(character, Vec3::new(0.0, 0.0, -2.0), TICK)?;
        if state.ground != Ground::Airborne {
            break;
        }
    }
    assert_eq!(state.ground, Ground::Steep, "{state:?}");
    Ok(())
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
