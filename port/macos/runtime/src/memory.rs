//! Guest addresses and the host memory behind them.
//!
//! The guest is ILP32 code: its addresses are 32 bits, and its address space
//! is 4 GB. The host reserves 4 GB of its own address space once, and a guest
//! address `a` is the host address `base + a`; the host maps that memory into
//! the VM at the same guest address. Only the ranges the host has mapped
//! ([`GuestMemory::map`]) may be used.
//!
//! The guest's threads run while the host reads and writes their memory, so
//! no Rust reference into guest memory is ever made: [`GuestMemory::read`] and
//! [`GuestMemory::write`] copy, and [`GuestMemory::host_pointer`] hands the C
//! host a raw pointer.

use std::collections::BTreeMap;
use std::fmt;
use std::io;

use crate::platform::{self, Reservation};

/// The size of the guest's address space: every 32-bit address.
pub const GUEST_SPACE: u64 = 1 << 32;

/// An address in the guest's address space.
#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, Default)]
pub struct GuestPtr(pub u32);

impl GuestPtr {
    /// The address `offset` bytes further on, or `None` past the end of the
    /// address space.
    pub fn checked_add(self, offset: u32) -> Option<GuestPtr> {
        self.0.checked_add(offset).map(GuestPtr)
    }
}

impl fmt::Debug for GuestPtr {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "GuestPtr({:#010x})", self.0)
    }
}

/// A non-empty range of guest addresses, inside the address space.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub struct GuestRange {
    start: u32,
    // (exclusive, and up to GUEST_SPACE, which does not fit a u32)
    end: u64,
}

impl GuestRange {
    /// The range of `size` bytes at `start`.
    pub fn new(start: GuestPtr, size: u64) -> Result<GuestRange, MemoryError> {
        if size == 0 {
            return Err(MemoryError::Empty { address: start.0 });
        }
        match u64::from(start.0).checked_add(size) {
            Some(end) if end <= GUEST_SPACE => Ok(GuestRange {
                start: start.0,
                end,
            }),
            _ => Err(MemoryError::OutOfRange {
                address: start.0,
                size,
            }),
        }
    }

    pub fn start(&self) -> GuestPtr {
        GuestPtr(self.start)
    }

    /// One past the last address (up to [`GUEST_SPACE`]).
    pub fn end(&self) -> u64 {
        self.end
    }

    pub fn size(&self) -> u64 {
        self.end - u64::from(self.start)
    }
}

/// Why a guest address or range cannot be used.
#[derive(Debug, thiserror::Error)]
pub enum MemoryError {
    #[error("an empty range at {address:#010x}")]
    Empty { address: u32 },
    #[error("{size:#x} bytes at {address:#010x} run past the end of the guest's address space")]
    OutOfRange { address: u32, size: u64 },
    #[error("{size:#x} bytes at {address:#010x} are not all mapped")]
    Unmapped { address: u32, size: u64 },
    #[error("{size:#x} bytes at {address:#010x} overlap memory already mapped")]
    Overlap { address: u32, size: u64 },
    #[error(
        "{size:#x} bytes at {address:#010x} are not aligned to the host's {page:#x}-byte pages"
    )]
    Misaligned {
        address: u32,
        size: u64,
        page: usize,
    },
    #[error("no NUL within {limit} bytes at {address:#010x}")]
    Unterminated { address: u32, limit: u32 },
    #[error("cannot reserve the guest's address space: {0}")]
    Reserve(io::Error),
    #[error("cannot map guest memory: {0}")]
    Commit(io::Error),
}

/// The host memory behind the guest's address space.
pub struct GuestMemory {
    reservation: Reservation,
    /// the mapped ranges: start -> end (exclusive), none overlapping
    mapped: BTreeMap<u32, u64>,
}

// SAFETY: the reservation is owned by this value alone; guest memory is only
// accessed by copies through raw pointers (never as Rust references), which
// any thread may make, as the guest's own threads do.
unsafe impl Send for GuestMemory {}
// SAFETY: as for Send; the methods that change the map of mapped ranges take
// &mut self.
unsafe impl Sync for GuestMemory {}

