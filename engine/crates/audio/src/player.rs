//! The sounds a level plays (`sound_new_impulse`): a sound's permutation is
//! chosen, its samples decoded on its first play and kept, and a voice of
//! the mixer started at the gain and pitch the original gives it. The
//! looping sounds are in [`looping`].

mod looping;

use std::collections::HashMap;
use std::sync::Arc;

use libm::sqrtf;
use opence_content::{Level, LoopingSound, Sound};

use crate::choice::{
    next_permutation, pitch_range, scale_random_value, scale_value, LocalRandom, Promoted,
    Promotion, RangeState,
};
use crate::classes::{class, ClassGains};
use crate::decode::{decode, Decoded};
use crate::mixer::{travel_delay_ms, Listener, Mixer, Placement, VoiceId, VoiceProperties};

/// The class of the music, which the music volume sets.
pub const MUSIC_CLASS: u16 = 32;
/// The classes of the scripted dialogue, which the nondialogue gain leaves
/// alone.
pub const DIALOG_CLASSES: [u16; 3] = [44, 46, 47];
/// The class whose sounds play unspatialized wherever they are made.
pub const UNSPATIALIZED_CLASS: u16 = 47;
/// The most permutations of a linked chain queued for one play.
const LINKED_LIMIT: usize = 64;

/// The sounds a player plays from: a level's, or a test's.
pub trait SoundBank: Send + Sync {
    /// The sounds, by index.
    fn sounds(&self) -> &[Sound];

    /// The looping sounds, by index.
    fn looping_sounds(&self) -> &[LoopingSound] {
        &[]
    }
}

impl SoundBank for Level {
    fn sounds(&self) -> &[Sound] {
        &self.sounds
    }

    fn looping_sounds(&self) -> &[LoopingSound] {
        &self.looping_sounds
    }
}

impl SoundBank for Vec<Sound> {
    fn sounds(&self) -> &[Sound] {
        self
    }
}

/// Where and how loudly a sound is made (`sound_source`).
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Source {
    /// Its position in the world; `None` for one heard everywhere alike.
    pub position: Option<[f32; 3]>,
    /// Its gain, 0 to 1.
    pub gain: f32,
    /// Its scale, 0 to 1, between the sound's zero and one modifiers (a
    /// footstep's speed, an impact's force).
    pub scale: f32,
    /// Its velocity, in world units a second.
    pub velocity: [f32; 3],
    /// How much of its direct path to the listener is blocked, 0 to 1.
    pub obstruction: f32,
    /// How much it is shut away from the listener, 0 to 1.
    pub occlusion: f32,
}

impl Default for Source {
    fn default() -> Self {
        Self {
            position: None,
            gain: 1.0,
            scale: 0.0,
            velocity: [0.0; 3],
            obstruction: 0.0,
            occlusion: 0.0,
        }
    }
}

/// Plays a level's sounds through a [`Mixer`].
pub struct SoundPlayer {
    bank: Arc<dyn SoundBank>,
    paths: HashMap<String, usize>,
    looping_paths: HashMap<String, usize>,
    loops: HashMap<u64, looping::Loop>,
    /// The voices of loops that ended or changed, fading out.
    retired: Vec<looping::Component>,
    decoded: HashMap<(usize, usize, u16), Option<Arc<Decoded>>>,
    ranges: HashMap<(usize, usize), RangeState>,
    promotions: HashMap<usize, Promotion>,
    classes: ClassGains,
    random: LocalRandom,
    effects_volume: f32,
    music_volume: f32,
    nondialog_gain: f32,
    time_ms: f32,
    mixer: Mixer,
}

impl SoundPlayer {
    /// A player of `bank`'s sounds.
    pub fn new(bank: Arc<dyn SoundBank>) -> Self {
        let paths = bank
            .sounds()
            .iter()
            .enumerate()
            .map(|(index, sound)| (sound.tag.path.clone(), index))
            .collect();
        let looping_paths = bank
            .looping_sounds()
            .iter()
            .enumerate()
            .map(|(index, sound)| (sound.tag.path.clone(), index))
            .collect();
        Self {
            bank,
            paths,
            looping_paths,
            loops: HashMap::new(),
            retired: Vec::new(),
            decoded: HashMap::new(),
            ranges: HashMap::new(),
            promotions: HashMap::new(),
            classes: ClassGains::default(),
            random: LocalRandom::default(),
            effects_volume: 1.0,
            music_volume: 1.0,
            nondialog_gain: 1.0,
            time_ms: 0.0,
            mixer: Mixer::new(),
        }
    }

    /// The index of the sound of the tag at `path`.
    pub fn sound_index(&self, path: &str) -> Option<usize> {
        self.paths.get(path).copied()
    }

