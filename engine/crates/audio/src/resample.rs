//! A decoded permutation brought to the output's rate once, as it is
//! decoded. The mixer steps through a voice's samples by linear
//! interpolation; on samples at 22.05 or 44.1 kHz that leaves images of
//! the sound above the source's Nyquist frequency, which a band-limited
//! resampler does not. At the output's rate the mixer's step is the
//! pitch alone, and linear interpolation of a pitch near 1 is close to
//! exact.

use rubato::audioadapter_buffers::direct::InterleavedSlice;
use rubato::{Fft, FixedSync, Resampler};

use crate::decode::Decoded;
use crate::mixer::OUTPUT_RATE;

/// The frames the resampler takes a call.
const CHUNK: usize = 1024;

/// `decoded` at [`OUTPUT_RATE`]. Samples already at it, and any the
/// resampler refuses (a rate of 0), are given back as they are, for the
/// mixer to interpolate.
pub fn to_output_rate(decoded: Decoded) -> Decoded {
    if decoded.sample_rate == OUTPUT_RATE || decoded.samples.is_empty() {
        return decoded;
    }
    resampled(&decoded).unwrap_or(decoded)
}

fn resampled(decoded: &Decoded) -> Option<Decoded> {
    let channels = usize::from(decoded.channels.max(1));
    let frames = decoded.frames();
    let mut resampler = Fft::<f32>::new(
        usize::try_from(decoded.sample_rate).ok()?,
        OUTPUT_RATE as usize,
        CHUNK,
        channels,
        FixedSync::Input,
    )
    .ok()?;
    let input: Vec<f32> = decoded
        .samples
        .iter()
        .take(frames * channels)
        .map(|&sample| f32::from(sample) / 32768.0)
        .collect();
    let input = InterleavedSlice::new(&input, channels, frames).ok()?;
    let output = resampler.process_all(&input, frames, None).ok()?;
    let samples = output
        .take_data()
        .into_iter()
        .map(|sample| (sample * 32768.0).round().clamp(-32768.0, 32767.0) as i16)
        .collect();
    Some(Decoded {
        sample_rate: OUTPUT_RATE,
        channels: decoded.channels,
        samples,
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    fn sine(rate: u32, frequency: f64, frames: usize) -> Vec<f64> {
        (0..frames)
            .map(|i| {
                16000.0
                    * libm::sin(2.0 * std::f64::consts::PI * frequency * i as f64 / f64::from(rate))
            })
            .collect()
    }

    /// The worst error of `samples` from the sine they should be, away
    /// from the ends.
    fn worst(samples: &[f64], frequency: f64) -> f64 {
        let expected = sine(OUTPUT_RATE, frequency, samples.len());
        let edge = samples.len() / 10;
        samples[edge..samples.len() - edge]
            .iter()
            .zip(&expected[edge..])
            .map(|(got, want)| (got - want).abs())
            .fold(0.0, f64::max)
    }

    /// What the mixer's linear interpolation makes of the samples at 48 kHz.
    fn linear(samples: &[i16], rate: u32, frames: usize) -> Vec<f64> {
        let step = f64::from(rate) / f64::from(OUTPUT_RATE);
        (0..frames)
            .map(|frame| {
                let at = frame as f64 * step;
                let whole = at as usize;
                let a = f64::from(samples.get(whole).copied().unwrap_or(0));
                let b = f64::from(samples.get(whole + 1).copied().unwrap_or(0));
                a + (b - a) * (at - whole as f64)
            })
            .collect()
    }

    #[test]
    fn a_sine_resampled_is_the_sine_at_the_output_rate_and_closer_than_linear() {
        for (rate, frequency) in [
            (22_050, 1000.0),
            (22_050, 7000.0),
            (44_100, 5000.0),
            (44_100, 15000.0),
        ] {
            let frames = rate as usize;
            let samples: Vec<i16> = sine(rate, frequency, frames)
                .into_iter()
                .map(|sample| sample.round() as i16)
                .collect();
            let resampled = to_output_rate(Decoded {
                sample_rate: rate,
                channels: 1,
                samples: samples.clone(),
            });
            assert_eq!(resampled.sample_rate, OUTPUT_RATE);
            let out_frames = frames * OUTPUT_RATE as usize / rate as usize;
            assert!(
                resampled.frames().abs_diff(out_frames) <= 1,
                "{rate} Hz: {} frames",
                resampled.frames()
            );
            let got: Vec<f64> = resampled.samples.iter().map(|&s| f64::from(s)).collect();
            let band_limited = worst(&got, frequency);
            let interpolated = worst(&linear(&samples, rate, resampled.frames()), frequency);
            assert!(
                band_limited < 4.0,
                "{rate} Hz, {frequency} Hz: off by {band_limited}"
            );
            assert!(
                band_limited < interpolated,
                "{rate} Hz, {frequency} Hz: {band_limited} against linear {interpolated}"
            );
        }
    }

    #[test]
    fn stereo_keeps_its_channels_apart() {
        let left = sine(22_050, 1000.0, 22_050);
        let samples: Vec<i16> = left.iter().flat_map(|&l| [l.round() as i16, 0]).collect();
        let resampled = to_output_rate(Decoded {
            sample_rate: 22_050,
            channels: 2,
            samples,
        });
        assert_eq!(resampled.channels, 2);
        let right_loudest = resampled
            .samples
            .iter()
            .skip(1)
            .step_by(2)
            .map(|s| s.unsigned_abs())
            .max();
        assert!(right_loudest.unwrap_or(0) <= 2, "{right_loudest:?}");
        let left: Vec<f64> = resampled
            .samples
            .iter()
            .step_by(2)
            .map(|&s| f64::from(s))
            .collect();
        assert!(worst(&left, 1000.0) < 4.0);
    }

    #[test]
    fn samples_at_the_output_rate_or_at_no_rate_are_left_alone() {
        for sample_rate in [OUTPUT_RATE, 0] {
            let decoded = Decoded {
                sample_rate,
                channels: 1,
                samples: vec![1, 2, 3],
            };
            assert_eq!(to_output_rate(decoded.clone()), decoded);
        }
    }
}
