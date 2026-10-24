//! Looping sounds (`sound_refresh_looping`, `process_looping_sounds`): a
//! level's machines, engines and backgrounds. Each track plays its start,
//! then its loop permutation after permutation on one voice, then its stop;
//! the alternate switches a track to its other loop, after the permutation
//! playing or by a crossfade; the details play now and again around the
//! source. A loop plays while it is refreshed between updates, and stops
//! when it is not.

use std::sync::Arc;

use libm::{cosf, sinf};
use opence_content::{LoopingSound, LoopingSoundDetail, LoopingSoundTrack, TagRef};

use super::{distance_between, SoundPlayer, Source, UNSPATIALIZED_CLASS};
use crate::census::{Dropped, Playing};
use crate::choice::{next_permutation, pitch_range, scale_value};
use crate::decode::Decoded;
use crate::mixer::{Placement, VoiceId, VoiceProperties};

/// The milliseconds a loop's voice fades out over as it leaves hearing
/// (`sound_inaudible_fade_out_time`).
const INAUDIBLE_FADE_OUT_MS: f32 = 2000.0;
/// The milliseconds it fades back in over when it is heard again, and an
/// old loop fades out over when another takes its place.
const FADE_BACK_IN_MS: f32 = 500.0;
/// How many permutations a looping voice keeps queued, the one playing
/// among them, so that it never runs dry between updates.
const QUEUE_DEPTH: usize = 2;

/// A linear fade of a gain.
#[derive(Clone, Copy, Debug, PartialEq)]
struct Fade {
    from: f32,
    to: f32,
    start_ms: f32,
    length_ms: f32,
}

impl Fade {
    const FULL: Self = Self::at(1.0);

    const fn at(value: f32) -> Self {
        Self {
            from: value,
            to: value,
            start_ms: 0.0,
            length_ms: 0.0,
        }
    }

    fn value(&self, now: f32) -> f32 {
        if self.done(now) {
            self.to
        } else {
            let t = ((now - self.start_ms) / self.length_ms).clamp(0.0, 1.0);
            self.from + (self.to - self.from) * t
        }
    }

    /// From its value now to `to` over `length_ms`.
    fn towards(&self, now: f32, to: f32, length_ms: f32) -> Self {
        Self {
            from: self.value(now),
            to,
            start_ms: now,
            length_ms: length_ms.max(0.0),
        }
    }

    fn done(&self, now: f32) -> bool {
        self.length_ms <= 0.0 || now >= self.start_ms + self.length_ms
    }

    /// It has faded out.
    fn silent(&self, now: f32) -> bool {
        self.to <= 0.0 && self.done(now)
    }
}

/// One voice of a track: a start or stop played once, a loop, or both.
#[derive(Clone, Debug)]
pub(super) struct Component {
    track: usize,
    /// Played once before the loop.
    head: Option<usize>,
    /// Played over and over after the head; `None` ends the voice after
    /// what it has queued.
    sound: Option<usize>,
    /// The sound whose gain and pitch it plays at.
    gain_sound: usize,
    /// The track's voice the alternate and the stop act on.
    primary: bool,
    voice: Option<VoiceId>,
    /// It has been heard (it fades back in when heard again).
    heard: bool,
    range: usize,
    pitch: f32,
    fade: Fade,
    /// Its fade out of hearing and back.
    hearing: Fade,
    /// Its first permutation's gain.
    permutation_gain: f32,
    /// The gain last set, before its fade.
    gain: f32,
}

impl Component {
    fn new(track: usize, head: Option<usize>, sound: Option<usize>, fade: Fade) -> Option<Self> {
        Some(Self {
            track,
            head,
            sound,
            gain_sound: sound.or(head)?,
            primary: true,
            voice: None,
            heard: false,
            range: 0,
            pitch: 1.0,
            fade,
            hearing: Fade::FULL,
            permutation_gain: 1.0,
            gain: 0.0,
        })
    }

    /// Fades it out over `length_ms`, looping on until it is silent.
    fn retire(&mut self, now: f32, length_ms: f32) {
        self.primary = false;
        self.fade = self.fade.towards(now, 0.0, length_ms);
    }
}

/// A looping sound playing.
#[derive(Clone, Debug)]
pub(super) struct Loop {
    index: usize,
    source: Source,
    alternate: bool,
    stopping: bool,
    refreshed: bool,
    components: Vec<Component>,
    /// When each detail next plays.
    details: Vec<f32>,
}

