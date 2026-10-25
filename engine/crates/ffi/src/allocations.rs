//! The engine's allocator: mimalloc, counted, so `stat allocations` can
//! show how many allocations a frame made (from one `advance` to the
//! next, on every thread, the app's calls between included).

use std::sync::{Mutex, PoisonError};

use mimalloc::MiMalloc;
use stats_alloc::{Stats, StatsAlloc};

#[global_allocator]
static ALLOCATOR: StatsAlloc<MiMalloc> = StatsAlloc::new(MiMalloc);

/// A frame's allocations.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Frame {
    /// Allocations, reallocations among them.
    pub allocations: usize,
    /// The bytes the allocations asked for (not the reallocations').
    pub bytes: usize,
}

struct Frames {
    started: Option<Stats>,
    last: Frame,
}

static FRAMES: Mutex<Frames> = Mutex::new(Frames {
    started: None,
    last: Frame {
        allocations: 0,
        bytes: 0,
    },
});

/// Ends a frame and starts the next (at each `advance`).
pub(crate) fn frame_end() {
    let now = ALLOCATOR.stats();
    let mut frames = FRAMES.lock().unwrap_or_else(PoisonError::into_inner);
    if let Some(started) = frames.started {
        frames.last = Frame {
            allocations: (now.allocations + now.reallocations)
                .saturating_sub(started.allocations + started.reallocations),
            bytes: now.bytes_allocated.saturating_sub(started.bytes_allocated),
        };
    }
    frames.started = Some(now);
}

/// The last whole frame's allocations.
pub fn last_frame() -> Frame {
    FRAMES.lock().unwrap_or_else(PoisonError::into_inner).last
}
