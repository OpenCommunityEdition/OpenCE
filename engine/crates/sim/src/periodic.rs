//! The game's periodic functions (`periodic_functions.c`): one, zero,
//! cosine, diagonal wave, slide, noise, jitter, wander and spark, each a
//! table of 1024 bytes built once from the game's seed, read at 25.6
//! entries a period and mixed between neighbours. The shaders' animations
//! (self-illumination, scrolling maps) and the objects' functions take
//! their values from them.

use std::sync::OnceLock;

use crate::random::RandomSeed;

/// The periodic functions, as the tags number them.
pub mod function {
    /// Always one.
    pub const ONE: i16 = 0;
    /// Always zero.
    pub const ZERO: i16 = 1;
    /// A cosine from one through zero and back.
    pub const COSINE: i16 = 2;
    /// A cosine whose period wanders.
    pub const COSINE_VARIABLE_PERIOD: i16 = 3;
    /// Up from zero to one and down again.
    pub const DIAGONAL_WAVE: i16 = 4;
    /// The diagonal wave, its period wandering.
    pub const DIAGONAL_WAVE_VARIABLE_PERIOD: i16 = 5;
    /// Up from zero to one, then back to zero at once.
    pub const SLIDE: i16 = 6;
    /// The slide, its period wandering.
    pub const SLIDE_VARIABLE_PERIOD: i16 = 7;
    /// A random value each entry.
    pub const NOISE: i16 = 8;
    /// A sum of cosines.
    pub const JITTER: i16 = 9;
    /// As the jitter.
    pub const WANDER: i16 = 10;
    /// The variable slide, squared.
    pub const SPARK: i16 = 11;
    /// How many there are.
    pub const COUNT: usize = 12;
}

const TABLE_SIZE: usize = 1024;
const TABLE_MASK: usize = TABLE_SIZE - 1;
/// The seed the tables are built from (`periodic_functions_initialize`).
const TABLE_SEED: u32 = 0x20F3_F660;
/// Table entries per period.
const ENTRIES_PER_PERIOD: f32 = 25.6;
/// Periods per table entry, as the builder spaces them (28/1024).
const PERIODS_PER_ENTRY: f32 = 0.027_343_748;
/// Periods the variable-period tables span.
const VARIABLE_PERIODS: f32 = 28.0;
const TAU: f32 = 6.283_185_5;

fn is_slide(function: i16) -> bool {
    function == function::SLIDE || function == function::SLIDE_VARIABLE_PERIOD
}

fn cos(x: f32) -> f32 {
    libm::cos(f64::from(x)) as f32
}

fn sin(x: f32) -> f32 {
    libm::sin(f64::from(x)) as f32
}

fn fraction(x: f32) -> f32 {
    libm::fmod(f64::from(x), 1.0) as f32
}

/// The wandering x of the variable-period tables: steps of random length
/// summed and scaled to end at one
/// (`periodic_function_build_variable_period_x_table`).
fn variable_period_x(seed: &mut RandomSeed) -> [f32; TABLE_SIZE] {
    let mut table = [0.0f32; TABLE_SIZE];
    let mut sum = 0.0f32;
    for (index, entry) in table.iter_mut().enumerate() {
        *entry = sum;
        let at = index as f32 / TABLE_SIZE as f32;
        let wave = |frequency: f32| cos(frequency * std::f32::consts::PI * at) + 1.0;
        let first = seed.next_real() * 0.25 + 0.25;
        let second = wave(8.2) * seed.next_real();
        let third = wave(10.2) * seed.next_real();
        let fourth = wave(14.6) * seed.next_real();
        sum += first + second + third + fourth;
    }
    for entry in &mut table {
        *entry /= sum;
    }
    table
}

/// One function's table (`periodic_function_build_table`), drawing from
/// `seed` as the game does.
fn build_table(function: i16, seed: &mut RandomSeed) -> [u8; TABLE_SIZE] {
    let random_x = variable_period_x(seed);
    let mut values = [0.0f32; TABLE_SIZE];
    let mut minimum = f32::MAX;
    let mut maximum = f32::MIN;
    for (index, value) in values.iter_mut().enumerate() {
        let x = index as f32 * PERIODS_PER_ENTRY;
        let random_x = random_x[index] * VARIABLE_PERIODS;
        let wave = |x: f32| {
            let x = fraction(x);
            if x < 0.5 {
                x * 2.0
            } else {
                1.0 - (x - 0.5) * 2.0
            }
        };
        let result = match function {
            function::ONE => 1.0,
            function::ZERO => 0.0,
            function::COSINE => cos(x * TAU),
            function::COSINE_VARIABLE_PERIOD => cos(random_x * TAU),
            function::SLIDE => fraction(x),
            function::SLIDE_VARIABLE_PERIOD => fraction(random_x),
            function::DIAGONAL_WAVE => wave(x),
            function::DIAGONAL_WAVE_VARIABLE_PERIOD => wave(random_x),
            function::NOISE => seed.next_real(),
            function::JITTER | function::WANDER => {
                let fast = cos(x * 25.132_742);
                let slow = cos(x * 0.897_597_9);
                (slow * fast + cos(x * 43.9823) * sin(x * 1.570_796_4)) * 0.5
                    + sin(x * std::f32::consts::PI) * cos(x * TAU)
            }
            _ => {
                let spark = fraction(random_x);
                spark * spark
            }
        };
        maximum = maximum.max(result);
        minimum = minimum.min(result);
        *value = result;
    }
    let range = if is_slide(function) {
        0.0
    } else {
        maximum - minimum
    };
    values.map(|value| {
        let value = if range != 0.0 {
            (value - minimum) / range
        } else {
            value
        };
        ((value * 255.0) as i32).clamp(0, 255) as u8
    })
}

