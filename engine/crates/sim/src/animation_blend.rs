//! The pose blender: an object's nodes from its base animation, its
//! graph's object overlays, its scale, a unit's layers and the blend from
//! its last pose, in the order `object_get_node_orientations` takes them
//! (objects.c), with a unit's layers in the order
//! `unit_preprocess_node_orientations` takes them (units.c).

use opence_content::{Animation, AnimationGraph, Model, ScreenBounds};

use crate::animation::{
    apply_aiming_screen, apply_overlay, apply_overlay_continuous, apply_overlay_scaled,
    apply_replacement, base_orientations, interpolate_orientations, model_orientations,
    AnimationState, Orientation, MAX_NODES,
};
use crate::animation_graph::{fits, SeatAndWeapon};

/// The functions an object's overlays may read (`NUMBER_OF_OUTGOING_OBJECT_FUNCTIONS`).
pub const OUTGOING_FUNCTIONS: usize = 4;

/// The seat's emotions (`_unit_seat_animation_emotions`).
const SEAT_EMOTIONS: usize = 11;
/// The seat's mouth (`_unit_seat_animation_mouth_aperture`, `talk`).
const SEAT_MOUTH: usize = 10;
/// The seat's first acceleration overlay
/// (`_unit_seat_animation_acceleration_front_back`); the three to
/// `push` are front-back, left-right and up-down.
const SEAT_ACCELERATION: usize = 2;

/// An object function's value as an overlay reads it.
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct FunctionValue {
    /// Its value, from 0 to 1.
    pub value: f32,
    /// Whether the function is additive: a frame overlay then runs to
    /// past its last frame, to its first again.
    pub additive: bool,
}

/// What an object's pose is made of.
#[derive(Clone, Copy, Debug, Default)]
pub struct ObjectLayers<'a> {
    /// Its base animation, if one plays.
    pub base: Option<AnimationState>,
    /// Whether it plays by the game's frame, not its own
    /// (`_object_animates_automatically_bit`).
    pub animates_automatically: bool,
    /// The game's frame for it (`OBJECT_FRAME_INDEX_GET`).
    pub object_frame: u32,
    /// Its outgoing functions' values: as many as its definition has.
    pub functions: &'a [FunctionValue],
    /// Its scale (none at 0 or less).
    pub scale: f32,
    /// The pose it blends from, at which frame of how many
    /// (`interpolation_frame_index` and `interpolation_frame_count`).
    pub interpolation: Option<(&'a [Orientation], i16, i16)>,
}

/// Where a unit aims and looks, for its aiming and looking screens.
#[derive(Clone, Copy, Debug, Default)]
pub struct Aim {
    /// Its aiming screen.
    pub aiming_screen: Option<u16>,
    /// Its looking screen.
    pub looking_screen: Option<u16>,
    /// Its forward.
    pub forward: [f32; 3],
    /// Its up.
    pub up: [f32; 3],
    /// Where it aims, in the world.
    pub aiming_vector: [f32; 3],
    /// Where it looks, in the world.
    pub looking_vector: [f32; 3],
    /// Whether it looks: it holds a weapon or is a player's.
    pub looks: bool,
}

/// A unit's layers over its object's pose.
#[derive(Clone, Copy, Debug, Default)]
pub struct UnitLayers {
    /// Its action, a replacement.
    pub action: Option<AnimationState>,
    /// Its overlay action.
    pub overlay_action: Option<AnimationState>,
    /// Its soft ping.
    pub soft_ping: Option<AnimationState>,
    /// Its seat and weapon; none for a simple creature or a unit with no
    /// seat, which have no seat layers.
    pub seat: Option<SeatAndWeapon>,
    /// Its emotion's frame.
    pub emotion: Option<i16>,
    /// The emotion animation that replaces its seat's.
    pub override_emotion: Option<u16>,
    /// How open its mouth is, from 0 to 1.
    pub mouth_aperture: f32,
    /// Its seat's acceleration, front-back, left-right and up-down, from 0
    /// to 1; none when it does not show it.
    pub acceleration: Option<[f32; 3]>,
    /// Where it aims; none for a unit with no aiming
    /// (`_unit_has_no_aiming_bit`) or in a state that hides it
    /// ([`aiming_screen_shown`]).
    pub aim: Option<Aim>,
}