    /// The mixer.
    pub fn mixer(&mut self) -> &mut Mixer {
        &mut self.mixer
    }

    /// The classes' gains the scripts fade.
    pub fn classes(&mut self) -> &mut ClassGains {
        &mut self.classes
    }

    /// Sets the listener: the local player's camera.
    pub fn set_listener(&mut self, listener: Listener) {
        self.mixer.set_listener(listener);
    }

    /// Sets the settings' effects and music volumes, each pinned to 0 to 1.
    pub fn set_volumes(&mut self, effects: f32, music: f32) {
        self.effects_volume = effects.clamp(0.0, 1.0);
        self.music_volume = music.clamp(0.0, 1.0);
    }

    /// Sets the gain of every sound but the scripted dialogue (lowered
    /// while a character speaks).
    pub fn set_nondialog_gain(&mut self, gain: f32) {
        self.nondialog_gain = gain.clamp(0.0, 1.0);
    }

    /// Moves the clock on by `ms` milliseconds and the classes' fades by
    /// `ticks` game ticks.
    pub fn advance(&mut self, ms: f32, ticks: i32) {
        self.time_ms += ms.max(0.0);
        self.classes.update(ticks);
    }

    /// A class's gain with the settings' volume and the nondialogue gain
    /// (`sound_manager_master_gain`).
    pub fn master_gain(&self, class_index: u16) -> f32 {
        let volume = if class_index == MUSIC_CLASS {
            self.music_volume
        } else {
            self.effects_volume
        };
        let gain = self.classes.gain(i32::from(class_index)) * volume;
        if DIALOG_CLASSES.contains(&class_index) {
            gain
        } else {
            gain * self.nondialog_gain
        }
    }

    /// The minimum and maximum distances of a sound, its class's where it
    /// gives none.
    pub fn distances(sound: &Sound) -> [f32; 2] {
        let class = class(i32::from(sound.class));
        let minimum = if sound.distance[0] == 0.0 {
            class.minimum_distance
        } else {
            sound.distance[0]
        };
        let maximum = if sound.distance[1] == 0.0 {
            class.maximum_distance
        } else {
            sound.distance[1]
        };
        [minimum, maximum]
    }

    /// Plays sound `index` from `source` once (`sound_new_impulse`): the
    /// voice playing it, or `None` when it is skipped, out of hearing, or
    /// has nothing to play.
    pub fn impulse(&mut self, index: usize, source: &Source) -> Option<VoiceId> {
        self.impulse_at(index, source, 0)
    }

    fn impulse_at(&mut self, index: usize, source: &Source, depth: u8) -> Option<VoiceId> {
        let bank = Arc::clone(&self.bank);
        let sound = bank.sounds().get(index)?;
        let scale = source.scale.clamp(0.0, 1.0);
        if scale == 0.0 && sound.zero.gain == 0.0 {
            return None;
        }
        let [minimum, maximum] = Self::distances(sound);
        let position = if sound.class == UNSPATIALIZED_CLASS {
            None
        } else {
            source.position
        };
        let distance = position.map_or(0.0, |position| {
            distance_between(position, self.mixer.listener().position)
        });
        if distance >= maximum {
            return None;
        }
        let skip = scale_value(
            sound.skip_fraction,
            sound.zero.skip_fraction,
            sound.one.skip_fraction,
            scale,
        );
        if skip > 0.0 && self.random.next_real() <= skip {
            return None;
        }
        if sound.promotion_count > 0 {
            #[allow(clippy::cast_precision_loss)]
            let longest = sound.longest_permutation_ms as f32;
            let promotion = sound
                .promotion
                .as_ref()
                .and_then(|tag| self.sound_index(&tag.path));
            let played = self.promotions.entry(index).or_default().play(
                self.time_ms,
                longest,
                sound.promotion_count,
                promotion.is_some(),
            );
            match (played, promotion) {
                (Promoted::Skip, _) => return None,
                (Promoted::Promote, Some(promoted)) if depth < 4 && promoted != index => {
                    return self.impulse_at(promoted, source, depth + 1);
                }
                _ => {}
            }
        }

        let pitch = scale_random_value(
            &mut self.random,
            sound.random_pitch_bounds,
            sound.zero.pitch,
            sound.one.pitch,
            scale,
        );
        let range = pitch_range(sound, pitch, None)?;
        let mut chain = Vec::new();
        let mut current = None;
        while chain.len() < LINKED_LIMIT {
            let state = self.ranges.entry((index, range)).or_default();
            let Some(permutation) =
                next_permutation(sound, range, state, current, &mut self.random)
            else {
                break;
            };
            chain.push(permutation);
            if sound.flags & Sound::LINKED_PERMUTATIONS == 0 {
                break;
            }
            current = Some(permutation);
            let linked = sound
                .pitch_ranges
                .get(range)
                .and_then(|range| range.permutations.get(usize::from(permutation)))
                .and_then(|permutation| permutation.next);
            if linked.is_none() {
                break;
            }
        }
        let first = *chain.first()?;
        let pitch_range = sound.pitch_ranges.get(range)?;
        let permutation = pitch_range.permutations.get(usize::from(first))?;

        let fade = 1.0;
        let gain = scale_value(
            fade * source.gain * self.master_gain(sound.class),
            sound.zero.gain,
            sound.one.gain,
            scale,
        );
        let properties = VoiceProperties {
            gain: (permutation.gain * sound.gain_modifier * gain).clamp(0.0, 1.0),
            pitch: pitch * pitch_range.playback_rate,
            placement: match position {
                Some(position) => Placement::World {
                    position,
                    distance: [minimum, f32::MAX],
                },
                None => Placement::Flat { pan: 0.0 },
            },
            velocity: source.velocity,
            obstruction: source.obstruction,
            occlusion: source.occlusion,
        };
        let delay = if position.is_some() {
            travel_delay_ms(distance)
        } else {
            0.0
        };
        let samples = self.samples(index, range, first)?;
        let voice = self.mixer.play(samples, properties, delay);
        for &next in chain.iter().skip(1) {
            if let Some(samples) = self.samples(index, range, next) {
                self.mixer.queue(voice, samples);
            }
        }
        Some(voice)
    }

