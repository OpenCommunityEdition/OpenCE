//! The level's sounds as the app hears them: the app's Core Audio node
//! pulls interleaved stereo at 48 kHz from an [`AudioOutput`], on its own
//! thread. A pull never waits for the tick: it mixes a little ahead of what
//! it is asked for when the player is free, and while the tick holds the
//! player it plays on from what it mixed ahead.

use std::collections::VecDeque;
use std::sync::{Arc, Mutex, MutexGuard, TryLockError};

use opence_audio::{Listener, SoundPlayer, OUTPUT_RATE};

/// The level's player, shared by the engine and its output.
pub(crate) type SharedPlayer = Arc<Mutex<Option<SoundPlayer>>>;

pub(crate) fn lock(player: &SharedPlayer) -> MutexGuard<'_, Option<SoundPlayer>> {
    player
        .lock()
        .unwrap_or_else(std::sync::PoisonError::into_inner)
}

/// The listener of a camera at `transform`, moving at `velocity`: it looks
/// down its +x and its up is its +z, as the game's are.
pub(crate) fn listener(transform: &opence_sim::Transform, velocity: opence_sim::Vec3) -> Listener {
    let forward = transform.rotation * opence_sim::Vec3::X;
    let up = transform.rotation * opence_sim::Vec3::Z;
    Listener {
        position: transform.position.to_array(),
        forward: forward.to_array(),
        up: up.to_array(),
        velocity: velocity.to_array(),
    }
}

/// One sound class in the dev panel's audio view.
#[derive(Clone, Debug, PartialEq, Eq, uniffi::Record)]
pub struct SoundClassCount {
    /// The class's name.
    pub class: String,
    /// Its voices playing.
    pub playing: u32,
    /// Its sounds culled (skipped, out of hearing or silent) since the
    /// level started.
    pub culled: u32,
    /// Its sounds with nothing to play since the level started.
    pub failed: u32,
}

/// The player's census, by class name.
pub(crate) fn census(player: &mut SoundPlayer) -> Vec<SoundClassCount> {
    player
        .census()
        .counts()
        .into_iter()
        .map(|count| SoundClassCount {
            class: opence_audio::class(i32::from(count.class)).name.to_owned(),
            playing: count.playing,
            culled: count.culled,
            failed: count.failed,
        })
        .collect()
}

/// The frames a pull mixes ahead of what it is asked for (about 11 ms),
/// to play on from while the tick holds the player.
pub(crate) const AHEAD: usize = 512;

/// Where the app's audio comes from.
#[derive(uniffi::Object)]
pub struct AudioOutput {
    player: SharedPlayer,
    ahead: Mutex<Ahead>,
}

/// What a pull mixed ahead, interleaved stereo.
#[derive(Default)]
struct Ahead {
    samples: VecDeque<f32>,
    mixing: Vec<f32>,
    missed: u64,
}

impl Ahead {
    /// Mixes enough ahead of the next `frames` frames, or forgets what was
    /// mixed when the level has gone.
    fn fill(&mut self, player: &mut Option<SoundPlayer>, frames: usize) {
        let Some(player) = player.as_mut() else {
            self.samples.clear();
            return;
        };
        let wanted = (frames + AHEAD) * 2;
        if self.samples.len() >= wanted {
            return;
        }
        self.mixing.clear();
        self.mixing.resize(wanted - self.samples.len(), 0.0);
        player.mix(&mut self.mixing);
        self.samples.extend(&self.mixing);
    }
}

impl AudioOutput {
    pub(crate) fn new(player: SharedPlayer) -> Arc<Self> {
        Arc::new(Self {
            player,
            ahead: Mutex::new(Ahead::default()),
        })
    }

    /// The frames pulled while the tick held the player and nothing was
    /// mixed ahead to play: the output's gaps.
    pub fn missed(&self) -> u64 {
        self.ahead
            .lock()
            .unwrap_or_else(std::sync::PoisonError::into_inner)
            .missed
    }
}

#[uniffi::export]
impl AudioOutput {
    /// The output's sample rate.
    pub fn sample_rate(&self) -> u32 {
        OUTPUT_RATE
    }