impl SoundPlayer {
    /// The index of the looping sound of the tag at `path`.
    pub fn looping_index(&self, path: &str) -> Option<usize> {
        self.looping_paths.get(path).copied()
    }

    /// How many looping sounds play, those stopping among them.
    pub fn looping_count(&self) -> usize {
        self.loops.len()
    }

    /// Keeps looping sound `index` playing from `source` under `key` (an
    /// object's attachment, the background): it starts on its first
    /// refresh, and its tracks switch loops as `alternate` changes. A loop
    /// not refreshed between two [`SoundPlayer::update_loops`] stops.
    pub fn refresh_looping(&mut self, key: u64, index: usize, source: &Source, alternate: bool) {
        let bank = Arc::clone(&self.bank);
        let Some(definition) = bank.looping_sounds().get(index) else {
            return;
        };
        let now = self.time_ms;
        if let Some(playing) = self.loops.get_mut(&key) {
            if playing.index == index && !playing.stopping {
                playing.source = *source;
                playing.refreshed = true;
                if playing.alternate != alternate {
                    playing.alternate = alternate;
                    self.switch_alternate(key, definition);
                }
                return;
            }
        }
        // another sound under the key, or this one stopping: what plays of
        // the old fades out, or finishes on its own
        if let Some(old) = self.loops.remove(&key) {
            let replaced = old.index != index;
            self.retired
                .extend(old.components.into_iter().map(|mut component| {
                    if replaced {
                        component.retire(now, FADE_BACK_IN_MS);
                    }
                    component
                }));
        }
        let scale = source.scale.clamp(0.0, 1.0);
        let details = definition
            .details
            .iter()
            .map(|detail| self.detail_period(definition, detail, scale) + now)
            .collect();
        let components = definition
            .tracks
            .iter()
            .enumerate()
            .flat_map(|(track, track_definition)| {
                self.start_track(track, track_definition, alternate)
            })
            .collect();
        self.loops.insert(
            key,
            Loop {
                index,
                source: *source,
                alternate,
                stopping: false,
                refreshed: true,
                components,
                details,
            },
        );
    }

    /// Stops the looping sound under `key`: its tracks fade, or play their
    /// stops, as each says.
    pub fn stop_looping(&mut self, key: u64) {
        let bank = Arc::clone(&self.bank);
        let now = self.time_ms;
        let Some(mut playing) = self.loops.remove(&key) else {
            return;
        };
        if playing.stopping {
            self.loops.insert(key, playing);
            return;
        }
        playing.stopping = true;
        let Some(definition) = bank.looping_sounds().get(playing.index) else {
            self.stop_components(&mut playing.components);
            return;
        };
        let fake_impulse = definition.flags & LoopingSound::FAKE_IMPULSE != 0;
        let mut stops = Vec::new();
        for (track, track_definition) in definition.tracks.iter().enumerate() {
            let fade_at_stop = track_definition.flags & LoopingSoundTrack::FADE_OUT_AT_STOP != 0;
            let stop = if playing.alternate {
                track_definition
                    .alternate_stop
                    .as_ref()
                    .or(track_definition.stop.as_ref())
            } else {
                track_definition.stop.as_ref()
            };
            let stop = self.find(stop);
            if fade_at_stop || (stop.is_none() && !fake_impulse) {
                for component in playing
                    .components
                    .iter_mut()
                    .filter(|component| component.track == track)
                {
                    component.retire(now, track_definition.fade_out * 1000.0);
                }
            }
            let primary = playing
                .components
                .iter_mut()
                .find(|component| component.track == track && component.primary);
            match (stop, primary) {
                (Some(stop), _) if fade_at_stop => {
                    stops.extend(Component::new(track, Some(stop), None, Fade::FULL));
                }
                (stop, Some(primary)) => {
                    // the stop follows the permutation playing
                    primary.sound = None;
                    primary.primary = false;
                    if let Some(voice) = primary.voice {
                        self.mixer.truncate(voice);
                        if let Some(samples) = stop.and_then(|stop| self.first_samples(stop)) {
                            self.mixer.queue(voice, samples);
                        }
                    }
                }
                (Some(stop), None) => {
                    stops.extend(Component::new(track, Some(stop), None, Fade::FULL));
                }
                (None, None) => {}
            }
        }
        playing.components.extend(stops);
        self.loops.insert(key, playing);
    }

