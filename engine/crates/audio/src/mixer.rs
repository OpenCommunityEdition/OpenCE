//! The mixer: voices of decoded samples, each resampled to the output's
//! rate at its pitch, attenuated by its distance from the listener and
//! panned to its side of them as DirectSound's 3D voices are (the port's
//! `dsound_sdl.c`), summed into 48 kHz stereo.

use std::collections::VecDeque;
use std::sync::Arc;

use crate::Decoded;

/// The rate the mixer writes, a second.
pub const OUTPUT_RATE: u32 = 48_000;

/// The milliseconds sound takes to travel a world unit (10 feet), from the
/// speed of sound (`oo_speed_of_sound`).
pub const MILLISECONDS_PER_WORLD_UNIT: f32 = 8.964_706;

/// Sounds further than this many milliseconds away are delayed by their
/// travel (`speed_of_sound_threshold`).
pub const TRAVEL_DELAY_THRESHOLD_MS: f32 = 250.0;

/// DirectSound's rolloff factor, which the game sets to 1.
const ROLLOFF: f32 = 1.0;

/// Where the listener is, and which way they face. Positions are in world
/// units; the vectors need not be of unit length.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Listener {
    /// Their position.
    pub position: [f32; 3],
    /// The way they face.
    pub forward: [f32; 3],
    /// Their up.
    pub up: [f32; 3],
}

impl Default for Listener {
    fn default() -> Self {
        Self {
            position: [0.0; 3],
            forward: [1.0, 0.0, 0.0],
            up: [0.0, 0.0, 1.0],
        }
    }
}

/// Where a voice plays from.
#[derive(Clone, Copy, Debug, PartialEq)]
pub enum Placement {
    /// From a point in the world: within the minimum distance at full
    /// gain, quieter by inverse distance out to the maximum.
    World {
        /// The position.
        position: [f32; 3],
        /// The minimum and maximum distances.
        distance: [f32; 2],
    },
    /// Not in the world (the music, the interface, a radio's speech),
    /// panned from -1 (left) to 1 (right).
    Flat {
        /// The pan.
        pan: f32,
    },
}

/// What a voice plays at: its gain, pitch and place. Changed while it plays
/// with [`Mixer::set`].
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct VoiceProperties {
    /// The gain, 0 to 1 (the sound's, its permutation's, its class's and
    /// the volume's).
    pub gain: f32,
    /// The pitch: 2 plays it an octave up.
    pub pitch: f32,
    /// Where it plays from.
    pub placement: Placement,
}

/// A voice a sound plays on, to change or stop it. Stale once it ends.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub struct VoiceId {
    index: u32,
    generation: u32,
}

#[derive(Debug)]
struct Voice {
    generation: u32,
    properties: VoiceProperties,
    /// The samples playing, then those queued after them.
    queue: VecDeque<Arc<Decoded>>,
    /// The frame playing, in the samples' frames.
    cursor: f64,
    /// The output frames before it starts.
    delay: u32,
    /// The gains last mixed, to the left and right, from which the next
    /// buffer's ramp.
    gains: [f32; 2],
    /// It has been mixed once (its gains do not ramp in from silence).
    started: bool,
}

/// The mixer, which the app's audio output pulls from.
#[derive(Debug, Default)]
pub struct Mixer {
    voices: Vec<Option<Voice>>,
    generations: Vec<u32>,
    listener: Listener,
    master_gain: f32,
}

/// The gain at `distance` of a voice whose minimum and maximum distances
/// are `bounds` (DirectSound's inverse distance rolloff).
pub fn distance_attenuation(bounds: [f32; 2], distance: f32) -> f32 {
    let minimum = bounds[0];
    let maximum = bounds[1].max(minimum);
    if minimum <= 0.0 || distance <= minimum {
        return 1.0;
    }
    let distance = distance.min(maximum);
    minimum / (minimum + ROLLOFF * (distance - minimum))
}

fn sub(a: [f32; 3], b: [f32; 3]) -> [f32; 3] {
    [a[0] - b[0], a[1] - b[1], a[2] - b[2]]
}

fn dot(a: [f32; 3], b: [f32; 3]) -> f32 {
    a[0] * b[0] + a[1] * b[1] + a[2] * b[2]
}

fn cross(a: [f32; 3], b: [f32; 3]) -> [f32; 3] {
    [
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    ]
}