/// The animation `index` of `graph`, if it fits a model of `model_nodes`
/// nodes (`unit_animation_get_fitting`).
fn fitting(graph: &AnimationGraph, index: u16, model_nodes: usize) -> Option<&Animation> {
    graph
        .animations
        .get(usize::from(index))
        .filter(|animation| fits(animation, None, model_nodes))
}

/// A unit's state `hard-ping` (`_unit_state_hard_ping`).
pub const STATE_HARD_PING: i16 = 23;
/// A unit's state `resurrect-back` (`_unit_state_resurrect_back`).
pub const STATE_RESURRECT_BACK: i16 = 35;
/// A unit's state `leap-melee` (`_unit_state_leap_melee`).
pub const STATE_LEAP_MELEE: i16 = 41;

/// Whether a unit shows its aiming screen (`unit_animation_aiming_screen`):
/// not during an action, nor in the states from hard-ping to
/// resurrect-back (pings, deaths, seats, melee, grenades), nor in a
/// leaping melee.
pub fn aiming_screen_shown(action: bool, state: i16) -> bool {
    !action
        && !(state >= STATE_HARD_PING
            && (state <= STATE_RESURRECT_BACK || state == STATE_LEAP_MELEE))
}

/// The yaw and pitch of `vector` relative to forward and up
/// (`matrix4x3_inverse_transform_normal`, then
/// `euler_angles2d_from_vector3d`): its left is up × forward.
pub fn relative_angles(forward: [f32; 3], up: [f32; 3], vector: [f32; 3]) -> (f32, f32) {
    let left = [
        up[1] * forward[2] - up[2] * forward[1],
        up[2] * forward[0] - up[0] * forward[2],
        up[0] * forward[1] - up[1] * forward[0],
    ];
    let dot = |axis: [f32; 3]| vector[0] * axis[0] + vector[1] * axis[1] + vector[2] * axis[2];
    let (i, j, k) = (dot(forward), dot(left), dot(up));
    let yaw = libm::atan2f(j, i);
    let pitch = libm::atan2f(k, libm::sqrtf(i * i + j * j));
    (yaw, pitch)
}

/// The object's pose before its type's layers: its base animation (at
/// the game's frame when it animates automatically), else its model's
/// pose; its graph's overlays, each driven by its function; its scale on
/// its first node.
pub fn object_pose(
    graph: &AnimationGraph,
    model: Option<&Model>,
    layers: &ObjectLayers,
    nodes: &mut [Orientation],
) {
    let base = layers
        .base
        .and_then(|state| Some((state, graph.animations.get(usize::from(state.animation))?)));
    match base {
        Some((state, animation)) => {
            let frame = if layers.animates_automatically && animation.frame_count > 0 {
                (layers.object_frame % u32::from(animation.frame_count)) as i16
            } else {
                i16::try_from(state.frame).unwrap_or(i16::MAX)
            };
            base_orientations(model, animation, frame, nodes);
        }
        None => {
            if let Some(model) = model {
                model_orientations(model, nodes);
            }
        }
    }

    let model_nodes = model.map_or(nodes.len(), |model| model.nodes.len());
    let functions = layers.functions.len().min(OUTGOING_FUNCTIONS);
    for overlay in &graph.object_overlays {
        let Some(animation) = overlay
            .animation
            .and_then(|index| graph.animations.get(usize::from(index)))
        else {
            continue;
        };
        let Some(function) = usize::try_from(overlay.function)
            .ok()
            .filter(|&index| index < functions)
            .and_then(|index| layers.functions.get(index))
        else {
            continue;
        };
        if usize::from(animation.node_count) > model_nodes {
            continue;
        }
        match overlay.mode {
            0 => {
                let frames = if function.additive {
                    f32::from(animation.frame_count)
                } else {
                    f32::from(animation.frame_count) - 1.0
                };
                apply_overlay_continuous(animation, frames * function.value, nodes);
            }
            1 if animation.frame_count > 0 => {
                let frame = (layers.object_frame % u32::from(animation.frame_count)) as i16;
                apply_overlay_scaled(animation, frame, function.value, nodes);
            }
            _ => {}
        }
    }

    if layers.scale > 0.0 {
        if let Some(first) = nodes.first_mut() {
            first.scale *= layers.scale;
            for axis in &mut first.translation {
                *axis *= layers.scale;
            }
        }
    }
}

