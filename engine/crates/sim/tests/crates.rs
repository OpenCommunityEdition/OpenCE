//! Crates on the test map, made over bloodgulch's definitions: dropped
//! over the physics station's slope, they fall onto it, slide down and
//! come to rest on it or the plane below; two simulations given the same
//! inputs, a player throwing a frag into the pile, agree to the bit; and a
//! particle's path meets the slope. Passes without
//! assets/maps/bloodgulch.map.

use std::error::Error;
use std::path::Path;

use opence_content::{devmap, Level};
use opence_import::{import_level, Cache};
use opence_sim::input::{Buttons, TickInput};
use opence_sim::units::Radians;
use opence_sim::{clock, Quat, Simulation, Vec3};

fn test_map() -> Result<Option<Level>, Box<dyn Error>> {
    let path = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../assets/maps/bloodgulch.map");
    let Ok(file) = std::fs::read(&path) else {
        return Ok(None); // (no game data here)
    };
    let donor = import_level(&Cache::open(file)?)?;
    Ok(Some(devmap::level(Some(&donor))))
}

#[test]
fn crates_fall_onto_the_slope_and_come_to_rest() -> Result<(), Box<dyn Error>> {
    let Some(level) = test_map()? else {
        return Ok(());
    };
    let mut simulation = Simulation::empty();
    // (without a player there is no physics world to drop them into)
    assert!(!simulation.drop_crate(Vec3::ZERO, Quat::IDENTITY, Vec3::ONE));
    simulation.clear_crates();
    simulation.spawn_player(&level, 0)?;
    let half = Vec3::splat(0.5);
    assert!(!simulation.drop_crate(Vec3::ZERO, Quat::IDENTITY, Vec3::ZERO));
    let slope = devmap::SLOPE;
    let dropped: Vec<Vec3> = (0..6)
        .map(|at| {
            let at = at as f32;
            Vec3::new(-48.0 + 1.5 * at, -42.0 + 0.8 * at, 12.0 + 1.2 * at)
        })
        .collect();
    for &position in &dropped {
        assert!(simulation.drop_crate(position, Quat::from_rotation_z(0.3), half));
    }
    assert_eq!(simulation.crates().all().len(), dropped.len());
    assert!(simulation.crates().moving());

    for _ in 0..20 * clock::TICKS_PER_SECOND {
        simulation.step(TickInput::default());
    }
    assert!(!simulation.crates().moving());
    for (crate_, start) in simulation.crates().all().iter().zip(&dropped) {
        let at = crate_.pose.position;
        assert!(crate_.resting(), "{at}");
        assert!(at.is_finite(), "{at}");
        // (down the slope, along +x, and lying on it or the plane)
        assert!(at.x > start.x, "{start} {at}");
        let ground = slope.height_at(at.x, at.y);
        assert!(at.z > ground - 0.1, "{at} over {ground}");
        assert!(at.z < ground + 2.5, "{at} over {ground}");
    }

    simulation.clear_crates();
    assert!(simulation.crates().all().is_empty());
    Ok(())
}

/// A world's hash and its crates' poses, as bits.
type Snapshot = (u64, Vec<[u32; 7]>);

/// The pile on the slope, the player at its foot throwing a frag up into
/// it and walking after it; the world's hash and the crates' poses, as
/// bits, after ten seconds.
fn frag_into_the_pile(level: &Level) -> Result<Snapshot, Box<dyn Error>> {
    let mut simulation = Simulation::empty();
    simulation.spawn_player(level, 0)?;
    let slope = devmap::SLOPE;
    assert!(simulation.teleport_player(0, Vec3::new(slope.foot[0] + 4.0, slope.foot[1], 0.5)));
    for at in 0..8 {
        let at = at as f32;
        let position = Vec3::new(-42.0 - 0.9 * at, -41.0 + 0.6 * at, 9.0 + 0.8 * at);
        assert!(simulation.drop_crate(position, Quat::from_rotation_z(0.5 * at), Vec3::splat(0.5)));
    }
    assert!(simulation.give_grenades(0, 1));
    // (turning up the slope, along -x, and a little up on the first tick;
    // the turns are each tick's, not where it faces)
    let turn = TickInput {
        yaw: Radians(std::f32::consts::PI),
        pitch: Radians(0.3),
        ..TickInput::default()
    };
    for tick in 0..10 * clock::TICKS_PER_SECOND {
        let input = match tick {
            0 => turn,
            30 => TickInput {
                buttons: Buttons::GRENADE,
                ..TickInput::default()
            },
            _ if tick > 120 => TickInput {
                forward: 64,
                ..TickInput::default()
            },
            _ => TickInput::default(),
        };
        simulation.step(input);
    }
    let poses = simulation
        .crates()
        .all()
        .iter()
        .map(|one| {
            let [x, y, z] = one.pose.position.to_array().map(f32::to_bits);
            let [a, b, c, d] = one.pose.rotation.to_array().map(f32::to_bits);
            [x, y, z, a, b, c, d]
        })
        .collect();
    Ok((simulation.world_hash(), poses))
}

#[test]
fn two_simulations_given_the_same_inputs_agree() -> Result<(), Box<dyn Error>> {
    let Some(level) = test_map()? else {
        return Ok(());
    };
    let first = frag_into_the_pile(&level)?;
    let second = frag_into_the_pile(&level)?;
    assert_eq!(first.1.len(), 8);
    assert_eq!(first, second);
    Ok(())
}

#[test]
fn a_particles_path_meets_the_slope() -> Result<(), Box<dyn Error>> {
    // (without a player there is no world to meet)
    let simulation = Simulation::empty();
    assert_eq!(
        simulation.collide_point(Vec3::new(0.0, 0.0, 5.0), Vec3::NEG_Z * 10.0),
        None
    );
    let Some(level) = test_map()? else {
        return Ok(());
    };
    let mut simulation = Simulation::empty();
    simulation.spawn_player(&level, 0)?;
    let slope = devmap::SLOPE;
    let (x, y) = (slope.foot[0] - 0.5 * slope.length, slope.foot[1]);
    let from = Vec3::new(x, y, slope.height + 2.0);
    let hit = simulation
        .collide_point(from, Vec3::new(0.0, 0.0, -20.0))
        .ok_or("the path missed the slope")?;
    let ground = slope.height_at(x, y);
    assert!(
        (hit.point.z - ground).abs() < 1e-2,
        "{} over {ground}",
        hit.point
    );
    // (the slope rises along -x, so it faces up and toward +x)
    assert!(hit.normal.z > 0.5 && hit.normal.x > 0.1, "{}", hit.normal);
    assert!(hit.fraction > 0.0 && hit.fraction < 1.0, "{}", hit.fraction);
    // a path with no length, or not finite, meets nothing
    assert_eq!(simulation.collide_point(from, Vec3::ZERO), None);
    assert_eq!(simulation.collide_point(Vec3::NAN, Vec3::NEG_Z), None);
    // and one up into the sky meets nothing either
    assert_eq!(simulation.collide_point(from, Vec3::Z * 50.0), None);
    Ok(())
}
