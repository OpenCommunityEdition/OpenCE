//! The loading screen (source/interface/progress_bar.c): a band of light
//! that pans across the loading picture as the map loads, its last frames
//! left behind in a fading trail, while the chant's four voices swell and
//! fall with the progress.
//!
//! The game moved it once a frame at its 30 frames a second; here a frame
//! of any length moves it as much as that many thirtieths of a second
//! would. Once the map is ready the light finishes its sweep (or the
//! player skips it) and the screen fades into the mission's first frame.

/// The frame rate the game's steps are taken at.
pub const REFERENCE_RATE: f32 = 30.0;
/// How far the wobble's phase moves a step.
pub const PHASE_STEP: f32 = 0.01;
/// How much of the way to the progress the light moves a step.
pub const SMOOTHING: f32 = 0.2;
/// How far the wobble moves the light, in progress.
pub const WOBBLE: f32 = 0.05;
/// The width of the light's band, in the screen's 640 pixels.
pub const MASK_WIDTH: f32 = 128.0;
/// How far the band travels over the whole progress.
pub const MASK_TRAVEL: f32 = 768.0;
/// The trail's alpha until the progress passes [`FADE_START`].
pub const FADE_ALPHA: f32 = 0.9;
/// Where the trail starts to fade out.
pub const FADE_START: f32 = 0.9;
/// How much the trail is enlarged each frame.
pub const ZOOM: f32 = 1.0 / 128.0;
/// The trail's blur, in texels.
pub const BLUR_OFFSET: f32 = 0.1;
/// Each voice's part of the progress.
pub const VOICE_RANGES: [(f32, f32); 4] = [(0.0, 1.0), (0.4, 1.0), (0.5, 1.0), (0.55, 1.0)];
/// How far each voice rises above [`SILENT_VOLUME`], in hundredths of a
/// decibel.
pub const VOICE_VOLUMES: [f32; 4] = [3500.0, 4500.0, 3500.0, 4500.0];
/// The game's own pi for the voices' swell (it wrote 3.1415, so a
/// voice ends a little above silence).
#[allow(clippy::approx_constant)]
pub const SWELL_PI: f32 = 3.1415;
/// A voice's volume outside its part, in hundredths of a decibel.
pub const SILENT_VOLUME: f32 = -5000.0;
/// How long the screen takes to fade into the mission.
pub const CROSSFADE_SECONDS: f32 = 0.5;
/// How close to the end the light must be for its sweep to be over.
pub const DONE_PROGRESS: f32 = 0.999;
/// How long the light may take to finish once the map is ready.
pub const SETTLE_SECONDS: f32 = 3.0;
/// The longest frame taken whole (a longer one is a hitch, not time).
pub const LONGEST_FRAME: f32 = 0.25;

/// Where the screen is.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Stage {
    /// The map is loading.
    Loading,
    /// The map is ready; the light finishes its sweep.
    Settling,
    /// The screen fades into the mission.
    Crossfade,
    /// The mission shows alone.
    Done,
}

/// What a frame of the screen draws and plays.
#[derive(Clone, Debug, PartialEq)]
pub struct LoadingFrame {
    /// Where the light is (0 to 1).
    pub progress: f32,
    /// The light's brightness.
    pub color: f32,
    /// The band's left edge, in the screen's 640 pixels.
    pub mask_position: f32,
    /// The trail's alpha.
    pub fade_alpha: f32,
    /// Each voice's volume, in hundredths of a decibel.
    pub volumes: [f32; 4],
    /// How much of the screen shows over the mission (1 until it fades).
    pub screen_alpha: f32,
    /// Where the screen is.
    pub stage: Stage,
    /// Whether a key skips to the mission.
    pub skippable: bool,
}

/// The loading screen's state.
#[derive(Clone, Debug)]
pub struct LoadingScreen {
    enabled: bool,
    phase: f32,
    shown: f32,
    initial: Option<f32>,
    target: f32,
    stage: Stage,
    /// Seconds in the stage.
    time: f32,
}

/// The light's brightness at `progress`.
pub fn color(progress: f32) -> f32 {
    libm::powf(
        libm::sinf(progress.clamp(0.0, 1.0) * core::f32::consts::PI).max(0.0),
        0.9,
    )
}

/// The trail's alpha at `progress`.
pub fn fade_alpha(progress: f32) -> f32 {
    if progress < FADE_START {
        FADE_ALPHA
    } else {
        (FADE_ALPHA - (progress - FADE_START) * 9.0).max(0.0)
    }
}

/// The band's left edge at `progress`.
pub fn mask_position(progress: f32) -> f32 {
    progress * MASK_TRAVEL - MASK_WIDTH
}

