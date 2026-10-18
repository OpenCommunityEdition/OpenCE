//! Small guest images for the tests: ELF executables and guest headers
//! laid out as the build makes them.

use crate::image::{EM_AARCH64, GUEST_ABI_VERSION, GUEST_MAGIC};
use crate::plan::IMAGE_BASE;

pub(crate) const PT_LOAD: u32 = 1;
pub(crate) const PF_X: u32 = 1;
pub(crate) const PF_W: u32 = 2;
const ELF_HEADER_SIZE: usize = 64;
const PROGRAM_HEADER_SIZE: usize = 56;

pub(crate) struct TestSegment {
    address: u64,
    flags: u32,
    data: Vec<u8>,
    memory_size: u64,
}

pub(crate) fn segment(address: u64, flags: u32, data: &[u8], memory_size: u64) -> TestSegment {
    TestSegment {
        address,
        flags,
        data: data.to_vec(),
        memory_size,
    }
}

/// A minimal ELF executable of these segments.
pub(crate) fn elf(segments: &[TestSegment]) -> Vec<u8> {
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
pub(crate) fn header_bytes(names: &[&str]) -> Vec<u8> {
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
