//! A limit on how many packets a peer may send: a bucket of tokens that
//! fills at a steady rate, one token taken by each packet.

/// A peer's packet allowance.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct RateLimit {
    per_tick: f32,
    burst: f32,
    tokens: f32,
}

impl RateLimit {
    /// At most `per_second` packets a second, `burst` at once, at the
    /// engine's `ticks_per_second`.
    pub fn new(per_second: u32, burst: u32, ticks_per_second: u32) -> Self {
        let burst = burst.max(1) as f32;
        Self {
            per_tick: per_second as f32 / ticks_per_second.max(1) as f32,
            burst,
            tokens: burst,
        }
    }

    /// A tick passed.
    pub fn tick(&mut self) {
        self.tokens = (self.tokens + self.per_tick).min(self.burst);
    }

    /// Whether a packet that arrived now is allowed.
    pub fn allow(&mut self) -> bool {
        if self.tokens >= 1.0 {
            self.tokens -= 1.0;
            true
        } else {
            false
        }
    }
}
