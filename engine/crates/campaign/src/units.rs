//! Units and vehicles: their vitality, their custom and recorded
//! animations, their weapons and riders (`hs_library_units.c`).

use std::sync::Arc;

use opence_content::{Model, TagRef};
use opence_script::{Native, Value};
use opence_sim::animation::AnimationPlayer;
use opence_sim::damage::Vitality;
use opence_sim::objects::ObjectHandle;

use crate::{arg, boolean, element, index, real, short, text, Mission, Playing};

pub(crate) const NATIVES: &[(&str, Native<Mission>)] = &[
    ("unit_get_health", unit_get_health),
    ("unit_get_shield", unit_get_shield),
    ("unit_set_current_vitality", unit_set_current_vitality),
    ("units_set_current_vitality", units_set_current_vitality),
    ("unit_set_maximum_vitality", unit_set_maximum_vitality),
    ("unit_impervious", unit_impervious),
    ("unit_has_weapon_readied", unit_has_weapon_readied),
    ("unit_get_total_grenade_count", unit_get_total_grenade_count),
    (
        "unit_solo_player_integrated_night_vision_is_active",
        unit_solo_player_integrated_night_vision_is_active,
    ),
    ("custom_animation", custom_animation),
    ("custom_animation_list", custom_animation_list),
    (
        "unit_custom_animation_at_frame",
        unit_custom_animation_at_frame,
    ),
    (
        "unit_get_custom_animation_time",
        unit_get_custom_animation_time,
    ),
    ("unit_stop_custom_animation", unit_stop_custom_animation),
    ("scenery_animation_start", scenery_animation_start),
    (
        "scenery_animation_start_at_frame",
        scenery_animation_start_at_frame,
    ),
    ("recording_play", recording_play),
    ("recording_play_and_delete", recording_play_and_delete),
    ("recording_play_and_hover", recording_play_and_hover),
    ("recording_kill", recording_kill),
    ("recording_time", recording_time),
    ("vehicle_riders", vehicle_riders),
    ("vehicle_test_seat_list", vehicle_test_seat_list),
    ("vehicle_load_magic", vehicle_load_magic),
    ("vehicle_unload", vehicle_unload),
];

impl Mission {
    /// Plays animation `name` of the graph `graph` on an object from
    /// `frame`, until its last frame; whether there is such an animation.
    pub fn play_animation(
        &mut self,
        handle: ObjectHandle,
        graph: &TagRef,
        name: &str,
        frame: u16,
    ) -> bool {
        let Some(place) = self
            .level
            .animation_graphs
            .iter()
            .position(|g| &g.tag == graph)
        else {
            return false;
        };
        let Some(graph) = self.graph(place) else {
            return false;
        };
        let Some((animation, frames)) = graph
            .animations
            .iter()
            .enumerate()
            .find(|(_, a)| a.name == name)
            .and_then(|(i, a)| Some((u16::try_from(i).ok()?, a.frame_count)))
        else {
            return false;
        };
        let model = self.model(handle);
        if !self
            .sim
            .animate(handle, AnimationPlayer::new(graph, model, animation))
        {
            return false;
        }
        let ends = self.sim.tick().0 + u64::from(frames.saturating_sub(frame));
        self.animations.insert(
            handle,
            Playing {
                ends,
                delete: false,
            },
        );
        true
    }

    /// An object's model, shared.
    fn model(&self, handle: ObjectHandle) -> Option<Arc<Model>> {
        let tag = self.definition(handle)?.model.clone()?;
        let model = self.level.models.iter().find(|m| m.tag == tag)?;
        Some(Arc::new(model.clone()))
    }

    /// Sets an object's vitality to so much body and shield.
    fn set_vitality(&mut self, handle: ObjectHandle, body: f32, shield: f32) {
        let [most_body, most_shield] = self.maximum_vitality(handle);
        let Some(mut vitality) = self.sim.component::<Vitality>(handle) else {
            return;
        };
        let fraction = |amount: f32, most: f32| if most > 0.0 { amount / most } else { 0.0 };
        vitality.body = fraction(body, most_body);
        vitality.shield = fraction(shield, most_shield);
        self.sim.insert(handle, vitality);
    }

