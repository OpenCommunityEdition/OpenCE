//! The cache file: its header, its decompression, and where its tag data
//! is.
//!
//! The layouts are the decompilation's (`source/cache/cache_files.h`) and
//! those `docs/custom_edition_caches.md` sets out for Custom Edition.

use crate::error::ImportError;
use crate::reader::{self, Fault};
use crate::tags::{GroupTag, TagIndex};

/// The header's size; the Xbox's caches are compressed after it.
pub const HEADER_SIZE: usize = 0x800;
/// The longest cache read (Halo PC's limit).
pub const MAX_FILE_LENGTH: u32 = 0x3000_0000;
/// The longest name and build string, terminator excluded.
const HEADER_STRING_LONGEST: usize = 31;

const VERSION_OFFSET: usize = 0x04;
const FILE_LENGTH_OFFSET: usize = 0x08;
const COMPRESSED_LENGTH_OFFSET: usize = 0x0C;
const TAG_DATA_OFFSET_OFFSET: usize = 0x10;
const TAG_DATA_SIZE_OFFSET: usize = 0x14;
const NAME_OFFSET: usize = 0x20;
const BUILD_OFFSET: usize = 0x40;
const SCENARIO_TYPE_OFFSET: usize = 0x60;
const CHECKSUM_OFFSET: usize = 0x64;
const FOOTER_OFFSET: usize = 0x7FC;
/// OpenSauce's header, in Custom Edition's padding: its signature, then
/// its flags at 6. A cache that sets none of them runs as stock.
const OPENSAUCE_OFFSET: usize = 0x70;
const OPENSAUCE_FLAGS_OFFSET: usize = 0x76;

/// Which game's cache this is.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum CacheKind {
    /// The Xbox's (version 5): compressed, its vertices in the tag data.
    Xbox,
    /// Halo Custom Edition's (version 609): uncompressed, its bitmaps and
    /// sounds in the shared resource maps.
    CustomEdition,
}

impl CacheKind {
    /// The header's version for the kind.
    pub const fn version(self) -> i32 {
        match self {
            Self::Xbox => 5,
            Self::CustomEdition => 609,
        }
    }

    /// The address the tag data is loaded at, which its pointers hold.
    pub const fn tag_base(self) -> u32 {
        match self {
            Self::Xbox => 0x803A_6000,
            Self::CustomEdition => 0x4044_0000,
        }
    }

    /// The most tag data the game has room for.
    pub const fn tag_cache_size(self) -> u32 {
        match self {
            Self::Xbox => 0x0160_0000,
            Self::CustomEdition => 0x0170_0000,
        }
    }

    /// The tag index header's size (Custom Edition's has a model data
    /// size the Xbox's has not); its signature is its last four bytes.
    pub const fn tag_header_size(self) -> usize {
        match self {
            Self::Xbox => 0x24,
            Self::CustomEdition => 0x28,
        }
    }
}

/// The cache's header.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct CacheHeader {
    /// The game the cache is for.
    pub kind: CacheKind,
    /// The cache's length, uncompressed.
    pub file_length: u32,
    /// Where the tag data starts, in the uncompressed cache.
    pub tag_data_offset: u32,
    /// The tag data's size.
    pub tag_data_size: u32,
    /// The map's name (`bloodgulch`).
    pub name: String,
    /// The build that wrote the cache.
    pub build: String,
    /// Campaign (0), multiplayer (1) or the main menu (2).
    pub scenario_type: i16,
    /// The cache's checksum, as the game compares them.
    pub checksum: u32,
}

/// A cache, uncompressed, with its tag index read.
#[derive(Debug)]
pub struct Cache {
    header: CacheHeader,
    file: Vec<u8>,
    pub(crate) index: TagIndex,
}

fn header_fault(field: &'static str, reason: impl ToString) -> ImportError {
    ImportError::Header {
        field,
        reason: reason.to_string(),
    }
}

fn header_read<T>(field: &'static str, read: Result<T, Fault>) -> Result<T, ImportError> {
    read.map_err(|fault| header_fault(field, fault))
}

