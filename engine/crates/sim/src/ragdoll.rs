//! Ragdolls (`ragdoll_bodies.c`, `ragdolls.c`): every biped that dies
//! becomes a body of parts joined by swing-twist joints, built from its
//! model's nodes and its collision model's pieces. Its motors follow the
//! biped's pose (its death animation, if one plays) for a moment, then it
//! goes limp and settles where it falls. Explosions push the ragdolls in
//! reach and can tear their limbs off at joints the overrides let break.
//! The corpses stay, up to a cap; the oldest goes first.

use std::collections::BTreeMap;
use std::f32::consts::FRAC_PI_2;
use std::sync::Arc;

use bevy_ecs::prelude::*;
use glam::{Affine3A, Mat3, Quat, Vec3};
use opence_content::{
    AnimationNode, CollisionNode, DamageEffect, Level, Model, ObjectKind, TagRef,
};
use opence_physics::{
    shape_volume, Material, Pose, RagdollId, RagdollPart, Shape, World as PhysicsWorld,
    RAGDOLL_PARTS_MAX,
};

use crate::animation::{
    model_orientations, node_matrices, root_matrix, AnimationPlayer, NodeMatrices, NodeMatrix,
    Orientation,
};
use crate::damage::DamageTable;
use crate::objects::{Object, ObjectHandle, ObjectTable};
use crate::projectile::Detonation;
use crate::Transform;

/// The ragdolls' overrides: each model's mass, forced and folded parts,
/// joints, shapes and the damage that breaks its joints.
pub const OVERRIDES: &str = include_str!("../../../../port/assets/ragdolls/ragdolls.txt");

/// A piece smaller than this share of the whole is folded into its
/// parent's part, unless it is a limb's.
const PART_VOLUME_SHARE: f32 = 0.02;
/// The least half size of a shape.
const SIZE_MIN: f32 = 0.01;
/// A piece this many times longer than it is wide is a capsule.
const CAPSULE_LENGTH: f32 = 1.5;
/// A body's mass, without an override, in kilograms.
const MASS_DEFAULT: f32 = 80.0;
/// The lightest a part is, as a share of its body.
const PART_MASS_SHARE: f32 = 0.01;
/// The most nodes a body is built from.
const NODES_MAX: usize = 64;
/// The longest node name the overrides match.
const NAME_MAX: usize = 32;

/// How long the motors follow the biped's pose: 12 of the game's ticks.
pub const DRIVE_TICKS: u32 = 24;
/// How stiffly they follow it, in hertz.
const DRIVE_FREQUENCY: f32 = 8.0;
/// The most joints one hit tears.
const BREAKS_PER_HIT: usize = 2;
/// How far up a push is lifted.
const PUSH_LIFT: f32 = 0.45;
/// The most corpses kept, by default.
pub const CORPSES_DEFAULT: usize = 64;
/// `_damage_category_grenade`, `_highexplosive` and `_mountedweapon`.
const EXPLOSION_CATEGORIES: [i16; 3] = [3, 4, 8];
/// How a ragdoll slides and bounces.
const MATERIAL: Material = Material {
    friction: 0.8,
    restitution: 0.05,
};

/// A joint's limits by its bone's name, in degrees (the first match); a
/// limb's node is a part wherever it has a piece, however small.
struct JointClass {
    name: &'static str,
    twist: f32,
    swing_normal: f32,
    swing_plane: f32,
    limb: bool,
}

const fn joint(name: &'static str, twist: f32, normal: f32, plane: f32, limb: bool) -> JointClass {
    JointClass {
        name,
        twist,
        swing_normal: normal,
        swing_plane: plane,
        limb,
    }
}

const JOINT_CLASSES: [JointClass; 13] = [
    // (hinges: bending about z)
    joint("calf", 5.0, 5.0, 70.0, true),
    joint("horselink", 5.0, 5.0, 70.0, true),
    joint("forearm", 5.0, 5.0, 70.0, true),
    joint("thigh", 20.0, 50.0, 50.0, true),
    joint("upperarm", 40.0, 80.0, 80.0, true),
    joint("clavicle", 10.0, 20.0, 20.0, false),
    joint("spine", 15.0, 20.0, 20.0, true),
    joint("neck", 40.0, 40.0, 40.0, true),
    joint("head", 40.0, 40.0, 40.0, true),
    joint("hand", 10.0, 30.0, 30.0, true),
    joint("foot", 10.0, 30.0, 30.0, true),
    joint("toe", 10.0, 30.0, 30.0, false),
    // (frames, flaps, spikes, sacks)
    joint("", 10.0, 20.0, 20.0, false),
];

fn joint_class(name: &str) -> &'static JointClass {
    let name: String = name
        .chars()
        .take(NAME_MAX - 1)
        .map(|c| c.to_ascii_lowercase())
        .collect();
    JOINT_CLASSES
        .iter()
        .find(|class| name.contains(class.name))
        .unwrap_or(&JOINT_CLASSES[JOINT_CLASSES.len() - 1])
}

/// A part of a ragdoll's body: a node's piece, joined to its parent part
/// at the node's origin.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct BodyPart {
    /// Its node.
    pub node: usize,
    /// The part it hangs from; none for the first.
    pub parent: Option<usize>,
    /// Its shape (a capsule along the node's x).
    pub shape: Shape,
    /// The shape's centre, in the node's frame.
    pub centre: Vec3,
    /// The least and most twist about the node's x, in radians.
    pub twist: [f32; 2],
    /// Half the swing cone about the node's y, in radians.
    pub swing_normal: f32,
    /// Half the swing cone about the node's z, in radians.
    pub swing_plane: f32,
    /// Its mass, in kilograms.
    pub mass: f32,
    /// The damage that tears its joint (none, if it does not tear).
    pub break_damage: f32,
}

