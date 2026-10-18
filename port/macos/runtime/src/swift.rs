//! What the Swift app (port/macos/app) calls, through UniFFI: a view of the
//! guest's memory with its range checks.
//!
//! The app gets the memory of the guest the C host is running
//! ([`running_guest_memory`]), or, in its tests, memory of its own
//! ([`GuestMemoryView::with_ranges`]). Either way the memory is mapped
//! before the view exists and never after, so the view needs no lock: a
//! guest address the view accepts stays valid for as long as the view does.
//!
//! The Swift bindings are generated from the library by uniffi-bindgen
//! (src/bin/uniffi-bindgen.rs), as tools/macos_build.py runs it.

use std::sync::{Arc, Mutex, Weak};

use crate::memory::{GuestMemory, GuestPtr, GuestRange, MemoryError};

/// Why a guest range cannot be used, as Swift sees it.
#[derive(Debug, PartialEq, Eq, thiserror::Error, uniffi::Error)]
pub enum GuestMemoryError {
    #[error("an empty range at {address:#010x}")]
    Empty { address: u32 },
    #[error("{size:#x} bytes at {address:#010x} run past the end of the guest's address space")]
    OutOfRange { address: u32, size: u64 },
    #[error("{size:#x} bytes at {address:#010x} are not all mapped")]
    Unmapped { address: u32, size: u64 },
    #[error("{size:#x} bytes at {address:#010x} cannot be mapped: {reason}")]
    NotMappable {
        address: u32,
        size: u64,
        reason: String,
    },
    #[error("the host cannot give the guest memory: {reason}")]
    Host { reason: String },
}

impl GuestMemoryError {
    fn from_memory(error: MemoryError, address: u32, size: u64) -> GuestMemoryError {
        match error {
            MemoryError::Empty { address } => GuestMemoryError::Empty { address },
            MemoryError::OutOfRange { address, size } => {
                GuestMemoryError::OutOfRange { address, size }
            }
            MemoryError::Unmapped { address, size } => GuestMemoryError::Unmapped { address, size },
            error @ (MemoryError::Overlap { .. } | MemoryError::Misaligned { .. }) => {
                GuestMemoryError::NotMappable {
                    address,
                    size,
                    reason: error.to_string(),
                }
            }
            error => GuestMemoryError::Host {
                reason: error.to_string(),
            },
        }
    }
}

/// A guest range a test maps (Swift: `GuestRangeSpec`).
#[derive(Debug, Clone, Copy, uniffi::Record)]
pub struct GuestRangeSpec {
    pub address: u32,
    pub size: u64,
}

/// The guest's memory, mapped once, read and written only inside what is
/// mapped.
#[derive(uniffi::Object)]
pub struct GuestMemoryView {
    memory: Arc<GuestMemory>,
}

/// The memory of the guest the C host made last (ffi.rs), while it lives.
static RUNNING: Mutex<Weak<GuestMemory>> = Mutex::new(Weak::new());

/// Held by the tests that make guests, which change RUNNING.
#[cfg(test)]
pub(crate) static RUNNING_TESTS: Mutex<()> = Mutex::new(());

/// Records `memory` as the running guest's (opence_guest_create).
pub(crate) fn set_running(memory: &Arc<GuestMemory>) {
    *RUNNING
        .lock()
        .unwrap_or_else(|poisoned| poisoned.into_inner()) = Arc::downgrade(memory);
}

/// The memory of the guest the C host is running; None before it has made
/// one, or after it has destroyed it with no view left.
#[uniffi::export]
pub fn running_guest_memory() -> Option<Arc<GuestMemoryView>> {
    let running = RUNNING
        .lock()
        .unwrap_or_else(|poisoned| poisoned.into_inner());
    running
        .upgrade()
        .map(|memory| Arc::new(GuestMemoryView { memory }))
}

#[uniffi::export]
impl GuestMemoryView {
    /// Reserves a guest address space of its own and maps `ranges` in it
    /// (whole host pages, none overlapping).
    #[uniffi::constructor]
    pub fn with_ranges(
        ranges: Vec<GuestRangeSpec>,
    ) -> Result<Arc<GuestMemoryView>, GuestMemoryError> {
        let mut memory =
            GuestMemory::reserve().map_err(|error| GuestMemoryError::from_memory(error, 0, 0))?;
        for spec in ranges {
            let range = GuestRange::new(GuestPtr(spec.address), spec.size)
                .map_err(|error| GuestMemoryError::from_memory(error, spec.address, spec.size))?;
            memory
                .map(range)
                .map_err(|error| GuestMemoryError::from_memory(error, spec.address, spec.size))?;
        }
        Ok(Arc::new(GuestMemoryView {
            memory: Arc::new(memory),
        }))
    }

    /// The host's page size, which mapped ranges are whole pages of.
    pub fn page_size(&self) -> u64 {
        GuestMemory::page_size() as u64
    }

    /// Whether all `size` bytes at `address` are mapped.
    pub fn is_mapped(&self, address: u32, size: u64) -> bool {
        GuestRange::new(GuestPtr(address), size).is_ok_and(|range| self.memory.is_mapped(range))
    }

    /// The host address of `size` mapped bytes at `address` (for Metal to
    /// read in place). It stays valid while this view lives.
    pub fn host_address(&self, address: u32, size: u64) -> Result<u64, GuestMemoryError> {
        self.memory
            .host_pointer(GuestPtr(address), size)
            .map(|pointer| pointer as u64)
            .map_err(|error| GuestMemoryError::from_memory(error, address, size))
    }

