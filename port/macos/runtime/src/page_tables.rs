//! The guest's stage-1 translation tables and the system registers its
//! vCPUs start with.
//!
//! The VM's guest physical addresses are the guest's addresses
//! (crate::memory), and the tables map each guest address to the same
//! physical address: 4 KB granule, 32-bit addresses (TCR_EL1.T0SZ = 32, so
//! the walk starts at level 1, whose first four entries cover 1 GB each).
//! The plan's regions (crate::plan) are 2 MB blocks, except the image, which
//! is mapped in 4 KB pages so that only its code is executable. All memory is
//! normal, write-back cacheable, inner shareable: with the MMU off it would
//! all be device memory, slow, and unaligned accesses would fault.
//!
//! The descriptors' bits are those of the Arm architecture (VMSAv8-64,
//! "Translation table descriptor formats").

use std::collections::BTreeMap;

use crate::memory::{GuestPtr, GuestRange};
use crate::plan::{AddressSpacePlan, RegionKind, GUEST_PAGE};

const BLOCK: u64 = 0x20_0000;
const GIGABYTE: u64 = 0x4000_0000;

/// The size of one table: 512 descriptors of 8 bytes.
pub const TABLE_SIZE: u64 = 0x1000;
const ENTRIES: usize = 512;

/// Descriptor bits.
pub mod descriptor {
    /// a valid descriptor
    pub const VALID: u64 = 1 << 0;
    /// at level 1 or 2: a table (else a block); at level 3 a page must set it
    pub const TABLE_OR_PAGE: u64 = 1 << 1;
    /// MAIR_EL1's attribute index: bits [4:2]
    pub const ATTR_INDEX_SHIFT: u32 = 2;
    /// AP[2:1], bits [7:6]: 0b00 EL1 read-write, 0b01 EL0 and EL1 read-write,
    /// 0b10 EL1 read-only, 0b11 EL0 and EL1 read-only
    pub const AP_SHIFT: u32 = 6;
    pub const AP_EL1_RW: u64 = 0b00;
    pub const AP_ALL_RW: u64 = 0b01;
    pub const AP_EL1_RO: u64 = 0b10;
    pub const AP_ALL_RO: u64 = 0b11;
    /// SH[1:0], bits [9:8]: 0b11 inner shareable
    pub const INNER_SHAREABLE: u64 = 0b11 << 8;
    /// the access flag (else the first access faults)
    pub const ACCESS_FLAG: u64 = 1 << 10;
    /// not executable at EL1
    pub const PXN: u64 = 1 << 53;
    /// not executable at EL0
    pub const UXN: u64 = 1 << 54;
    /// the output address of a table or a page: bits [47:12]
    pub const ADDRESS_MASK: u64 = 0x0000_ffff_ffff_f000;
    /// the output address of a level 2 block: bits [47:21]
    pub const BLOCK_ADDRESS_MASK: u64 = 0x0000_ffff_ffe0_0000;
}

/// MAIR_EL1's attributes: index 0 normal write-back memory, index 1 device
/// memory (unused for now).
pub const MAIR_EL1: u64 = 0x04ff;

/// How the image's ELF segments may be used: from crate::image.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub struct SegmentAccess {
    pub range: GuestRange,
    pub writable: bool,
    pub executable: bool,
}

/// What a guest address translates to (PageTables::translate).
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub struct Translation {
    pub physical: u64,
    /// the descriptor that maps the address
    pub descriptor: u64,
    /// 2, a 2 MB block; 3, a 4 KB page
    pub level: u8,
}

impl Translation {
    /// AP[2:1] (descriptor::AP_*)
    pub fn access(&self) -> u64 {
        (self.descriptor >> descriptor::AP_SHIFT) & 0b11
    }

    pub fn guest_executable(&self) -> bool {
        self.descriptor & descriptor::UXN == 0
    }

    pub fn el1_executable(&self) -> bool {
        self.descriptor & descriptor::PXN == 0
    }
}

#[derive(Debug, PartialEq, Eq, thiserror::Error)]
pub enum PageTableError {
    #[error("the plan has no {0}")]
    MissingRegion(RegionKind),
    #[error("the image segment at {0:#010x} lies outside the image's range")]
    SegmentOutsideImage(u32),
    #[error("the image page at {0:#010x} would be both writable and executable")]
    WritableCode(u32),
    #[error(
        "the tables must start at a 4 KB-aligned address in the EL1 runtime's range, not {0:#010x}"
    )]
    TablesMisplaced(u32),
    #[error("{0} tables do not fit the EL1 runtime's range")]
    TablesTooLarge(usize),
    #[error("the exception vectors must start at a 2 KB-aligned address, not {0:#010x}")]
    VectorsMisaligned(u32),
}

