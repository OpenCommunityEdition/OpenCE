//! The sounds of play on Blood Gulch, played alone: firing, the bullets'
//! impacts, reloading, footsteps, picking up and a vehicle's engine each
//! voice their class, and none fails (needs assets/maps; passes without it).

use std::collections::BTreeMap;
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

/// What a stretch of play voiced: the most voices of each class at once
/// beyond those playing when it began, sampled each frame.
#[derive(Default, Debug)]
struct Heard(BTreeMap<String, u32>);

impl Heard {
    fn voiced(&self, class: &str) -> bool {
        self.0.get(class).is_some_and(|most| *most > 0)
    }
}

fn playing(engine: &Engine) -> BTreeMap<String, u32> {
    engine
        .audio_census()
        .into_iter()
        .map(|count| (count.class, count.playing))
        .collect()
}

/// Holds `input` for `frames` frames of a thirtieth, mixing each frame's
/// output as the app does (so that voices end) and listening.
fn play(engine: &Engine, input: InputState, frames: u32) -> Result<Heard> {
    let output = engine.audio_output();
    let start = playing(engine);
    let mut heard = Heard::default();
    for _ in 0..frames {
        engine.set_input(input);
        engine.advance(1.0 / 30.0)?;
        output.pull(output.sample_rate() / 30);
        for (class, now) in playing(engine) {
            let more = now.saturating_sub(start.get(&class).copied().unwrap_or(0));
            let most = heard.0.entry(class).or_default();
            *most = (*most).max(more);
        }
    }
    engine.set_input(still(0));
    Ok(heard)
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

fn first(engine: &Engine, kind: &str) -> Result<Value> {
    let rows = run(engine, &format!("objects {kind}"))?;
    rows.as_array()
        .and_then(|rows| rows.first())
        .cloned()
        .ok_or_else(|| format!("no {kind}").into())
}

fn teleport_to(engine: &Engine, row: &Value, up: f64) -> Result {
    let at = &row["position"];
    let axis = |axis: usize| at[axis].as_f64().unwrap_or(0.0);
    run(
        engine,
        &format!("teleport {} {} {}", axis(0), axis(1), axis(2) + up),
    )?;
    Ok(())
}

/// Blood Gulch, alone, the player at the first start; none without game data.
fn blood_gulch() -> Result<Option<Arc<Engine>>> {
    let map = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../assets/maps/bloodgulch.map");
    if !map.exists() {
        return Ok(None);
    }
    let cache = Path::new(env!("CARGO_TARGET_TMPDIR")).join("play_sounds");
    std::fs::create_dir_all(&cache)?;
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

/// The weapons the player carries, by tag (it is the level's only biped).
fn carried(engine: &Engine) -> Result<Vec<String>> {
    let player = first(engine, "biped")?;
    let id = player["id"].as_i64().ok_or("the player has no id")?;
    let data = run(engine, &format!("inspect {id}"))?;
    let data = data
        .as_array()
        .and_then(|rows| rows.first())
        .unwrap_or(&data);
    Ok(data["weapons"]
        .as_array()
        .map(|weapons| {
            weapons
                .iter()
                .filter_map(|weapon| weapon.as_str().map(str::to_owned))
                .collect()
        })
        .unwrap_or_default())
}

/// Adds what `more` voiced to `heard`.
fn and(mut heard: Heard, more: Heard) -> Heard {
    for (class, most) in more.0 {
        let entry = heard.0.entry(class).or_default();
        *entry = (*entry).max(most);
    }
    heard
}

#[test]
fn play_makes_its_sounds() -> Result {
    let Some(engine) = blood_gulch()? else {
        return Ok(()); // (no game data here)
    };
    let mut report = Vec::new();
    // firing at the ground ahead
    let down = InputState {
        pitch: -0.6,
        ..still(0)
    };
    play(&engine, down, 1)?;
    let fired = play(&engine, still(Buttons::FIRE.0), 45)?;
    report.push(("fire", fired));
    play(&engine, still(0), 60)?;
    let reloaded = and(
        play(&engine, still(Buttons::RELOAD.0), 3)?,
        play(&engine, still(0), 90)?,
    );
    report.push(("reload", reloaded));
    let swapped = and(
        play(&engine, still(Buttons::SWAP.0), 3)?,
        play(&engine, still(0), 60)?,
    );
    report.push(("swap", swapped));
    let walked = play(
        &engine,
        InputState {
            forward: 127,
            ..still(0)
        },
        90,
    )?;
    report.push(("walk", walked));
    // (a weapon of a kind it does not carry, picked up and readied)
    let have = carried(&engine)?;
    let weapons = run(&engine, "objects weapon")?;
    let other = weapons
        .as_array()
        .and_then(|rows| {
            rows.iter().find(|row| {
                row["tag"]
                    .as_str()
                    .is_some_and(|tag| !have.iter().any(|had| had == tag))
            })
        })
        .cloned()
        .ok_or("no weapon of another kind")?;
    teleport_to(&engine, &other, 0.3)?;
    let picked = and(
        play(&engine, still(Buttons::ACTION.0), 3)?,
        play(&engine, still(0), 30)?,
    );
    let now = carried(&engine)?;
    report.push(("pick up", picked));
    let vehicle = first(&engine, "warthog")?;
    let id = vehicle["id"].as_i64().ok_or("the warthog has no id")?;
    run(&engine, &format!("enter {id} 0"))?;
    play(&engine, still(0), 30)?;
    let drove = play(
        &engine,
        InputState {
            forward: 127,
            ..still(0)
        },
        90,
    )?;
    // (an engine loops from the time it is in range, so it is heard as
    // playing, not as begun)
    let engines = playing(&engine).get("vehicle_engine").copied().unwrap_or(0);
    report.push(("drive", drove));
    for (what, heard) in &report {
        println!("{what}: {:?}", heard.0);
    }
    let failed: Vec<_> = engine
        .audio_census()
        .into_iter()
        .filter(|count| count.failed > 0)
        .collect();
    println!("failed: {failed:?}");
    let heard = |what: &str| {
        report
            .iter()
            .find(|(name, _)| *name == what)
            .map(|(_, heard)| heard)
    };
    let voiced = |what: &str, class: &str| heard(what).is_some_and(|heard| heard.voiced(class));
    assert!(voiced("fire", "weapon_fire"), "firing made no weapon_fire");
    assert!(
        voiced("fire", "projectile_impact"),
        "the shots made no impact"
    );
    assert!(
        voiced("reload", "weapon_reload"),
        "reloading made no weapon_reload"
    );
    assert!(
        voiced("swap", "weapon_ready"),
        "the swap made no weapon_ready"
    );
    assert!(
        voiced("walk", "unit_footsteps"),
        "walking made no footsteps"
    );
    assert!(
        now.iter().any(|tag| other["tag"].as_str() == Some(tag)),
        "{other} was not picked up: {now:?}"
    );
    assert!(
        voiced("pick up", "weapon_ready"),
        "the pickup readied nothing"
    );
    println!("engines playing while driving: {engines}");
    assert!(engines > 0, "driving made no engine");
    assert!(failed.is_empty(), "{failed:?}");
    Ok(())
}