    /// Plays recording `recording` on a unit; whether there is one.
    fn play_recording(&mut self, handle: ObjectHandle, recording: i32, delete: bool) -> bool {
        let Some(length) =
            element(&self.level.scenario.campaign.recordings, recording).map(|r| r.length_in_ticks)
        else {
            return false;
        };
        let ends = self.sim.tick().0 + u64::try_from(length).unwrap_or(0);
        self.recordings.insert(handle, Playing { ends, delete });
        true
    }

    /// The players seated in a vehicle.
    fn riders(&self, vehicle: ObjectHandle) -> Vec<ObjectHandle> {
        self.sim
            .players()
            .iter()
            .filter(|p| p.seated().is_some_and(|s| s.vehicle == vehicle))
            .map(|p| p.handle())
            .collect()
    }
}

fn unit(mission: &Mission, arguments: &[Value]) -> Option<ObjectHandle> {
    mission.handle(index(arguments, 0))
}

fn tag(arguments: &[Value], at: usize) -> Option<&TagRef> {
    match arg(arguments, at) {
        Value::Tag(tag) => tag.as_ref(),
        _ => None,
    }
}

fn unit_get_health(mission: &mut Mission, arguments: &[Value]) -> Value {
    let health = unit(mission, arguments)
        .and_then(|h| mission.sim.component::<Vitality>(h))
        .map_or(-1.0, |v| v.body.max(0.0));
    Value::Real(health)
}

fn unit_get_shield(mission: &mut Mission, arguments: &[Value]) -> Value {
    let shield = unit(mission, arguments)
        .and_then(|h| mission.sim.component::<Vitality>(h))
        .map_or(-1.0, |v| v.shield.max(0.0));
    Value::Real(shield)
}

fn unit_set_current_vitality(mission: &mut Mission, arguments: &[Value]) -> Value {
    if let Some(handle) = unit(mission, arguments) {
        mission.set_vitality(handle, real(arguments, 1), real(arguments, 2));
    }
    Value::Void
}

fn units_set_current_vitality(mission: &mut Mission, arguments: &[Value]) -> Value {
    for handle in mission.list(index(arguments, 0)) {
        mission.set_vitality(handle, real(arguments, 1), real(arguments, 2));
    }
    Value::Void
}

fn unit_set_maximum_vitality(mission: &mut Mission, arguments: &[Value]) -> Value {
    if let Some(handle) = unit(mission, arguments) {
        mission
            .maximums
            .insert(handle, [real(arguments, 1), real(arguments, 2)]);
    }
    Value::Void
}

fn unit_impervious(mission: &mut Mission, arguments: &[Value]) -> Value {
    let impervious = boolean(arguments, 1);
    for handle in mission.list(index(arguments, 0)) {
        if impervious {
            mission.invulnerable.insert(handle);
        } else {
            mission.invulnerable.remove(&handle);
        }
    }
    Value::Void
}

fn unit_has_weapon_readied(mission: &mut Mission, arguments: &[Value]) -> Value {
    let readied = unit(mission, arguments)
        .and_then(|h| mission.sim.players().iter().find(|p| p.handle() == h))
        .and_then(|p| p.weapon())
        .is_some_and(|(weapon, _)| Some(weapon) == tag(arguments, 1));
    Value::Boolean(readied)
}

fn unit_get_total_grenade_count(mission: &mut Mission, arguments: &[Value]) -> Value {
    let count = unit(mission, arguments)
        .and_then(|h| mission.sim.players().iter().find(|p| p.handle() == h))
        .map_or(0, |p| p.inventory().grenades.iter().sum::<i16>());
    Value::Short(count)
}

fn unit_solo_player_integrated_night_vision_is_active(_: &mut Mission, _: &[Value]) -> Value {
    Value::Boolean(false)
}

fn custom_animation(mission: &mut Mission, arguments: &[Value]) -> Value {
    let started = match (unit(mission, arguments), tag(arguments, 1).cloned()) {
        (Some(handle), Some(graph)) => {
            mission.play_animation(handle, &graph, text(arguments, 2), 0)
        }
        _ => false,
    };
    Value::Boolean(started)
}

fn custom_animation_list(mission: &mut Mission, arguments: &[Value]) -> Value {
    let Some(graph) = tag(arguments, 1).cloned() else {
        return Value::Boolean(false);
    };
    let mut started = false;
    for handle in mission.list(index(arguments, 0)) {
        started |= mission.play_animation(handle, &graph, text(arguments, 2), 0);
    }
    Value::Boolean(started)
}

