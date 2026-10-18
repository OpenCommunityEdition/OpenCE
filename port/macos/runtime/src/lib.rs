//! The pure-logic pieces of the macOS host, which runs the game's ILP32 guest
//! image in a VM under Hypervisor.framework (docs/macos_host.md).
//!
//! - [`memory`]: guest addresses ([`GuestPtr`]) and the host memory that
//!   backs the guest's 4 GB address space.
//!
//! The C host (port/macos/host) calls this crate through its C ABI.

pub mod memory;
mod platform;

pub use memory::{GuestMemory, GuestPtr, GuestRange, MemoryError, GUEST_SPACE};
