//! The first-person view's weapon and hands (`first_person_weapons.c`):
//! the weapon's first-person animation graph poses both, from the camera,
//! and each model takes the graph's nodes by name.
//!
//! None of it is the world's state: it is kept beside the world, as the
//! player is, and never hashed.

use std::collections::BTreeMap;
use std::sync::Arc;

use opence_content::{Animation, AnimationGraph, AnimationKind, Level, Model, TagRef};

use crate::animation::{
    apply_overlay, apply_overlay_continuous_scaled, apply_overlay_scaled, base_orientations,
    interpolate_orientations, node_matrices, root_matrix, AnimationState, NodeMatrix, Orientation,
    MAX_NODES,
};
use crate::clock;
pub use crate::first_person_states::{FirstPersonAnimation, FirstPersonState};
use crate::first_person_states::{
    FirstPersonMachine, FirstPersonMessage, HeldMagazine, HeldWeapon, IdleTimes, Surroundings,
    WeaponType,
};
use crate::random::RandomSeed;
use crate::weapon::{MagazinePhase, WeaponState};
use crate::Transform;

/// The overlay frames of the sway: right, left, down, up as the weapon
/// lags behind the holder's run, then the same for its turning, then a
/// shot's push-back (`NUMBER_OF_FIRST_PERSON_WEAPON_OVERLAY_FRAMES`).
const OVERLAY_FRAMES: u16 = 9;

/// The ticks (of the game's 30 a second) a needler's reload goes before
/// its rounds start to show the magazine filling
/// (`FIRST_PERSON_WEAPON_RELOAD_SETTLE_TICKS`).
const RELOAD_SETTLE_TICKS: i16 = 44;

/// What the holder is doing, as the first-person weapon sways after it.
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct Holder {
    /// The throttle, forward and left, each -1 to 1.
    pub throttle: [f32; 2],
    /// Whether it is off the ground.
    pub airborne: bool,
    /// Where it looks: yaw and pitch, in radians.
    pub look: [f32; 2],
    /// Whether it began a melee this tick.
    pub melee: bool,
}

/// A model the first-person view draws, and which of the graph's nodes
/// each of its nodes is.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct RiggedModel {
    /// The model, in the level's models.
    pub model: usize,
    /// For each of its nodes, the graph's node of its name.
    pub remap: Vec<usize>,
}

/// A weapon's first-person graph and the models it poses.
#[derive(Clone, Debug)]
pub struct Rig {
    graph: Arc<AnimationGraph>,
    models: Vec<RiggedModel>,
    /// What the state machine knows of the weapon: its type, and the
    /// graph's animation for each first-person animation.
    weapon_type: WeaponType,
    animations: Vec<Option<u16>>,
    /// The first magazine's rounds when full.
    maximum: i16,
}

/// Which of `graph`'s nodes each of `model`'s nodes is, by name
/// (`model_build_remapping_table_for_animation_graph`); `None` if any of
/// them is not in the graph, and the game does not draw the model.
pub fn remap_nodes(model: &Model, graph: &AnimationGraph) -> Option<Vec<usize>> {
    let graph_nodes = &graph.nodes[..graph.nodes.len().min(MAX_NODES)];
    model
        .nodes
        .iter()
        .take(MAX_NODES)
        .map(|node| graph_nodes.iter().position(|other| other.name == node.name))
        .collect()
}

/// Whether `animation` can go over `graph`'s nodes
/// (`first_person_weapon_animation_fits`).
fn fits(graph: &AnimationGraph, animation: &Animation) -> bool {
    usize::from(animation.node_count) <= graph.nodes.len().min(MAX_NODES)
}

/// The graph's animation for `slot`, if it has one.
fn slot(graph: &AnimationGraph, slot: FirstPersonAnimation) -> Option<u16> {
    graph
        .first_person_weapon
        .get(slot as usize)
        .copied()
        .flatten()
        .filter(|&index| usize::from(index) < graph.animations.len())
}

/// The graph's animation of `index`, if it can go over the graph's nodes.
fn fitting(graph: &AnimationGraph, index: Option<u16>) -> Option<&Animation> {
    let animation = graph.animations.get(usize::from(index?))?;
    fits(graph, animation).then_some(animation)
}

