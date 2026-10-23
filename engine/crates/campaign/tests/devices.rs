//! The campaign's devices on its maps: their definitions are imported, and
//! a door that opens by itself opens for the player standing in it and
//! closes after the player has gone (skipped where there is no game
//! data).

use std::error::Error;
use std::path::Path;
use std::sync::Arc;

use opence_campaign::Mission;
use opence_content::Machine;
use opence_import::{import_level, Cache};
use opence_sim::input::TickInput;
use opence_sim::objects::Difficulty;
use opence_sim::{Simulation, Vec3};

const MAPS: [&str; 4] = ["a10", "a30", "b30", "c10"];

/// The mission of `map`, if the map is here.
fn mission(map: &str) -> Result<Option<Mission>, Box<dyn Error>> {
    let path = Path::new(env!("CARGO_MANIFEST_DIR"))
        .join("../../../assets/maps")
        .join(format!("{map}.map"));
    let Ok(file) = std::fs::read(&path) else {
        return Ok(None);
    };
    let level = Arc::new(import_level(&Cache::open(file)?)?);
    let mut sim = Simulation::empty();
    sim.spawn_player(&level, 0)?;
    Ok(Some(Mission::new(sim, level, Difficulty::Normal)))
}

#[test]
fn a_door_opens_for_the_player_and_closes_after() -> Result<(), Box<dyn Error>> {
    let mut opened = Vec::new();
    for map in MAPS {
        let Some(mut mission) = mission(map)? else {
            continue;
        };
        mission.step(TickInput::default());
        let defined = mission
            .devices()
            .filter(|(_, d)| d.definition.is_some())
            .count();
        assert!(defined > 0, "{map}: no device has its definition");
        let doors: Vec<_> = mission
            .devices()
            .filter(|(_, d)| {
                let door = d
                    .definition
                    .as_ref()
                    .and_then(|definition| definition.machine)
                    .is_some_and(|m| m.machine_type == Machine::DOOR);
                door && !d.manual && !d.one_sided && d.power() > 0.0 && d.position() == 0.0
            })
            .map(|(handle, _)| handle)
            .collect();
        for door in doors.into_iter().take(8) {
            let Some(at) = mission.sim().transform(door).map(|t| t.position) else {
                continue;
            };
            mission.sim_mut().teleport_player(0, at + Vec3::Z * 0.1);
            let mut open = false;
            for _ in 0..240 {
                mission.step(TickInput::default());
                open |= mission.device(door).is_some_and(|d| d.position() == 1.0);
            }
            if !open {
                continue;
            }
            mission
                .sim_mut()
                .teleport_player(0, at + Vec3::new(500.0, 500.0, 0.0));
            let mut closed = false;
            for _ in 0..900 {
                mission.step(TickInput::default());
                closed |= mission.device(door).is_some_and(|d| d.position() == 0.0);
            }
            assert!(closed, "{map}: a door stayed open with nobody near");
            opened.push(map);
            break;
        }
    }
    if Path::new(env!("CARGO_MANIFEST_DIR"))
        .join("../../../assets/maps/a10.map")
        .exists()
    {
        assert!(!opened.is_empty(), "no door opened for the player");
    }
    Ok(())
}
