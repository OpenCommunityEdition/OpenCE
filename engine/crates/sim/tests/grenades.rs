//! Grenades thrown on bloodgulch with their tags: a frag bounces off the
//! ground, rolls to rest and detonates there; a plasma thrown at another
//! player sticks to it and detonates on it. Passes without
//! assets/maps/bloodgulch.map.

use std::error::Error;
use std::path::Path;

use opence_content::Level;
use opence_import::{import_level, Cache};
use opence_sim::input::{Buttons, TickInput};
use opence_sim::projectile::{Detonation, Flight};
use opence_sim::units::Radians;
use opence_sim::{clock, Simulation, Vec3};

type Result<T = ()> = std::result::Result<T, Box<dyn Error>>;

/// The frag's kind and the plasma's, in the globals' order.
const FRAG: usize = 0;
const PLASMA: usize = 1;

fn bloodgulch() -> Result<Option<Level>> {
    let path = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../assets/maps/bloodgulch.map");
    let Ok(file) = std::fs::read(&path) else {
        return Ok(None); // (no game data here)
    };
    Ok(Some(import_level(&Cache::open(file)?)?))
}

/// The player settled at the first start, looking across open ground.
fn settled(level: &Level) -> Result<Simulation> {
    let mut simulation = Simulation::empty();
    simulation.spawn_player(level, 0)?;
    simulation.step(TickInput {
        yaw: Radians(std::f32::consts::PI),
        ..TickInput::default()
    });
    for _ in 0..90 {
        simulation.step(TickInput::default());
    }
    Ok(simulation)
}

/// The player's grenade in flight.
fn grenade(simulation: &Simulation) -> Option<Flight> {
    simulation.player()?.flights().first().cloned()
}

/// Throws the grenade the player has ready, and steps until it detonates,
/// handing `watch` the grenade before each tick.
fn throw(simulation: &mut Simulation, mut watch: impl FnMut(&Flight)) -> Result<Detonation> {
    simulation.step(TickInput {
        buttons: Buttons::GRENADE,
        ..TickInput::default()
    });
    for _ in 0..10 * clock::TICKS_PER_SECOND {
        if let Some(flight) = grenade(simulation) {
            watch(&flight);
        }
        simulation.step(TickInput::default());
        if let Some(detonation) = simulation.detonations().first() {
            return Ok(detonation.clone());
        }
    }
    Err("the grenade never detonated".into())
}

#[test]
fn a_frag_bounces_rolls_to_rest_and_detonates_there() -> Result {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let mut simulation = settled(&level)?;
    let feet = simulation.player().ok_or("no player")?.mover().position;
    assert!(simulation.give_grenades(FRAG, 1));
    let mut bounced = false;
    let mut falling = false;
    let mut rest = None;
    let detonation = throw(&mut simulation, |flight| {
        bounced |= falling && flight.velocity.z > 0.0;
        falling = flight.velocity.z < -0.5;
        if rest.is_none() && flight.fuse.as_ref().is_some_and(|fuse| fuse.at_rest) {
            rest = Some(flight.position);
        }
    })?;
    assert!(bounced, "it never bounced");
    let rest = rest.ok_or("it never came to rest")?;
    assert!(
        detonation.point.distance(rest) < 0.05,
        "it rested at {rest} and detonated at {}",
        detonation.point
    );
    assert!(
        (detonation.point - feet).truncate().length() > 2.0,
        "it detonated at {} from {feet}",
        detonation.point
    );
    assert_eq!(detonation.attached, None);
    assert!(detonation.effect.is_some());
    Ok(())
}

#[test]
fn a_plasma_sticks_to_the_player_it_strikes() -> Result {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let mut simulation = settled(&level)?;
    let player = simulation.player().ok_or("no player")?;
    let (feet, yaw) = (player.mover().position, player.look().0 .0);
    let towards = Vec3::new(libm::cosf(yaw), libm::sinf(yaw), 0.0);
    let (other, victim) = simulation.add_player(&level, 0, None)?;
    assert!(simulation.teleport_player(other, feet + towards * 3.0 + Vec3::Z * 0.2));
    for _ in 0..clock::TICKS_PER_SECOND {
        simulation.step(TickInput::default());
    }
    assert!(simulation.give_grenades(PLASMA, 1));
    simulation.step(TickInput {
        buttons: Buttons::SWITCH_GRENADE,
        ..TickInput::default()
    });
    let mut stuck = false;
    let detonation = throw(&mut simulation, |flight| {
        stuck |= flight
            .fuse
            .as_ref()
            .and_then(|fuse| fuse.attached)
            .is_some_and(|attachment| attachment.parent == Some(victim));
    })?;
    assert!(stuck, "it never stuck to the other player");
    assert_eq!(detonation.attached, Some(victim), "{detonation:?}");
    assert!(detonation.attached_damage.is_some());
    let there = simulation.players()[other].mover().position;
    assert!(
        detonation.point.distance(there) < 1.0,
        "it detonated at {} by the player at {there}",
        detonation.point
    );
    Ok(())
}
