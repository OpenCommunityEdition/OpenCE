//! The pure-logic pieces of the macOS host, which runs the game's ILP32 guest
//! image in a VM under Hypervisor.framework (docs/macos_host.md).
//!
//! - [`memory`]: guest addresses ([`GuestPtr`]) and the host memory that
//!   backs the guest's 4 GB address space.
//! - [`plan`]: where each kind of memory lives in that space, and the
//!   allocator of its pools.
//! - [`page_tables`]: the guest's translation tables and the system
//!   registers its vCPUs start with.
//! - [`image`]: the guest image's checks, and its loading into guest memory.
//!
//! The C host (port/macos/host) calls this crate through its C ABI
//! ([`ffi`]); the Swift app (port/macos/app) through UniFFI ([`swift`]).

pub mod ffi;
pub mod image;
pub mod memory;
pub mod page_tables;
pub mod plan;
mod platform;
pub mod swift;
#[cfg(test)]
mod test_images;

pub use image::{check_imports, GuestHeader, GuestImage, ImageError, Segment};
pub use memory::{GuestMemory, GuestPtr, GuestRange, MemoryError, GUEST_SPACE};
pub use page_tables::{PageTableError, PageTables, SegmentAccess, SystemRegisters, Translation};
pub use plan::{AddressSpacePlan, PlanError, PoolAllocator, Region, RegionKind};

uniffi::setup_scaffolding!();
