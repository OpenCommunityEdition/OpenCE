//! The physics station's ground (the devmap's plane and slope) under a
//! pile of crates: they come to rest without jitter and never fall through
//! it, explosions throw them and they settle again, two runs with a lift
//! and explosions are identical, and (in a release build) 500 bodies
//! tumbling together step in under 2 ms.

use std::time::Instant;

use glam::{Quat, Vec3};
use opence_content::devmap::{self, Wedge};
use opence_physics::{
    BodyId, LevelMesh, Material, Motion, PhysicsError, Pose, Shape, World, WorldSettings,
};

const TICK: f32 = 1.0 / 60.0;
const CRATE_HALF: f32 = 0.4;
const CRATES: usize = 24;
/// The most a crate at rest may move in a second, in world units.
const STILL: f32 = 1e-3;

fn ground() -> Result<World, PhysicsError> {
    let bsp = devmap::ground(devmap::HALF_SIZE, devmap::CELLS);
    let mut world = World::new(WorldSettings::default())?;
    world.set_level(&LevelMesh::from_collision(&bsp.collision))?;
    Ok(world)
}

fn material() -> Material {
    Material {
        friction: 0.7,
        restitution: 0.1,
    }
}

/// A point `along` the slope's rise and `across` it, `over` its surface.
fn on_slope(slope: &Wedge, along: f32, across: f32, over: f32) -> Vec3 {
    let [fx, fy] = slope.rising;
    let x = slope.foot[0] + fx * along - fy * across;
    let y = slope.foot[1] + fy * along + fx * across;
    Vec3::new(x, y, slope.height_at(x, y) + over)
}

/// A loose pile of crates over the slope's upper half, each turned.
fn pile(world: &mut World) -> Result<Vec<BodyId>, PhysicsError> {
    let slope = devmap::SLOPE;
    (0..CRATES)
        .map(|at| {
            let row = (at / 6) as f32;
            let column = (at % 6) as f32;
            let position = on_slope(
                &slope,
                0.5 * slope.length + 1.2 * row,
                1.5 * (column - 2.5),
                1.5 + 1.0 * row + 0.3 * column,
            );
            let rotation =
                Quat::from_rotation_z(0.4 * at as f32) * Quat::from_rotation_x(0.3 * row);
            world.add_body(
                Shape::Box {
                    half_extents: Vec3::splat(CRATE_HALF),
                },
                Pose { position, rotation },
                Motion::Dynamic,
                30.0,
                material(),
            )
        })
        .collect()
}

/// Steps until every body sleeps, or `seconds` pass; the ticks it took.
fn settle(world: &mut World, bodies: &[BodyId], seconds: f32) -> Result<usize, PhysicsError> {
    let ticks = (seconds / TICK) as usize;
    for tick in 0..ticks {
        let mut awake = false;
        for &body in bodies {
            awake |= world.body_awake(body)?;
        }
        if !awake {
            return Ok(tick);
        }
        world.step(TICK)?;
    }
    Ok(ticks)
}

/// Pushes every body away from `centre`, harder the nearer it is, as an
/// explosion there would.
fn explode(world: &mut World, bodies: &[BodyId], centre: Vec3) -> Result<(), PhysicsError> {
    for &body in bodies {
        let pose = world.body_pose(body)?;
        let away = pose.position - centre;
        let distance = away.length().max(0.5);
        if distance < 8.0 {
            let impulse = away / distance * 300.0 * (1.0 - distance / 8.0);
            // (a little above its centre, so that it turns)
            world.push_body(body, impulse, pose.position + Vec3::new(0.0, 0.0, 0.1))?;
        }
    }
    Ok(())
}

/// Each body is above the ground under it (no part of it through), and
/// still on the plane.
fn above_ground(world: &World, bodies: &[BodyId]) -> Result<(), PhysicsError> {
    let slope = devmap::SLOPE;
    // (a crate's centre is never nearer a surface than its half, less
    // what the solver lets bodies sink in)
    for &body in bodies {
        let at = world.body_pose(body)?.position;
        assert!(at.is_finite(), "{at}");
        assert!(
            at.x.abs() < devmap::HALF_SIZE && at.y.abs() < devmap::HALF_SIZE,
            "off the plane at {at}"
        );
        let under = slope.height_at(at.x, at.y);
        assert!(
            at.z > under + CRATE_HALF * 0.5,
            "a crate at {at} sank into the ground at {under}"
        );
    }
    Ok(())
}

fn poses(world: &World, bodies: &[BodyId]) -> Result<Vec<Pose>, PhysicsError> {
    bodies.iter().map(|&body| world.body_pose(body)).collect()
}

