//! The player's weapons on bloodgulch: it starts with the assault rifle and
//! the pistol, fires the rifle at its rate, its bullets fly and strike the
//! level, and it reloads and changes weapons. Passes without
//! assets/maps/bloodgulch.map.

use std::error::Error;
use std::path::Path;

use opence_content::{Level, ObjectKind};
use opence_import::{import_level, Cache};
use opence_physics::Struck;
use opence_sim::armory::WeaponItem;
use opence_sim::input::{Buttons, TickInput};
use opence_sim::objects::ObjectSpawn;
use opence_sim::units::Radians;
use opence_sim::{Simulation, Transform};

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

/// The rounds loaded in the weapon in the player's hands.
fn loaded(simulation: &Simulation) -> i16 {
    simulation
        .player()
        .and_then(|player| player.weapon())
        .and_then(|(_, state)| state.magazines.first())
        .map_or(-1, |magazine| magazine.loaded)
}

fn path(simulation: &Simulation) -> String {
    simulation
        .player()
        .and_then(|player| player.weapon())
        .map(|(tag, _)| tag.path.clone())
        .unwrap_or_default()
}

/// A simulation with the player settled at the first start, looking
/// steeply down so that its bullets strike the ground.
fn settled(level: &Level) -> Result<Simulation, Box<dyn Error>> {
    let mut simulation = Simulation::empty();
    simulation.spawn_player(level, 0)?;
    simulation.step(TickInput {
        pitch: Radians(-1.2),
        ..TickInput::default()
    });
    for _ in 0..90 {
        simulation.step(TickInput::default());
    }
    Ok(simulation)
}

#[test]
fn the_player_starts_with_the_rifle_and_the_pistol() -> Result<(), Box<dyn Error>> {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let simulation = settled(&level)?;
    let player = simulation.player().ok_or("no player")?;
    let carried: Vec<String> = player
        .inventory()
        .weapons
        .iter()
        .filter_map(|carried| player.armory().get(carried.armory))
        .map(|weapon| weapon.tag.path.clone())
        .collect();
    assert_eq!(carried.len(), 2, "{carried:?}");
    assert!(carried[0].contains("assault rifle"), "{carried:?}");
    assert!(carried[1].contains("pistol"), "{carried:?}");
    assert_eq!(loaded(&simulation), 60);
    Ok(())
}

#[test]
fn the_rifle_fires_and_its_bullets_strike_the_ground() -> Result<(), Box<dyn Error>> {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let mut simulation = settled(&level)?;
    let mut impacts = Vec::new();
    for _ in 0..60 {
        simulation.step(held(Buttons::FIRE));
        impacts.extend(simulation.impacts().iter().cloned());
    }
    let fired = 60 - loaded(&simulation);
    // (the rifle fires from 15 to 16 rounds a second, once it is ready)
    assert!((10..=16).contains(&fired), "{fired} rounds");
    for _ in 0..30 {
        simulation.step(TickInput::default());
        impacts.extend(simulation.impacts().iter().cloned());
    }
    assert!(!impacts.is_empty());
    assert!(impacts.iter().all(|impact| impact.struck == Struck::Level));
    assert!(impacts.iter().all(|impact| impact.damage.is_some()));
    let player = simulation.player().ok_or("no player")?;
    let feet = player.mover().position;
    assert!(impacts
        .iter()
        .all(|impact| impact.point.distance(feet) < 5.0 && impact.normal.z > 0.5));
    // (every bullet has struck, and its object is gone)
    assert!(player.flights().is_empty());
    let projectiles = simulation
        .objects()
        .into_iter()
        .filter(|&handle| {
            simulation
                .object(handle)
                .is_some_and(|object| object.kind == ObjectKind::Projectile)
        })
        .count();
    assert_eq!(projectiles, 0);
    Ok(())
}

