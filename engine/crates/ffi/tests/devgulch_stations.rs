//! devgulch's stations, each run on the copy of Blood Gulch made from the
//! player's bloodgulch.map with wizard's definitions: where they stand the
//! player and put their set, and what they measured (they pass without
//! assets/maps).

use std::path::{Path, PathBuf};
use std::sync::Arc;

use opence_content::devgulch;
use opence_ffi::{Engine, LevelGeometry};
use serde_json::Value;

type Result<T = ()> = std::result::Result<T, Box<dyn std::error::Error>>;

fn folder(name: &str) -> std::io::Result<PathBuf> {
    let folder = Path::new(env!("CARGO_TARGET_TMPDIR")).join(name);
    let _ = std::fs::remove_dir_all(&folder);
    std::fs::create_dir_all(&folder)?;
    Ok(folder)
}

fn text(path: &Path) -> String {
    path.to_string_lossy().into_owned()
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
            let results = shown["run"]["results"].as_array().ok_or("no results")?;
            return Ok(results.clone());
        }
    }
    Err(format!("{line} did not finish in {seconds} s").into())
}

fn number(value: &Value, name: &str) -> f64 {
    value[name].as_f64().unwrap_or(f64::NAN)
}

/// Each biped's place.
fn bipeds(engine: &Engine) -> Result<Vec<[f64; 3]>> {
    let rows = run(engine, "objects biped")?;
    let rows = rows.as_array().ok_or("no objects")?;
    Ok(rows
        .iter()
        .filter_map(|row| {
            let at = row["position"].as_array()?;
            Some([at.first()?, at.get(1)?, at.get(2)?].map(|v| v.as_f64().unwrap_or(f64::NAN)))
        })
        .collect())
}

/// An engine on devgulch made over bloodgulch and wizard, its player
/// spawned; none where there is no game data.
fn devgulch(name: &str) -> Result<Option<Arc<Engine>>> {
    let maps = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../assets/maps");
    let names = ["bloodgulch.map", "wizard.map"];
    if !names.iter().all(|name| maps.join(name).exists()) {
        return Ok(None);
    }
    let folder = folder(name)?;
    for name in names {
        std::os::unix::fs::symlink(maps.join(name).canonicalize()?, folder.join(name))?;
    }
    let cache = text(&folder.join("cache"));
    let geometry = LevelGeometry::load(text(&folder.join("devgulch.map")), cache, 0)?;
    let engine = Engine::new();
    engine.start_level(geometry, 1)?;
    engine.spawn_player(0)?;
    engine.advance(1.0 / 30.0)?;
    Ok(Some(engine))
}

#[test]
fn each_station_stands_the_player_at_it_and_puts_its_set_on_the_floor() -> Result {
    let Some(engine) = devgulch("devgulch_stations_each")? else {
        return Ok(()); // (no game data here)
    };
    let listed = run(&engine, "station")?;
    let names: Vec<&str> = listed["stations"]
        .as_array()
        .ok_or("no stations")?
        .iter()
        .filter_map(Value::as_str)
        .collect();
    let all: Vec<&str> = devgulch::STATIONS.iter().map(|one| one.name).collect();
    assert_eq!(names, all);
    // (the vehicles station seats the player, and stopping it leaves them
    // seated, which the others refuse: it is started last)
    let mut order: Vec<_> = devgulch::STATIONS.iter().collect();
    order.sort_by_key(|one| one.name == "vehicles");
    for one in order {
        run(&engine, &format!("station {}", one.name))?;
        engine.advance(1.0 / 30.0)?;
        let set = run(&engine, "station")?["run"]["set"].clone();
        assert!(
            set.as_array().is_some_and(|set| !set.is_empty()),
            "{}: {set}",
            one.name
        );
        // the player stands at the station, and what it put stands on the floor
        let [x, y] = one.at.map(f64::from);
        let placed = bipeds(&engine)?;
        assert!(
            placed.iter().any(|at| (at[0] - x).hypot(at[1] - y) < 1.5),
            "{}: {placed:?}",
            one.name
        );
        assert!(
            placed.iter().all(|at| at[2] < 5.0),
            "{}: {placed:?}",
            one.name
        );
        run(&engine, "station stop")?;
    }
    Ok(())
}