#[test]
fn the_pile_comes_to_rest_without_jitter_and_settles_again_after_explosions(
) -> Result<(), PhysicsError> {
    let mut world = ground()?;
    let crates = pile(&mut world)?;
    let ticks = settle(&mut world, &crates, 30.0)?;
    assert!(ticks < (30.0 / TICK) as usize, "still moving after 30 s");
    above_ground(&world, &crates)?;

    // (asleep, a second of steps moves none of them)
    let rested = poses(&world, &crates)?;
    for _ in 0..60 {
        world.step(TICK)?;
    }
    for (before, after) in rested.iter().zip(poses(&world, &crates)?) {
        assert!(
            before.position.distance(after.position) < STILL,
            "{} jittered to {}",
            before.position,
            after.position
        );
    }

    // two explosions among them, each just under one of them where it came
    // to rest (the first scatters them), then rest again
    for under in [0, CRATES / 2] {
        let lying = crates.get(under).copied().ok_or(PhysicsError::Body)?;
        let centre = world.body_pose(lying)?.position - Vec3::new(0.3, 0.0, 0.5);
        explode(&mut world, &crates, centre)?;
        let mut thrown = 0;
        for &body in &crates {
            thrown += usize::from(world.body_awake(body)?);
        }
        assert!(thrown > 0, "the explosion at {centre} threw nothing");
        // (and no fast crate passes through the ground on the way)
        for _ in 0..30 {
            world.step(TICK)?;
            above_ground(&world, &crates)?;
        }
        let ticks = settle(&mut world, &crates, 30.0)?;
        assert!(ticks < (30.0 / TICK) as usize, "still moving after 30 s");
        above_ground(&world, &crates)?;
    }
    Ok(())
}

#[test]
fn a_crate_thrown_at_the_ground_does_not_pass_through_it() -> Result<(), PhysicsError> {
    let mut world = ground()?;
    let body = world.add_body(
        Shape::Box {
            half_extents: Vec3::splat(0.1),
        },
        Pose::at(Vec3::new(10.0, 10.0, 2.0)),
        Motion::Dynamic,
        5.0,
        material(),
    )?;
    // (300 units a second: 5 a tick, 25 times its size)
    world.set_body_velocity(body, Vec3::new(0.0, 0.0, -300.0), Vec3::ZERO)?;
    for _ in 0..30 {
        world.step(TICK)?;
    }
    let at = world.body_pose(body)?.position;
    assert!(at.z > 0.0, "it fell through to {at}");
    Ok(())
}

/// The pile, a lift rising under its foot and two explosions; the crates'
/// and the lift's poses after four seconds.
fn run() -> Result<Vec<Pose>, PhysicsError> {
    let mut world = ground()?;
    let mut bodies = pile(&mut world)?;
    let slope = devmap::SLOPE;
    let mut lift_at = on_slope(&slope, 0.0, 0.0, 0.0) + Vec3::new(2.0, 0.0, 0.1);
    let lift = world.add_body(
        Shape::Box {
            half_extents: Vec3::new(1.5, 3.0, 0.1),
        },
        Pose::at(lift_at),
        Motion::Kinematic,
        1000.0,
        material(),
    )?;
    for tick in 0..240 {
        if tick == 60 || tick == 150 {
            let centre = on_slope(&slope, 0.4 * slope.length, (tick / 60) as f32, 0.0);
            explode(&mut world, &bodies, centre)?;
        }
        if tick >= 90 {
            lift_at.z += 0.5 * TICK;
        }
        world.move_body(lift, Pose::at(lift_at), TICK)?;
        world.step(TICK)?;
    }
    bodies.push(lift);
    poses(&world, &bodies)
}

#[test]
fn two_runs_with_a_lift_and_explosions_are_identical() -> Result<(), PhysicsError> {
    let first = run()?;
    let second = run()?;
    assert_eq!(first.len(), second.len());
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

/// 500 bodies, boxes and capsules, thrown together into a heap on the
/// plane: the median step while all are awake must take under 2 ms. Only
/// a release build is timed.
#[test]
#[cfg_attr(debug_assertions, ignore = "timed in a release build only")]
fn five_hundred_active_bodies_step_in_under_two_milliseconds() -> Result<(), PhysicsError> {
    let mut world = ground()?;
    let mut bodies = Vec::new();
    for at in 0..500 {
        let (x, y, z) = (at % 10, (at / 10) % 10, at / 100);
        let position = Vec3::new(
            20.0 + 1.1 * x as f32,
            20.0 + 1.1 * y as f32,
            0.6 + 1.1 * z as f32,
        );
        let shape = if at % 2 == 0 {
            Shape::Box {
                half_extents: Vec3::new(0.4, 0.3, 0.25),
            }
        } else {
            Shape::Capsule {
                radius: 0.25,
                height: 0.5,
            }
        };
        let body = world.add_body(
            shape,
            Pose {
                position,
                rotation: Quat::from_rotation_z(0.1 * at as f32),
            },
            Motion::Dynamic,
            20.0,
            material(),
        )?;
        // (thrown toward the heap's middle, so that they keep meeting)
        let inward = (Vec3::new(25.0, 25.0, 3.0) - position) * 0.8;
        world.set_body_velocity(body, inward, Vec3::new(0.5, -0.3, 0.2))?;
        bodies.push(body);
    }
    let mut times = Vec::new();
    for _ in 0..90 {
        let start = Instant::now();
        world.step(TICK)?;
        times.push(start.elapsed().as_secs_f64() * 1000.0);
    }
    let mut awake = 0;
    for &body in &bodies {
        awake += usize::from(world.body_awake(body)?);
    }
    assert!(awake > 450, "only {awake} bodies stayed awake");
    times.sort_by(f64::total_cmp);
    let median = times.get(times.len() / 2).copied().unwrap_or(f64::MAX);
    assert!(median < 2.0, "the median step took {median:.3} ms");
    Ok(())
}