    /// A copy of `size` mapped bytes at `address`.
    pub fn read(&self, address: u32, size: u64) -> Result<Vec<u8>, GuestMemoryError> {
        // (checked first, so that a wild size allocates nothing)
        self.host_address(address, size)?;
        let mut bytes = vec![0; size as usize];
        self.memory
            .read(GuestPtr(address), &mut bytes)
            .map_err(|error| GuestMemoryError::from_memory(error, address, size))?;
        Ok(bytes)
    }

    /// Writes `bytes` at `address`, all of which must be mapped.
    pub fn write(&self, address: u32, bytes: Vec<u8>) -> Result<(), GuestMemoryError> {
        self.memory
            .write(GuestPtr(address), &bytes)
            .map_err(|error| GuestMemoryError::from_memory(error, address, bytes.len() as u64))
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    const PAGE: u64 = 0x4000;

    fn view() -> Arc<GuestMemoryView> {
        GuestMemoryView::with_ranges(vec![
            GuestRangeSpec {
                address: 0x10_0000,
                size: 2 * PAGE,
            },
            GuestRangeSpec {
                address: 0x20_0000,
                size: PAGE,
            },
        ])
        .unwrap()
    }

    #[test]
    fn the_page_size_is_the_hosts() {
        assert_eq!(view().page_size(), GuestMemory::page_size() as u64);
    }

    #[test]
    fn mapped_ranges_are_read_and_written() {
        let view = view();
        assert!(view.is_mapped(0x10_0000, 2 * PAGE));
        view.write(0x10_3ffe, vec![1, 2, 3, 4]).unwrap();
        assert_eq!(view.read(0x10_3ffe, 4).unwrap(), [1, 2, 3, 4]);
        // fresh pages are zero
        assert_eq!(view.read(0x20_0000, 8).unwrap(), [0; 8]);
    }

    #[test]
    fn the_host_address_is_the_bytes_in_place() {
        let view = view();
        view.write(0x20_0010, vec![0xab]).unwrap();
        let address = view.host_address(0x20_0010, 1).unwrap();
        assert_eq!(unsafe { *(address as *const u8) }, 0xab);
        // the gap between the ranges is not mapped
        assert_eq!(
            view.host_address(0x10_0000, 3 * PAGE),
            Err(GuestMemoryError::Unmapped {
                address: 0x10_0000,
                size: 3 * PAGE
            })
        );
    }

    #[test]
    fn ranges_outside_what_is_mapped_are_refused() {
        let view = view();
        assert!(!view.is_mapped(0x30_0000, 1));
        assert!(!view.is_mapped(0x10_0000, 0));
        assert_eq!(
            view.read(0x30_0000, 4),
            Err(GuestMemoryError::Unmapped {
                address: 0x30_0000,
                size: 4
            })
        );
        assert_eq!(
            view.read(0x10_0000, 0),
            Err(GuestMemoryError::Empty { address: 0x10_0000 })
        );
        assert_eq!(
            view.read(0xffff_fff0, 0x20),
            Err(GuestMemoryError::OutOfRange {
                address: 0xffff_fff0,
                size: 0x20
            })
        );
        // a size that would not fit the host's memory is refused before
        // anything is allocated
        assert!(view.read(0x10_0000, u64::MAX).is_err());
        assert_eq!(
            view.write(0x20_3ffe, vec![0; 4]),
            Err(GuestMemoryError::Unmapped {
                address: 0x20_3ffe,
                size: 4
            })
        );
    }

    #[test]
    fn ranges_that_cannot_be_mapped_are_refused() {
        let overlap = GuestMemoryView::with_ranges(vec![
            GuestRangeSpec {
                address: 0x10_0000,
                size: 2 * PAGE,
            },
            GuestRangeSpec {
                address: 0x10_4000,
                size: PAGE,
            },
        ]);
        assert!(matches!(
            overlap,
            Err(GuestMemoryError::NotMappable {
                address: 0x10_4000,
                ..
            })
        ));
        let misaligned = GuestMemoryView::with_ranges(vec![GuestRangeSpec {
            address: 0x10_0010,
            size: PAGE,
        }]);
        assert!(matches!(
            misaligned,
            Err(GuestMemoryError::NotMappable {
                address: 0x10_0010,
                ..
            })
        ));
        let empty = GuestMemoryView::with_ranges(vec![GuestRangeSpec {
            address: 0x10_0000,
            size: 0,
        }]);
        assert_eq!(
            empty.err(),
            Some(GuestMemoryError::Empty { address: 0x10_0000 })
        );
    }

    #[test]
    fn host_errors_keep_their_reason() {
        let error = GuestMemoryError::from_memory(
            MemoryError::Reserve(std::io::Error::from_raw_os_error(libc::ENOMEM)),
            0,
            0,
        );
        assert!(matches!(error, GuestMemoryError::Host { reason } if reason.contains("reserve")));
    }

    #[test]
    fn the_running_guest_is_shared_while_it_lives() {
        let _running = RUNNING_TESTS
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        let memory = Arc::new(GuestMemory::reserve().unwrap());
        set_running(&memory);
        let view = running_guest_memory().expect("a running guest");
        assert!(Arc::ptr_eq(&view.memory, &memory));
        drop(view);
        drop(memory);
        assert!(running_guest_memory().is_none());
    }
}
