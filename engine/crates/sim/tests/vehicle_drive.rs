//! Driving against the reference game: each vehicle trace in
//! engine/tests/traces (recorded by port/linux/game/movement_trace.c on
//! bloodgulch at 30 Hz) is driven again from its first tick with its
//! script's controls, a game tick a row, and the Warthog and the Ghost must
//! go where Halo's went (the Warthog steered by the turn of its wheels the
//! traces hold; VEHICLE_TRACE=1 prints each tick). The Scorpion and the
//! Banshee, which have no traces, must answer their controls as their tags
//! say, and a vehicle left alone must come to rest on bloodgulch's
//! ground and wake when driven. Passes without assets/maps/bloodgulch.map and b40.map.

use std::error::Error;
use std::f32::consts::FRAC_PI_2;
use std::path::Path;

use glam::{Vec2, Vec3};
use opence_content::{Level, Physics, Vehicle};
use opence_import::testing::{stock_level, stock_level_unheld};
use opence_physics::{LevelMesh, SurfaceMaterials, World, WorldSettings};
use opence_sim::vehicle::{game_tick, VehicleControls, VehicleLevel, VehicleState};

type Result = std::result::Result<(), Box<dyn Error>>;

/// The yaw the trace's driver starts looking at (movement_trace.c's).
const START_YAW: f32 = 1.895099;

fn bloodgulch() -> std::result::Result<Option<(Level, World)>, Box<dyn Error>> {
    let Some(stock) = stock_level("bloodgulch")? else {
        return Ok(None); // (no game data here)
    };
    let level = Level::clone(&stock);
    let mut world = World::new(WorldSettings::default())?;
    let bsp = level.structure_bsps.first().ok_or("no structure bsp")?;
    world.set_level(&LevelMesh::from_collision(&bsp.collision))?;
    Ok(Some((level, world)))
}

fn vehicle<'a>(
    level: &'a Level,
    path: &str,
) -> std::result::Result<(&'a Vehicle, &'a Physics), Box<dyn Error>> {
    let object = level
        .objects
        .iter()
        .find(|object| object.tag.path == path)
        .ok_or(format!("no {path}"))?;
    let vehicle = object.vehicle.as_ref().ok_or("not a vehicle")?;
    let tag = object.physics.as_ref().ok_or("no physics")?;
    let physics = level
        .physics
        .iter()
        .find(|physics| &physics.tag == tag)
        .ok_or("no physics tag")?;
    Ok((vehicle, physics))
}

/// A trace's tick: the vehicle's state after it.
fn read(name: &str) -> std::result::Result<Vec<Vec<f32>>, Box<dyn Error>> {
    let path = Path::new(env!("CARGO_MANIFEST_DIR")).join(format!("../../tests/traces/{name}.csv"));
    let text = std::fs::read_to_string(path)?;
    let mut rows = Vec::new();
    for line in text.lines().skip(1) {
        rows.push(
            line.split(',')
                .map(str::parse)
                .collect::<std::result::Result<Vec<f32>, _>>()?,
        );
    }
    Ok(rows)
}

fn vector(row: &[f32], at: usize) -> Vec3 {
    Vec3::new(row[at], row[at + 1], row[at + 2])
}

/// The state a row records.
fn state(row: &[f32], physics: &Physics) -> VehicleState {
    let mut state = VehicleState::placed(physics, Vec3::ZERO, vector(row, 7), vector(row, 10));
    state.position = vector(row, 1);
    state.velocity = vector(row, 4);
    state.angular_velocity = vector(row, 13);
    state.speed = row[16];
    state.turn = row[17];
    state.airborne_ticks = row[18] as u8;
    // (a Ghost left alone hovers fully; the traces don't hold it)
    state.hover = 1.0;
    state
}

/// The script's controls in the trace's tick `tick`.
fn controls(script: &str, tick: usize) -> VehicleControls {
    let step = FRAC_PI_2 / 90.0;
    let (throttle, turned) = match script.split('_').nth(1).unwrap_or("") {
        "drive" => (if tick < 90 { 1.0 } else { 0.0 }, 0),
        "turn" => (1.0, (tick + 1).min(90)),
        "brake" => (
            if tick < 60 {
                1.0
            } else if tick < 120 {
                -1.0
            } else {
                0.0
            },
            0,
        ),
        _ => (1.0, 0),
    };
    let yaw = START_YAW + step * turned as f32;
    VehicleControls {
        throttle: Vec2::new(throttle, 0.0),
        facing: Vec3::new(libm::cosf(yaw), libm::sinf(yaw), 0.0),
        seat_power: [1.0, 1.0],
        ..VehicleControls::default()
    }
}