impl BodyPart {
    fn physics(&self) -> RagdollPart {
        // (the physics' capsule lies along z; the part's along x)
        let rotation = match self.shape {
            Shape::Capsule { .. } => Quat::from_rotation_y(FRAC_PI_2),
            _ => Quat::IDENTITY,
        };
        RagdollPart {
            parent: self.parent,
            shape: self.shape,
            offset: Pose {
                position: self.centre,
                rotation,
            },
            mass: self.mass,
            twist_axis: Vec3::X,
            plane_axis: Vec3::Z,
            twist: self.twist,
            swing_normal: self.swing_normal,
            swing_plane: self.swing_plane,
        }
    }
}

/// A biped's ragdoll body: its parts, and each node's part.
#[derive(Clone, Debug, PartialEq)]
pub struct RagdollBody {
    /// The whole body's mass, in kilograms.
    pub mass: f32,
    /// Its parts, each after its parent.
    pub parts: Vec<BodyPart>,
    /// The part each of the model's nodes moves with.
    pub node_parts: Vec<usize>,
}

// ---------- the overrides

/// Each line of `text`, without its comment and trimmed.
fn override_lines(text: &str) -> impl Iterator<Item = &str> {
    text.split('\n').map(|line| {
        let line = line.split('#').next().unwrap_or_default();
        line.trim_end_matches([' ', '\t', '\r'])
            .trim_start_matches([' ', '\t'])
    })
}

/// What follows `word` on `line`, if it starts with it.
fn override_word<'a>(line: &'a str, word: &str) -> Option<&'a str> {
    let rest = line.strip_prefix(word)?;
    if !(rest.is_empty() || rest.starts_with([' ', '\t'])) {
        return None;
    }
    Some(rest.trim_start_matches([' ', '\t']))
}

/// A number (an optional sign, digits, an optional fraction) after any
/// spaces, and what follows it.
fn override_number(text: &str) -> Option<(f32, &str)> {
    let text = text.trim_start_matches([' ', '\t']);
    let bytes = text.as_bytes();
    let mut at = 0;
    let mut negative = false;
    if let Some(&sign @ (b'-' | b'+')) = bytes.first() {
        negative = sign == b'-';
        at += 1;
    }
    let mut digits = false;
    let mut result = 0.0f32;
    while let Some(&digit @ b'0'..=b'9') = bytes.get(at) {
        result = 10.0 * result + f32::from(digit - b'0');
        digits = true;
        at += 1;
    }
    if bytes.get(at) == Some(&b'.') {
        at += 1;
        let mut scale = 1.0f32;
        while let Some(&digit @ b'0'..=b'9') = bytes.get(at) {
            scale /= 10.0;
            result += scale * f32::from(digit - b'0');
            digits = true;
            at += 1;
        }
    }
    if !digits {
        return None;
    }
    Some((if negative { -result } else { result }, &text[at..]))
}

/// A line's node name, up to its `:`: the node's index, and what follows.
fn override_node<'a>(model: &Model, text: &'a str) -> (Option<usize>, &'a str) {
    let (name, rest) = match text.split_once(':') {
        Some((name, rest)) => (name, rest),
        None => (text, ""),
    };
    let name = name.trim_end_matches([' ', '\t']);
    if name.is_empty() || name.len() >= NAME_MAX {
        return (None, rest);
    }
    let node = model
        .nodes
        .iter()
        .take(NODES_MAX)
        .position(|node| truncated(&node.name).eq_ignore_ascii_case(name));
    (node, rest)
}

fn truncated(name: &str) -> &str {
    let mut end = name.len().min(NAME_MAX - 1);
    while !name.is_char_boundary(end) {
        end -= 1;
    }
    &name[..end]
}

/// The lines of `model`'s section of the overrides (from its `model` line
/// to the next); none if it has none.
fn override_section<'a>(overrides: &'a str, model: &str) -> Option<Vec<&'a str>> {
    let mut section: Option<Vec<&str>> = None;
    for line in override_lines(overrides) {
        if let Some(name) = override_word(line, "model") {
            if section.is_some() {
                break;
            }
            if name.eq_ignore_ascii_case(model) {
                section = Some(Vec::new());
            }
        } else if let Some(lines) = section.as_mut() {
            lines.push(line);
        }
    }
    section
}

/// An override's angle in degrees, as radians within [low, high].
fn override_angle(text: &str, low: f32, high: f32) -> Option<(f32, &str)> {
    let (degrees, rest) = override_number(text)?;
    (low..=high)
        .contains(&degrees)
        .then(|| (degrees.to_radians(), rest))
}

/// A joint's, a shape's or a break's override line, applied to the part
/// of its node (only a node that is its part's own).
fn override_part(body: &mut RagdollBody, model: &Model, line: &str) {
    enum Kind {
        Joint,
        Shape(usize),
        Break,
    }
    let words = [
        ("joint", Kind::Joint),
        ("sphere", Kind::Shape(1)),
        ("capsule", Kind::Shape(2)),
        ("box", Kind::Shape(3)),
        ("break", Kind::Break),
    ];
    let Some((kind, rest)) = words
        .into_iter()
        .find_map(|(word, kind)| override_word(line, word).map(|rest| (kind, rest)))
    else {
        return;
    };
    let (Some(node), text) = override_node(model, rest) else {
        return;
    };
    let Some(&index) = body.node_parts.get(node) else {
        return;
    };
    let Some(part) = body.parts.get_mut(index).filter(|part| part.node == node) else {
        return;
    };
    let mut changed = *part;
    match kind {
        Kind::Break => {
            // (the root has no joint to tear)
            let Some((damage, _)) = override_number(text) else {
                return;
            };
            if part.parent.is_none() || !(1.0..=100_000.0).contains(&damage) {
                return;
            }
            changed.break_damage = damage;
        }
        Kind::Joint => {
            let parsed = (|| {
                let text = override_word(text.trim_start_matches([' ', '\t']), "twist")?;
                let (minimum, text) = override_angle(text, -180.0, 180.0)?;
                let (maximum, text) = override_angle(text, -180.0, 180.0)?;
                let text = override_word(text.trim_start_matches([' ', '\t']), "swing")?;
                let (normal, text) = override_angle(text, 0.0, 180.0)?;
                let (plane, _) = override_angle(text, 0.0, 180.0)?;
                (minimum <= maximum).then_some(([minimum, maximum], normal, plane))
            })();
            let Some((twist, normal, plane)) = parsed else {
                return;
            };
            changed.twist = twist;
            changed.swing_normal = normal;
            changed.swing_plane = plane;
        }
        Kind::Shape(count) => {
            let mut sizes = [0.0f32; 3];
            let mut text = text;
            for (index, size) in sizes.iter_mut().enumerate().take(count) {
                let Some((value, rest)) = override_number(text) else {
                    return;
                };
                // (a capsule's half length may be nothing)
                let least = if count == 2 && index == 1 {
                    0.0
                } else {
                    SIZE_MIN
                };
                if !(least..=10.0).contains(&value) {
                    return;
                }
                *size = value;
                text = rest;
            }
            changed.shape = match count {
                1 => Shape::Sphere { radius: sizes[0] },
                2 => Shape::Capsule {
                    radius: sizes[0],
                    height: 2.0 * sizes[1],
                },
                _ => Shape::Box {
                    half_extents: Vec3::from_array(sizes),
                },
            };
        }
    }
    *part = changed;
}