/// The tables, as the host writes them into guest memory at `base`: the
/// level 1 table first, then the level 2 tables in address order, then the
/// level 3 tables in address order.
#[derive(Clone, Debug)]
pub struct PageTables {
    base: GuestPtr,
    descriptors: Vec<u64>,
}

impl PageTables {
    /// The tables for `plan`, with the image's pages as `segments` say, to
    /// be placed at `base` (in the EL1 runtime's range).
    pub fn build(
        plan: &AddressSpacePlan,
        segments: &[SegmentAccess],
        base: GuestPtr,
    ) -> Result<PageTables, PageTableError> {
        let runtime = plan
            .region(RegionKind::Runtime)
            .ok_or(PageTableError::MissingRegion(RegionKind::Runtime))?;
        let image = plan
            .region(RegionKind::Image)
            .ok_or(PageTableError::MissingRegion(RegionKind::Image))?;
        if u64::from(base.0) % TABLE_SIZE != 0
            || base < runtime.range.start()
            || u64::from(base.0) >= runtime.range.end()
        {
            return Err(PageTableError::TablesMisplaced(base.0));
        }
        let pages = image_pages(image.range, segments)?;

        // which gigabytes need a level 2 table, and which 2 MB of the image
        // a level 3 one (those its segments touch)
        let mut gigabytes: Vec<u64> = Vec::new();
        for region in plan.regions() {
            let first = u64::from(region.range.start().0) / GIGABYTE;
            let last = (region.range.end() - 1) / GIGABYTE;
            gigabytes.extend(first..=last);
        }
        gigabytes.sort_unstable();
        gigabytes.dedup();
        let mut image_blocks: Vec<u64> = pages
            .keys()
            .map(|&page| u64::from(page) / BLOCK * BLOCK)
            .collect();
        image_blocks.dedup();
        let count = 1 + gigabytes.len() + image_blocks.len();
        if u64::from(base.0) + count as u64 * TABLE_SIZE > runtime.range.end() {
            return Err(PageTableError::TablesTooLarge(count));
        }

        let table_address = |index: usize| u64::from(base.0) + index as u64 * TABLE_SIZE;
        let level2_index =
            |gigabyte: u64| 1 + gigabytes.iter().position(|&g| g == gigabyte).unwrap_or(0);
        let level3_index = |block: u64| {
            1 + gigabytes.len() + image_blocks.iter().position(|&b| b == block).unwrap_or(0)
        };
        let mut descriptors = vec![0u64; count * ENTRIES];
        for &gigabyte in &gigabytes {
            descriptors[gigabyte as usize] =
                table_descriptor(table_address(level2_index(gigabyte)));
        }
        for region in plan.regions() {
            let mut block = u64::from(region.range.start().0);
            while block < region.range.end() {
                let entry =
                    level2_index(block / GIGABYTE) * ENTRIES + ((block / BLOCK) as usize % ENTRIES);
                descriptors[entry] = match region.kind {
                    RegionKind::Image if image_blocks.contains(&block) => {
                        table_descriptor(table_address(level3_index(block)))
                    }
                    // (the image's range past its segments stays unmapped)
                    RegionKind::Image => 0,
                    RegionKind::Runtime => block_descriptor(block, runtime_attributes()),
                    RegionKind::Window | RegionKind::Pool(_) => {
                        block_descriptor(block, data_attributes())
                    }
                };
                block += BLOCK;
            }
        }
        for (&page, &(writable, executable)) in &pages {
            let page = u64::from(page);
            let entry = level3_index(page / BLOCK * BLOCK) * ENTRIES
                + ((page / GUEST_PAGE) as usize % ENTRIES);
            descriptors[entry] = page_descriptor(page, image_attributes(writable, executable));
        }
        Ok(PageTables { base, descriptors })
    }

    /// Where the level 1 table starts: TTBR0_EL1.
    pub fn base(&self) -> GuestPtr {
        self.base
    }

    pub fn table_count(&self) -> usize {
        self.descriptors.len() / ENTRIES
    }

    /// The tables' bytes, little-endian, as they go into guest memory.
    pub fn to_bytes(&self) -> Vec<u8> {
        self.descriptors
            .iter()
            .flat_map(|descriptor| descriptor.to_le_bytes())
            .collect()
    }