/// How far the replay strays from the trace: the furthest apart they get,
/// and the vehicle's heading apart at the end.
struct Strayed {
    position: f32,
    heading: f32,
    speed: f32,
}

fn replay(
    level: &Level,
    world: &World,
    script: &str,
    path: &str,
    steered: bool,
) -> std::result::Result<Strayed, Box<dyn Error>> {
    let (vehicle, physics) = vehicle(level, path)?;
    let rows = read(script)?;
    let mut state = state(&rows[0], physics);
    let mut strayed = Strayed {
        position: 0.0,
        heading: 0.0,
        speed: 0.0,
    };
    let level = VehicleLevel {
        world,
        floor: 0.0,
        ceiling: 0.0,
        water: None,
        surfaces: None,
        materials: &[],
    };
    for (tick, row) in rows.iter().enumerate().skip(1) {
        let mut controls = controls(script, tick);
        if steered {
            // (the driver looks where Halo's wheels turned to this tick)
            let angle = if row[16] < 0.0 { -row[17] } else { row[17] };
            let left = state.up.cross(state.forward);
            controls.facing = state.forward * libm::cosf(angle) + left * libm::sinf(angle);
        }
        game_tick(&mut state, vehicle, physics, &controls, level)?;
        let position = (state.position - vector(row, 1)).length();
        let heading = libm::atan2f(state.forward.y, state.forward.x) - libm::atan2f(row[8], row[7]);
        let heading = libm::atan2f(libm::sinf(heading), libm::cosf(heading)).abs();
        if std::env::var_os("VEHICLE_TRACE").is_some() {
            println!(
                "{script} {tick}: {position:.3} off, heading {heading:.3}, speed {:.4}/{:.4}, turn {:.4}/{:.4}, z {:.3}/{:.3}, air {}/{}",
                state.speed, row[16], state.turn, row[17], state.position.z, row[3], state.airborne_ticks, row[18]
            );
        }
        strayed.position = strayed.position.max(position);
        strayed.heading = strayed.heading.max(heading);
        strayed.speed = strayed.speed.max((state.speed - row[16]).abs());
    }
    Ok(strayed)
}

fn heading(forward: Vec3) -> f32 {
    libm::atan2f(forward.y, forward.x)
}

/// An angle unwound to within a half turn.
fn unwound(angle: f32) -> f32 {
    libm::atan2f(libm::sinf(angle), libm::cosf(angle))
}

// (The recorder stepped the driver's look once a drawn frame, not once a
// tick, so the facing that reached Halo's vehicles in the turning scripts
// was faster and uneven, and is not in the traces. The Warthog's replays
// are steered by the turn of its wheels the traces hold instead; the
// Ghost's, which has no wheels, only by the script.)

#[test]
fn the_warthog_drives_as_halos_did() -> Result {
    let Some((level, world)) = bloodgulch()? else {
        return Ok(());
    };
    for (script, position, heading) in [
        ("warthog_drive", 0.05, 0.01),
        ("warthog_turn", 0.05, 0.01),
        ("warthog_brake", 0.05, 0.01),
        // (over the hills, two jumps and their landings apart)
        ("warthog_ramp", 1.0, 0.2),
    ] {
        let strayed = replay(&level, &world, script, "vehicles\\warthog\\warthog", true)?;
        assert!(
            strayed.position < position,
            "{script}: {} units",
            strayed.position
        );
        assert!(
            strayed.heading < heading,
            "{script}: {} radians",
            strayed.heading
        );
        assert!(strayed.speed < 1e-3, "{script}: {} speed", strayed.speed);
    }
    Ok(())
}

