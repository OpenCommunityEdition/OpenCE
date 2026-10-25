//! An Xbox cache's sounds and looping sounds, from its header and its
//! inflated body (deflated here, as xbox_cache does), read or refused.
#![no_main]

use libfuzzer_sys::fuzz_target;
use opence_import::cache::HEADER_SIZE;
use opence_import::{sound::import_sounds, Cache};

fuzz_target!(|input: &[u8]| {
    if input.len() < HEADER_SIZE {
        return;
    }
    let (header, body) = input.split_at(HEADER_SIZE);
    let mut file = header.to_vec();
    file[8..12].copy_from_slice(&((HEADER_SIZE + body.len()) as u32).to_le_bytes());
    file.extend_from_slice(&miniz_oxide::deflate::compress_to_vec_zlib(body, 1));
    if let Ok(cache) = Cache::open(file) {
        let _ = import_sounds(&cache);
    }
});
