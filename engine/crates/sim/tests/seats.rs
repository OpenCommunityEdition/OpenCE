//! Seats on bloodgulch: the player walks to each of a Warthog's seats and
//! enters and leaves it with the action button, drives it, fires its gun
//! from the gunner's seat and its own rifle from the passenger's, and a
//! Warthog destroyed takes no rider. Passes without
//! assets/maps/bloodgulch.map.

use std::error::Error;
use std::path::Path;
use std::sync::Arc;

use opence_content::{Level, ObjectKind, VehicleSeat};
use opence_import::{import_level, Cache};
use opence_sim::animation::{self, AnimationPlayer};
use opence_sim::input::{Buttons, TickInput};
use opence_sim::objects::{ObjectHandle, ObjectSpawn};
use opence_sim::units::Radians;
use opence_sim::{clock, Quat, Simulation, Transform, Vec3};

type Result<T = ()> = std::result::Result<T, Box<dyn Error>>;

const WARTHOG: &str = "vehicles\\warthog\\warthog";

fn bloodgulch() -> Result<Option<Level>> {
    let path = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../assets/maps/bloodgulch.map");
    let Ok(file) = std::fs::read(&path) else {
        return Ok(None); // (no game data here)
    };
    Ok(Some(import_level(&Cache::open(file)?)?))
}

fn pressed(buttons: Buttons) -> TickInput {
    TickInput {
        buttons,
        ..TickInput::default()
    }
}

/// The player settled at the first start, and a Warthog settled on the
/// ground 5 units in front of it.
fn beside_a_warthog(level: &Level) -> Result<(Simulation, ObjectHandle)> {
    warthog_rolled(level, 0.0)
}

/// The same, the Warthog dropped rolled `roll` radians about its length.
fn warthog_rolled(level: &Level, roll: f32) -> Result<(Simulation, ObjectHandle)> {
    let mut simulation = Simulation::empty();
    simulation.spawn_player(level, 0)?;
    for _ in 0..90 {
        simulation.step(TickInput::default());
    }
    let (feet, yaw) = {
        let player = simulation.player().ok_or("no player")?;
        (player.mover().position, player.look().0 .0)
    };
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
            position: feet + turn * Vec3::new(5.0, 0.0, 0.6 + roll.abs()),
            rotation: turn
                * Quat::from_rotation_z(std::f32::consts::FRAC_PI_2)
                * Quat::from_rotation_x(roll),
            scale: 1.0,
        },
        name: None,
        permutation: None,
        team: None,
    })?;
    for _ in 0..2 * clock::TICKS_PER_SECOND {
        simulation.step(TickInput::default());
    }
    Ok((simulation, warthog))
}

/// Walks the player to `target` (across the ground), turning to it as it
/// goes; whether it got there.
fn walk_to(simulation: &mut Simulation, target: impl Fn(&Simulation) -> Option<Vec3>) -> bool {
    for _ in 0..10 * clock::TICKS_PER_SECOND {
        let (Some(player), Some(target)) = (simulation.player(), target(simulation)) else {
            return false;
        };
        let to = target - player.mover().position;
        let distance = to.truncate().length();
        if distance < 0.2 {
            return true;
        }
        let heading = libm::atan2f(to.y, to.x);
        let turn = libm::remainderf(heading - player.look().0 .0, std::f32::consts::TAU);
        simulation.step(TickInput {
            yaw: Radians(turn),
            forward: if distance < 0.5 { 40 } else { 127 },
            ..TickInput::default()
        });
    }
    false
}

/// The seats of the Warthog's kind: their flags, by number.
fn seat_flags(level: &Level) -> Result<Vec<u32>> {
    let vehicle = level
        .objects
        .iter()
        .find(|object| object.tag.path.eq_ignore_ascii_case(WARTHOG))
        .and_then(|object| object.vehicle.as_ref())
        .ok_or("no warthog vehicle")?;
    Ok(vehicle.seats.iter().map(|seat| seat.flags).collect())
}

/// The seat with `flag` and none of `not`.
fn seat_with(flags: &[u32], flag: u32, not: u32) -> Result<usize> {
    flags
        .iter()
        .position(|&flags| flags & flag != 0 && flags & not == 0)
        .ok_or_else(|| "no such seat".into())
}