fn normalize(a: [f32; 3]) -> [f32; 3] {
    let length = libm::sqrtf(dot(a, a));
    if length < 1e-6 {
        [0.0; 3]
    } else {
        [a[0] / length, a[1] / length, a[2] / length]
    }
}

/// The equal-power gains to the left and right of a pan from -1 to 1, each
/// 1 at the centre's √½·√2.
fn pan_gains(pan: f32) -> [f32; 2] {
    let angle = (pan.clamp(-1.0, 1.0) + 1.0) * 0.25 * core::f32::consts::PI;
    let scale = core::f32::consts::SQRT_2;
    [libm::cosf(angle) * scale, libm::sinf(angle) * scale]
}

/// The gains to the left and right of a voice placed so (the port's
/// `stream_3d_gains`): its distance's attenuation, and a pan by how far to
/// the side of the listener it is, softened as it nears them.
pub fn placement_gains(listener: &Listener, placement: Placement) -> [f32; 2] {
    match placement {
        Placement::Flat { pan } => pan_gains(pan),
        Placement::World { position, distance } => {
            let offset = sub(position, listener.position);
            let range = libm::sqrtf(dot(offset, offset));
            let attenuation = distance_attenuation(distance, range);
            let front = normalize(listener.forward);
            let up = normalize(listener.up);
            // DirectSound's space is left-handed: its right is up × front.
            let right = normalize(cross(front, up));
            let side = dot(offset, right);
            let ahead = dot(offset, front);
            let horizontal = libm::sqrtf(side * side + ahead * ahead);
            let mut pan = if horizontal < 1e-4 {
                0.0
            } else {
                side / horizontal
            };
            if distance[0] > 0.0 && range < distance[0] {
                pan *= range / distance[0];
            }
            let [left, right] = pan_gains(pan * 0.75);
            let centre = core::f32::consts::FRAC_1_SQRT_2;
            [left * centre * attenuation, right * centre * attenuation]
        }
    }
}

/// The milliseconds a sound `distance` world units away is heard after it
/// is made, or 0 if it is near enough to be heard at once.
pub fn travel_delay_ms(distance: f32) -> f32 {
    let milliseconds = distance * MILLISECONDS_PER_WORLD_UNIT;
    if milliseconds > TRAVEL_DELAY_THRESHOLD_MS {
        milliseconds
    } else {
        0.0
    }
}

impl Mixer {
    /// A mixer with no voices, at full gain.
    pub fn new() -> Self {
        Self {
            master_gain: 1.0,
            ..Self::default()
        }
    }

    /// Moves the listener.
    pub fn set_listener(&mut self, listener: Listener) {
        self.listener = listener;
    }

    /// The listener.
    pub fn listener(&self) -> &Listener {
        &self.listener
    }

    /// Sets the gain every voice is mixed at.
    pub fn set_master_gain(&mut self, gain: f32) {
        self.master_gain = gain.clamp(0.0, 1.0);
    }

    /// How many voices play.
    pub fn voice_count(&self) -> usize {
        self.voices.iter().filter(|v| v.is_some()).count()
    }

    /// Plays `samples` after `delay_ms` milliseconds.
    pub fn play(
        &mut self,
        samples: Arc<Decoded>,
        properties: VoiceProperties,
        delay_ms: f32,
    ) -> VoiceId {
        let delay = (delay_ms.max(0.0) * OUTPUT_RATE as f32 / 1000.0) as u32;
        let mut queue = VecDeque::new();
        queue.push_back(samples);
        let index = self
            .voices
            .iter()
            .position(Option::is_none)
            .unwrap_or_else(|| {
                self.voices.push(None);
                self.generations.push(0);
                self.voices.len() - 1
            });
        let generation = self.generations.get(index).copied().unwrap_or(0);
        if let Some(slot) = self.voices.get_mut(index) {
            *slot = Some(Voice {
                generation,
                properties,
                queue,
                cursor: 0.0,
                delay,
                gains: [0.0; 2],
                started: false,
            });
        }
        VoiceId {
            index: u32::try_from(index).unwrap_or(u32::MAX),
            generation,
        }
    }

    fn voice_mut(&mut self, id: VoiceId) -> Option<&mut Voice> {
        self.voices
            .get_mut(usize::try_from(id.index).ok()?)?
            .as_mut()
            .filter(|v| v.generation == id.generation)
    }

