//! The level's sounds as the app hears them: the app's Core Audio node
//! pulls interleaved stereo at 48 kHz from an [`AudioOutput`], on its own
//! thread, behind a lock of its own so that a frame's tick never waits for
//! it.

use std::sync::{Arc, Mutex, MutexGuard};

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

/// Where the app's audio comes from.
#[derive(uniffi::Object)]
pub struct AudioOutput {
    player: SharedPlayer,
}

impl AudioOutput {
    pub(crate) fn new(player: SharedPlayer) -> Arc<Self> {
        Arc::new(Self { player })
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
        if let Some(player) = lock(&self.player).as_mut() {
            player.mix(&mut output);
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

    #[test]
    fn a_camera_looks_down_its_x() {
        let transform = opence_sim::Transform::default();
        let listener = listener(&transform, opence_sim::Vec3::X);
        assert_eq!(listener.forward, [1.0, 0.0, 0.0]);
        assert_eq!(listener.up, [0.0, 0.0, 1.0]);
        assert_eq!(listener.velocity, [1.0, 0.0, 0.0]);
    }
}
