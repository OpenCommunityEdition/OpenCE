//! A level's sound played through the engine is heard at its audio output,
//! and the output is silent before a level starts. The parts that need
//! assets/maps/bloodgulch.map pass without it.

use std::error::Error;

use opence_ffi::{Engine, LevelGeometry};
use opence_import::testing::{cache_folder, maps_folder, stock_level};

type Result = std::result::Result<(), Box<dyn Error>>;

#[test]
fn a_level_sound_reaches_the_output() -> Result {
    let engine = Engine::new();
    let output = engine.audio_output();
    assert!(!engine.play_sound("anything".into(), 1.0));
    assert!(engine.audio_census().is_empty());
    assert!(output.pull(128).iter().all(|sample| *sample == 0.0));

    let Some(stock) = stock_level("bloodgulch")? else {
        return Ok(()); // (no game data here)
    };
    let level = stock;
    // (the game's own cache folder, where the map's pack is)
    let geometry = LevelGeometry::load(
        maps_folder()
            .join("bloodgulch.map")
            .to_string_lossy()
            .into_owned(),
        cache_folder().to_string_lossy().into_owned(),
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
    // (the audio view counts it by its class)
    let class = opence_audio::class(i32::from(sound.class)).name;
    let census = engine.audio_census();
    let counted = census
        .iter()
        .find(|count| count.class == class)
        .ok_or("not counted")?;
    assert!(counted.playing >= 1, "{census:?}");
    let mut heard = false;
    for _ in 0..40 {
        heard |= output.pull(1024).iter().any(|sample| *sample != 0.0);
    }
    assert!(heard, "{} was not heard", sound.tag.path);
    engine.start_empty_world();
    assert!(output.pull(128).iter().all(|sample| *sample == 0.0));
    Ok(())
}
