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

use crate::memory::{GuestPtr, GuestRange};
use crate::plan::{AddressSpacePlan, RegionKind, GUEST_PAGE};

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
    #[error("the tables must start at a 4 KB-aligned address in the EL1 runtime's range, not {0:#010x}")]
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
    pub fn build(plan: &AddressSpacePlan, segments: &[SegmentAccess], base: GuestPtr) -> Result<PageTables, PageTableError> {
        todo!("{} {} {base:?}", plan.regions().len(), segments.len())
    }

    /// Where the level 1 table starts: TTBR0_EL1.
    pub fn base(&self) -> GuestPtr {
        todo!()
    }

    pub fn table_count(&self) -> usize {
        todo!()
    }

    /// The tables' bytes, little-endian, as they go into guest memory.
    pub fn to_bytes(&self) -> Vec<u8> {
        todo!()
    }

    /// Walks the tables as the MMU would.
    pub fn translate(&self, address: GuestPtr) -> Option<Translation> {
        todo!("{address:?}")
    }
}

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
        todo!("{} {vectors:?}", tables.descriptors.len())
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
        PageTables::build(&AddressSpacePlan::standard(), &segments(), TABLES).expect("the tables build")
    }

    fn at(tables: &PageTables, address: u32) -> Translation {
        tables.translate(GuestPtr(address)).unwrap_or_else(|| panic!("{address:#x} is mapped"))
    }

    #[test]
    fn every_mapped_address_is_its_own_physical_address() {
        let tables = tables();
        for address in [WINDOW_BASE, WINDOW_BASE + 0x1234, 0x87ff_fffc, IMAGE_BASE, 0x8800_0040, 0x10_0000_00 + 0x5678, 0xdfff_fffc] {
            assert_eq!(at(&tables, address).physical, u64::from(address), "{address:#x}");
        }
    }

    #[test]
    fn what_the_plan_leaves_out_is_unmapped() {
        let tables = tables();
        // the null page, the gap above the runtime, above the last pool, and
        // the image's range past its bss
        for address in [0, 0x1000, 0x1f_ffff, 0x40_0000, 0x0fff_ffff, 0xe000_0000, 0xffff_fffc, 0x8a87_0000, 0x8bff_f000] {
            assert_eq!(tables.translate(GuestPtr(address)), None, "{address:#x}");
        }
    }

    #[test]
    fn the_window_and_pools_are_guest_data_in_blocks() {
        let tables = tables();
        for address in [WINDOW_BASE, (WINDOW_BASE + WINDOW_SIZE as u32) - 1, 0x1000_0000, 0xd000_0000] {
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
            assert!(translation.guest_executable() && !translation.el1_executable(), "{address:#x}");
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
        for address in [RUNTIME_BASE, WINDOW_BASE, IMAGE_BASE, 0x88f0_0000, 0x9000_0000] {
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
            assert_eq!(entry & ADDRESS_MASK, u64::from(TABLES.0) + TABLE_SIZE * (index as u64 + 1));
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
        assert_eq!(PageTables::build(&plan, &both, TABLES).expect_err("W+X"), PageTableError::WritableCode(IMAGE_BASE));
        // a writable segment that shares a page with code
        let mut sharing = segments();
        sharing[3].range = range(0x8840_4000, 0x1000);
        assert!(matches!(PageTables::build(&plan, &sharing, TABLES), Err(PageTableError::WritableCode(0x8840_4000))));
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
            PageTables::build(&plan, &segments(), GuestPtr(RUNTIME_BASE + 0x10)).expect_err("misaligned"),
            PageTableError::TablesMisplaced(RUNTIME_BASE + 0x10)
        );
        assert_eq!(
            PageTables::build(&plan, &segments(), GuestPtr(WINDOW_BASE)).expect_err("not in the runtime"),
            PageTableError::TablesMisplaced(WINDOW_BASE)
        );
        // too near the runtime's end for 26 tables
        assert_eq!(
            PageTables::build(&plan, &segments(), GuestPtr(RUNTIME_BASE + 0x1f_0000)).expect_err("too large"),
            PageTableError::TablesTooLarge(26)
        );
    }

    #[test]
    fn the_system_registers_turn_on_this_translation() {
        let tables = tables();
        let registers = SystemRegisters::new(&tables, GuestPtr(RUNTIME_BASE)).expect("aligned vectors");
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
        for (bit, name) in [(0, "M"), (2, "C"), (12, "I"), (16, "nTWI"), (18, "nTWE"), (15, "UCT"), (26, "UCI")] {
            assert_ne!(sctlr & (1 << bit), 0, "SCTLR_EL1.{name}");
        }
        // ARMv8.0's RES1 bits
        for bit in [11, 20, 22, 23, 28, 29] {
            assert_ne!(sctlr & (1 << bit), 0, "RES1 bit {bit}");
        }
        assert_eq!(sctlr & (1 << 19), 0, "WXN off: the tables decide");
        assert_eq!(sctlr & (1 << 25), 0, "EE: little-endian");
        assert_eq!((registers.cpacr_el1 >> 20) & 0b11, 0b11, "FPEN: no FP/SIMD traps");
        assert_eq!(
            SystemRegisters::new(&tables, GuestPtr(RUNTIME_BASE + 0x400)).expect_err("2 KB"),
            PageTableError::VectorsMisaligned(RUNTIME_BASE + 0x400)
        );
    }

    #[test]
    fn a_plan_without_the_image_or_runtime_is_refused() {
        let plan = AddressSpacePlan::new(AddressSpacePlan::standard().regions().iter().copied().filter(|r| r.kind != RegionKind::Image))
            .expect("still a valid plan");
        assert_eq!(
            PageTables::build(&plan, &segments(), TABLES).expect_err("no image"),
            PageTableError::MissingRegion(RegionKind::Image)
        );
    }
}
