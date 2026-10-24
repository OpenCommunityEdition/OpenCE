//! The reverb of the listener's surroundings: a sound environment's (`snde`)
//! room, reflections and decay, applied to what the voices in the world
//! send it. The Xbox's I3DL2 reverb is approximated by a Schroeder and
//! Moorer reverb (four damped combs and two allpasses a side, as Freeverb's)
//! after a delay line for the early reflections.

use crate::mixer::OUTPUT_RATE;

/// A sound environment, its fields as the tag's (`sound_environment`): the
/// intensities are linear gains from 0 to 1, the times seconds.
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct Environment {
    /// The room's gain, which the reflections and reverb are heard through.
    pub room_intensity: f32,
    /// The room's gain at high frequencies.
    pub room_intensity_hf: f32,
    /// How the room's effect falls off with distance (unused: the send is
    /// the voice's own gain).
    pub room_rolloff: f32,
    /// The seconds the reverb takes to fall by 60 dB.
    pub decay_time: f32,
    /// The decay at high frequencies to that at low.
    pub decay_hf_ratio: f32,
    /// The early reflections' gain.
    pub reflections_intensity: f32,
    /// The seconds before the first reflection.
    pub reflections_delay: f32,
    /// The late reverb's gain.
    pub reverb_intensity: f32,
    /// The seconds after the first reflection before the late reverb.
    pub reverb_delay: f32,
    /// The echo density of the late reverb, 0 to 1.
    pub diffusion: f32,
    /// The modal density of the late reverb, 0 to 1.
    pub density: f32,
    /// The frequency the high-frequency values are taken at, in hertz.
    pub hf_reference: f32,
}

/// How far each of an environment's values moves toward the next
/// environment's in an update, in the tag's order
/// (`sound_environment_update`).
const GLIDE_STEPS: [f32; 12] = [
    0.03, 0.03, 0.3, 0.1, 0.03, 0.03, 0.09, 0.03, 0.003, 0.03, 0.03, 600.0,
];

impl Environment {
    /// The environment of a place without one (`default_sound_environment`):
    /// no room, so no reverb.
    pub const DEFAULT: Self = Self {
        room_intensity: 0.0,
        room_intensity_hf: 0.0,
        room_rolloff: 0.0,
        decay_time: 1.0,
        decay_hf_ratio: 0.5,
        reflections_intensity: 0.0,
        reflections_delay: 0.02,
        reverb_intensity: 0.0,
        reverb_delay: 0.04,
        diffusion: 1.0,
        density: 1.0,
        hf_reference: 5000.0,
    };

    /// The environment of the tag's twelve values, in its order.
    pub fn from_values(values: [f32; 12]) -> Self {
        let [room_intensity, room_intensity_hf, room_rolloff, decay_time, decay_hf_ratio, reflections_intensity, reflections_delay, reverb_intensity, reverb_delay, diffusion, density, hf_reference] =
            values;
        Self {
            room_intensity,
            room_intensity_hf,
            room_rolloff,
            decay_time,
            decay_hf_ratio,
            reflections_intensity,
            reflections_delay,
            reverb_intensity,
            reverb_delay,
            diffusion,
            density,
            hf_reference,
        }
    }

    /// Its twelve values, in the tag's order.
    pub fn values(&self) -> [f32; 12] {
        [
            self.room_intensity,
            self.room_intensity_hf,
            self.room_rolloff,
            self.decay_time,
            self.decay_hf_ratio,
            self.reflections_intensity,
            self.reflections_delay,
            self.reverb_intensity,
            self.reverb_delay,
            self.diffusion,
            self.density,
            self.hf_reference,
        ]
    }

    /// Moves it toward `target` by one update's steps, as the game glides
    /// from one room's sound to the next; whether it has arrived.
    pub fn glide_towards(&mut self, target: &Self) -> bool {
        let mut values = self.values();
        for ((value, goal), step) in values.iter_mut().zip(target.values()).zip(GLIDE_STEPS) {
            *value += (goal - *value).clamp(-step, step);
        }
        *self = Self::from_values(values);
        values == target.values()
    }

    /// Whether it is heard at all: the default environment, of zeros, is
    /// no reverb.
    pub fn is_audible(&self) -> bool {
        self.room_intensity > 0.0
            && (self.reflections_intensity > 0.0 || self.reverb_intensity > 0.0)
            && self.decay_time > 0.0
    }
}