    /// Stops every looping sound at once (the level ends).
    pub fn stop_all_loops(&mut self) {
        let mut components: Vec<Component> = self
            .loops
            .drain()
            .flat_map(|(_, playing)| playing.components)
            .collect();
        components.append(&mut self.retired);
        self.stop_components(&mut components);
    }

    fn stop_components(&mut self, components: &mut Vec<Component>) {
        for component in components.drain(..) {
            if let Some(voice) = component.voice {
                self.mixer.stop(voice);
            }
        }
    }

    fn find(&self, tag: Option<&TagRef>) -> Option<usize> {
        tag.and_then(|tag| self.paths.get(&tag.path).copied())
    }

    /// The loop a track plays with the alternate on or off.
    fn track_loop(&self, definition: &LoopingSoundTrack, alternate: bool) -> Option<usize> {
        let looping = self.find(definition.looping.as_ref());
        if alternate {
            self.find(definition.alternate_loop.as_ref()).or(looping)
        } else {
            looping
        }
    }

    /// The components a track starts with: its start then its loop on one
    /// voice, or, when it fades in, its start over the loop fading in.
    fn start_track(
        &self,
        track: usize,
        definition: &LoopingSoundTrack,
        alternate: bool,
    ) -> Vec<Component> {
        let start = self.find(definition.start.as_ref());
        let looping = self.track_loop(definition, alternate);
        if definition.flags & LoopingSoundTrack::FADE_IN_AT_START == 0 {
            return Component::new(track, start, looping, Fade::FULL)
                .into_iter()
                .collect();
        }
        let fade = Fade::at(0.0).towards(self.time_ms, 1.0, definition.fade_in * 1000.0);
        let head = start.and_then(|start| {
            let mut head = Component::new(track, Some(start), None, Fade::FULL)?;
            head.primary = false;
            Some(head)
        });
        head.into_iter()
            .chain(Component::new(track, None, looping, fade))
            .collect()
    }

    /// Switches each track of the loop under `key` to the loop of its
    /// alternate's new state.
    fn switch_alternate(&mut self, key: u64, definition: &LoopingSound) {
        let now = self.time_ms;
        let Some(mut playing) = self.loops.remove(&key) else {
            return;
        };
        for (track, track_definition) in definition.tracks.iter().enumerate() {
            let Some(looping) = self.track_loop(track_definition, playing.alternate) else {
                continue;
            };
            let Some(primary) = playing
                .components
                .iter_mut()
                .find(|component| component.track == track && component.primary)
            else {
                continue;
            };
            if primary.sound == Some(looping) {
                continue;
            }
            if track_definition.flags & LoopingSoundTrack::FADE_IN_ALTERNATE != 0 {
                // the old loop fades out under the new fading in
                let fade_ms = track_definition.fade_out * 1000.0;
                primary.retire(now, fade_ms);
                playing.components.extend(Component::new(
                    track,
                    None,
                    Some(looping),
                    Fade::at(0.0).towards(now, 1.0, fade_ms),
                ));
            } else {
                // the new loop follows the permutation playing
                primary.sound = Some(looping);
                primary.gain_sound = looping;
                if let Some(sound) = self.bank.sounds().get(looping) {
                    primary.range = pitch_range(sound, primary.pitch, None).unwrap_or(0);
                }
                if let Some(voice) = primary.voice {
                    self.mixer.truncate(voice);
                }
            }
        }
        self.loops.insert(key, playing);
    }

    /// Moves the loops on: those not refreshed since the last update stop,
    /// voices fade and follow their sources, leave hearing and come back,
    /// and queue their next permutations, and the details play.
    pub fn update_loops(&mut self) {
        let bank = Arc::clone(&self.bank);
        let now = self.time_ms;
        let keys: Vec<u64> = self.loops.keys().copied().collect();
        for key in keys {
            if self
                .loops
                .get(&key)
                .is_some_and(|playing| !playing.stopping && !playing.refreshed)
            {
                self.stop_looping(key);
            }
            let Some(mut playing) = self.loops.remove(&key) else {
                continue;
            };
            let Some(definition) = bank.looping_sounds().get(playing.index) else {
                self.stop_components(&mut playing.components);
                continue;
            };
            let source = playing.source;
            let mut components = std::mem::take(&mut playing.components);
            components.retain_mut(|component| {
                definition
                    .tracks
                    .get(component.track)
                    .is_some_and(|track| self.update_component(component, track, &source))
            });
            playing.components = components;
            if !playing.stopping {
                self.play_details(&mut playing, definition);
            }
            playing.refreshed = false;
            if !(playing.stopping && playing.components.is_empty()) {
                self.loops.insert(key, playing);
            }
        }
        let mut retired = std::mem::take(&mut self.retired);
        retired.retain(|component| {
            let Some(voice) = component.voice else {
                return false;
            };
            if !self.mixer.is_playing(voice) || component.fade.silent(now) {
                self.mixer.stop(voice);
                return false;
            }
            self.refill(component, voice);
            if let Some(mut properties) = self.mixer.properties(voice) {
                properties.gain = component.gain * component.fade.value(now);
                self.mixer.set(voice, properties);
            }
            true
        });
        self.retired = retired;
    }

