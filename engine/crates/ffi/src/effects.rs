//! The effects the game makes, run beside the simulation: those of its
//! shots, impacts and detonations, and those its objects carry (a
//! scenery's smoke, a vehicle's exhaust), their parts made at the objects'
//! markers. The parts made each tick are handed to the lights and decals.

use std::cell::RefCell;
use std::collections::{BTreeMap, HashMap};

use opence_content::{Level, TagRef};
use opence_sim::animation::NodeMatrix;
use opence_sim::effects::{Effects, FiredPart};
use opence_sim::objects::ObjectHandle;
use opence_sim::seats::default_pose;
use opence_sim::{Mat3, Quat, Simulation, Vec3};

use crate::dynamic_decals::DynamicDecals;
use crate::lights::marker_frames;
use crate::snapshot::{EffectSnapshot, MAX_EFFECTS};

/// An effect an object definition carries.
#[derive(Clone, Debug, PartialEq)]
struct Carried {
    /// The attachment's index among the definition's.
    slot: usize,
    effect: TagRef,
    marker: String,
}

/// The effects running and what the level's objects carry.
#[derive(Clone, Debug, Default)]
pub(crate) struct LevelEffects {
    effects: Effects,
    /// Each object definition's effects, and the model their markers are
    /// on.
    carried: BTreeMap<TagRef, (Vec<Carried>, Option<usize>)>,
}

/// The turn whose axes are a frame's.
fn turn(frame: &NodeMatrix) -> Quat {
    let axes = Mat3::from_cols(
        Vec3::from_array(frame.forward).normalize_or_zero(),
        Vec3::from_array(frame.left).normalize_or_zero(),
        Vec3::from_array(frame.up).normalize_or_zero(),
    );
    Quat::from_mat3(&axes).normalize()
}

impl LevelEffects {
    /// The effects `level`'s object definitions carry.
    pub(crate) fn new(level: &Level) -> Self {
        let mut carried = BTreeMap::new();
        for object in &level.objects {
            let effects: Vec<Carried> = object
                .attachments
                .iter()
                .enumerate()
                .filter_map(|(slot, attachment)| {
                    let tag = attachment.tag.as_ref()?;
                    level.effect(tag)?;
                    Some(Carried {
                        slot,
                        effect: tag.clone(),
                        marker: attachment.marker.clone(),
                    })
                })
                .collect();
            if effects.is_empty() {
                continue;
            }
            let model = object
                .model
                .as_ref()
                .and_then(|model| level.model_index(model));
            carried.insert(object.tag.clone(), (effects, model));
        }
        Self {
            effects: Effects::default(),
            carried,
        }
    }

    /// The effects running.
    pub(crate) fn running(&self) -> &[opence_sim::effects::Running] {
        self.effects.running()
    }

    /// The parts made in the last tick.
    pub(crate) fn fired(&self) -> &[FiredPart] {
        self.effects.fired()
    }

    /// Starts the effect `tag` at `point` on a surface facing `normal`
    /// (a footstep's), if the level has it.
    pub(crate) fn start(&mut self, level: &Level, tag: &TagRef, point: Vec3, normal: Vec3) {
        let origin = opence_sim::effects::Origin::at(point, opence_sim::effects::facing(normal));
        self.effects.start(level, tag, origin);
    }

