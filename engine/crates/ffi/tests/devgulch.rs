//! The development playground, `devgulch`: made from the player's own
//! bloodgulch.map when it is loaded, never written; each station on flat
//! ground (the parts that need assets/maps pass without it).

use std::path::{Path, PathBuf};

use opence_ffi::{Engine, LevelGeometry};

type Result = std::result::Result<(), Box<dyn std::error::Error>>;

fn folder(name: &str) -> std::io::Result<PathBuf> {
    let folder = Path::new(env!("CARGO_TARGET_TMPDIR")).join(name);
    let _ = std::fs::remove_dir_all(&folder);
    std::fs::create_dir_all(&folder)?;
    Ok(folder)
}

fn text(path: &Path) -> String {
    path.to_string_lossy().into_owned()
}

/// A folder holding links to `names` of assets/maps, and its cache; none
/// where there is no game data.
fn linked(name: &str, names: &[&str]) -> std::io::Result<Option<(PathBuf, String)>> {
    let maps = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../assets/maps");
    if !names.iter().all(|name| maps.join(name).exists()) {
        return Ok(None);
    }
    let folder = folder(name)?;
    for name in names {
        std::os::unix::fs::symlink(maps.join(name).canonicalize()?, folder.join(name))?;
    }
    // (the game's cache: its packs are named by their maps' contents)
    let cache = text(&opence_import::testing::cache_folder());
    Ok(Some((folder, cache)))
}

#[test]
fn devgulch_is_refused_without_bloodgulch() -> Result {
    let empty = folder("devgulch_empty")?;
    let cache = text(&empty.join("cache"));
    let refused = LevelGeometry::load(text(&empty.join("devgulch.map")), cache.clone(), 0);
    let Err(error) = refused else {
        return Err("devgulch loaded without bloodgulch.map".into());
    };
    assert!(error.to_string().contains("bloodgulch.map"), "{error}");
    // (it has one BSP)
    assert!(LevelGeometry::load(text(&empty.join("devgulch.map")), cache, 1).is_err());
    Ok(())
}

#[test]
fn devgulch_is_blood_gulch_with_the_other_maps_definitions() -> Result {
    let Some((folder, cache)) = linked("devgulch_made", &["bloodgulch.map", "wizard.map"])? else {
        return Ok(()); // (no game data here)
    };
    let devgulch = LevelGeometry::load(text(&folder.join("devgulch.map")), cache.clone(), 0)?;
    assert_eq!(devgulch.name(), "devgulch");
    let blood_gulch = LevelGeometry::load(text(&folder.join("bloodgulch.map")), cache, 0)?;
    // Blood Gulch's ground and starts
    assert_eq!(devgulch.bounds(), blood_gulch.bounds());
    let (made, stock) = (devgulch.start_view(), blood_gulch.start_view());
    assert_eq!([made.x, made.y, made.z], [stock.x, stock.y, stock.z]);
    assert_eq!(devgulch.draws().len(), blood_gulch.draws().len());
    // nothing of it is written: the folder has only its links (the packs
    // are in the game's cache)
    let mut written: Vec<_> = std::fs::read_dir(&folder)?
        .filter_map(|entry| entry.ok())
        .map(|entry| entry.file_name().to_string_lossy().into_owned())
        .collect();
    written.sort();
    assert_eq!(written, ["bloodgulch.map", "wizard.map"]);
    // and the player spawns on it
    let engine = Engine::new();
    engine.start_level(devgulch, 1)?;
    engine.spawn_player(0)?;
    engine.advance(1.0 / 30.0)?;
    Ok(())
}

#[test]
fn each_station_of_devgulch_stands_on_flat_open_ground() -> Result {
    let Some((folder, cache)) = linked("devgulch_ground", &["bloodgulch.map"])? else {
        return Ok(()); // (no game data here)
    };
    let loaded = opence_import::load_level(&folder.join("bloodgulch.map"), Path::new(&cache))?;
    for station in &opence_content::devgulch::STATIONS {
        let [x, y] = station.at;
        let here = opence_content::devgulch::ground(&loaded.level, station.at)
            .ok_or_else(|| format!("{}: no ground", station.name))?;
        // the floor, not a hill or a base's roof
        assert!(here < 3.0, "{}: {here}", station.name);
        // flat within 6 units every way
        for [dx, dy] in [[6.0, 0.0], [-6.0, 0.0], [0.0, 6.0], [0.0, -6.0]] {
            let near = opence_content::devgulch::ground(&loaded.level, [x + dx, y + dy])
                .ok_or_else(|| format!("{}: no ground near", station.name))?;
            assert!(
                (near - here).abs() < 1.0,
                "{}: {here} and {near}",
                station.name
            );
        }
    }
    Ok(())
}