    /// Queues a looping voice's next permutations.
    fn refill(&mut self, component: &Component, voice: VoiceId) {
        if let Some(looping) = component.sound {
            while self.mixer.queued(voice) < QUEUE_DEPTH {
                let Some(samples) = self.next_samples(looping, component.range) else {
                    break;
                };
                self.mixer.queue(voice, samples);
            }
        }
    }

    /// Updates a component's voice; whether it plays on.
    fn update_component(
        &mut self,
        component: &mut Component,
        track: &LoopingSoundTrack,
        source: &Source,
    ) -> bool {
        let bank = Arc::clone(&self.bank);
        let now = self.time_ms;
        let Some(sound) = bank.sounds().get(component.gain_sound) else {
            return false;
        };
        if component.fade.silent(now) {
            if let Some(voice) = component.voice {
                self.mixer.stop(voice);
            }
            return false;
        }
        let [minimum, maximum] = Self::distances(sound);
        let listener = self.mixer.listener().position;
        let audible = source
            .position
            .is_none_or(|position| distance_between(position, listener) < maximum);
        match component.voice {
            Some(voice) if !self.mixer.is_playing(voice) => return false,
            Some(voice) if !audible => {
                if component.hearing.to > 0.0 {
                    component.hearing = component.hearing.towards(now, 0.0, INAUDIBLE_FADE_OUT_MS);
                } else if component.hearing.done(now) {
                    // out of hearing it stops; a loop starts again when it
                    // is heard, a start or a stop is not heard later
                    self.mixer.stop(voice);
                    component.voice = None;
                    component.head = None;
                    return component.sound.is_some();
                }
            }
            Some(_) => {
                if component.hearing.to < 1.0 {
                    component.hearing = component.hearing.towards(now, 1.0, FADE_BACK_IN_MS);
                }
            }
            None if !audible => {
                component.head = None;
                return component.sound.is_some();
            }
            None => {
                if !self.start_component(component, source) {
                    return false;
                }
                component.hearing = if component.heard {
                    Fade::at(0.0).towards(now, 1.0, FADE_BACK_IN_MS)
                } else {
                    Fade::FULL
                };
                component.heard = true;
            }
        }
        let Some(voice) = component.voice else {
            return true;
        };
        self.refill(component, voice);
        let scale = source.scale.clamp(0.0, 1.0);
        component.gain = (self.master_gain(sound.class)
            * track.gain
            * scale_value(sound.gain_modifier, sound.zero.gain, sound.one.gain, scale)
            * source.gain
            * component.permutation_gain
            * component.hearing.value(now))
        .clamp(0.0, 1.0);
        let playback = sound
            .pitch_ranges
            .get(component.range)
            .map_or(1.0, |range| range.playback_rate);
        self.mixer.set(
            voice,
            VoiceProperties {
                gain: component.gain * component.fade.value(now),
                pitch: scale_value(component.pitch, sound.zero.pitch, sound.one.pitch, scale)
                    * playback,
                ..properties(source, minimum)
            },
        );
        true
    }