    /// Walks the tables as the MMU would.
    pub fn translate(&self, address: GuestPtr) -> Option<Translation> {
        let address = u64::from(address.0);
        let level1 = self.descriptor(0, (address / GIGABYTE) as usize)?;
        // (no level 1 blocks are made)
        let level2_table = self.table_index(level1)?;
        let level2 = self.descriptor(level2_table, ((address / BLOCK) as usize) % ENTRIES)?;
        if level2 & descriptor::TABLE_OR_PAGE == 0 {
            return Some(Translation {
                physical: (level2 & descriptor::BLOCK_ADDRESS_MASK) | (address % BLOCK),
                descriptor: level2,
                level: 2,
            });
        }
        let level3_table = self.table_index(level2)?;
        let level3 = self.descriptor(level3_table, ((address / GUEST_PAGE) as usize) % ENTRIES)?;
        if level3 & descriptor::TABLE_OR_PAGE == 0 {
            // (reserved at level 3: the MMU faults)
            return None;
        }
        Some(Translation {
            physical: (level3 & descriptor::ADDRESS_MASK) | (address % GUEST_PAGE),
            descriptor: level3,
            level: 3,
        })
    }

    /// Descriptor `entry` of table `table`, if valid.
    fn descriptor(&self, table: usize, entry: usize) -> Option<u64> {
        let descriptor = *self.descriptors.get(table * ENTRIES + entry)?;
        (descriptor & descriptor::VALID != 0).then_some(descriptor)
    }

    /// The index of the table a table descriptor points to.
    fn table_index(&self, descriptor: u64) -> Option<usize> {
        if descriptor & descriptor::TABLE_OR_PAGE == 0 {
            return None;
        }
        let offset = (descriptor & descriptor::ADDRESS_MASK).checked_sub(u64::from(self.base.0))?;
        let index = (offset / TABLE_SIZE) as usize;
        (index < self.table_count()).then_some(index)
    }
}

/// The image's pages, each with whether a segment that touches it is
/// writable, and whether one is executable.
fn image_pages(
    image: GuestRange,
    segments: &[SegmentAccess],
) -> Result<BTreeMap<u32, (bool, bool)>, PageTableError> {
    let mut pages: BTreeMap<u32, (bool, bool)> = BTreeMap::new();
    for segment in segments {
        if segment.range.start() < image.start() || segment.range.end() > image.end() {
            return Err(PageTableError::SegmentOutsideImage(segment.range.start().0));
        }
        let mut page = u64::from(segment.range.start().0) / GUEST_PAGE * GUEST_PAGE;
        while page < segment.range.end() {
            // (inside the image's range, so below 4 GB)
            let flags = pages.entry(page as u32).or_insert((false, false));
            flags.0 |= segment.writable;
            flags.1 |= segment.executable;
            page += GUEST_PAGE;
        }
    }
    if let Some((&page, _)) = pages
        .iter()
        .find(|(_, &(writable, executable))| writable && executable)
    {
        return Err(PageTableError::WritableCode(page));
    }
    Ok(pages)
}

fn common_attributes() -> u64 {
    descriptor::ACCESS_FLAG | descriptor::INNER_SHAREABLE | (0 << descriptor::ATTR_INDEX_SHIFT)
}

/// the guest's data: read-write at EL0 and EL1, never executable
fn data_attributes() -> u64 {
    common_attributes()
        | (descriptor::AP_ALL_RW << descriptor::AP_SHIFT)
        | descriptor::PXN
        | descriptor::UXN
}

/// the EL1 runtime: read-only and executable at EL1, out of EL0's reach
fn runtime_attributes() -> u64 {
    common_attributes() | (descriptor::AP_EL1_RO << descriptor::AP_SHIFT) | descriptor::UXN
}

fn image_attributes(writable: bool, executable: bool) -> u64 {
    if executable {
        common_attributes() | (descriptor::AP_ALL_RO << descriptor::AP_SHIFT) | descriptor::PXN
    } else if writable {
        data_attributes()
    } else {
        common_attributes()
            | (descriptor::AP_ALL_RO << descriptor::AP_SHIFT)
            | descriptor::PXN
            | descriptor::UXN
    }
}

fn table_descriptor(address: u64) -> u64 {
    descriptor::VALID | descriptor::TABLE_OR_PAGE | (address & descriptor::ADDRESS_MASK)
}

