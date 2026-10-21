//! The game's flow as the scripts drive it: won or lost, saved or
//! reverted, its speed and structure, the HUD's timer and what of the HUD
//! shows, and the player's input as the scripts test it
//! (`hs_library_game.c`, `hs_library_hud.c`, `hs_library_player.c`).

use opence_script::{Native, Value};
use opence_sim::input::Buttons;
use opence_sim::objects::Difficulty;

use crate::{boolean, real, short, text, Mission};

/// The ticks a save takes (Halo's saves are written while the game goes
/// on; the scripts wait out `game_saving`).
const SAVE_TICKS: u16 = 30;

/// The game's flow.
#[derive(Clone, Debug, PartialEq)]
pub struct Flow {
    /// Whether the mission is won.
    pub won: bool,
    /// Whether the mission is lost.
    pub lost: bool,
    /// The saves asked for.
    pub saves: u32,
    /// The ticks of the save being written.
    pub saving: u16,
    /// Whether a revert to the last save was asked for.
    pub revert: bool,
    /// The game's speed (1 is the normal).
    pub speed: f32,
    /// The structure the scripts switched to.
    pub bsp: i16,
    /// The HUD timer's ticks left.
    pub timer: i32,
    /// Whether the HUD timer is paused.
    pub timer_paused: bool,
    /// Whether the HUD shows.
    pub hud: bool,
    /// Whether the HUD's help text shows.
    pub help_text: bool,
    /// Whether lights render.
    pub lights: bool,
    /// Whether the player's camera follows the player.
    pub player_camera: bool,
}

impl Default for Flow {
    fn default() -> Self {
        Self {
            won: false,
            lost: false,
            saves: 0,
            saving: 0,
            revert: false,
            speed: 1.0,
            bsp: 0,
            timer: 0,
            timer_paused: true,
            hud: true,
            help_text: true,
            lights: true,
            player_camera: true,
        }
    }
}

impl Flow {
    pub(crate) fn advance(&mut self) {
        self.saving = self.saving.saturating_sub(1);
        if !self.timer_paused {
            self.timer = (self.timer - 1).max(0);
        }
    }

    fn save(&mut self) {
        self.saves += 1;
        self.saving = SAVE_TICKS;
    }
}

pub(crate) const NATIVES: &[(&str, Native<Mission>)] = &[
    ("game_won", game_won),
    ("game_lost", game_lost),
    ("game_save", game_save),
    ("game_save_no_timeout", game_save),
    ("game_save_totally_unsafe", game_save),
    ("game_save_cancel", game_save_cancel),
    ("game_revert", game_revert),
    ("game_reverted", game_reverted),
    ("game_saving", game_saving),
    ("game_safe_to_save", always),
    ("game_safe_to_speak", always),
    ("game_all_quiet", always),
    ("game_difficulty_get", game_difficulty_get),
    ("game_difficulty_get_real", game_difficulty_get),
    ("game_is_cooperative", game_is_cooperative),
    ("game_time", game_time),
    ("game_speed", game_speed),
    ("game_skip_ticks", game_skip_ticks),
    ("structure_bsp_index", structure_bsp_index),
    ("switch_bsp", switch_bsp),
    ("random_range", random_range),
    ("real_random_range", real_random_range),
    ("not", not),
    ("print", print),
    ("sound_impulse_start", sound_impulse_start),
    ("sound_impulse_stop", sound_impulse_stop),
    ("sound_impulse_time", sound_impulse_time),
    ("hud_get_timer_ticks", hud_get_timer_ticks),
    ("hud_set_timer_time", hud_set_timer_time),
    ("pause_hud_timer", pause_hud_timer),
    ("show_hud", show_hud),
    ("show_hud_help_text", show_hud_help_text),
    ("render_lights", render_lights),
    ("player_camera_control", player_camera_control),
    ("player0_joystick_set_is_normal", always),
    ("player0_look_pitch_is_inverted", never),
    ("player_enable_input", player_enable_input),
    ("player_action_test_reset", player_action_test_reset),
    ("player_action_test_accept", player_action_test_accept),
    ("player_action_test_action", player_action_test_action),
    ("player_action_test_back", player_action_test_back),
    (
        "player_action_test_grenade_trigger",
        player_action_test_grenade_trigger,
    ),
    (
        "player_action_test_primary_trigger",
        player_action_test_primary_trigger,
    ),
    ("player_action_test_look_relative_left", look_left),
    ("player_action_test_look_relative_right", look_right),
    ("player_action_test_look_relative_up", look_up),
    ("player_action_test_look_relative_down", look_down),
    ("player_action_test_look_relative_all_directions", look_all),
];

