//! A frame of devgulch's stations, through the calls the app makes each
//! frame (`cargo bench -p opence_ffi`; nothing without assets/maps). It is
//! the release build's measure: LTO and PGO were measured on it, and
//! declined (the rust_tools report).

use std::path::Path;
use std::sync::Arc;

use divan::Bencher;
use opence_ffi::{Engine, LevelGeometry};

type Result<T = ()> = std::result::Result<T, Box<dyn std::error::Error>>;

fn main() {
    divan::main();
}

/// An engine on devgulch with `station` running and its first second
/// played; none where there is no game data.
fn devgulch(station: &str) -> Result<Option<Arc<Engine>>> {
    let maps = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../assets/maps");
    let names = ["bloodgulch.map", "wizard.map"];
    if !names.iter().all(|name| maps.join(name).exists()) {
        return Ok(None);
    }
    let folder = Path::new(env!("CARGO_TARGET_TMPDIR")).join(format!("frame_{station}"));
    let _ = std::fs::remove_dir_all(&folder);
    std::fs::create_dir_all(&folder)?;
    for name in names {
        std::os::unix::fs::symlink(maps.join(name).canonicalize()?, folder.join(name))?;
    }
    let cache = opence_import::testing::cache_folder();
    let geometry = LevelGeometry::load(
        folder.join("devgulch.map").to_string_lossy().into_owned(),
        cache.to_string_lossy().into_owned(),
        0,
    )?;
    let engine = Engine::new();
    engine.start_level(geometry, 1)?;
    engine.spawn_player(0)?;
    let output = engine.run_command(format!("station {station}"));
    if !output.ok {
        return Err(output.text.into());
    }
    for _ in 0..60 {
        engine.advance(1.0 / 60.0)?;
    }
    Ok(Some(engine))
}

#[divan::bench(args = ["ai", "vehicles warthog", "physics", "weapons pistol"], sample_count = 50, sample_size = 10)]
fn a_frame_of_a_station(bencher: Bencher, station: &str) {
    let engine = match devgulch(station) {
        Ok(Some(engine)) => engine,
        Ok(None) => return,
        Err(error) => panic!("devgulch's {station} station: {error}"),
    };
    bencher.bench_local(|| engine.advance(1.0 / 60.0));
}