    /// Whether a voice plays (or waits to).
    pub fn is_playing(&self, id: VoiceId) -> bool {
        usize::try_from(id.index)
            .ok()
            .and_then(|i| self.voices.get(i))
            .and_then(Option::as_ref)
            .is_some_and(|v| v.generation == id.generation)
    }

    /// Changes what a voice plays at.
    pub fn set(&mut self, id: VoiceId, properties: VoiceProperties) {
        if let Some(voice) = self.voice_mut(id) {
            voice.properties = properties;
        }
    }

    /// Queues samples to play after those the voice plays.
    pub fn queue(&mut self, id: VoiceId, samples: Arc<Decoded>) {
        if let Some(voice) = self.voice_mut(id) {
            voice.queue.push_back(samples);
        }
    }

    /// How many samples a voice has queued, those playing among them.
    pub fn queued(&self, id: VoiceId) -> usize {
        usize::try_from(id.index)
            .ok()
            .and_then(|i| self.voices.get(i))
            .and_then(Option::as_ref)
            .filter(|v| v.generation == id.generation)
            .map_or(0, |v| v.queue.len())
    }

    /// Stops a voice.
    pub fn stop(&mut self, id: VoiceId) {
        let Ok(index) = usize::try_from(id.index) else {
            return;
        };
        if self.is_playing(id) {
            self.end(index);
        }
    }

    /// Stops every voice.
    pub fn stop_all(&mut self) {
        for index in 0..self.voices.len() {
            if self.voices.get(index).is_some_and(Option::is_some) {
                self.end(index);
            }
        }
    }

    fn end(&mut self, index: usize) {
        if let Some(slot) = self.voices.get_mut(index) {
            *slot = None;
        }
        if let Some(generation) = self.generations.get_mut(index) {
            *generation = generation.wrapping_add(1);
        }
    }

    /// Mixes the next `output.len() / 2` frames of interleaved stereo into
    /// `output`, which it overwrites; the voices that ended.
    pub fn mix(&mut self, output: &mut [f32]) -> Vec<VoiceId> {
        output.fill(0.0);
        let frames = output.len() / 2;
        let mut ended = Vec::new();
        for index in 0..self.voices.len() {
            let Some(Some(voice)) = self.voices.get_mut(index) else {
                continue;
            };
            let done = mix_voice(voice, &self.listener, self.master_gain, output, frames);
            if done {
                ended.push(VoiceId {
                    index: u32::try_from(index).unwrap_or(u32::MAX),
                    generation: voice.generation,
                });
                self.end(index);
            }
        }
        ended
    }
}

/// Mixes a voice into the frames; whether it has ended.
fn mix_voice(
    voice: &mut Voice,
    listener: &Listener,
    master_gain: f32,
    output: &mut [f32],
    frames: usize,
) -> bool {
    let skip = usize::try_from(voice.delay)
        .unwrap_or(usize::MAX)
        .min(frames);
    voice.delay -= u32::try_from(skip).unwrap_or(0);
    if skip == frames {
        return false;
    }
    let gain = voice.properties.gain.max(0.0) * master_gain;
    let target = placement_gains(listener, voice.properties.placement).map(|g| g * gain);
    let from = if voice.started { voice.gains } else { target };
    voice.started = true;
    voice.gains = target;
    let span = frames - skip;
    let ramp = 1.0 / span as f32;
    let mut frame = skip;
    while frame < frames {
        let Some(samples) = voice.queue.front().cloned() else {
            return true;
        };
        let length = samples.frames();
        let channels = usize::from(samples.channels.max(1));
        let step = f64::from(samples.sample_rate) * f64::from(voice.properties.pitch.max(0.0))
            / f64::from(OUTPUT_RATE);
        while frame < frames {
            let whole = voice.cursor as usize;
            if whole >= length {
                break;
            }
            let fraction = (voice.cursor - whole as f64) as f32;
            let next = (whole + 1).min(length - 1);
            let read = |at: usize, channel: usize| {
                samples
                    .samples
                    .get(at * channels + channel.min(channels - 1))
                    .map_or(0.0, |s| f32::from(*s) / 32768.0)
            };
            let left = read(whole, 0) + (read(next, 0) - read(whole, 0)) * fraction;
            let right = read(whole, 1) + (read(next, 1) - read(whole, 1)) * fraction;
            let t = (frame - skip) as f32 * ramp;
            let gain_left = from[0] + (target[0] - from[0]) * t;
            let gain_right = from[1] + (target[1] - from[1]) * t;
            if channels == 1 {
                if let Some(out) = output.get_mut(frame * 2..frame * 2 + 2) {
                    out[0] += left * gain_left;
                    out[1] += left * gain_right;
                }
            } else if let Some(out) = output.get_mut(frame * 2..frame * 2 + 2) {
                // a stereo sound keeps its sides, at the louder side's gain
                let both = gain_left.max(gain_right);
                out[0] += left * both;
                out[1] += right * both;
            }
            voice.cursor += step;
            frame += 1;
        }
        if voice.cursor as usize >= length {
            voice.cursor -= length as f64;
            voice.queue.pop_front();
            if voice.queue.is_empty() {
                return true;
            }
        }
    }
    false
}

