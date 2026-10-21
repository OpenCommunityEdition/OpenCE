//! A cache file of either kind, imported or refused.
#![no_main]

use libfuzzer_sys::fuzz_target;
use opence_import::{import_level, Cache};

fuzz_target!(|file: &[u8]| {
    if let Ok(cache) = Cache::open(file.to_vec()) {
        let _ = import_level(&cache);
    }
});