/// Each voice's volume at `progress`, in hundredths of a decibel.
pub fn volumes(progress: f32) -> [f32; 4] {
    let mut volumes = [SILENT_VOLUME; 4];
    for (volume, (&(start, end), &rise)) in volumes
        .iter_mut()
        .zip(VOICE_RANGES.iter().zip(&VOICE_VOLUMES))
    {
        let t = if progress < start || progress > end {
            0.0
        } else {
            (progress - start) / (end - start)
        };
        *volume = libm::powf(libm::sinf(t * SWELL_PI).max(0.0), 0.3) * rise + SILENT_VOLUME;
    }
    volumes
}

/// The band of light's intensity at a texel of its 128 by 16 mask.
pub fn mask_intensity(x: u32, y: u32) -> f32 {
    use core::f32::consts::PI;
    let across = libm::sinf(PI * x as f32 / 127.0).max(0.0);
    let down = libm::powf(libm::sinf(PI * y as f32 / 15.0).max(0.0), 0.75);
    across * down * (250.0 / 255.0) + 5.0 / 255.0
}

impl LoadingScreen {
    /// A screen with the light and the chant (`enabled`, the setting
    /// `ui.loading_screen`), or a plain one that only fades.
    pub fn new(enabled: bool) -> Self {
        Self {
            enabled,
            phase: 0.0,
            shown: 0.0,
            initial: None,
            target: 0.0,
            stage: Stage::Loading,
            time: 0.0,
        }
    }

    /// Where the screen is.
    pub fn stage(&self) -> Stage {
        self.stage
    }

    /// How far the load has come (0 to 1); the first value given is the
    /// start, as the game measured from where its progress began.
    pub fn set_progress(&mut self, progress: f32) {
        let progress = if progress.is_finite() {
            progress.clamp(0.0, 1.0)
        } else {
            0.0
        };
        let initial = *self.initial.get_or_insert(progress);
        self.target = if initial >= 1.0 {
            1.0
        } else {
            ((progress - initial) / (1.0 - initial)).max(self.target)
        };
    }

    /// The map is ready: the light finishes its sweep (a plain screen
    /// fades at once).
    pub fn set_ready(&mut self) {
        if self.stage == Stage::Loading {
            self.target = 1.0;
            self.enter(if self.enabled {
                Stage::Settling
            } else {
                Stage::Crossfade
            });
        }
    }

    /// Skips to the fade into the mission, once the map is ready; whether
    /// it did.
    pub fn skip(&mut self) -> bool {
        if self.stage == Stage::Settling {
            self.enter(Stage::Crossfade);
            true
        } else {
            false
        }
    }

    fn enter(&mut self, stage: Stage) {
        self.stage = stage;
        self.time = 0.0;
    }

    /// The screen `seconds` after the last frame.
    pub fn update(&mut self, seconds: f32) -> LoadingFrame {
        let seconds = if seconds.is_finite() {
            seconds.clamp(0.0, LONGEST_FRAME)
        } else {
            0.0
        };
        self.time += seconds;
        let steps = seconds * REFERENCE_RATE;
        if self.stage != Stage::Done {
            self.phase += PHASE_STEP * steps;
            let toward = libm::sinf(self.phase) * WOBBLE + self.target;
            let moved = 1.0 - libm::powf(1.0 - SMOOTHING, steps);
            self.shown = ((toward - self.shown) * moved + self.shown).clamp(0.0, 1.0);
        }
        match self.stage {
            Stage::Settling if self.shown >= DONE_PROGRESS || self.time >= SETTLE_SECONDS => {
                self.enter(Stage::Crossfade)
            }
            Stage::Crossfade if self.time >= CROSSFADE_SECONDS => self.enter(Stage::Done),
            _ => {}
        }
        let screen_alpha = match self.stage {
            Stage::Loading | Stage::Settling => 1.0,
            Stage::Crossfade => {
                let t = (self.time / CROSSFADE_SECONDS).clamp(0.0, 1.0);
                1.0 - t * t * (3.0 - 2.0 * t)
            }
            Stage::Done => 0.0,
        };
        let progress = self.shown;
        let lit = self.enabled;
        LoadingFrame {
            progress,
            color: if lit { color(progress) } else { 0.0 },
            mask_position: mask_position(progress),
            fade_alpha: if lit { fade_alpha(progress) } else { 0.0 },
            volumes: if lit {
                volumes(progress)
            } else {
                [-10_000.0; 4]
            },
            screen_alpha,
            stage: self.stage,
            skippable: self.stage == Stage::Settling,
        }
    }
}