#[test]
fn the_ghost_drives_as_halos_did() -> Result {
    let Some((level, world)) = bloodgulch()? else {
        return Ok(());
    };
    for script in ["ghost_drive", "ghost_brake"] {
        let strayed = replay(&level, &world, script, "vehicles\\ghost\\ghost_mp", false)?;
        assert!(
            strayed.position < 0.2,
            "{script}: {} units",
            strayed.position
        );
        assert!(
            strayed.heading < 0.05,
            "{script}: {} radians",
            strayed.heading
        );
    }
    // (looking left, it turns left, as Halo's did)
    let (vehicle, physics) = vehicle(&level, "vehicles\\ghost\\ghost_mp")?;
    let rows = read("ghost_turn")?;
    let mut state = state(&rows[0], physics);
    let start = heading(state.forward);
    let level = VehicleLevel {
        world: &world,
        floor: 0.0,
        ceiling: 0.0,
        water: None,
        surfaces: None,
        materials: &[],
    };
    for tick in 1..rows.len() {
        game_tick(
            &mut state,
            vehicle,
            physics,
            &controls("ghost_turn", tick),
            level,
        )?;
    }
    let turned = unwound(heading(state.forward) - start);
    assert!(turned > 0.5, "{turned}");
    Ok(())
}

/// A vehicle set down where the traces' Warthog starts, and left to settle.
fn set_down(
    world: &World,
    vehicle: &Vehicle,
    physics: &Physics,
) -> std::result::Result<VehicleState, Box<dyn Error>> {
    let row = &read("warthog_drive")?[0];
    let forward = vector(row, 7);
    let mut state = VehicleState::placed(physics, vector(row, 1) + Vec3::Z, forward, Vec3::Z);
    let level = VehicleLevel {
        world,
        floor: 0.0,
        ceiling: 0.0,
        water: None,
        surfaces: None,
        materials: &[],
    };
    let still = VehicleControls {
        facing: forward,
        seat_power: [1.0, 1.0],
        ..VehicleControls::default()
    };
    for _ in 0..60 {
        game_tick(&mut state, vehicle, physics, &still, level)?;
    }
    Ok(state)
}

#[test]
fn the_scorpion_answers_its_controls_as_its_tag_says() -> Result {
    let Some((level, world)) = bloodgulch()? else {
        return Ok(());
    };
    let (vehicle, physics) = vehicle(&level, "vehicles\\scorpion\\scorpion_mp")?;
    let mut state = set_down(&world, vehicle, physics)?;
    let start = state.position;
    let forward = state.forward;
    let level = VehicleLevel {
        world: &world,
        floor: 0.0,
        ceiling: 0.0,
        water: None,
        surfaces: None,
        materials: &[],
    };
    // (forward: its acceleration a tick, to its fastest, the way it faces)
    let drive = VehicleControls {
        throttle: Vec2::new(1.0, 0.0),
        facing: forward,
        seat_power: [1.0, 1.0],
        ..VehicleControls::default()
    };
    for tick in 1..=90 {
        game_tick(&mut state, vehicle, physics, &drive, level)?;
        let expected =
            (vehicle.speed_acceleration * tick as f32).min(vehicle.maximum_forward_speed);
        assert!(
            (state.speed - expected).abs() < 1e-4,
            "tick {tick}: {}",
            state.speed
        );
    }
    let travelled = state.position - start;
    assert!(travelled.dot(forward) > 3.0, "{travelled}");
    // (looking left, it turns left)
    let start = heading(state.forward);
    for _ in 0..60 {
        let yaw = heading(state.forward) + std::f32::consts::FRAC_PI_4;
        let left = VehicleControls {
            facing: Vec3::new(libm::cosf(yaw), libm::sinf(yaw), 0.0),
            ..drive
        };
        game_tick(&mut state, vehicle, physics, &left, level)?;
    }
    let turned = unwound(heading(state.forward) - start);
    assert!(turned > 0.3, "{turned}");
    Ok(())
}

