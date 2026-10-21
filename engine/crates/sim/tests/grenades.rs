//! Grenades on bloodgulch: a frag thrown at a cyborg's feet comes to rest
//! and blows its shield away, a plasma grenade sticks to it and detonates
//! on it, and the replay hashes the same. Passes without
//! assets/maps/bloodgulch.map.

use std::error::Error;
use std::path::Path;

use opence_content::{Level, ObjectKind};
use opence_import::{import_level, Cache};
use opence_sim::damage::{DamageEvent, Vitality};
use opence_sim::input::{Buttons, TickInput};
use opence_sim::objects::{ObjectHandle, ObjectSpawn};
use opence_sim::projectile::Detonation;
use opence_sim::units::Radians;
use opence_sim::{clock, Quat, Simulation, Transform, Vec3};

fn bloodgulch() -> Result<Option<Level>, Box<dyn Error>> {
    let path = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../assets/maps/bloodgulch.map");
    let Ok(file) = std::fs::read(&path) else {
        return Ok(None); // (no game data here)
    };
    Ok(Some(import_level(&Cache::open(file)?)?))
}

const FRAG: usize = 0;
const PLASMA: usize = 1;

fn held(buttons: Buttons) -> TickInput {
    TickInput {
        buttons,
        ..TickInput::default()
    }
}

/// The player settled at the first start, turned round from the wall it
/// faces to look across open ground, with two grenades of each kind and a
/// cyborg `distance` in front of it.
fn facing_a_cyborg(
    level: &Level,
    distance: f32,
) -> Result<(Simulation, ObjectHandle), Box<dyn Error>> {
    let mut simulation = Simulation::empty();
    simulation.spawn_player(level, 0)?;
    simulation.step(TickInput {
        yaw: Radians(std::f32::consts::PI),
        ..TickInput::default()
    });
    for _ in 0..90 {
        simulation.step(TickInput::default());
    }
    let (feet, yaw) = {
        let player = simulation.player().ok_or("no player")?;
        (player.mover().position, player.look().0 .0)
    };
    let unit = level
        .globals
        .multiplayer_unit
        .clone()
        .ok_or("no multiplayer unit")?;
    let forward = Quat::from_rotation_z(yaw) * Vec3::X;
    let cyborg = simulation.spawn(ObjectSpawn {
        kind: ObjectKind::Biped,
        tag: unit,
        transform: Transform {
            position: feet + forward * distance,
            rotation: Quat::from_rotation_z(yaw),
            scale: 1.0,
        },
        name: None,
        permutation: None,
        team: None,
    })?;
    assert!(simulation.give_grenades(FRAG, 2));
    assert!(simulation.give_grenades(PLASMA, 2));
    simulation.step(TickInput::default());
    Ok((simulation, cyborg))
}

/// Throws the kind in hand and waits up to `seconds` for it to detonate:
/// the detonation and the damage done to `cyborg`.
fn throw(
    simulation: &mut Simulation,
    cyborg: ObjectHandle,
    seconds: u32,
) -> Result<(Option<Detonation>, Vec<DamageEvent>), Box<dyn Error>> {
    simulation.step(held(Buttons::GRENADE));
    let mut events = Vec::new();
    for _ in 0..seconds * clock::TICKS_PER_SECOND {
        simulation.step(TickInput::default());
        events.extend(
            simulation
                .damage_events()
                .iter()
                .filter(|event| event.object == cyborg)
                .cloned(),
        );
        if let Some(detonation) = simulation.detonations().first() {
            return Ok((Some(detonation.clone()), events));
        }
    }
    Ok((None, events))
}

fn vitality(simulation: &Simulation, cyborg: ObjectHandle) -> Result<Vitality, Box<dyn Error>> {
    Ok(simulation
        .component::<Vitality>(cyborg)
        .ok_or("the cyborg has no vitality")?)
}

fn grenades(simulation: &Simulation) -> Result<[i16; 2], Box<dyn Error>> {
    Ok(simulation.player().ok_or("no player")?.inventory().grenades)
}

#[test]
fn a_frag_comes_to_rest_and_blows_the_shield_away() -> Result<(), Box<dyn Error>> {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let (mut simulation, cyborg) = facing_a_cyborg(&level, 2.0)?;
    let (detonation, events) = throw(&mut simulation, cyborg, 8)?;
    let detonation = detonation.ok_or("the frag never detonated")?;
    assert_eq!(grenades(&simulation)?, [1, 2]);
    assert!(detonation.attached.is_none(), "{detonation:?}");
    assert!(
        detonation.projectile.path.contains("frag"),
        "{detonation:?}"
    );
    let player = simulation.player().ok_or("no player")?.handle();
    assert_eq!(detonation.owner, player);
    assert!(!events.is_empty(), "{detonation:?}");
    assert!(events.iter().all(|event| event.owner == Some(player)));
    let hurt = vitality(&simulation, cyborg)?;
    assert!(hurt.shield < 1.0, "{hurt:?} {detonation:?}");
    assert!(simulation.player().ok_or("no player")?.flights().is_empty());
    Ok(())
}

#[test]
fn a_plasma_grenade_sticks_to_a_cyborg_and_detonates_on_it() -> Result<(), Box<dyn Error>> {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let (mut simulation, cyborg) = facing_a_cyborg(&level, 3.0)?;
    simulation.step(held(Buttons::SWITCH_GRENADE));
    simulation.step(TickInput::default());
    let (detonation, events) = throw(&mut simulation, cyborg, 8)?;
    let detonation = detonation.ok_or("the plasma grenade never detonated")?;
    assert_eq!(grenades(&simulation)?, [2, 1]);
    assert!(
        detonation.projectile.path.contains("plasma"),
        "{detonation:?}"
    );
    assert_eq!(detonation.attached, Some(cyborg), "{detonation:?}");
    assert!(!events.is_empty());
    let hurt = vitality(&simulation, cyborg)?;
    assert!(hurt.shield < 1.0, "{hurt:?}");
    Ok(())
}

#[test]
fn no_grenade_is_thrown_without_one() -> Result<(), Box<dyn Error>> {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let mut simulation = Simulation::empty();
    simulation.spawn_player(&level, 0)?;
    simulation.step(TickInput::default());
    simulation.step(held(Buttons::GRENADE));
    assert!(simulation.player().ok_or("no player")?.flights().is_empty());
    Ok(())
}

#[test]
fn a_grenade_replay_hashes_the_same() -> Result<(), Box<dyn Error>> {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let hash = || -> Result<u64, Box<dyn Error>> {
        let (mut simulation, cyborg) = facing_a_cyborg(&level, 2.0)?;
        throw(&mut simulation, cyborg, 8)?;
        Ok(simulation.world_hash())
    };
    assert_eq!(hash()?, hash()?);
    Ok(())
}