fn unit_custom_animation_at_frame(mission: &mut Mission, arguments: &[Value]) -> Value {
    let frame = u16::try_from(short(arguments, 4)).unwrap_or(0);
    let started = match (unit(mission, arguments), tag(arguments, 1).cloned()) {
        (Some(handle), Some(graph)) => {
            mission.play_animation(handle, &graph, text(arguments, 2), frame)
        }
        _ => false,
    };
    Value::Boolean(started)
}

/// The ticks left of a unit's custom animation or recording.
fn remaining(mission: &Mission, ends: Option<u64>) -> Value {
    let tick = mission.sim.tick().0;
    let left = ends.map_or(0, |ends| ends.saturating_sub(tick));
    Value::Short(i16::try_from(left).unwrap_or(i16::MAX))
}

fn unit_get_custom_animation_time(mission: &mut Mission, arguments: &[Value]) -> Value {
    let ends = unit(mission, arguments)
        .and_then(|h| mission.animations.get(&h))
        .map(|p| p.ends);
    remaining(mission, ends)
}

fn unit_stop_custom_animation(mission: &mut Mission, arguments: &[Value]) -> Value {
    if let Some(handle) = unit(mission, arguments) {
        mission.animations.remove(&handle);
    }
    Value::Void
}

fn scenery_animation_start(mission: &mut Mission, arguments: &[Value]) -> Value {
    if let (Some(handle), Some(graph)) = (unit(mission, arguments), tag(arguments, 1).cloned()) {
        mission.play_animation(handle, &graph, text(arguments, 2), 0);
    }
    Value::Void
}

fn scenery_animation_start_at_frame(mission: &mut Mission, arguments: &[Value]) -> Value {
    let frame = u16::try_from(short(arguments, 3)).unwrap_or(0);
    if let (Some(handle), Some(graph)) = (unit(mission, arguments), tag(arguments, 1).cloned()) {
        mission.play_animation(handle, &graph, text(arguments, 2), frame);
    }
    Value::Void
}

fn recording_play(mission: &mut Mission, arguments: &[Value]) -> Value {
    let played = unit(mission, arguments)
        .is_some_and(|h| mission.play_recording(h, index(arguments, 1), false));
    Value::Boolean(played)
}

fn recording_play_and_delete(mission: &mut Mission, arguments: &[Value]) -> Value {
    let played = unit(mission, arguments)
        .is_some_and(|h| mission.play_recording(h, index(arguments, 1), true));
    Value::Boolean(played)
}

fn recording_play_and_hover(mission: &mut Mission, arguments: &[Value]) -> Value {
    recording_play(mission, arguments)
}

fn recording_kill(mission: &mut Mission, arguments: &[Value]) -> Value {
    if let Some(handle) = unit(mission, arguments) {
        mission.recordings.remove(&handle);
    }
    Value::Void
}

fn recording_time(mission: &mut Mission, arguments: &[Value]) -> Value {
    let ends = unit(mission, arguments)
        .and_then(|h| mission.recordings.get(&h))
        .map(|p| p.ends);
    remaining(mission, ends)
}

fn vehicle_riders(mission: &mut Mission, arguments: &[Value]) -> Value {
    let riders = unit(mission, arguments)
        .map(|h| mission.riders(h))
        .unwrap_or_default();
    Value::Index(mission.list_of(riders))
}

fn vehicle_test_seat_list(mission: &mut Mission, arguments: &[Value]) -> Value {
    // (any seat: the seats' names are not the simulation's yet)
    let riders = unit(mission, arguments)
        .map(|h| mission.riders(h))
        .unwrap_or_default();
    let objects = mission.list(index(arguments, 2));
    Value::Boolean(objects.iter().any(|o| riders.contains(o)))
}

fn vehicle_load_magic(mission: &mut Mission, arguments: &[Value]) -> Value {
    mission.present("vehicle_load_magic", arguments);
    Value::Short(0)
}

fn vehicle_unload(mission: &mut Mission, arguments: &[Value]) -> Value {
    mission.present("vehicle_unload", arguments);
    Value::Short(0)
}
