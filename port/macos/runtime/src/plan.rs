//! The guest's address space: where each kind of memory lives.
//!
//! On Android the host finds room below 4 GB among the process's other
//! mappings (port/android/host/host_memory.c). In the VM the whole space is
//! the guest's, so every range is fixed: the Xbox memory window and the image
//! where the game was linked to find them, pools of pages for everything else
//! (the C library's heap, thread stacks, mappings), and a small range for the
//! guest's EL1 runtime (its exception vectors and page tables). Nothing is
//! mapped in the first 2 MB, so that a null pointer faults.
//!
//! Every range is aligned to 2 MB, so that the page tables can map it with
//! 2 MB blocks (crate::page_tables).

use std::fmt;

use crate::memory::{GuestPtr, GuestRange};

/// The alignment of every range.
pub const REGION_ALIGNMENT: u64 = 0x20_0000;
/// Nothing is mapped below this address.
pub const NULL_GUARD: u32 = 0x20_0000;
/// The guest's pages: the pools are handed out in these.
pub const GUEST_PAGE: u64 = 0x1000;

/// The guest's EL1 runtime: exception vectors and page tables.
pub const RUNTIME_BASE: u32 = 0x0020_0000;
pub const RUNTIME_SIZE: u64 = 0x0020_0000;
/// The Xbox contiguous memory window (halo_android_abi.h's HALO_GUEST_WINDOW_BASE).
pub const WINDOW_BASE: u32 = 0x8000_0000;
pub const WINDOW_SIZE: u64 = 0x0800_0000;
/// The guest image's range (HALO_GUEST_IMAGE_BASE, HALO_GUEST_IMAGE_RESERVE).
pub const IMAGE_BASE: u32 = 0x8800_0000;
pub const IMAGE_SIZE: u64 = 0x0400_0000;
/// The pools (as host_memory.c's POOL_SIZE and MAXIMUM_POOLS).
pub const POOL_SIZE: u64 = 0x1000_0000;
pub const POOL_COUNT: u8 = 12;

/// What a range of the address space is for.
#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, Debug)]
pub enum RegionKind {
    /// the EL1 runtime, which the guest's own code (EL0) cannot reach
    Runtime,
    /// the Xbox memory window
    Window,
    /// the guest image: its code, data and bss
    Image,
    /// a pool of pages, numbered from 0
    Pool(u8),
}

impl fmt::Display for RegionKind {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            RegionKind::Runtime => f.write_str("the EL1 runtime"),
            RegionKind::Window => f.write_str("the Xbox window"),
            RegionKind::Image => f.write_str("the image"),
            RegionKind::Pool(index) => write!(f, "pool {index}"),
        }
    }
}

/// One range of the plan.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub struct Region {
    pub kind: RegionKind,
    pub range: GuestRange,
}

impl Region {
    /// Whether the guest's code (EL0) may use the range.
    pub fn guest_accessible(&self) -> bool {
        todo!()
    }
}

/// Why a plan or an allocation is refused.
#[derive(Debug, PartialEq, Eq, thiserror::Error)]
pub enum PlanError {
    #[error("{first} and {second} overlap")]
    Overlap { first: RegionKind, second: RegionKind },
    #[error("{0} appears twice")]
    Duplicate(RegionKind),
    #[error("{0} is not aligned to 2 MB")]
    Misaligned(RegionKind),
    #[error("{0} reaches into the first 2 MB, which stays unmapped")]
    NullPage(RegionKind),
    #[error("no pool has {0:#x} free bytes in a row")]
    NoRoom(u64),
    #[error("an empty allocation")]
    Empty,
    #[error("{size:#x} bytes at {address:#010x} are not memory the pools handed out")]
    NotAllocated { address: u32, size: u64 },
}

/// The guest's address space: regions that do not overlap, in address order.
#[derive(Clone, Debug)]
pub struct AddressSpacePlan {
    regions: Vec<Region>,
}

impl AddressSpacePlan {
    /// A plan of these regions, checked.
    pub fn new(regions: impl IntoIterator<Item = Region>) -> Result<AddressSpacePlan, PlanError> {
        todo!("{}", regions.into_iter().count())
    }

    /// The plan the macOS host uses: the table in this module's description.
    pub fn standard() -> AddressSpacePlan {
        todo!()
    }

    /// The regions, in address order.
    pub fn regions(&self) -> &[Region] {
        todo!()
    }

