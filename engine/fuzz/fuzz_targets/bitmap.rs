//! Each of a cache file's bitmaps, its images and their pixels, read or
//! refused.
#![no_main]

use libfuzzer_sys::fuzz_target;
use opence_import::{bitmap::import_bitmap, Cache, GroupTag};

fuzz_target!(|file: &[u8]| {
    if let Ok(cache) = Cache::open(file.to_vec()) {
        for (index, instance) in cache.tags().iter().enumerate() {
            if instance.group == GroupTag::new(b"bitm") && !instance.in_resource_map {
                let _ = import_bitmap(&cache, index);
            }
        }
    }
});