// ---------- the body

/// The bounds of a collision node's first bsp's vertices; none if it has
/// none, or one is not a number or is far out.
fn node_bounds(node: &CollisionNode) -> Option<(Vec3, Vec3)> {
    let vertices = &node.bsps.first()?.vertices;
    let mut bounds: Option<(Vec3, Vec3)> = None;
    for vertex in vertices {
        let point = Vec3::from_array(vertex.point);
        if point.is_nan() || point.abs().max_element() > 100.0 {
            return None;
        }
        bounds = Some(match bounds {
            Some((minimum, maximum)) => (minimum.min(point), maximum.max(point)),
            None => (point, point),
        });
    }
    bounds
}

fn extent(minimum: f32, maximum: f32) -> f32 {
    (0.5 * (maximum - minimum)).max(SIZE_MIN)
}

fn extents(minimum: Vec3, maximum: Vec3) -> Vec3 {
    Vec3::new(
        extent(minimum.x, maximum.x),
        extent(minimum.y, maximum.y),
        extent(minimum.z, maximum.z),
    )
}

/// A part's shape from its piece's bounds: a capsule along x when it is
/// long, else a box; and its centre.
fn shape_from_bounds(minimum: Vec3, maximum: Vec3) -> (Shape, Vec3) {
    let size = extents(minimum, maximum);
    let centre = 0.5 * (minimum + maximum);
    if size.x > CAPSULE_LENGTH * size.y.max(size.z) {
        let radius = 0.5 * (size.y + size.z);
        let half = (size.x - radius).max(0.0);
        (
            Shape::Capsule {
                radius,
                height: 2.0 * half,
            },
            centre,
        )
    } else {
        (Shape::Box { half_extents: size }, centre)
    }
}

/// A sphere at a node's origin, half as far across as its furthest child.
fn shape_between_children(model: &Model, node: usize) -> Shape {
    let radius = model
        .nodes
        .iter()
        .enumerate()
        .take(NODES_MAX)
        .skip(node + 1)
        .filter(|(_, child)| child.parent.map(usize::from) == Some(node))
        .map(|(_, child)| 0.5 * Vec3::from_array(child.translation).length())
        .fold(0.0f32, f32::max);
    Shape::Sphere {
        radius: radius.max(2.0 * SIZE_MIN),
    }
}

/// The ragdoll body of a biped's `model`, its pieces from `pieces` (its
/// collision model's nodes: the one of each node's name) and changed by its section
/// of `overrides`; none if the model is not one a body can be built of
/// (too few or too many nodes, out of order, no pieces, fewer than two
/// parts) or its overrides say `none`.
pub fn build_body(model: &Model, pieces: &[CollisionNode], overrides: &str) -> Option<RagdollBody> {
    let count = model.nodes.len();
    if !(2..=NODES_MAX).contains(&count) {
        return None;
    }
    let mut bounds: Vec<Option<(Vec3, Vec3)>> = Vec::with_capacity(count);
    let mut volumes = vec![0.0f32; count];
    let mut total_volume = 0.0f32;
    // (each node after its parent)
    for (index, node) in model.nodes.iter().enumerate() {
        let ordered = match node.parent.map(usize::from) {
            None => index == 0,
            Some(parent) => index > 0 && parent < index,
        };
        if !ordered {
            return None;
        }
        let name = truncated(&node.name);
        let piece = pieces
            .iter()
            .find(|piece| truncated(&piece.name).eq_ignore_ascii_case(name));
        let node_bounds = piece.and_then(node_bounds);
        if let Some((minimum, maximum)) = node_bounds {
            let size = extents(minimum, maximum);
            volumes[index] = 8.0 * size.x * size.y * size.z;
            total_volume += volumes[index];
        }
        bounds.push(node_bounds);
    }
    if total_volume <= 0.0 {
        return None;
    }

    // (the overrides that choose the parts: 1 a part, -1 folded)
    let section = override_section(overrides, &model.tag.path).unwrap_or_default();
    let mut mass = MASS_DEFAULT;
    let mut forced = vec![0i8; count];
    for &line in &section {
        if line == "none" {
            return None;
        }
        if let Some(rest) = override_word(line, "mass") {
            if let Some((value, _)) = override_number(rest) {
                if (1.0..=10_000.0).contains(&value) {
                    mass = value;
                }
            }
        } else if let Some(rest) = override_word(line, "part") {
            if let (Some(node), _) = override_node(model, rest) {
                forced[node] = 1;
            }
        } else if let Some(rest) = override_word(line, "fold") {
            if let (Some(node), _) = override_node(model, rest) {
                if node > 0 {
                    forced[node] = -1;
                }
            }
        }
    }

    let mut body = RagdollBody {
        mass,
        parts: Vec::new(),
        node_parts: vec![0; count],
    };
    let mut mass_volumes: Vec<f32> = Vec::new();
    let mut total_mass_volume = 0.0f32;
    for (index, node) in model.nodes.iter().enumerate() {
        let class = joint_class(&node.name);
        let big = bounds[index].is_some()
            && (class.limb || volumes[index] >= PART_VOLUME_SHARE * total_volume);
        let is_part = index == 0
            || match forced[index] {
                0 => big,
                forced => forced > 0,
            };
        let parent_part = node
            .parent
            .map(|parent| body.node_parts[usize::from(parent)]);
        if !is_part || body.parts.len() >= RAGDOLL_PARTS_MAX {
            // (folded into its parent's part; the root is always a part)
            let part = parent_part.unwrap_or(0);
            body.node_parts[index] = part;
            if let Some(volume) = mass_volumes.get_mut(part) {
                *volume += volumes[index];
            }
            total_mass_volume += volumes[index];
            continue;
        }
        let (shape, centre) = match bounds[index] {
            Some((minimum, maximum)) => shape_from_bounds(minimum, maximum),
            None => (shape_between_children(model, index), Vec3::ZERO),
        };
        body.node_parts[index] = body.parts.len();
        body.parts.push(BodyPart {
            node: index,
            parent: parent_part,
            shape,
            centre,
            twist: [(-class.twist).to_radians(), class.twist.to_radians()],
            swing_normal: class.swing_normal.to_radians(),
            swing_plane: class.swing_plane.to_radians(),
            mass: 0.0,
            break_damage: 0.0,
        });
        let volume = if bounds[index].is_some() {
            volumes[index]
        } else {
            shape_volume(&shape)
        };
        mass_volumes.push(volume);
        total_mass_volume += volume;
    }
    if body.parts.len() < 2 {
        return None;
    }

    // (the overrides of the joints and the shapes)
    for &line in &section {
        override_part(&mut body, model, line);
    }

    // (the masses, by volume)
    for (part, volume) in body.parts.iter_mut().zip(&mass_volumes) {
        part.mass = (mass * volume / total_mass_volume).max(PART_MASS_SHARE * mass);
    }
    Some(body)
}