/// Freeverb's comb and allpass lengths at 44.1 kHz.
const COMBS: [usize; 4] = [1116, 1188, 1277, 1356];
const ALLPASSES: [usize; 2] = [556, 441];
/// The right side's lengths are longer by so much, to decorrelate them.
const SPREAD: usize = 23;
/// I3DL2's longest reflection and reverb delays, in seconds.
const MAXIMUM_REFLECTIONS_DELAY: f32 = 0.3;
const MAXIMUM_REVERB_DELAY: f32 = 0.1;

fn at_output_rate(length: usize) -> usize {
    length * OUTPUT_RATE as usize / 44_100
}

#[derive(Debug, Clone)]
struct Comb {
    buffer: Vec<f32>,
    at: usize,
    feedback: f32,
    damp: f32,
    store: f32,
}

impl Comb {
    fn new(length: usize) -> Self {
        Self {
            buffer: vec![0.0; length.max(1)],
            at: 0,
            feedback: 0.0,
            damp: 0.0,
            store: 0.0,
        }
    }

    fn process(&mut self, input: f32) -> f32 {
        let Some(slot) = self.buffer.get_mut(self.at) else {
            return 0.0;
        };
        let output = *slot;
        self.store = output * (1.0 - self.damp) + self.store * self.damp;
        *slot = input + self.store * self.feedback;
        self.at = (self.at + 1) % self.buffer.len();
        output
    }
}

#[derive(Debug, Clone)]
struct Allpass {
    buffer: Vec<f32>,
    at: usize,
    feedback: f32,
}

impl Allpass {
    fn new(length: usize) -> Self {
        Self {
            buffer: vec![0.0; length.max(1)],
            at: 0,
            feedback: 0.5,
        }
    }

    fn process(&mut self, input: f32) -> f32 {
        let Some(slot) = self.buffer.get_mut(self.at) else {
            return input;
        };
        let delayed = *slot;
        *slot = input + delayed * self.feedback;
        self.at = (self.at + 1) % self.buffer.len();
        delayed - input
    }
}

/// A delay line, read at two taps: the reflections' and the reverb's.
#[derive(Debug, Clone)]
struct Delay {
    buffer: Vec<f32>,
    at: usize,
}

impl Delay {
    fn new(length: usize) -> Self {
        Self {
            buffer: vec![0.0; length.max(1)],
            at: 0,
        }
    }

    fn write(&mut self, input: f32) {
        if let Some(slot) = self.buffer.get_mut(self.at) {
            *slot = input;
        }
        self.at = (self.at + 1) % self.buffer.len();
    }

    /// The input `frames` frames ago (0 is the one last written).
    fn tap(&self, frames: usize) -> f32 {
        let length = self.buffer.len();
        let back = frames.min(length - 1) + 1;
        self.buffer
            .get((self.at + length - back) % length)
            .copied()
            .unwrap_or(0.0)
    }
}

#[derive(Debug, Clone)]
struct Side {
    delay: Delay,
    combs: Vec<Comb>,
    allpasses: Vec<Allpass>,
    /// The send's high-frequency lowpass state.
    tone: f32,
}

impl Side {
    fn new(spread: usize) -> Self {
        let reach = (MAXIMUM_REFLECTIONS_DELAY + MAXIMUM_REVERB_DELAY) * OUTPUT_RATE as f32;
        Self {
            delay: Delay::new(reach as usize + 1),
            combs: COMBS
                .iter()
                .map(|length| Comb::new(at_output_rate(length + spread)))
                .collect(),
            allpasses: ALLPASSES
                .iter()
                .map(|length| Allpass::new(at_output_rate(length + spread)))
                .collect(),
            tone: 0.0,
        }
    }
}

/// The reverb. Its buffers are made once, so that mixing it allocates
/// nothing.
#[derive(Debug, Clone)]
pub struct Reverb {
    environment: Option<Environment>,
    sides: [Side; 2],
    reflections_frames: usize,
    reverb_frames: usize,
    reflections_gain: f32,
    reverb_gain: f32,
    /// The send's one-pole lowpass coefficient, from the room's
    /// high-frequency gain.
    tone: f32,
}

impl Default for Reverb {
    fn default() -> Self {
        Self::new()
    }
}

