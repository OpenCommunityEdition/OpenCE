//! The guest image: a statically linked AArch64 ELF executable of ILP32 code
//! (tools/android_build.py's guest_image_build), checked and copied into
//! guest memory at the addresses it was linked at, as the Android host's
//! loader does (port/android/host/host_loader.c).
//!
//! The image is a file, so nothing in it is trusted: every offset, size and
//! address is checked before it is used.

use crate::memory::{GuestMemory, GuestPtr, GuestRange, MemoryError};
use crate::page_tables::SegmentAccess;

/// e_machine of AArch64
pub const EM_AARCH64: u16 = 183;
/// halo_android_abi.h's HALO_GUEST_MAGIC ('HALO') and HALO_GUEST_ABI_VERSION
pub const GUEST_MAGIC: u32 = 0x4f4c_4148;
pub const GUEST_ABI_VERSION: u32 = 2;
/// the longest import name looked for
pub const IMPORT_NAME_LIMIT: u32 = 256;

const PT_LOAD: u32 = 1;
const PF_X: u32 = 1;
const PF_W: u32 = 2;
const ELF_HEADER_SIZE: usize = 64;
const PROGRAM_HEADER_SIZE: usize = 56;

#[derive(Debug, thiserror::Error)]
pub enum ImageError {
    #[error("the guest image is not a 64-bit little-endian AArch64 executable")]
    NotAnExecutable,
    #[error("the guest image's program headers lie outside the file")]
    ProgramHeaders,
    #[error("segment {0}'s file contents lie outside the file")]
    SegmentData(usize),
    #[error("segment {0} has more file contents than memory")]
    SegmentSize(usize),
    #[error("segment {index} ({address:#x}) lies outside the image's range")]
    SegmentOutsideImage { index: usize, address: u64 },
    #[error("the guest image has no loadable segments")]
    NoSegments,
    #[error("the guest image starts at {found:#010x}, not at {expected:#010x}")]
    WrongBase { expected: u32, found: u32 },
    #[error("the guest image's header does not match this host (magic {magic:#010x}, ABI version {abi_version})")]
    WrongHeader { magic: u32, abi_version: u32 },
    #[error("the guest image's {0} lies outside the image")]
    OutsideImage(&'static str),
    #[error("the guest's import {index} is {found:?}; the host expects {expected:?}")]
    ImportMismatch {
        index: usize,
        expected: String,
        found: String,
    },
    #[error("the guest has {found} imports; the host expects {expected}")]
    ImportCount { expected: usize, found: usize },
    #[error("guest memory: {0}")]
    Memory(#[from] MemoryError),
}

/// One loadable segment.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub struct Segment {
    pub range: GuestRange,
    pub file_offset: u64,
    pub file_size: u64,
    pub writable: bool,
    pub executable: bool,
}

/// A checked guest image.
#[derive(Clone, Debug)]
pub struct GuestImage {
    segments: Vec<Segment>,
    range: GuestRange,
}

/// The image's header (halo_android_abi.h's struct halo_guest_header), at
/// the start of the image.
#[derive(Clone, Copy, PartialEq, Eq, Debug, Default)]
pub struct GuestHeader {
    pub magic: u32,
    pub abi_version: u32,
    pub image_end: u32,
    pub import_table: u32,
    pub import_names: u32,
    pub import_count: u32,
    pub start: u32,
    pub thread_start: u32,
    pub thread_attach: u32,
    pub thread_detach: u32,
    pub init_array_start: u32,
    pub init_array_end: u32,
}

impl GuestImage {
    /// Checks `file` as an image to be loaded in `image_range`, starting at
    /// its start.
    pub fn parse(file: &[u8], image_range: GuestRange) -> Result<GuestImage, ImageError> {
        let is_executable = file.len() >= ELF_HEADER_SIZE
            && file[..4] == *b"\x7fELF"
            && file[4] == 2 // ELFCLASS64
            && file[5] == 1 // little-endian
            && u16_at(file, 16) == Some(2) // ET_EXEC
            && u16_at(file, 18) == Some(EM_AARCH64);
        if !is_executable {
            return Err(ImageError::NotAnExecutable);
        }
        let headers = u64_at(file, 32).ok_or(ImageError::ProgramHeaders)?;
        let entry_size = u16_at(file, 54).ok_or(ImageError::ProgramHeaders)? as u64;
        let count = u16_at(file, 56).ok_or(ImageError::ProgramHeaders)? as u64;
        let headers_end = headers
            .checked_add(entry_size * count)
            .ok_or(ImageError::ProgramHeaders)?;
        if entry_size < PROGRAM_HEADER_SIZE as u64 || headers_end > file.len() as u64 {
            return Err(ImageError::ProgramHeaders);
        }

        let mut segments = Vec::new();
        for index in 0..count as usize {
            // (inside the file: checked above)
            let at = (headers + index as u64 * entry_size) as usize;
            let field = |offset| u64_at(file, at + offset).ok_or(ImageError::ProgramHeaders);
            if u32_at(file, at) != Some(PT_LOAD) {
                continue;
            }
            let flags = u32_at(file, at + 4).ok_or(ImageError::ProgramHeaders)?;
            let (file_offset, address, file_size, memory_size) =
                (field(8)?, field(16)?, field(32)?, field(40)?);
            if file_offset
                .checked_add(file_size)
                .is_none_or(|end| end > file.len() as u64)
            {
                return Err(ImageError::SegmentData(index));
            }
            if file_size > memory_size {
                return Err(ImageError::SegmentSize(index));
            }
            if memory_size == 0 {
                continue;
            }
            let inside = address >= u64::from(image_range.start().0)
                && address
                    .checked_add(memory_size)
                    .is_some_and(|end| end <= image_range.end());
            if !inside {
                return Err(ImageError::SegmentOutsideImage { index, address });
            }
            segments.push(Segment {
                // (inside the image's range, so below 4 GB)
                range: GuestRange::new(GuestPtr(address as u32), memory_size)?,
                file_offset,
                file_size,
                writable: flags & PF_W != 0,
                executable: flags & PF_X != 0,
            });
        }

        let low = segments
            .iter()
            .map(|segment| segment.range.start().0 & !(PAGE - 1))
            .min()
            .ok_or(ImageError::NoSegments)?;
        if low != image_range.start().0 {
            return Err(ImageError::WrongBase {
                expected: image_range.start().0,
                found: low,
            });
        }
        let high = segments
            .iter()
            .map(|segment| segment.range.end())
            .max()
            .unwrap_or(u64::from(low));
        let high = high.div_ceil(u64::from(PAGE)) * u64::from(PAGE);
        Ok(GuestImage {
            segments,
            range: GuestRange::new(GuestPtr(low), high - u64::from(low))?,
        })
    }

