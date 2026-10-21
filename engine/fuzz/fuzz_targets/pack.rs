//! A pack, read or refused.
#![no_main]

use libfuzzer_sys::fuzz_target;

fuzz_target!(|pack: &[u8]| {
    let _ = opence_content::pack::read_pack(pack);
});
