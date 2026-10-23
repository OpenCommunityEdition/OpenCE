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
    advance, apply_overlay, base_orientations, node_matrices, root_matrix, AnimationState,
    NodeMatrix, Orientation, Update, MAX_NODES,
};
use crate::clock;
use crate::weapon::{MagazinePhase, WeaponState};
use crate::Transform;

/// The first-person animations, in the graph's order (the game's
/// `_first_person_weapon_animation_*`).
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum FirstPersonAnimation {
    /// Held.
    Idle = 0,
    /// Fired.
    PrimaryFire = 2,
    /// Reloaded, empty.
    ReloadEmpty = 7,
    /// Reloaded, not empty.
    ReloadFull = 8,
    /// Drawn.
    Ready = 10,
    /// Put away.
    PutAway = 11,
    /// A grenade thrown.
    ThrowGrenade = 16,
    /// The rounds shown, a frame for each round loaded.
    Ammunition = 17,
}

/// What the first-person weapon is doing (the game's
/// `_first_person_weapon_state_*`, those the engine plays yet).
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum FirstPersonState {
    /// Held still.
    #[default]
    Idle,
    /// Being drawn.
    Ready,
    /// Firing.
    Fire,
    /// Reloading an empty magazine.
    ReloadEmpty,
    /// Reloading one with rounds still in it.
    ReloadFull,
    /// Throwing a grenade.
    ThrowGrenade,
}

impl FirstPersonState {
    /// Its animation (`first_person_animation_type_from_weapon_state`).
    pub fn animation(self) -> FirstPersonAnimation {
        match self {
            FirstPersonState::Idle => FirstPersonAnimation::Idle,
            FirstPersonState::Ready => FirstPersonAnimation::Ready,
            FirstPersonState::Fire => FirstPersonAnimation::PrimaryFire,
            FirstPersonState::ReloadEmpty => FirstPersonAnimation::ReloadEmpty,
            FirstPersonState::ReloadFull => FirstPersonAnimation::ReloadFull,
            FirstPersonState::ThrowGrenade => FirstPersonAnimation::ThrowGrenade,
        }
    }

    /// Whether `next` may start over it (`first_person_weapon_set_state`):
    /// firing only from holding, firing, or the ends of reloads; drawing
    /// not over drawing.
    fn yields_to(self, next: FirstPersonState) -> bool {
        match next {
            FirstPersonState::Fire => matches!(
                self,
                FirstPersonState::Idle
                    | FirstPersonState::Fire
                    | FirstPersonState::ReloadEmpty
                    | FirstPersonState::ReloadFull
            ),
            FirstPersonState::Ready => self != FirstPersonState::Ready,
            _ => true,
        }
    }
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

/// The nodes of `graph` posed at `state`'s frame with the rounds loaded
/// shown (`first_person_weapon_build_node_matrices`): the state's
/// animation, then the ammunition overlay at the frame of the rounds.
pub fn pose(
    graph: &AnimationGraph,
    state: &AnimationState,
    rounds: Option<i16>,
) -> Vec<Orientation> {
    let mut nodes = vec![Orientation::REST; graph.nodes.len().min(MAX_NODES)];
    if let Some(animation) = graph.animations.get(usize::from(state.animation)) {
        if fits(graph, animation) {
            let frame = i16::try_from(state.frame).unwrap_or(i16::MAX);
            match animation.kind {
                AnimationKind::Base => base_orientations(None, animation, frame, &mut nodes),
                _ => apply_overlay(animation, frame, &mut nodes),
            }
        }
    }
    let ammunition = slot(graph, FirstPersonAnimation::Ammunition)
        .and_then(|index| graph.animations.get(usize::from(index)));
    if let (Some(ammunition), Some(rounds)) = (ammunition, rounds) {
        if fits(graph, ammunition) && rounds >= 0 && (rounds as u16) < ammunition.frame_count {
            apply_overlay(ammunition, rounds, &mut nodes);
        }
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
    state: FirstPersonState,
    animation: AnimationState,
    /// What the weapon was at the last tick, to see it fire and reload.
    rounds: Option<i16>,
    reloading: bool,
    grenades: i16,
    models: Vec<FirstPersonModel>,
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
            rigs.insert(
                object.tag.clone(),
                Rig {
                    graph: Arc::new(graph.clone()),
                    models,
                },
            );
        }
        Self {
            rigs,
            ..Self::default()
        }
    }