    pub fn segments(&self) -> &[Segment] {
        &self.segments
    }

    /// From the first segment's page to the last one's end, in whole pages.
    pub fn range(&self) -> GuestRange {
        self.range
    }

    /// How the page tables are to map the segments.
    pub fn access(&self) -> Vec<SegmentAccess> {
        self.segments
            .iter()
            .map(|segment| SegmentAccess {
                range: segment.range,
                writable: segment.writable,
                executable: segment.executable,
            })
            .collect()
    }

    /// Copies the segments from `file` (the one parse checked) into guest
    /// memory, where the image's range must be mapped; each segment's
    /// memory past its file contents (its bss) is zeroed.
    pub fn load(&self, file: &[u8], memory: &GuestMemory) -> Result<(), ImageError> {
        static ZEROES: [u8; 0x1_0000] = [0; 0x1_0000];
        for (index, segment) in self.segments.iter().enumerate() {
            let contents = usize::try_from(segment.file_offset)
                .ok()
                .zip(usize::try_from(segment.file_size).ok())
                .and_then(|(start, size)| file.get(start..start.checked_add(size)?))
                .ok_or(ImageError::SegmentData(index))?;
            memory.write(segment.range.start(), contents)?;
            // the bss: (file_size <= the segment's size, checked by parse)
            let mut offset = segment.file_size;
            while offset < segment.range.size() {
                let size = (segment.range.size() - offset).min(ZEROES.len() as u64);
                // (offset < the segment's size, which is below 4 GB)
                let at = GuestPtr(segment.range.start().0 + offset as u32);
                memory.write(at, &ZEROES[..size as usize])?;
                offset += size;
            }
        }
        Ok(())
    }