#[test]
fn the_banshee_answers_its_controls_as_its_tag_says() -> Result {
    let Some(level) = stock_level_unheld("b40")? else {
        return Ok(()); // (no game data here)
    };
    let Ok((vehicle, physics)) = vehicle(&level, "vehicles\\banshee\\banshee") else {
        return Err("b40 has no Banshee".into());
    };
    // (in the open air, nothing under it)
    let world = World::new(WorldSettings::default())?;
    let level = VehicleLevel {
        world: &world,
        floor: 0.0,
        ceiling: 0.0,
        water: None,
        surfaces: None,
        materials: &[],
    };
    let mut state = VehicleState::placed(physics, Vec3::ZERO, Vec3::X, Vec3::Z);
    let fly = VehicleControls {
        throttle: Vec2::new(1.0, 0.0),
        facing: Vec3::X,
        seat_power: [1.0, 1.0],
        ..VehicleControls::default()
    };
    // (forward, to its fastest, the way it faces; a player looking level
    // flies its nose up by its tag's fixed gun pitch, as in Halo)
    for _ in 0..150 {
        game_tick(&mut state, vehicle, physics, &fly, level)?;
    }
    let fastest = vehicle.maximum_forward_speed;
    assert!((state.speed - fastest).abs() < 1e-4, "{}", state.speed);
    let (along, climb) = (state.velocity.x, state.velocity.z);
    assert!(
        libm::hypotf(along, climb) > fastest * 0.95,
        "{}",
        state.velocity
    );
    let pitch = libm::atan2f(climb, along);
    assert!((pitch - vehicle.fixed_gun_pitch).abs() < 0.01, "{pitch}");
    // (looking left, it banks round to the left)
    let left = VehicleControls {
        facing: Vec3::Y,
        ..fly
    };
    for _ in 0..150 {
        game_tick(&mut state, vehicle, physics, &left, level)?;
    }
    let turned = heading(state.forward);
    assert!(turned > std::f32::consts::FRAC_PI_4, "{turned}");
    assert!(state.velocity.y > 0.0, "{}", state.velocity);
    Ok(())
}

#[test]
fn a_warthog_left_alone_comes_to_rest_and_wakes_when_driven() -> Result {
    let Some((level, world)) = bloodgulch()? else {
        return Ok(());
    };
    let (vehicle, physics) = vehicle(&level, "vehicles\\warthog\\warthog")?;
    let bsp = level.structure_bsps.first().ok_or("no structure bsp")?;
    let types: Vec<i16> = bsp
        .collision_materials
        .iter()
        .map(|material| material.material_type)
        .collect();
    let surfaces = SurfaceMaterials::new(&LevelMesh::from_collision(&bsp.collision), &types);
    let ground = VehicleLevel {
        world: &world,
        floor: 0.0,
        ceiling: 0.0,
        water: None,
        surfaces: Some(&surfaces),
        materials: &level.globals.materials,
    };
    let row = &read("warthog_drive")?[0];
    let mut state =
        VehicleState::placed(physics, vector(row, 1) + Vec3::Z, vector(row, 7), Vec3::Z);
    // (no one in it: it faces where it faces, as an empty vehicle does)
    let still = |state: &VehicleState| VehicleControls {
        facing: state.forward,
        seat_power: [1.0, 1.0],
        ..VehicleControls::default()
    };
    let mut ticks = 0;
    while !state.at_rest {
        ticks += 1;
        assert!(ticks < 600, "still moving: {}", state.velocity);
        let controls = still(&state);
        game_tick(&mut state, vehicle, physics, &controls, ground)?;
    }
    assert!(state.on_ground);
    // (at rest, and no one in it, it is not stepped, as seats.rs leaves it;
    // it stays at rest a further second)
    let resting = state.clone();
    for _ in 0..30 {
        if !state.at_rest {
            let controls = still(&state);
            game_tick(&mut state, vehicle, physics, &controls, ground)?;
        }
    }
    assert_eq!(state.position, resting.position);
    assert_eq!(state.forward, resting.forward);
    // (the throttle wakes it, and it drives off)
    let drive = VehicleControls {
        throttle: Vec2::new(1.0, 0.0),
        ..still(&state)
    };
    game_tick(&mut state, vehicle, physics, &drive, ground)?;
    assert!(!state.at_rest);
    for _ in 0..30 {
        game_tick(&mut state, vehicle, physics, &drive, ground)?;
    }
    assert!((state.position - resting.position).length() > 0.1);
    // (and a push wakes it as well)
    let mut pushed = resting;
    pushed.wake();
    assert!(!pushed.at_rest);
    Ok(())
}