    /// Starts a component's voice, silent until its update sets its gain;
    /// whether it has one.
    fn start_component(&mut self, component: &mut Component, source: &Source) -> bool {
        let bank = Arc::clone(&self.bank);
        let Some(sound) = bank.sounds().get(component.gain_sound) else {
            return false;
        };
        let pitch = self.random.between(sound.random_pitch_bounds);
        let Some(range) = pitch_range(sound, pitch, None) else {
            self.census.dropped(sound.class, Dropped::Failed);
            return false;
        };
        component.pitch = pitch;
        component.range = range;
        component.permutation_gain = sound
            .pitch_ranges
            .get(range)
            .and_then(|range| range.permutations.first())
            .map_or(1.0, |permutation| permutation.gain);
        let mut queue = Vec::new();
        if let Some(head) = component.head.take() {
            queue.extend(self.first_samples(head));
        }
        if let Some(looping) = component.sound {
            queue.extend(self.next_samples(looping, range));
        }
        let mut queue = queue.into_iter();
        let Some(first) = queue.next() else {
            self.census.dropped(sound.class, Dropped::Failed);
            return false;
        };
        let voice = self.mixer.play(
            first,
            VoiceProperties {
                gain: 0.0,
                ..properties(source, Self::distances(sound)[0])
            },
            0.0,
        );
        for samples in queue {
            self.mixer.queue(voice, samples);
        }
        component.voice = Some(voice);
        self.census.played(Playing {
            voice,
            sound: component.gain_sound,
            class: sound.class,
            position: (sound.class != UNSPATIALIZED_CLASS)
                .then_some(source.position)
                .flatten(),
        });
        true
    }

    /// The next permutation of sound `index`, at its natural pitch's range.
    fn first_samples(&mut self, index: usize) -> Option<Arc<Decoded>> {
        let range = pitch_range(self.bank.sounds().get(index)?, 1.0, None)?;
        self.next_samples(index, range)
    }

    /// The next permutation of sound `index` in pitch range `range`.
    fn next_samples(&mut self, index: usize, range: usize) -> Option<Arc<Decoded>> {
        let bank = Arc::clone(&self.bank);
        let sound = bank.sounds().get(index)?;
        let state = self.ranges.entry((index, range)).or_default();
        let permutation = next_permutation(sound, range, state, None, &mut self.random)?;
        self.samples(index, range, permutation)
    }

    /// The milliseconds until a detail plays again: its period, scaled
    /// between the loop's at 0 and at 1.
    fn detail_period(
        &mut self,
        definition: &LoopingSound,
        detail: &LoopingSoundDetail,
        scale: f32,
    ) -> f32 {
        let lerp = (definition.detail_period[1] - definition.detail_period[0]) * scale
            + definition.detail_period[0];
        lerp * self.random.between(detail.period_bounds) * 1000.0
    }

    /// Plays the details that are due, each from a random place around the
    /// source (the listener, for a loop heard everywhere).
    fn play_details(&mut self, playing: &mut Loop, definition: &LoopingSound) {
        let now = self.time_ms;
        let scale = playing.source.scale.clamp(0.0, 1.0);
        let listener = self.mixer.listener().position;
        let mut times = std::mem::take(&mut playing.details);
        for (detail, time) in definition.details.iter().zip(times.iter_mut()) {
            if *time > now {
                continue;
            }
            let index = self.find(detail.sound.as_ref());
            let allowed = if playing.alternate {
                detail.flags & LoopingSoundDetail::NOT_WITH_ALTERNATE == 0
            } else {
                detail.flags & LoopingSoundDetail::NOT_WITHOUT_ALTERNATE == 0
            };
            if let (Some(index), true) = (index, allowed) {
                let offset = self.detail_offset(detail);
                let origin = playing.source.position.unwrap_or(listener);
                let source = Source {
                    position: Some([
                        origin[0] + offset[0],
                        origin[1] + offset[1],
                        origin[2] + offset[2],
                    ]),
                    gain: detail.gain,
                    scale,
                    ..playing.source
                };
                self.impulse(index, &source);
            }
            #[allow(clippy::cast_precision_loss)]
            let longest = index
                .and_then(|index| self.bank.sounds().get(index))
                .map_or(0.0, |sound| sound.longest_permutation_ms as f32);
            *time = self.detail_period(definition, detail, scale) + longest + now;
        }
        playing.details = times;
    }

    /// A detail's random offset from its source
    /// (`detail_sound_random_offset`): a random distance along a random yaw
    /// and pitch.
    fn detail_offset(&mut self, detail: &LoopingSoundDetail) -> [f32; 3] {
        let distance = self.random.between(detail.distance_bounds);
        if distance == 0.0 {
            return [0.0; 3];
        }
        let pitch = self.random.between(detail.phi_bounds);
        let yaw = self.random.between(detail.theta_bounds);
        [
            cosf(yaw) * cosf(pitch) * distance,
            sinf(yaw) * cosf(pitch) * distance,
            sinf(pitch) * distance,
        ]
    }
}