    /// The header of the loaded image, checked: the magic and ABI version,
    /// and its end, import table, count and names inside the image.
    pub fn header(&self, memory: &GuestMemory) -> Result<GuestHeader, ImageError> {
        let base = self.range.start();
        let mut words = [0u32; 12];
        for (index, word) in words.iter_mut().enumerate() {
            *word = memory.read_u32(GuestPtr(base.0 + 4 * index as u32))?;
        }
        let header = GuestHeader {
            magic: words[0],
            abi_version: words[1],
            image_end: words[2],
            import_table: words[3],
            import_names: words[4],
            import_count: words[5],
            start: words[6],
            thread_start: words[7],
            thread_attach: words[8],
            thread_detach: words[9],
            init_array_start: words[10],
            init_array_end: words[11],
        };
        if header.magic != GUEST_MAGIC || header.abi_version != GUEST_ABI_VERSION {
            return Err(ImageError::WrongHeader {
                magic: header.magic,
                abi_version: header.abi_version,
            });
        }
        let header_end = u64::from(base.0) + 4 * words.len() as u64;
        if u64::from(header.image_end) < header_end
            || u64::from(header.image_end) > self.range.end()
        {
            return Err(ImageError::OutsideImage("end"));
        }
        self.inside(&header, header.import_count, 4, "import count")?;
        let count = memory.read_u32(GuestPtr(header.import_count))?;
        self.inside(
            &header,
            header.import_table,
            8 * u64::from(count),
            "import table",
        )?;
        self.inside(&header, header.import_names, 1, "import names")?;
        Ok(header)
    }

    /// The loaded image's import names, in the order its stubs number them.
    pub fn import_names(
        &self,
        memory: &GuestMemory,
        header: &GuestHeader,
    ) -> Result<Vec<String>, ImageError> {
        let count = memory.read_u32(GuestPtr(header.import_count))?;
        let mut names = Vec::new();
        let mut at = header.import_names;
        for _ in 0..count {
            self.inside(header, at, 1, "import names")?;
            let limit = IMPORT_NAME_LIMIT.min(header.image_end - at);
            let name = memory
                .read_c_string(GuestPtr(at), limit)
                .map_err(|error| match error {
                    MemoryError::Unterminated { .. } => ImageError::OutsideImage("import names"),
                    other => ImageError::Memory(other),
                })?;
            // (shorter than limit, so at stays below image_end)
            at += name.len() as u32 + 1;
            names.push(String::from_utf8_lossy(&name).into_owned());
        }
        Ok(names)
    }