// ---------- the ragdolls

/// A biped definition's ragdoll: its body and its model's nodes.
#[derive(Clone, Debug)]
struct Species {
    body: RagdollBody,
    /// The model's nodes' links.
    links: Vec<AnimationNode>,
    /// The nodes at the model's default pose, from the object's origin.
    rest: Vec<NodeMatrix>,
    /// How much a push moves it.
    acceleration_scale: f32,
}

/// A ragdoll in the world.
#[derive(Clone, Debug)]
pub struct Ragdoll {
    /// The biped it is.
    pub object: ObjectHandle,
    id: RagdollId,
    species: Arc<Species>,
    /// The ticks since it started.
    pub age: u32,
    /// Its motors are off.
    pub limp: bool,
    /// The parts whose joints are torn, a bit each.
    pub torn: u32,
    /// Each part's frame in the world.
    poses: Vec<Pose>,
    /// Each node from its part's node, as it died.
    from_part: Vec<Affine3A>,
    /// Each node's scale.
    scales: Vec<f32>,
    /// The object's origin from its root part, as it died.
    origin: Vec3,
}

impl Ragdoll {
    /// Each part's frame in the world, as at the last tick.
    pub fn poses(&self) -> &[Pose] {
        &self.poses
    }

    /// Its body.
    pub fn body(&self) -> &RagdollBody {
        &self.species.body
    }
}

/// A hit's damage, as a ragdoll takes it.
struct Hit {
    epicenter: Vec3,
    explosion: bool,
    scaled: bool,
    damage_minimum: f32,
    damage_maximum: f32,
    falloff: f32,
    cutoff: f32,
    /// In world units a second.
    speed: f32,
}

impl Hit {
    fn from_effect(effect: &DamageEffect, epicenter: Vec3, acceleration_scale: f32) -> Hit {
        let [falloff, cutoff] = effect.radius;
        Hit {
            epicenter,
            explosion: EXPLOSION_CATEGORIES.contains(&effect.category),
            scaled: effect.flags & DamageEffect::DONT_SCALE_DAMAGE_BY_DISTANCE == 0,
            damage_minimum: effect.damage_minimum,
            damage_maximum: 0.5 * (effect.damage[0] + effect.damage[1]),
            falloff,
            cutoff,
            // (as the game pushes the object: all of it, however far from
            // the explosion)
            speed: effect.instantaneous_acceleration * acceleration_scale,
        }
    }

    /// Its damage at `point`.
    fn damage_at(&self, point: Vec3) -> f32 {
        if !self.explosion {
            return 0.0;
        }
        let distance = point.distance(self.epicenter);
        let mut scale = 1.0;
        if self.cutoff > self.falloff {
            scale =
                (1.0 - (distance - self.falloff) / (self.cutoff - self.falloff)).clamp(0.0, 1.0);
        }
        // (no damage where the distance's scale is nothing)
        if scale <= 0.0 {
            return 0.0;
        }
        if !self.scaled {
            scale = 1.0;
        }
        (1.0 - scale) * self.damage_minimum + scale * self.damage_maximum
    }

    /// How hard it tears at `point`: its damage there, less the further it
    /// is from the epicenter.
    fn tearing_at(&self, point: Vec3) -> f32 {
        let closeness = if self.cutoff > f32::EPSILON {
            (1.0 - point.distance(self.epicenter) / self.cutoff).clamp(0.0, 1.0)
        } else {
            1.0
        };
        self.damage_at(point) * closeness
    }
}