/// The nodes of `graph` posed at `state`'s frame with the rounds loaded
/// shown: the state's animation, then the ammunition overlay at the frame
/// of the rounds.
pub fn pose(
    graph: &AnimationGraph,
    state: &AnimationState,
    rounds: Option<i16>,
) -> Vec<Orientation> {
    let mut nodes = vec![Orientation::REST; graph.nodes.len().min(MAX_NODES)];
    state_pose(graph, Some(state), &mut nodes);
    if let Some(rounds) = rounds {
        ammunition(graph, rounds, &mut nodes);
    }
    nodes
}

/// The state's animation at its frame over the nodes; none leaves them at
/// rest.
fn state_pose(graph: &AnimationGraph, state: Option<&AnimationState>, nodes: &mut [Orientation]) {
    let Some(state) = state else { return };
    let Some(animation) = fitting(graph, Some(state.animation)) else {
        return;
    };
    let frame = i16::try_from(state.frame).unwrap_or(i16::MAX);
    match animation.kind {
        AnimationKind::Base => base_orientations(None, animation, frame, nodes),
        _ => apply_overlay(animation, frame, nodes),
    }
}

/// The ammunition overlay at the frame of `rounds`, if it has one.
fn ammunition(graph: &AnimationGraph, rounds: i16, nodes: &mut [Orientation]) {
    let Some(animation) = fitting(graph, slot(graph, FirstPersonAnimation::Ammunition)) else {
        return;
    };
    if rounds >= 0 && (rounds as u16) < animation.frame_count {
        apply_overlay(animation, rounds, nodes);
    }
}

/// What a pose shows of the weapon besides its state's animation.
struct Shown {
    /// The rounds in its magazine.
    rounds: Option<i16>,
    /// For a needler reloading, the rounds it will hold and the ticks the
    /// reload has gone.
    refilling: Option<(i16, i16)>,
    /// Its charge, 0 to 1.
    overcharged: f32,
}

/// The full pose (`first_person_weapon_build_node_matrices`): the state's
/// animation, the rounds, the run, the charge's shaking, the sway after
/// the holder and a shot's push-back, then the blend from the pose before.
fn full_pose(
    graph: &AnimationGraph,
    machine: &FirstPersonMachine,
    shown: &Shown,
    original: &[Orientation],
) -> Vec<Orientation> {
    let mut nodes = vec![Orientation::REST; graph.nodes.len().min(MAX_NODES)];
    state_pose(graph, machine.animation_state().as_ref(), &mut nodes);

    if let Some(animation) = fitting(graph, slot(graph, FirstPersonAnimation::Ammunition)) {
        match (shown.refilling, shown.rounds) {
            (Some((predicted, ticks)), Some(rounds)) => {
                let mut frame = rounds;
                if ticks >= RELOAD_SETTLE_TICKS {
                    let settle = (f32::from(ticks - RELOAD_SETTLE_TICKS) * 0.2).min(1.0);
                    frame += (f32::from(predicted - rounds) * settle) as i16;
                }
                apply_overlay(animation, frame, &mut nodes);
            }
            (None, Some(rounds)) if rounds >= 0 && (rounds as u16) < animation.frame_count => {
                apply_overlay(animation, rounds, &mut nodes);
            }
            _ => {}
        }
    }

    if let Some(moving) = machine.moving {
        if let Some(animation) = fitting(graph, Some(moving.animation)) {
            let frame = i16::try_from(moving.frame).unwrap_or(i16::MAX);
            apply_overlay(animation, frame, &mut nodes);
        }
    }

    if let Some((jitter, frame)) = machine.jitter {
        if let Some(animation) = fitting(graph, Some(jitter)) {
            apply_overlay_continuous_scaled(animation, frame, shown.overcharged + 0.5, &mut nodes);
        }
    }

    let overlays = fitting(graph, slot(graph, FirstPersonAnimation::Overlays))
        .filter(|animation| animation.frame_count >= OVERLAY_FRAMES);
    if let Some(animation) = overlays {
        // (each lag, with the frame for its positive and its negative side)
        let sways = [
            (machine.position[0], 0, 1),
            (machine.position[1], 3, 2),
            (machine.turning[0], 4, 5),
            (machine.turning[1], 7, 6),
            (machine.push_back, 8, 8),
        ];
        for (lag, positive, negative) in sways {
            if lag > 0.0 {
                apply_overlay_scaled(animation, positive, lag, &mut nodes);
            } else if lag < 0.0 && negative != positive {
                apply_overlay_scaled(animation, negative, -lag, &mut nodes);
            }
        }
    }

    if machine.interpolation_count > 0 {
        interpolate_orientations(
            original,
            &mut nodes,
            machine.interpolation_index,
            machine.interpolation_count,
        );
    }
    nodes
}

