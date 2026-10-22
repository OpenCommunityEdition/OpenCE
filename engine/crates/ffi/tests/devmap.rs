//! The test map, `devmap`: its plane drawn with the grid under its sky,
//! and, made over bloodgulch, the player standing on it; its galleries,
//! made over every map of a folder (the parts that need assets/maps pass
//! without it).

use std::path::Path;

use opence_ffi::render_level::MaterialKind;
use opence_ffi::{Engine, EngineError, LevelGeometry};

type Result = std::result::Result<(), Box<dyn std::error::Error>>;

fn cache() -> std::io::Result<String> {
    let cache = Path::new(env!("CARGO_TARGET_TMPDIR")).join("devmap_cache");
    std::fs::create_dir_all(&cache)?;
    Ok(cache.to_string_lossy().into_owned())
}

#[test]
fn the_plane_is_drawn_with_the_grid_under_a_generated_sky() -> Result {
    let geometry = LevelGeometry::devmap(None, cache()?)?;
    assert_eq!(geometry.name(), "devmap");
    let draws = geometry.draws();
    // (the plane, then the ramps and the slope)
    assert_eq!(draws.len(), 2);
    let material = &geometry.materials()[draws[0].material as usize];
    assert_eq!(material.kind, MaterialKind::Opaque);
    let texture = material.base_map.ok_or("no grid")?;
    assert_eq!(geometry.textures()[texture as usize].width, 64);
    assert!(!geometry.texture_level(texture, 6).is_empty());
    assert_eq!(geometry.bounds(), vec![-80.0, 80.0, -80.0, 80.0, 0.0, 8.0]);
    assert_eq!(draws[0].index_count, 32 * 32 * 6);
    // the sky: no model, its fog the horizon behind the plane
    let sky = geometry.sky().ok_or("no sky")?;
    assert_eq!(sky.model, None);
    assert!(sky.fog.maximum_density > 0.0);
    assert!(sky.fog.end_distance > 80.0);
    // the view: from the player start's eyes, at the middle
    let view = geometry.start_view();
    assert!(view.x.abs() < 1e-4 && view.y.abs() < 1e-4);
    assert!(view.z > 0.0);
    Ok(())
}

#[test]
fn a_devmap_without_a_donor_has_no_player() -> Result {
    let geometry = LevelGeometry::devmap(None, cache()?)?;
    let engine = Engine::new();
    assert_eq!(engine.start_level(geometry, 2)?, 0);
    assert!(engine.spawn_player(0).is_err());
    Ok(())
}

#[test]
fn a_missing_donor_is_refused() -> Result {
    assert!(LevelGeometry::devmap(Some("/nowhere/no.map".into()), cache()?).is_err());
    Ok(())
}

#[test]
fn the_player_stands_on_the_plane() -> Result {
    let map = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../assets/maps/bloodgulch.map");
    if !map.exists() {
        return Ok(()); // (no game data here)
    }
    let geometry = LevelGeometry::devmap(Some(map.to_string_lossy().into_owned()), cache()?)?;
    // (bloodgulch's sky, not the generated one)
    assert!(geometry.sky().ok_or("no sky")?.model.is_some());
    let engine = Engine::new();
    assert_eq!(engine.start_level(geometry, 2)?, 0);
    engine.spawn_player(0)?;
    for _ in 0..60 {
        engine.advance(1.0 / 30.0)?;
    }
    let output = engine.run_command("objects cyborg".into());
    assert!(output.ok, "{}", output.text);
    let player: serde_json::Value = serde_json::from_str(&output.json)?;
    let id = player[0]["id"].as_i64().ok_or("no player")?;
    let inspected = engine.run_command(format!("inspect {id}"));
    let inspected: serde_json::Value = serde_json::from_str(&inspected.json)?;
    let height = inspected["position"][2].as_f64().ok_or("no height")?;
    assert!(height.abs() < 0.1, "stands at {height}");
    Ok(())
}

fn folder(name: &str) -> std::io::Result<std::path::PathBuf> {
    let folder = Path::new(env!("CARGO_TARGET_TMPDIR")).join(name);
    let _ = std::fs::remove_dir_all(&folder);
    std::fs::create_dir_all(&folder)?;
    Ok(folder)
}

#[test]
fn galleries_need_a_map_that_loads() -> Result {
    let folder = folder("devmap_none")?;
    let maps = folder.to_string_lossy().into_owned();
    assert!(LevelGeometry::devmap_gathered(maps.clone(), cache()?).is_err());
    std::fs::write(folder.join("junk.map"), b"not a map")?;
    std::fs::write(folder.join("notes.txt"), b"not a map either")?;
    assert!(LevelGeometry::devmap_gathered(maps, cache()?).is_err());
    assert!(LevelGeometry::devmap_gathered("/nowhere/at/all".into(), cache()?).is_err());
    // (load sends a missing devmap.map to them, with its one structure)
    let named = folder.join("devmap.map").to_string_lossy().into_owned();
    assert!(matches!(
        LevelGeometry::load(named.clone(), cache()?, 1),
        Err(EngineError::NoBsp { index: 1 })
    ));
    assert!(matches!(
        LevelGeometry::load(named, cache()?, 0),
        Err(EngineError::Load { .. })
    ));
    Ok(())
}

#[test]
fn the_galleries_hold_every_maps_objects() -> Result {
    let maps = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../assets/maps");
    let names = ["bloodgulch.map", "wizard.map"];
    if !names.iter().all(|name| maps.join(name).exists()) {
        return Ok(()); // (no game data here)
    }
    let folder = folder("devmap_galleries")?;
    for name in names {
        std::os::unix::fs::symlink(maps.join(name).canonicalize()?, folder.join(name))?;
    }
    std::fs::write(folder.join("junk.map"), b"left out")?;
    let gathered = folder.join("devmap.map").to_string_lossy().into_owned();
    let geometry = LevelGeometry::load(gathered, cache()?, 0)?;
    assert_eq!(geometry.name(), "devmap");
    // (a plane larger than the bare one, to hold them)
    let bounds = geometry.bounds();
    assert!(bounds[1] >= 80.0, "{bounds:?}");
    let engine = Engine::new();
    let spawned = engine.start_level(geometry, 1)?;
    assert!(spawned > 50, "{spawned} spawned");
    engine.spawn_player(0)?;
    engine.advance(1.0 / 30.0)?;
    // the warthog stands in the vehicles' gallery, named by its tag
    let output = engine.run_command("objects warthog".into());
    assert!(output.ok, "{}", output.text);
    let found: serde_json::Value = serde_json::from_str(&output.json)?;
    assert!(!found.as_array().ok_or("no list")?.is_empty());
    Ok(())
}
