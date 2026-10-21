//! The app's settings: one typed structure for the game, the controls,
//! the video and the audio, saved as TOML in
//! `~/Library/Application Support/OpenCE/settings.toml`. A setting that is
//! missing or out of range is the stock one; unknown ones are ignored.

use std::path::{Path, PathBuf};

use serde::{Deserialize, Serialize};

use crate::EngineError;

/// The game's settings: the player, and the game the app hosts.
#[derive(Clone, Debug, PartialEq, Serialize, Deserialize, uniffi::Record)]
#[serde(default)]
pub struct GameSettings {
    /// The player's name (at most 32 bytes).
    pub player_name: String,
    /// The hosted game's name (at most 32 bytes).
    pub server_name: String,
    /// The hosted game's type: "slayer", "team_slayer" or "ctf".
    pub game_type: String,
    /// The hosted game's map (its file's name, without .map).
    pub map: String,
    /// The most players the hosted game takes (2 to 16).
    pub max_players: u8,
    /// The port the hosted game listens on.
    pub port: u16,
}

impl Default for GameSettings {
    fn default() -> Self {
        Self {
            player_name: "Spartan".into(),
            server_name: "Halo".into(),
            game_type: "slayer".into(),
            map: "bloodgulch".into(),
            max_players: 16,
            port: opence_net::DEFAULT_PORT,
        }
    }
}

/// A key bound to an action of play.
#[derive(Clone, Debug, PartialEq, Eq, Serialize, Deserialize, uniffi::Record)]
pub struct KeyBinding {
    /// The action (the app's `PlayAction`, such as "jump").
    pub action: String,
    /// Its keys' codes (the mouse's buttons from 0x100).
    pub keys: Vec<u16>,
}

/// The keyboard and mouse.
#[derive(Clone, Debug, PartialEq, Serialize, Deserialize, uniffi::Record)]
#[serde(default)]
pub struct ControlSettings {
    /// The turn per count of the mouse, in radians (above 0, below 0.1).
    pub mouse_sensitivity: f32,
    /// Whether moving the mouse forward looks down.
    pub invert_mouse: bool,
    /// The keys of each action; none: the stock keys.
    pub bindings: Vec<KeyBinding>,
}

impl Default for ControlSettings {
    fn default() -> Self {
        Self {
            mouse_sensitivity: 0.0012,
            invert_mouse: false,
            bindings: Vec::new(),
        }
    }
}

/// The picture.
#[derive(Clone, Debug, PartialEq, Serialize, Deserialize, uniffi::Record)]
#[serde(default)]
pub struct VideoOptions {
    /// "temporal" (MetalFX) or "native".
    pub anti_aliasing: String,
    /// The share of the window drawn before it is scaled up (0.5 to 1).
    pub render_scale: f32,
    /// Whether the window fills the screen.
    pub full_screen: bool,
    /// Whether the frame times are shown.
    pub show_frame_times: bool,
}

impl Default for VideoOptions {
    fn default() -> Self {
        Self {
            anti_aliasing: "temporal".into(),
            render_scale: 1.0,
            full_screen: false,
            show_frame_times: false,
        }
    }
}

/// The sound's volumes, each from 0 to 1.
#[derive(Clone, Debug, PartialEq, Serialize, Deserialize, uniffi::Record)]
#[serde(default)]
pub struct AudioSettings {
    /// Everything.
    pub master: f32,
    /// The world's sounds.
    pub effects: f32,
    /// The music.
    pub music: f32,
}

impl Default for AudioSettings {
    fn default() -> Self {
        Self {
            master: 1.0,
            effects: 1.0,
            music: 0.8,
        }
    }
}

/// Every setting.
#[derive(Clone, Debug, Default, PartialEq, Serialize, Deserialize, uniffi::Record)]
#[serde(default)]
pub struct Settings {
    /// The game.
    pub game: GameSettings,
    /// The keyboard and mouse.
    pub controls: ControlSettings,
    /// The picture.
    pub video: VideoOptions,
    /// The sound.
    pub audio: AudioSettings,
}

/// `value` if it is finite and in `range`, else `stock`.
fn within(value: f32, range: std::ops::RangeInclusive<f32>, stock: f32) -> f32 {
    if value.is_finite() && range.contains(&value) {
        value
    } else {
        stock
    }
}

