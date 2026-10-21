//! Damage on bloodgulch: the player's rifle empties a cyborg's shield and
//! then its body, its shield comes back after the stun, its melee strikes
//! one at arm's length, and the replay hashes the same. Passes without
//! assets/maps/bloodgulch.map.

use std::error::Error;
use std::path::Path;

use opence_content::{Level, ObjectKind};
use opence_import::{import_level, Cache};
use opence_sim::damage::{DamageEvent, Vitality};
use opence_sim::input::{Buttons, TickInput};
use opence_sim::objects::{ObjectHandle, ObjectSpawn};
use opence_sim::{clock, Quat, Simulation, Transform, Vec3};

fn bloodgulch() -> Result<Option<Level>, Box<dyn Error>> {
    let path = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../assets/maps/bloodgulch.map");
    let Ok(file) = std::fs::read(&path) else {
        return Ok(None); // (no game data here)
    };
    Ok(Some(import_level(&Cache::open(file)?)?))
}

/// How far in front of the first start the cyborg stands: the level is
/// 1.6 units on, at the player's eyes.
const CLEAR: f32 = 1.2;

fn held(buttons: Buttons) -> TickInput {
    TickInput {
        buttons,
        ..TickInput::default()
    }
}

/// The player settled at the first start, looking level, and a
/// multiplayer cyborg `distance` in front of its feet facing `turn` from
/// the way the player faces (0: away from it).
fn facing_a_cyborg(
    level: &Level,
    distance: f32,
    turn: f32,
) -> Result<(Simulation, ObjectHandle), Box<dyn Error>> {
    let mut simulation = Simulation::empty();
    simulation.spawn_player(level, 0)?;
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
            rotation: Quat::from_rotation_z(yaw + turn),
            scale: 1.0,
        },
        name: None,
        permutation: None,
        team: None,
    })?;
    simulation.step(TickInput::default());
    Ok((simulation, cyborg))
}

fn vitality(simulation: &Simulation, cyborg: ObjectHandle) -> Result<Vitality, Box<dyn Error>> {
    Ok(simulation
        .component::<Vitality>(cyborg)
        .ok_or("the cyborg has no vitality")?)
}

/// Fires (reloading when empty) until `done`, or `ticks` have passed; the
/// damage done to `cyborg`.
fn fire_until(
    simulation: &mut Simulation,
    cyborg: ObjectHandle,
    ticks: u32,
    mut done: impl FnMut(&Vitality) -> bool,
) -> Result<Vec<DamageEvent>, Box<dyn Error>> {
    let mut events = Vec::new();
    for tick in 0..ticks {
        let empty = simulation
            .player()
            .and_then(|player| player.weapon())
            .and_then(|(_, state)| state.magazines.first())
            .is_some_and(|magazine| magazine.loaded == 0);
        let buttons = if empty && tick % 2 == 0 {
            Buttons::RELOAD
        } else if empty {
            Buttons::default()
        } else {
            Buttons::FIRE
        };
        simulation.step(held(buttons));
        events.extend(
            simulation
                .damage_events()
                .iter()
                .filter(|event| event.object == cyborg)
                .cloned(),
        );
        if done(&vitality(simulation, cyborg)?) {
            break;
        }
    }
    Ok(events)
}

#[test]
fn the_rifle_takes_the_shield_then_the_body_then_its_life() -> Result<(), Box<dyn Error>> {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let (mut simulation, cyborg) = facing_a_cyborg(&level, CLEAR, 0.0)?;
    let whole = vitality(&simulation, cyborg)?;
    assert_eq!((whole.shield, whole.body), (1.0, 1.0));
    let events = fire_until(&mut simulation, cyborg, 1200, |vitality| vitality.dead)?;
    assert!(!events.is_empty(), "no bullet struck it");
    let player = simulation.player().ok_or("no player")?.handle();
    assert!(events.iter().all(|event| event.owner == Some(player)));
    // (the shield alone, until it fails)
    let depleted = events
        .iter()
        .position(|event| event.outcome.shield_depleted)
        .ok_or("the shield never failed")?;
    assert!(events[..depleted]
        .iter()
        .all(|event| event.outcome.shield > 0.0 && event.outcome.body == 0.0));
    // (then the body, to death)
    assert!(events[depleted + 1..]
        .iter()
        .all(|event| event.outcome.shield == 0.0));
    let last = events.last().ok_or("no events")?;
    assert!(last.outcome.killed, "{last:?}");
    let dead = vitality(&simulation, cyborg)?;
    assert!(dead.dead && dead.body <= 0.0, "{dead:?}");
    Ok(())
}

