//! The test map, `devmap`: its plane drawn with the grid under its sky,
//! and, made over bloodgulch, the player standing on it (that part passes
//! without assets/maps/bloodgulch.map).

use std::path::Path;

use opence_ffi::render_level::MaterialKind;
use opence_ffi::{Engine, LevelGeometry};

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
    assert_eq!(draws.len(), 1);
    let material = &geometry.materials()[draws[0].material as usize];
    assert_eq!(material.kind, MaterialKind::Opaque);
    let texture = material.base_map.ok_or("no grid")?;
    assert_eq!(geometry.textures()[texture as usize].width, 64);
    assert!(!geometry.texture_level(texture, 6).is_empty());
    assert_eq!(geometry.bounds(), vec![-80.0, 80.0, -80.0, 80.0, 0.0, 0.0]);
    let indices = geometry.indices().len() / 4;
    assert_eq!(indices, 32 * 32 * 6);
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
