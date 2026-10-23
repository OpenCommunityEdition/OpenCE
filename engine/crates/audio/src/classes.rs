//! The game's 51 sound classes (`sound_classes.c`): a sound's class sets how
//! many of it play at once, its priority, how much of it the environment's
//! reverb takes, and its distances when the sound gives none. A script
//! fades a class's gain (`sound_class_set_gain`).

/// One sound class.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct SoundClass {
    /// Its name; empty for the indices the game leaves unused.
    pub name: &'static str,
    /// How many of one sound play at once.
    pub maximum_per_definition: u8,
    /// How many of one sound play at once from one object.
    pub maximum_per_object: u8,
    /// How long, in milliseconds, a playing sound is safe from being
    /// replaced by a newer one.
    pub preemption_ms: u16,
    /// It is speech: a unit's mouth moves to it.
    pub speech: bool,
    /// Its priority: a sound of a higher one takes a voice from a lower.
    pub priority: u8,
    /// A sound whose samples are not loaded waits for them (rather than
    /// being dropped).
    pub waits_for_samples: bool,
    /// How much of it the environment's reverb takes.
    pub wet_gain: f32,
    /// Its minimum distance, for a sound that gives none.
    pub minimum_distance: f32,
    /// Its maximum distance, for a sound that gives none.
    pub maximum_distance: f32,
    /// A gain the game defines but never reads (`unknown_gain`).
    pub unknown_gain: f32,
    /// A scale the game defines but never reads (`unknown_scale`).
    pub unknown_scale: f32,
}

#[allow(clippy::too_many_arguments)]
const fn c(
    name: &'static str,
    maximum_per_definition: u8,
    maximum_per_object: u8,
    preemption_ms: u16,
    speech: bool,
    priority: u8,
    waits_for_samples: bool,
    wet_gain: f32,
    minimum_distance: f32,
    maximum_distance: f32,
    unknown_gain: f32,
    unknown_scale: f32,
) -> SoundClass {
    SoundClass {
        name,
        maximum_per_definition,
        maximum_per_object,
        preemption_ms,
        speech,
        priority,
        waits_for_samples,
        wet_gain,
        minimum_distance,
        maximum_distance,
        unknown_gain,
        unknown_scale,
    }
}

/// The number of sound classes.
pub const SOUND_CLASS_COUNT: usize = 51;

