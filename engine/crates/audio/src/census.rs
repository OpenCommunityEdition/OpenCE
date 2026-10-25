//! What the player made of the sounds asked of it, by class, for the dev
//! panel's audio view and the object inspector: the voices playing, the
//! sounds culled (skipped by their skip fraction or a promotion, out of
//! hearing, silent at their scale) and those that failed (no pitch range or
//! permutation to play, samples that would not decode).

use std::collections::BTreeMap;

use crate::mixer::{Mixer, VoiceId};

/// Why a sound asked for did not play.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Dropped {
    /// Skipped, out of hearing or silent.
    Culled,
    /// Nothing to play, or samples that would not decode.
    Failed,
}

/// A voice playing, and what plays on it.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Playing {
    /// The mixer's voice.
    pub voice: VoiceId,
    /// The sound, in the bank's.
    pub sound: usize,
    /// Its sound class.
    pub class: u16,
    /// Where it plays from; `None` for one heard everywhere alike.
    pub position: Option<[f32; 3]>,
    /// When it started, on the player's clock, in milliseconds.
    pub started_ms: f32,
}

/// One class's sounds: the voices playing now, and the sounds culled and
/// failed since the level started.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct ClassCount {
    /// The sound class.
    pub class: u16,
    /// Its voices playing.
    pub playing: u32,
    /// Its sounds culled.
    pub culled: u32,
    /// Its sounds that failed.
    pub failed: u32,
}

/// The voices playing and the sounds dropped, kept by the player.
#[derive(Debug, Default)]
pub struct Census {
    playing: Vec<Playing>,
    /// Each class's culled and failed.
    dropped: BTreeMap<u16, [u32; 2]>,
}

impl Census {
    pub(crate) fn played(&mut self, playing: Playing) {
        self.playing.push(playing);
    }

    pub(crate) fn dropped(&mut self, class: u16, why: Dropped) {
        let counts = self.dropped.entry(class).or_default();
        let count = &mut counts[usize::from(why == Dropped::Failed)];
        *count = count.saturating_add(1);
    }

    /// Forgets the voices that ended.
    pub(crate) fn prune(&mut self, mixer: &Mixer) {
        self.playing
            .retain(|playing| mixer.is_playing(playing.voice));
    }

    /// The voices playing (as of the last prune).
    pub fn playing(&self) -> &[Playing] {
        &self.playing
    }

    /// Every class with a sound playing, culled or failed, in the classes'
    /// order.
    pub fn counts(&self) -> Vec<ClassCount> {
        let mut counts: BTreeMap<u16, ClassCount> = self
            .dropped
            .iter()
            .map(|(&class, &[culled, failed])| {
                (
                    class,
                    ClassCount {
                        class,
                        playing: 0,
                        culled,
                        failed,
                    },
                )
            })
            .collect();
        for playing in &self.playing {
            let count = counts.entry(playing.class).or_insert(ClassCount {
                class: playing.class,
                ..ClassCount::default()
            });
            count.playing = count.playing.saturating_add(1);
        }
        counts.into_values().collect()
    }
}
