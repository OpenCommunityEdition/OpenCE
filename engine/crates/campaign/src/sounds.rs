//! The sounds the scripts play (`hs_library_sound.c`): impulses started and
//! stopped, each timed by its length so the scripts can wait it out;
//! looping sounds (the music) started, stopped and switched to their
//! alternates; and sound classes' gains. The mission keeps them for the
//! presentation to play, and times the dialogue so a conversation's lines
//! wait for whoever speaks before.

use std::collections::BTreeMap;

use opence_content::TagRef;
use opence_script::{Native, Value};
use opence_sim::objects::ObjectHandle;

use crate::{boolean, index, real, short, tag, text, Mission};

/// The sound classes of the scripted dialogue (`scripted_dialog_to_player`,
/// `_to_other`, `_force_unspatialized`): while one plays, the next line of
/// dialogue waits.
pub const DIALOGUE_CLASSES: [u16; 3] = [44, 46, 47];

/// The game ticks after a line of dialogue ends before the next may begin
/// (the sound manager's pause between speakers).
const DIALOGUE_PAUSE: u64 = 10;

/// An impulse the scripts started.
#[derive(Clone, Debug, PartialEq)]
pub struct ScriptedSound {
    /// The sound.
    pub tag: TagRef,
    /// The object it is heard from; none for one heard everywhere alike.
    pub object: Option<ObjectHandle>,
    /// Its gain's scale.
    pub scale: f32,
}

/// A looping sound the scripts started, which plays until they stop it.
#[derive(Clone, Debug, PartialEq)]
pub struct ScriptedLoop {
    /// The looping sound.
    pub tag: TagRef,
    /// The object it is heard from; none for one heard everywhere alike.
    pub object: Option<ObjectHandle>,
    /// Its gain's scale.
    pub scale: f32,
    /// Whether its tracks' alternates play.
    pub alternate: bool,
}

/// A sound class's gain the scripts set: the class's name, the gain and
/// the game ticks it glides there over.
#[derive(Clone, Debug, PartialEq)]
pub struct ClassGain {
    /// The class's name (`sound_class_set_gain`'s, as the scripts give it).
    pub class: String,
    /// The gain.
    pub gain: f32,
    /// The game ticks it glides over.
    pub ticks: i16,
}

/// What the scripts did to the sounds since it was last taken.
#[derive(Clone, Debug, Default, PartialEq)]
pub struct SoundOrders {
    /// The impulses started, in order.
    pub started: Vec<ScriptedSound>,
    /// The impulses stopped, by their paths in lowercase.
    pub stopped: Vec<String>,
    /// The classes' gains set, in order.
    pub gains: Vec<ClassGain>,
}

/// The scripts' sounds.
#[derive(Clone, Debug, Default)]
pub(crate) struct ScriptedSounds {
    pub(crate) orders: SoundOrders,
    // (each impulse playing, by its path in lowercase, and the game tick it
    // ends on)
    pub(crate) ends: BTreeMap<String, u64>,
    // (each loop playing, by its path in lowercase)
    pub(crate) loops: BTreeMap<String, ScriptedLoop>,
    // (the game tick the last dialogue started ends on, with its pause)
    pub(crate) dialogue_ends: u64,
}

pub(crate) const NATIVES: &[(&str, Native<Mission>)] = &[
    ("sound_impulse_start", sound_impulse_start),
    ("sound_impulse_stop", sound_impulse_stop),
    ("sound_impulse_time", sound_impulse_time),
    ("sound_looping_start", sound_looping_start),
    ("sound_looping_stop", sound_looping_stop),
    ("sound_looping_set_alternate", sound_looping_set_alternate),
    ("sound_class_set_gain", sound_class_set_gain),
];

impl Mission {
    /// What the scripts did to the sounds since it was last taken.
    pub fn take_sounds(&mut self) -> SoundOrders {
        std::mem::take(&mut self.sounds.orders)
    }

    /// The looping sounds the scripts play, by their paths in lowercase.
    pub fn loops(&self) -> impl Iterator<Item = (&str, &ScriptedLoop)> {
        self.sounds.loops.iter().map(|(key, l)| (key.as_str(), l))
    }

    /// Whether a line of dialogue the scripts started plays (or its pause
    /// after).
    pub fn dialogue_playing(&self) -> bool {
        self.game_tick() < self.sounds.dialogue_ends
    }