/// A first-person model to draw: the level's model, and its nodes in the
/// world at the last two ticks.
#[derive(Clone, Debug, PartialEq)]
pub struct FirstPersonModel {
    /// The model, in the level's models.
    pub model: usize,
    /// Its nodes at the tick before the last.
    pub previous: Vec<NodeMatrix>,
    /// Its nodes at the last tick.
    pub current: Vec<NodeMatrix>,
}

/// The player's first-person weapon.
#[derive(Clone, Debug, Default)]
pub struct FirstPersonWeapon {
    rigs: BTreeMap<TagRef, Rig>,
    /// The weapon it shows.
    weapon: Option<TagRef>,
    machine: FirstPersonMachine,
    /// What the weapon was at the last tick, to see it fire, reload and
    /// overheat.
    rounds: Option<i16>,
    reloading: bool,
    overheated: bool,
    grenades: i16,
    /// The ticks (of 30 a second) the reload has gone.
    reload_ticks: i16,
    /// Where the holder looked at the last of the machine's ticks.
    look: Option<[f32; 2]>,
    random: RandomSeed,
    idle: IdleTimes,
    /// The last pose, and the one a blend starts from.
    orientations: Vec<Orientation>,
    original: Vec<Orientation>,
    models: Vec<FirstPersonModel>,
}

/// What the state machine sees of `weapon` in `rig`.
fn held_weapon<'a>(rig: &'a Rig, weapon: &WeaponState) -> HeldWeapon<'a> {
    HeldWeapon {
        weapon_type: rig.weapon_type,
        overheated: weapon.overheated,
        overheated_exit: false,
        magazine: weapon.magazines.first().map(|magazine| HeldMagazine {
            loaded: magazine.loaded,
            total: magazine.total,
            maximum: rig.maximum,
            busy: magazine.phase != MagazinePhase::Idle,
        }),
        animations: &rig.animations,
    }
}

impl FirstPersonWeapon {
    /// The first-person rigs of `level`'s weapons: each one's graph, its
    /// first-person model and the globals' hands, for those whose every
    /// node the graph has.
    pub fn new(level: &Level) -> Self {
        let model_index = |tag: Option<&TagRef>| tag.and_then(|tag| level.model_index(tag));
        let hands = model_index(level.globals.first_person_hands.as_ref());
        let mut rigs = BTreeMap::new();
        for object in &level.objects {
            let Some(weapon) = object.weapon.as_ref() else {
                continue;
            };
            let Some(graph) = weapon
                .first_person_animations
                .as_ref()
                .and_then(|tag| level.animation_graph(tag))
            else {
                continue;
            };
            let models = [hands, model_index(weapon.first_person_model.as_ref())]
                .into_iter()
                .flatten()
                .filter_map(|model| {
                    let remap = remap_nodes(&level.models[model], graph)?;
                    Some(RiggedModel { model, remap })
                })
                .collect();
            let animations = graph
                .first_person_weapon
                .iter()
                .map(|index| index.filter(|&index| usize::from(index) < graph.animations.len()))
                .collect();
            rigs.insert(
                object.tag.clone(),
                Rig {
                    graph: Arc::new(graph.clone()),
                    models,
                    weapon_type: WeaponType::from_index(weapon.weapon_type),
                    animations,
                    maximum: weapon
                        .magazines
                        .first()
                        .map_or(0, |magazine| magazine.rounds_loaded_maximum),
                },
            );
        }
        let player = &level.globals.player;
        Self {
            rigs,
            idle: IdleTimes {
                lower: player.first_person_idle_lower,
                upper: player.first_person_idle_upper,
                skip_fraction: player.first_person_idle_skip_fraction,
            },
            ..Self::default()
        }
    }