/// A unit's layers over its object's pose: its action, overlay action and
/// soft ping; then, in a seat, its emotion, mouth, acceleration and its
/// aiming and looking screens. Each is skipped unless it fits the model's
/// `model_nodes` nodes.
pub fn unit_pose(
    graph: &AnimationGraph,
    model_nodes: usize,
    layers: &UnitLayers,
    nodes: &mut [Orientation],
) {
    let state_animation = |state: Option<AnimationState>| {
        Some((state?, fitting(graph, state?.animation, model_nodes)?))
    };
    let frame = |state: AnimationState| i16::try_from(state.frame).unwrap_or(i16::MAX);
    if let Some((state, animation)) = state_animation(layers.action) {
        apply_replacement(animation, frame(state), nodes);
    }
    if let Some((state, animation)) = state_animation(layers.overlay_action) {
        apply_overlay(animation, frame(state), nodes);
    }
    if let Some((state, animation)) = state_animation(layers.soft_ping) {
        apply_overlay(animation, frame(state), nodes);
    }

    let Some(seat) = layers.seat else {
        return;
    };
    let Some(unit_seat) = seat.seat(graph) else {
        return;
    };
    // (the seat's own index, unchecked against the graph's animations, as
    // the game reads it; fitting checks it)
    let seat_index = |index: usize| unit_seat.animations.get(index).copied().flatten();

    if let Some(emotion) = layers.emotion {
        let index = layers
            .override_emotion
            .or_else(|| seat_index(SEAT_EMOTIONS));
        if let Some(animation) = index.and_then(|index| fitting(graph, index, model_nodes)) {
            if emotion >= 0 && i32::from(emotion) < i32::from(animation.frame_count) {
                apply_overlay(animation, emotion, nodes);
            }
        }
    }

    if layers.mouth_aperture > 0.0 {
        if let Some(animation) =
            seat_index(SEAT_MOUTH).and_then(|index| fitting(graph, index, model_nodes))
        {
            apply_overlay_scaled(animation, 0, layers.mouth_aperture, nodes);
        }
    }

    if let Some(acceleration) = layers.acceleration {
        for (offset, value) in acceleration.iter().enumerate() {
            if let Some(animation) = seat_index(SEAT_ACCELERATION + offset)
                .and_then(|index| fitting(graph, index, model_nodes))
            {
                let frames = f32::from(animation.frame_count) - 1.0;
                apply_overlay_continuous(animation, frames * value, nodes);
            }
        }
    }

    let Some(aim) = layers.aim else {
        return;
    };
    let mut aiming = (0.0, 0.0);
    if let (Some(animation), Some(class)) = (
        aim.aiming_screen
            .and_then(|index| fitting(graph, index, model_nodes)),
        seat.class(graph),
    ) {
        aiming = relative_angles(aim.forward, aim.up, aim.aiming_vector);
        apply_aiming_screen(animation, &class.aiming_bounds, aiming.0, aiming.1, nodes);
    }
    if aim.looks {
        if let Some(animation) = aim
            .looking_screen
            .and_then(|index| fitting(graph, index, model_nodes))
        {
            let (yaw, pitch) = relative_angles(aim.forward, aim.up, aim.looking_vector);
            apply_aiming_screen(
                animation,
                &unit_seat.looking_bounds,
                yaw - aiming.0,
                pitch - aiming.1,
                nodes,
            );
        }
    }
}

/// The extents an aiming or looking screen turns to: left, right, down
/// and up (`aiming_screen_bounds.x0` and the others).
pub fn screen_extents(bounds: &ScreenBounds) -> [f32; 4] {
    [
        -(f32::from(bounds.negative_yaw_frames) * bounds.negative_yaw),
        f32::from(bounds.positive_yaw_frames) * bounds.positive_yaw,
        -(f32::from(bounds.negative_pitch_frames) * bounds.negative_pitch),
        f32::from(bounds.positive_pitch_frames) * bounds.positive_pitch,
    ]
}