fn always(_: &mut Mission, _: &[Value]) -> Value {
    Value::Boolean(true)
}

fn never(_: &mut Mission, _: &[Value]) -> Value {
    Value::Boolean(false)
}

fn game_won(mission: &mut Mission, _: &[Value]) -> Value {
    mission.flow.won = true;
    Value::Void
}

fn game_lost(mission: &mut Mission, _: &[Value]) -> Value {
    mission.flow.lost = true;
    Value::Void
}

fn game_save(mission: &mut Mission, _: &[Value]) -> Value {
    mission.flow.save();
    // (the scripts run before the tick's input is played: the save holds
    // this tick too, so that a load does not ask for it again)
    mission.saving = Some(mission.inputs.len() + 1);
    Value::Void
}

fn game_save_cancel(mission: &mut Mission, _: &[Value]) -> Value {
    mission.flow.saving = 0;
    mission.saving = None;
    Value::Void
}

fn game_revert(mission: &mut Mission, _: &[Value]) -> Value {
    mission.flow.revert = true;
    Value::Void
}

fn game_reverted(mission: &mut Mission, _: &[Value]) -> Value {
    // (true only in the tick after a revert loads)
    Value::Boolean(mission.reverted)
}

fn game_saving(mission: &mut Mission, _: &[Value]) -> Value {
    Value::Boolean(mission.flow.saving > 0)
}

fn game_difficulty_get(mission: &mut Mission, _: &[Value]) -> Value {
    let difficulty = match mission.difficulty {
        Difficulty::Easy => 0,
        Difficulty::Normal => 1,
        Difficulty::Hard => 2,
        Difficulty::Legendary => 3,
    };
    Value::Short(difficulty)
}

fn game_is_cooperative(mission: &mut Mission, _: &[Value]) -> Value {
    Value::Boolean(mission.sim.players().len() > 1)
}

fn game_time(mission: &mut Mission, _: &[Value]) -> Value {
    Value::Long(i32::try_from(mission.game_tick()).unwrap_or(i32::MAX))
}

fn game_speed(mission: &mut Mission, arguments: &[Value]) -> Value {
    mission.flow.speed = real(arguments, 0).max(0.0);
    Value::Void
}

fn game_skip_ticks(mission: &mut Mission, arguments: &[Value]) -> Value {
    mission.present("game_skip_ticks", arguments);
    Value::Void
}

fn structure_bsp_index(mission: &mut Mission, _: &[Value]) -> Value {
    Value::Short(mission.flow.bsp)
}

fn switch_bsp(mission: &mut Mission, arguments: &[Value]) -> Value {
    mission.switch_structure(short(arguments, 0));
    Value::Void
}

fn random_range(mission: &mut Mission, arguments: &[Value]) -> Value {
    // (from the first up to, not including, the second, as Halo's is)
    let (low, high) = (short(arguments, 0), short(arguments, 1));
    if high <= low {
        return Value::Short(low);
    }
    let span = u32::from(high.abs_diff(low));
    let offset = i32::try_from(mission.next_random() % span).unwrap_or(0);
    Value::Short(i16::try_from(i32::from(low) + offset).unwrap_or(low))
}

fn real_random_range(mission: &mut Mission, arguments: &[Value]) -> Value {
    let (low, high) = (real(arguments, 0), real(arguments, 1));
    #[allow(clippy::cast_precision_loss)] // (24 bits, exact in a real)
    let along = mission.next_random() as f32 / (1u32 << 24) as f32;
    Value::Real(low + (high - low) * along)
}

fn not(_: &mut Mission, arguments: &[Value]) -> Value {
    Value::Boolean(!boolean(arguments, 0))
}

fn print(mission: &mut Mission, arguments: &[Value]) -> Value {
    let line = text(arguments, 0).to_owned();
    mission.printed.push(line);
    Value::Void
}

/// The sound's path as the mission keys it.
fn sound_key(arguments: &[Value]) -> Option<String> {
    match arguments.first() {
        Some(Value::Tag(Some(tag))) => Some(tag.path.to_ascii_lowercase()),
        _ => None,
    }
}

fn sound_impulse_start(mission: &mut Mission, arguments: &[Value]) -> Value {
    mission.present("sound_impulse_start", arguments);
    if let Some(key) = sound_key(arguments) {
        // (one in the resource maps, of no known length, has ended at once)
        let milliseconds = mission
            .level
            .scenario
            .campaign
            .sound_lengths
            .iter()
            .find(|sound| sound.path.eq_ignore_ascii_case(&key))
            .map_or(0, |sound| sound.milliseconds);
        let ticks = u64::from(milliseconds) * u64::from(opence_script::TICKS_PER_SECOND) / 1000;
        let ends = mission.game_tick() + ticks;
        mission.sounds.insert(key, ends);
    }
    Value::Void
}