    /// What it is doing; none before a weapon is drawn.
    pub fn state(&self) -> Option<FirstPersonState> {
        self.machine.state
    }

    /// The models to draw: none in the third person, dead, or with no
    /// weapon.
    pub fn models(&self) -> &[FirstPersonModel] {
        &self.models
    }

    /// Tells the state machine of `message`, as the weapon does.
    fn message(&mut self, rig: &Rig, weapon: &WeaponState, message: FirstPersonMessage) {
        let held = held_weapon(rig, weapon);
        if message == FirstPersonMessage::Ready {
            self.machine.switch_weapons(&held);
        }
        self.machine.message(Some(&held), message);
    }

    /// One tick: what the weapon `held` did since the last (fired,
    /// reloaded, overheated, changed), the grenades left, what its holder
    /// did, and the camera it is posed from; `held` none hides it.
    pub fn update(
        &mut self,
        tick: u64,
        held: Option<(&TagRef, &WeaponState)>,
        grenades: i16,
        holder: &Holder,
        camera: &Transform,
    ) {
        let Some((tag, weapon)) = held.filter(|(tag, _)| self.rigs.contains_key(*tag)) else {
            self.weapon = None;
            self.models.clear();
            return;
        };
        let Some(rig) = self.rigs.get(tag).cloned() else {
            return;
        };
        let magazine = weapon.magazines.first();
        let rounds = magazine.map(|magazine| magazine.loaded);
        let reloading = magazine.is_some_and(|magazine| magazine.phase == MagazinePhase::Reloading);
        if self.weapon.as_ref() != Some(tag) {
            self.weapon = Some(tag.clone());
            self.models.clear();
            self.look = None;
            self.message(&rig, weapon, FirstPersonMessage::Ready);
        } else {
            if reloading && !self.reloading {
                let message = if self.rounds.unwrap_or(0) == 0 {
                    FirstPersonMessage::ReloadWhileEmpty
                } else {
                    FirstPersonMessage::ReloadWhileFull
                };
                self.message(&rig, weapon, message);
            } else if rounds < self.rounds {
                self.message(&rig, weapon, FirstPersonMessage::PrimaryFire);
            }
            if holder.melee {
                self.message(&rig, weapon, FirstPersonMessage::Melee);
            }
            if grenades < self.grenades {
                self.message(&rig, weapon, FirstPersonMessage::ThrowGrenade);
            }
            if weapon.overheated && !self.overheated {
                self.message(&rig, weapon, FirstPersonMessage::Overheating);
            }
        }
        if !reloading {
            self.reload_ticks = 0;
        }
        self.rounds = rounds;
        self.reloading = reloading;
        self.overheated = weapon.overheated;
        self.grenades = grenades;

        // (the game's first person runs at its 30 ticks a second)
        if tick.is_multiple_of(u64::from(clock::TICKS_PER_SECOND / 30)) {
            let turned = self.look.map_or([0.0; 2], |[yaw, pitch]| {
                [
                    libm::remainderf(holder.look[0] - yaw, std::f32::consts::TAU),
                    libm::remainderf(holder.look[1] - pitch, std::f32::consts::TAU),
                ]
            });
            self.look = Some(holder.look);
            let surroundings = Surroundings {
                throttle: holder.throttle,
                moving: libm::hypotf(holder.throttle[0], holder.throttle[1]) > 0.1
                    && !holder.airborne,
                rendered: true,
                turned,
                aiming: false,
                overcharged: 0.0,
            };
            let held = held_weapon(&rig, weapon);
            self.machine.update(
                &held,
                &rig.graph.animations,
                &surroundings,
                &self.idle,
                &mut self.random,
            );
            if reloading {
                self.reload_ticks = self.reload_ticks.saturating_add(1);
            }
        }

        let nodes = rig.graph.nodes.len().min(MAX_NODES);
        if self.machine.interpolation_started {
            self.machine.interpolation_started = false;
            self.original = self.orientations.clone();
        }
        self.original.resize(nodes, Orientation::REST);
        let refilling = (rig.weapon_type == WeaponType::Needler
            && self.machine.state.is_some_and(|state| {
                matches!(
                    state,
                    FirstPersonState::ReloadWhileEmpty | FirstPersonState::ReloadWhileFull
                )
            }))
        .then(|| {
            let total = magazine.map_or(0, |magazine| magazine.total);
            (total.min(rig.maximum), self.reload_ticks)
        });
        let shown = Shown {
            rounds,
            refilling,
            overcharged: 0.0,
        };
        let orientations = full_pose(&rig.graph, &self.machine, &shown, &self.original);
        let matrices = node_matrices(&rig.graph.nodes, &orientations, &root_matrix(camera));
        self.orientations = orientations;
        let fresh = self.models.len() != rig.models.len();
        let models: Vec<FirstPersonModel> = rig
            .models
            .iter()
            .enumerate()
            .map(|(at, rigged)| {
                let current: Vec<NodeMatrix> = rigged
                    .remap
                    .iter()
                    .map(|&node| matrices.get(node).copied().unwrap_or(NodeMatrix::IDENTITY))
                    .collect();
                let previous = match self.models.get(at) {
                    Some(earlier) if !fresh && earlier.model == rigged.model => {
                        earlier.current.clone()
                    }
                    _ => current.clone(),
                };
                FirstPersonModel {
                    model: rigged.model,
                    previous,
                    current,
                }
            })
            .collect();
        self.models = models;
    }

