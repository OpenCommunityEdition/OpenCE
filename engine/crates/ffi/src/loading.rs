//! The loading screen (source/interface/progress_bar.c): its chant, made
//! from the player's own executable into the cache, and its model.

use std::path::Path;
use std::sync::{Arc, Mutex, MutexGuard};

use opence_ui::chant::{
    decode_xbox_adpcm, find_tone, gain, pitch_rate, wave_file, SAMPLE_RATE, TONE_BLOCKS,
    VOICE_PITCHES,
};
use opence_ui::loading::{mask_intensity, LoadingScreen, Stage};

/// The chant's file in the cache folder.
pub const CHANT_FILE: &str = "chant.wav";
/// The largest executable read (the retail one is 2.4 MB).
const MAXIMUM_XBE_BYTES: u64 = 64 << 20;

/// The chant as a WAVE file in `cache_folder`, made from the tone in the
/// executable at `xbe_path` the first time; none when there is no
/// executable or no tone in it (the loading screen is then silent).
#[uniffi::export]
pub fn chant_wave(xbe_path: String, cache_folder: String) -> Option<String> {
    let wave = Path::new(&cache_folder).join(CHANT_FILE);
    let samples = TONE_BLOCKS * opence_ui::chant::SAMPLES_PER_BLOCK;
    let expected = 44 + 2 * samples as u64;
    if std::fs::metadata(&wave).is_ok_and(|meta| meta.len() == expected) {
        return Some(wave.to_string_lossy().into_owned());
    }
    if std::fs::metadata(&xbe_path).ok()?.len() > MAXIMUM_XBE_BYTES {
        return None;
    }
    let xbe = std::fs::read(&xbe_path).ok()?;
    let file = wave_file(&decode_xbox_adpcm(find_tone(&xbe)?), SAMPLE_RATE);
    std::fs::create_dir_all(&cache_folder).ok()?;
    // (written whole, then named, so that a cut-off write is not taken)
    let part = wave.with_extension("wav.part");
    std::fs::write(&part, file).ok()?;
    std::fs::rename(&part, &wave).ok()?;
    Some(wave.to_string_lossy().into_owned())
}

/// Where the loading screen is.
#[derive(Clone, Copy, Debug, PartialEq, Eq, uniffi::Enum)]
pub enum LoadingStage {
    /// The map is loading.
    Loading,
    /// The map is ready; the light finishes its sweep.
    Settling,
    /// The screen fades into the mission.
    Crossfade,
    /// The mission shows alone.
    Done,
}

impl From<Stage> for LoadingStage {
    fn from(stage: Stage) -> Self {
        match stage {
            Stage::Loading => Self::Loading,
            Stage::Settling => Self::Settling,
            Stage::Crossfade => Self::Crossfade,
            Stage::Done => Self::Done,
        }
    }
}

/// A frame of the loading screen: what its pass draws and its voices play.
#[derive(Clone, Debug, PartialEq, uniffi::Record)]
pub struct LoadingView {
    /// The light's brightness.
    pub color: f32,
    /// The band's left edge, in the screen's 640 pixels.
    pub mask_position: f32,
    /// The trail's alpha.
    pub fade_alpha: f32,
    /// Each voice's gain (0 to 1), the fade into the mission taken in.
    pub gains: Vec<f32>,
    /// Each voice's rate against the tone's.
    pub rates: Vec<f32>,
    /// How much of the screen shows over the mission.
    pub screen_alpha: f32,
    /// Where the screen is.
    pub stage: LoadingStage,
    /// Whether a key skips to the mission.
    pub skippable: bool,
}

/// The loading screen, shared with the frame loop and the loader.
#[derive(uniffi::Object)]
pub struct LoadingScreenModel {
    state: Mutex<LoadingScreen>,
}

#[uniffi::export]
impl LoadingScreenModel {
    /// A screen with the light and the chant (`ui.loading_screen`), or a
    /// plain one that only fades.
    #[uniffi::constructor]
    pub fn new(enabled: bool) -> Arc<Self> {
        Arc::new(Self {
            state: Mutex::new(LoadingScreen::new(enabled)),
        })
    }

    /// How far the load has come (0 to 1).
    pub fn set_progress(&self, progress: f32) {
        self.lock().set_progress(progress);
    }

    /// The map is ready.
    pub fn set_ready(&self) {
        self.lock().set_ready();
    }

    /// Skips to the fade into the mission once the map is ready; whether
    /// it did.
    pub fn skip(&self) -> bool {
        self.lock().skip()
    }

    /// The screen `seconds` after the last frame.
    pub fn update(&self, seconds: f32) -> LoadingView {
        let frame = self.lock().update(seconds);
        let fading = frame.screen_alpha;
        LoadingView {
            color: frame.color,
            mask_position: frame.mask_position,
            fade_alpha: frame.fade_alpha,
            gains: frame.volumes.iter().map(|&v| gain(v) * fading).collect(),
            rates: VOICE_PITCHES.iter().map(|&p| pitch_rate(p)).collect(),
            screen_alpha: frame.screen_alpha,
            stage: frame.stage.into(),
            skippable: frame.skippable,
        }
    }

    /// The light's band as a 128 by 16 mask of 8-bit alpha.
    pub fn mask_pixels(&self) -> Vec<u8> {
        mask_pixels()
    }
}

impl LoadingScreenModel {
    fn lock(&self) -> MutexGuard<'_, LoadingScreen> {
        self.state
            .lock()
            .unwrap_or_else(std::sync::PoisonError::into_inner)
    }
}

/// The light's band as a 128 by 16 mask of 8-bit alpha, row by row.
pub fn mask_pixels() -> Vec<u8> {
    (0..MASK_HEIGHT)
        .flat_map(|y| (0..MASK_WIDTH).map(move |x| (mask_intensity(x, y) * 255.0).round() as u8))
        .collect()
}

/// The mask's width in texels.
pub const MASK_WIDTH: u32 = 128;
/// The mask's height in texels.
pub const MASK_HEIGHT: u32 = 16;
