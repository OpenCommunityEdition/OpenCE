//! The operating system's side of the guest memory: reserving address space
//! and committing pages in it. Only this module calls the system, so that a
//! wasm32 implementation can be added without touching the rest.

/// A reservation of address space, inaccessible until parts of it are
/// committed. Released when dropped.
pub(crate) struct Reservation {
    base: *mut u8,
    size: usize,
}

impl Reservation {
    pub(crate) fn base(&self) -> *mut u8 {
        self.base
    }

    pub(crate) fn size(&self) -> usize {
        self.size
    }
}

#[cfg(unix)]
mod system {
    use super::Reservation;
    use std::io;

    /// The host's page size: what [`Reservation::commit`] works in.
    pub(crate) fn page_size() -> usize {
        // SAFETY: sysconf has no preconditions; _SC_PAGESIZE is always known.
        let size = unsafe { libc::sysconf(libc::_SC_PAGESIZE) };
        usize::try_from(size).unwrap_or(4096)
    }

    impl Reservation {
        pub(crate) fn new(size: usize) -> io::Result<Reservation> {
            // SAFETY: a new anonymous mapping at an address the system picks
            // touches no existing memory.
            let base = unsafe {
                libc::mmap(
                    std::ptr::null_mut(),
                    size,
                    libc::PROT_NONE,
                    libc::MAP_PRIVATE | libc::MAP_ANON | libc::MAP_NORESERVE,
                    -1,
                    0,
                )
            };
            if base == libc::MAP_FAILED {
                return Err(io::Error::last_os_error());
            }
            Ok(Reservation {
                base: base.cast(),
                size,
            })
        }

        /// Makes [offset, offset + size) readable and writable, zero-filled.
        /// The caller has checked that the range lies in the reservation and
        /// is page-aligned.
        pub(crate) fn commit(&self, offset: usize, size: usize) -> io::Result<()> {
            debug_assert!(offset.checked_add(size).is_some_and(|end| end <= self.size));
            // SAFETY: the range lies inside this reservation (checked by the
            // caller), which no one else maps, so replacing it touches only
            // memory this reservation owns.
            let address = unsafe {
                libc::mmap(
                    self.base.add(offset).cast(),
                    size,
                    libc::PROT_READ | libc::PROT_WRITE,
                    libc::MAP_PRIVATE | libc::MAP_ANON | libc::MAP_FIXED,
                    -1,
                    0,
                )
            };
            if address == libc::MAP_FAILED {
                return Err(io::Error::last_os_error());
            }
            Ok(())
        }
    }

    impl Drop for Reservation {
        fn drop(&mut self) {
            // SAFETY: base and size are those mmap returned; nothing refers to
            // the memory once the reservation is dropped.
            unsafe {
                libc::munmap(self.base.cast(), self.size);
            }
        }
    }
}

#[cfg(not(unix))]
mod system {
    use super::Reservation;
    use std::io;

    pub(crate) fn page_size() -> usize {
        4096
    }

    impl Reservation {
        pub(crate) fn new(_size: usize) -> io::Result<Reservation> {
            Err(io::Error::from(io::ErrorKind::Unsupported))
        }

        pub(crate) fn commit(&self, _offset: usize, _size: usize) -> io::Result<()> {
            Err(io::Error::from(io::ErrorKind::Unsupported))
        }
    }
}

pub(crate) use system::page_size;
