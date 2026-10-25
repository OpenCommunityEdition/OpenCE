//! Properties of the readers that game files go through: the
//! bounds-checked reads give the bytes they name or a fault, never more or
//! a panic, and a pack gives back the level written to it, or refuses any
//! damage to it. (`fuzz_smoke` mutates whole files; these state what a
//! read must return.)

use opence_content::pack::{read_pack, write_pack, PackError, PackHeader};
use opence_content::{Group, Level, TagRef};
use proptest::prelude::*;
use proptest::test_runner::{Config, RngSeed};

use crate::cache::CacheKind;
use crate::level::tests::{level, Breaks};
use crate::reader::{self, Fault};
use crate::{import_level, Cache};

/// A fixed seed, so that each run tries the same cases.
fn config(cases: u32) -> Config {
    Config {
        cases,
        rng_seed: RngSeed::Fixed(0x0CE0_0CE0),
        failure_persistence: None,
        ..Config::default()
    }
}

fn test_level(kind: CacheKind) -> Level {
    Cache::open(level(kind, &Breaks::default()).build())
        .and_then(|cache| import_level(&cache))
        .unwrap_or_else(|error| panic!("{error}"))
}

proptest! {
    #![proptest_config(config(512))]

    #[test]
    fn a_slice_is_the_bytes_it_names_or_out_of_range(
        bytes in prop::collection::vec(any::<u8>(), 0..64),
        at in 0usize..96,
        size in 0usize..96,
    ) {
        match reader::slice(&bytes, at, size) {
            Ok(slice) => prop_assert_eq!(slice, &bytes[at..at + size]),
            Err(fault) => {
                prop_assert!(at + size > bytes.len());
                prop_assert_eq!(fault, Fault::OutOfRange {
                    at: at as u64,
                    size: size as u64,
                    length: bytes.len() as u64,
                });
            }
        }
    }

    #[test]
    fn a_slice_past_the_end_of_the_address_space_is_out_of_range(
        bytes in prop::collection::vec(any::<u8>(), 0..16),
        size in 1usize..16,
    ) {
        prop_assert!(reader::slice(&bytes, usize::MAX, size).is_err());
        prop_assert!(reader::u32(&bytes, usize::MAX - 1).is_err());
    }

    #[test]
    fn a_word_is_its_little_endian_bytes(
        bytes in prop::collection::vec(any::<u8>(), 0..16),
        at in 0usize..20,
    ) {
        let expected = bytes
            .get(at..at + 4)
            .map(|word| u32::from_le_bytes([word[0], word[1], word[2], word[3]]));
        prop_assert_eq!(reader::u32(&bytes, at).ok(), expected);
        prop_assert_eq!(reader::i32(&bytes, at).ok(), expected.map(|word| word as i32));
        prop_assert_eq!(
            reader::f32(&bytes, at).ok().map(f32::to_bits),
            expected
        );
    }

    #[test]
    fn text_is_printable_and_ends_at_its_terminator_within_its_longest(
        bytes in prop::collection::vec(any::<u8>(), 0..48),
        at in 0usize..56,
        longest in 0usize..48,
    ) {
        if let Ok(text) = reader::text(&bytes, at, longest) {
            prop_assert!(text.len() <= longest);
            prop_assert!(text.bytes().all(|byte| (0x20..0x7f).contains(&byte)));
            prop_assert_eq!(&bytes[at..at + text.len()], text.as_bytes());
            prop_assert_eq!(bytes[at + text.len()], 0);
        }
    }
}

proptest! {
    #![proptest_config(config(64))]

    #[test]
    fn a_pack_gives_back_the_level_and_header_written_to_it(
        xbox in any::<bool>(),
        importer_version in any::<u32>(),
        source in any::<u64>(),
        paths in prop::collection::vec(("[a-z]{4}", ".{0,40}"), 0..8),
    ) {
        let mut level = test_level(if xbox { CacheKind::Xbox } else { CacheKind::CustomEdition });
        // (content of any text, to round-trip with the importer's own)
        level.resource_bitmaps.extend(paths.into_iter().map(|(group, path)| TagRef {
            group: Group(group.as_bytes().try_into().unwrap_or(*b"bitm")),
            path,
        }));
        let header = PackHeader { importer_version, source };
        let pack = write_pack(&level, header).map_err(|error| TestCaseError::fail(error.to_string()))?;
        let (read_header, read_level) =
            read_pack(&pack).map_err(|error| TestCaseError::fail(error.to_string()))?;
        prop_assert_eq!(read_header, header);
        prop_assert_eq!(read_level, level);
    }

    #[test]
    fn a_pack_with_any_byte_of_its_archive_changed_is_refused(
        xbox in any::<bool>(),
        at in any::<prop::sample::Index>(),
        flip in 1u8..=255,
    ) {
        let level = test_level(if xbox { CacheKind::Xbox } else { CacheKind::CustomEdition });
        let header = PackHeader { importer_version: 1, source: 2 };
        let mut pack = write_pack(&level, header).map_err(|error| TestCaseError::fail(error.to_string()))?;
        // (the archive starts after the header's 0x28 bytes)
        let at = 0x28 + at.index(pack.len() - 0x28);
        pack[at] ^= flip;
        prop_assert_eq!(read_pack(&pack).err(), Some(PackError::Fingerprint));
    }

    #[test]
    fn a_shortened_or_lengthened_pack_is_refused(
        at in any::<prop::sample::Index>(),
        extra in prop::collection::vec(any::<u8>(), 1..16),
        lengthen in any::<bool>(),
    ) {
        let level = test_level(CacheKind::Xbox);
        let mut pack = write_pack(&level, PackHeader { importer_version: 1, source: 2 })
            .map_err(|error| TestCaseError::fail(error.to_string()))?;
        if lengthen {
            pack.extend_from_slice(&extra);
        } else {
            pack.truncate(at.index(pack.len()));
        }
        prop_assert!(read_pack(&pack).is_err());
    }
}
