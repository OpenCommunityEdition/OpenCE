//! The game's seeded random numbers (`random_math.c`): a linear
//! congruential generator whose seed is part of the game's state, so that
//! every machine that steps the same game draws the same numbers.

use bevy_ecs::prelude::*;

const RANDOM_A: u32 = 1_664_525;
const RANDOM_C: u32 = 1_013_904_223;

/// The seed a game starts with, unless its options give another.
pub const DEFAULT_SEED: u32 = 0xDEAD_BEEF;

/// A random seed: the game's (which the game's state draws from).
#[derive(Resource, Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub struct RandomSeed(pub u32);

impl Default for RandomSeed {
    fn default() -> Self {
        Self(DEFAULT_SEED)
    }
}

impl RandomSeed {
    /// The next number, from 0 to 65535 (`seed_random`).
    pub fn next_u16(&mut self) -> u16 {
        self.0 = self.0.wrapping_mul(RANDOM_A).wrapping_add(RANDOM_C);
        (self.0 >> 16) as u16
    }

    /// The next number, from 0 to 1 (`real_seed_random`).
    pub fn next_real(&mut self) -> f32 {
        f32::from(self.next_u16()) / 65535.0
    }

    /// The next number from `lower` up to but not including `upper`
    /// (`seed_random_range`).
    pub fn next_range(&mut self, lower: i16, upper: i16) -> i16 {
        let span = (i32::from(upper) - i32::from(lower)) as u32;
        let offset = span.wrapping_mul(u32::from(self.next_u16())) >> 16;
        (i32::from(lower) + offset as i32) as i16
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn the_numbers_are_the_games() {
        let mut seed = RandomSeed::default();
        // 0xDEADBEEF * 1664525 + 1013904223, kept to 32 bits
        let first = 0xDEAD_BEEFu32
            .wrapping_mul(1_664_525)
            .wrapping_add(1_013_904_223);
        assert_eq!(seed.next_u16(), (first >> 16) as u16);
        assert_eq!(seed.0, first);
        let mut again = RandomSeed::default();
        assert_eq!(again.next_real(), f32::from((first >> 16) as u16) / 65535.0);
        for _ in 0..1000 {
            let value = seed.next_range(3, 9);
            assert!((3..9).contains(&value));
            assert!((0.0..=1.0).contains(&seed.next_real()));
        }
    }
}