impl GuestMemory {
    /// Reserves the guest's whole address space, with nothing mapped yet.
    pub fn reserve() -> Result<GuestMemory, MemoryError> {
        let size = usize::try_from(GUEST_SPACE)
            .map_err(|_| MemoryError::Reserve(io::Error::from(io::ErrorKind::Unsupported)))?;
        let reservation = Reservation::new(size).map_err(MemoryError::Reserve)?;
        debug_assert_eq!(reservation.size(), size);
        Ok(GuestMemory {
            reservation,
            mapped: BTreeMap::new(),
        })
    }

    /// The host's page size, which [`GuestMemory::map`] works in.
    pub fn page_size() -> usize {
        platform::page_size()
    }

    /// The host address of guest address 0.
    pub fn host_base(&self) -> *mut u8 {
        self.reservation.base()
    }

    /// Maps `range`, zero-filled, readable and writable. It must be aligned
    /// to the host's pages and not overlap a range already mapped.
    pub fn map(&mut self, range: GuestRange) -> Result<(), MemoryError> {
        let page = GuestMemory::page_size();
        let page_mask = page as u64 - 1;
        if u64::from(range.start) & page_mask != 0 || range.size() & page_mask != 0 {
            return Err(MemoryError::Misaligned {
                address: range.start,
                size: range.size(),
                page,
            });
        }
        if self.overlaps(range) {
            return Err(MemoryError::Overlap {
                address: range.start,
                size: range.size(),
            });
        }
        // (both fit a usize: the reservation is GUEST_SPACE bytes)
        self.reservation
            .commit(range.start as usize, range.size() as usize)
            .map_err(MemoryError::Commit)?;
        self.mapped.insert(range.start, range.end);
        Ok(())
    }

    fn overlaps(&self, range: GuestRange) -> bool {
        // the mapping that starts at or before the range's start, and any
        // that starts inside it
        let before = self
            .mapped
            .range(..=range.start)
            .next_back()
            .is_some_and(|(_, &end)| end > u64::from(range.start));
        before
            || self
                .mapped
                .range(range.start..)
                .next()
                .is_some_and(|(&start, _)| u64::from(start) < range.end)
    }

    /// Whether all of `range` is mapped (by one or by adjacent mappings).
    pub fn is_mapped(&self, range: GuestRange) -> bool {
        // follow the mappings from the range's start while each ends where
        // the next begins
        let mut covered = u64::from(range.start);
        while covered < range.end {
            let Ok(address) = u32::try_from(covered) else {
                return false;
            };
            match self.mapped.range(..=address).next_back() {
                Some((_, &end)) if end > covered => covered = end,
                _ => return false,
            }
        }
        true
    }

    /// `size` bytes at `address`, if all are mapped.
    fn mapped_range(&self, address: GuestPtr, size: u64) -> Result<GuestRange, MemoryError> {
        let range = GuestRange::new(address, size)?;
        if self.is_mapped(range) {
            Ok(range)
        } else {
            Err(MemoryError::Unmapped {
                address: address.0,
                size,
            })
        }
    }

    /// The host address of `size` mapped bytes at `address`, for the C host.
    pub fn host_pointer(&self, address: GuestPtr, size: u64) -> Result<*mut u8, MemoryError> {
        self.mapped_range(address, size)?;
        // SAFETY: the address is below GUEST_SPACE, the reservation's size,
        // so the result stays inside the reservation.
        Ok(unsafe { self.host_base().add(address.0 as usize) })
    }

    /// Copies guest memory at `address` into `buffer`.
    pub fn read(&self, address: GuestPtr, buffer: &mut [u8]) -> Result<(), MemoryError> {
        if buffer.is_empty() {
            return Ok(());
        }
        let source = self.host_pointer(address, buffer.len() as u64)?;
        // SAFETY: host_pointer checked that all buffer.len() bytes at source
        // are mapped; buffer is host memory the caller owns, not guest
        // memory, so the two do not overlap.
        unsafe { std::ptr::copy_nonoverlapping(source, buffer.as_mut_ptr(), buffer.len()) };
        Ok(())
    }

    /// Copies `data` into guest memory at `address`.
    pub fn write(&self, address: GuestPtr, data: &[u8]) -> Result<(), MemoryError> {
        if data.is_empty() {
            return Ok(());
        }
        let destination = self.host_pointer(address, data.len() as u64)?;
        // SAFETY: as for read: data.len() mapped bytes at destination, and
        // data is the caller's own memory.
        unsafe { std::ptr::copy_nonoverlapping(data.as_ptr(), destination, data.len()) };
        Ok(())
    }