/// An object's pose, every layer in the game's order: [`object_pose`],
/// a unit's [`unit_pose`], then the blend from its last pose.
pub fn blend_pose(
    graph: &AnimationGraph,
    model: Option<&Model>,
    object: &ObjectLayers,
    unit: Option<&UnitLayers>,
) -> Vec<Orientation> {
    let count = model
        .map_or(graph.nodes.len(), |model| model.nodes.len())
        .min(MAX_NODES);
    let mut nodes = vec![Orientation::REST; count];
    object_pose(graph, model, object, &mut nodes);
    if let Some(unit) = unit {
        let model_nodes = model.map_or(count, |model| model.nodes.len());
        unit_pose(graph, model_nodes, unit, &mut nodes);
    }
    if let Some((original, frame, frame_count)) = object.interpolation {
        interpolate_orientations(original, &mut nodes, frame, frame_count);
    }
    nodes
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::animation::tests::{animation_named, graph_of};
    use opence_content::{
        AnimationKind, AnimationNode, ObjectOverlay, UnitSeat, WeaponClass, WeaponType,
    };

    type Result = std::result::Result<(), Box<dyn std::error::Error>>;

    /// A one-node animation of `kind` that moves its node to each of
    /// `translations` in turn.
    fn moving(kind: AnimationKind, translations: &[[f32; 3]]) -> Animation {
        let mut animation = animation_named("moving", 1);
        animation.kind = kind;
        animation.frame_count = translations.len() as u16;
        animation.frame_size = 12;
        animation.translation_flags = [1, 0];
        animation.data = translations
            .iter()
            .flatten()
            .flat_map(|value| value.to_le_bytes())
            .collect();
        // (a base animation's node at rest: no turn, at its scale)
        animation.default_data = [0i16, 0, 0, 32767]
            .iter()
            .flat_map(|value| value.to_le_bytes())
            .chain(1.0f32.to_le_bytes())
            .collect();
        animation
    }

    fn x(nodes: &[Orientation]) -> std::result::Result<f32, &'static str> {
        Ok(nodes.first().ok_or("no node")?.translation[0])
    }

    fn close(found: f32, expected: f32) -> Result {
        if (found - expected).abs() > 1e-5 {
            return Err(format!("{found} is not {expected}").into());
        }
        Ok(())
    }

    fn line(count: usize, step: f32) -> Vec<[f32; 3]> {
        (0..count)
            .map(|frame| [frame as f32 * step, 0.0, 0.0])
            .collect()
    }

    fn overlay(animation: u16, function: i16, mode: i16) -> ObjectOverlay {
        ObjectOverlay {
            animation: Some(animation),
            function,
            mode,
        }
    }

    fn function(value: f32) -> FunctionValue {
        FunctionValue {
            value,
            additive: false,
        }
    }

    #[test]
    fn an_objects_overlay_follows_its_function() -> Result {
        let mut graph = graph_of(vec![
            moving(AnimationKind::Overlay, &line(3, 10.0)),
            moving(AnimationKind::Overlay, &line(3, 10.0)),
        ]);
        graph.object_overlays = vec![overlay(0, 0, 0)];
        let functions = [function(0.75)];
        let layers = ObjectLayers {
            functions: &functions,
            ..ObjectLayers::default()
        };
        let mut nodes = [Orientation::REST];
        object_pose(&graph, None, &layers, &mut nodes);
        // (frame 1.5 of 0 to 2)
        close(x(&nodes)?, 15.0)?;

        // an additive function runs through every frame, back to the first
        let functions = [FunctionValue {
            value: 0.5,
            additive: true,
        }];
        let layers = ObjectLayers {
            functions: &functions,
            ..ObjectLayers::default()
        };
        let mut nodes = [Orientation::REST];
        object_pose(&graph, None, &layers, &mut nodes);
        close(x(&nodes)?, 15.0)?;

        // a scaling overlay plays by the game's frame, as far as its value
        graph.object_overlays = vec![overlay(1, 0, 1)];
        let functions = [function(0.5)];
        let layers = ObjectLayers {
            functions: &functions,
            object_frame: 5,
            ..ObjectLayers::default()
        };
        let mut nodes = [Orientation::REST];
        object_pose(&graph, None, &layers, &mut nodes);
        close(x(&nodes)?, 10.0)
    }

    #[test]
    fn an_overlay_without_its_function_or_nodes_is_skipped() -> Result {
        let mut wide = moving(AnimationKind::Overlay, &line(3, 10.0));
        wide.node_count = 2;
        let mut graph = graph_of(vec![moving(AnimationKind::Overlay, &line(3, 10.0)), wide]);
        let functions = [function(1.0); 6];
        graph.object_overlays = vec![
            overlay(0, 1, 0),
            overlay(0, 4, 0),
            overlay(0, -1, 0),
            overlay(9, 0, 0),
            overlay(1, 0, 0),
            overlay(0, 0, 7),
        ];
        let layers = ObjectLayers {
            functions: &functions[..1],
            ..ObjectLayers::default()
        };
        let mut nodes = [Orientation::REST];
        object_pose(&graph, None, &layers, &mut nodes);
        close(x(&nodes)?, 0.0)?;

        // (the fifth function is past those an object has)
        let layers = ObjectLayers {
            functions: &functions,
            ..ObjectLayers::default()
        };
        graph.object_overlays = vec![overlay(0, 4, 0)];
        object_pose(&graph, None, &layers, &mut nodes);
        close(x(&nodes)?, 0.0)
    }

    #[test]
    fn a_base_animation_plays_by_its_state_or_the_games_frame() -> Result {
        let graph = graph_of(vec![moving(AnimationKind::Base, &line(4, 1.0))]);
        let state = AnimationState {
            animation: 0,
            frame: 1,
        };
        let mut layers = ObjectLayers {
            base: Some(state),
            object_frame: 7,
            ..ObjectLayers::default()
        };
        let mut nodes = [Orientation::REST];
        object_pose(&graph, None, &layers, &mut nodes);
        close(x(&nodes)?, 1.0)?;
        layers.animates_automatically = true;
        object_pose(&graph, None, &layers, &mut nodes);
        close(x(&nodes)?, 3.0)
    }

    #[test]
    fn the_objects_scale_scales_its_first_node() -> Result {
        let graph = graph_of(vec![moving(AnimationKind::Base, &[[2.0, 4.0, 6.0]])]);
        let layers = ObjectLayers {
            base: Some(AnimationState::default()),
            scale: 0.5,
            ..ObjectLayers::default()
        };
        let mut nodes = [Orientation::REST; 2];
        object_pose(&graph, None, &layers, &mut nodes);
        let first = nodes.first().ok_or("no node")?;
        if first.translation != [1.0, 2.0, 3.0] || first.scale != 0.5 {
            return Err(format!("{first:?}").into());
        }
        if nodes.get(1) != Some(&Orientation::REST) {
            return Err("the second node was scaled".into());
        }
        Ok(())
    }

    #[test]
    fn a_units_action_replaces_before_its_overlays_add() -> Result {
        let graph = graph_of(vec![
            moving(AnimationKind::Replacement, &[[7.0, 0.0, 0.0]]),
            moving(AnimationKind::Overlay, &[[1.0, 0.0, 0.0]]),
            moving(AnimationKind::Overlay, &[[0.5, 0.0, 0.0]]),
        ]);
        let played = |animation| {
            Some(AnimationState {
                animation,
                frame: 0,
            })
        };
        let layers = UnitLayers {
            action: played(0),
            overlay_action: played(1),
            soft_ping: played(2),
            ..UnitLayers::default()
        };
        let mut nodes = [Orientation::REST];
        unit_pose(&graph, 1, &layers, &mut nodes);
        close(x(&nodes)?, 8.5)?;

        // an animation of more nodes than the model's is skipped
        let mut nodes = [Orientation::REST];
        unit_pose(&graph, 0, &layers, &mut nodes);
        close(x(&nodes)?, 0.0)
    }

    /// A graph of one seat whose animations are `seat` (by the seat's
    /// order), with one weapon class aiming as `aiming`.
    fn seated(
        animations: Vec<Animation>,
        seat: Vec<Option<u16>>,
        aiming: ScreenBounds,
    ) -> AnimationGraph {
        let mut graph = graph_of(animations);
        graph.nodes = vec![AnimationNode {
            name: "root".into(),
            parent: None,
            first_child: None,
            next_sibling: None,
        }];
        graph.unit_seats = vec![UnitSeat {
            label: "stand".into(),
            looking_bounds: aiming,
            animations: seat,
            ik_points: Vec::new(),
            weapon_classes: vec![WeaponClass {
                label: "rifle".into(),
                grip_marker: String::new(),
                hand_marker: String::new(),
                aiming_bounds: aiming,
                animations: Vec::new(),
                ik_points: Vec::new(),
                weapon_types: vec![WeaponType {
                    label: String::new(),
                    animations: Vec::new(),
                }],
            }],
        }];
        graph
    }

    const SEAT: SeatAndWeapon = SeatAndWeapon {
        seat: 0,
        class: 0,
        weapon_type: 0,
        showing_acceleration: true,
    };

    #[test]
    fn a_seat_shows_its_emotion_mouth_and_acceleration() -> Result {
        let mut list = vec![None; 12];
        list[SEAT_ACCELERATION] = Some(1);
        list[SEAT_ACCELERATION + 2] = Some(1);
        list[SEAT_MOUTH] = Some(2);
        list[SEAT_EMOTIONS] = Some(0);
        let graph = seated(
            vec![
                moving(AnimationKind::Overlay, &line(3, 100.0)),
                moving(AnimationKind::Overlay, &line(5, 1.0)),
                moving(AnimationKind::Overlay, &[[10.0, 0.0, 0.0]]),
                moving(AnimationKind::Overlay, &line(3, 1000.0)),
            ],
            list,
            ScreenBounds::default(),
        );
        let base = UnitLayers {
            seat: Some(SEAT),
            ..UnitLayers::default()
        };
        let pose = |layers: &UnitLayers| -> std::result::Result<f32, &'static str> {
            let mut nodes = [Orientation::REST];
            unit_pose(&graph, 1, layers, &mut nodes);
            x(&nodes)
        };
        close(
            pose(&UnitLayers {
                emotion: Some(2),
                ..base
            })?,
            200.0,
        )?;
        // (an emotion past the animation's frames, or none, shows nothing)
        close(
            pose(&UnitLayers {
                emotion: Some(3),
                ..base
            })?,
            0.0,
        )?;
        close(
            pose(&UnitLayers {
                emotion: Some(-1),
                ..base
            })?,
            0.0,
        )?;
        close(
            pose(&UnitLayers {
                emotion: Some(1),
                override_emotion: Some(3),
                ..base
            })?,
            1000.0,
        )?;
        close(
            pose(&UnitLayers {
                mouth_aperture: 0.25,
                ..base
            })?,
            2.5,
        )?;
        // (front-back and up-down, each along its four frames)
        close(
            pose(&UnitLayers {
                acceleration: Some([0.5, 1.0, 0.25]),
                ..base
            })?,
            3.0,
        )?;
        // a unit not in a seat has none of them
        close(
            pose(&UnitLayers {
                seat: None,
                emotion: Some(2),
                mouth_aperture: 1.0,
                ..base
            })?,
            0.0,
        )
    }

    /// A 3 by 3 aiming screen, a frame half a radian each way, whose
    /// frames move the node to their column (x, by 10) and row (y).
    fn screen() -> (AnimationGraph, ScreenBounds) {
        let bounds = ScreenBounds {
            negative_yaw: 0.5,
            positive_yaw: 0.5,
            negative_yaw_frames: 1,
            positive_yaw_frames: 1,
            negative_pitch: 0.5,
            positive_pitch: 0.5,
            negative_pitch_frames: 1,
            positive_pitch_frames: 1,
        };
        let frames: Vec<[f32; 3]> = (0..9)
            .map(|frame| [(frame % 3) as f32 * 10.0, (frame / 3) as f32, 0.0])
            .collect();
        (
            seated(
                vec![moving(AnimationKind::Overlay, &frames)],
                Vec::new(),
                bounds,
            ),
            bounds,
        )
    }

    fn aimed(graph: &AnimationGraph, aim: Aim) -> std::result::Result<[f32; 3], &'static str> {
        let layers = UnitLayers {
            seat: Some(SEAT),
            aim: Some(aim),
            ..UnitLayers::default()
        };
        let mut nodes = [Orientation::REST];
        unit_pose(graph, 1, &layers, &mut nodes);
        Ok(nodes.first().ok_or("no node")?.translation)
    }

    fn towards(yaw: f32, pitch: f32) -> [f32; 3] {
        [
            libm::cosf(yaw) * libm::cosf(pitch),
            libm::sinf(yaw) * libm::cosf(pitch),
            libm::sinf(pitch),
        ]
    }

    const AHEAD: Aim = Aim {
        aiming_screen: Some(0),
        looking_screen: None,
        forward: [1.0, 0.0, 0.0],
        up: [0.0, 0.0, 1.0],
        aiming_vector: [1.0, 0.0, 0.0],
        looking_vector: [1.0, 0.0, 0.0],
        looks: false,
    };

    #[test]
    fn aiming_blends_the_screens_frames_and_stops_at_its_edges() -> Result {
        let (graph, _) = screen();
        let at = |yaw, pitch| {
            aimed(
                &graph,
                Aim {
                    aiming_vector: towards(yaw, pitch),
                    ..AHEAD
                },
            )
        };
        let centre = at(0.0, 0.0)?;
        close(centre[0], 10.0)?;
        close(centre[1], 1.0)?;
        let between = at(0.25, -0.25)?;
        close(between[0], 15.0)?;
        close(between[1], 0.5)?;
        // (far past each edge, the edge's frames)
        let left_up = at(-1.5, 1.4)?;
        close(left_up[0], 0.0)?;
        close(left_up[1], 2.0)?;
        let right_down = at(1.5, -1.4)?;
        close(right_down[0], 20.0)?;
        close(right_down[1], 0.0)
    }

    #[test]
    fn looking_turns_from_where_the_unit_aims() -> Result {
        let (graph, _) = screen();
        let look = Aim {
            aiming_screen: Some(0),
            looking_screen: Some(0),
            aiming_vector: towards(0.25, 0.0),
            looking_vector: towards(0.25, 0.0),
            looks: true,
            ..AHEAD
        };
        // aiming a quarter right, and looking there: the look adds the centre
        let found = aimed(&graph, look)?;
        close(found[0], 25.0)?;
        // a unit that doesn't look (no weapon, no player) only aims
        let found = aimed(
            &graph,
            Aim {
                looks: false,
                ..look
            },
        )?;
        close(found[0], 15.0)?;
        // with no aiming screen the look is from straight ahead
        let found = aimed(
            &graph,
            Aim {
                aiming_screen: None,
                ..look
            },
        )?;
        close(found[0], 15.0)
    }

    #[test]
    fn a_turned_unit_aims_relative_to_its_facing() -> Result {
        let (yaw, pitch) = relative_angles([0.0, 1.0, 0.0], [0.0, 0.0, 1.0], [-1.0, 0.0, 0.0]);
        close(yaw, std::f32::consts::FRAC_PI_2)?;
        close(pitch, 0.0)?;
        let (_, pitch) = relative_angles([1.0, 0.0, 0.0], [0.0, 0.0, 1.0], [0.0, 0.0, 1.0]);
        close(pitch, std::f32::consts::FRAC_PI_2)
    }

    #[test]
    fn some_states_hide_the_aiming_screen() {
        assert!(aiming_screen_shown(false, 0));
        assert!(!aiming_screen_shown(true, 0));
        assert!(!aiming_screen_shown(false, STATE_HARD_PING));
        assert!(!aiming_screen_shown(false, STATE_RESURRECT_BACK));
        assert!(aiming_screen_shown(false, STATE_RESURRECT_BACK + 1));
        assert!(!aiming_screen_shown(false, STATE_LEAP_MELEE));
        assert!(aiming_screen_shown(false, STATE_LEAP_MELEE + 1));
    }

    #[test]
    fn the_screens_extents_are_its_frames_turns() {
        let (_, bounds) = screen();
        assert_eq!(screen_extents(&bounds), [-0.5, 0.5, -0.5, 0.5]);
    }

    #[test]
    fn the_pose_blends_from_the_last_over_the_transition() -> Result {
        let graph = graph_of(vec![moving(AnimationKind::Base, &[[8.0, 0.0, 0.0]])]);
        let mut graph = graph;
        graph.nodes = vec![AnimationNode {
            name: "root".into(),
            parent: None,
            first_child: None,
            next_sibling: None,
        }];
        let original = [Orientation::REST];
        let layers = ObjectLayers {
            base: Some(AnimationState::default()),
            interpolation: Some((&original, 1, 4)),
            ..ObjectLayers::default()
        };
        // (frame 1 of 4 is half the way)
        close(x(&blend_pose(&graph, None, &layers, None))?, 4.0)?;
        let layers = ObjectLayers {
            interpolation: Some((&original, 3, 4)),
            ..layers
        };
        let unit = UnitLayers::default();
        close(x(&blend_pose(&graph, None, &layers, Some(&unit)))?, 8.0)
    }
}
