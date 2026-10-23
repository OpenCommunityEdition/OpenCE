//! A level's sound played through the engine is heard at its audio output,
//! and the output is silent before a level starts. The parts that need
//! assets/maps/bloodgulch.map pass without it.

use std::error::Error;
use std::path::Path;

use opence_ffi::{Engine, LevelGeometry};
use opence_import::{import_level, Cache};

type Result = std::result::Result<(), Box<dyn Error>>;

#[test]
fn a_level_sound_reaches_the_output() -> Result {
    let engine = Engine::new();
    let output = engine.audio_output();
    assert!(!engine.play_sound("anything".into(), 1.0));
    assert!(output.pull(128).iter().all(|sample| *sample == 0.0));

    let map = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../assets/maps/bloodgulch.map");
    let Ok(file) = std::fs::read(&map) else {
        return Ok(()); // (no game data here)
    };
    let level = import_level(&Cache::open(file)?)?;
    let cache = Path::new(env!("CARGO_TARGET_TMPDIR")).join("sounds_cache");
    std::fs::create_dir_all(&cache)?;
    let geometry = LevelGeometry::load(
        map.to_string_lossy().into_owned(),
        cache.to_string_lossy().into_owned(),
        0,
    )?;
    engine.start_level(geometry, 1)?;
    let sound = level
        .sounds
        .iter()
        .find(|sound| {
            sound.zero.gain > 0.0
                && sound.skip_fraction == 0.0
                && sound.promotion_count == 0
                && sound.class != 32
                && sound.pitch_ranges.iter().all(|range| {
                    !range.permutations.is_empty()
                        && range.permutations.iter().all(|p| p.skip_fraction == 0.0)
                })
        })
        .ok_or("no plain sound")?;
    assert!(
        engine.play_sound(sound.tag.path.clone(), 1.0),
        "{}",
        sound.tag.path
    );
    let mut heard = false;
    for _ in 0..40 {
        heard |= output.pull(1024).iter().any(|sample| *sample != 0.0);
    }
    assert!(heard, "{} was not heard", sound.tag.path);
    engine.start_empty_world();
    assert!(output.pull(128).iter().all(|sample| *sample == 0.0));
    Ok(())
}