#[cfg(test)]
mod tests {
    use super::*;

    fn tone(frames: usize, value: i16, channels: u8) -> Arc<Decoded> {
        Arc::new(Decoded {
            sample_rate: OUTPUT_RATE,
            channels,
            samples: vec![value; frames * usize::from(channels)],
        })
    }

    fn flat(gain: f32) -> VoiceProperties {
        VoiceProperties {
            gain,
            pitch: 1.0,
            placement: Placement::Flat { pan: 0.0 },
        }
    }

    #[test]
    fn distance_attenuates_by_its_inverse() {
        assert!((distance_attenuation([1.0, 10.0], 0.5) - 1.0).abs() < 1e-6);
        assert!((distance_attenuation([1.0, 10.0], 2.0) - 0.5).abs() < 1e-6);
        assert!((distance_attenuation([1.0, 10.0], 4.0) - 0.25).abs() < 1e-6);
        // held at the maximum's
        assert!((distance_attenuation([1.0, 10.0], 100.0) - 0.1).abs() < 1e-6);
        assert!((distance_attenuation([0.0, 10.0], 5.0) - 1.0).abs() < 1e-6);
        assert!((distance_attenuation([2.0, 1.0], 5.0) - 1.0).abs() < 1e-6);
    }

    #[test]
    fn voices_pan_to_their_side_with_equal_power() {
        let listener = Listener::default();
        let centre = placement_gains(&listener, Placement::Flat { pan: 0.0 });
        assert!((centre[0] - 1.0).abs() < 1e-5 && (centre[1] - 1.0).abs() < 1e-5);
        let left = placement_gains(&listener, Placement::Flat { pan: -1.0 });
        assert!((left[0] - core::f32::consts::SQRT_2).abs() < 1e-5 && left[1].abs() < 1e-5);
        // facing +x with z up, +y is to the left
        let world = |position| Placement::World {
            position,
            distance: [1.0, 100.0],
        };
        let ahead = placement_gains(&listener, world([5.0, 0.0, 0.0]));
        assert!((ahead[0] - ahead[1]).abs() < 1e-5);
        assert!((ahead[0] - core::f32::consts::FRAC_1_SQRT_2 * 0.2).abs() < 1e-5);
        let to_the_left = placement_gains(&listener, world([0.0, 5.0, 0.0]));
        assert!(to_the_left[0] > to_the_left[1] * 3.0);
        let to_the_right = placement_gains(&listener, world([0.0, -5.0, 0.0]));
        assert!(to_the_right[1] > to_the_right[0] * 3.0);
        let power = to_the_left[0] * to_the_left[0] + to_the_left[1] * to_the_left[1];
        let half = 0.2 * core::f32::consts::FRAC_1_SQRT_2;
        assert!((power - 2.0 * half * half).abs() < 1e-5);
        // near the listener the pan softens
        let near = placement_gains(&listener, world([0.0, 0.1, 0.0]));
        assert!(near[0] > near[1] && near[0] < near[1] * 1.5);
        let overhead = placement_gains(&listener, world([0.0, 0.0, 5.0]));
        assert!((overhead[0] - overhead[1]).abs() < 1e-5);
    }

    #[test]
    fn far_sounds_arrive_later() {
        assert!(travel_delay_ms(20.0).abs() < 1e-6);
        assert!((travel_delay_ms(100.0) - 896.470_64).abs() < 1e-3);
    }

