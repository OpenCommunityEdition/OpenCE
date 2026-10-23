//! The loading screen (source/interface/progress_bar.c): its chant, made
//! from the player's own executable into the cache.

use std::path::Path;

use opence_ui::chant::{decode_xbox_adpcm, find_tone, wave_file, SAMPLE_RATE, TONE_BLOCKS};

/// The chant's file in the cache folder.
pub const CHANT_FILE: &str = "chant.wav";
/// The largest executable read (the retail one is 2.4 MB).
const MAXIMUM_XBE_BYTES: u64 = 64 << 20;

/// The chant as a WAVE file in `cache_folder`, made from the tone in the
/// executable at `xbe_path` the first time; none when there is no
/// executable or no tone in it (the loading screen is then silent).
#[uniffi::export]
pub fn chant_wave(xbe_path: String, cache_folder: String) -> Option<String> {
    let wave = Path::new(&cache_folder).join(CHANT_FILE);
    let samples = TONE_BLOCKS * opence_ui::chant::SAMPLES_PER_BLOCK;
    let expected = 44 + 2 * samples as u64;
    if std::fs::metadata(&wave).is_ok_and(|meta| meta.len() == expected) {
        return Some(wave.to_string_lossy().into_owned());
    }
    if std::fs::metadata(&xbe_path).ok()?.len() > MAXIMUM_XBE_BYTES {
        return None;
    }
    let xbe = std::fs::read(&xbe_path).ok()?;
    let file = wave_file(&decode_xbox_adpcm(find_tone(&xbe)?), SAMPLE_RATE);
    std::fs::create_dir_all(&cache_folder).ok()?;
    // (written whole, then named, so that a cut-off write is not taken)
    let part = wave.with_extension("wav.part");
    std::fs::write(&part, file).ok()?;
    std::fs::rename(&part, &wave).ok()?;
    Some(wave.to_string_lossy().into_owned())
}
