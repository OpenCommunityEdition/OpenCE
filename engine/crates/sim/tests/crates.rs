//! Crates on the test map, made over bloodgulch's definitions: dropped
//! over the physics station's slope, they fall onto it, slide down and
//! come to rest on it or the plane below. Passes without
//! assets/maps/bloodgulch.map.

use std::error::Error;
use std::path::Path;

use opence_content::{devmap, Level};
use opence_import::{import_level, Cache};
use opence_sim::input::TickInput;
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