    /// One tick, after the simulation's: this tick's shots, impacts (by
    /// the materials `decals` finds on BSP `bsp`) and detonations make
    /// their effects, objects start those they carry, and every effect
    /// runs on.
    pub(crate) fn tick(
        &mut self,
        level: &Level,
        simulation: &mut Simulation,
        decals: &mut DynamicDecals,
        bsp: usize,
    ) {
        {
            let decals = RefCell::new(decals);
            let world = &*simulation;
            self.effects.trigger(
                level,
                world,
                |impact| decals.borrow_mut().material(level, world, bsp, impact),
                |point| decals.borrow_mut().ground(level, bsp, point),
            );
        }
        let handles: HashMap<u32, ObjectHandle> = simulation
            .objects()
            .into_iter()
            .map(|handle| (handle.index, handle))
            .collect();
        // (each object's markers its effects are at, where they are now)
        let mut wanted: HashMap<u32, Vec<String>> = HashMap::new();
        for running in self.effects.running() {
            let Some((handle, marker)) = &running.origin.on else {
                continue;
            };
            let markers = wanted.entry(handle.index).or_default();
            markers.push(marker.clone());
            if let Some(effect) = level.effect(&running.tag) {
                markers.extend(effect.locations.iter().cloned());
            }
        }
        let mut placed: HashMap<(ObjectHandle, String), (Vec3, Quat)> = HashMap::new();
        let effects = &mut self.effects;
        let carried = &self.carried;
        simulation.visit_lit(|object| {
            let Some(&handle) = handles.get(&object.id.0) else {
                return;
            };
            let carries = carried.get(object.tag);
            let markers = wanted.get(&object.id.0);
            if carries.is_none() && markers.is_none() {
                return;
            }
            let model = carries
                .and_then(|(_, model)| *model)
                .or_else(|| {
                    let model = level.object(object.tag)?.model.as_ref()?;
                    level.model_index(model)
                })
                .and_then(|index| level.models.get(index));
            let rest = match (object.nodes, model) {
                (None, Some(model)) => default_pose(model),
                _ => Vec::new(),
            };
            let place = |marker: &str| {
                let (current, _) = marker_frames(model, &object, &rest, marker);
                (Vec3::from_array(current.position), turn(&current))
            };
            for marker in markers.into_iter().flatten() {
                placed.insert((handle, marker.clone()), place(marker));
            }
            for carry in carries
                .map(|(effects, _)| effects.as_slice())
                .unwrap_or(&[])
            {
                placed
                    .entry((handle, carry.marker.clone()))
                    .or_insert_with(|| place(&carry.marker));
                effects.attach(level, handle, carry.slot, &carry.effect, &carry.marker);
            }
        });
        let exists = |handle: ObjectHandle| handles.get(&handle.index) == Some(&handle);
        self.effects.tick(level, exists, |handle, marker| {
            placed.get(&(handle, marker.to_owned())).copied()
        });
    }

