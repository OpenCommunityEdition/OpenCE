//! What the scripts tell the AI: encounters placed, conversations begun,
//! command lists given (`hs_library_ai.c`). The AI acts on these orders;
//! until it has actors, its counts are of none.

use std::collections::{BTreeMap, BTreeSet};

use opence_script::{Native, Value};
use opence_sim::objects::ObjectHandle;

use crate::{boolean, index, Mission};

/// An AI reference as a script holds it: an encounter, or one of its
/// platoons or squads (`ai_index`'s encoding).
#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord)]
pub enum AiReference {
    /// No AI.
    None,
    /// A whole encounter.
    Encounter(u16),
    /// An encounter's platoon.
    Platoon(u16, u8),
    /// An encounter's squad.
    Squad(u16, u8),
}

impl AiReference {
    /// The reference a script's value encodes: its type in the top two
    /// bits, the platoon or squad in the next byte up from the encounter's
    /// sixteen bits.
    /// (A squad's is negative as a script's long: its bits are read as
    /// they are, and `-1`'s type is none.)
    pub fn of(value: i32) -> Self {
        let bits = u32::from_ne_bytes(value.to_ne_bytes());
        let encounter = u16::try_from(bits & 0xFFFF).unwrap_or(0);
        let sub = u8::try_from((bits >> 16) & 0xFF).unwrap_or(0);
        match bits >> 30 {
            0 => Self::Encounter(encounter),
            1 => Self::Platoon(encounter, sub),
            2 => Self::Squad(encounter, sub),
            _ => Self::None,
        }
    }

    /// The encounter it is of.
    pub fn encounter(self) -> Option<u16> {
        match self {
            Self::None => None,
            Self::Encounter(e) | Self::Platoon(e, _) | Self::Squad(e, _) => Some(e),
        }
    }
}