/// A body's own share of its species' threshold for a part's joint: a
/// quarter either way, the same for the body each time.
fn break_chance(object: ObjectHandle, part: usize) -> f32 {
    let hash =
        (object.index.wrapping_mul(31).wrapping_add(part as u32)).wrapping_mul(2_654_435_761);
    0.75 + 0.5 * ((hash >> 16) & 0xFFFF) as f32 / 65535.0
}

fn pose_from_matrix(matrix: &NodeMatrix) -> Pose {
    let columns = Mat3::from_cols(
        Vec3::from_array(matrix.forward).normalize_or_zero(),
        Vec3::from_array(matrix.left).normalize_or_zero(),
        Vec3::from_array(matrix.up).normalize_or_zero(),
    );
    Pose {
        position: Vec3::from_array(matrix.position),
        rotation: Quat::from_mat3(&columns).normalize(),
    }
}

fn affine(matrix: &NodeMatrix) -> Affine3A {
    let columns = Mat3::from_cols(
        Vec3::from_array(matrix.forward),
        Vec3::from_array(matrix.left),
        Vec3::from_array(matrix.up),
    ) * matrix.scale;
    Affine3A::from_mat3_translation(columns, Vec3::from_array(matrix.position))
}

fn node_matrix(affine: &Affine3A, scale: f32) -> NodeMatrix {
    let unscale = if scale != 0.0 { 1.0 / scale } else { 1.0 };
    NodeMatrix {
        forward: (Vec3::from(affine.matrix3.x_axis) * unscale).to_array(),
        left: (Vec3::from(affine.matrix3.y_axis) * unscale).to_array(),
        up: (Vec3::from(affine.matrix3.z_axis) * unscale).to_array(),
        position: Vec3::from(affine.translation).to_array(),
        scale,
    }
}

/// The world's ragdolls, and the bodies of the level's bipeds.
#[derive(Clone, Debug)]
pub struct Ragdolls {
    species: BTreeMap<TagRef, Arc<Species>>,
    active: Vec<Ragdoll>,
    corpses: usize,
    dismemberment: bool,
}

impl Default for Ragdolls {
    fn default() -> Self {
        Self {
            species: BTreeMap::new(),
            active: Vec::new(),
            corpses: CORPSES_DEFAULT,
            dismemberment: true,
        }
    }
}

impl Ragdolls {
    /// The bodies of `level`'s bipeds, with no ragdolls yet, and the cap
    /// and the tearing as `self` has them.
    pub fn for_level(&self, level: &Level) -> Ragdolls {
        let species = level
            .objects
            .iter()
            .filter(|object| object.kind == ObjectKind::Biped)
            .filter_map(|object| {
                let model = level
                    .models
                    .get(level.model_index(object.model.as_ref()?)?)?;
                let pieces = object
                    .collision_model
                    .as_ref()
                    .and_then(|tag| level.collision_model(tag))
                    .map_or(&[][..], |collision| &collision.nodes[..]);
                let body = build_body(model, pieces, OVERRIDES)?;
                let links: Vec<AnimationNode> = model
                    .nodes
                    .iter()
                    .map(|node| AnimationNode {
                        name: node.name.clone(),
                        parent: node.parent,
                        first_child: node.first_child,
                        next_sibling: node.next_sibling,
                    })
                    .collect();
                let mut orientations = vec![Orientation::REST; links.len()];
                model_orientations(model, &mut orientations);
                let rest = node_matrices(&links, &orientations, &NodeMatrix::IDENTITY);
                let species = Species {
                    body,
                    links,
                    rest,
                    acceleration_scale: object.acceleration_scale,
                };
                Some((object.tag.clone(), Arc::new(species)))
            })
            .collect();
        Ragdolls {
            species,
            active: Vec::new(),
            corpses: self.corpses,
            dismemberment: self.dismemberment,
        }
    }

    /// The ragdoll body of a biped definition, if it has one.
    pub fn body(&self, tag: &TagRef) -> Option<&RagdollBody> {
        self.species.get(tag).map(|species| &species.body)
    }

    /// Sets the most corpses kept (at least one).
    pub fn set_corpses(&mut self, corpses: usize) {
        self.corpses = corpses.max(1);
    }

    /// Sets whether explosions tear limbs off.
    pub fn set_dismemberment(&mut self, dismemberment: bool) {
        self.dismemberment = dismemberment;
    }

    /// The ragdolls, oldest first.
    pub fn active(&self) -> &[Ragdoll] {
        &self.active
    }

    /// The ragdoll of `object`, if it is one.
    pub fn of(&self, object: ObjectHandle) -> Option<&Ragdoll> {
        self.active.iter().find(|ragdoll| ragdoll.object == object)
    }

    /// Removes every ragdoll from `physics`.
    pub fn clear(&mut self, physics: &mut PhysicsWorld) {
        for ragdoll in self.active.drain(..) {
            physics.remove_ragdoll(ragdoll.id);
        }
    }

    /// One tick: [`Ragdolls::prepare`], the physics stepped by `seconds`
    /// (if any ragdoll there is), then [`Ragdolls::settle`].
    pub fn update(
        &mut self,
        world: &mut World,
        physics: &mut PhysicsWorld,
        killed: &[ObjectHandle],
        detonations: &[Detonation],
        seconds: f32,
    ) {
        if self.prepare(world, physics, killed, detonations) {
            // (a step the physics refuses leaves the ragdolls where they were)
            let _ = physics.step(seconds);
            self.settle(world, physics);
        }
    }

    /// Before the physics steps: a ragdoll for each biped in `killed`, the
    /// push and the tearing of this tick's `detonations`, and the motors.
    /// Whether any ragdoll there is (to step).
    pub fn prepare(
        &mut self,
        world: &mut World,
        physics: &mut PhysicsWorld,
        killed: &[ObjectHandle],
        detonations: &[Detonation],
    ) -> bool {
        for &object in killed {
            self.start(world, physics, object);
        }
        for detonation in detonations {
            self.detonate(world, physics, detonation);
        }
        // (a biped gone takes its ragdoll with it)
        let table = world.resource::<ObjectTable>();
        let (kept, gone): (Vec<_>, Vec<_>) = std::mem::take(&mut self.active)
            .into_iter()
            .partition(|ragdoll| table.entity(ragdoll.object).is_some());
        for ragdoll in gone {
            physics.remove_ragdoll(ragdoll.id);
        }
        self.active = kept;
        for ragdoll in &mut self.active {
            drive(world, physics, ragdoll);
        }
        !self.active.is_empty()
    }