fn block_descriptor(address: u64, attributes: u64) -> u64 {
    descriptor::VALID | (address & descriptor::BLOCK_ADDRESS_MASK) | attributes
}

fn page_descriptor(address: u64, attributes: u64) -> u64 {
    descriptor::VALID
        | descriptor::TABLE_OR_PAGE
        | (address & descriptor::ADDRESS_MASK)
        | attributes
}

/// TCR_EL1: 32-bit addresses through TTBR0 (T0SZ = 32), 4 KB granule,
/// write-back inner shareable walks, no TTBR1 walks (EPD1, with TG1 a valid
/// 4 KB), 32-bit physical addresses (IPS = 0).
const TCR_EL1: u64 = 32 | (0b01 << 8) | (0b01 << 10) | (0b11 << 12) | (1 << 23) | (0b10 << 30);

/// SCTLR_EL1: the MMU and both caches on, stack alignment checks at EL0 and
/// EL1, and EL0 may use WFI, WFE, DC ZVA, CTR_EL0 and the cache maintenance
/// instructions; with ARMv8.0's RES1 bits (11, 20, 22, 23, 28, 29).
const SCTLR_EL1: u64 = (1 << 0) // M
    | (1 << 2) // C
    | (1 << 3) // SA
    | (1 << 4) // SA0
    | (1 << 12) // I
    | (1 << 14) // DZE
    | (1 << 15) // UCT
    | (1 << 16) // nTWI
    | (1 << 18) // nTWE
    | (1 << 26) // UCI
    | (1 << 11) | (1 << 20) | (1 << 22) | (1 << 23) | (1 << 28) | (1 << 29);

/// CPACR_EL1: FP and SIMD at EL0 and EL1 without traps (FPEN = 0b11).
const CPACR_EL1: u64 = 0b11 << 20;

/// The system registers a vCPU starts with.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub struct SystemRegisters {
    pub ttbr0_el1: u64,
    pub tcr_el1: u64,
    pub mair_el1: u64,
    pub sctlr_el1: u64,
    pub cpacr_el1: u64,
    pub vbar_el1: u64,
}

impl SystemRegisters {
    /// For `tables`, with the guest's EL1 exception vectors at `vectors`.
    pub fn new(tables: &PageTables, vectors: GuestPtr) -> Result<SystemRegisters, PageTableError> {
        if !vectors.0.is_multiple_of(0x800) {
            return Err(PageTableError::VectorsMisaligned(vectors.0));
        }
        Ok(SystemRegisters {
            ttbr0_el1: u64::from(tables.base.0),
            tcr_el1: TCR_EL1,
            mair_el1: MAIR_EL1,
            sctlr_el1: SCTLR_EL1,
            cpacr_el1: CPACR_EL1,
            vbar_el1: u64::from(vectors.0),
        })
    }
}

#[cfg(test)]
mod tests {
    use super::descriptor::*;
    use super::*;
    use crate::plan::{IMAGE_BASE, RUNTIME_BASE, WINDOW_BASE, WINDOW_SIZE};

    const TABLES: GuestPtr = GuestPtr(RUNTIME_BASE + 0x1000);

    fn range(start: u32, size: u64) -> GuestRange {
        GuestRange::new(GuestPtr(start), size).expect("a valid range")
    }

    /// The image as the build links it (llvm-readelf -l build/macos/halo_guest.elf):
    /// the header, code, read-only data and data with bss.
    fn segments() -> Vec<SegmentAccess> {
        let segment = |start, size, writable, executable| SegmentAccess {
            range: range(start, size),
            writable,
            executable,
        };
        vec![
            segment(0x8800_0000, 0x30, false, false),
            segment(0x8800_0030, 0x40_45c0, false, true),
            segment(0x8840_45f0, 0xaf_206a, false, false),
            segment(0x88f0_0000, 0x196_bc64, true, false),
        ]
    }

    fn tables() -> PageTables {
        PageTables::build(&AddressSpacePlan::standard(), &segments(), TABLES)
            .expect("the tables build")
    }

    fn at(tables: &PageTables, address: u32) -> Translation {
        tables
            .translate(GuestPtr(address))
            .unwrap_or_else(|| panic!("{address:#x} is mapped"))
    }