/// A loop's voice's placement and what stands between it and the
/// listener; its gain and pitch are the caller's.
fn properties(source: &Source, minimum: f32) -> VoiceProperties {
    VoiceProperties {
        placement: match source.position {
            // a loop is heard to any distance once in hearing
            Some(position) => Placement::World {
                position,
                distance: [minimum, f32::MAX],
            },
            None => Placement::Flat { pan: 0.0 },
        },
        velocity: source.velocity,
        obstruction: source.obstruction,
        occlusion: source.occlusion,
        ..VoiceProperties::default()
    }
}

#[cfg(test)]
mod tests {
    use super::super::tests::{permutation, sound};
    use super::super::SoundBank;
    use super::*;
    use opence_content::{Group, Sound};
    use std::error::Error;

    struct Bank {
        sounds: Vec<Sound>,
        looping: Vec<LoopingSound>,
    }

    impl SoundBank for Bank {
        fn sounds(&self) -> &[Sound] {
            &self.sounds
        }

        fn looping_sounds(&self) -> &[LoopingSound] {
            &self.looping
        }
    }

    fn tag(path: &str) -> Option<TagRef> {
        Some(TagRef {
            group: Group(*b"snd!"),
            path: path.to_owned(),
        })
    }

    fn track(flags: u32) -> LoopingSoundTrack {
        LoopingSoundTrack {
            flags,
            gain: 1.0,
            fade_in: 1.0,
            fade_out: 1.0,
            start: None,
            looping: tag("hum"),
            stop: None,
            alternate_loop: None,
            alternate_stop: None,
        }
    }

    fn looping(path: &str, tracks: Vec<LoopingSoundTrack>) -> LoopingSound {
        LoopingSound {
            tag: TagRef {
                group: Group(*b"lsnd"),
                path: path.to_owned(),
            },
            flags: 0,
            detail_period: [1.0, 1.0],
            tracks,
            details: Vec::new(),
        }
    }

    /// The sounds `hum` (0.1), `whine` (0.2), `start` (0.3) and `stop`
    /// (0.4), each a tenth of a second.
    fn player(looping: Vec<LoopingSound>) -> SoundPlayer {
        let tone = |path, value| sound(path, 0, vec![permutation(value, 4800, None)]);
        let sounds = vec![
            tone("hum", 3277),
            tone("whine", 6554),
            tone("start", 9830),
            tone("stop", 13107),
        ];
        SoundPlayer::new(Arc::new(Bank { sounds, looping }))
    }

    /// Mixes `frames` frames: the mean of their absolute samples.
    fn level(player: &mut SoundPlayer, frames: usize) -> f32 {
        let mut output = vec![0.0; frames * 2];
        player.mix(&mut output);
        #[allow(clippy::cast_precision_loss)]
        let count = output.len() as f32;
        output.iter().map(|sample| sample.abs()).sum::<f32>() / count
    }

    /// The level of a flat voice of `value` at the centre, which plays
    /// it whole on both sides.
    fn flat(value: f32) -> f32 {
        value
    }

    /// Advances `ms`, refreshing loop 0 under key 1 first when `refresh`
    /// gives its alternate.
    fn step(player: &mut SoundPlayer, ms: f32, refresh: Option<bool>) {
        if let Some(alternate) = refresh {
            player.refresh_looping(1, 0, &Source::default(), alternate);
        }
        player.advance(ms, 1);
        player.update_loops();
    }

    #[test]
    fn a_loop_plays_on_and_stops_when_not_refreshed() -> Result<(), Box<dyn Error>> {
        let mut player = player(vec![looping("machine", vec![track(0)])]);
        assert_eq!(player.looping_index("machine"), Some(0));
        assert_eq!(player.looping_index("missing"), None);
        // an index without a looping sound is not played
        player.refresh_looping(1, 7, &Source::default(), false);
        assert_eq!(player.looping_count(), 0);
        for _ in 0..10 {
            step(&mut player, 50.0, Some(false));
            let heard = level(&mut player, 2400);
            if (heard - flat(0.1)).abs() > 1e-3 {
                return Err(format!("{heard}").into());
            }
        }
        // half a second of a tenth-second sound: it went on looping
        assert_eq!(player.mixer().voice_count(), 1);
        // (one voice in the census, though it played five times)
        assert_eq!(player.census().playing().len(), 1);
        // not refreshed, a track with no stop fades out over its second
        step(&mut player, 50.0, None);
        assert_eq!(player.looping_count(), 1);
        step(&mut player, 500.0, None);
        // (the mixer ramps to a new gain over a mix)
        level(&mut player, 480);
        let half = level(&mut player, 480);
        assert!((half - flat(0.05)).abs() < 1e-3, "{half}");
        step(&mut player, 600.0, None);
        assert_eq!(player.looping_count(), 0);
        assert_eq!(player.mixer().voice_count(), 0);
        assert!(player.census().playing().is_empty());
        Ok(())
    }