/// Every function's table, built in the game's order from its seed.
fn tables() -> &'static [[u8; TABLE_SIZE]; function::COUNT] {
    static TABLES: OnceLock<[[u8; TABLE_SIZE]; function::COUNT]> = OnceLock::new();
    TABLES.get_or_init(|| {
        let mut seed = RandomSeed(TABLE_SEED);
        let mut tables = [[0u8; TABLE_SIZE]; function::COUNT];
        for (function, table) in tables.iter_mut().enumerate() {
            *table = build_table(function as i16, &mut seed);
        }
        tables
    })
}

/// A periodic function at `time` periods, from 0 to 1
/// (`periodic_function_evaluate`). A function out of range is one, as the
/// port's game takes it.
pub fn evaluate(function: i16, time: f32) -> f32 {
    let Some(table) = usize::try_from(function)
        .ok()
        .filter(|&function| function != 0)
        .and_then(|function| tables().get(function))
    else {
        return 1.0;
    };
    let time = if time.is_finite() {
        time * ENTRIES_PER_PERIOD
    } else {
        0.0
    };
    let fraction = fraction(time);
    let index = ((time - fraction) as i64 as usize) & TABLE_MASK;
    let first = f32::from(table[index]) / 255.0;
    let mut second = f32::from(table[(index + 1) & TABLE_MASK]) / 255.0;
    if is_slide(function) {
        // (a slide wraps from one to zero: mixed across the wrap)
        if first > 0.75 && second < 0.25 {
            second += 1.0;
        }
        let result = (1.0 - fraction) * first + second * fraction;
        return if result > 1.0 { result - 1.0 } else { result };
    }
    (1.0 - fraction) * first + second * fraction
}

/// A periodic function at `seconds`, over a `period` and a `phase` in
/// seconds, as the shaders animate (a period of zero is one second).
pub fn animate(function: i16, seconds: f32, period: f32, phase: f32) -> f32 {
    let period = if period.is_finite() && period != 0.0 {
        period
    } else {
        1.0
    };
    evaluate(function, (seconds + phase) / period)
}

#[cfg(test)]
mod tests {
    use super::*;

    fn close(a: f32, b: f32) -> bool {
        (a - b).abs() < 1.0 / 255.0 + 1e-4
    }

    #[test]
    fn one_and_zero_and_out_of_range() {
        assert_eq!(evaluate(function::ONE, 3.7), 1.0);
        assert_eq!(evaluate(function::ZERO, 3.7), 0.0);
        assert_eq!(evaluate(-1, 0.5), 1.0);
        assert_eq!(evaluate(12, 0.5), 1.0);
        assert_eq!(evaluate(function::ZERO, f32::NAN), 0.0);
    }

    #[test]
    fn the_cosine_runs_from_one_through_zero_and_back() {
        // (worked from the builder: entry 0 is cos 0, scaled to 255; the
        // table spans 28 periods over 1024 entries, 36.57 a period, but is
        // read at 25.6 a period, as the game reads it)
        assert!(close(evaluate(function::COSINE, 0.0), 1.0));
        // entry 18 is x = 0.492 periods: cos 2π·0.492, near the bottom
        let bottom = evaluate(function::COSINE, 18.0 / 25.6);
        assert!(bottom < 0.01, "{bottom}");
        // between entries the neighbours are mixed
        let a = evaluate(function::COSINE, 4.0 / 25.6);
        let b = evaluate(function::COSINE, 5.0 / 25.6);
        let middle = evaluate(function::COSINE, 4.5 / 25.6);
        assert!(close(middle, (a + b) / 2.0));
    }

    #[test]
    fn the_diagonal_wave_and_slide_rise_and_fall() {
        // entry 9: x = 0.246 periods, the wave at 0.492, the slide at 0.246
        let wave = evaluate(function::DIAGONAL_WAVE, 9.0 / 25.6);
        assert!(close(wave, 0.4921875), "{wave}");
        let slide = evaluate(function::SLIDE, 9.0 / 25.6);
        assert!(close(slide, 0.24609375), "{slide}");
        // the slide is not rescaled (its range is taken as zero) and wraps
        // from near one to near zero without mixing through the middle
        let top = evaluate(function::SLIDE, 36.0 / 25.6);
        assert!(top > 0.9, "{top}");
        let across = evaluate(function::SLIDE, 36.5 / 25.6);
        assert!(!(0.25..=0.75).contains(&across), "{across}");
    }

    #[test]
    fn every_table_stays_in_range_and_is_made_once() {
        for function in 0..function::COUNT as i16 {
            for step in 0..200 {
                let value = evaluate(function, step as f32 * 0.37);
                assert!((0.0..=1.0).contains(&value), "{function} {value}");
            }
        }
        assert!(std::ptr::eq(tables(), tables()));
        // the noise, jitter and spark tables are not flat
        for function in [function::NOISE, function::JITTER, function::SPARK] {
            let table = &tables()[function as usize];
            assert!(table.iter().any(|&entry| entry != table[0]));
        }
        // the jitter and wander are the same function
        assert_eq!(
            tables()[function::JITTER as usize],
            tables()[function::WANDER as usize]
        );
    }

    #[test]
    fn animations_take_their_period_and_phase() {
        assert_eq!(
            animate(function::COSINE, 1.0, 2.0, 1.0),
            evaluate(function::COSINE, 1.0)
        );
        assert_eq!(
            animate(function::COSINE, 0.25, 0.0, 0.0),
            evaluate(function::COSINE, 0.25)
        );
        assert_eq!(
            animate(function::COSINE, 0.25, f32::NAN, 0.0),
            evaluate(function::COSINE, 0.25)
        );
    }
}