    /// After the physics steps: the nodes and the origins of the bipeds set
    /// where their ragdolls lie.
    pub fn settle(&mut self, world: &mut World, physics: &PhysicsWorld) {
        for ragdoll in &mut self.active {
            let count = ragdoll.species.body.parts.len();
            if let Ok(poses) = physics.ragdoll_poses(ragdoll.id, count) {
                ragdoll.poses = poses;
            }
            ragdoll.age = ragdoll.age.saturating_add(1);
            pose_object(world, ragdoll);
        }
    }

    fn start(&mut self, world: &mut World, physics: &mut PhysicsWorld, object: ObjectHandle) {
        if self.of(object).is_some() {
            return;
        }
        let Some(entity) = world.resource::<ObjectTable>().entity(object) else {
            return;
        };
        let Some(species) = world
            .get::<Object>(entity)
            .and_then(|definition| self.species.get(&definition.tag))
            .cloned()
        else {
            return;
        };
        let transform = world.get::<Transform>(entity).copied().unwrap_or_default();
        // (its nodes as posed, else at the model's default pose where it is)
        let nodes: Vec<NodeMatrix> = match world.get::<NodeMatrices>(entity) {
            Some(nodes) if nodes.current.len() == species.links.len() => nodes.current.clone(),
            _ => {
                let root = root_matrix(&transform);
                species
                    .rest
                    .iter()
                    .map(|node| root.multiply(node))
                    .collect()
            }
        };
        let body = &species.body;
        let poses: Vec<Pose> = body
            .parts
            .iter()
            .map(|part| pose_from_matrix(&nodes[part.node]))
            .collect();
        let parts: Vec<RagdollPart> = body.parts.iter().map(BodyPart::physics).collect();
        if self.active.len() >= self.corpses {
            // (the oldest goes)
            let oldest = self.active.remove(0);
            physics.remove_ragdoll(oldest.id);
        }
        // (its joints at rest as it died: the limits centre on it)
        let Ok(id) = physics.add_ragdoll(&parts, &poses, &poses, MATERIAL) else {
            return;
        };
        let frames: Vec<Affine3A> = body
            .parts
            .iter()
            .zip(&poses)
            .map(|(part, pose)| {
                Affine3A::from_rotation_translation(pose.rotation, pose.position)
                    * Affine3A::from_scale(Vec3::splat(nodes[part.node].scale))
            })
            .collect();
        let from_part = nodes
            .iter()
            .zip(&body.node_parts)
            .map(|(node, &part)| frames[part].inverse() * affine(node))
            .collect();
        self.active.push(Ragdoll {
            object,
            id,
            age: 0,
            limp: false,
            torn: 0,
            poses: poses.clone(),
            from_part,
            scales: nodes.iter().map(|node| node.scale).collect(),
            origin: transform.position - poses[0].position,
            species,
        });
    }

    /// A detonation's damage effects pushing and tearing every ragdoll in
    /// their reach.
    fn detonate(&mut self, world: &World, physics: &mut PhysicsWorld, detonation: &Detonation) {
        let Some(table) = world.get_resource::<DamageTable>() else {
            return;
        };
        let Some(damage) = detonation
            .effect
            .as_ref()
            .and_then(|effect| table.explosions.get(effect))
        else {
            return;
        };
        for effect in damage.iter().filter_map(|effect| table.effects.get(effect)) {
            for ragdoll in &mut self.active {
                let scale = ragdoll.species.acceleration_scale;
                let hit = Hit::from_effect(effect, detonation.point, scale);
                if ragdoll.poses[0].position.distance(hit.epicenter) > hit.cutoff {
                    continue;
                }
                push(physics, ragdoll, &hit);
                if self.dismemberment {
                    tear(physics, ragdoll, &hit);
                }
            }
        }
    }
}

/// A ragdoll's motors: following the biped's pose until it has driven
/// for [`DRIVE_TICKS`], then off.
fn drive(world: &mut World, physics: &mut PhysicsWorld, ragdoll: &mut Ragdoll) {
    if ragdoll.limp {
        return;
    }
    let entity = world.resource::<ObjectTable>().entity(ragdoll.object);
    if ragdoll.age < DRIVE_TICKS {
        // (toward the biped's pose now: its death animation, if one
        // plays; else as it died)
        let animated = entity.and_then(|entity| {
            world.get::<AnimationPlayer>(entity)?;
            world.get::<NodeMatrices>(entity)
        });
        let target: Vec<Pose> = match animated {
            Some(nodes) => ragdoll
                .species
                .body
                .parts
                .iter()
                .zip(&ragdoll.poses)
                .map(|(part, pose)| nodes.current.get(part.node).map_or(*pose, pose_from_matrix))
                .collect(),
            None => ragdoll.poses.clone(),
        };
        let _ = physics.drive_ragdoll(ragdoll.id, &target, DRIVE_FREQUENCY);
        return;
    }
    ragdoll.limp = true;
    let _ = physics.ragdoll_go_limp(ragdoll.id);
    if let Some(entity) = entity {
        world.entity_mut(entity).remove::<AnimationPlayer>();
    }
}

/// Where a part's shape is centred in the world.
fn part_centre(ragdoll: &Ragdoll, part: usize) -> Vec3 {
    let pose = ragdoll.poses[part];
    pose.position + pose.rotation * ragdoll.species.body.parts[part].centre
}