/// What the scripts have told the AI.
#[derive(Clone, Debug, PartialEq)]
pub struct AiOrders {
    /// Whether the AI runs at all.
    pub enabled: bool,
    /// The references placed, in the order they were.
    pub placed: Vec<AiReference>,
    /// The conversations begun, by index, and whether each was stopped.
    pub conversations: BTreeMap<i32, bool>,
    /// The command lists given to units.
    pub command_lists: BTreeMap<ObjectHandle, i32>,
    /// The units each reference's actors are, as the AI fills them in.
    pub actors: BTreeMap<AiReference, Vec<ObjectHandle>>,
    /// Every other order, kept for the AI to act on.
    pub orders: Vec<(&'static str, Vec<Value>)>,
    /// The teams whose allegiance was set.
    pub allegiances: BTreeSet<(i32, i32)>,
}

impl Default for AiOrders {
    fn default() -> Self {
        Self {
            enabled: true,
            placed: Vec::new(),
            conversations: BTreeMap::new(),
            command_lists: BTreeMap::new(),
            actors: BTreeMap::new(),
            orders: Vec::new(),
            allegiances: BTreeSet::new(),
        }
    }
}

impl AiOrders {
    /// The units of a reference's actors: an encounter's are its platoons'
    /// and squads' too.
    pub fn units(&self, reference: AiReference) -> Vec<ObjectHandle> {
        match reference {
            AiReference::None => Vec::new(),
            AiReference::Encounter(e) => self
                .actors
                .iter()
                .filter(|(r, _)| r.encounter() == Some(e))
                .flat_map(|(_, units)| units.iter().copied())
                .collect(),
            other => self.actors.get(&other).cloned().unwrap_or_default(),
        }
    }
}

pub(crate) const NATIVES: &[(&str, Native<Mission>)] = &[
    ("ai", ai),
    ("ai_actors", ai_actors),
    ("ai_living_count", ai_living_count),
    ("ai_nonswarm_count", ai_living_count),
    ("ai_status", ai_status),
    ("ai_living_fraction", ai_living_fraction),
    ("ai_strength", ai_living_fraction),
    ("ai_allegiance", ai_allegiance),
    ("ai_allegiance_remove", ai_allegiance_remove),
    ("ai_allegiance_broken", ai_allegiance_broken),
    ("ai_going_to_vehicle", ai_going_to_vehicle),
    ("ai_command_list", ai_command_list),
    ("ai_command_list_by_unit", ai_command_list_by_unit),
    ("ai_command_list_status", ai_command_list_status),
    ("ai_conversation", ai_conversation),
    ("ai_conversation_stop", ai_conversation_stop),
    ("ai_conversation_status", ai_conversation_status),
    ("ai_conversation_line", ai_conversation_line),
    ("ai_place", ai_place),
];

/// A reference's living actors.
fn living(mission: &Mission, arguments: &[Value]) -> Vec<ObjectHandle> {
    mission
        .ai
        .units(AiReference::of(index(arguments, 0)))
        .into_iter()
        .filter(|&h| mission.sim.object(h).is_some())
        .collect()
}

fn count(n: usize) -> Value {
    Value::Short(i16::try_from(n).unwrap_or(i16::MAX))
}

fn ai(mission: &mut Mission, arguments: &[Value]) -> Value {
    mission.ai.enabled = boolean(arguments, 0);
    Value::Void
}

fn ai_actors(mission: &mut Mission, arguments: &[Value]) -> Value {
    let units = living(mission, arguments);
    Value::Index(mission.list_of(units))
}

fn ai_living_count(mission: &mut Mission, arguments: &[Value]) -> Value {
    count(living(mission, arguments).len())
}

fn ai_status(mission: &mut Mission, arguments: &[Value]) -> Value {
    // (0 is none living; the AI's states above it are the AI's to tell)
    let living = living(mission, arguments).len();
    Value::Short(i16::from(living > 0))
}

fn ai_living_fraction(mission: &mut Mission, arguments: &[Value]) -> Value {
    let all = mission.ai.units(AiReference::of(index(arguments, 0))).len();
    let living = living(mission, arguments).len();
    #[allow(clippy::cast_precision_loss)] // (counts of actors are small)
    let fraction = if all == 0 {
        0.0
    } else {
        living as f32 / all as f32
    };
    Value::Real(fraction)
}

fn teams(arguments: &[Value]) -> (i32, i32) {
    let (a, b) = (index(arguments, 0), index(arguments, 1));
    (a.min(b), a.max(b))
}

fn ai_allegiance(mission: &mut Mission, arguments: &[Value]) -> Value {
    mission.ai.allegiances.insert(teams(arguments));
    Value::Void
}

fn ai_allegiance_remove(mission: &mut Mission, arguments: &[Value]) -> Value {
    mission.ai.allegiances.remove(&teams(arguments));
    Value::Void
}

fn ai_allegiance_broken(_: &mut Mission, _: &[Value]) -> Value {
    Value::Boolean(false)
}

fn ai_going_to_vehicle(_: &mut Mission, _: &[Value]) -> Value {
    Value::Short(0)
}

fn ai_command_list(mission: &mut Mission, arguments: &[Value]) -> Value {
    let list = index(arguments, 1);
    for unit in mission.ai.units(AiReference::of(index(arguments, 0))) {
        mission.ai.command_lists.insert(unit, list);
    }
    Value::Void
}

fn ai_command_list_by_unit(mission: &mut Mission, arguments: &[Value]) -> Value {
    if let Some(unit) = mission.handle(index(arguments, 0)) {
        mission.ai.command_lists.insert(unit, index(arguments, 1));
    }
    Value::Void
}

fn ai_command_list_status(_: &mut Mission, _: &[Value]) -> Value {
    // (0: no unit of the list is following one; the AI tells the rest)
    Value::Short(0)
}

fn ai_conversation(mission: &mut Mission, arguments: &[Value]) -> Value {
    mission.ai.conversations.insert(index(arguments, 0), false);
    Value::Boolean(true)
}

fn ai_conversation_stop(mission: &mut Mission, arguments: &[Value]) -> Value {
    if let Some(stopped) = mission.ai.conversations.get_mut(&index(arguments, 0)) {
        *stopped = true;
    }
    Value::Void
}

fn ai_conversation_status(mission: &mut Mission, arguments: &[Value]) -> Value {
    // (Halo's statuses: 0 none, 4 aborted, 6 finished; a conversation's
    // lines are the presentation's, so one begun is taken as finished)
    let status = match mission.ai.conversations.get(&index(arguments, 0)) {
        None => 0,
        Some(true) => 4,
        Some(false) => 6,
    };
    Value::Short(status)
}

fn ai_conversation_line(_: &mut Mission, _: &[Value]) -> Value {
    Value::Short(0)
}

fn ai_place(mission: &mut Mission, arguments: &[Value]) -> Value {
    mission.ai.placed.push(AiReference::of(index(arguments, 0)));
    Value::Void
}

/// Orders kept for the AI to act on.
macro_rules! ordered {
    ($($name:ident),* $(,)?) => {
        $(
            fn $name(mission: &mut Mission, arguments: &[Value]) -> Value {
                mission.ai.orders.push((stringify!($name), arguments.to_vec()));
                Value::Void
            }
        )*
        pub(crate) const PRESENTED: &[(&str, Native<Mission>)] =
            &[$((stringify!($name), $name)),*];
    };
}

ordered! {
    ai_allow_dormant, ai_attach, ai_attach_free, ai_attack,
    ai_automatic_migration_target, ai_berserk, ai_braindead,
    ai_braindead_by_unit, ai_command_list_advance, ai_conversation_advance,
    ai_defend, ai_detach, ai_dialogue_triggers, ai_disregard, ai_erase,
    ai_erase_all, ai_exit_vehicle, ai_follow_distance, ai_follow_target_ai,
    ai_follow_target_disable, ai_follow_target_players, ai_force_active,
    ai_free, ai_free_units, ai_go_to_vehicle, ai_grenades, ai_kill,
    ai_kill_silent, ai_link_activation, ai_magically_see_encounter,
    ai_magically_see_players, ai_maneuver, ai_maneuver_enable, ai_migrate,
    ai_migrate_and_speak, ai_migrate_by_unit, ai_playfight, ai_prefer_target,
    ai_reconnect, ai_renew, ai_retreat, ai_set_blind, ai_set_deaf,
    ai_spawn_actor, ai_teleport_to_starting_location,
    ai_teleport_to_starting_location_if_unsupported, ai_timer_expire,
    ai_try_to_fight, ai_try_to_fight_nothing, ai_try_to_fight_player,
    ai_vehicle_encounter, ai_vehicle_enterable_actor_type,
    ai_vehicle_enterable_disable, ai_vehicle_enterable_distance,
}