    #[test]
    fn every_mapped_address_is_its_own_physical_address() {
        let tables = tables();
        for address in [
            WINDOW_BASE,
            WINDOW_BASE + 0x1234,
            0x87ff_fffc,
            IMAGE_BASE,
            0x8800_0040,
            0x1000_5678,
            0xdfff_fffc,
        ] {
            assert_eq!(
                at(&tables, address).physical,
                u64::from(address),
                "{address:#x}"
            );
        }
    }

    #[test]
    fn what_the_plan_leaves_out_is_unmapped() {
        let tables = tables();
        // the null page, the gap above the runtime, above the last pool, and
        // the image's range past its bss
        for address in [
            0,
            0x1000,
            0x1f_ffff,
            0x40_0000,
            0x0fff_ffff,
            0xe000_0000,
            0xffff_fffc,
            0x8a87_0000,
            0x8bff_f000,
        ] {
            assert_eq!(tables.translate(GuestPtr(address)), None, "{address:#x}");
        }
    }

    #[test]
    fn the_window_and_pools_are_guest_data_in_blocks() {
        let tables = tables();
        for address in [
            WINDOW_BASE,
            (WINDOW_BASE + WINDOW_SIZE as u32) - 1,
            0x1000_0000,
            0xd000_0000,
        ] {
            let translation = at(&tables, address);
            assert_eq!(translation.level, 2, "{address:#x}");
            assert_eq!(translation.access(), AP_ALL_RW);
            assert!(!translation.guest_executable() && !translation.el1_executable());
        }
    }

    #[test]
    fn the_runtime_is_for_el1_alone() {
        let translation = at(&tables(), RUNTIME_BASE);
        assert_eq!(translation.access(), AP_EL1_RO);
        assert!(translation.el1_executable() && !translation.guest_executable());
    }

    #[test]
    fn only_the_images_code_is_executable() {
        let tables = tables();
        // the first page holds the header and the start of the code
        for address in [IMAGE_BASE, 0x8820_0000, 0x8840_4000] {
            let translation = at(&tables, address);
            assert_eq!(translation.level, 3);
            assert_eq!(translation.access(), AP_ALL_RO, "{address:#x}");
            assert!(
                translation.guest_executable() && !translation.el1_executable(),
                "{address:#x}"
            );
        }
        let rodata = at(&tables, 0x8850_0000);
        assert_eq!(rodata.access(), AP_ALL_RO);
        assert!(!rodata.guest_executable());
        for address in [0x88f0_0000, 0x8a86_b000] {
            let data = at(&tables, address);
            assert_eq!(data.access(), AP_ALL_RW, "{address:#x}");
            assert!(!data.guest_executable() && !data.el1_executable());
        }
    }

    #[test]
    fn every_descriptor_is_normal_shareable_memory_with_its_access_flag() {
        let tables = tables();
        for address in [
            RUNTIME_BASE,
            WINDOW_BASE,
            IMAGE_BASE,
            0x88f0_0000,
            0x9000_0000,
        ] {
            let d = at(&tables, address).descriptor;
            assert_eq!((d >> ATTR_INDEX_SHIFT) & 0b111, 0, "{address:#x}");
            assert_eq!(d & (0b11 << 8), INNER_SHAREABLE);
            assert_ne!(d & ACCESS_FLAG, 0);
        }
    }

    #[test]
    fn the_tables_are_laid_out_level_by_level() {
        let tables = tables();
        assert_eq!(tables.base(), TABLES);
        let bytes = tables.to_bytes();
        assert_eq!(bytes.len() as u64, tables.table_count() as u64 * TABLE_SIZE);
        let level1: Vec<u64> = bytes[..TABLE_SIZE as usize]
            .chunks(8)
            .map(|word| u64::from_le_bytes(word.try_into().expect("8 bytes")))
            .collect();
        // all four gigabytes hold a region, each through a level 2 table
        for (index, entry) in level1.iter().enumerate().take(4) {
            assert_eq!(entry & 0b11, 0b11, "entry {index}");
            assert_eq!(
                entry & ADDRESS_MASK,
                u64::from(TABLES.0) + TABLE_SIZE * (index as u64 + 1)
            );
        }
        assert!(level1[4..].iter().all(|&entry| entry == 0));
        // 1 + 4 level 2 tables + a level 3 table for each 2 MB of the image
        // its segments touch (0x88000000-0x8a86bc64: 21 of them)
        assert_eq!(tables.table_count(), 1 + 4 + 21);
    }