    #[test]
    fn a_start_plays_before_the_loop_and_a_stop_after_it() {
        let mut machine = track(0);
        machine.start = tag("start");
        machine.stop = tag("stop");
        let mut player = player(vec![looping("machine", vec![machine])]);
        step(&mut player, 10.0, Some(false));
        let start = level(&mut player, 4000);
        assert!((start - flat(0.3)).abs() < 1e-3, "{start}");
        level(&mut player, 800);
        step(&mut player, 10.0, Some(false));
        let hum = level(&mut player, 2400);
        assert!((hum - flat(0.1)).abs() < 1e-3, "{hum}");
        // stopped, the stop follows the permutation playing
        player.stop_looping(1);
        step(&mut player, 10.0, None);
        level(&mut player, 2400);
        let after = level(&mut player, 4800);
        assert!((after - flat(0.4)).abs() < 1e-3, "{after}");
        step(&mut player, 10.0, None);
        assert_eq!(player.looping_count(), 0);
        assert_eq!(player.mixer().voice_count(), 0);
    }

    #[test]
    fn a_track_fades_in_at_its_start_and_its_stop_plays_apart() {
        let mut machine =
            track(LoopingSoundTrack::FADE_IN_AT_START | LoopingSoundTrack::FADE_OUT_AT_STOP);
        machine.start = tag("start");
        machine.stop = tag("stop");
        let mut player = player(vec![looping("machine", vec![machine])]);
        step(&mut player, 0.0, Some(false));
        // the start at full, the loop at none
        assert_eq!(player.mixer().voice_count(), 2);
        let start = level(&mut player, 480);
        assert!((start - flat(0.3)).abs() < 1e-3, "{start}");
        step(&mut player, 500.0, Some(false));
        level(&mut player, 4320);
        let halfway = level(&mut player, 480);
        assert!((halfway - flat(0.05)).abs() < 2e-3, "{halfway}");
        player.stop_looping(1);
        step(&mut player, 0.0, None);
        // the stop plays apart while the loop fades
        assert_eq!(player.mixer().voice_count(), 2);
        step(&mut player, 1000.0, None);
        assert_eq!(player.mixer().voice_count(), 1);
        level(&mut player, 4800);
        step(&mut player, 10.0, None);
        assert_eq!(player.looping_count(), 0);
    }

    #[test]
    fn the_alternate_switches_after_the_permutation_playing() {
        let mut machine = track(0);
        machine.alternate_loop = tag("whine");
        let mut player = player(vec![looping("machine", vec![machine])]);
        step(&mut player, 10.0, Some(false));
        level(&mut player, 2400);
        step(&mut player, 10.0, Some(true));
        // the hum's permutation finishes, then the whine
        let finishing = level(&mut player, 2400);
        assert!((finishing - flat(0.1)).abs() < 1e-3, "{finishing}");
        step(&mut player, 10.0, Some(true));
        let whine = level(&mut player, 2400);
        assert!((whine - flat(0.2)).abs() < 1e-3, "{whine}");
        assert_eq!(player.mixer().voice_count(), 1);
    }

    #[test]
    fn the_alternate_crossfades_when_its_track_says() {
        let mut fading = track(LoopingSoundTrack::FADE_IN_ALTERNATE);
        fading.alternate_loop = tag("whine");
        let mut player = player(vec![looping("fading", vec![fading])]);
        step(&mut player, 10.0, Some(false));
        level(&mut player, 480);
        step(&mut player, 10.0, Some(true));
        // both voices, the old fading out under the new
        assert_eq!(player.mixer().voice_count(), 2);
        for _ in 0..11 {
            step(&mut player, 100.0, Some(true));
            level(&mut player, 480);
        }
        assert_eq!(player.mixer().voice_count(), 1);
        let whine = level(&mut player, 2400);
        assert!((whine - flat(0.2)).abs() < 1e-3, "{whine}");
    }