    /// What it is doing.
    pub fn state(&self) -> FirstPersonState {
        self.state
    }

    /// The models to draw: none in the third person, dead, or with no
    /// weapon.
    pub fn models(&self) -> &[FirstPersonModel] {
        &self.models
    }

    fn set_state(&mut self, state: FirstPersonState) {
        if !self.state.yields_to(state) {
            return;
        }
        let graph = self
            .weapon
            .as_ref()
            .and_then(|weapon| self.rigs.get(weapon));
        let Some(rig) = graph else { return };
        // (a weapon without the state's animation keeps playing what it
        // was: the rifle has none for firing)
        let Some(animation) = slot(&rig.graph, state.animation()) else {
            return;
        };
        self.state = state;
        self.animation = AnimationState {
            animation,
            frame: 0,
        };
    }

    /// One tick: what the weapon `held` did since the last (fired,
    /// reloaded, changed), the grenades left, and the camera it is posed
    /// from; `held` none hides it.
    pub fn update(
        &mut self,
        tick: u64,
        held: Option<(&TagRef, &WeaponState)>,
        grenades: i16,
        camera: &Transform,
    ) {
        let Some((tag, weapon)) = held.filter(|(tag, _)| self.rigs.contains_key(*tag)) else {
            self.weapon = None;
            self.models.clear();
            return;
        };
        let rounds = weapon.magazines.first().map(|magazine| magazine.loaded);
        let reloading = weapon
            .magazines
            .first()
            .is_some_and(|magazine| magazine.phase == MagazinePhase::Reloading);
        if self.weapon.as_ref() != Some(tag) {
            self.weapon = Some(tag.clone());
            self.state = FirstPersonState::Idle;
            self.set_state(FirstPersonState::Ready);
            self.models.clear();
        } else {
            if reloading && !self.reloading {
                self.set_state(if self.rounds.unwrap_or(0) == 0 {
                    FirstPersonState::ReloadEmpty
                } else {
                    FirstPersonState::ReloadFull
                });
            } else if rounds < self.rounds {
                self.set_state(FirstPersonState::Fire);
            }
            if grenades < self.grenades {
                self.set_state(FirstPersonState::ThrowGrenade);
            }
        }
        self.rounds = rounds;
        self.reloading = reloading;
        self.grenades = grenades;
        let Some(rig) = self.rigs.get(tag) else {
            return;
        };
        // (its animations are the game's 30 frames a second)
        if tick.is_multiple_of(u64::from(clock::TICKS_PER_SECOND / 30)) {
            let update = advance(&rig.graph.animations, &mut self.animation, || 0.0);
            let ended = matches!(update, Update::Restarted);
            if ended && self.state != FirstPersonState::Idle {
                self.state = FirstPersonState::Idle;
                if let Some(idle) = slot(&rig.graph, FirstPersonAnimation::Idle) {
                    self.animation = AnimationState {
                        animation: idle,
                        frame: 0,
                    };
                }
            }
        }
        let orientations = pose(&rig.graph, &self.animation, rounds);
        let matrices = node_matrices(&rig.graph.nodes, &orientations, &root_matrix(camera));
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
    fn firing_waits_for_a_reload_to_finish_but_drawing_does_not() {
        assert!(FirstPersonState::Idle.yields_to(FirstPersonState::Fire));
        assert!(!FirstPersonState::Ready.yields_to(FirstPersonState::Fire));
        assert!(!FirstPersonState::ThrowGrenade.yields_to(FirstPersonState::Fire));
        assert!(FirstPersonState::ReloadFull.yields_to(FirstPersonState::Ready));
        assert!(!FirstPersonState::Ready.yields_to(FirstPersonState::Ready));
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
