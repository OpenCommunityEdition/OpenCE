//! A hash of the world, the same for the same state on any run.
//!
//! It is FNV-1a over the bits of what it is given, in the order given:
//! std's hashers are not promised to stay the same between releases.

/// FNV-1a, 64 bits.
#[derive(Clone, Copy, Debug)]
pub struct WorldHasher(u64);

impl Default for WorldHasher {
    fn default() -> Self {
        Self(0xcbf2_9ce4_8422_2325)
    }
}

impl WorldHasher {
    /// Adds bytes.
    pub fn bytes(&mut self, bytes: &[u8]) {
        for &byte in bytes {
            self.0 ^= u64::from(byte);
            self.0 = self.0.wrapping_mul(0x0000_0100_0000_01b3);
        }
    }

    /// Adds a number.
    pub fn u64(&mut self, value: u64) {
        self.bytes(&value.to_le_bytes());
    }

    /// Adds a float by its bits, so that 0.0 and -0.0 differ and so do
    /// values a comparison would call equal.
    pub fn f32(&mut self, value: f32) {
        self.bytes(&value.to_bits().to_le_bytes());
    }

    /// The hash so far.
    pub fn finish(&self) -> u64 {
        self.0
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn it_is_fnv_1a() {
        // (FNV-1a's published value for "a")
        let mut hasher = WorldHasher::default();
        hasher.bytes(b"a");
        assert_eq!(hasher.finish(), 0xaf63_dc4c_8601_ec8c);
    }

    #[test]
    fn signed_zeros_differ() {
        let mut a = WorldHasher::default();
        let mut b = WorldHasher::default();
        a.f32(0.0);
        b.f32(-0.0);
        assert_ne!(a.finish(), b.finish());
    }
}