#[test]
fn the_player_reloads_and_changes_weapons() -> Result<(), Box<dyn Error>> {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let mut simulation = settled(&level)?;
    for _ in 0..30 {
        simulation.step(held(Buttons::FIRE));
    }
    let left = loaded(&simulation);
    assert!(left < 60);
    simulation.step(held(Buttons::RELOAD));
    // (the rifle's reload takes 3.7 seconds)
    for _ in 0..300 {
        simulation.step(TickInput::default());
    }
    assert_eq!(loaded(&simulation), 60);
    assert!(path(&simulation).contains("assault rifle"));
    simulation.step(held(Buttons::SWAP));
    // (held, it changes once)
    for _ in 0..10 {
        simulation.step(held(Buttons::SWAP));
    }
    assert!(
        path(&simulation).contains("pistol"),
        "{}",
        path(&simulation)
    );
    simulation.step(TickInput::default());
    simulation.step(held(Buttons::SWAP));
    assert!(path(&simulation).contains("assault rifle"));
    Ok(())
}

#[test]
fn a_weapons_replay_hashes_the_same() -> Result<(), Box<dyn Error>> {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let hash = || -> Result<u64, Box<dyn Error>> {
        let mut simulation = settled(&level)?;
        for tick in 0..120 {
            let buttons = if tick % 40 < 25 {
                Buttons::FIRE
            } else {
                Buttons::default()
            };
            simulation.step(TickInput {
                buttons,
                yaw: Radians(0.01),
                ..TickInput::default()
            });
        }
        Ok(simulation.world_hash())
    };
    assert_eq!(hash()?, hash()?);
    Ok(())
}

#[test]
fn weapons_are_picked_up_dropped_and_their_rounds_taken() -> Result<(), Box<dyn Error>> {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let mut simulation = settled(&level)?;
    let (feet, armory) = {
        let player = simulation.player().ok_or("no player")?;
        (player.mover().position, player.armory().clone())
    };
    let tag_of = |name: &str| {
        (0..armory.len())
            .filter_map(|index| armory.get(index))
            .find(|weapon| weapon.tag.path.contains(name))
            .map(|weapon| weapon.tag.clone())
    };
    let shotgun = tag_of("shotgun").ok_or("no shotgun")?;
    let rifle = tag_of("assault rifle").ok_or("no rifle")?;
    let lying = |tag| ObjectSpawn {
        kind: ObjectKind::Weapon,
        tag,
        transform: Transform {
            position: feet,
            ..Transform::default()
        },
        name: None,
        permutation: None,
        team: None,
    };
    // (a rifle at its feet: its rounds are taken, and it stays)
    for _ in 0..30 {
        simulation.step(held(Buttons::FIRE));
    }
    let total = |simulation: &Simulation| {
        simulation
            .player()
            .and_then(|player| player.weapon())
            .map_or(0, |(_, state)| state.magazines[0].total)
    };
    let before = total(&simulation);
    let lying_rifle = simulation.spawn(lying(rifle.clone()))?;
    simulation.step(TickInput::default());
    assert!(total(&simulation) > before);
    let item = simulation
        .component::<WeaponItem>(lying_rifle)
        .ok_or("the rifle keeps no rounds")?;
    assert!(item.0.magazines[0].total < 180 || item.0.magazines[0].loaded < 60);
    simulation.remove(lying_rifle);
    // (a shotgun: walked over, nothing; picked up, in place of the rifle)
    let lying_shotgun = simulation.spawn(lying(shotgun.clone()))?;
    simulation.step(TickInput::default());
    assert!(path(&simulation).contains("assault rifle"));
    simulation.step(held(Buttons::ACTION));
    assert!(
        path(&simulation).contains("shotgun"),
        "{}",
        path(&simulation)
    );
    assert!(simulation.object(lying_shotgun).is_none());
    let dropped: Vec<_> = simulation
        .objects()
        .into_iter()
        .filter(|&handle| {
            simulation
                .object(handle)
                .is_some_and(|object| object.tag == rifle)
        })
        .collect();
    assert_eq!(dropped.len(), 1);
    let kept = simulation
        .component::<WeaponItem>(dropped[0])
        .ok_or("the dropped rifle keeps no rounds")?;
    assert!(kept.0.magazines[0].loaded < 60, "{:?}", kept.0.magazines);
    // (held, it is picked up once: the rifle is not taken back)
    for _ in 0..10 {
        simulation.step(held(Buttons::ACTION));
    }
    assert!(path(&simulation).contains("shotgun"));
    Ok(())
}
