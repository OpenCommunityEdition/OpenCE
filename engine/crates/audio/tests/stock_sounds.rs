//! Every sound of every stock map imports and decodes: each permutation's
//! samples are whole blocks of the sound's channels, and decode to sound
//! (skipped where there is no game data).

use std::error::Error;
use std::path::Path;

use opence_audio::decode;
use opence_content::SoundCompression;
use opence_import::testing::stock_level_unheld;

#[test]
fn every_stock_sound_decodes() -> Result<(), Box<dyn Error>> {
    let maps = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../assets/maps");
    let Ok(entries) = std::fs::read_dir(&maps) else {
        return Ok(());
    };
    let mut names: Vec<String> = entries
        .filter_map(Result::ok)
        .filter_map(|entry| entry.file_name().into_string().ok())
        .filter_map(|name| name.strip_suffix(".map").map(String::from))
        .filter(|name| name != "ui")
        .collect();
    names.sort();
    let mut failures = Vec::new();
    for name in &names {
        let level = stock_level_unheld(name)?.ok_or_else(|| format!("{name}: no map"))?;
        assert!(!level.sounds.is_empty(), "{name}: no sounds");
        let mut permutations = 0usize;
        for sound in &level.sounds {
            for range in &sound.pitch_ranges {
                for permutation in &range.permutations {
                    permutations += 1;
                    let what = format!("{name}: {} {}", sound.tag.path, permutation.name);
                    if permutation.compression != SoundCompression::XboxAdpcm {
                        failures.push(format!("{what}: {:?}", permutation.compression));
                        continue;
                    }
                    let block = 36 * usize::from(sound.channels);
                    if permutation.samples.len() % block != 0 {
                        failures.push(format!("{what}: {} bytes", permutation.samples.len()));
                    }
                    match decode(
                        permutation.compression,
                        sound.sample_rate,
                        sound.channels,
                        &permutation.samples,
                    ) {
                        Ok(decoded) => {
                            if decoded.frames() != permutation.samples.len() / block * 64 {
                                failures.push(format!("{what}: {} frames", decoded.frames()));
                            }
                        }
                        Err(error) => failures.push(format!("{what}: {error}")),
                    }
                }
            }
        }
        assert!(permutations > 0, "{name}: no permutations");
        eprintln!(
            "{name}: {} sounds, {permutations} permutations",
            level.sounds.len()
        );
    }
    assert!(
        failures.is_empty(),
        "{} failures, the first {:?}",
        failures.len(),
        failures.iter().take(10).collect::<Vec<_>>()
    );
    Ok(())
}