    /// The decoded samples of a permutation, decoded on its first play.
    fn samples(&mut self, index: usize, range: usize, permutation: u16) -> Option<Arc<Decoded>> {
        let bank = Arc::clone(&self.bank);
        self.decoded
            .entry((index, range, permutation))
            .or_insert_with(|| {
                let sound = bank.sounds().get(index)?;
                let permutation = sound
                    .pitch_ranges
                    .get(range)?
                    .permutations
                    .get(usize::from(permutation))?;
                decode(
                    permutation.compression,
                    sound.sample_rate,
                    sound.channels,
                    &permutation.samples,
                )
                .ok()
                .map(Arc::new)
            })
            .clone()
    }

    /// Mixes the next `output.len() / 2` frames of interleaved stereo at
    /// 48 kHz; the voices that ended.
    pub fn mix(&mut self, output: &mut [f32]) -> Vec<VoiceId> {
        self.mixer.mix(output)
    }
}

fn distance_between(a: [f32; 3], b: [f32; 3]) -> f32 {
    let x = a[0] - b[0];
    let y = a[1] - b[1];
    let z = a[2] - b[2];
    sqrtf(x * x + y * y + z * z)
}

#[cfg(test)]
mod tests {
    use super::*;
    use opence_content::{
        Group, SoundCompression, SoundModifiers, SoundPermutation, SoundPitchRange, TagRef,
    };
    use std::error::Error;

    pub(super) fn permutation(value: i16, frames: usize, next: Option<u16>) -> SoundPermutation {
        SoundPermutation {
            name: String::new(),
            skip_fraction: 0.0,
            gain: 1.0,
            compression: SoundCompression::Pcm,
            next,
            samples: std::iter::repeat_n(value.to_le_bytes(), frames)
                .flatten()
                .collect(),
            mouth: Vec::new(),
        }
    }

    pub(super) fn sound(path: &str, flags: u32, permutations: Vec<SoundPermutation>) -> Sound {
        let one = SoundModifiers {
            skip_fraction: 0.0,
            gain: 1.0,
            pitch: 1.0,
        };
        Sound {
            tag: TagRef {
                group: Group(*b"snd!"),
                path: path.to_owned(),
            },
            flags,
            class: 0,
            sample_rate: 48000,
            channels: 1,
            distance: [1.0, 10.0],
            skip_fraction: 0.0,
            random_pitch_bounds: [1.0, 1.0],
            cone_angles: [0.0, 0.0],
            outer_cone_gain: 1.0,
            gain_modifier: 1.0,
            maximum_bend_per_second: 0.0,
            zero: one,
            one,
            promotion: None,
            promotion_count: 0,
            longest_permutation_ms: 0,
            pitch_ranges: vec![SoundPitchRange {
                name: String::new(),
                natural_pitch: 1.0,
                bend_bounds: [0.0, 0.0],
                actual_permutation_count: u16::try_from(permutations.len()).unwrap_or(0),
                playback_rate: 1.0,
                permutations,
            }],
        }
    }

    fn player(sounds: Vec<Sound>) -> SoundPlayer {
        SoundPlayer::new(Arc::new(sounds))
    }

