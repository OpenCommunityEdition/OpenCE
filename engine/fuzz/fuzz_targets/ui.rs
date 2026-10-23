//! A cache file's UI tags (widgets, string lists, fonts and the rest),
//! read or refused.
#![no_main]

use libfuzzer_sys::fuzz_target;
use opence_import::{ui::import_ui_tags, Cache};

fuzz_target!(|file: &[u8]| {
    if let Ok(cache) = Cache::open(file.to_vec()) {
        let _ = import_ui_tags(&cache);
    }
});
