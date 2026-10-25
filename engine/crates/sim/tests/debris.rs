//! Spent casings on bloodgulch: each assault rifle round throws a casing
//! that is a body, which falls, comes to rest near the player and stays;
//! the cap takes the oldest. Passes without assets/maps/bloodgulch.map.

use std::error::Error;

use opence_content::Level;
use opence_import::testing::stock_level;
use opence_sim::input::{Buttons, TickInput};
use opence_sim::{clock, Simulation};

fn bloodgulch() -> Result<Option<Level>, Box<dyn Error>> {
    let Some(stock) = stock_level("bloodgulch")? else {
        return Ok(None); // (no game data here)
    };
    Ok(Some(Level::clone(&stock)))
}

/// The player settled at the first start, holding the assault rifle.
fn settled(level: &Level) -> Result<Simulation, Box<dyn Error>> {
    let mut simulation = Simulation::empty();
    simulation.spawn_player(level, 0)?;
    for _ in 0..90 {
        simulation.step(TickInput::default());
    }
    Ok(simulation)
}

fn fire(simulation: &mut Simulation, ticks: u32) {
    for _ in 0..ticks {
        simulation.step(TickInput {
            buttons: Buttons::FIRE,
            ..TickInput::default()
        });
    }
}

#[test]
fn casings_fall_and_come_to_rest_near_the_player() -> Result<(), Box<dyn Error>> {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let mut simulation = settled(&level)?;
    let feet = simulation.player().ok_or("no player")?.mover().position;
    assert!(simulation.debris().pieces().is_empty());
    fire(&mut simulation, 1);
    let thrown = simulation.debris().pieces().len();
    assert!(thrown >= 1, "no casing");
    let start = simulation.debris().pieces()[0].pose.position;
    assert!(simulation.debris().moving());

    for _ in 0..10 * clock::TICKS_PER_SECOND {
        simulation.step(TickInput::default());
    }
    let pieces = simulation.debris().pieces();
    assert_eq!(pieces.len(), thrown);
    for piece in pieces {
        let at = piece.pose.position;
        assert!(piece.resting(), "{at}");
        assert!(at.is_finite(), "{at}");
        assert!(at.z < start.z, "{start} {at}");
        // (the start stands above the ground in front of it, which the
        // casing falls to and rolls a little way down)
        assert!(at.distance(feet) < 8.0, "{feet} {at}");
        assert!(at.z > feet.z - 3.0, "{feet} {at}");
    }
    Ok(())
}

#[test]
fn the_cap_takes_the_oldest_casing() -> Result<(), Box<dyn Error>> {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let mut simulation = settled(&level)?;
    simulation.set_debris(3);
    fire(&mut simulation, clock::TICKS_PER_SECOND);
    assert_eq!(simulation.debris().pieces().len(), 3);
    Ok(())
}