    fn peak(player: &mut SoundPlayer, frames: usize) -> f32 {
        let mut output = vec![0.0; frames * 2];
        player.mix(&mut output);
        output
            .iter()
            .fold(0.0f32, |peak, &sample| peak.max(sample.abs()))
    }

    #[test]
    fn an_impulse_plays_its_samples() -> Result<(), Box<dyn Error>> {
        let mut player = player(vec![sound("a", 0, vec![permutation(16384, 4800, None)])]);
        assert_eq!(player.sound_index("a"), Some(0));
        let voice = player.impulse(0, &Source::default()).ok_or("no voice")?;
        assert!(player.mixer().is_playing(voice));
        assert!(peak(&mut player, 256) > 0.1);
        Ok(())
    }

    #[test]
    fn a_sound_out_of_hearing_does_not_play() {
        let mut player = player(vec![sound("a", 0, vec![permutation(16384, 480, None)])]);
        let far = Source {
            position: Some([50.0, 0.0, 0.0]),
            ..Source::default()
        };
        assert!(player.impulse(0, &far).is_none());
        let near = Source {
            position: Some([5.0, 0.0, 0.0]),
            ..Source::default()
        };
        assert!(player.impulse(0, &near).is_some());
        assert!(player.impulse(7, &near).is_none());
    }

    #[test]
    fn a_silent_zero_with_no_scale_does_not_play() {
        let mut quiet = sound("a", 0, vec![permutation(16384, 480, None)]);
        quiet.zero.gain = 0.0;
        let mut player = player(vec![quiet]);
        assert!(player.impulse(0, &Source::default()).is_none());
        let scaled = Source {
            scale: 1.0,
            ..Source::default()
        };
        assert!(player.impulse(0, &scaled).is_some());
    }

    #[test]
    fn a_linked_chain_queues_its_permutations() -> Result<(), Box<dyn Error>> {
        let chain = vec![
            permutation(1000, 100, Some(1)),
            permutation(2000, 100, Some(2)),
            permutation(3000, 100, None),
        ];
        let mut linked = sound("a", Sound::LINKED_PERMUTATIONS, chain);
        linked.pitch_ranges[0].actual_permutation_count = 1;
        let mut player = player(vec![linked]);
        let voice = player.impulse(0, &Source::default()).ok_or("no voice")?;
        assert_eq!(player.mixer().queued(voice), 3);
        Ok(())
    }

    #[test]
    fn the_volumes_and_classes_set_the_gain() {
        let mut player = player(Vec::new());
        player.set_volumes(0.5, 0.25);
        player.set_nondialog_gain(0.5);
        assert!((player.master_gain(0) - 0.25).abs() < 1e-6);
        assert!((player.master_gain(MUSIC_CLASS) - 0.125).abs() < 1e-6);
        assert!((player.master_gain(44) - 0.5).abs() < 1e-6);
        player.classes().set(class(0).name, 0.0, 2);
        player.advance(33.0, 2);
        assert!(player.master_gain(0).abs() < 1e-6);
    }

    #[test]
    fn a_sound_with_no_distances_takes_its_classs() {
        let mut bare = sound("a", 0, Vec::new());
        bare.distance = [0.0, 0.0];
        let expected = class(0);
        assert_eq!(
            SoundPlayer::distances(&bare),
            [expected.minimum_distance, expected.maximum_distance]
        );
    }

    #[test]
    fn a_promoted_sound_plays_its_promotion() -> Result<(), Box<dyn Error>> {
        let mut promoted = sound("a", 0, vec![permutation(1000, 100, None)]);
        promoted.promotion = Some(TagRef {
            group: Group(*b"snd!"),
            path: "b".to_owned(),
        });
        promoted.promotion_count = 1;
        promoted.longest_permutation_ms = 1000;
        let promotion = sound("b", 0, vec![permutation(30000, 4800, None)]);
        let mut player = player(vec![promoted, promotion]);
        player.impulse(0, &Source::default()).ok_or("first")?;
        player.mixer().stop_all();
        player.impulse(0, &Source::default()).ok_or("second")?;
        assert!(peak(&mut player, 64) > 0.5);
        Ok(())
    }

    #[test]
    fn a_far_sound_is_heard_after_its_travel() -> Result<(), Box<dyn Error>> {
        let mut far = sound("a", 0, vec![permutation(16384, 48000, None)]);
        far.distance = [1.0, 100.0];
        let mut player = player(vec![far]);
        let source = Source {
            position: Some([60.0, 0.0, 0.0]),
            ..Source::default()
        };
        player.impulse(0, &source).ok_or("no voice")?;
        assert!(peak(&mut player, 4800) == 0.0);
        let mut later = vec![0.0; 48000 * 2];
        player.mix(&mut later);
        assert!(later.iter().any(|sample| *sample != 0.0));
        Ok(())
    }
}
