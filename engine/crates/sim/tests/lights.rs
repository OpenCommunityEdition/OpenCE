//! What lights the player on bloodgulch: its flashlight, the functions its
//! objects compute each tick, and the effects its shots make. Passes
//! without assets/maps/bloodgulch.map.

use std::error::Error;

use opence_content::Level;
use opence_import::testing::stock_level;
use opence_sim::functions::{FunctionValues, IntegratedLight};
use opence_sim::input::{Buttons, TickInput};
use opence_sim::Simulation;

fn bloodgulch() -> Result<Option<Level>, Box<dyn Error>> {
    let Some(stock) = stock_level("bloodgulch")? else {
        return Ok(None); // (no game data here)
    };
    Ok(Some(Level::clone(&stock)))
}

fn held(buttons: Buttons) -> TickInput {
    TickInput {
        buttons,
        ..TickInput::default()
    }
}

#[test]
fn the_flashlight_switches_on_and_off() -> Result<(), Box<dyn Error>> {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let mut simulation = Simulation::empty();
    let handle = simulation.spawn_player(&level, 0)?;
    simulation.step(TickInput::default());
    simulation.step(held(Buttons::FLASHLIGHT));
    // (held, it does not switch again)
    for _ in 0..10 {
        simulation.step(held(Buttons::FLASHLIGHT));
    }
    let light: IntegratedLight = simulation.component(handle).ok_or("no flashlight")?;
    assert!(light.on);
    assert_eq!(light.power, 1.0);
    assert!(light.battery < 1.0);
    simulation.step(TickInput::default());
    simulation.step(held(Buttons::FLASHLIGHT));
    let light: IntegratedLight = simulation.component(handle).ok_or("no flashlight")?;
    assert!(!light.on);
    Ok(())
}

#[test]
fn the_players_functions_are_computed_and_its_shots_make_effects() -> Result<(), Box<dyn Error>> {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let mut simulation = Simulation::empty();
    let handle = simulation.spawn_player(&level, 0)?;
    for _ in 0..60 {
        simulation.step(TickInput::default());
    }
    let values: FunctionValues = simulation.component(handle).ok_or("no function values")?;
    // (a cyborg at full health exports its body vitality, 1, and is alive)
    assert!(values
        .incoming
        .iter()
        .all(|value| (0.0..=1.0).contains(value)));
    let mut effects = 0;
    for _ in 0..30 {
        simulation.step(held(Buttons::FIRE));
        effects += simulation.effects().len();
    }
    assert!(effects > 0, "the rifle's shots made no effects");
    Ok(())
}