    /// Checks that `size` bytes at `address` lie between the image's start
    /// and the header's image_end.
    fn inside(
        &self,
        header: &GuestHeader,
        address: u32,
        size: u64,
        what: &'static str,
    ) -> Result<(), ImageError> {
        let start = u64::from(address);
        let fits = address >= self.range.start().0
            && start
                .checked_add(size)
                .is_some_and(|end| end <= u64::from(header.image_end));
        if fits {
            Ok(())
        } else {
            Err(ImageError::OutsideImage(what))
        }
    }
}

/// Checks the guest's import names against the host's, index for index.
pub fn check_imports(guest: &[String], host: &[&str]) -> Result<(), ImageError> {
    if guest.len() != host.len() {
        return Err(ImageError::ImportCount {
            expected: host.len(),
            found: guest.len(),
        });
    }
    match guest
        .iter()
        .zip(host)
        .position(|(found, expected)| found != expected)
    {
        Some(index) => Err(ImageError::ImportMismatch {
            index,
            expected: host[index].to_string(),
            found: guest[index].clone(),
        }),
        None => Ok(()),
    }
}

/// The guest's pages, which segments are rounded to.
const PAGE: u32 = 0x1000;

fn u16_at(bytes: &[u8], at: usize) -> Option<u16> {
    Some(u16::from_le_bytes(
        bytes.get(at..at.checked_add(2)?)?.try_into().ok()?,
    ))
}

fn u32_at(bytes: &[u8], at: usize) -> Option<u32> {
    Some(u32::from_le_bytes(
        bytes.get(at..at.checked_add(4)?)?.try_into().ok()?,
    ))
}

fn u64_at(bytes: &[u8], at: usize) -> Option<u64> {
    Some(u64::from_le_bytes(
        bytes.get(at..at.checked_add(8)?)?.try_into().ok()?,
    ))
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::plan::{IMAGE_BASE, IMAGE_SIZE};
    use std::path::Path;

    fn image_range() -> GuestRange {
        GuestRange::new(GuestPtr(IMAGE_BASE), IMAGE_SIZE).expect("the image range")
    }

    struct TestSegment {
        address: u64,
        flags: u32,
        data: Vec<u8>,
        memory_size: u64,
    }

    fn segment(address: u64, flags: u32, data: &[u8], memory_size: u64) -> TestSegment {
        TestSegment {
            address,
            flags,
            data: data.to_vec(),
            memory_size,
        }
    }

    /// A minimal ELF executable of these segments.
    fn elf(segments: &[TestSegment]) -> Vec<u8> {
        let headers_end = ELF_HEADER_SIZE + PROGRAM_HEADER_SIZE * segments.len();
        let mut file = vec![0u8; headers_end];
        file[..4].copy_from_slice(b"\x7fELF");
        file[4] = 2; // ELFCLASS64
        file[5] = 1; // little-endian
        file[6] = 1; // version
        file[16..18].copy_from_slice(&2u16.to_le_bytes()); // ET_EXEC
        file[18..20].copy_from_slice(&EM_AARCH64.to_le_bytes());
        file[24..32].copy_from_slice(&u64::from(IMAGE_BASE).to_le_bytes());
        file[32..40].copy_from_slice(&(ELF_HEADER_SIZE as u64).to_le_bytes());
        file[54..56].copy_from_slice(&(PROGRAM_HEADER_SIZE as u16).to_le_bytes());
        file[56..58].copy_from_slice(&(segments.len() as u16).to_le_bytes());
        for (index, segment) in segments.iter().enumerate() {
            let offset = file.len() as u64;
            file.extend_from_slice(&segment.data);
            let header = ELF_HEADER_SIZE + index * PROGRAM_HEADER_SIZE;
            let fields: [(usize, u64, usize); 6] = [
                (8, offset, 8),
                (16, segment.address, 8),
                (24, segment.address, 8),
                (32, segment.data.len() as u64, 8),
                (40, segment.memory_size, 8),
                (48, 0x1000, 8),
            ];
            file[header..header + 4].copy_from_slice(&PT_LOAD.to_le_bytes());
            file[header + 4..header + 8].copy_from_slice(&segment.flags.to_le_bytes());
            for (at, value, size) in fields {
                file[header + at..header + at + size].copy_from_slice(&value.to_le_bytes()[..size]);
            }
        }
        file
    }

    /// A guest header followed by an import table, count and names, as
    /// tools/android_imports.py lays them out.
    fn header_bytes(names: &[&str]) -> Vec<u8> {
        let base = IMAGE_BASE;
        let mut names_bytes = Vec::new();
        for name in names {
            names_bytes.extend_from_slice(name.as_bytes());
            names_bytes.push(0);
        }
        let count_at = base + 0x30;
        let table_at = base + 0x40;
        let names_at = table_at + 8 * names.len() as u32;
        let header = [
            GUEST_MAGIC,
            GUEST_ABI_VERSION,
            names_at + names_bytes.len() as u32,
            table_at,
            names_at,
            count_at,
            base + 0x100,
            0,
            0,
            0,
            0,
            0,
        ];
        let mut bytes: Vec<u8> = header.iter().flat_map(|word| word.to_le_bytes()).collect();
        bytes.extend_from_slice(&(names.len() as u32).to_le_bytes());
        bytes.resize(0x40, 0);
        bytes.resize(0x40 + 8 * names.len(), 0);
        bytes.extend_from_slice(&names_bytes);
        bytes
    }

    fn mapped_memory() -> GuestMemory {
        let mut memory = GuestMemory::reserve().expect("4 GB of address space");
        memory.map(image_range()).expect("the image range maps");
        memory
    }

    fn sample() -> Vec<u8> {
        elf(&[
            segment(
                0x8800_0000,
                4 | PF_X,
                &header_bytes(&["host_syscall", "host_log"]),
                0x2000,
            ),
            segment(0x8801_0000, 4 | PF_W, b"data", 0x3000),
        ])
    }

    #[test]
    fn a_good_image_has_its_segments_and_range() {
        let image = GuestImage::parse(&sample(), image_range()).expect("a good image");
        let segments = image.segments();
        assert_eq!(segments.len(), 2);
        assert!(segments[0].executable && !segments[0].writable);
        assert!(segments[1].writable && !segments[1].executable);
        assert_eq!(segments[1].range.start(), GuestPtr(0x8801_0000));
        assert_eq!(segments[1].range.size(), 0x3000);
        assert_eq!(
            (image.range().start(), image.range().end()),
            (GuestPtr(IMAGE_BASE), 0x8801_3000)
        );
        let access = image.access();
        assert_eq!(access[0].range, segments[0].range);
        assert!(access[0].executable && access[1].writable);
    }

    #[test]
    fn loading_copies_the_contents_and_zeroes_the_bss() {
        let file = sample();
        let image = GuestImage::parse(&file, image_range()).expect("a good image");
        let memory = mapped_memory();
        // (what was there before is overwritten)
        memory
            .write(GuestPtr(0x8801_0004), &[0xff; 8])
            .expect("mapped");
        image.load(&file, &memory).expect("it loads");
        let mut data = [0u8; 12];
        memory
            .read(GuestPtr(0x8801_0000), &mut data)
            .expect("mapped");
        assert_eq!(&data, b"data\0\0\0\0\0\0\0\0");
        let header = image.header(&memory).expect("a good header");
        assert_eq!(
            (header.magic, header.abi_version),
            (GUEST_MAGIC, GUEST_ABI_VERSION)
        );
        assert_eq!(
            image.import_names(&memory, &header).expect("names"),
            ["host_syscall", "host_log"]
        );
    }

    #[test]
    fn files_that_are_not_aarch64_executables_are_refused() {
        let good = sample();
        let mut cases: Vec<Vec<u8>> = vec![good[..40].to_vec()];
        for (at, value) in [(0usize, 0u8), (4, 1), (5, 2), (16, 3), (18, 62)] {
            let mut bad = good.clone();
            bad[at] = value;
            cases.push(bad);
        }
        for (index, bad) in cases.iter().enumerate() {
            assert!(
                matches!(
                    GuestImage::parse(bad, image_range()),
                    Err(ImageError::NotAnExecutable)
                ),
                "case {index}"
            );
        }
    }

    #[test]
    fn headers_and_contents_outside_the_file_are_refused() {
        let good = sample();
        let mut far = good.clone();
        far[32..40].copy_from_slice(&u64::MAX.to_le_bytes());
        assert!(matches!(
            GuestImage::parse(&far, image_range()),
            Err(ImageError::ProgramHeaders)
        ));
        let mut short_entries = good.clone();
        short_entries[54..56].copy_from_slice(&8u16.to_le_bytes());
        assert!(matches!(
            GuestImage::parse(&short_entries, image_range()),
            Err(ImageError::ProgramHeaders)
        ));
        // the second segment's file size past the end of the file
        let mut data_past = good.clone();
        let at = ELF_HEADER_SIZE + PROGRAM_HEADER_SIZE + 32;
        data_past[at..at + 8].copy_from_slice(&0x1_0000u64.to_le_bytes());
        assert!(matches!(
            GuestImage::parse(&data_past, image_range()),
            Err(ImageError::SegmentData(1))
        ));
        // an offset that wraps
        let mut wrapping = good.clone();
        let at = ELF_HEADER_SIZE + PROGRAM_HEADER_SIZE + 8;
        wrapping[at..at + 8].copy_from_slice(&u64::MAX.to_le_bytes());
        assert!(matches!(
            GuestImage::parse(&wrapping, image_range()),
            Err(ImageError::SegmentData(1))
        ));
    }

    #[test]
    fn segments_must_fit_their_memory_and_the_image_range() {
        let bigger_file = elf(&[segment(0x8800_0000, 4, b"0123456789", 4)]);
        assert!(matches!(
            GuestImage::parse(&bigger_file, image_range()),
            Err(ImageError::SegmentSize(0))
        ));
        let past_end = elf(&[
            segment(0x8800_0000, 4, b"x", 0x1000),
            segment(0x8bff_f000, 4, b"x", 0x2000),
        ]);
        assert!(matches!(
            GuestImage::parse(&past_end, image_range()),
            Err(ImageError::SegmentOutsideImage {
                index: 1,
                address: 0x8bff_f000
            })
        ));
        let huge = elf(&[segment(0x8800_0000, 4, b"x", u64::MAX)]);
        assert!(matches!(
            GuestImage::parse(&huge, image_range()),
            Err(ImageError::SegmentOutsideImage { index: 0, .. })
        ));
        let elsewhere = elf(&[segment(0x8801_0000, 4, b"x", 0x1000)]);
        assert!(matches!(
            GuestImage::parse(&elsewhere, image_range()),
            Err(ImageError::WrongBase {
                expected: IMAGE_BASE,
                found: 0x8801_0000
            })
        ));
        assert!(matches!(
            GuestImage::parse(&elf(&[]), image_range()),
            Err(ImageError::NoSegments)
        ));
    }

    fn loaded_with_header(edit: impl FnOnce(&mut Vec<u8>)) -> Result<GuestHeader, ImageError> {
        let mut header = header_bytes(&["host_syscall"]);
        edit(&mut header);
        let file = elf(&[segment(0x8800_0000, 4 | PF_X, &header, 0x2000)]);
        let image = GuestImage::parse(&file, image_range())?;
        let memory = mapped_memory();
        image.load(&file, &memory)?;
        let header = image.header(&memory)?;
        image.import_names(&memory, &header)?;
        Ok(header)
    }

    #[test]
    fn headers_must_match_this_host_and_point_inside_the_image() {
        assert!(loaded_with_header(|_| {}).is_ok());
        assert!(matches!(
            loaded_with_header(|h| h[0] = 0),
            Err(ImageError::WrongHeader { abi_version: 2, .. })
        ));
        assert!(matches!(
            loaded_with_header(|h| h[4] = 1),
            Err(ImageError::WrongHeader { abi_version: 1, .. })
        ));
        // the table, the count and the names, each pointing out of the image
        for field in [3usize, 4, 5] {
            assert!(
                matches!(
                    loaded_with_header(|h| h[field * 4..field * 4 + 4]
                        .copy_from_slice(&0x8000_0000u32.to_le_bytes())),
                    Err(ImageError::OutsideImage(_))
                ),
                "field {field}"
            );
        }
        // a count whose table runs past the image
        assert!(matches!(
            loaded_with_header(|h| h[0x30..0x34].copy_from_slice(&0x0100_0000u32.to_le_bytes())),
            Err(ImageError::OutsideImage(_))
        ));
        // names that are not terminated before the image's end
        assert!(loaded_with_header(|h| {
            let last = h.len() - 1;
            h[last] = b'x';
        })
        .is_err());
    }

    #[test]
    fn the_guests_imports_must_be_the_hosts() {
        let guest: Vec<String> = ["host_syscall", "host_log"]
            .iter()
            .map(|s| s.to_string())
            .collect();
        assert!(check_imports(&guest, &["host_syscall", "host_log"]).is_ok());
        assert!(matches!(
            check_imports(&guest, &["host_syscall", "host_abort"]),
            Err(ImageError::ImportMismatch { index: 1, .. })
        ));
        assert!(matches!(
            check_imports(&guest, &["host_syscall"]),
            Err(ImageError::ImportCount {
                expected: 1,
                found: 2
            })
        ));
    }

    #[test]
    fn the_built_guest_image_loads() {
        // (after `ninja macos`)
        let path =
            Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../build/macos/halo_guest.elf");
        let Ok(file) = std::fs::read(&path) else {
            eprintln!("skipped: no {}", path.display());
            return;
        };
        let image = GuestImage::parse(&file, image_range()).expect("the built image is good");
        assert!(image.segments().iter().any(|segment| segment.executable));
        let memory = mapped_memory();
        image.load(&file, &memory).expect("it loads");
        let header = image.header(&memory).expect("its header is good");
        let names = image
            .import_names(&memory, &header)
            .expect("its names are good");
        assert_eq!(names.first().map(String::as_str), Some("host_syscall"));
        assert_eq!(names.get(1).map(String::as_str), Some("host_log"));
        assert!(names.len() > 100);
    }
}
