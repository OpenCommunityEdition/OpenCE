//! What the Swift app (port/macos/app) calls, through UniFFI: a view of the
//! guest's memory with its range checks.
//!
//! The app gets the memory of the guest the C host is running
//! ([`running_guest_memory`]), or, in its tests, memory of its own
//! ([`GuestMemoryView::with_ranges`]). Either way the memory is mapped
//! before the view exists and never after, so the view needs no lock: a
//! guest address the view accepts stays valid for as long as the view does.
//!
//! The app's Metal renderer decodes the guest's render batches through a
//! [`RenderDecoder`], which checks every command against the objects it
//! has seen (render.rs) before Metal is given any of it.
//!
//! The Swift bindings are generated from the library by uniffi-bindgen
//! (src/bin/uniffi-bindgen.rs), as tools/macos_build.py runs it.

use std::sync::{Arc, Mutex, Weak};

use crate::memory::{GuestMemory, GuestPtr, GuestRange, MemoryError};
use crate::render::{copy_batch, BatchError, DecodedBatch, RenderValidator};

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

/// The render ring's decoder for one guest (Swift: `RenderDecoder`). It
/// keeps the objects and the open pass between batches, so the app makes
/// one per guest and gives it every batch in order.
#[derive(uniffi::Object)]
pub struct RenderDecoder {
    validator: Mutex<RenderValidator>,
    memory: Arc<GuestMemory>,
}

#[uniffi::export]
impl RenderDecoder {
    /// A decoder for the guest whose memory `memory` views, with no window
    /// yet (a pass on the window is refused until the app gives its size).
    #[uniffi::constructor]
    pub fn new(memory: Arc<GuestMemoryView>) -> Arc<RenderDecoder> {
        Arc::new(RenderDecoder {
            validator: Mutex::new(RenderValidator::new()),
            memory: Arc::clone(&memory.memory),
        })
    }

    /// The window's drawable size in pixels; 0 by 0 while it has none.
    pub fn set_drawable_size(&self, width: u32, height: u32) {
        self.validator().set_drawable_size(width, height);
    }

    /// Decodes a batch the app already holds.
    pub fn decode_bytes(&self, bytes: Vec<u8>) -> Result<DecodedBatch, BatchError> {
        self.validator().decode(&bytes, &*self.memory)
    }

    /// Decodes the `size` bytes of a batch at `address` in the guest. The
    /// batch is copied out first, so the guest writing the ring meanwhile
    /// cannot change what was checked.
    pub fn decode_guest(&self, address: u32, size: u32) -> Result<DecodedBatch, BatchError> {
        let bytes = copy_batch(&self.memory, address, size)?;
        self.validator().decode(&bytes, &*self.memory)
    }
}

impl RenderDecoder {
    fn validator(&self) -> std::sync::MutexGuard<'_, RenderValidator> {
        self.validator
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::render::MAX_BATCH_LENGTH;

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

    /// A batch of `commands` (each its op and fields).
    fn batch(commands: &[(u32, &[u32])]) -> Vec<u8> {
        let mut words = vec![crate::render::MAGIC, crate::render::VERSION, 0, 0];
        for (op, fields) in commands {
            words.push(op | (fields.len() as u32 + 1) << 16);
            words.extend_from_slice(fields);
        }
        words[2] = words.len() as u32 * 4;
        words[3] = commands.len() as u32;
        words.iter().flat_map(|word| word.to_le_bytes()).collect()
    }

    #[test]
    fn the_decoder_keeps_its_objects_between_batches() {
        use crate::render::{op, Command};
        let decoder = RenderDecoder::new(view());
        let made = decoder
            .decode_bytes(batch(&[(op::CREATE_BUFFER, &[1, 64])]))
            .unwrap();
        assert_eq!(made.commands, [Command::CreateBuffer { id: 1, length: 64 }]);
        // an upload from mapped guest memory into the buffer made before
        let upload = decoder
            .decode_bytes(batch(&[(op::UPLOAD_BUFFER, &[1, 0, 0x10_0000, 64])]))
            .unwrap();
        assert!(upload.rejected.is_empty());
        assert_eq!(upload.commands.len(), 1);
        // and none from memory that is not mapped
        let wild = decoder
            .decode_bytes(batch(&[(op::UPLOAD_BUFFER, &[1, 0, 0x30_0000, 64])]))
            .unwrap();
        assert!(wild.commands.is_empty());
        assert_eq!(wild.rejected.len(), 1);
    }

    #[test]
    fn the_decoder_reads_batches_from_the_guest() {
        use crate::render::{op, Command};
        let view = view();
        let decoder = RenderDecoder::new(Arc::clone(&view));
        // a pass on the window that leaves its contents be, before the
        // window has a size and after
        let mut pass = [0; 13];
        pass[2..5].copy_from_slice(&[crate::render::Load::DontCare.word(); 3]);
        let early = batch(&[(op::BEGIN_PASS, &pass), (op::END_PASS, &[])]);
        view.write(0x20_0000, early.clone()).unwrap();
        let refused = decoder.decode_guest(0x20_0000, early.len() as u32).unwrap();
        assert!(refused.commands.is_empty());
        decoder.set_drawable_size(640, 480);
        let shown = decoder.decode_guest(0x20_0000, early.len() as u32).unwrap();
        assert!(matches!(
            shown.commands[..],
            [
                Command::BeginPass {
                    width: 640,
                    height: 480,
                    ..
                },
                Command::EndPass
            ]
        ));
        assert!(matches!(
            decoder.decode_guest(0x30_0000, 16),
            Err(BatchError::Guest { .. })
        ));
        assert_eq!(
            decoder.decode_guest(0x20_0000, MAX_BATCH_LENGTH + 4),
            Err(BatchError::Size {
                size: u64::from(MAX_BATCH_LENGTH) + 4
            })
        );
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