/// Walks to seat `seat`'s entrance and presses the action button.
fn enter(simulation: &mut Simulation, warthog: ObjectHandle, seat: usize) -> Result {
    if !walk_to(simulation, |simulation| {
        simulation.rides().entrance(warthog, seat)
    }) {
        return Err(format!("never reached seat {seat}'s entrance").into());
    }
    simulation.step(TickInput::default());
    simulation.step(pressed(Buttons::ACTION));
    Ok(())
}

fn seated(simulation: &Simulation) -> Option<usize> {
    simulation.player()?.seated().map(|seated| seated.seat)
}

#[test]
fn the_player_enters_and_leaves_each_seat_of_a_warthog() -> Result {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let flags = seat_flags(&level)?;
    let (mut simulation, warthog) = beside_a_warthog(&level)?;
    for (seat, flags) in flags.iter().enumerate() {
        if flags & VehicleSeat::REQUIRES_DRIVER != 0 {
            continue; // (the player alone has no driver)
        }
        enter(&mut simulation, warthog, seat)?;
        assert_eq!(seated(&simulation), Some(seat), "seat {seat}");
        // (it sits in the seat, and moves with it)
        let sits = simulation.rides().seat(warthog, seat).ok_or("no seat")?;
        let body = simulation.player().ok_or("no player")?.mover().position;
        assert!(
            body.distance(sits) < 0.01,
            "seat {seat}: {body} not at {sits}"
        );
        for _ in 0..30 {
            simulation.step(TickInput::default());
        }
        assert_eq!(seated(&simulation), Some(seat));
        simulation.step(TickInput::default());
        simulation.step(pressed(Buttons::ACTION));
        assert_eq!(seated(&simulation), None, "seat {seat}: never left");
        let entrance = simulation
            .rides()
            .entrance(warthog, seat)
            .ok_or("no entrance")?;
        let feet = simulation.player().ok_or("no player")?.mover().position;
        assert!(
            feet.distance(entrance) < 0.2,
            "seat {seat}: left at {feet}, not {entrance}"
        );
        for _ in 0..30 {
            simulation.step(TickInput::default());
        }
    }
    Ok(())
}

#[test]
fn a_flipped_warthog_is_turned_back_over_and_then_entered() -> Result {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let flags = seat_flags(&level)?;
    let driver = seat_with(&flags, VehicleSeat::DRIVER, 0)?;
    // (the globals hold an angle above level, under a quarter turn)
    let angle = level.globals.player.minimum_vehicle_flipping_angle;
    assert!(
        angle > 0.0 && angle < std::f32::consts::FRAC_PI_2,
        "{angle}"
    );
    let (mut simulation, warthog) = warthog_rolled(&level, std::f32::consts::PI)?;
    let state = |simulation: &Simulation| {
        simulation
            .rides()
            .ride(warthog)
            .map(|ride| ride.state.clone())
            .ok_or("no warthog")
    };
    assert!(
        state(&simulation)?.is_flipped(),
        "{:?}",
        state(&simulation)?.up
    );
    // (the action button, by the driver's door, turns it over rather than
    // seating the player; the door, upside down, is against its hull)
    let beside = |simulation: &Simulation| {
        let entrance = simulation.rides().entrance(warthog, driver)?;
        let centre = simulation.transform(warthog)?.position;
        let out = (entrance - centre).truncate().normalize_or_zero();
        Some(entrance + out.extend(0.0) * 0.6)
    };
    if !walk_to(&mut simulation, beside) {
        return Err("never reached the driver's door".into());
    }
    simulation.step(TickInput::default());
    simulation.step(pressed(Buttons::ACTION));
    assert_eq!(seated(&simulation), None);
    assert!(state(&simulation)?.upending.is_some());
    for _ in 0..3 * clock::TICKS_PER_SECOND {
        simulation.step(TickInput::default());
    }
    let up = state(&simulation)?.up;
    assert!(up.z > 0.9, "it stands with {up} up");
    assert_eq!(state(&simulation)?.upending, None);
    // (and up, it is entered)
    enter(&mut simulation, warthog, driver)?;
    assert_eq!(seated(&simulation), Some(driver));
    Ok(())
}