/// `name` if it is not empty, cut to 32 bytes; else `stock`.
fn name_or(name: &str, stock: &str) -> String {
    let name = opence_net::packet::fit(name.trim(), opence_net::packet::MAX_NAME_BYTES);
    if name.is_empty() {
        stock.into()
    } else {
        name
    }
}

impl Settings {
    /// The settings with every one out of range made the stock one.
    fn checked(self) -> Self {
        let stock = Self::default();
        let Self {
            game,
            controls,
            video,
            audio,
        } = self;
        let one_of = |value: String, allowed: &[&str], stock: String| {
            if allowed.contains(&value.as_str()) {
                value
            } else {
                stock
            }
        };
        Self {
            game: GameSettings {
                player_name: name_or(&game.player_name, &stock.game.player_name),
                server_name: name_or(&game.server_name, &stock.game.server_name),
                game_type: one_of(
                    game.game_type,
                    &["slayer", "team_slayer", "ctf"],
                    stock.game.game_type,
                ),
                map: if game.map.is_empty() || game.map.contains(['/', '\\']) {
                    stock.game.map
                } else {
                    game.map
                },
                max_players: if (2..=16).contains(&game.max_players) {
                    game.max_players
                } else {
                    stock.game.max_players
                },
                port: game.port,
            },
            controls: ControlSettings {
                mouse_sensitivity: if controls.mouse_sensitivity > 0.0 {
                    within(controls.mouse_sensitivity, 0.0..=0.1, 0.0012)
                } else {
                    stock.controls.mouse_sensitivity
                },
                invert_mouse: controls.invert_mouse,
                bindings: controls
                    .bindings
                    .into_iter()
                    .filter(|binding| !binding.action.is_empty())
                    .take(64)
                    .collect(),
            },
            video: VideoOptions {
                anti_aliasing: one_of(
                    video.anti_aliasing,
                    &["temporal", "native"],
                    stock.video.anti_aliasing,
                ),
                render_scale: within(video.render_scale, 0.5..=1.0, stock.video.render_scale),
                full_screen: video.full_screen,
                show_frame_times: video.show_frame_times,
            },
            audio: AudioSettings {
                master: within(audio.master, 0.0..=1.0, stock.audio.master),
                effects: within(audio.effects, 0.0..=1.0, stock.audio.effects),
                music: within(audio.music, 0.0..=1.0, stock.audio.music),
            },
        }
    }
}

fn settings_error(error: impl std::fmt::Display) -> EngineError {
    EngineError::Settings {
        message: error.to_string(),
    }
}

/// The stock settings.
#[uniffi::export]
pub fn stock_settings() -> Settings {
    Settings::default()
}

/// The settings `text` gives (TOML), each missing or out of range the
/// stock one; fails only when it is not TOML.
#[uniffi::export]
pub fn settings_from_toml(text: String) -> Result<Settings, EngineError> {
    toml::from_str::<Settings>(&text)
        .map(Settings::checked)
        .map_err(settings_error)
}

/// `settings` as TOML.
#[uniffi::export]
pub fn settings_to_toml(settings: Settings) -> Result<String, EngineError> {
    toml::to_string_pretty(&settings.checked()).map_err(settings_error)
}

/// Where the settings are kept: settings.toml in
/// `~/Library/Application Support/OpenCE`.
#[uniffi::export]
pub fn settings_path() -> String {
    let home = std::env::var_os("HOME").map_or_else(|| PathBuf::from("."), PathBuf::from);
    home.join("Library/Application Support/OpenCE/settings.toml")
        .to_string_lossy()
        .into_owned()
}

/// The settings saved at `path`: the stock ones when there is no file, or
/// when it cannot be read as settings.
#[uniffi::export]
pub fn load_settings(path: String) -> Settings {
    std::fs::read_to_string(&path)
        .ok()
        .and_then(|text| settings_from_toml(text).ok())
        .unwrap_or_default()
}

/// Saves `settings` at `path`, making its folder if need be.
#[uniffi::export]
pub fn save_settings(settings: Settings, path: String) -> Result<(), EngineError> {
    let path = Path::new(&path);
    if let Some(folder) = path.parent() {
        std::fs::create_dir_all(folder).map_err(settings_error)?;
    }
    std::fs::write(path, settings_to_toml(settings)?).map_err(settings_error)
}