    #[test]
    fn a_voice_plays_its_samples_then_ends() {
        let mut mixer = Mixer::new();
        let id = mixer.play(tone(100, 16384, 1), flat(1.0), 0.0);
        assert!(mixer.is_playing(id));
        let mut output = vec![0.0; 128];
        let ended = mixer.mix(&mut output);
        assert!(ended.is_empty());
        assert!(output.iter().all(|s| (s - 0.5).abs() < 1e-5));
        let ended = mixer.mix(&mut output);
        assert_eq!(ended, vec![id]);
        assert!(!mixer.is_playing(id));
        // 100 frames: 64, then 36
        assert!(output
            .get(..72)
            .is_some_and(|s| s.iter().all(|v| (v - 0.5).abs() < 1e-5)));
        assert!(output
            .get(72..)
            .is_some_and(|s| s.iter().all(|v| *v == 0.0)));
        // a stale id changes nothing
        mixer.set(id, flat(0.0));
        mixer.stop(id);
        assert_eq!(mixer.voice_count(), 0);
    }

    #[test]
    fn pitch_resamples_and_queues_follow_on() {
        let mut mixer = Mixer::new();
        let mut properties = flat(1.0);
        properties.pitch = 2.0;
        let id = mixer.play(tone(64, 8192, 1), properties, 0.0);
        mixer.queue(id, tone(64, 16384, 2));
        assert_eq!(mixer.queued(id), 2);
        let mut output = vec![0.0; 128];
        mixer.mix(&mut output);
        // at twice the pitch, 32 frames of each
        assert!(output
            .get(..64)
            .is_some_and(|s| s.iter().all(|v| (v - 0.25).abs() < 1e-3)));
        assert!(output
            .get(64..)
            .is_some_and(|s| s.iter().all(|v| (v - 0.5).abs() < 1e-3)));
        assert!(!mixer.is_playing(id) || mixer.queued(id) == 0);
    }

    #[test]
    fn delays_gains_and_stops() {
        let mut mixer = Mixer::new();
        // 1 ms is 48 frames
        let id = mixer.play(tone(1000, 16384, 1), flat(0.5), 1.0);
        let mut output = vec![0.0; 128];
        mixer.mix(&mut output);
        assert!(output
            .get(..96)
            .is_some_and(|s| s.iter().all(|v| *v == 0.0)));
        assert!(output
            .get(96..)
            .is_some_and(|s| s.iter().all(|v| (v - 0.25).abs() < 1e-5)));
        mixer.set_master_gain(0.5);
        mixer.mix(&mut output);
        // ramps down to the new gain over the buffer
        assert!(output.first().is_some_and(|v| (v - 0.25).abs() < 1e-3));
        assert!(output.last().is_some_and(|v| (v - 0.125).abs() < 1e-2));
        mixer.mix(&mut output);
        assert!(output.iter().all(|v| (v - 0.125).abs() < 1e-5));
        mixer.stop(id);
        assert!(!mixer.is_playing(id));
        let again = mixer.play(tone(1000, 16384, 1), flat(1.0), 0.0);
        assert_ne!(again, id, "a reused voice is a new id");
        mixer.play(tone(1000, 16384, 1), flat(1.0), 0.0);
        assert_eq!(mixer.voice_count(), 2);
        mixer.stop_all();
        assert_eq!(mixer.voice_count(), 0);
        mixer.set_listener(Listener::default());
        assert_eq!(*mixer.listener(), Listener::default());
    }

    #[test]
    fn a_hundred_and_twenty_eight_voices_mix_in_time() {
        let mut mixer = Mixer::new();
        let samples = Arc::new(Decoded {
            sample_rate: 22_050,
            channels: 1,
            samples: (0..22_050)
                .map(|i| i16::try_from(i % 2000).unwrap_or(0))
                .collect(),
        });
        for voice in 0..128u16 {
            let angle = f32::from(voice) * 0.3;
            mixer.play(
                samples.clone(),
                VoiceProperties {
                    gain: 0.1,
                    pitch: 0.8 + f32::from(voice % 8) * 0.05,
                    placement: Placement::World {
                        position: [libm::cosf(angle) * 10.0, libm::sinf(angle) * 10.0, 0.0],
                        distance: [1.0, 50.0],
                    },
                },
                0.0,
            );
        }
        // a buffer of 512 frames, about 10.7 ms
        let mut output = vec![0.0; 1024];
        mixer.mix(&mut output);
        let start = std::time::Instant::now();
        for _ in 0..20 {
            mixer.mix(&mut output);
        }
        let each = start.elapsed() / 20;
        assert_eq!(mixer.voice_count(), 128);
        assert!(
            each.as_micros() < 2000 || cfg!(debug_assertions),
            "a buffer took {each:?}"
        );
    }
}