    /// A little-endian 32-bit value; the address need not be aligned.
    pub fn read_u32(&self, address: GuestPtr) -> Result<u32, MemoryError> {
        let mut bytes = [0u8; 4];
        self.read(address, &mut bytes)?;
        Ok(u32::from_le_bytes(bytes))
    }

    pub fn write_u32(&self, address: GuestPtr, value: u32) -> Result<(), MemoryError> {
        self.write(address, &value.to_le_bytes())
    }

    /// The NUL-terminated string at `address`, without its NUL; at most
    /// `limit` bytes are looked at.
    pub fn read_c_string(&self, address: GuestPtr, limit: u32) -> Result<Vec<u8>, MemoryError> {
        let mut text = Vec::new();
        for offset in 0..limit {
            let at = address.checked_add(offset).ok_or(MemoryError::OutOfRange {
                address: address.0,
                size: u64::from(offset) + 1,
            })?;
            let mut byte = [0u8];
            self.read(at, &mut byte)?;
            if byte[0] == 0 {
                return Ok(text);
            }
            text.push(byte[0]);
        }
        Err(MemoryError::Unterminated {
            address: address.0,
            limit,
        })
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    const WINDOW: u32 = 0x8000_0000;
    const WINDOW_SIZE: u64 = 0x0800_0000;

    fn page() -> u64 {
        GuestMemory::page_size() as u64
    }

    fn range(start: u32, size: u64) -> GuestRange {
        GuestRange::new(GuestPtr(start), size).expect("a valid range")
    }

    fn with_window() -> GuestMemory {
        let mut memory = GuestMemory::reserve().expect("4 GB of address space");
        memory
            .map(range(WINDOW, WINDOW_SIZE))
            .expect("the window maps");
        memory
    }

    #[test]
    fn pointers_add_up_to_the_end_of_the_address_space() {
        assert_eq!(GuestPtr(0x1000).checked_add(0x20), Some(GuestPtr(0x1020)));
        assert_eq!(
            GuestPtr(0xffff_fff0).checked_add(0xf),
            Some(GuestPtr(0xffff_ffff))
        );
        assert_eq!(GuestPtr(0xffff_fff0).checked_add(0x10), None);
        assert_eq!(
            format!("{:?}", GuestPtr(0x8000_0000)),
            "GuestPtr(0x80000000)"
        );
    }

    #[test]
    fn ranges_stay_inside_the_address_space() {
        let top = range(0xffff_f000, 0x1000);
        assert_eq!(top.end(), GUEST_SPACE);
        assert_eq!(top.size(), 0x1000);
        assert!(matches!(
            GuestRange::new(GuestPtr(0xffff_f000), 0x1001),
            Err(MemoryError::OutOfRange {
                address: 0xffff_f000,
                size: 0x1001
            })
        ));
        assert!(matches!(
            GuestRange::new(GuestPtr(0xffff_fff0), u64::MAX),
            Err(MemoryError::OutOfRange { .. })
        ));
        assert!(matches!(
            GuestRange::new(GuestPtr(0x1000), 0),
            Err(MemoryError::Empty { address: 0x1000 })
        ));
    }

    #[test]
    fn nothing_is_mapped_at_first() {
        let memory = GuestMemory::reserve().expect("4 GB of address space");
        let mut byte = [0u8];
        assert!(!memory.is_mapped(range(WINDOW, 1)));
        assert!(matches!(
            memory.read(GuestPtr(WINDOW), &mut byte),
            Err(MemoryError::Unmapped {
                address: WINDOW,
                size: 1
            })
        ));
        assert!(memory.host_pointer(GuestPtr(0), 1).is_err());
    }

    #[test]
    fn mapped_memory_is_zero_and_keeps_what_is_written() {
        let memory = with_window();
        let last = GuestPtr(WINDOW + (WINDOW_SIZE as u32) - 4);
        assert_eq!(memory.read_u32(GuestPtr(WINDOW)).expect("mapped"), 0);
        memory
            .write_u32(GuestPtr(WINDOW), 0x1234_5678)
            .expect("mapped");
        memory.write_u32(last, 0x9abc_def0).expect("mapped");
        assert_eq!(
            memory.read_u32(GuestPtr(WINDOW)).expect("mapped"),
            0x1234_5678
        );
        assert_eq!(memory.read_u32(last).expect("mapped"), 0x9abc_def0);
        let mut bytes = [0u8; 4];
        memory.read(GuestPtr(WINDOW), &mut bytes).expect("mapped");
        assert_eq!(bytes, 0x1234_5678u32.to_le_bytes());
    }

    #[test]
    fn host_addresses_are_the_base_plus_the_guest_address() {
        let memory = with_window();
        let pointer = memory
            .host_pointer(GuestPtr(WINDOW + 0x10), 0x10)
            .expect("mapped");
        assert_eq!(
            pointer as usize,
            memory.host_base() as usize + WINDOW as usize + 0x10
        );
        memory
            .write_u32(GuestPtr(WINDOW + 0x10), 7)
            .expect("mapped");
        // SAFETY: the pointer is to four mapped bytes, written just above.
        assert_eq!(unsafe { pointer.cast::<u32>().read_unaligned() }, 7);
    }

    #[test]
    fn accesses_past_a_mapped_range_are_refused() {
        let memory = with_window();
        let end = WINDOW + WINDOW_SIZE as u32;
        assert!(matches!(
            memory.write(GuestPtr(end - 2), &[1, 2, 3, 4]),
            Err(MemoryError::Unmapped { address, size: 4 }) if address == end - 2
        ));
        assert!(matches!(
            memory.host_pointer(GuestPtr(WINDOW - 1), 2),
            Err(MemoryError::Unmapped { .. })
        ));
        // and at the very top, a range that would wrap
        let mut bytes = [0u8; 32];
        assert!(matches!(
            memory.read(GuestPtr(0xffff_fff0), &mut bytes),
            Err(MemoryError::OutOfRange { .. })
        ));
        // nothing was written by the refused write
        assert_eq!(memory.read_u32(GuestPtr(end - 4)).expect("mapped"), 0);
    }

    #[test]
    fn adjacent_mappings_read_as_one() {
        let mut memory = with_window();
        let end = WINDOW + WINDOW_SIZE as u32;
        memory.map(range(end, page())).expect("the next pages map");
        assert!(memory.is_mapped(range(end - 4, 8)));
        memory
            .write(GuestPtr(end - 4), &[1, 2, 3, 4, 5, 6, 7, 8])
            .expect("both mapped");
        assert_eq!(
            memory.read_u32(GuestPtr(end)).expect("mapped"),
            u32::from_le_bytes([5, 6, 7, 8])
        );
    }

    #[test]
    fn mappings_must_be_page_aligned_and_must_not_overlap() {
        let mut memory = with_window();
        assert!(matches!(
            memory.map(range(WINDOW + 0x10_0000, page())),
            Err(MemoryError::Overlap { .. })
        ));
        assert!(matches!(
            memory.map(range(WINDOW - page() as u32, 2 * page())),
            Err(MemoryError::Overlap { .. })
        ));
        assert!(matches!(
            memory.map(range(0x1000_0000 + 0x100, page())),
            Err(MemoryError::Misaligned { .. })
        ));
        assert!(matches!(
            memory.map(range(0x1000_0000, page() + 1)),
            Err(MemoryError::Misaligned { .. })
        ));
        // a refused mapping maps nothing
        assert!(!memory.is_mapped(range(0x1000_0000, 1)));
    }

    #[test]
    fn strings_end_at_their_nul_within_the_limit() {
        let memory = with_window();
        memory
            .write(GuestPtr(WINDOW), b"host_syscall\0host_log\0")
            .expect("mapped");
        assert_eq!(
            memory
                .read_c_string(GuestPtr(WINDOW), 64)
                .expect("terminated"),
            b"host_syscall"
        );
        assert_eq!(
            memory
                .read_c_string(GuestPtr(WINDOW + 13), 64)
                .expect("terminated"),
            b"host_log"
        );
        assert!(matches!(
            memory.read_c_string(GuestPtr(WINDOW), 12),
            Err(MemoryError::Unterminated {
                address: WINDOW,
                limit: 12
            })
        ));
        // a string that runs off the mapped memory
        let end = WINDOW + WINDOW_SIZE as u32;
        memory.write(GuestPtr(end - 2), b"ab").expect("mapped");
        assert!(memory.read_c_string(GuestPtr(end - 2), 64).is_err());
    }
}