/// The classes, by index, as the game defines them.
pub const SOUND_CLASSES: [SoundClass; SOUND_CLASS_COUNT] = [
    c(
        "projectile_impact",
        6,
        4,
        100,
        false,
        4,
        false,
        0.5,
        1.4,
        8.0,
        1.0,
        1.0,
    ),
    c(
        "projectile_detonation",
        4,
        1,
        200,
        false,
        5,
        true,
        0.5,
        8.0,
        120.0,
        1.0,
        1.0,
    ),
    c("", 0, 0, 0, false, 0, false, 0.0, 0.0, 0.0, 0.0, 0.0),
    c("", 0, 0, 0, false, 0, false, 0.0, 0.0, 0.0, 0.0, 0.0),
    c(
        "weapon_fire",
        4,
        1,
        0,
        false,
        4,
        true,
        0.5,
        4.0,
        70.0,
        1.0,
        1.0,
    ),
    c(
        "weapon_ready",
        4,
        1,
        500,
        false,
        4,
        true,
        0.5,
        1.0,
        9.0,
        1.0,
        1.0,
    ),
    c(
        "weapon_reload",
        4,
        1,
        500,
        false,
        4,
        true,
        0.5,
        1.0,
        9.0,
        1.0,
        1.0,
    ),
    c(
        "weapon_empty",
        4,
        1,
        60,
        false,
        4,
        true,
        0.5,
        1.0,
        9.0,
        1.0,
        1.0,
    ),
    c(
        "weapon_charge",
        4,
        1,
        500,
        false,
        4,
        true,
        0.5,
        1.0,
        9.0,
        1.0,
        1.0,
    ),
    c(
        "weapon_overheat",
        4,
        1,
        500,
        false,
        4,
        true,
        0.5,
        1.0,
        9.0,
        1.0,
        1.0,
    ),
    c(
        "weapon_idle",
        4,
        1,
        500,
        false,
        4,
        true,
        0.5,
        1.0,
        9.0,
        1.0,
        1.0,
    ),
    c("", 0, 0, 0, false, 0, false, 0.0, 0.0, 0.0, 0.0, 0.0),
    c("", 0, 0, 0, false, 0, false, 0.0, 0.0, 0.0, 0.0, 0.0),
    c(
        "object_impacts",
        4,
        1,
        100,
        false,
        3,
        true,
        0.5,
        0.5,
        3.0,
        0.0,
        1.0,
    ),
    c(
        "particle_impacts",
        4,
        1,
        100,
        false,
        3,
        false,
        0.5,
        0.5,
        3.0,
        0.0,
        1.0,
    ),
    c(
        "slow_particle_impacts",
        4,
        1,
        1000,
        false,
        3,
        false,
        0.5,
        0.5,
        3.0,
        0.0,
        1.0,
    ),
    c("", 0, 0, 0, false, 0, false, 0.0, 0.0, 0.0, 0.0, 0.0),
    c("", 0, 0, 0, false, 0, false, 0.0, 0.0, 0.0, 0.0, 0.0),
    c(
        "unit_footsteps",
        4,
        1,
        200,
        false,
        3,
        false,
        0.5,
        0.9,
        10.0,
        1.0,
        1.0,
    ),
    c(
        "unit_dialog",
        4,
        1,
        100,
        true,
        3,
        true,
        0.8,
        3.0,
        20.0,
        0.0,
        1.0,
    ),
    c("", 0, 0, 0, false, 0, false, 0.0, 0.0, 0.0, 0.0, 0.0),
    c("", 0, 0, 0, false, 0, false, 0.0, 0.0, 0.0, 0.0, 0.0),
    c(
        "vehicle_collision",
        4,
        2,
        400,
        false,
        3,
        false,
        0.5,
        1.4,
        8.0,
        1.0,
        1.0,
    ),
    c(
        "vehicle_engine",
        4,
        2,
        100,
        false,
        3,
        true,
        0.9,
        1.4,
        8.0,
        1.0,
        1.0,
    ),
    c("", 0, 0, 0, false, 0, false, 0.0, 0.0, 0.0, 0.0, 0.0),
    c("", 0, 0, 0, false, 0, false, 0.0, 0.0, 0.0, 0.0, 0.0),
    c(
        "device_door",
        4,
        1,
        100,
        false,
        2,
        true,
        0.5,
        0.9,
        5.0,
        1.0,
        1.0,
    ),
    c(
        "device_force_field",
        4,
        1,
        100,
        false,
        2,
        true,
        0.5,
        0.9,
        5.0,
        1.0,
        1.0,
    ),
    c(
        "device_machinery",
        4,
        1,
        100,
        false,
        1,
        true,
        0.5,
        0.9,
        5.0,
        1.0,
        1.0,
    ),
    c(
        "device_nature",
        4,
        1,
        100,
        false,
        1,
        true,
        0.5,
        0.9,
        5.0,
        1.0,
        1.0,
    ),
    c(
        "device_computers",
        4,
        1,
        100,
        false,
        1,
        true,
        0.5,
        0.5,
        3.0,
        1.0,
        1.0,
    ),
    c("", 0, 0, 0, false, 0, false, 0.0, 0.0, 0.0, 0.0, 0.0),
    c("music", 4, 4, 100, false, 2, true, 1.0, 0.9, 5.0, 0.0, 1.0),
    c(
        "ambient_nature",
        4,
        1,
        100,
        false,
        1,
        true,
        1.0,
        0.9,
        5.0,
        0.0,
        1.0,
    ),
    c(
        "ambient_machinery",
        4,
        1,
        100,
        false,
        1,
        true,
        1.0,
        0.9,
        5.0,
        0.0,
        1.0,
    ),
    c(
        "ambient_computers",
        4,
        1,
        100,
        false,
        1,
        true,
        1.0,
        0.5,
        3.0,
        0.0,
        1.0,
    ),
    c("", 0, 0, 0, false, 0, false, 0.0, 0.0, 0.0, 0.0, 0.0),
    c("", 0, 0, 0, false, 0, false, 0.0, 0.0, 0.0, 0.0, 0.0),
    c("", 0, 0, 0, false, 0, false, 0.0, 0.0, 0.0, 0.0, 0.0),
    c(
        "first_person_damage",
        4,
        1,
        100,
        false,
        4,
        true,
        1.0,
        0.5,
        3.0,
        1.0,
        1.0,
    ),
    c("", 0, 0, 0, false, 0, false, 0.0, 0.0, 0.0, 0.0, 0.0),
    c("", 0, 0, 0, false, 0, false, 0.0, 0.0, 0.0, 0.0, 0.0),
    c("", 0, 0, 0, false, 0, false, 0.0, 0.0, 0.0, 0.0, 0.0),
    c("", 0, 0, 0, false, 0, false, 0.0, 0.0, 0.0, 0.0, 0.0),
    c(
        "scripted_dialog_player",
        4,
        4,
        100,
        true,
        6,
        true,
        0.8,
        3.0,
        20.0,
        0.0,
        1.0,
    ),
    c(
        "scripted_effect",
        4,
        4,
        100,
        false,
        3,
        true,
        0.8,
        2.0,
        5.0,
        0.0,
        1.0,
    ),
    c(
        "scripted_dialog_other",
        4,
        4,
        100,
        true,
        5,
        true,
        0.8,
        3.0,
        20.0,
        0.0,
        1.0,
    ),
    c(
        "scripted_dialog_force_unspatialized",
        4,
        4,
        100,
        true,
        6,
        true,
        0.8,
        3.0,
        20.0,
        0.0,
        1.0,
    ),
    c("", 0, 0, 0, false, 0, false, 0.0, 0.0, 0.0, 0.0, 0.0),
    c("", 0, 0, 0, false, 0, false, 0.0, 0.0, 0.0, 0.0, 0.0),
    c(
        "game_event",
        4,
        1,
        100,
        false,
        5,
        true,
        1.0,
        3.0,
        20.0,
        1.0,
        1.0,
    ),
];