/// A hit's push on a ragdoll: lifted, the ragdoll's mass at the hit's
/// speed, on the part nearest the epicenter, away from it.
fn push(physics: &mut PhysicsWorld, ragdoll: &Ragdoll, hit: &Hit) {
    if hit.speed <= 0.0 {
        return;
    }
    let count = ragdoll.species.body.parts.len();
    let Some(nearest) = (0..count).min_by(|&a, &b| {
        let a = part_centre(ragdoll, a).distance_squared(hit.epicenter);
        let b = part_centre(ragdoll, b).distance_squared(hit.epicenter);
        a.total_cmp(&b)
    }) else {
        return;
    };
    let point = part_centre(ragdoll, nearest);
    let away = (point - hit.epicenter).normalize_or_zero();
    let direction = (away + Vec3::new(0.0, 0.0, PUSH_LIFT)).normalize_or_zero();
    if direction == Vec3::ZERO {
        return;
    }
    let impulse = direction * hit.speed * ragdoll.species.body.mass;
    let _ = physics.push_ragdoll(ragdoll.id, nearest, impulse, point);
}

/// A hit's tearing of a ragdoll's joints: each part whose joint can break
/// and that the hit hurts past its threshold, at most [`BREAKS_PER_HIT`]
/// of them, the worst hurt first; none torn already, or on a limb torn off
/// already.
fn tear(physics: &mut PhysicsWorld, ragdoll: &mut Ragdoll, hit: &Hit) {
    let parts = &ragdoll.species.body.parts;
    for _ in 0..BREAKS_PER_HIT {
        let mut worst: Option<(usize, f32)> = None;
        for (index, part) in parts.iter().enumerate().skip(1) {
            if part.break_damage <= 0.0 {
                continue;
            }
            let mut above = Some(index);
            while let Some(at) = above.filter(|&at| ragdoll.torn & (1 << at) == 0) {
                above = parts[at].parent;
            }
            if above.is_some() {
                continue;
            }
            let share = hit.tearing_at(part_centre(ragdoll, index))
                / (part.break_damage * break_chance(ragdoll.object, index));
            if share >= worst.map_or(1.0, |(_, worst)| worst) {
                worst = Some((index, share));
            }
        }
        let Some((part, _)) = worst else {
            break;
        };
        ragdoll.torn |= 1 << part;
        let _ = physics.break_ragdoll_joint(ragdoll.id, part);
    }
}