#[test]
fn the_driver_drives_the_warthog_where_it_looks() -> Result {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let flags = seat_flags(&level)?;
    let driver = seat_with(&flags, VehicleSeat::DRIVER, 0)?;
    let (mut simulation, warthog) = beside_a_warthog(&level)?;
    enter(&mut simulation, warthog, driver)?;
    assert_eq!(seated(&simulation), Some(driver));
    let start = simulation.transform(warthog).ok_or("no warthog")?.position;
    // (it turns to the driver's look, a quarter turn and more from where it
    // stood, then drives off along it)
    for _ in 0..4 * clock::TICKS_PER_SECOND {
        simulation.step(TickInput {
            forward: 127,
            ..TickInput::default()
        });
    }
    let pose = simulation.transform(warthog).ok_or("no warthog")?;
    let end = pose.position;
    assert!(
        end.distance(start) > 5.0,
        "the warthog went from {start} to {end}"
    );
    let yaw = simulation.player().ok_or("no player")?.look().0 .0;
    let look = Vec3::new(libm::cosf(yaw), libm::sinf(yaw), 0.0);
    let forward = pose.rotation * Vec3::X;
    assert!(
        forward.dot(look) > 0.9,
        "the warthog faces {forward}, not {look}"
    );
    // (its driver went with it, and its seats powered up)
    let sits = simulation.rides().seat(warthog, driver).ok_or("no seat")?;
    let body = simulation.player().ok_or("no player")?.mover().position;
    assert!(body.distance(sits) < 0.01);
    let ride = simulation.rides().ride(warthog).ok_or("no ride")?;
    assert_eq!(ride.power[0], 1.0);
    Ok(())
}

#[test]
fn the_gunner_fires_the_warthogs_gun_and_the_passenger_its_rifle() -> Result {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let flags = seat_flags(&level)?;
    let (mut simulation, warthog) = beside_a_warthog(&level)?;
    let rounds = |simulation: &Simulation| {
        simulation
            .player()
            .and_then(|player| player.weapon())
            .and_then(|(_, state)| state.magazines.first().map(|magazine| magazine.loaded))
    };
    // (the gun's projectiles, from the armory)
    let gun = {
        let vehicle = level
            .objects
            .iter()
            .find(|object| object.tag.path.eq_ignore_ascii_case(WARTHOG))
            .and_then(|object| object.vehicle.as_ref())
            .ok_or("no warthog vehicle")?;
        let armory = simulation.player().ok_or("no player")?.armory();
        let weapon = vehicle
            .initial_weapons
            .first()
            .and_then(|tag| armory.find(tag))
            .and_then(|index| armory.get(index))
            .ok_or("no warthog gun")?;
        weapon
            .projectiles
            .iter()
            .flatten()
            .map(|(tag, _)| tag.clone())
            .collect::<Vec<_>>()
    };
    assert!(!gun.is_empty());
    let flying = |simulation: &Simulation, gun_rounds: bool| {
        simulation.player().is_some_and(|player| {
            player
                .flights()
                .iter()
                .any(|flight| gun.contains(&flight.tag) == gun_rounds)
        })
    };
    // (the gunner's seat fires the gun, and not the rifle)
    let gunner = seat_with(&flags, VehicleSeat::GUNNER, VehicleSeat::DRIVER)?;
    enter(&mut simulation, warthog, gunner)?;
    assert_eq!(seated(&simulation), Some(gunner));
    let before = rounds(&simulation);
    for _ in 0..20 {
        simulation.step(pressed(Buttons::FIRE));
    }
    assert!(flying(&simulation, true), "the gun never fired");
    assert!(!flying(&simulation, false), "the rifle fired from the gun");
    assert_eq!(rounds(&simulation), before);
    // (its rounds pass out of the Warthog it fires from, not stopping on
    // it: Halo's Warthog has no vitality for them to take)
    let at = simulation.transform(warthog).ok_or("no warthog")?.position;
    assert!(simulation.player().is_some_and(|player| {
        player
            .flights()
            .iter()
            .any(|flight| gun.contains(&flight.tag) && flight.position.distance(at) > 3.0)
    }));
    simulation.step(TickInput::default());
    simulation.step(pressed(Buttons::ACTION));
    assert_eq!(seated(&simulation), None);
    for _ in 0..3 * clock::TICKS_PER_SECOND {
        simulation.step(TickInput::default());
    }
    // (the passenger's seat allows weapons: its rifle fires)
    let passenger = seat_with(&flags, VehicleSeat::ALLOWS_WEAPONS, VehicleSeat::GUNNER)?;
    enter(&mut simulation, warthog, passenger)?;
    assert_eq!(seated(&simulation), Some(passenger));
    let before = rounds(&simulation);
    for _ in 0..20 {
        simulation.step(pressed(Buttons::FIRE));
    }
    assert!(rounds(&simulation) < before, "the rifle never fired");
    assert!(flying(&simulation, false), "no rifle rounds in flight");
    Ok(())
}