    /// The effects running, as the snapshot gives them.
    pub(crate) fn gather(&self, out: &mut Vec<EffectSnapshot>) {
        out.clear();
        for running in self.effects.running().iter().rev().take(MAX_EFFECTS) {
            let position = running.origin.position;
            let forward = running.origin.rotation * Vec3::X;
            out.push(EffectSnapshot {
                position: [
                    position.x,
                    position.y,
                    position.z,
                    running.age as f32 / 30.0,
                ],
                forward: [forward.x, forward.y, forward.z, 0.0],
                ids: [
                    running.number,
                    u32::try_from(running.event).unwrap_or(u32::MAX),
                    running
                        .origin
                        .on
                        .as_ref()
                        .map_or(u32::MAX, |(handle, _)| handle.index),
                    u32::from(running.skipped),
                ],
            });
        }
        out.reverse();
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use opence_content::object::Attachment;
    use opence_content::{Effect, EffectEvent, EffectPart, Group, ObjectDefinition, ObjectKind};
    use opence_sim::objects::ObjectSpawn;
    use opence_sim::Transform;

    fn tag(group: &[u8; 4], path: &str) -> TagRef {
        TagRef {
            group: Group(*group),
            path: path.into(),
        }
    }

    /// A level whose scenery `rock` carries the effect `smoke`, which makes
    /// the light `puff` at once.
    fn level() -> Level {
        let mut level = opence_content::devmap::level(None);
        level.effects.push(Effect {
            events: vec![EffectEvent {
                skip_fraction: 0.0,
                delay: [0.0, 0.0],
                duration: [1.0, 1.0],
                parts: vec![EffectPart {
                    create_in: 0,
                    create_in_mode: 0,
                    location: 0,
                    flags: 0,
                    tag: tag(b"ligh", "puff"),
                    velocity: [0.0, 0.0],
                    velocity_cone_angle: 0.0,
                    angular_velocity: [0.0, 0.0],
                    radius_modifier: [1.0, 1.0],
                }],
                particles: Vec::new(),
            }],
            locations: vec![String::new()],
            ..Effect::new(tag(b"effe", "smoke"))
        });
        let rock = ObjectDefinition {
            widgets: Vec::new(),
            tag: tag(b"scen", "rock"),
            kind: ObjectKind::Scenery,
            flags: 0,
            bounding_radius: 1.0,
            bounding_offset: [0.0, 0.0, 0.0],
            acceleration_scale: 1.0,
            render_bounding_radius: 1.0,
            model: None,
            animation_graph: None,
            collision_model: None,
            physics: None,
            modifier_shader: None,
            creation_effect: None,
            function_modes: Default::default(),
            functions: Vec::new(),
            change_colors: Vec::new(),
            unit_hud: None,
            material_effects: None,
            unit_melee_damage: None,
            biped: None,
            weapon: None,
            projectile: None,
            equipment: None,
            vehicle: None,
            device: None,
            attachments: vec![
                Attachment {
                    tag: Some(tag(b"effe", "smoke")),
                    marker: String::new(),
                    primary_scale: 0,
                    change_color: 0,
                },
                Attachment {
                    tag: Some(tag(b"ligh", "not an effect")),
                    marker: String::new(),
                    primary_scale: 0,
                    change_color: 0,
                },
            ],
        };
        level.objects.push(rock);
        level
    }

    #[test]
    fn an_objects_carried_effects_start_at_its_markers_and_go_with_it() {
        let level = level();
        let mut effects = LevelEffects::new(&level);
        assert_eq!(effects.carried.len(), 1);
        let mut simulation = Simulation::empty();
        let mut decals = DynamicDecals::new(&level);
        let spawned = simulation.spawn(ObjectSpawn {
            kind: ObjectKind::Scenery,
            tag: tag(b"scen", "rock"),
            transform: Transform {
                position: Vec3::new(1.0, 2.0, 3.0),
                ..Default::default()
            },
            name: None,
            permutation: None,
            team: None,
        });
        let Ok(handle) = spawned else {
            panic!("the rock was not made");
        };
        effects.tick(&level, &mut simulation, &mut decals, 0);
        let fired = effects.fired();
        assert_eq!(fired.len(), 1, "{fired:?}");
        assert_eq!(fired[0].tag.path, "puff");
        assert_eq!(fired[0].position, Vec3::new(1.0, 2.0, 3.0));
        assert_eq!(fired[0].on, Some(handle));
        let mut out = Vec::new();
        effects.gather(&mut out);
        assert_eq!(out.len(), 1);
        assert_eq!(out[0].position[..3], [1.0, 2.0, 3.0]);
        assert_eq!(out[0].ids[2], handle.index);
        // (started once in the rock's life)
        effects.tick(&level, &mut simulation, &mut decals, 0);
        assert_eq!(effects.effects.running().len(), 1);
        assert!(simulation.remove(handle));
        effects.tick(&level, &mut simulation, &mut decals, 0);
        assert!(effects.effects.running().is_empty());
    }

    #[test]
    fn a_turn_is_read_from_a_frames_axes() {
        let frame = NodeMatrix {
            scale: 1.0,
            forward: [0.0, 1.0, 0.0],
            left: [-1.0, 0.0, 0.0],
            up: [0.0, 0.0, 1.0],
            position: [0.0, 0.0, 0.0],
        };
        let forward = turn(&frame) * Vec3::X;
        assert!((forward - Vec3::Y).length() < 1e-5, "{forward}");
    }
}