/// How much of the reverb is heard: I3DL2's late reverb at full gain is
/// far quieter than a comb bank's sum.
const WET_SCALE: f32 = 0.015;

impl Reverb {
    /// A reverb in no environment, which is silent.
    pub fn new() -> Self {
        Self {
            environment: None,
            sides: [Side::new(0), Side::new(SPREAD)],
            reflections_frames: 0,
            reverb_frames: 0,
            reflections_gain: 0.0,
            reverb_gain: 0.0,
            tone: 0.0,
        }
    }

    /// The environment it reverberates.
    pub fn environment(&self) -> Option<&Environment> {
        self.environment.as_ref()
    }

    /// Changes the environment; `None`, or one that is not audible, is
    /// silent. Its tails are kept, so that a change is not a click.
    pub fn set_environment(&mut self, environment: Option<Environment>) {
        if self.environment == environment {
            return;
        }
        self.environment = environment.filter(Environment::is_audible);
        let Some(environment) = self.environment else {
            self.reflections_gain = 0.0;
            self.reverb_gain = 0.0;
            return;
        };
        let rate = OUTPUT_RATE as f32;
        self.reflections_frames = (environment
            .reflections_delay
            .clamp(0.0, MAXIMUM_REFLECTIONS_DELAY)
            * rate) as usize;
        self.reverb_frames = self.reflections_frames
            + (environment.reverb_delay.clamp(0.0, MAXIMUM_REVERB_DELAY) * rate) as usize;
        let room = environment.room_intensity.clamp(0.0, 1.0);
        // (the Xbox lets the reflections reach +10 dB and the reverb +20)
        self.reflections_gain = room * environment.reflections_intensity.clamp(0.0, 1.0) * 3.16;
        self.reverb_gain = room * environment.reverb_intensity.clamp(0.0, 1.0) * 10.0;
        self.tone = 1.0 - environment.room_intensity_hf.clamp(0.0, 1.0);
        let decay = environment.decay_time.max(0.1);
        // the high frequencies decay by the ratio's time: damped by so much
        let damp = (1.0 - environment.decay_hf_ratio.clamp(0.1, 2.0)).clamp(0.0, 0.9);
        let diffusion = environment.diffusion.clamp(0.0, 1.0);
        for side in &mut self.sides {
            for comb in &mut side.combs {
                let seconds = comb.buffer.len() as f32 / rate;
                // 60 dB down after the decay time: 10^(-3·delay/decay)
                comb.feedback = libm::expf(-6.907_755 * seconds / decay).min(0.98);
                comb.damp = damp;
            }
            for allpass in &mut side.allpasses {
                allpass.feedback = 0.3 + 0.4 * diffusion;
            }
        }
    }