    pub fn region(&self, kind: RegionKind) -> Option<&Region> {
        todo!("{kind}")
    }

    /// The region that holds all of `range`, if one does.
    pub fn region_containing(&self, range: GuestRange) -> Option<&Region> {
        todo!("{range:?}")
    }

    /// The pools, in address order.
    pub fn pools(&self) -> impl Iterator<Item = &Region> {
        todo!();
        #[allow(unreachable_code)]
        self.regions.iter()
    }
}

/// Hands out the pools' pages, as host_memory.c's host_low_map does: the
/// first run of free pages that is long enough, in the first pool that has
/// one.
pub struct PoolAllocator {
    pools: Vec<Pool>,
}

struct Pool {
    range: GuestRange,
    /// one bit for each page, set when it is handed out
    used: Vec<u64>,
}

impl PoolAllocator {
    pub fn new(plan: &AddressSpacePlan) -> PoolAllocator {
        todo!("{}", plan.regions.len())
    }

    /// `size` bytes (rounded up to whole pages) of free pool memory.
    pub fn allocate(&mut self, size: u64) -> Result<GuestRange, PlanError> {
        todo!("{size}")
    }

    /// Gives back the pages of `range` (rounded out to whole pages); the
    /// range must lie in one pool. Pages already free stay free.
    pub fn free(&mut self, range: GuestRange) -> Result<(), PlanError> {
        todo!("{range:?}")
    }