    /// Hides it (the third person, or death).
    pub fn hide(&mut self) {
        self.models.clear();
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use opence_content::{AnimationNode, Group, ModelNode};

    fn node(name: &str) -> ModelNode {
        ModelNode {
            name: name.into(),
            parent: None,
            first_child: None,
            next_sibling: None,
            translation: [0.0; 3],
            rotation: [0.0, 0.0, 0.0, 1.0],
            distance_from_parent: 0.0,
            default_inverse_scale: 1.0,
            default_inverse: [[0.0; 3]; 4],
        }
    }

    fn model(names: &[&str]) -> Model {
        Model {
            tag: TagRef {
                group: Group(*b"mod2"),
                path: "m".into(),
            },
            flags: 0,
            node_list_checksum: 0,
            detail_cutoffs: [0.0; 5],
            base_map_scale: [1.0; 2],
            nodes: names.iter().map(|name| node(name)).collect(),
            markers: Vec::new(),
            regions: Vec::new(),
            geometries: Vec::new(),
            shaders: Vec::new(),
        }
    }

    fn graph(names: &[&str]) -> AnimationGraph {
        AnimationGraph {
            nodes: names
                .iter()
                .map(|name| AnimationNode {
                    name: (*name).into(),
                    parent: None,
                    first_child: None,
                    next_sibling: None,
                })
                .collect(),
            tag: TagRef {
                group: Group(*b"antr"),
                path: "a".into(),
            },
            flags: 0,
            limp_body_node_radius: 0.0,
            animations: Vec::new(),
            object_overlays: Vec::new(),
            unit_seats: Vec::new(),
            weapon: Vec::new(),
            vehicle: None,
            device: Vec::new(),
            unit_damage: Vec::new(),
            first_person_weapon: Vec::new(),
            sound_references: Vec::new(),
        }
    }

    #[test]
    fn nodes_are_remapped_by_name() {
        let graph = graph(&["frame gun", "l hand", "r hand"]);
        assert_eq!(
            remap_nodes(&model(&["r hand", "frame gun"]), &graph),
            Some(vec![2, 0])
        );
        // (a node the graph doesn't have: the model isn't drawn)
        assert_eq!(remap_nodes(&model(&["r hand", "bayonet"]), &graph), None);
    }

    #[test]
    fn a_pose_without_animations_is_at_rest() {
        let graph = graph(&["a", "b"]);
        let nodes = pose(
            &graph,
            &AnimationState {
                animation: 0,
                frame: 0,
            },
            Some(3),
        );
        assert_eq!(nodes, vec![Orientation::REST; 2]);
    }
}