/// A frag the player threw behind it, to detonate again where a test

#[test]
fn a_destroyed_warthog_takes_no_rider() -> Result {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let flags = seat_flags(&level)?;
    let driver = seat_with(&flags, VehicleSeat::DRIVER, 0)?;
    let (mut simulation, warthog) = beside_a_warthog(&level)?;
    assert!(simulation.kill(warthog));
    simulation.step(TickInput::default());
    // (the walk to it fails or the action button does nothing)
    let _ = enter(&mut simulation, warthog, driver);
    assert_eq!(seated(&simulation), None, "it entered a wreck");
    Ok(())
}

#[test]
fn a_warthog_destroyed_under_its_driver_throws_it_out() -> Result {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let flags = seat_flags(&level)?;
    let driver = seat_with(&flags, VehicleSeat::DRIVER, 0)?;
    let (mut simulation, warthog) = beside_a_warthog(&level)?;
    enter(&mut simulation, warthog, driver)?;
    assert_eq!(seated(&simulation), Some(driver));
    assert!(simulation.kill(warthog));
    simulation.step(TickInput::default());
    assert_eq!(seated(&simulation), None, "the wreck kept its driver");
    Ok(())
}

/// The node matrices of the drawn object standing at `position`.
fn nodes_at(simulation: &mut Simulation, position: Vec3) -> Vec<[f32; 3]> {
    let mut found = Vec::new();
    simulation.visit_drawn(|drawn| {
        if drawn.current.position.distance(position) < 1.0e-4 {
            if let Some(nodes) = drawn.nodes {
                found = nodes.current.iter().map(|node| node.forward).collect();
            }
        }
    });
    found
}

#[test]
fn a_driven_warthog_rides_its_suspension_and_turns_its_wheels() -> Result {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let flags = seat_flags(&level)?;
    let driver = seat_with(&flags, VehicleSeat::DRIVER, 0)?;
    let (mut simulation, warthog) = beside_a_warthog(&level)?;
    // (animated as the app animates it)
    let tag = level
        .objects
        .iter()
        .find(|object| object.tag.path.eq_ignore_ascii_case(WARTHOG))
        .map(|object| object.tag.clone())
        .ok_or("no warthog")?;
    let definition = level.object(&tag).ok_or("no definition")?;
    let graph = definition.animation_graph.as_ref().ok_or("no graph")?;
    let graph = Arc::new(level.animation_graph(graph).ok_or("no graph")?.clone());
    let model = definition
        .model
        .as_ref()
        .and_then(|model| level.model_index(model))
        .map(|model| Arc::new(level.models[model].clone()));
    let standing = animation::standing_animation(&graph, true);
    assert!(simulation.animate(warthog, AnimationPlayer::new(graph, model, standing)));
    // (standing on the ground, its wheels touch it)
    let resting = simulation
        .rides()
        .ride(warthog)
        .ok_or("no ride")?
        .suspension;
    assert!(
        resting.iter().any(|&shift| shift != 0),
        "a Warthog on the ground has no suspension: {resting:?}"
    );
    enter(&mut simulation, warthog, driver)?;
    let position = simulation.transform(warthog).ok_or("no warthog")?.position;
    let still = nodes_at(&mut simulation, position);
    assert!(!still.is_empty(), "the Warthog has no nodes");
    let mut moved = false;
    for _ in 0..2 * clock::TICKS_PER_SECOND {
        simulation.step(TickInput {
            forward: 127,
            ..TickInput::default()
        });
        let position = simulation.transform(warthog).ok_or("no warthog")?.position;
        let rotation = simulation.transform(warthog).ok_or("no warthog")?.rotation;
        // (the nodes relative to the body: its wheels turn and its springs
        // work as it drives)
        let driving = nodes_at(&mut simulation, position);
        let local = |nodes: &[[f32; 3]]| -> Vec<Vec3> {
            nodes
                .iter()
                .map(|&forward| rotation.inverse() * Vec3::from_array(forward))
                .collect()
        };
        let before: Vec<Vec3> = still.iter().map(|&f| Vec3::from_array(f)).collect();
        if local(&driving)
            .iter()
            .zip(&before)
            .any(|(now, then)| now.distance(*then) > 0.1)
        {
            moved = true;
            break;
        }
    }
    assert!(moved, "the Warthog's nodes never moved as it drove");
    Ok(())
}