    #[test]
    fn writable_code_is_refused() {
        let plan = AddressSpacePlan::standard();
        let mut both = segments();
        both[1].writable = true;
        assert_eq!(
            PageTables::build(&plan, &both, TABLES).expect_err("W+X"),
            PageTableError::WritableCode(IMAGE_BASE)
        );
        // a writable segment that shares a page with code
        let mut sharing = segments();
        sharing[3].range = range(0x8840_4000, 0x1000);
        assert!(matches!(
            PageTables::build(&plan, &sharing, TABLES),
            Err(PageTableError::WritableCode(0x8840_4000))
        ));
    }

    #[test]
    fn segments_and_tables_must_be_where_they_belong() {
        let plan = AddressSpacePlan::standard();
        let mut outside = segments();
        outside[3].range = range(0x8bff_f000, 0x2000);
        assert_eq!(
            PageTables::build(&plan, &outside, TABLES).expect_err("outside"),
            PageTableError::SegmentOutsideImage(0x8bff_f000)
        );
        assert_eq!(
            PageTables::build(&plan, &segments(), GuestPtr(RUNTIME_BASE + 0x10))
                .expect_err("misaligned"),
            PageTableError::TablesMisplaced(RUNTIME_BASE + 0x10)
        );
        assert_eq!(
            PageTables::build(&plan, &segments(), GuestPtr(WINDOW_BASE))
                .expect_err("not in the runtime"),
            PageTableError::TablesMisplaced(WINDOW_BASE)
        );
        // too near the runtime's end for 26 tables
        assert_eq!(
            PageTables::build(&plan, &segments(), GuestPtr(RUNTIME_BASE + 0x1f_0000))
                .expect_err("too large"),
            PageTableError::TablesTooLarge(26)
        );
    }

    #[test]
    fn the_system_registers_turn_on_this_translation() {
        let tables = tables();
        let registers =
            SystemRegisters::new(&tables, GuestPtr(RUNTIME_BASE)).expect("aligned vectors");
        assert_eq!(registers.ttbr0_el1, u64::from(TABLES.0));
        assert_eq!(registers.vbar_el1, u64::from(RUNTIME_BASE));
        assert_eq!(registers.mair_el1, MAIR_EL1);
        let tcr = registers.tcr_el1;
        assert_eq!(tcr & 0x3f, 32, "T0SZ: 32-bit addresses");
        assert_eq!((tcr >> 8) & 0b11, 0b01, "IRGN0: write-back");
        assert_eq!((tcr >> 10) & 0b11, 0b01, "ORGN0: write-back");
        assert_eq!((tcr >> 12) & 0b11, 0b11, "SH0: inner shareable");
        assert_eq!((tcr >> 14) & 0b11, 0b00, "TG0: 4 KB");
        assert_ne!(tcr & (1 << 23), 0, "EPD1: no TTBR1 walks");
        assert_eq!((tcr >> 32) & 0b111, 0b000, "IPS: 32-bit physical addresses");
        let sctlr = registers.sctlr_el1;
        for (bit, name) in [
            (0, "M"),
            (2, "C"),
            (12, "I"),
            (16, "nTWI"),
            (18, "nTWE"),
            (15, "UCT"),
            (26, "UCI"),
        ] {
            assert_ne!(sctlr & (1 << bit), 0, "SCTLR_EL1.{name}");
        }
        // ARMv8.0's RES1 bits
        for bit in [11, 20, 22, 23, 28, 29] {
            assert_ne!(sctlr & (1 << bit), 0, "RES1 bit {bit}");
        }
        assert_eq!(sctlr & (1 << 19), 0, "WXN off: the tables decide");
        assert_eq!(sctlr & (1 << 25), 0, "EE: little-endian");
        assert_eq!(
            (registers.cpacr_el1 >> 20) & 0b11,
            0b11,
            "FPEN: no FP/SIMD traps"
        );
        assert_eq!(
            SystemRegisters::new(&tables, GuestPtr(RUNTIME_BASE + 0x400)).expect_err("2 KB"),
            PageTableError::VectorsMisaligned(RUNTIME_BASE + 0x400)
        );
    }

    #[test]
    fn a_plan_without_the_image_or_runtime_is_refused() {
        let plan = AddressSpacePlan::new(
            AddressSpacePlan::standard()
                .regions()
                .iter()
                .copied()
                .filter(|r| r.kind != RegionKind::Image),
        )
        .expect("still a valid plan");
        assert_eq!(
            PageTables::build(&plan, &segments(), TABLES).expect_err("no image"),
            PageTableError::MissingRegion(RegionKind::Image)
        );
    }
}
