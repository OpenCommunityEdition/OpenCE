//! Units, named where they are confusable.

use std::ops::{Add, AddAssign, Mul, Sub};

macro_rules! unit {
    ($(#[$doc:meta])* $name:ident) => {
        $(#[$doc])*
        #[derive(Clone, Copy, Debug, Default, PartialEq, PartialOrd)]
        pub struct $name(pub f32);

        impl Add for $name {
            type Output = Self;
            fn add(self, other: Self) -> Self { Self(self.0 + other.0) }
        }
        impl AddAssign for $name {
            fn add_assign(&mut self, other: Self) { self.0 += other.0; }
        }
        impl Sub for $name {
            type Output = Self;
            fn sub(self, other: Self) -> Self { Self(self.0 - other.0) }
        }
        impl Mul<f32> for $name {
            type Output = Self;
            fn mul(self, scale: f32) -> Self { Self(self.0 * scale) }
        }
    };
}

unit!(
    /// A length in the world's units (a Halo world unit is about 3 m).
    WorldUnits
);
unit!(
    /// A duration of simulated time.
    Seconds
);
unit!(
    /// An angle.
    Radians
);
