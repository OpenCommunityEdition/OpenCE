//! The AI's actors on the campaign's maps: every encounter placed stands
//! its actors on the ground, armed and on a team, and an enemy put before
//! the player sees it, turns to it and shoots it (skipped where there is
//! no game data).

use std::error::Error;
use std::sync::Arc;

use opence_campaign::{AiReference, Mission};
use opence_import::testing::stock_level_unheld;
use opence_sim::damage::Vitality;
use opence_sim::input::TickInput;
use opence_sim::objects::Difficulty;
use opence_sim::{Simulation, Vec3};

/// A body no enemy's fire takes in the ticks a test runs.
const STURDY: f32 = 1.0e6;

const MAPS: [&str; 10] = [
    "a10", "a30", "a50", "b30", "b40", "c10", "c20", "c40", "d20", "d40",
];

/// The mission of `map` at legendary with every encounter placed, if the
/// map is here.
fn mission(map: &str) -> Result<Option<Mission>, Box<dyn Error>> {
    let Some(stock) = stock_level_unheld(map)? else {
        return Ok(None);
    };
    let level = Arc::new(stock);
    let encounters = level.scenario.campaign.encounters.len();
    let mut sim = Simulation::empty();
    sim.spawn_player(&level, 0)?;
    let mut mission = Mission::new(sim, level, Difficulty::Legendary);
    for e in 0..encounters {
        mission.place_ai(AiReference::Encounter(u16::try_from(e)?));
    }
    Ok(Some(mission))
}

#[test]
fn every_encounter_places_its_actors() -> Result<(), Box<dyn Error>> {
    for map in MAPS {
        let Some(mission) = mission(map)? else {
            continue;
        };
        assert!(!mission.actors().is_empty(), "{map}: no actors");
        assert_eq!(
            mission.actors().len(),
            mission.sim().actors().len(),
            "{map}"
        );
        for (actor, player) in mission.actors().iter().zip(mission.sim().actors()) {
            assert_eq!(actor.unit, player.handle(), "{map}");
            assert!(actor.team > 0, "{map}: no team");
            assert!(mission.sim().object(actor.unit).is_some(), "{map}");
        }
    }
    Ok(())
}

#[test]
fn an_enemy_before_the_player_shoots_it() -> Result<(), Box<dyn Error>> {
    let mut missed = Vec::new();
    for map in MAPS {
        let Some(mut mission) = mission(map)? else {
            continue;
        };
        mission.step(TickInput::default());
        let Some(player) = mission.sim().players().first().map(|p| p.handle()) else {
            continue;
        };
        // (the player too strong to die, so that no ragdoll takes its
        // body, and each enemy tried in turn until one hurts it)
        let Some(mut sturdy) = mission.sim().component::<Vitality>(player) else {
            continue;
        };
        sturdy.body = STURDY;
        let enemies: Vec<usize> = (0..mission.actors().len())
            .filter(|&i| ![1, 2].contains(&mission.actors()[i].team))
            .collect();
        let mut shot = false;
        for index in enemies.into_iter().take(24) {
            let Some(actor) = mission.sim().actors().get(index) else {
                continue;
            };
            let (yaw, _) = actor.look();
            let ahead = Vec3::new(libm::cosf(yaw.0), libm::sinf(yaw.0), 0.0);
            let at = actor.mover().position + ahead * 4.0 + Vec3::Z * 0.1;
            let unit = mission.actors()[index].unit;
            mission.sim_mut().insert(player, sturdy.clone());
            mission.sim_mut().teleport_player(0, at);
            let structure = mission.flow().bsp;
            let (mut targeted, mut hurt) = (false, false);
            // (a teleport into a switching volume loads a structure the
            // enemy may not stand on, and the others would fall through
            // it: the structure is put back and that enemy passed over)
            mission.step(TickInput::default());
            if mission.flow().bsp != structure {
                mission.switch_structure(structure);
                continue;
            }
            for _ in 1..150 {
                mission.step(TickInput::default());
                targeted |= mission.actors().get(index).and_then(|a| a.target) == Some(player);
                hurt |= mission
                    .sim()
                    .damage_events()
                    .iter()
                    .any(|event| event.object == player && event.owner == Some(unit));
            }
            if targeted && hurt {
                shot = true;
                break;
            }
        }
        if !shot {
            missed.push(map);
        }
    }
    assert!(missed.is_empty(), "no enemy shot the player on {missed:?}");
    Ok(())
}