#[test]
fn the_stations_measure_on_devgulch() -> Result {
    let Some(engine) = devgulch("devgulch_stations_measure")? else {
        return Ok(()); // (no game data here)
    };
    // the pistol fires at the targets and hurts them
    let weapons = station(&engine, "station weapons pistol", 30)?;
    let pistol = weapons.first().ok_or("no pistol")?;
    assert!(number(pistol, "shots") > 3.0, "{pistol}");
    assert!(number(pistol, "damage") > 0.0, "{pistol}");

    // the warthog goes round the course
    let vehicles = station(&engine, "station vehicles warthog", 90)?;
    let warthog = vehicles.first().ok_or("no warthog")?;
    assert!(number(warthog, "corners") >= 1.0, "{warthog}");

    // the crates come to rest on the ground
    let crates = station(&engine, "station physics", 40)?;
    assert!(!crates.is_empty());
    assert!(crates.iter().all(|one| one["resting"] == Value::Bool(true)));

    // a squad of each other biped, every sound scenery, a shader of each class
    let squads = station(&engine, "station ai", 5)?;
    assert!(squads.iter().all(|one| number(one, "squad") >= 1.0));
    assert!(!squads.is_empty());
    assert!(!station(&engine, "station sounds", 5)?.is_empty());
    let shaders = station(&engine, "station shaders", 5)?;
    assert!(shaders.len() >= 2, "{shaders:?}");

    // each weapon's HUD is shown
    let huds = station(&engine, "station hud pistol", 10)?;
    let hud = huds.first().ok_or("no pistol HUD")?;
    assert!(hud["error"].is_null(), "{hud}");
    Ok(())
}

#[test]
fn the_sound_station_voices_a_sound_of_each_class() -> Result {
    let Some(engine) = devgulch("devgulch_stations_sounds")? else {
        return Ok(()); // (no game data here)
    };
    let results = station(&engine, "station sounds", 10)?;
    let emitters: Vec<&Value> = results
        .iter()
        .filter(|one| one["class"].is_string())
        .collect();
    let scenery: Vec<&Value> = results
        .iter()
        .filter(|one| one["sound_scenery"].is_string())
        .collect();
    let unplayed: Vec<&Value> = results
        .iter()
        .filter_map(|one| one.get("unplayed_class"))
        .collect();
    println!(
        "{} classes voiced ({} by a sound that may be skipped), {} scenery heard, classes with nothing to play: {unplayed:?}",
        emitters.len(),
        emitters
            .iter()
            .filter(|one| one["plain"] == Value::Bool(false))
            .count(),
        scenery.len()
    );
    assert!(unplayed.is_empty(), "{unplayed:?}");
    assert!(emitters.len() >= 20, "{results:?}");
    assert!(!scenery.is_empty(), "{results:?}");
    // each class's sound played on a voice, counted under its class; none
    // failed
    let unvoiced: Vec<_> = emitters
        .iter()
        .filter(|one| one["played"] != Value::Bool(true) || one["counted"] != Value::Bool(true))
        .collect();
    assert!(unvoiced.is_empty(), "{unvoiced:#?}");
    // each sound scenery placed within hearing has a voice where it stands
    let unheard: Vec<_> = scenery
        .iter()
        .filter(|one| one["heard"] != Value::Bool(true))
        .collect();
    assert!(unheard.is_empty(), "{unheard:#?}");
    let census = engine.audio_census();
    assert!(census.iter().all(|count| count.failed == 0), "{census:?}");
    Ok(())
}

#[test]
fn each_effect_of_the_row_spawns_its_full_particle_set() -> Result {
    let Some(engine) = devgulch("devgulch_stations_effects")? else {
        return Ok(()); // (no game data here)
    };
    let results = station(&engine, "station effects", 900)?;
    for one in &results {
        println!(
            "{} peak {} planned {} made {} emitters {} systems {}/{} missing {}",
            one["effect"],
            one["particles"],
            one["planned"],
            one["made"],
            one["emitters expected"],
            one["systems"],
            one["systems expected"],
            one["missing"]
        );
    }
    assert!(!results.is_empty());
    let number = |one: &Value, key: &str| one[key].as_u64().unwrap_or(0);
    for one in &results {
        let effect = &one["effect"];
        assert_eq!(
            number(one, "made"),
            number(one, "planned"),
            "{effect} made other than it planned"
        );
        assert_eq!(
            one["missing"],
            serde_json::json!([]),
            "{effect} lacks particles"
        );
        assert!(
            number(one, "systems") >= number(one, "systems expected"),
            "{effect} began too few particle systems"
        );
        // (an emitter that surely emits plans its particles, even in an
        // event that lasts no time)
        if number(one, "emitters expected") > 0 {
            assert!(number(one, "planned") > 0, "{effect} planned no particles");
        }
        if number(one, "planned") > 0 || number(one, "systems expected") > 0 {
            assert!(number(one, "particles") > 0, "{effect} had none alive");
        }
    }
    Ok(())
}