/// A ragdoll's biped posed where it lies: each node carried with its part,
/// and its origin with the root part.
fn pose_object(world: &mut World, ragdoll: &Ragdoll) {
    let Some(entity) = world.resource::<ObjectTable>().entity(ragdoll.object) else {
        return;
    };
    let body = &ragdoll.species.body;
    let frames: Vec<Affine3A> = body
        .parts
        .iter()
        .zip(&ragdoll.poses)
        .map(|(part, pose)| {
            Affine3A::from_rotation_translation(pose.rotation, pose.position)
                * Affine3A::from_scale(Vec3::splat(ragdoll.scales[part.node]))
        })
        .collect();
    let current: Vec<NodeMatrix> = ragdoll
        .from_part
        .iter()
        .zip(&body.node_parts)
        .zip(&ragdoll.scales)
        .map(|((from_part, &part), &scale)| node_matrix(&(frames[part] * *from_part), scale))
        .collect();
    let animated = world.get::<AnimationPlayer>(entity).is_some();
    let mut object = world.entity_mut(entity);
    if let Some(mut transform) = object.get_mut::<Transform>() {
        transform.position = ragdoll.poses[0].position + ragdoll.origin;
    }
    match object.get_mut::<NodeMatrices>() {
        // (an animation has moved the last tick's to previous already)
        Some(mut nodes) if animated => nodes.current = current,
        Some(mut nodes) => {
            nodes.previous = std::mem::replace(&mut nodes.current, current);
        }
        None => {
            object.insert(NodeMatrices {
                previous: current.clone(),
                current,
            });
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use opence_content::level::CollisionVertex;
    use opence_content::{CollisionBsp, Group, ModelNode};

    fn tag(path: &str) -> TagRef {
        TagRef {
            group: Group(*b"mod2"),
            path: path.to_string(),
        }
    }

    fn node(name: &str, parent: Option<u16>, translation: [f32; 3]) -> ModelNode {
        ModelNode {
            name: name.to_string(),
            parent,
            first_child: None,
            next_sibling: None,
            translation,
            rotation: [0.0, 0.0, 0.0, 1.0],
            distance_from_parent: Vec3::from_array(translation).length(),
            default_inverse_scale: 1.0,
            default_inverse: [[1.0, 0.0, 0.0], [0.0, 1.0, 0.0], [0.0, 0.0, 1.0], [0.0; 3]],
        }
    }

    /// A pelvis, a spine, a thigh with a calf and a toe.
    fn leg() -> Model {
        Model {
            tag: tag("characters\\leg\\leg"),
            flags: 0,
            node_list_checksum: 0,
            detail_cutoffs: [0.0; 5],
            base_map_scale: [1.0; 2],
            nodes: vec![
                node("bip01 pelvis", None, [0.0, 0.0, 0.5]),
                node("bip01 spine", Some(0), [0.1, 0.0, 0.0]),
                node("bip01 l thigh", Some(0), [0.0, 0.1, 0.0]),
                node("bip01 l calf", Some(2), [0.25, 0.0, 0.0]),
                node("bip01 l toe0", Some(3), [0.25, 0.0, 0.0]),
            ],
            markers: Vec::new(),
            regions: Vec::new(),
            geometries: Vec::new(),
            shaders: Vec::new(),
        }
    }

    fn piece(name: &str, minimum: [f32; 3], maximum: [f32; 3]) -> CollisionNode {
        let vertex = |point| CollisionVertex {
            point,
            first_edge: 0,
        };
        CollisionNode {
            name: name.to_string(),
            region: None,
            parent: None,
            next_sibling: None,
            first_child: None,
            bsps: vec![CollisionBsp {
                vertices: vec![vertex(minimum), vertex(maximum)],
                ..CollisionBsp::default()
            }],
        }
    }

    fn pieces() -> Vec<CollisionNode> {
        vec![
            piece("bip01 pelvis", [-0.1, -0.1, -0.1], [0.1, 0.1, 0.1]),
            piece("bip01 spine", [0.0, -0.1, -0.1], [0.2, 0.1, 0.1]),
            // (long: a capsule)
            piece("bip01 l thigh", [0.0, -0.05, -0.05], [0.25, 0.05, 0.05]),
            piece("bip01 l calf", [0.0, -0.04, -0.04], [0.25, 0.04, 0.04]),
            // (tiny and no limb: folded into the calf's part)
            piece("bip01 l toe0", [0.0, -0.01, -0.01], [0.02, 0.01, 0.01]),
        ]
    }

    fn body(overrides: &str) -> Option<RagdollBody> {
        build_body(&leg(), &pieces(), overrides)
    }

    #[test]
    fn parts_are_the_big_pieces_and_limbs() -> Result<(), Box<dyn std::error::Error>> {
        let body = body("").ok_or("no body")?;
        let nodes: Vec<usize> = body.parts.iter().map(|part| part.node).collect();
        assert_eq!(nodes, [0, 1, 2, 3]);
        assert_eq!(body.node_parts, [0, 1, 2, 3, 3]);
        assert_eq!(body.parts[2].parent, Some(0));
        assert_eq!(body.parts[3].parent, Some(2));
        assert!(matches!(body.parts[2].shape, Shape::Capsule { .. }));
        assert!(matches!(body.parts[0].shape, Shape::Box { .. }));
        let mass: f32 = body.parts.iter().map(|part| part.mass).sum();
        assert!((mass - MASS_DEFAULT).abs() < 0.01, "{mass}");
        // (a calf is a hinge)
        assert!((body.parts[3].swing_plane - 70f32.to_radians()).abs() < 1e-6);
        assert!((body.parts[3].twist[0] + 5f32.to_radians()).abs() < 1e-6);
        Ok(())
    }

    #[test]
    fn the_overrides_change_the_body() -> Result<(), Box<dyn std::error::Error>> {
        let overrides = "\
# a comment
model characters\\other\\other
mass 5
model CHARACTERS\\LEG\\LEG
mass 140   # heavy
fold bip01 spine
part bip01 l toe0
break bip01 l calf: 100
break bip01 pelvis: 100
joint bip01 l thigh: twist -10 30 swing 15 25
sphere bip01 l calf: 0.2
model characters\\next\\next
mass 9
";
        let body = body(overrides).ok_or("no body")?;
        assert_eq!(body.mass, 140.0);
        let nodes: Vec<usize> = body.parts.iter().map(|part| part.node).collect();
        assert_eq!(nodes, [0, 2, 3, 4]);
        assert_eq!(body.node_parts[1], 0);
        let calf = body.parts[2];
        assert_eq!(calf.break_damage, 100.0);
        assert_eq!(calf.shape, Shape::Sphere { radius: 0.2 });
        // (the root has no joint to tear)
        assert_eq!(body.parts[0].break_damage, 0.0);
        let thigh = body.parts[1];
        assert!((thigh.twist[0] + 10f32.to_radians()).abs() < 1e-6);
        assert!((thigh.twist[1] - 30f32.to_radians()).abs() < 1e-6);
        assert!((thigh.swing_plane - 25f32.to_radians()).abs() < 1e-6);
        let mass: f32 = body.parts.iter().map(|part| part.mass).sum();
        assert!(mass >= 140.0 - 0.01, "{mass}");
        Ok(())
    }

    #[test]
    fn bad_overrides_are_ignored() -> Result<(), Box<dyn std::error::Error>> {
        let overrides = "\
model characters\\leg\\leg
mass 0.5
mass12
break bip01 l calf: 0.5
joint bip01 l calf: twist 30 -30 swing 10 10
box bip01 l thigh: 0.1 0.1
fold bip01 pelvis
";
        let plain = body("").ok_or("no body")?;
        assert_eq!(body(overrides).ok_or("no body")?, plain);
        Ok(())
    }

    #[test]
    fn some_models_have_no_body() {
        assert!(body("model characters\\leg\\leg\nnone\n").is_none());
        // (no pieces at all)
        assert!(build_body(&leg(), &[], "").is_none());
        // (out of order)
        let mut model = leg();
        model.nodes[1].parent = Some(3);
        assert!(build_body(&model, &pieces(), "").is_none());
        // (one node)
        model.nodes.truncate(1);
        assert!(build_body(&model, &pieces(), "").is_none());
    }

    #[test]
    fn numbers_and_words_are_read_as_the_game_reads_them() {
        assert_eq!(override_number("  -12.5 rest"), Some((-12.5, " rest")));
        assert_eq!(override_number("+.25"), Some((0.25, "")));
        assert_eq!(override_number("x"), None);
        assert_eq!(override_word("mass 5", "mass"), Some("5"));
        assert_eq!(override_word("massive", "mass"), None);
        assert_eq!(override_word("mass", "mass"), Some(""));
    }

    #[test]
    fn a_blast_tears_harder_closer() {
        let hit = Hit {
            epicenter: Vec3::ZERO,
            explosion: true,
            scaled: true,
            damage_minimum: 0.0,
            damage_maximum: 100.0,
            falloff: 1.0,
            cutoff: 5.0,
            speed: 0.0,
        };
        assert_eq!(hit.damage_at(Vec3::new(0.5, 0.0, 0.0)), 100.0);
        assert_eq!(hit.damage_at(Vec3::new(6.0, 0.0, 0.0)), 0.0);
        assert!(
            hit.tearing_at(Vec3::new(1.0, 0.0, 0.0)) > hit.tearing_at(Vec3::new(3.0, 0.0, 0.0))
        );
        let chance = break_chance(ObjectHandle { index: 7, salt: 1 }, 3);
        assert!((0.75..=1.25).contains(&chance));
    }
}
