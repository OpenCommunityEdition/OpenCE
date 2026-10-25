//! The effects of play on Blood Gulch: firing makes the weapon's muzzle
//! flash and the bullets' impacts, a grenade its detonation, and a Warthog
//! driven over the ground its wheels' slipping on the ground's material
//! (needs assets/maps; passes without it). A Ghost ridden makes no
//! thrusters' blasts: its effect is Blood Gulch's antigravity particles,
//! but its model has no hover thrusters for `create_ghost_effect` to blow
//! from, so the original makes none either.

use std::collections::BTreeSet;
use std::path::Path;
use std::sync::Arc;

use opence_ffi::{Engine, InputState, LevelGeometry};
use opence_sim::input::Buttons;
use serde_json::Value;

type Result<T = ()> = std::result::Result<T, Box<dyn std::error::Error>>;

fn run(engine: &Engine, line: &str) -> Result<Value> {
    let output = engine.run_command(line.into());
    if !output.ok {
        return Err(format!("{line}: {}", output.text).into());
    }
    Ok(serde_json::from_str(&output.json)?)
}

fn still(buttons: u32) -> InputState {
    InputState {
        buttons,
        forward: 0,
        right: 0,
        up: 0,
        yaw: 0.0,
        pitch: 0.0,
    }
}

/// The effects that ran in a stretch of play: their numbers and tags.
#[derive(Default, Debug)]
struct Seen {
    numbers: BTreeSet<u64>,
    paths: BTreeSet<String>,
}

impl Seen {
    fn any(&self, word: &str) -> bool {
        self.paths.iter().any(|path| path.contains(word))
    }
}

/// Holds `input` for `frames` frames of a thirtieth, listing the effects
/// that begin in them.
fn play(engine: &Engine, input: InputState, frames: u32) -> Result<Seen> {
    let before: BTreeSet<u64> = effects(engine)?
        .into_iter()
        .map(|(number, _)| number)
        .collect();
    let mut seen = Seen::default();
    for _ in 0..frames {
        engine.set_input(input);
        engine.advance(1.0 / 30.0)?;
        for (number, path) in effects(engine)? {
            if !before.contains(&number) {
                seen.numbers.insert(number);
                seen.paths.insert(path);
            }
        }
    }
    engine.set_input(still(0));
    Ok(seen)
}

/// The last effects begun: their numbers and tags.
fn effects(engine: &Engine) -> Result<Vec<(u64, String)>> {
    let rows = run(engine, "started")?;
    Ok(rows
        .as_array()
        .map(Vec::as_slice)
        .unwrap_or_default()
        .iter()
        .filter_map(|row| Some((row["number"].as_u64()?, row["path"].as_str()?.to_owned())))
        .collect())
}

fn first(engine: &Engine, kind: &str) -> Result<Value> {
    let rows = run(engine, &format!("objects {kind}"))?;
    rows.as_array()
        .and_then(|rows| rows.first())
        .cloned()
        .ok_or_else(|| format!("no {kind}").into())
}

/// Blood Gulch, alone, the player at the first start; none without game data.
fn blood_gulch() -> Result<Option<Arc<Engine>>> {
    let map = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../assets/maps/bloodgulch.map");
    if !map.exists() {
        return Ok(None);
    }
    let cache = opence_import::testing::cache_folder();
    let geometry = LevelGeometry::load(
        map.to_string_lossy().into(),
        cache.to_string_lossy().into(),
        0,
    )?;
    let engine = Engine::new();
    engine.start_level(geometry, 1)?;
    engine.spawn_player(0)?;
    play(&engine, still(0), 30)?;
    Ok(Some(engine))
}

#[test]
fn play_makes_its_effects() -> Result {
    let Some(engine) = blood_gulch()? else {
        return Ok(()); // (no game data here)
    };
    let down = InputState {
        pitch: -0.6,
        ..still(0)
    };
    play(&engine, down, 1)?;
    let fired = play(&engine, still(Buttons::FIRE.0), 20)?;
    play(&engine, still(0), 60)?;
    let walked = play(
        &engine,
        InputState {
            forward: 127,
            ..still(0)
        },
        45,
    )?;
    let mut thrown = play(&engine, still(Buttons::GRENADE.0), 3)?;
    thrown.paths.extend(play(&engine, still(0), 150)?.paths);
    let vehicle = first(&engine, "warthog")?;
    let id = vehicle["id"].as_i64().ok_or("the warthog has no id")?;
    run(&engine, &format!("enter {id} 0"))?;
    let sat = play(&engine, still(0), 30)?;
    let drove = play(
        &engine,
        InputState {
            forward: 127,
            ..still(0)
        },
        90,
    )?;
    for (what, seen) in [
        ("fire", &fired),
        ("walk", &walked),
        ("grenade", &thrown),
        ("sit", &sat),
        ("drive", &drove),
    ] {
        println!("{what}: {} effects {:?}", seen.numbers.len(), seen.paths);
    }
    assert!(fired.any("fire bullet"), "firing made no muzzle flash");
    assert!(fired.any("impact"), "the bullets made no impacts");
    assert!(thrown.any("explosion"), "the grenade made no explosion");
    assert!(drove.any("slip"), "the wheels made no slipping");
    Ok(())
}

#[test]
fn a_ghost_ridden_makes_no_thrusters_blasts_as_its_model_has_no_thrusters() -> Result {
    let Some(engine) = blood_gulch()? else {
        return Ok(()); // (no game data here)
    };
    let ghost = first(&engine, "ghost")?;
    let id = ghost["id"].as_i64().ok_or("the ghost has no id")?;
    run(&engine, &format!("enter {id} 0"))?;
    let rode = play(
        &engine,
        InputState {
            forward: 127,
            ..still(0)
        },
        60,
    )?;
    println!("ride: {} effects {:?}", rode.numbers.len(), rode.paths);
    assert!(
        !rode.any("antigravity"),
        "the ghost's thrusters made effects: {:?}",
        rode.paths
    );
    Ok(())
}
