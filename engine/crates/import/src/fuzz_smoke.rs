//! The importer fuzzed for a fixed time in the suite: the fake levels and
//! their pack, mutated, must each be imported or refused, never panic.
//! The same entry points are the `cargo fuzz` targets (engine/fuzz), for
//! longer runs with coverage.

use std::time::{Duration, Instant};

use opence_content::pack::{self, PackHeader};

use crate::cache::{CacheKind, HEADER_SIZE};
use crate::fake::FakeCache;
use crate::level::tests::{level, Breaks};
use crate::ui::tests::{add_font, add_shell};
use crate::{import_level, Cache};

/// xorshift64*: a fixed seed, so that a failure is found again.
struct Random(u64);

impl Random {
    fn next(&mut self) -> u64 {
        self.0 ^= self.0 >> 12;
        self.0 ^= self.0 << 25;
        self.0 ^= self.0 >> 27;
        self.0.wrapping_mul(0x2545_F491_4F6C_DD1D)
    }

    fn below(&mut self, limit: usize) -> usize {
        (self.next() % limit.max(1) as u64) as usize
    }
}

/// Values a reader is likeliest to mishandle, as counts, offsets and
/// addresses.
const INTERESTING: [u32; 10] = [
    0,
    1,
    0x7F,
    0xFFFF,
    0x7FFF_FFFF,
    0x8000_0000,
    0xFFFF_FFFF,
    0x4000_0000,
    0x803A_6000,
    0x4044_0000,
];

/// Changes one to four things about `bytes`, at `from` or after.
fn mutate(random: &mut Random, bytes: &mut Vec<u8>, from: usize) {
    for _ in 0..1 + random.below(4) {
        let span = bytes.len().saturating_sub(from);
        if span == 0 {
            return;
        }
        let at = from + random.below(span);
        match random.below(6) {
            0 => bytes[at] ^= 1 << random.below(8),
            1 => bytes[at] = random.next() as u8,
            2 | 3 => {
                let at = (at & !3).max(from);
                let value = INTERESTING[random.below(INTERESTING.len())];
                let end = (at + 4).min(bytes.len());
                bytes[at..end].copy_from_slice(&value.to_le_bytes()[..end - at]);
            }
            4 => bytes.truncate(at),
            _ => {
                let length = random.below(64).min(bytes.len() - at);
                let chunk = bytes[at..at + length].to_vec();
                let to = from + random.below(span);
                bytes.splice(to..to, chunk);
            }
        }
    }
}

/// The fuzz targets' entry: a cache file, imported or refused.
pub(crate) fn import_file(file: &[u8]) {
    if let Ok(cache) = Cache::open(file.to_vec()) {
        let _ = import_level(&cache);
    }
}

/// The UI fuzz target's entry: a cache file's UI tags, read or refused.
pub(crate) fn import_ui_file(file: &[u8]) {
    if let Ok(cache) = Cache::open(file.to_vec()) {
        let _ = crate::ui::import_ui_tags(&cache);
    }
}

/// An Xbox cache from its header and its inflated body (so that the
/// mutations reach the tags, not just the stream).
pub(crate) fn xbox_file(header: &[u8], body: &[u8]) -> Vec<u8> {
    let mut file = header.to_vec();
    let length = (HEADER_SIZE + body.len()) as u32;
    file[8..12].copy_from_slice(&length.to_le_bytes());
    file.extend_from_slice(&miniz_oxide::deflate::compress_to_vec_zlib(body, 1));
    file
}

fn budget() -> Duration {
    let seconds = std::env::var("OPENCE_FUZZ_SECONDS")
        .ok()
        .and_then(|seconds| seconds.parse().ok())
        .unwrap_or(2.0);
    Duration::from_secs_f64(seconds)
}

/// Runs `case` until the budget is spent, and says how many cases ran.
fn fuzz(seed: u64, mut case: impl FnMut(&mut Random)) -> usize {
    let mut random = Random(seed);
    let started = Instant::now();
    let mut cases = 0;
    while cases < 64 || started.elapsed() < budget() {
        case(&mut random);
        cases += 1;
    }
    cases
}

#[test]
fn mutated_custom_edition_caches_are_imported_or_refused() {
    let file = level(CacheKind::CustomEdition, &Breaks::default()).build();
    let cases = fuzz(0x0CE0_0001, |random| {
        let mut mutated = file.clone();
        // (mostly the tags, sometimes the header)
        let from = if random.below(8) == 0 { 0 } else { HEADER_SIZE };
        mutate(random, &mut mutated, from);
        import_file(&mutated);
    });
    println!("{cases} Custom Edition caches");
}

#[test]
fn mutated_xbox_caches_are_imported_or_refused() {
    let file = level(CacheKind::Xbox, &Breaks::default()).build();
    let cache = match Cache::open(file.clone()) {
        Ok(cache) => cache,
        Err(error) => panic!("{error}"),
    };
    let body = cache.file()[HEADER_SIZE..].to_vec();
    let header = &file[..HEADER_SIZE];
    let cases = fuzz(0x0B0B_0002, |random| {
        let mut mutated = body.clone();
        mutate(random, &mut mutated, 0);
        import_file(&xbox_file(header, &mutated));
        // and the stream itself, damaged
        if random.below(16) == 0 {
            let mut stream = file.clone();
            mutate(random, &mut stream, HEADER_SIZE);
            import_file(&stream);
        }
    });
    println!("{cases} Xbox caches");
}

#[test]
fn mutated_packs_are_read_or_refused() {
    let cache = match Cache::open(level(CacheKind::Xbox, &Breaks::default()).build()) {
        Ok(cache) => cache,
        Err(error) => panic!("{error}"),
    };
    let level = match import_level(&cache) {
        Ok(level) => level,
        Err(error) => panic!("{error}"),
    };
    let header = PackHeader {
        importer_version: 1,
        source: 2,
    };
    let packed = match pack::write_pack(&level, header) {
        Ok(packed) => packed,
        Err(error) => panic!("{error}"),
    };
    let cases = fuzz(0x0C0C_0003, |random| {
        let mut mutated = packed.clone();
        mutate(random, &mut mutated, 0);
        // a pack whose fingerprint is mended, so that bytecheck is reached
        if random.below(2) == 0 && mutated.len() >= 0x28 {
            let archive = pack::fingerprint(&mutated[0x28..]);
            let length = (mutated.len() - 0x28) as u64;
            mutated[0x18..0x20].copy_from_slice(&length.to_le_bytes());
            mutated[0x20..0x28].copy_from_slice(&archive.to_le_bytes());
        }
        let _ = pack::read_pack(&mutated);
    });
    println!("{cases} packs");
}

#[test]
fn mutated_ui_tags_are_read_or_refused() {
    let mut fake = FakeCache::new(CacheKind::CustomEdition);
    add_shell(&mut fake);
    add_font(&mut fake, false);
    let file = fake.build();
    let cases = fuzz(0x0C0C_0004, |random| {
        let mut mutated = file.clone();
        mutate(random, &mut mutated, HEADER_SIZE);
        import_ui_file(&mutated);
    });
    println!("{cases} UI caches");
}
