//! The fixed tick, and how far the display is between the last two ticks.

use crate::units::Seconds;

/// The simulation's rate.
pub const TICKS_PER_SECOND: u32 = 60;
/// One tick.
pub const TICK: Seconds = Seconds(1.0 / TICKS_PER_SECOND as f32);

/// The most ticks one frame runs: after a long stall (a breakpoint, the app
/// in the background) the world slows down rather than running a burst of
/// ticks that each make the next frame later still.
pub const MAX_TICKS_PER_FRAME: u32 = 8;

/// Turns the display's frame times into whole ticks.
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct Clock {
    /// The time not yet run, in ticks (kept in f64 so that hours of frames
    /// do not drift; it is not part of the simulation, so it does not need
    /// to be deterministic).
    pending: f64,
}

/// What a frame should do.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Frame {
    /// The ticks to run now.
    pub ticks: u32,
    /// Where the display is between the tick before the last (0) and the
    /// last (1).
    pub alpha: f32,
}

impl Clock {
    /// Adds a frame's time and says how many ticks to run.
    pub fn advance(&mut self, frame_seconds: f64) -> Frame {
        let frame_seconds = if frame_seconds.is_finite() && frame_seconds > 0.0 {
            frame_seconds
        } else {
            0.0
        };
        self.pending += frame_seconds * f64::from(TICKS_PER_SECOND);
        // (pending is finite and non-negative)
        let whole = self.pending.floor();
        let ticks = if whole >= f64::from(MAX_TICKS_PER_FRAME) {
            self.pending = 0.0;
            MAX_TICKS_PER_FRAME
        } else {
            self.pending -= whole;
            whole as u32
        };
        Frame {
            ticks,
            // (a fraction just short of a tick narrows to 1.0 in f32; the
            // display is never at the next tick yet)
            alpha: (self.pending as f32).min(1.0 - f32::EPSILON / 2.0),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn run(frames: &[f64]) -> (u32, f32) {
        let mut clock = Clock::default();
        let mut ticks = 0;
        let mut alpha = 0.0;
        for &frame in frames {
            let step = clock.advance(frame);
            ticks += step.ticks;
            alpha = step.alpha;
        }
        (ticks, alpha)
    }

    #[test]
    fn a_second_of_frames_at_any_rate_is_sixty_ticks() {
        for rate in [30.0, 60.0, 120.0, 144.0, 240.0] {
            let frames = vec![1.0 / rate; rate as usize];
            let (ticks, alpha) = run(&frames);
            // (the last tick lands within rounding of the frame's end)
            assert!(
                ticks == 60 || (ticks == 59 && alpha > 0.999),
                "{rate} Hz: {ticks} ticks, alpha {alpha}"
            );
        }
    }

    #[test]
    fn alpha_is_the_fraction_of_a_tick_left_over() {
        let mut clock = Clock::default();
        let frame = clock.advance(1.5 / 60.0);
        assert_eq!(frame.ticks, 1);
        assert!((frame.alpha - 0.5).abs() < 1e-6);
        let frame = clock.advance(0.25 / 60.0);
        assert_eq!(frame.ticks, 0);
        assert!((frame.alpha - 0.75).abs() < 1e-6);
    }

    #[test]
    fn uneven_frames_keep_alpha_in_range_and_lose_no_time() {
        // a frame pattern from a display that misses deadlines
        let pattern = [0.004, 0.021, 0.0083, 0.0167, 0.033, 0.001, 0.012];
        let mut clock = Clock::default();
        let mut ticks = 0u32;
        let mut total = 0.0;
        for i in 0..7000 {
            let seconds = pattern[i % pattern.len()];
            total += seconds;
            let frame = clock.advance(seconds);
            assert!((0.0..1.0).contains(&frame.alpha), "alpha {}", frame.alpha);
            ticks += frame.ticks;
        }
        let expected = total * 60.0;
        assert!((f64::from(ticks) + f64::from(clock.pending as f32) - expected).abs() < 1e-3);
    }

    #[test]
    fn a_stall_runs_at_most_the_cap() {
        let mut clock = Clock::default();
        assert_eq!(
            clock.advance(10.0),
            Frame {
                ticks: MAX_TICKS_PER_FRAME,
                alpha: 0.0
            }
        );
    }

    #[test]
    fn nonsense_frame_times_run_nothing() {
        let mut clock = Clock::default();
        for seconds in [f64::NAN, f64::INFINITY, -1.0, 0.0] {
            assert_eq!(clock.advance(seconds).ticks, 0);
        }
    }

    #[test]
    fn alpha_stays_below_one() {
        let mut clock = Clock::default();
        for _ in 0..100_000 {
            let frame = clock.advance(1.0 / 144.0);
            assert!((0.0..1.0).contains(&frame.alpha), "{}", frame.alpha);
        }
        let frame = Clock {
            pending: 1.0 - 1e-12,
        }
        .advance(0.0);
        assert_eq!(frame.ticks, 0);
        assert!(frame.alpha < 1.0);
    }
}
