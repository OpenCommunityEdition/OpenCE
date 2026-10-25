//! Damage's pushes on bloodgulch: a frag beside a Warthog at rest wakes it
//! and throws it away and up; a frag throws a living player off its feet;
//! a Warthog driven into a player throws it and hurts it, its driver
//! owning the damage. Passes without assets/maps/bloodgulch.map.

use std::error::Error;

use opence_content::{Level, ObjectKind, VehicleSeat};
use opence_import::testing::stock_level;
use opence_sim::input::{Buttons, TickInput};
use opence_sim::objects::{ObjectHandle, ObjectSpawn};
use opence_sim::projectile::Detonation;
use opence_sim::units::Radians;
use opence_sim::{clock, Quat, Simulation, Transform, Vec3};

type Result<T = ()> = std::result::Result<T, Box<dyn Error>>;

const WARTHOG: &str = "vehicles\\warthog\\warthog";

fn bloodgulch() -> Result<Option<Level>> {
    let Some(stock) = stock_level("bloodgulch")? else {
        return Ok(None); // (no game data here)
    };
    Ok(Some(Level::clone(&stock)))
}

/// The player settled at the first start, looking across open ground, and
/// the detonation of a frag it threw there (a grenade's explosion to
/// script).
fn settled(level: &Level) -> Result<(Simulation, Detonation)> {
    let mut simulation = Simulation::empty();
    simulation.spawn_player(level, 0)?;
    simulation.step(TickInput {
        yaw: Radians(std::f32::consts::PI),
        ..TickInput::default()
    });
    for _ in 0..90 {
        simulation.step(TickInput::default());
    }
    assert!(simulation.give_grenades(0, 1));
    simulation.step(TickInput {
        buttons: Buttons::GRENADE,
        ..TickInput::default()
    });
    for _ in 0..8 * clock::TICKS_PER_SECOND {
        simulation.step(TickInput::default());
        if let Some(detonation) = simulation.detonations().first() {
            let detonation = detonation.clone();
            return Ok((simulation, detonation));
        }
    }
    Err("the frag never detonated".into())
}

/// Where the player stands and the way it looks, along the ground.
fn standing(simulation: &Simulation) -> Result<(Vec3, f32)> {
    let player = simulation.player().ok_or("no player")?;
    Ok((player.mover().position, player.look().0 .0))
}

/// A Warthog `distance` in front of the player, side on, settled.
fn warthog(simulation: &mut Simulation, level: &Level, distance: f32) -> Result<ObjectHandle> {
    let (feet, yaw) = standing(simulation)?;
    let tag = level
        .objects
        .iter()
        .filter(|object| object.kind == ObjectKind::Vehicle)
        .map(|object| object.tag.clone())
        .find(|tag| tag.path.eq_ignore_ascii_case(WARTHOG))
        .ok_or("no warthog")?;
    let turn = Quat::from_rotation_z(yaw);
    let warthog = simulation.spawn(ObjectSpawn {
        kind: ObjectKind::Vehicle,
        tag,
        transform: Transform {
            position: feet + turn * Vec3::new(distance, 0.0, 0.6),
            rotation: turn * Quat::from_rotation_z(std::f32::consts::FRAC_PI_2),
            scale: 1.0,
        },
        name: None,
        permutation: None,
        team: None,
    })?;
    for _ in 0..4 * clock::TICKS_PER_SECOND {
        simulation.step(TickInput::default());
    }
    Ok(warthog)
}

/// Detonates `grenade` at `point` and runs a tick.
fn blast(simulation: &mut Simulation, grenade: &Detonation, point: Vec3) {
    simulation.detonate(Detonation {
        point,
        attached: None,
        attached_damage: None,
        ..grenade.clone()
    });
    simulation.step(TickInput::default());
}