fn sound_impulse_stop(mission: &mut Mission, arguments: &[Value]) -> Value {
    mission.present("sound_impulse_stop", arguments);
    if let Some(key) = sound_key(arguments) {
        mission.sounds.remove(&key);
    }
    Value::Void
}

/// The game ticks left of a sound the scripts started, as
/// `scripted_sound_time` counts them; 0 once it has ended or been stopped.
fn sound_impulse_time(mission: &mut Mission, arguments: &[Value]) -> Value {
    let tick = mission.game_tick();
    let left = sound_key(arguments)
        .and_then(|key| mission.sounds.get(&key))
        .map_or(0, |ends| ends.saturating_sub(tick));
    Value::Long(i32::try_from(left).unwrap_or(i32::MAX))
}

fn hud_get_timer_ticks(mission: &mut Mission, _: &[Value]) -> Value {
    Value::Short(i16::try_from(mission.flow.timer).unwrap_or(i16::MAX))
}

fn hud_set_timer_time(mission: &mut Mission, arguments: &[Value]) -> Value {
    let (minutes, seconds) = (
        i32::from(short(arguments, 0)),
        i32::from(short(arguments, 1)),
    );
    mission.flow.timer = (minutes * 60 + seconds).max(0) * 30;
    Value::Void
}

fn pause_hud_timer(mission: &mut Mission, arguments: &[Value]) -> Value {
    mission.flow.timer_paused = boolean(arguments, 0);
    Value::Void
}

fn show_hud(mission: &mut Mission, arguments: &[Value]) -> Value {
    mission.flow.hud = boolean(arguments, 0);
    Value::Boolean(mission.flow.hud)
}

fn show_hud_help_text(mission: &mut Mission, arguments: &[Value]) -> Value {
    mission.flow.help_text = boolean(arguments, 0);
    Value::Boolean(mission.flow.help_text)
}

fn render_lights(mission: &mut Mission, arguments: &[Value]) -> Value {
    mission.flow.lights = boolean(arguments, 0);
    Value::Boolean(mission.flow.lights)
}

fn player_camera_control(mission: &mut Mission, arguments: &[Value]) -> Value {
    mission.flow.player_camera = boolean(arguments, 0);
    Value::Boolean(mission.flow.player_camera)
}

fn player_enable_input(mission: &mut Mission, arguments: &[Value]) -> Value {
    mission.input_enabled = boolean(arguments, 0);
    Value::Void
}

fn player_action_test_reset(mission: &mut Mission, _: &[Value]) -> Value {
    mission.actions = Buttons::default();
    mission.looked = [false; 4];
    Value::Void
}

fn pressed(mission: &Mission, buttons: Buttons) -> Value {
    Value::Boolean(mission.actions.0 & buttons.0 != 0)
}

fn player_action_test_accept(mission: &mut Mission, _: &[Value]) -> Value {
    pressed(mission, Buttons(Buttons::JUMP.0 | Buttons::ACTION.0))
}

fn player_action_test_action(mission: &mut Mission, _: &[Value]) -> Value {
    pressed(mission, Buttons::ACTION)
}

fn player_action_test_back(mission: &mut Mission, _: &[Value]) -> Value {
    pressed(mission, Buttons::MELEE)
}

fn player_action_test_grenade_trigger(mission: &mut Mission, _: &[Value]) -> Value {
    pressed(mission, Buttons::GRENADE)
}

fn player_action_test_primary_trigger(mission: &mut Mission, _: &[Value]) -> Value {
    pressed(mission, Buttons::FIRE)
}

fn look_left(mission: &mut Mission, _: &[Value]) -> Value {
    Value::Boolean(mission.looked[0])
}

fn look_right(mission: &mut Mission, _: &[Value]) -> Value {
    Value::Boolean(mission.looked[1])
}

fn look_up(mission: &mut Mission, _: &[Value]) -> Value {
    Value::Boolean(mission.looked[2])
}

fn look_down(mission: &mut Mission, _: &[Value]) -> Value {
    Value::Boolean(mission.looked[3])
}

fn look_all(mission: &mut Mission, _: &[Value]) -> Value {
    Value::Boolean(mission.looked.iter().all(|&l| l))
}
