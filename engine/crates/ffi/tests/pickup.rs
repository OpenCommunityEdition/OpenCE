//! Picking up on a multiplayer map played alone: the map's netgame weapons
//! and equipment lie about, as in a game of Slayer, and the action button
//! takes one (needs assets/maps; passes without it).

use std::path::Path;
use std::sync::Arc;

use opence_ffi::{Engine, InputState, LevelGeometry};
use serde_json::Value;

type Result<T = ()> = std::result::Result<T, Box<dyn std::error::Error>>;

fn run(engine: &Engine, line: &str) -> Result<Value> {
    let output = engine.run_command(line.into());
    if !output.ok {
        return Err(format!("{line}: {}", output.text).into());
    }
    Ok(serde_json::from_str(&output.json)?)
}

fn ticks(engine: &Engine, count: u32) -> Result {
    for _ in 0..count {
        engine.advance(1.0 / 30.0)?;
    }
    Ok(())
}

fn press(engine: &Engine, buttons: u32) {
    engine.set_input(InputState {
        buttons,
        forward: 0,
        right: 0,
        up: 0,
        yaw: 0.0,
        pitch: 0.0,
    });
}

/// The weapons lying about (a carried weapon is no object).
fn lying(engine: &Engine) -> Result<Vec<Value>> {
    let rows = run(engine, "objects weapon")?;
    Ok(rows.as_array().cloned().unwrap_or_default())
}

/// The tags of the weapons the player carries.
fn carried(engine: &Engine) -> Result<Vec<String>> {
    let rows = run(engine, "objects cyborg")?;
    let id = rows
        .as_array()
        .and_then(|rows| rows.first())
        .and_then(|row| row["id"].as_i64())
        .ok_or("no player")?;
    let player = run(engine, &format!("inspect {id}"))?;
    Ok(player["weapons"]
        .as_array()
        .into_iter()
        .flatten()
        .filter_map(|tag| tag.as_str().map(str::to_owned))
        .collect())
}

/// Blood Gulch, alone, the player at the first start; none without game data.
fn blood_gulch() -> Result<Option<Arc<Engine>>> {
    let maps = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../assets/maps");
    let map = maps.join("bloodgulch.map");
    if !map.exists() {
        return Ok(None);
    }
    let cache = Path::new(env!("CARGO_TARGET_TMPDIR")).join("pickup");
    std::fs::create_dir_all(&cache)?;
    let geometry = LevelGeometry::load(
        map.to_string_lossy().into(),
        cache.to_string_lossy().into(),
        0,
    )?;
    let engine = Engine::new();
    engine.start_level(geometry, 1)?;
    engine.spawn_player(0)?;
    ticks(&engine, 2)?;
    Ok(Some(engine))
}

#[test]
fn blood_gulch_alone_has_its_netgame_weapons_and_equipment() -> Result {
    let Some(engine) = blood_gulch()? else {
        return Ok(()); // (no game data here)
    };
    let weapons = lying(&engine)?;
    assert!(
        weapons.len() >= 10,
        "the weapons lying about: {}",
        weapons.len()
    );
    let equipment = run(&engine, "objects equipment")?;
    let equipment = equipment.as_array().map_or(0, Vec::len);
    assert!(
        equipment >= 4,
        "the grenades and powerups lying about: {equipment}"
    );
    Ok(())
}

#[test]
fn the_action_button_takes_a_weapon_lying_at_the_feet() -> Result {
    let Some(engine) = blood_gulch()? else {
        return Ok(());
    };
    let before = carried(&engine)?;
    assert_eq!(
        before.len(),
        2,
        "the player starts with two weapons: {before:?}"
    );
    // (a weapon of a kind the player does not carry)
    let weapon = lying(&engine)?
        .into_iter()
        .find(|row| {
            row["tag"]
                .as_str()
                .is_some_and(|tag| !before.iter().any(|c| c == tag))
        })
        .ok_or("no weapon of another kind lies about")?;
    let at = &weapon["position"];
    let coordinate = |axis: usize| at[axis].as_f64().unwrap_or(0.0);
    run(
        &engine,
        &format!(
            "teleport {} {} {}",
            coordinate(0),
            coordinate(1),
            coordinate(2) + 0.3
        ),
    )?;
    ticks(&engine, 20)?;
    press(&engine, opence_sim::input::Buttons::ACTION.0);
    ticks(&engine, 3)?;
    press(&engine, 0);
    ticks(&engine, 3)?;
    let tag = weapon["tag"].as_str().unwrap_or_default();
    let now = carried(&engine)?;
    assert!(
        now.iter().any(|carried| carried == tag),
        "{tag} is not carried: {now:?}"
    );
    assert_eq!(now.len(), 2, "the held weapon was dropped for it: {now:?}");
    Ok(())
}