    /// Starts `tag` from `object` at `scale`: timed by its length (one of
    /// no known length ends at once), kept for the presentation, and, for
    /// dialogue, held against the next line. Returns its key.
    pub(crate) fn start_sound(
        &mut self,
        tag: &TagRef,
        object: Option<ObjectHandle>,
        scale: f32,
    ) -> String {
        let key = tag.path.to_ascii_lowercase();
        let (milliseconds, class) = self
            .level
            .scenario
            .campaign
            .sound_lengths
            .iter()
            .find(|sound| sound.path.eq_ignore_ascii_case(&key))
            .map_or((0, u16::MAX), |sound| (sound.milliseconds, sound.class));
        let ticks = u64::from(milliseconds) * u64::from(opence_script::TICKS_PER_SECOND) / 1000;
        let now = self.game_tick();
        self.sounds.ends.insert(key.clone(), now + ticks);
        if DIALOGUE_CLASSES.contains(&class) {
            self.sounds.dialogue_ends = self.sounds.dialogue_ends.max(now + ticks + DIALOGUE_PAUSE);
        }
        self.sounds.orders.started.push(ScriptedSound {
            tag: tag.clone(),
            object,
            scale,
        });
        key
    }

    /// Whether the impulse of key `key` has ended or was stopped.
    pub(crate) fn sound_ended(&self, key: &str) -> bool {
        let tick = self.game_tick();
        self.sounds.ends.get(key).is_none_or(|&ends| ends <= tick)
    }
}

/// The scale a script gives, pinned to 0 to 1 as the original pins it; 1
/// where it gives none or a nonsense one.
fn scale(arguments: &[Value], at: usize) -> f32 {
    let scale = real(arguments, at);
    if scale.is_finite() {
        scale.clamp(0.0, 1.0)
    } else {
        1.0
    }
}

fn sound_impulse_start(mission: &mut Mission, arguments: &[Value]) -> Value {
    if let Some(sound) = tag(arguments, 0).cloned() {
        let object = mission.handle(index(arguments, 1));
        mission.start_sound(&sound, object, scale(arguments, 2));
    }
    Value::Void
}

fn sound_impulse_stop(mission: &mut Mission, arguments: &[Value]) -> Value {
    if let Some(sound) = tag(arguments, 0) {
        let key = sound.path.to_ascii_lowercase();
        if mission.sounds.ends.remove(&key).is_some() {
            mission.sounds.orders.stopped.push(key);
        }
    }
    Value::Void
}

/// The game ticks left of a sound the scripts started, as
/// `scripted_sound_time` counts them; 0 once it has ended or been stopped.
fn sound_impulse_time(mission: &mut Mission, arguments: &[Value]) -> Value {
    let tick = mission.game_tick();
    let left = tag(arguments, 0)
        .and_then(|sound| mission.sounds.ends.get(&sound.path.to_ascii_lowercase()))
        .map_or(0, |ends| ends.saturating_sub(tick));
    Value::Long(i32::try_from(left).unwrap_or(i32::MAX))
}

fn sound_looping_start(mission: &mut Mission, arguments: &[Value]) -> Value {
    if let Some(sound) = tag(arguments, 0).cloned() {
        let object = mission.handle(index(arguments, 1));
        mission.sounds.loops.insert(
            sound.path.to_ascii_lowercase(),
            ScriptedLoop {
                tag: sound,
                object,
                scale: scale(arguments, 2),
                alternate: false,
            },
        );
    }
    Value::Void
}

fn sound_looping_stop(mission: &mut Mission, arguments: &[Value]) -> Value {
    if let Some(sound) = tag(arguments, 0) {
        mission
            .sounds
            .loops
            .remove(&sound.path.to_ascii_lowercase());
    }
    Value::Void
}

fn sound_looping_set_alternate(mission: &mut Mission, arguments: &[Value]) -> Value {
    if let Some(sound) = tag(arguments, 0) {
        if let Some(playing) = mission
            .sounds
            .loops
            .get_mut(&sound.path.to_ascii_lowercase())
        {
            playing.alternate = boolean(arguments, 1);
        }
    }
    Value::Void
}

fn sound_class_set_gain(mission: &mut Mission, arguments: &[Value]) -> Value {
    let gain = real(arguments, 1);
    if gain.is_finite() {
        mission.sounds.orders.gains.push(ClassGain {
            class: text(arguments, 0).to_owned(),
            gain: gain.clamp(0.0, 1.0),
            ticks: short(arguments, 2).max(0),
        });
    }
    Value::Void
}