fn four_cc(
    bytes: &[u8],
    at: usize,
    field: &'static str,
    code: &[u8; 4],
) -> Result<(), ImportError> {
    let found = GroupTag(header_read(field, reader::u32(bytes, at))?);
    if found == GroupTag::new(code) {
        Ok(())
    } else {
        Err(header_fault(
            field,
            format!("is {found}, not {}", GroupTag::new(code)),
        ))
    }
}

impl Cache {
    /// Reads a cache file: checks its header, decompresses an Xbox cache,
    /// and reads its tag index.
    pub fn open(mut file: Vec<u8>) -> Result<Self, ImportError> {
        if file.len() < HEADER_SIZE {
            return Err(header_fault(
                "length",
                format!("is {:#x} bytes, shorter than its header", file.len()),
            ));
        }
        let bytes = &file[..HEADER_SIZE];
        four_cc(bytes, 0, "header signature", b"head")?;
        four_cc(bytes, FOOTER_OFFSET, "footer signature", b"foot")?;
        let version = header_read("version", reader::i32(bytes, VERSION_OFFSET))?;
        let kind = match version {
            5 => CacheKind::Xbox,
            609 => CacheKind::CustomEdition,
            _ => {
                return Err(header_fault(
                    "version",
                    format!("is {version}, not 5 (Xbox) or 609 (Custom Edition)"),
                ))
            }
        };
        let text = |at: usize, field: &'static str| {
            header_read(field, reader::text(bytes, at, HEADER_STRING_LONGEST)).map(str::to_owned)
        };
        let mut header = CacheHeader {
            kind,
            file_length: header_read("file length", reader::u32(bytes, FILE_LENGTH_OFFSET))?,
            tag_data_offset: header_read(
                "tag data offset",
                reader::u32(bytes, TAG_DATA_OFFSET_OFFSET),
            )?,
            tag_data_size: header_read("tag data size", reader::u32(bytes, TAG_DATA_SIZE_OFFSET))?,
            name: text(NAME_OFFSET, "name")?,
            build: text(BUILD_OFFSET, "build")?,
            scenario_type: header_read("scenario type", reader::i16(bytes, SCENARIO_TYPE_OFFSET))?,
            checksum: header_read("checksum", reader::u32(bytes, CHECKSUM_OFFSET))?,
        };

        match kind {
            CacheKind::CustomEdition => {
                let opensauce = GroupTag(header_read(
                    "OpenSauce signature",
                    reader::u32(bytes, OPENSAUCE_OFFSET),
                )?);
                let flags = header_read(
                    "OpenSauce flags",
                    reader::u16(bytes, OPENSAUCE_FLAGS_OFFSET),
                )?;
                if opensauce == GroupTag::new(b"yelo") && flags != 0 {
                    return Err(header_fault(
                        "OpenSauce flags",
                        format!("are {flags:#x}: the map needs OpenSauce, which is not supported"),
                    ));
                }
                let compressed = header_read(
                    "compressed length",
                    reader::u32(bytes, COMPRESSED_LENGTH_OFFSET),
                )?;
                if compressed != 0 {
                    return Err(header_fault(
                        "compressed length",
                        format!(
                            "is {compressed:#x}: compressed Custom Edition caches are not read"
                        ),
                    ));
                }
                // (Invader leaves the length 0; the game reads the whole file)
                if header.file_length == 0 {
                    header.file_length = u32::try_from(file.len()).unwrap_or(u32::MAX);
                }
                check_file_length(header.file_length, file.len())?;
                file.truncate(header.file_length as usize);
            }
            CacheKind::Xbox => {
                check_file_length(header.file_length, usize::MAX)?;
                let inflated_length = header.file_length as usize - HEADER_SIZE;
                let inflated = miniz_oxide::inflate::decompress_to_vec_zlib_with_limit(
                    &file[HEADER_SIZE..],
                    inflated_length,
                )
                .map_err(|error| {
                    header_fault(
                        "compressed data",
                        format!(
                            "does not inflate to {inflated_length:#x} bytes ({:?})",
                            error.status
                        ),
                    )
                })?;
                if inflated.len() != inflated_length {
                    return Err(header_fault(
                        "compressed data",
                        format!(
                            "inflates to {:#x} bytes, not the {inflated_length:#x} of the file length",
                            inflated.len()
                        ),
                    ));
                }
                file.truncate(HEADER_SIZE);
                file.extend_from_slice(&inflated);
            }
        }

