//! The campaign's flow on its maps: a mission saved and loaded again is the
//! same world, a structure the level has is switched to, the opening
//! cinematic holds the camera, and the difficulties scale damage as the
//! map's globals say (skipped where there is no game data).

use std::error::Error;
use std::sync::Arc;

use opence_campaign::{Checkpoint, MissionScripts};
use opence_content::Level;
use opence_import::testing::stock_level;
use opence_script::TICKS_PER_SECOND;
use opence_sim::damage::DifficultyScale;
use opence_sim::input::{Buttons, TickInput};
use opence_sim::objects::Difficulty;
use opence_sim::units::Radians;
use opence_sim::Simulation;

/// The level of `map`, if the map is here.
fn level(map: &str) -> Result<Option<Arc<Level>>, Box<dyn Error>> {
    let Some(stock) = stock_level(map)? else {
        return Ok(None);
    };
    Ok(Some(stock))
}

/// A tick's input that walks, turns and now and then fires.
fn input(tick: u32) -> TickInput {
    TickInput {
        buttons: if tick.is_multiple_of(45) {
            Buttons::FIRE
        } else {
            Buttons::default()
        },
        forward: 100,
        right: if (tick / 60).is_multiple_of(2) {
            30
        } else {
            -30
        },
        up: 0,
        yaw: Radians(0.002 * tick as f32),
        pitch: Radians(0.0),
    }
}

#[test]
fn a_mission_saved_and_loaded_is_the_same_world() -> Result<(), Box<dyn Error>> {
    for map in ["a10", "a30"] {
        let Some(level) = level(map)? else {
            continue;
        };
        let mut played = MissionScripts::start(Arc::clone(&level), Difficulty::Hard)?;
        for tick in 0..10 * TICKS_PER_SECOND {
            played.tick(input(tick));
        }
        let saved = Checkpoint::from_bytes(&played.mission().played().to_bytes())?;
        let mut loaded = MissionScripts::load(level, &saved)?;
        assert_eq!(
            loaded.mission_mut().sim_mut().world_hash(),
            played.mission_mut().sim_mut().world_hash(),
            "{map}"
        );
        assert_eq!(loaded.runtime().time(), played.runtime().time(), "{map}");
    }
    Ok(())
}

#[test]
fn a_save_of_another_scenario_is_refused() -> Result<(), Box<dyn Error>> {
    let Some(level) = level("a10")? else {
        return Ok(());
    };
    let other = Checkpoint {
        scenario: "levels\\b30\\b30".into(),
        difficulty: Difficulty::Easy,
        inputs: Vec::new(),
    };
    assert!(MissionScripts::load(level, &other).is_err());
    Ok(())
}

#[test]
fn each_structure_of_a_level_is_switched_to() -> Result<(), Box<dyn Error>> {
    for map in ["a10", "c40"] {
        let Some(level) = level(map)? else {
            continue;
        };
        let mut sim = Simulation::empty();
        sim.spawn_player(&level, 0)?;
        for bsp in 0..level.structure_bsps.len() {
            sim.switch_bsp(&level, bsp)?;
        }
        assert!(sim.switch_bsp(&level, level.structure_bsps.len()).is_err());
    }
    Ok(())
}

#[test]
fn the_opening_cinematic_holds_the_camera() -> Result<(), Box<dyn Error>> {
    let Some(level) = level("a10")? else {
        return Ok(());
    };
    let mut scripts = MissionScripts::start(level, Difficulty::Normal)?;
    let mut held = false;
    for _ in 0..30 * TICKS_PER_SECOND {
        scripts.tick(TickInput::default());
        held |= scripts.mission().cinematic_camera().is_some();
    }
    assert!(held, "a10's opening never placed the camera");
    Ok(())
}

#[test]
fn harder_difficulties_hurt_more() -> Result<(), Box<dyn Error>> {
    let Some(level) = level("a10")? else {
        return Ok(());
    };
    let values = &level.globals.difficulty;
    let easy = DifficultyScale::new(values, 0);
    let legendary = DifficultyScale::new(values, 3);
    assert!(legendary.enemy_damage > easy.enemy_damage);
    // (the player's team is a friend: `game_difficulty_get_team_value`
    // makes it sturdier on legendary too, if less than the enemy)
    assert!(legendary.enemy_vitality > easy.enemy_vitality);
    assert!(legendary.friend_vitality >= easy.friend_vitality);
    Ok(())
}
