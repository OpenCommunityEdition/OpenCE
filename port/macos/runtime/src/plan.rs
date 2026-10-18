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
        self.kind != RegionKind::Runtime
    }
}

/// Why a plan or an allocation is refused.
#[derive(Debug, PartialEq, Eq, thiserror::Error)]
pub enum PlanError {
    #[error("{first} and {second} overlap")]
    Overlap {
        first: RegionKind,
        second: RegionKind,
    },
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
    /// Each region is checked in turn; one that overlaps a region before it
    /// names itself first and the region it overlaps second.
    pub fn new(regions: impl IntoIterator<Item = Region>) -> Result<AddressSpacePlan, PlanError> {
        let mut sorted: Vec<Region> = Vec::new();
        for region in regions {
            let start = u64::from(region.range.start().0);
            if start % REGION_ALIGNMENT != 0 || region.range.size() % REGION_ALIGNMENT != 0 {
                return Err(PlanError::Misaligned(region.kind));
            }
            if region.range.start().0 < NULL_GUARD {
                return Err(PlanError::NullPage(region.kind));
            }
            if sorted.iter().any(|other| other.kind == region.kind) {
                return Err(PlanError::Duplicate(region.kind));
            }
            if let Some(other) = sorted.iter().find(|other| {
                start < other.range.end() && u64::from(other.range.start().0) < region.range.end()
            }) {
                return Err(PlanError::Overlap {
                    first: region.kind,
                    second: other.kind,
                });
            }
            let at = sorted.partition_point(|other| other.range.start() < region.range.start());
            sorted.insert(at, region);
        }
        Ok(AddressSpacePlan { regions: sorted })
    }

    /// The plan the macOS host uses: the table in this module's description.
    pub fn standard() -> AddressSpacePlan {
        let fixed = |kind, start, size| Region {
            kind,
            range: GuestRange::new(GuestPtr(start), size).expect("a fixed range lies below 4 GB"),
        };
        let mut regions = vec![
            fixed(RegionKind::Runtime, RUNTIME_BASE, RUNTIME_SIZE),
            fixed(RegionKind::Window, WINDOW_BASE, WINDOW_SIZE),
            fixed(RegionKind::Image, IMAGE_BASE, IMAGE_SIZE),
        ];
        // seven pools below the window and five above the image
        for index in 0..POOL_COUNT {
            let start = if index < 7 {
                0x1000_0000 * (u32::from(index) + 1)
            } else {
                0x9000_0000 + 0x1000_0000 * (u32::from(index) - 7)
            };
            regions.push(fixed(RegionKind::Pool(index), start, POOL_SIZE));
        }
        AddressSpacePlan::new(regions).expect("the standard plan is valid (its tests check it)")
    }

    /// The regions, in address order.
    pub fn regions(&self) -> &[Region] {
        &self.regions
    }

    pub fn region(&self, kind: RegionKind) -> Option<&Region> {
        self.regions.iter().find(|region| region.kind == kind)
    }

    /// The region that holds all of `range`, if one does.
    pub fn region_containing(&self, range: GuestRange) -> Option<&Region> {
        self.regions.iter().find(|region| {
            region.range.start() <= range.start() && range.end() <= region.range.end()
        })
    }

    /// The pools, in address order.
    pub fn pools(&self) -> impl Iterator<Item = &Region> {
        self.regions
            .iter()
            .filter(|region| matches!(region.kind, RegionKind::Pool(_)))
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
        let pools = plan
            .pools()
            .map(|pool| {
                let pages = pool.range.size() / GUEST_PAGE;
                Pool {
                    range: pool.range,
                    used: vec![0; pages.div_ceil(64) as usize],
                }
            })
            .collect();
        PoolAllocator { pools }
    }

    /// `size` bytes (rounded up to whole pages) of free pool memory.
    pub fn allocate(&mut self, size: u64) -> Result<GuestRange, PlanError> {
        if size == 0 {
            return Err(PlanError::Empty);
        }
        let pages = size.div_ceil(GUEST_PAGE);
        for pool in &mut self.pools {
            if let Some(first) = pool.free_run(pages) {
                pool.set(first, pages, true);
                let start = u64::from(pool.range.start().0) + first * GUEST_PAGE;
                // (inside the pool, so below 4 GB)
                return GuestRange::new(GuestPtr(start as u32), pages * GUEST_PAGE)
                    .map_err(|_| PlanError::NoRoom(size));
            }
        }
        Err(PlanError::NoRoom(size))
    }