#[test]
fn a_frag_beside_a_warthog_at_rest_wakes_it_and_throws_it() -> Result {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let (mut simulation, grenade) = settled(&level)?;
    let warthog = warthog(&mut simulation, &level, 8.0)?;
    let ride = simulation.rides().ride(warthog).ok_or("no ride")?;
    assert!(ride.state.at_rest, "the warthog never came to rest");
    let before = ride.state.position;
    // (on its near side, as low as the ground there leaves it in sight:
    // it is woken and goes away)
    let (feet, yaw) = standing(&simulation)?;
    let towards = Vec3::new(libm::cosf(yaw), libm::sinf(yaw), 0.0);
    let point = [0.3f32, 0.6, 1.0, 1.5]
        .iter()
        .map(|up| before - towards * 1.2 + Vec3::Z * *up)
        .find(|point| simulation.level_clear(*point, before + Vec3::Z * 0.1))
        .ok_or("nowhere beside the warthog in sight of it")?;
    blast(&mut simulation, &grenade, point);
    let ride = simulation.rides().ride(warthog).ok_or("no ride")?;
    assert!(!ride.state.at_rest, "the blast never woke it");
    let velocity = ride.state.velocity;
    assert!(velocity.dot(towards) > 0.0, "{velocity}");
    for _ in 0..clock::TICKS_PER_SECOND {
        simulation.step(TickInput::default());
    }
    let after = simulation
        .rides()
        .ride(warthog)
        .ok_or("no ride")?
        .state
        .position;
    assert!(
        (after - feet).dot(towards) > (before - feet).dot(towards) + 0.1,
        "the warthog went from {before} to {after}"
    );
    Ok(())
}

#[test]
fn a_frag_throws_a_living_player_off_its_feet() -> Result {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let (mut simulation, grenade) = settled(&level)?;
    let (feet, yaw) = standing(&simulation)?;
    let towards = Vec3::new(libm::cosf(yaw), libm::sinf(yaw), 0.0);
    let (other, _) = simulation.add_player(&level, 0, None)?;
    let at = feet + towards * 6.0;
    assert!(simulation.teleport_player(other, at + Vec3::Z * 0.2));
    for _ in 0..clock::TICKS_PER_SECOND {
        simulation.step(TickInput::default());
    }
    let there = simulation.players()[other].mover().position;
    blast(
        &mut simulation,
        &grenade,
        there - towards * 1.0 + Vec3::Z * 0.3,
    );
    let thrown = simulation.players()[other].mover();
    assert!(thrown.velocity.dot(towards) > 0.5, "{:?}", thrown.velocity);
    assert!(thrown.velocity.z > 0.0, "{:?}", thrown.velocity);
    Ok(())
}

#[test]
fn a_warthog_driven_into_a_player_throws_it_and_hurts_it() -> Result {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let mut simulation = Simulation::empty();
    simulation.spawn_player(&level, 0)?;
    for _ in 0..90 {
        simulation.step(TickInput::default());
    }
    let warthog = warthog(&mut simulation, &level, 5.0)?;
    let driver = level
        .objects
        .iter()
        .find(|object| object.tag.path.eq_ignore_ascii_case(WARTHOG))
        .and_then(|object| object.vehicle.as_ref())
        .and_then(|vehicle| {
            vehicle
                .seats
                .iter()
                .position(|seat| seat.flags & VehicleSeat::DRIVER != 0)
        })
        .ok_or("no driver's seat")?;
    assert!(simulation.enter_vehicle(warthog, driver));
    let ahead = TickInput {
        forward: 127,
        ..TickInput::default()
    };
    // (under way and straight, the other player dropped in its path, 5
    // on)
    for _ in 0..4 * clock::TICKS_PER_SECOND {
        simulation.step(ahead);
    }
    let ride = simulation
        .rides()
        .ride(warthog)
        .ok_or("no ride")?
        .state
        .clone();
    let heading = ride.velocity.truncate().normalize_or_zero().extend(0.0);
    assert!(heading != Vec3::ZERO, "the warthog never got going");
    let (other, victim) = simulation.add_player(&level, 0, None)?;
    assert!(simulation.teleport_player(other, ride.position + heading * 5.0 + Vec3::Z));
    let driven = simulation.players()[0].handle();
    let collision = level
        .globals
        .falling
        .vehicle_collision_damage
        .clone()
        .ok_or("no vehicle collision damage")?;
    let mut struck = None;
    for _ in 0..4 * clock::TICKS_PER_SECOND {
        simulation.step(ahead);
        if let Some(event) = simulation
            .damage_events()
            .iter()
            .find(|event| event.object == victim && event.effect == collision)
        {
            struck = Some(event.clone());
            break;
        }
    }
    let event = struck.ok_or("the warthog never struck the player")?;
    assert_eq!(event.owner, Some(driven), "{event:?}");
    let thrown = simulation.players()[other].mover().velocity;
    assert!(thrown.dot(heading) > 1.0, "{thrown}");
    Ok(())
}