    /// Adds the reverb of `send`, interleaved stereo, into `output`.
    pub fn process(&mut self, send: &[f32], output: &mut [f32]) {
        if self.environment.is_none() {
            return;
        }
        let (reflections, reverb) = (self.reflections_frames, self.reverb_frames);
        let (reflections_gain, reverb_gain, tone) =
            (self.reflections_gain, self.reverb_gain, self.tone);
        for (input, out) in send
            .as_chunks::<2>()
            .0
            .iter()
            .zip(output.as_chunks_mut::<2>().0.iter_mut())
        {
            for (channel, side) in self.sides.iter_mut().enumerate() {
                let (Some(input), Some(out)) = (input.get(channel), out.get_mut(channel)) else {
                    continue;
                };
                side.tone = input * (1.0 - tone) + side.tone * tone;
                side.delay.write(side.tone);
                let early = side.delay.tap(reflections);
                let late_input = side.delay.tap(reverb);
                let mut late = 0.0;
                for comb in &mut side.combs {
                    late += comb.process(late_input);
                }
                for allpass in &mut side.allpasses {
                    late = allpass.process(late);
                }
                *out += early * reflections_gain * 0.25 + late * reverb_gain * WET_SCALE;
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn an_environment_glides_to_the_next_a_step_at_a_time() {
        let room = Environment {
            room_intensity: 0.1,
            decay_time: 1.25,
            hf_reference: 5900.0,
            ..Environment::DEFAULT
        };
        assert_eq!(Environment::from_values(room.values()), room);
        let mut heard = Environment::DEFAULT;
        assert!(!heard.glide_towards(&room));
        assert!((heard.room_intensity - 0.03).abs() < 1e-6);
        assert!((heard.decay_time - 1.1).abs() < 1e-6);
        assert!((heard.hf_reference - 5600.0).abs() < 1e-3);
        let mut updates = 1;
        while !heard.glide_towards(&room) {
            updates += 1;
            assert!(updates < 10, "the glide never arrives");
        }
        assert_eq!(heard, room);
        assert!(heard.glide_towards(&room), "an arrived glide stays");
        assert!(!Environment::DEFAULT.is_audible());
    }

    fn hall() -> Environment {
        Environment {
            room_intensity: 1.0,
            room_intensity_hf: 0.8,
            decay_time: 2.0,
            decay_hf_ratio: 0.8,
            reflections_intensity: 0.5,
            reflections_delay: 0.01,
            reverb_intensity: 0.5,
            reverb_delay: 0.02,
            diffusion: 1.0,
            density: 1.0,
            hf_reference: 5000.0,
            ..Environment::default()
        }
    }

    fn energy(samples: &[f32]) -> f32 {
        samples.iter().map(|s| s * s).sum()
    }

    /// An impulse, then silence, through the reverb in `environment`: the
    /// energy of each of `blocks` blocks of 4800 frames.
    fn tail(environment: Option<Environment>, blocks: usize) -> Vec<f32> {
        let mut reverb = Reverb::new();
        reverb.set_environment(environment);
        let mut send = vec![0.0; 9600];
        send[0] = 1.0;
        send[1] = 1.0;
        let mut energies = Vec::new();
        for _ in 0..blocks {
            let mut output = vec![0.0; 9600];
            reverb.process(&send, &mut output);
            send.fill(0.0);
            energies.push(energy(&output));
        }
        energies
    }

    #[test]
    fn no_environment_is_silent() {
        assert!(tail(None, 2).iter().all(|e| *e == 0.0));
        assert!(tail(Some(Environment::default()), 2)
            .iter()
            .all(|e| *e == 0.0));
        assert!(!Environment::default().is_audible());
        assert!(hall().is_audible());
    }

    #[test]
    fn a_room_rings_and_decays_by_its_time() -> Result<(), Box<dyn std::error::Error>> {
        let long = tail(Some(hall()), 20);
        let short = tail(
            Some(Environment {
                decay_time: 0.3,
                ..hall()
            }),
            20,
        );
        let first = long.first().copied().ok_or("no blocks")?;
        assert!(first > 0.0);
        // it falls, and a longer decay rings on longer
        let last = long.last().copied().ok_or("no blocks")?;
        assert!(last < first);
        assert!(last > 0.0);
        let short_last = short.last().copied().ok_or("no blocks")?;
        assert!(short_last < last * 1e-3);
        Ok(())
    }

    #[test]
    fn the_reflections_arrive_after_their_delay() {
        let mut reverb = Reverb::new();
        reverb.set_environment(Some(Environment {
            reverb_intensity: 0.0,
            reflections_delay: 0.05,
            ..hall()
        }));
        let mut send = vec![0.0; 9600];
        send[0] = 1.0;
        let mut output = vec![0.0; 9600];
        reverb.process(&send, &mut output);
        let first = output.iter().position(|s| s.abs() > 1e-6).unwrap_or(0);
        // 50 ms is 2400 frames, the left's sample 4800
        assert_eq!(first, 4800);
    }

    #[test]
    fn changing_the_environment_keeps_the_tail() {
        let mut reverb = Reverb::new();
        reverb.set_environment(Some(hall()));
        let mut send = vec![0.0; 960];
        send[0] = 1.0;
        let mut output = vec![0.0; 960];
        reverb.process(&send, &mut output);
        assert_eq!(reverb.environment(), Some(&hall()));
        reverb.set_environment(Some(Environment {
            decay_time: 1.0,
            ..hall()
        }));
        send.fill(0.0);
        let mut later = vec![0.0; 9600];
        reverb.process(&vec![0.0; 9600], &mut later);
        assert!(energy(&later) > 0.0);
        reverb.set_environment(None);
        let mut silent = vec![0.0; 960];
        reverb.process(&send, &mut silent);
        assert!(silent.iter().all(|s| *s == 0.0));
    }
}