        let tag_data_end = u64::from(header.tag_data_offset) + u64::from(header.tag_data_size);
        if (header.tag_data_offset as usize) < HEADER_SIZE {
            return Err(header_fault(
                "tag data offset",
                format!("is {:#x}, inside the header", header.tag_data_offset),
            ));
        }
        if (header.tag_data_size as usize) < kind.tag_header_size()
            || header.tag_data_size > kind.tag_cache_size()
        {
            return Err(header_fault(
                "tag data size",
                format!(
                    "is {:#x}, not between the tag index's {:#x} and the tag cache's {:#x}",
                    header.tag_data_size,
                    kind.tag_header_size(),
                    kind.tag_cache_size()
                ),
            ));
        }
        if tag_data_end > u64::from(header.file_length) {
            return Err(header_fault(
                "tag data",
                format!(
                    "ends at {tag_data_end:#x}, past the file's {:#x} bytes",
                    header.file_length
                ),
            ));
        }

        let index = TagIndex::read(kind, tag_data(&file, &header))?;
        Ok(Self {
            header,
            file,
            index,
        })
    }

    /// The header.
    pub fn header(&self) -> &CacheHeader {
        &self.header
    }

    /// The whole cache, uncompressed.
    pub fn file(&self) -> &[u8] {
        &self.file
    }

    /// The tag data, which the tags' addresses point into.
    pub fn tag_data(&self) -> &[u8] {
        tag_data(&self.file, &self.header)
    }

    /// Where an address of the tag data is, in [`Self::tag_data`], if all
    /// `size` bytes from it are there.
    pub fn tag_offset(&self, address: u32, size: usize) -> Result<usize, Fault> {
        tag_offset(self.header.kind, self.header.tag_data_size, address, size)
    }
}

fn check_file_length(length: u32, available: usize) -> Result<(), ImportError> {
    if (length as usize) < HEADER_SIZE || length > MAX_FILE_LENGTH || length as usize > available {
        return Err(header_fault(
            "file length",
            format!(
                "is {length:#x}, not between the header's {HEADER_SIZE:#x} and {:#x}",
                (MAX_FILE_LENGTH as usize).min(available)
            ),
        ));
    }
    Ok(())
}

fn tag_data<'a>(file: &'a [u8], header: &CacheHeader) -> &'a [u8] {
    // (Cache::open checked the range against the file's length)
    let start = header.tag_data_offset as usize;
    file.get(start..start + header.tag_data_size as usize)
        .unwrap_or(&[])
}

pub(crate) fn tag_offset(
    kind: CacheKind,
    tag_data_size: u32,
    address: u32,
    size: usize,
) -> Result<usize, Fault> {
    region_offset(kind.tag_base(), u64::from(tag_data_size), address, size)
}

