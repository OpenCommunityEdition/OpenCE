//! The chant's file: made into the cache from the player's executable and
//! taken from there afterwards; nothing without an executable or a tone.

use std::path::Path;

use opence_ffi::loading::{chant_wave, CHANT_FILE};

fn folder(name: &str) -> String {
    let folder = Path::new(env!("CARGO_TARGET_TMPDIR")).join(name);
    let _ = std::fs::remove_dir_all(&folder);
    folder.to_string_lossy().into_owned()
}

#[test]
fn without_an_executable_there_is_no_chant() {
    let cache = folder("chant_none");
    assert_eq!(
        chant_wave("/nonexistent/default.xbe".into(), cache.clone()),
        None
    );
    let fake = Path::new(env!("CARGO_TARGET_TMPDIR")).join("not_an.xbe");
    assert!(std::fs::write(&fake, b"XBEH and nothing else").is_ok());
    assert_eq!(
        chant_wave(fake.to_string_lossy().into_owned(), cache.clone()),
        None
    );
    assert!(!Path::new(&cache).join(CHANT_FILE).exists());
}

#[test]
fn the_chant_is_made_from_the_players_executable_into_the_cache() {
    let xbe = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../Haloce_dump/default.xbe");
    if !xbe.exists() {
        return; // (no executable here)
    }
    let cache = folder("chant_made");
    let made = chant_wave(xbe.to_string_lossy().into_owned(), cache.clone());
    let wave = Path::new(&cache).join(CHANT_FILE);
    assert_eq!(made.as_deref(), Some(&*wave.to_string_lossy()));
    let length = std::fs::metadata(&wave).map(|meta| meta.len()).unwrap_or(0);
    assert_eq!(length, 44 + 2 * 511 * 64);
    // taken from the cache afterwards, without the executable
    assert_eq!(
        chant_wave("/nonexistent/default.xbe".into(), cache).as_deref(),
        Some(&*wave.to_string_lossy())
    );
}
