//! The first-person view and the HUD on bloodgulch: the rifle is drawn
//! with the hands from the camera, fires and reloads in its animations, is
//! hidden in the third person, and the HUD reads the player's shield,
//! rounds and grenades. Passes without assets/maps/bloodgulch.map.

use std::error::Error;
use std::path::Path;

use opence_content::Level;
use opence_import::{import_level, Cache};
use opence_sim::first_person::FirstPersonState;
use opence_sim::input::{Buttons, TickInput};
use opence_sim::player::View;
use opence_sim::{Simulation, Vec3};

fn bloodgulch() -> Result<Option<Level>, Box<dyn Error>> {
    let path = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../assets/maps/bloodgulch.map");
    let Ok(file) = std::fs::read(&path) else {
        return Ok(None); // (no game data here)
    };
    Ok(Some(import_level(&Cache::open(file)?)?))
}

fn held(buttons: Buttons) -> TickInput {
    TickInput {
        buttons,
        ..TickInput::default()
    }
}

fn settled(level: &Level) -> Result<Simulation, Box<dyn Error>> {
    let mut simulation = Simulation::empty();
    simulation.spawn_player(level, 0)?;
    for _ in 0..120 {
        simulation.step(TickInput::default());
    }
    Ok(simulation)
}

#[test]
fn the_rifle_and_hands_are_drawn_at_the_camera() -> Result<(), Box<dyn Error>> {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let simulation = settled(&level)?;
    let models = simulation.first_person().models();
    // (the hands and the rifle, every node of each in the rifle's graph)
    assert_eq!(models.len(), 2, "{} models", models.len());
    let hands = level
        .globals
        .first_person_hands
        .as_ref()
        .and_then(|tag| level.model_index(tag));
    assert_eq!(Some(models[0].model), hands);
    let eye = simulation.camera().transform.position;
    for model in models {
        assert_eq!(model.current.len(), level.models[model.model].nodes.len());
        for node in &model.current {
            let at = Vec3::from_array(node.position);
            assert!(
                at.distance(eye) < 1.0,
                "a node {} from the eye",
                at.distance(eye)
            );
        }
    }
    assert!(held_still(&simulation));
    Ok(())
}

/// Whether the weapon is held: still, or shown off.
fn held_still(simulation: &Simulation) -> bool {
    matches!(
        simulation.first_person().state(),
        Some(FirstPersonState::Idle | FirstPersonState::Posing)
    )
}

#[test]
fn the_rifle_fires_and_reloads_in_its_animations() -> Result<(), Box<dyn Error>> {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let mut simulation = settled(&level)?;
    // (the pistol: the rifle has no animation for firing)
    simulation.step(held(Buttons::SWAP));
    for _ in 0..120 {
        simulation.step(TickInput::default());
    }
    assert!(held_still(&simulation));
    let mut fired = false;
    for _ in 0..30 {
        simulation.step(held(Buttons::FIRE));
        fired |= simulation.first_person().state() == Some(FirstPersonState::PrimaryFire);
    }
    assert!(fired);
    let mut reloaded = false;
    simulation.step(held(Buttons::RELOAD));
    for _ in 0..30 {
        simulation.step(TickInput::default());
        reloaded |= simulation.first_person().state() == Some(FirstPersonState::ReloadWhileFull);
    }
    assert!(reloaded);
    // (the reload ends, and the rifle is held again)
    for _ in 0..600 {
        simulation.step(TickInput::default());
    }
    assert!(held_still(&simulation));
    Ok(())
}

#[test]
fn the_third_person_hides_it() -> Result<(), Box<dyn Error>> {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let mut simulation = settled(&level)?;
    assert!(simulation.set_player_view(View::ThirdPerson));
    simulation.step(TickInput::default());
    assert!(simulation.first_person().models().is_empty());
    assert!(simulation.set_player_view(View::FirstPerson));
    simulation.step(TickInput::default());
    assert_eq!(simulation.first_person().models().len(), 2);
    Ok(())
}

#[test]
fn the_hud_reads_the_player() -> Result<(), Box<dyn Error>> {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let mut simulation = settled(&level)?;
    let hud = simulation.hud().ok_or("no HUD")?;
    assert_eq!(hud.shield, 1.0);
    assert_eq!(hud.body, 1.0);
    assert!(!hud.dead);
    let weapon = hud.weapon.ok_or("no weapon")?;
    assert!(weapon.weapon.path.contains("assault rifle"));
    assert_eq!(weapon.magazines[0].loaded, 60);
    for _ in 0..20 {
        simulation.step(held(Buttons::FIRE));
    }
    let hud = simulation.hud().ok_or("no HUD")?;
    let loaded = hud.weapon.ok_or("no weapon")?.magazines[0].loaded;
    assert!(loaded < 60, "{loaded} loaded");
    // (no one else on the level: nothing on the motion sensor)
    assert!(hud.blips.is_empty());
    Ok(())
}

/// Whether `state` plays within `ticks` of pressing `buttons` once.
fn plays(
    simulation: &mut Simulation,
    buttons: Buttons,
    state: FirstPersonState,
    ticks: usize,
) -> bool {
    let mut played = false;
    simulation.step(held(buttons));
    for _ in 0..ticks {
        played |= simulation.first_person().state() == Some(state);
        simulation.step(TickInput::default());
    }
    played
}

#[test]
fn melee_throw_and_switch_play_their_animations() -> Result<(), Box<dyn Error>> {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let mut simulation = settled(&level)?;
    assert!(plays(
        &mut simulation,
        Buttons::MELEE,
        FirstPersonState::Melee,
        60
    ));
    for _ in 0..240 {
        simulation.step(TickInput::default());
    }
    // (the generic starting equipment carries none)
    assert!(simulation.give_grenades(0, 2));
    simulation.step(TickInput::default());
    assert!(plays(
        &mut simulation,
        Buttons::GRENADE,
        FirstPersonState::ThrowGrenade,
        60
    ));
    for _ in 0..240 {
        simulation.step(TickInput::default());
    }
    // (a switch readies the pistol: the source puts the rifle away and
    // readies the pistol in one call, unit_ready_desired_weapon, so only
    // the ready plays)
    let rifle = simulation.first_person().models()[1].model;
    assert!(plays(
        &mut simulation,
        Buttons::SWAP,
        FirstPersonState::Ready,
        60
    ));
    assert_ne!(simulation.first_person().models()[1].model, rifle);
    for _ in 0..600 {
        simulation.step(TickInput::default());
    }
    assert!(held_still(&simulation));
    Ok(())
}