    /// The next `frames` frames, interleaved stereo; silence without a
    /// level.
    pub fn pull(&self, frames: u32) -> Vec<f32> {
        let frames = usize::try_from(frames)
            .unwrap_or(0)
            .min(OUTPUT_RATE as usize);
        let mut output = vec![0.0; frames * 2];
        // (only the app's audio thread pulls, so this lock is free; were it
        // not, the pull would be silent rather than wait)
        let mut ahead = match self.ahead.try_lock() {
            Ok(ahead) => ahead,
            Err(TryLockError::Poisoned(poisoned)) => poisoned.into_inner(),
            Err(TryLockError::WouldBlock) => return output,
        };
        let blocked = match self.player.try_lock() {
            Ok(mut player) => {
                ahead.fill(&mut player, frames);
                false
            }
            Err(TryLockError::Poisoned(poisoned)) => {
                ahead.fill(&mut poisoned.into_inner(), frames);
                false
            }
            Err(TryLockError::WouldBlock) => true,
        };
        let have = ahead.samples.len().min(output.len());
        for (out, sample) in output.iter_mut().zip(ahead.samples.drain(..have)) {
            *out = sample;
        }
        if blocked {
            ahead.missed += ((output.len() - have) / 2) as u64;
        }
        output
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn without_a_level_the_output_is_silent() {
        let output = AudioOutput::new(SharedPlayer::default());
        assert_eq!(output.sample_rate(), 48000);
        let frames = output.pull(256);
        assert_eq!(frames.len(), 512);
        assert!(frames.iter().all(|sample| *sample == 0.0));
        assert_eq!(output.pull(u32::MAX).len(), 96000);
    }

    /// A player that plays a steady tone, heard alike everywhere.
    fn toned() -> SharedPlayer {
        let mut player = SoundPlayer::new(Arc::new(Vec::<opence_content::sound::Sound>::new()));
        let tone = Arc::new(opence_audio::Decoded {
            sample_rate: OUTPUT_RATE,
            channels: 1,
            samples: vec![8000; OUTPUT_RATE as usize],
        });
        player
            .mixer()
            .play(tone, opence_audio::VoiceProperties::default(), 0.0);
        Arc::new(Mutex::new(Some(player)))
    }

    /// Holds the player's lock, as a long tick does, until `release` is
    /// dropped.
    fn held(player: &SharedPlayer) -> (std::thread::JoinHandle<()>, std::sync::mpsc::Sender<()>) {
        let (release, wait) = std::sync::mpsc::channel::<()>();
        let (taken, take) = std::sync::mpsc::channel();
        let player = Arc::clone(player);
        let holder = std::thread::spawn(move || {
            let _held = lock(&player);
            let _ = taken.send(());
            let _ = wait.recv();
        });
        let _ = take.recv();
        (holder, release)
    }

    #[test]
    fn a_pull_while_the_tick_holds_the_player_does_not_wait() {
        let player = toned();
        let output = AudioOutput::new(Arc::clone(&player));
        let (holder, release) = held(&player);
        let start = std::time::Instant::now();
        let frames = output.pull(256);
        let waited = start.elapsed();
        drop(release);
        let _ = holder.join();
        assert!(waited < std::time::Duration::from_millis(20), "{waited:?}");
        assert_eq!(frames.len(), 512);
        assert_eq!(output.missed(), 256);
    }

    #[test]
    fn what_is_mixed_ahead_carries_the_output_through_a_held_lock() {
        let player = toned();
        let output = AudioOutput::new(Arc::clone(&player));
        assert!(output.pull(256).iter().all(|sample| *sample != 0.0));
        let (holder, release) = held(&player);
        let carried = output.pull(u32::try_from(AHEAD).unwrap_or(0));
        drop(release);
        let _ = holder.join();
        assert!(carried.iter().all(|sample| *sample != 0.0));
        assert_eq!(output.missed(), 0);
        // (and once the lock is free, the tone goes on without a gap)
        assert!(output.pull(256).iter().all(|sample| *sample != 0.0));
    }

    #[test]
    fn what_was_mixed_ahead_is_dropped_with_the_level() {
        let player = toned();
        let output = AudioOutput::new(Arc::clone(&player));
        output.pull(256);
        *lock(&player) = None;
        assert!(output.pull(128).iter().all(|sample| *sample == 0.0));
    }

    #[test]
    fn a_camera_looks_down_its_x() {
        let transform = opence_sim::Transform::default();
        let listener = listener(&transform, opence_sim::Vec3::X);
        assert_eq!(listener.forward, [1.0, 0.0, 0.0]);
        assert_eq!(listener.up, [0.0, 0.0, 1.0]);
        assert_eq!(listener.velocity, [1.0, 0.0, 0.0]);
    }
}