#[test]
fn a_shield_recharges_after_its_stun() -> Result<(), Box<dyn Error>> {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let (mut simulation, cyborg) = facing_a_cyborg(&level, CLEAR, 0.0)?;
    fire_until(&mut simulation, cyborg, 300, |vitality| {
        vitality.shield < 0.7
    })?;
    let hurt = vitality(&simulation, cyborg)?;
    assert!(hurt.shield < 0.7 && hurt.body == 1.0, "{hurt:?}");
    assert!(hurt.stun_ticks > 0);
    let model = level
        .collision_model(
            level
                .object(&level.globals.multiplayer_unit.clone().ok_or("no unit")?)
                .and_then(|object| object.collision_model.clone())
                .as_ref()
                .ok_or("no collision model")?,
        )
        .ok_or("no collision model")?;
    let tps = clock::TICKS_PER_SECOND as f32;
    let stun = (model.stun_time * tps) as u32;
    let recharge = (model.recharge_time * tps) as u32;
    // (held through the stun: a stun counted from the last hit)
    for _ in 0..hurt.stun_ticks.saturating_sub(2) {
        simulation.step(TickInput::default());
    }
    let stunned = vitality(&simulation, cyborg)?;
    assert_eq!(stunned.shield, hurt.shield, "{stunned:?}");
    assert!(hurt.stun_ticks <= stun);
    for _ in 0..recharge + 4 {
        simulation.step(TickInput::default());
    }
    let whole = vitality(&simulation, cyborg)?;
    assert_eq!(whole.shield, 1.0, "{whole:?}");
    Ok(())
}

#[test]
fn the_melee_strikes_one_at_arms_length() -> Result<(), Box<dyn Error>> {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let (mut simulation, cyborg) = facing_a_cyborg(&level, 0.5, std::f32::consts::PI)?;
    simulation.step(held(Buttons::MELEE));
    let events: Vec<_> = simulation
        .damage_events()
        .iter()
        .filter(|event| event.object == cyborg)
        .cloned()
        .collect();
    assert_eq!(events.len(), 1, "{events:?}");
    assert!(events[0].effect.path.contains("melee"), "{events:?}");
    assert!(vitality(&simulation, cyborg)?.shield < 1.0);
    // (held, it strikes once; and it recovers before the next)
    for _ in 0..5 {
        simulation.step(held(Buttons::MELEE));
        assert!(simulation.damage_events().is_empty());
    }
    simulation.step(TickInput::default());
    simulation.step(held(Buttons::MELEE));
    assert!(simulation.damage_events().is_empty());
    for _ in 0..clock::TICKS_PER_SECOND {
        simulation.step(TickInput::default());
    }
    simulation.step(held(Buttons::MELEE));
    assert_eq!(simulation.damage_events().len(), 1);
    Ok(())
}

#[test]
fn a_damage_replay_hashes_the_same() -> Result<(), Box<dyn Error>> {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let hash = || -> Result<u64, Box<dyn Error>> {
        let (mut simulation, cyborg) = facing_a_cyborg(&level, CLEAR, 0.0)?;
        fire_until(&mut simulation, cyborg, 200, |_| false)?;
        Ok(simulation.world_hash())
    };
    assert_eq!(hash()?, hash()?);
    Ok(())
}