    /// Gives back the pages of `range` (rounded out to whole pages); the
    /// range must lie in one pool. Pages already free stay free.
    pub fn free(&mut self, range: GuestRange) -> Result<(), PlanError> {
        let not_allocated = PlanError::NotAllocated {
            address: range.start().0,
            size: range.size(),
        };
        let (pool, first, pages) = self.pages_of(range).ok_or(not_allocated)?;
        self.pools[pool].set(first, pages, false);
        Ok(())
    }

    /// Whether all of `range` was handed out and not given back.
    pub fn owns(&self, range: GuestRange) -> bool {
        self.pages_of(range).is_some_and(|(pool, first, pages)| {
            (first..first + pages).all(|page| self.pools[pool].is_used(page))
        })
    }

    /// The pool and its pages (first, count) that hold `range`, rounded out
    /// to whole pages.
    fn pages_of(&self, range: GuestRange) -> Option<(usize, u64, u64)> {
        let start = u64::from(range.start().0) / GUEST_PAGE * GUEST_PAGE;
        let end = range.end().div_ceil(GUEST_PAGE) * GUEST_PAGE;
        self.pools
            .iter()
            .position(|pool| u64::from(pool.range.start().0) <= start && end <= pool.range.end())
            .map(|index| {
                let first = (start - u64::from(self.pools[index].range.start().0)) / GUEST_PAGE;
                (index, first, (end - start) / GUEST_PAGE)
            })
    }
}

impl Pool {
    fn pages(&self) -> u64 {
        self.range.size() / GUEST_PAGE
    }

    fn is_used(&self, page: u64) -> bool {
        self.used[(page / 64) as usize] & (1 << (page % 64)) != 0
    }

    fn set(&mut self, first: u64, count: u64, used: bool) {
        for page in first..first + count {
            let (word, bit) = ((page / 64) as usize, 1u64 << (page % 64));
            if used {
                self.used[word] |= bit;
            } else {
                self.used[word] &= !bit;
            }
        }
    }

    /// The first page of the first run of `count` free pages.
    fn free_run(&self, count: u64) -> Option<u64> {
        let mut run = 0;
        for page in 0..self.pages() {
            if self.is_used(page) {
                run = 0;
                continue;
            }
            run += 1;
            if run == count {
                return Some(page + 1 - count);
            }
        }
        None
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
        assert_eq!(
            (window.range.start(), window.range.size()),
            (GuestPtr(0x8000_0000), 0x0800_0000)
        );
        let image = plan.region(RegionKind::Image).expect("an image range");
        assert_eq!(
            (image.range.start(), image.range.end()),
            (GuestPtr(0x8800_0000), 0x8c00_0000)
        );
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
            assert!(
                pair[0].range.end() <= u64::from(pair[1].range.start().0),
                "{pair:?}"
            );
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
            AddressSpacePlan::new([region(RegionKind::Image, 0x8800_1000, 0x0020_0000)])
                .expect_err("misaligned"),
            PlanError::Misaligned(RegionKind::Image)
        );
        assert_eq!(
            AddressSpacePlan::new([region(RegionKind::Pool(0), 0x1000_0000, 0x1000)])
                .expect_err("misaligned size"),
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
        assert_eq!(
            plan.region_containing(inside).map(|r| r.kind),
            Some(RegionKind::Pool(3))
        );
        assert_eq!(
            plan.region_containing(range(0x8800_0000, 0x10))
                .map(|r| r.kind),
            Some(RegionKind::Image)
        );
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
        assert_eq!(
            (b.start(), b.size()),
            (GuestPtr(first_pool + 0x1000), 2 * GUEST_PAGE)
        );
        assert!(pools.owns(a) && pools.owns(b));
        pools.free(a).expect("handed out");
        assert!(!pools.owns(a));
        // the freed page is the first that is free again
        assert_eq!(
            pools.allocate(GUEST_PAGE).expect("room").start(),
            GuestPtr(first_pool)
        );
        assert_eq!(pools.allocate(0), Err(PlanError::Empty));
    }

    #[test]
    fn a_full_pool_spills_into_the_next() {
        let plan = AddressSpacePlan::standard();
        let starts: Vec<u32> = plan.pools().map(|pool| pool.range.start().0).collect();
        let mut pools = PoolAllocator::new(&plan);
        let whole = pools.allocate(POOL_SIZE).expect("a whole pool");
        assert_eq!(whole.start(), GuestPtr(starts[0]));
        assert_eq!(
            pools.allocate(GUEST_PAGE).expect("room").start(),
            GuestPtr(starts[1])
        );
        assert_eq!(
            pools.allocate(POOL_SIZE + GUEST_PAGE),
            Err(PlanError::NoRoom(POOL_SIZE + GUEST_PAGE))
        );
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