    #[test]
    fn a_loop_out_of_hearing_fades_stops_and_comes_back() {
        let mut player = player(vec![looping("machine", vec![track(0)])]);
        let near = Source {
            position: Some([2.0, 0.0, 0.0]),
            ..Source::default()
        };
        let far = Source {
            position: Some([20.0, 0.0, 0.0]),
            ..near
        };
        let refresh = |player: &mut SoundPlayer, source: &Source, ms: f32| {
            player.refresh_looping(1, 0, source, false);
            player.advance(ms, 1);
            player.update_loops();
        };
        refresh(&mut player, &near, 10.0);
        assert!(level(&mut player, 480) > 0.0);
        refresh(&mut player, &far, 10.0);
        refresh(&mut player, &far, 2000.0);
        refresh(&mut player, &far, 10.0);
        assert_eq!(player.mixer().voice_count(), 0);
        assert_eq!(player.looping_count(), 1);
        refresh(&mut player, &near, 10.0);
        assert_eq!(player.mixer().voice_count(), 1);
        // fading back in
        assert!(level(&mut player, 480) < 1e-3);
        // a loop out of hearing from its start waits for it
        player.refresh_looping(2, 0, &far, false);
        player.update_loops();
        assert_eq!(player.looping_count(), 2);
        assert_eq!(player.mixer().voice_count(), 1);
        player.stop_all_loops();
        assert_eq!(player.looping_count(), 0);
        assert_eq!(player.mixer().voice_count(), 0);
    }

    #[test]
    fn details_play_around_their_source_unless_the_alternate_forbids() {
        let detail = |flags| LoopingSoundDetail {
            sound: tag("start"),
            period_bounds: [0.5, 0.5],
            gain: 1.0,
            flags,
            theta_bounds: [0.0, 0.0],
            phi_bounds: [0.0, 0.0],
            distance_bounds: [3.0, 3.0],
        };
        let mut background = looping("background", Vec::new());
        background.details = vec![detail(0), detail(LoopingSoundDetail::NOT_WITHOUT_ALTERNATE)];
        let mut player = player(vec![background]);
        step(&mut player, 0.0, Some(false));
        assert_eq!(player.mixer().voice_count(), 0);
        step(&mut player, 500.0, Some(false));
        // the first only, three units ahead of the listener
        assert_eq!(player.mixer().voice_count(), 1);
        let offset = player.detail_offset(&detail(0));
        assert!((offset[0] - 3.0).abs() < 1e-6 && offset[1].abs() < 1e-6);
        let mut still = detail(0);
        still.distance_bounds = [0.0, 0.0];
        assert_eq!(player.detail_offset(&still), [0.0; 3]);
        // both, with the alternate
        step(&mut player, 600.0, Some(true));
        assert_eq!(player.mixer().voice_count(), 3);
    }

    #[test]
    fn a_fade_moves_linearly_and_holds() {
        let fade = Fade::at(0.0).towards(100.0, 1.0, 200.0);
        assert!((fade.value(200.0) - 0.5).abs() < 1e-6);
        assert!((fade.value(400.0) - 1.0).abs() < 1e-6);
        assert!(!fade.silent(400.0));
        let out = fade.towards(200.0, 0.0, 100.0);
        assert!((out.value(250.0) - 0.25).abs() < 1e-6);
        assert!(out.silent(300.0));
    }

    #[test]
    fn another_sound_under_a_key_replaces_the_first() {
        let mut other = track(0);
        other.looping = tag("whine");
        let mut player = player(vec![
            looping("machine", vec![track(0)]),
            looping("other", vec![other]),
        ]);
        step(&mut player, 10.0, Some(false));
        level(&mut player, 480);
        player.refresh_looping(1, 1, &Source::default(), false);
        player.update_loops();
        assert_eq!(player.looping_count(), 1);
        assert_eq!(player.mixer().voice_count(), 2);
        for _ in 0..6 {
            player.refresh_looping(1, 1, &Source::default(), false);
            player.advance(100.0, 1);
            player.update_loops();
            level(&mut player, 480);
        }
        assert_eq!(player.mixer().voice_count(), 1);
    }
}
