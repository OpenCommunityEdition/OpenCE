//! The test map's stations, each run on the galleries made over
//! bloodgulch and wizard: what they measured (they pass without
//! assets/maps).

use std::path::Path;
use std::sync::Arc;

use opence_ffi::{Engine, LevelGeometry};
use serde_json::Value;

type Result<T = ()> = std::result::Result<T, Box<dyn std::error::Error>>;

fn folder(name: &str) -> std::io::Result<std::path::PathBuf> {
    let folder = Path::new(env!("CARGO_TARGET_TMPDIR")).join(name);
    let _ = std::fs::remove_dir_all(&folder);
    std::fs::create_dir_all(&folder)?;
    Ok(folder)
}

/// Runs `line`, which must work, and its JSON.
fn run(engine: &Engine, line: &str) -> Result<Value> {
    let output = engine.run_command(line.into());
    if !output.ok {
        return Err(format!("{line}: {}", output.text).into());
    }
    Ok(serde_json::from_str(&output.json)?)
}

/// Runs a station to its end (at most `seconds`), and its results.
fn station(engine: &Engine, line: &str, seconds: u32) -> Result<Vec<Value>> {
    run(engine, line)?;
    for _ in 0..seconds * 30 {
        engine.advance(1.0 / 30.0)?;
        let shown = run(engine, "station")?;
        if shown["run"]["done"] == Value::Bool(true) {
            let status = shown["run"]["status"].as_str().unwrap_or("");
            assert!(status.contains("done"), "{line}: {status}");
            let results = shown["run"]["results"].as_array().ok_or("no results")?;
            return Ok(results.clone());
        }
    }
    Err(format!("{line} did not finish in {seconds} s").into())
}

fn number(value: &Value, name: &str) -> f64 {
    value[name].as_f64().unwrap_or(f64::NAN)
}

/// An engine on the galleries made over bloodgulch and wizard, its player
/// spawned, in a folder of `name`; none where there is no game data.
fn galleries(name: &str) -> Result<Option<Arc<Engine>>> {
    let maps = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../assets/maps");
    let names = ["bloodgulch.map", "wizard.map"];
    if !names.iter().all(|name| maps.join(name).exists()) {
        return Ok(None);
    }
    let folder = folder(name)?;
    for name in names {
        std::os::unix::fs::symlink(maps.join(name).canonicalize()?, folder.join(name))?;
    }
    let cache = folder.join("cache");
    let gathered = folder.join("devmap.map").to_string_lossy().into_owned();
    let geometry = LevelGeometry::load(gathered, cache.to_string_lossy().into_owned(), 0)?;
    let engine = Engine::new();
    engine.start_level(geometry, 1)?;
    engine.spawn_player(0)?;
    engine.advance(1.0 / 30.0)?;
    Ok(Some(engine))
}

#[test]
fn the_animation_station_plays_every_animation_of_the_graph() -> Result {
    let Some(engine) = galleries("stations_animation")? else {
        return Ok(()); // (no game data here)
    };
    // with no filter, the status counts the whole graph: "animation: 1/N name"
    run(&engine, "station animation")?;
    engine.advance(1.0 / 30.0)?;
    let shown = run(&engine, "station")?;
    let status = shown["run"]["status"].as_str().unwrap_or("").to_owned();
    let count: usize = status
        .split_once('/')
        .and_then(|(_, rest)| rest.split_whitespace().next())
        .ok_or_else(|| format!("no count in {status}"))?
        .parse()?;
    assert!(count > 100, "{status}");
    run(&engine, "station stop")?;

    let played = station(&engine, "station animation", 1200)?;
    assert_eq!(played.len(), count);
    for (at, animation) in played.iter().enumerate() {
        assert_eq!(animation["index"].as_u64(), Some(u64::try_from(at)?));
        assert!(number(animation, "frames") >= 1.0, "{animation}");
    }
    Ok(())
}

#[test]
fn the_stations_run_and_measure() -> Result {
    let Some(engine) = galleries("stations_galleries")? else {
        return Ok(()); // (no game data here)
    };

    // listed, none running; a name that is not one is refused
    let listed = run(&engine, "station")?;
    assert_eq!(listed["stations"].as_array().map(Vec::len), Some(6));
    assert!(listed["run"].is_null());
    assert!(!engine.run_command("station nowhere".into()).ok);
    assert!(!engine.run_command("station stop".into()).ok);

    // the pistol fires at the targets and hurts them
    let weapons = station(&engine, "station weapons pistol", 30)?;
    let pistol = weapons.first().ok_or("no pistol")?;
    assert!(number(pistol, "shots") > 3.0, "{pistol}");
    assert!(number(pistol, "damage") > 0.0, "{pistol}");

    // the warthog goes round the course
    let vehicles = station(&engine, "station vehicles warthog", 60)?;
    let warthog = vehicles.first().ok_or("no warthog")?;
    assert!(number(warthog, "top_speed") > 1.0, "{warthog}");
    assert!(number(warthog, "corners") >= 1.0, "{warthog}");

    // the biped's standing animations, each shown
    let animations = station(&engine, "station animation stand", 120)?;
    assert!(!animations.is_empty());
    assert!(animations.iter().all(|animation| animation["name"]
        .as_str()
        .is_some_and(|name| name.contains("stand"))));

    // the devices move through their positions
    let doors = station(&engine, "station doors", 30)?;
    assert!(!doors.is_empty());
    assert!(doors.iter().all(|device| number(device, "frames") >= 1.0));

    // the crates come to rest
    let crates = station(&engine, "station physics", 40)?;
    assert_eq!(crates.len(), 12);
    assert!(crates.iter().all(|one| one["resting"] == Value::Bool(true)));

    // the effects make their particles, each counted alone
    let all = run(&engine, "station particles")?;
    eprintln!("{}", all["status"]);
    run(&engine, "station stop")?;
    let effects = station(&engine, "station particles explosion", 120)?;
    eprintln!("{effects:?}");
    assert!(!effects.is_empty());
    assert!(effects
        .iter()
        .any(|effect| number(effect, "particles") > 0.0));

    // a run is stopped
    run(&engine, "station physics")?;
    run(&engine, "station stop")?;
    assert!(run(&engine, "station")?["run"].is_null());
    Ok(())
}