/// The class of a sound's index; an index out of range is the nearest
/// class, as the game pins it.
pub fn class(index: i32) -> &'static SoundClass {
    let index = usize::try_from(index.max(0))
        .unwrap_or(0)
        .min(SOUND_CLASS_COUNT - 1);
    &SOUND_CLASSES[index]
}

/// The index of the class of `name`, if there is one.
pub fn class_index(name: &str) -> Option<usize> {
    if name.is_empty() {
        return None;
    }
    SOUND_CLASSES.iter().position(|class| class.name == name)
}

/// The gains the scripts fade the classes to (`sound_class_set_gain`),
/// stepped each game tick.
#[derive(Clone, Debug, PartialEq)]
pub struct ClassGains {
    gains: [ClassGain; SOUND_CLASS_COUNT],
}

#[derive(Clone, Copy, Debug, PartialEq)]
struct ClassGain {
    gain: f32,
    desired: f32,
    ticks: i32,
}

impl Default for ClassGains {
    fn default() -> Self {
        Self {
            gains: [ClassGain {
                gain: 1.0,
                desired: 1.0,
                ticks: 0,
            }; SOUND_CLASS_COUNT],
        }
    }
}

impl ClassGains {
    /// A class's gain now.
    pub fn gain(&self, index: i32) -> f32 {
        let index = usize::try_from(index.max(0))
            .unwrap_or(0)
            .min(SOUND_CLASS_COUNT - 1);
        self.gains[index].gain
    }

    /// Fades every class whose name contains `name` to `gain` (pinned to 0
    /// to 1) over `ticks` game ticks, as `sound_class_set_gain` does.
    pub fn set(&mut self, name: &str, gain: f32, ticks: i32) {
        for (class, state) in SOUND_CLASSES.iter().zip(self.gains.iter_mut()) {
            if !class.name.is_empty() && class.name.contains(name) {
                state.desired = gain.clamp(0.0, 1.0);
                state.ticks = ticks.max(0);
            }
        }
    }

    /// Steps the fades by `ticks` game ticks (`sound_classes_update`).
    pub fn update(&mut self, ticks: i32) {
        if ticks <= 0 {
            return;
        }
        for state in &mut self.gains {
            if state.ticks > ticks {
                #[allow(clippy::cast_precision_loss)]
                let fraction = ticks as f32 / state.ticks as f32;
                state.gain += fraction * (state.desired - state.gain);
                state.ticks -= ticks;
            } else {
                state.gain = state.desired;
                state.ticks = 0;
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn the_classes_are_the_games() {
        assert_eq!(
            SOUND_CLASSES.iter().filter(|c| !c.name.is_empty()).count(),
            31
        );
        assert_eq!(class_index("music"), Some(32));
        assert_eq!(class_index("game_event"), Some(50));
        assert_eq!(class_index(""), None);
        let damage = class(39);
        assert_eq!(damage.name, "first_person_damage");
        assert_eq!(damage.priority, 4);
        assert!((damage.maximum_distance - 3.0).abs() < 1e-6);
        assert!(class(19).speech);
        assert_eq!(class(-4).name, "projectile_impact");
        assert_eq!(class(400).name, "game_event");
    }

    #[test]
    fn a_class_fades_to_its_gain() {
        let mut gains = ClassGains::default();
        gains.set("scripted_dialog", 2.0, 10);
        gains.update(5);
        assert!((gains.gain(44) - 1.0).abs() < 1e-6, "pinned to 1");
        gains.set("ambient", 0.0, 10);
        gains.update(5);
        assert!((gains.gain(33) - 0.5).abs() < 1e-6);
        assert!((gains.gain(34) - 0.5).abs() < 1e-6);
        gains.update(0);
        assert!((gains.gain(33) - 0.5).abs() < 1e-6);
        gains.update(20);
        assert!(gains.gain(33).abs() < 1e-6);
        assert!((gains.gain(32) - 1.0).abs() < 1e-6, "music is untouched");
    }
}