/// Where `address` is in bytes whose first is at `base`, if all `size`
/// bytes from it are within `length`.
pub(crate) fn region_offset(
    base: u32,
    length: u64,
    address: u32,
    size: usize,
) -> Result<usize, Fault> {
    let outside = Fault::OutOfRange {
        at: u64::from(address),
        size: size as u64,
        length,
    };
    let offset = address.checked_sub(base).ok_or(outside)?;
    let end = u64::from(offset).checked_add(size as u64).ok_or(outside)?;
    if end > length {
        return Err(outside);
    }
    Ok(offset as usize)
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::fake::FakeCache;

    fn error(file: Vec<u8>) -> String {
        Cache::open(file)
            .err()
            .map(|e| e.to_string())
            .unwrap_or_default()
    }

    #[test]
    fn both_kinds_read_back() {
        for kind in [CacheKind::Xbox, CacheKind::CustomEdition] {
            let fake = FakeCache::new(kind);
            let cache = Cache::open(fake.build());
            let cache = match cache {
                Ok(cache) => cache,
                Err(error) => panic!("{kind:?}: {error}"),
            };
            assert_eq!(cache.header().kind, kind);
            assert_eq!(cache.header().name, "fake");
            assert_eq!(cache.header().build, "01.01.14.2342");
            assert_eq!(cache.file().len(), cache.header().file_length as usize);
        }
    }

    #[test]
    fn a_short_file_is_refused() {
        assert_eq!(
            error(vec![0; 16]),
            "the cache's length is 0x10 bytes, shorter than its header"
        );
    }

    #[test]
    fn bad_signatures_and_versions_are_named() {
        let mut file = FakeCache::new(CacheKind::CustomEdition).build();
        file[0x7FC] = b'x';
        assert_eq!(
            error(file),
            "the cache's footer signature is foox, not foot"
        );
        let mut file = FakeCache::new(CacheKind::CustomEdition).build();
        file[4..8].copy_from_slice(&7i32.to_le_bytes());
        assert_eq!(
            error(file),
            "the cache's version is 7, not 5 (Xbox) or 609 (Custom Edition)"
        );
    }

    #[test]
    fn opensauce_flags_are_refused_and_its_bare_header_is_not() {
        let mut file = FakeCache::new(CacheKind::CustomEdition).build();
        file[0x70..0x74].copy_from_slice(&GroupTag::new(b"yelo").0.to_le_bytes());
        assert!(Cache::open(file.clone()).is_ok());
        file[0x76] = 1;
        assert!(error(file).starts_with("the cache's OpenSauce flags are 0x1"));
    }

    #[test]
    fn tag_data_outside_the_file_is_refused() {
        let mut file = FakeCache::new(CacheKind::CustomEdition).build();
        file[0x14..0x18].copy_from_slice(&0x0100_0000u32.to_le_bytes());
        assert!(error(file).starts_with("the cache's tag data ends at"));
        let mut file = FakeCache::new(CacheKind::CustomEdition).build();
        file[0x14..0x18].copy_from_slice(&0x0200_0000u32.to_le_bytes());
        assert!(error(file).starts_with("the cache's tag data size is 0x2000000"));
        let mut file = FakeCache::new(CacheKind::CustomEdition).build();
        file[0x10..0x14].copy_from_slice(&0x10u32.to_le_bytes());
        assert!(error(file).starts_with("the cache's tag data offset is 0x10"));
    }

    #[test]
    fn a_file_length_past_the_file_is_refused() {
        let mut file = FakeCache::new(CacheKind::CustomEdition).build();
        let length = file.len() as u32 + 1;
        file[8..12].copy_from_slice(&length.to_le_bytes());
        assert!(error(file).starts_with("the cache's file length is"));
    }

    #[test]
    fn an_xbox_cache_must_inflate_to_its_length() {
        let mut file = FakeCache::new(CacheKind::Xbox).build();
        let length = u32::from_le_bytes([file[8], file[9], file[10], file[11]]) + 4;
        file[8..12].copy_from_slice(&length.to_le_bytes());
        assert!(error(file).starts_with("the cache's compressed data inflates to"));
        let mut file = FakeCache::new(CacheKind::Xbox).build();
        file.truncate(HEADER_SIZE + 8);
        assert!(error(file).starts_with("the cache's compressed data does not inflate"));
    }

    #[test]
    fn addresses_outside_the_tag_data_are_refused() {
        let kind = CacheKind::Xbox;
        assert_eq!(
            tag_offset(kind, 0x100, kind.tag_base() + 0xF0, 0x10),
            Ok(0xF0)
        );
        assert!(tag_offset(kind, 0x100, kind.tag_base() + 0xF1, 0x10).is_err());
        assert!(tag_offset(kind, 0x100, kind.tag_base() - 1, 1).is_err());
        assert!(tag_offset(kind, 0x100, u32::MAX, usize::MAX).is_err());
    }
}