    /// Whether all of `range` was handed out and not given back.
    pub fn owns(&self, range: GuestRange) -> bool {
        todo!("{range:?}")
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::memory::GUEST_SPACE;

    fn range(start: u32, size: u64) -> GuestRange {
        GuestRange::new(GuestPtr(start), size).expect("a valid range")
    }

    fn region(kind: RegionKind, start: u32, size: u64) -> Region {
        Region {
            kind,
            range: range(start, size),
        }
    }

    #[test]
    fn the_standard_plan_puts_the_game_where_it_was_linked() {
        let plan = AddressSpacePlan::standard();
        let window = plan.region(RegionKind::Window).expect("a window");
        assert_eq!((window.range.start(), window.range.size()), (GuestPtr(0x8000_0000), 0x0800_0000));
        let image = plan.region(RegionKind::Image).expect("an image range");
        assert_eq!((image.range.start(), image.range.end()), (GuestPtr(0x8800_0000), 0x8c00_0000));
        let runtime = plan.region(RegionKind::Runtime).expect("a runtime range");
        assert!(!runtime.guest_accessible());
        assert!(window.guest_accessible() && image.guest_accessible());
    }

    #[test]
    fn the_standard_plan_has_twelve_pools_of_256_mb() {
        let plan = AddressSpacePlan::standard();
        let pools: Vec<&Region> = plan.pools().collect();
        assert_eq!(pools.len(), 12);
        for (index, pool) in pools.iter().enumerate() {
            assert_eq!(pool.kind, RegionKind::Pool(index as u8));
            assert_eq!(pool.range.size(), POOL_SIZE);
            assert_eq!(u64::from(pool.range.start().0) % POOL_SIZE, 0);
            assert!(pool.guest_accessible());
        }
    }

    #[test]
    fn the_standard_plan_is_ordered_aligned_and_clear_of_the_null_page() {
        let plan = AddressSpacePlan::standard();
        let regions = plan.regions();
        assert_eq!(regions.len(), 15);
        for pair in regions.windows(2) {
            assert!(pair[0].range.end() <= u64::from(pair[1].range.start().0), "{pair:?}");
        }
        for region in regions {
            assert_eq!(u64::from(region.range.start().0) % REGION_ALIGNMENT, 0);
            assert_eq!(region.range.size() % REGION_ALIGNMENT, 0);
            assert!(region.range.start().0 >= NULL_GUARD);
            assert!(region.range.end() <= GUEST_SPACE);
        }
        // it is the plan new() accepts
        assert!(AddressSpacePlan::new(regions.iter().copied()).is_ok());
    }

    #[test]
    fn a_range_already_taken_is_refused() {
        let error = AddressSpacePlan::new([
            region(RegionKind::Window, 0x8000_0000, 0x0800_0000),
            region(RegionKind::Pool(0), 0x8600_0000, 0x0400_0000),
        ])
        .expect_err("they overlap");
        assert_eq!(
            error,
            PlanError::Overlap {
                first: RegionKind::Pool(0),
                second: RegionKind::Window
            }
        );
        assert_eq!(error.to_string(), "pool 0 and the Xbox window overlap");
    }

    #[test]
    fn plans_refuse_duplicates_misalignment_and_the_null_page() {
        assert_eq!(
            AddressSpacePlan::new([
                region(RegionKind::Window, 0x8000_0000, 0x0020_0000),
                region(RegionKind::Window, 0x9000_0000, 0x0020_0000),
            ])
            .expect_err("two windows"),
            PlanError::Duplicate(RegionKind::Window)
        );
        assert_eq!(
            AddressSpacePlan::new([region(RegionKind::Image, 0x8800_1000, 0x0020_0000)]).expect_err("misaligned"),
            PlanError::Misaligned(RegionKind::Image)
        );
        assert_eq!(
            AddressSpacePlan::new([region(RegionKind::Pool(0), 0x1000_0000, 0x1000)]).expect_err("misaligned size"),
            PlanError::Misaligned(RegionKind::Pool(0))
        );
        assert_eq!(
            AddressSpacePlan::new([region(RegionKind::Runtime, 0, 0x0020_0000)]).expect_err("at 0"),
            PlanError::NullPage(RegionKind::Runtime)
        );
    }

    #[test]
    fn ranges_are_found_in_the_region_that_holds_them() {
        let plan = AddressSpacePlan::standard();
        let pool = plan.pools().nth(3).expect("a fourth pool");
        let inside = range(pool.range.start().0 + 0x1000, 0x2000);
        assert_eq!(plan.region_containing(inside).map(|r| r.kind), Some(RegionKind::Pool(3)));
        assert_eq!(plan.region_containing(range(0x8800_0000, 0x10)).map(|r| r.kind), Some(RegionKind::Image));
        // in no region, or across two
        assert!(plan.region_containing(range(0x0000_1000, 0x10)).is_none());
        assert!(plan.region_containing(range(0x87ff_fff0, 0x20)).is_none());
    }

    #[test]
    fn the_pools_hand_out_free_pages_in_order() {
        let plan = AddressSpacePlan::standard();
        let first_pool = plan.pools().next().expect("a pool").range.start().0;
        let mut pools = PoolAllocator::new(&plan);
        let a = pools.allocate(1).expect("room");
        assert_eq!((a.start(), a.size()), (GuestPtr(first_pool), GUEST_PAGE));
        let b = pools.allocate(GUEST_PAGE + 1).expect("room");
        assert_eq!((b.start(), b.size()), (GuestPtr(first_pool + 0x1000), 2 * GUEST_PAGE));
        assert!(pools.owns(a) && pools.owns(b));
        pools.free(a).expect("handed out");
        assert!(!pools.owns(a));
        // the freed page is the first that is free again
        assert_eq!(pools.allocate(GUEST_PAGE).expect("room").start(), GuestPtr(first_pool));
        assert_eq!(pools.allocate(0), Err(PlanError::Empty));
    }

    #[test]
    fn a_full_pool_spills_into_the_next() {
        let plan = AddressSpacePlan::standard();
        let starts: Vec<u32> = plan.pools().map(|pool| pool.range.start().0).collect();
        let mut pools = PoolAllocator::new(&plan);
        let whole = pools.allocate(POOL_SIZE).expect("a whole pool");
        assert_eq!(whole.start(), GuestPtr(starts[0]));
        assert_eq!(pools.allocate(GUEST_PAGE).expect("room").start(), GuestPtr(starts[1]));
        assert_eq!(pools.allocate(POOL_SIZE + GUEST_PAGE), Err(PlanError::NoRoom(POOL_SIZE + GUEST_PAGE)));
    }

    #[test]
    fn only_pool_memory_can_be_given_back() {
        let plan = AddressSpacePlan::standard();
        let mut pools = PoolAllocator::new(&plan);
        let window = range(WINDOW_BASE, GUEST_PAGE);
        assert_eq!(
            pools.free(window),
            Err(PlanError::NotAllocated {
                address: WINDOW_BASE,
                size: GUEST_PAGE
            })
        );
        // across the end of one pool and the start of the next
        let starts: Vec<u32> = plan.pools().map(|pool| pool.range.start().0).collect();
        assert!(pools.free(range(starts[1] - 0x1000, 0x2000)).is_err());
        // pages never handed out may be freed: they stay free
        assert!(pools.free(range(starts[0], GUEST_PAGE)).is_ok());
        assert!(!pools.owns(range(starts[0], GUEST_PAGE)));
    }
}
