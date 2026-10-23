//! Animation, as the game does it (`model_animations.c`): an animation's
//! frame read into each node's orientation, replacement and overlay
//! animations over it, blending from one pose to another, the nodes'
//! matrices from their orientations, and playing an animation frame by
//! frame: each object's, by [`AnimationPlayer`], into its [`NodeMatrices`].
//!
//! The arithmetic is the game's, in its order, on the game's own
//! matrix: rows forward, left, up and position, and a scale, so that a
//! point `p` is `scale * (p.x * forward + p.y * left + p.z * up) +
//! position`. Every offset and count is a map's, and is checked: a bad
//! animation is the rest pose, never a read past its data.

use std::sync::Arc;

use bevy_ecs::prelude::*;
use glam::Vec3;
use opence_content::{
    Animation, AnimationGraph, AnimationKind, AnimationNode, Model, ScreenBounds,
};

use crate::device_animation::{apply_device_overlays, DevicePose};
use crate::objects::ObjectTable;
use crate::random::RandomSeed;
use crate::seats::VehicleMotions;
use crate::vehicle_animation::{apply_vehicle_overlays, VehicleMotion};
use crate::{ObjectId, Transform};

/// The most nodes an animation moves, or a model has.
pub const MAX_NODES: usize = 64;

const QUATERNION_MAXIMUM: f32 = 32767.0;
const KEYFRAME_COUNT_BITS: u32 = 12;
/// Where the rotations' node headers start in compressed data, after the
/// header's eleven offsets.
const ROTATION_NODE_HEADERS: i64 = 0x2C;

/// A quaternion, `[i, j, k, w]`.
pub type Quaternion = [f32; 4];

/// The identity quaternion.
pub const IDENTITY: Quaternion = [0.0, 0.0, 0.0, 1.0];

/// A node's rotation, translation and scale, from its parent.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Orientation {
    /// Its rotation.
    pub rotation: Quaternion,
    /// Its translation.
    pub translation: [f32; 3],
    /// Its scale.
    pub scale: f32,
}

impl Orientation {
    /// No rotation, at the origin, at scale 1.
    pub const REST: Self = Self {
        rotation: IDENTITY,
        translation: [0.0; 3],
        scale: 1.0,
    };
}

/// The game's matrix (`real_matrix4x3`).
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct NodeMatrix {
    /// Where its x goes.
    pub forward: [f32; 3],
    /// Where its y goes.
    pub left: [f32; 3],
    /// Where its z goes.
    pub up: [f32; 3],
    /// Where its origin goes.
    pub position: [f32; 3],
    /// How much it scales.
    pub scale: f32,
}

impl NodeMatrix {
    /// The identity.
    pub const IDENTITY: Self = Self {
        forward: [1.0, 0.0, 0.0],
        left: [0.0, 1.0, 0.0],
        up: [0.0, 0.0, 1.0],
        position: [0.0; 3],
        scale: 1.0,
    };

    /// A matrix at `point`, facing `forward` with `up` up
    /// (`matrix4x3_from_point_and_vectors`).
    pub fn from_point_and_vectors(point: [f32; 3], forward: [f32; 3], up: [f32; 3]) -> Self {
        Self {
            forward,
            left: cross(up, forward),
            up,
            position: point,
            scale: 1.0,
        }
    }

    /// A node's matrix from its orientation (`matrix4x3_from_orientation`).
    pub fn from_orientation(orientation: &Orientation) -> Self {
        let [qi, qj, qk, qw] = orientation.rotation;
        let length = qi * qi + qj * qj + qk * qk + qw * qw;
        let scale = if length != 0.0 { 2.0 / length } else { 0.0 };
        let x = scale * qi;
        let y = scale * qj;
        let z = scale * qk;
        let wx = qw * x;
        let wy = qw * y;
        let wz = qw * z;
        let xx = qi * x;
        let xy = qi * y;
        let xz = qi * z;
        let yy = qj * y;
        let yz = qj * z;
        let zz = qk * z;
        Self {
            forward: [1.0 - (yy + zz), xy - wz, xz + wy],
            left: [xy + wz, 1.0 - (xx + zz), yz - wx],
            up: [xz - wy, yz + wx, 1.0 - (xx + yy)],
            position: orientation.translation,
            scale: orientation.scale,
        }
    }

    /// This matrix after `b`: `b`'s rows in this one's frame
    /// (`matrix4x3_multiply(self, b)`).
    pub fn multiply(&self, b: &Self) -> Self {
        let rows = [self.forward, self.left, self.up];
        let row = |of: [f32; 3]| -> [f32; 3] {
            let mut result = [0.0; 3];
            for (column, value) in result.iter_mut().enumerate() {
                *value =
                    of[0] * rows[0][column] + of[1] * rows[1][column] + of[2] * rows[2][column];
            }
            result
        };
        let mut position = row(b.position);
        for (column, value) in position.iter_mut().enumerate() {
            *value = *value * self.scale + self.position[column];
        }
        Self {
            forward: row(b.forward),
            left: row(b.left),
            up: row(b.up),
            position,
            scale: self.scale * b.scale,
        }
    }

    /// Where it takes `point`.
    pub fn transform_point(&self, point: [f32; 3]) -> [f32; 3] {
        let mut result = [0.0; 3];
        for (column, value) in result.iter_mut().enumerate() {
            *value = self.scale
                * (point[0] * self.forward[column]
                    + point[1] * self.left[column]
                    + point[2] * self.up[column])
                + self.position[column];
        }
        result
    }

    /// Where it turns `vector`, without its scale or position
    /// (`matrix4x3_transform_normal`).
    pub fn transform_vector(&self, vector: [f32; 3]) -> [f32; 3] {
        let mut result = [0.0; 3];
        for (column, value) in result.iter_mut().enumerate() {
            *value = vector[0] * self.forward[column]
                + vector[1] * self.left[column]
                + vector[2] * self.up[column];
        }
        result
    }
}

fn cross(a: [f32; 3], b: [f32; 3]) -> [f32; 3] {
    [
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    ]
}

/// A quaternion at length 1, or the identity if it has none
/// (`quaternion_normalize`).
pub fn quaternion_normalize(q: Quaternion) -> Quaternion {
    let length = q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3];
    if length > 0.0 {
        let scale = 1.0 / length.sqrt();
        [scale * q[0], scale * q[1], scale * q[2], scale * q[3]]
    } else {
        IDENTITY
    }
}

/// From `q0` to `q1` by `t`, the short way round, at length 1
/// (`quaternions_interpolate_and_normalize`).
pub fn quaternions_interpolate(q0: Quaternion, q1: Quaternion, t: f32) -> Quaternion {
    quaternion_normalize(quaternions_blend(q0, q1, t))
}

/// From `q0` to `q1` by `t`, the short way round, at whatever length the
/// blend gives (`quaternions_interpolate`).
pub fn quaternions_blend(q0: Quaternion, q1: Quaternion, t: f32) -> Quaternion {
    let v = 1.0 - t;
    let dot = q0[0] * q1[0] + q0[1] * q1[1] + q0[2] * q1[2] + q0[3] * q1[3];
    let t = if dot < 0.0 { -t } else { t };
    [
        q0[0] * v + q1[0] * t,
        q0[1] * v + q1[1] * t,
        q0[2] * v + q1[2] * t,
        q0[3] * v + q1[3] * t,
    ]
}

/// `q0` after `q1` (`quaternions_multiply`).
pub fn quaternions_multiply(q0: Quaternion, q1: Quaternion) -> Quaternion {
    let [i0, j0, k0, w0] = q0;
    let [i1, j1, k1, w1] = q1;
    [
        j0 * k1 + i0 * w1 + w0 * i1 - k0 * j1,
        w0 * j1 + j0 * w1 + k0 * i1 - i0 * k1,
        k0 * w1 + i0 * j1 + w0 * k1 - j0 * i1,
        w0 * w1 - i0 * i1 - j0 * j1 - k0 * k1,
    ]
}

fn bytes_at<const N: usize>(bytes: &[u8], at: usize) -> Option<[u8; N]> {
    let slice = bytes.get(at..at.checked_add(N)?)?;
    let mut word = [0; N];
    word.copy_from_slice(slice);
    Some(word)
}

fn u16_at(bytes: &[u8], at: usize) -> Option<u16> {
    bytes_at(bytes, at).map(u16::from_le_bytes)
}

fn u32_at(bytes: &[u8], at: usize) -> Option<u32> {
    bytes_at(bytes, at).map(u32::from_le_bytes)
}

fn f32_at(bytes: &[u8], at: usize) -> Option<f32> {
    bytes_at(bytes, at).map(f32::from_le_bytes)
}

fn point_at(bytes: &[u8], at: usize) -> Option<[f32; 3]> {
    Some([
        f32_at(bytes, at)?,
        f32_at(bytes, at.checked_add(4)?)?,
        f32_at(bytes, at.checked_add(8)?)?,
    ])
}

/// An uncompressed rotation: four shorts (`quaternion_decompress_8byte`).
fn quaternion_8byte(bytes: &[u8], at: usize) -> Option<Quaternion> {
    let scale = 1.0 / QUATERNION_MAXIMUM;
    let mut q = [0.0; 4];
    for (index, value) in q.iter_mut().enumerate() {
        let word = u16_at(bytes, at.checked_add(index * 2)?)? as i16;
        *value = f32::from(word) * scale;
    }
    Some(q)
}

/// A compressed rotation: four 12-bit components in three words
/// (`quaternion_decompress_6byte`), not normalized.
fn quaternion_6byte(bytes: &[u8], at: usize) -> Option<Quaternion> {
    let w0 = u32::from(u16_at(bytes, at)?);
    let w1 = u32::from(u16_at(bytes, at.checked_add(2)?)?);
    let w2 = u32::from(u16_at(bytes, at.checked_add(4)?)?);
    let i = ((w0 >> 12) | (w0 & 0xFFF0)) as u16 as i16;
    let j = (((w1 >> 4) & 0x0FF0) | (w0 & 0x000F) | (w0 << 12)) as u16 as i16;
    let k = (((((w2 >> 4) & 0x0F00) | (w1 & 0x00F0)) >> 4) | (w1 << 8)) as u16 as i16;
    let w = (((w2 >> 8) & 0x000F) | (w2 << 4)) as u16 as i16;
    let scale = 1.0 / QUATERNION_MAXIMUM;
    Some([
        f32::from(i) * scale,
        f32::from(j) * scale,
        f32::from(k) * scale,
        f32::from(w) * scale,
    ])
}

/// Whether `count` elements of `size` bytes from element `first` on,
/// `relative` bytes past `offset`, are inside `length` bytes
/// (`animation_data_contains`).
fn contains(length: usize, offset: i64, relative: i64, first: i64, count: i64, size: i64) -> bool {
    let mut length = length as i64;
    if offset < 0 || offset > length {
        return false;
    }
    length -= offset;
    if relative < 0 || relative > length {
        return false;
    }
    length -= relative;
    if first < 0 || count < 0 || size < 0 || first > i64::from(i16::MAX) || count > 0xFFFF {
        return false;
    }
    size == 0 || first + count <= length / size
}

fn node_flag(flags: [u32; 2], node: usize) -> bool {
    flags
        .get(node / 32)
        .is_some_and(|&word| word >> (node % 32) & 1 != 0)
}

fn flag_count(flags: [u32; 2], node_count: usize) -> i64 {
    (0..node_count)
        .filter(|&node| node_flag(flags, node))
        .count() as i64
}

/// Whether the frame an animation reads is inside its data
/// (`animation_frame_valid`).
fn frame_valid(animation: &Animation, frame: i16, reads_default_data: bool) -> bool {
    let node_count = usize::from(animation.node_count);
    if node_count > MAX_NODES {
        return false;
    }
    let length = animation.data.len();
    if animation.is_compressed() {
        return contains(
            length,
            i64::from(animation.compressed_data_offset),
            0,
            0,
            1,
            ROTATION_NODE_HEADERS,
        );
    }
    let rotations = flag_count(animation.rotation_flags, node_count);
    let translations = flag_count(animation.translation_flags, node_count);
    let scales = flag_count(animation.scale_flags, node_count);
    let frame_bytes = rotations * 8 + translations * 12 + scales * 4;
    let nodes = node_count as i64;
    let default_bytes =
        (nodes - rotations) * 8 + (nodes - translations) * 12 + (nodes - scales) * 4;
    let frame_offset = i64::from(frame) * i64::from(animation.frame_size);
    contains(length, frame_offset, 0, 0, 1, frame_bytes)
        && (!reads_default_data || default_bytes <= animation.default_data.len() as i64)
}

/// One channel of compressed data: where its parts are, and how one of
/// its values is read and blended.
struct Channel<T> {
    node_headers: i64,
    frame_indices: i64,
    defaults: i64,
    keyframes: i64,
    size: usize,
    read: fn(&[u8], usize) -> Option<T>,
    /// A value as it is used (a rotation at length 1).
    finish: fn(T) -> T,
    blend: fn(T, T, f32) -> T,
}

/// An animation's compressed data, from its header.
struct Compressed<'a> {
    data: &'a [u8],
    base: i64,
    header: [i64; 11],
}

impl<'a> Compressed<'a> {
    fn new(animation: &'a Animation) -> Option<Self> {
        let data = animation.data.as_slice();
        let base = i64::from(animation.compressed_data_offset);
        if !contains(data.len(), base, 0, 0, 1, ROTATION_NODE_HEADERS) {
            return None;
        }
        let mut header = [0; 11];
        for (index, value) in header.iter_mut().enumerate() {
            *value = i64::from(u32_at(data, base as usize + index * 4)? as i32);
        }
        Some(Self { data, base, header })
    }

    fn rotations(&self) -> Channel<Quaternion> {
        Channel {
            node_headers: ROTATION_NODE_HEADERS,
            frame_indices: self.header[0],
            defaults: self.header[1],
            keyframes: self.header[2],
            size: 6,
            read: quaternion_6byte,
            finish: quaternion_normalize,
            blend: quaternions_interpolate,
        }
    }

    fn translations(&self) -> Channel<[f32; 3]> {
        Channel {
            node_headers: self.header[3],
            frame_indices: self.header[4],
            defaults: self.header[5],
            keyframes: self.header[6],
            size: 12,
            read: point_at,
            finish: |point| point,
            blend: |a, b, t| {
                let s = 1.0 - t;
                [
                    s * a[0] + t * b[0],
                    s * a[1] + t * b[1],
                    s * a[2] + t * b[2],
                ]
            },
        }
    }

    fn scales(&self) -> Channel<f32> {
        Channel {
            node_headers: self.header[7],
            frame_indices: self.header[8],
            defaults: self.header[9],
            keyframes: self.header[10],
            size: 4,
            read: f32_at,
            finish: |scale| scale,
            blend: |a, b, t| (1.0 - t) * a + b * t,
        }
    }

    fn has(&self, relative: i64, first: i64, count: i64, size: usize) -> bool {
        contains(
            self.data.len(),
            self.base,
            relative,
            first,
            count,
            size as i64,
        )
    }

    fn at(&self, relative: i64, index: i64, size: usize) -> usize {
        // (inside the data, which `has` checked first)
        (self.base + relative + index * size as i64) as usize
    }

    /// A node's value at `real_frame` (`animation_get_keyframe_rotation`,
    /// and its translation and scale): its keyframes, or its default before
    /// the first and when it has none. `None` if any of it is past the data.
    fn value<T: Copy>(
        &self,
        channel: &Channel<T>,
        real_frame: f32,
        adjusted_node: usize,
        default_node: usize,
    ) -> Option<T> {
        let (adjusted, default_index) = (adjusted_node as i64, default_node as i64);
        if !self.has(channel.node_headers, adjusted, 1, 4)
            || !self.has(channel.defaults, default_index, 1, channel.size)
        {
            return None;
        }
        let read = |relative: i64, index: i64| -> Option<T> {
            (channel.read)(self.data, self.at(relative, index, channel.size))
        };
        let default = read(channel.defaults, default_index)?;
        let node_header = u32_at(self.data, self.at(channel.node_headers, adjusted, 4))?;
        let first = i64::from((node_header >> KEYFRAME_COUNT_BITS) as u16 as i16);
        let count = i64::from((node_header & ((1 << KEYFRAME_COUNT_BITS) - 1)) as i16);
        if count == 0 {
            return Some((channel.finish)(default));
        }
        if !self.has(channel.keyframes, first, count, channel.size)
            || !self.has(channel.frame_indices, first, count, 2)
        {
            return None;
        }
        let keyframe = |index: i64| read(channel.keyframes, first + index);
        let frame_of = |index: i64| -> Option<i32> {
            let at = self.at(channel.frame_indices, first + index, 2);
            u16_at(self.data, at).map(i32::from)
        };
        let target = i32::from(real_frame.floor() as i16);
        let last = frame_of(count - 1)?;
        if target > last {
            return Some((channel.finish)(keyframe(count - 1)?));
        }
        let (this_frame, this, next_frame, next) = if target < frame_of(0)? {
            (0, default, frame_of(0)?, keyframe(0)?)
        } else if target == last {
            (last, keyframe(count - 1)?, last + 1, default)
        } else {
            let index = keyframe_search(&frame_of, count, target)?;
            (
                frame_of(index)?,
                keyframe(index)?,
                frame_of(index + 1)?,
                keyframe(index + 1)?,
            )
        };
        // (the frames are the game's shorts)
        let (this_frame, next_frame) = (i32::from(this_frame as i16), i32::from(next_frame as i16));
        if real_frame == this_frame as f32 {
            Some((channel.finish)(this))
        } else {
            let fraction = (real_frame - this_frame as f32) / (next_frame - this_frame) as f32;
            Some((channel.blend)(this, next, fraction))
        }
    }
}

/// The keyframe whose frames hold `target`, with one after it
/// (`animation_keyframe_search`).
fn keyframe_search(frame_of: &dyn Fn(i64) -> Option<i32>, count: i64, target: i32) -> Option<i64> {
    let mut low = 0;
    let mut high = count - 1;
    let mut index;
    let mut steps = 0;
    loop {
        index = (low + high) >> 1;
        if index + 1 < count && frame_of(index + 1)? <= target {
            low = index;
        } else if frame_of(index)? > target {
            high = index;
        } else {
            break;
        }
        // (a map's frame indices out of order would loop forever)
        steps += 1;
        if steps >= 200 {
            break;
        }
    }
    Some(index.min(count - 2).max(0))
}

/// What an animation's frame does to the nodes it moves.
#[derive(Clone, Copy, PartialEq, Eq)]
enum Apply {
    /// Sets them, and the others to its defaults.
    Base,
    /// Sets the channels it has.
    Replace,
    /// Adds to the channels it has.
    Overlay,
}

/// Reads `animation`'s frame into the nodes, as `apply` says.
fn read_frame(animation: &Animation, frame: i16, nodes: &mut [Orientation], apply: Apply) {
    let compressed = if animation.is_compressed() {
        match Compressed::new(animation) {
            Some(compressed) => Some(compressed),
            None => return,
        }
    } else {
        None
    };
    let channels = compressed
        .as_ref()
        .map(|data| (data.rotations(), data.translations(), data.scales()));
    let data = animation.data.as_slice();
    let mut cursor = (frame.max(0) as usize).saturating_mul(usize::from(animation.frame_size));
    let mut default_cursor = 0;
    let (mut rotation_index, mut translation_index, mut scale_index) = (0, 0, 0);
    let node_count = usize::from(animation.node_count).min(MAX_NODES);
    for (node, orientation) in nodes.iter_mut().enumerate().take(node_count) {
        // each channel: a frame's value, or none (the node's default, from
        // the default data or the compressed data's)
        let mut next = |size: usize| {
            let at = cursor;
            cursor += size;
            at
        };
        let mut next_default = |size: usize| {
            let at = default_cursor;
            default_cursor += size;
            at
        };

        let rotation = if node_flag(animation.rotation_flags, node) {
            let value = match (&compressed, &channels) {
                (Some(data), Some((rotations, _, _))) => {
                    rotation_index += 1;
                    data.value(rotations, f32::from(frame), rotation_index - 1, node)
                        .unwrap_or(IDENTITY)
                }
                _ => quaternion_8byte(data, next(8)).unwrap_or(IDENTITY),
            };
            Some(value)
        } else if apply != Apply::Base {
            None
        } else if let (Some(data), Some((rotations, _, _))) = (&compressed, &channels) {
            Some(if data.has(rotations.defaults, node as i64, 1, 6) {
                quaternion_6byte(data.data, data.at(rotations.defaults, node as i64, 6))
                    .map_or(IDENTITY, quaternion_normalize)
            } else {
                IDENTITY
            })
        } else {
            Some(quaternion_8byte(&animation.default_data, next_default(8)).unwrap_or(IDENTITY))
        };

        let translation = if node_flag(animation.translation_flags, node) {
            let value = match (&compressed, &channels) {
                (Some(data), Some((_, translations, _))) => {
                    translation_index += 1;
                    data.value(translations, f32::from(frame), translation_index - 1, node)
                        .unwrap_or([0.0; 3])
                }
                _ => point_at(data, next(12)).unwrap_or([0.0; 3]),
            };
            Some(value)
        } else if apply != Apply::Base {
            None
        } else if let (Some(data), Some((_, translations, _))) = (&compressed, &channels) {
            Some(if data.has(translations.defaults, node as i64, 1, 12) {
                point_at(data.data, data.at(translations.defaults, node as i64, 12))
                    .unwrap_or([0.0; 3])
            } else {
                [0.0; 3]
            })
        } else {
            Some(point_at(&animation.default_data, next_default(12)).unwrap_or([0.0; 3]))
        };

        let scale = if node_flag(animation.scale_flags, node) {
            let value = match (&compressed, &channels) {
                (Some(data), Some((_, _, scales))) => {
                    scale_index += 1;
                    // (a scale's default is by its adjusted index)
                    data.value(scales, f32::from(frame), scale_index - 1, scale_index - 1)
                        .unwrap_or(1.0)
                }
                _ => f32_at(data, next(4)).unwrap_or(1.0),
            };
            Some(value)
        } else if apply != Apply::Base {
            None
        } else if compressed.is_some() {
            Some(1.0)
        } else {
            Some(f32_at(&animation.default_data, next_default(4)).unwrap_or(1.0))
        };

        match apply {
            Apply::Base | Apply::Replace => {
                if let Some(rotation) = rotation {
                    orientation.rotation = rotation;
                }
                if let Some(translation) = translation {
                    orientation.translation = translation;
                }
                if let Some(scale) = scale {
                    orientation.scale = scale;
                }
            }
            Apply::Overlay => {
                if let Some(rotation) = rotation {
                    orientation.rotation = quaternions_multiply(rotation, orientation.rotation);
                }
                if let Some([x, y, z]) = translation {
                    orientation.translation[0] += x;
                    orientation.translation[1] += y;
                    orientation.translation[2] += z;
                }
                if let Some(scale) = scale {
                    orientation.scale *= scale;
                }
            }
        }
    }
}

/// A model's nodes at their default pose (`model_get_node_orientations`).
pub fn model_orientations(model: &Model, nodes: &mut [Orientation]) {
    for (orientation, node) in nodes.iter_mut().zip(model.nodes.iter().take(MAX_NODES)) {
        *orientation = Orientation {
            rotation: node.rotation,
            translation: node.translation,
            scale: 1.0,
        };
    }
}

/// The nodes at a base animation's frame (`animation_get_node_orientations`):
/// the animation's, if it was made for `model`'s nodes and its frame is
/// inside its data; otherwise the model's default pose, or with no model
/// the rest pose.
pub fn base_orientations(
    model: Option<&Model>,
    animation: &Animation,
    frame: i16,
    nodes: &mut [Orientation],
) {
    let made_for = |model: &Model| {
        (animation.node_list_checksum == 0
            || animation.node_list_checksum == model.node_list_checksum
            || model.node_list_checksum == 0)
            && model.nodes.len() == usize::from(animation.node_count)
    };
    if animation.kind == AnimationKind::Base
        && model.is_none_or(made_for)
        && frame_valid(animation, frame, true)
    {
        read_frame(animation, frame, nodes, Apply::Base);
    } else if let Some(model) = model {
        model_orientations(model, nodes);
    } else {
        let count = usize::from(animation.node_count).min(MAX_NODES);
        for orientation in nodes.iter_mut().take(count) {
            *orientation = Orientation::REST;
        }
    }
}

/// A replacement animation's frame over the nodes: the channels it has
/// (`replacement_animation_apply`).
pub fn apply_replacement(animation: &Animation, frame: i16, nodes: &mut [Orientation]) {
    if animation.kind == AnimationKind::Replacement
        && frame >= 0
        && frame < animation.frame_count as i16
        && frame_valid(animation, frame, false)
    {
        read_frame(animation, frame, nodes, Apply::Replace);
    }
}

/// An overlay animation's frame over the nodes: its rotations before
/// theirs, its translations added, its scales multiplied
/// (`overlay_animation_apply`).
pub fn apply_overlay(animation: &Animation, frame: i16, nodes: &mut [Orientation]) {
    if animation.kind == AnimationKind::Overlay
        && frame >= 0
        && frame < animation.frame_count as i16
        && frame_valid(animation, frame, false)
    {
        read_frame(animation, frame, nodes, Apply::Overlay);
    }
}

/// One node's values in a frame: the channels the animation moves it in.
#[derive(Clone, Copy, Debug, Default)]
struct NodeValues {
    rotation: Option<Quaternion>,
    translation: Option<[f32; 3]>,
    scale: Option<f32>,
}

/// The values of the nodes `animation` moves in a frame, by node: an
/// uncompressed animation's from `frame`'s data, a compressed one's from
/// its keyframes, its rotations at `rotation_frame` and the rest at
/// `real_frame`. Without `scales`, none are read, and an uncompressed
/// frame's are not stepped over (as `aiming_screen_apply` reads it).
/// `None` for compressed data without its header.
fn frame_values(
    animation: &Animation,
    frame: i16,
    rotation_frame: f32,
    real_frame: f32,
    scales: bool,
) -> Option<[NodeValues; MAX_NODES]> {
    let compressed = if animation.is_compressed() {
        Some(Compressed::new(animation)?)
    } else {
        None
    };
    let data = animation.data.as_slice();
    let mut cursor = (frame.max(0) as usize).saturating_mul(usize::from(animation.frame_size));
    let mut next = |size: usize| {
        let at = cursor;
        cursor += size;
        at
    };
    let (mut rotation_index, mut translation_index, mut scale_index) = (0, 0, 0);
    let mut values = [NodeValues::default(); MAX_NODES];
    let node_count = usize::from(animation.node_count).min(MAX_NODES);
    for (node, value) in values.iter_mut().enumerate().take(node_count) {
        if node_flag(animation.rotation_flags, node) {
            value.rotation = Some(match &compressed {
                Some(compressed) => {
                    rotation_index += 1;
                    let channel = compressed.rotations();
                    compressed
                        .value(&channel, rotation_frame, rotation_index - 1, node)
                        .unwrap_or(IDENTITY)
                }
                None => quaternion_8byte(data, next(8)).unwrap_or(IDENTITY),
            });
        }
        if node_flag(animation.translation_flags, node) {
            value.translation = Some(match &compressed {
                Some(compressed) => {
                    translation_index += 1;
                    let channel = compressed.translations();
                    compressed
                        .value(&channel, real_frame, translation_index - 1, node)
                        .unwrap_or([0.0; 3])
                }
                None => point_at(data, next(12)).unwrap_or([0.0; 3]),
            });
        }
        if scales && node_flag(animation.scale_flags, node) {
            value.scale = Some(match &compressed {
                Some(compressed) => {
                    scale_index += 1;
                    let channel = compressed.scales();
                    compressed
                        .value(&channel, real_frame, scale_index - 1, scale_index - 1)
                        .unwrap_or(1.0)
                }
                None => f32_at(data, next(4)).unwrap_or(1.0),
            });
        }
    }
    Some(values)
}

/// Adds a node's overlay values to it: the rotation before its own, the
/// translation added, the scale multiplied.
fn add_overlay(orientation: &mut Orientation, value: &NodeValues) {
    if let Some(rotation) = value.rotation {
        orientation.rotation = quaternions_multiply(rotation, orientation.rotation);
    }
    if let Some([x, y, z]) = value.translation {
        orientation.translation[0] += x;
        orientation.translation[1] += y;
        orientation.translation[2] += z;
    }
    if let Some(scale) = value.scale {
        orientation.scale *= scale;
    }
}

/// An overlay animation's frame over the nodes, by `scale` from none (0)
/// to all of it (1) (`overlay_animation_apply_scaled`).
pub fn apply_overlay_scaled(
    animation: &Animation,
    frame: i16,
    scale: f32,
    nodes: &mut [Orientation],
) {
    if animation.kind != AnimationKind::Overlay
        || frame < 0
        || frame >= animation.frame_count as i16
        || !frame_valid(animation, frame, false)
    {
        return;
    }
    let Some(values) = frame_values(animation, frame, f32::from(frame), f32::from(frame), true)
    else {
        return;
    };
    let inverse = 1.0 - scale;
    for (orientation, value) in nodes.iter_mut().zip(&values) {
        if let Some(rotation) = value.rotation {
            let rotation = quaternions_blend(IDENTITY, rotation, scale);
            orientation.rotation = quaternions_multiply(rotation, orientation.rotation);
        }
        if let Some([x, y, z]) = value.translation {
            orientation.translation[0] += x * scale;
            orientation.translation[1] += y * scale;
            orientation.translation[2] += z * scale;
        }
        if let Some(value) = value.scale {
            orientation.scale *= value * scale + inverse;
        }
    }
}

/// An overlay animation over the nodes between two of its frames, at
/// `real_frame` (`overlay_animation_apply_continuous`): past the last frame
/// it is the last, which blends toward the first. A compressed animation's
/// rotations are its frame's, without the fraction, as the game reads them.
pub fn apply_overlay_continuous(animation: &Animation, real_frame: f32, nodes: &mut [Orientation]) {
    let Some(values) = continuous_values(animation, real_frame) else {
        return;
    };
    for (orientation, value) in nodes.iter_mut().zip(&values) {
        add_overlay(orientation, value);
    }
}

/// An overlay animation between two of its frames, by `scale` from none (0)
/// to all of it (1) (`overlay_animation_apply_continuous_scaled`): as
/// [`apply_overlay_continuous`], each rotation blended from none.
pub fn apply_overlay_continuous_scaled(
    animation: &Animation,
    real_frame: f32,
    scale: f32,
    nodes: &mut [Orientation],
) {
    let Some(values) = continuous_values(animation, real_frame) else {
        return;
    };
    let inverse = 1.0 - scale;
    for (orientation, value) in nodes.iter_mut().zip(&values) {
        if let Some(rotation) = value.rotation {
            let rotation = quaternions_interpolate(IDENTITY, rotation, scale);
            orientation.rotation = quaternions_multiply(rotation, orientation.rotation);
        }
        if let Some([x, y, z]) = value.translation {
            orientation.translation[0] += x * scale;
            orientation.translation[1] += y * scale;
            orientation.translation[2] += z * scale;
        }
        if let Some(value) = value.scale {
            orientation.scale *= value * scale + inverse;
        }
    }
}

/// An overlay's values between two of its frames, at `real_frame`: past the
/// last frame they are the last's, which blends toward the first; a
/// compressed animation's rotations are its frame's, without the fraction,
/// as the game reads them.
fn continuous_values(animation: &Animation, real_frame: f32) -> Option<[NodeValues; MAX_NODES]> {
    let frame_count = animation.frame_count as i16;
    if frame_count <= 0 {
        return None;
    }
    let mut real_frame = real_frame;
    let mut fraction = real_frame % 1.0;
    let mut frame = real_frame.abs().floor() as i16;
    if frame >= frame_count {
        frame = frame_count - 1;
        fraction = 1.0;
        real_frame = f32::from(frame);
    }
    let next_frame = if frame == frame_count - 1 {
        0
    } else {
        frame + 1
    };
    if animation.kind != AnimationKind::Overlay
        || !frame_valid(animation, frame, false)
        || !frame_valid(animation, next_frame, false)
    {
        return None;
    }
    let values = frame_values(animation, frame, f32::from(frame), real_frame, true)?;
    if animation.is_compressed() {
        return Some(values);
    }
    let next_values = frame_values(
        animation,
        next_frame,
        f32::from(next_frame),
        f32::from(next_frame),
        true,
    )?;
    let inverse = 1.0 - fraction;
    let mut blended = values;
    for (value, next) in blended.iter_mut().zip(&next_values) {
        *value = NodeValues {
            rotation: value
                .rotation
                .zip(next.rotation)
                .map(|(a, b)| quaternions_interpolate(a, b, fraction)),
            translation: value.translation.zip(next.translation).map(|(a, b)| {
                [
                    inverse * a[0] + fraction * b[0],
                    inverse * a[1] + fraction * b[1],
                    inverse * a[2] + fraction * b[2],
                ]
            }),
            scale: value
                .scale
                .zip(next.scale)
                .map(|(a, b)| (1.0 - fraction) * a + b * fraction),
        };
    }
    Some(blended)
}

/// Which frames of one side of an aiming screen hold `angle`, and how far
/// it is from the first to the next: the cell's index from the screen's
/// negative edge, clamped to the screen.
fn screen_cell(
    angle: f32,
    negative_delta: f32,
    positive_delta: f32,
    negative_frames: i16,
    positive_frames: i16,
) -> (i32, f32) {
    let delta = if angle < 0.0 {
        negative_delta
    } else {
        positive_delta
    };
    let frame = if delta == 0.0 { 0.0 } else { angle / delta };
    let mut index = i32::from(frame as i16);
    let mut fraction = frame % 1.0;
    if fraction < 0.0 {
        fraction += 1.0;
        index -= 1;
    }
    if index >= i32::from(positive_frames) {
        index = i32::from(positive_frames) - 1;
        fraction = 1.0;
    }
    if index < -i32::from(negative_frames) {
        index = -i32::from(negative_frames);
        fraction = 0.0;
    }
    (
        i32::from((index + i32::from(negative_frames)) as i16),
        fraction,
    )
}

/// An aiming (or looking) screen over the nodes: an overlay whose frames
/// are a grid of yaws across and pitches down, blended between the four
/// around `yaw` and `pitch` (radians from straight ahead), which its
/// `bounds` clamp to its edges (`aiming_screen_apply`). Its scales are not
/// read, and an uncompressed frame's are not stepped over, as the game
/// reads them.
pub fn apply_aiming_screen(
    animation: &Animation,
    bounds: &ScreenBounds,
    yaw: f32,
    pitch: f32,
    nodes: &mut [Orientation],
) {
    let grid_width = i32::from(
        (i32::from(bounds.negative_yaw_frames) + i32::from(bounds.positive_yaw_frames) + 1) as i16,
    );
    let grid_height = i32::from(
        (i32::from(bounds.negative_pitch_frames) + i32::from(bounds.positive_pitch_frames) + 1)
            as i16,
    );
    if animation.kind != AnimationKind::Overlay
        || i32::from(animation.frame_count as i16) < grid_width * grid_height
    {
        return;
    }
    let (yaw_index, yaw_fraction) = screen_cell(
        yaw,
        bounds.negative_yaw,
        bounds.positive_yaw,
        bounds.negative_yaw_frames,
        bounds.positive_yaw_frames,
    );
    let (pitch_index, pitch_fraction) = screen_cell(
        pitch,
        bounds.negative_pitch,
        bounds.positive_pitch,
        bounds.negative_pitch_frames,
        bounds.positive_pitch_frames,
    );
    if pitch_index < 0 || pitch_index >= grid_height || yaw_index < 0 || yaw_index >= grid_width {
        return;
    }
    let next_yaw = if yaw_index + 1 == grid_width {
        yaw_index
    } else {
        yaw_index + 1
    };
    let next_pitch = if pitch_index + 1 == grid_height {
        pitch_index
    } else {
        pitch_index + 1
    };
    let corners = [
        (yaw_index, pitch_index),
        (next_yaw, pitch_index),
        (yaw_index, next_pitch),
        (next_yaw, next_pitch),
    ]
    .map(|(x, y)| (x + y * grid_width) as i16);
    if corners
        .iter()
        .any(|&frame| !frame_valid(animation, frame, false))
    {
        return;
    }
    let mut values = [[NodeValues::default(); MAX_NODES]; 4];
    for (values, &frame) in values.iter_mut().zip(&corners) {
        match frame_values(animation, frame, f32::from(frame), f32::from(frame), false) {
            Some(frame_values) => *values = frame_values,
            None => return,
        }
    }
    let [v00, v10, v01, v11] = &values;
    let (inverse_yaw, inverse_pitch) = (1.0 - yaw_fraction, 1.0 - pitch_fraction);
    let node_count = usize::from(animation.node_count).min(MAX_NODES);
    for (node, orientation) in nodes.iter_mut().enumerate().take(node_count) {
        let corner = |values: &[NodeValues; MAX_NODES]| values[node];
        let (c00, c10, c01, c11) = (corner(v00), corner(v10), corner(v01), corner(v11));
        if let (Some(r00), Some(r10), Some(r01), Some(r11)) =
            (c00.rotation, c10.rotation, c01.rotation, c11.rotation)
        {
            let yaw0 = quaternions_interpolate(r00, r10, yaw_fraction);
            let yaw1 = quaternions_interpolate(r01, r11, yaw_fraction);
            let rotation = quaternions_interpolate(yaw0, yaw1, pitch_fraction);
            orientation.rotation = quaternions_multiply(rotation, orientation.rotation);
        }
        if let (Some(t00), Some(t10), Some(t01), Some(t11)) = (
            c00.translation,
            c10.translation,
            c01.translation,
            c11.translation,
        ) {
            for axis in 0..3 {
                orientation.translation[axis] +=
                    (t01[axis] * inverse_yaw + t11[axis] * yaw_fraction) * pitch_fraction
                        + (t00[axis] * inverse_yaw + t10[axis] * yaw_fraction) * inverse_pitch;
            }
        }
    }
}

/// Blends from `original` toward `target`, into `target`, by frame
/// `frame` of `frame_count` (`interpolate_node_orientations`): at the last
/// frame it is all `target`.
pub fn interpolate_orientations(
    original: &[Orientation],
    target: &mut [Orientation],
    frame: i16,
    frame_count: i16,
) {
    if frame_count <= 0 {
        return;
    }
    let fraction = f32::from(frame + 1) / f32::from(frame_count);
    let inverse = 1.0 - fraction;
    for (target, original) in target.iter_mut().zip(original).take(MAX_NODES) {
        target.scale = original.scale * inverse + target.scale * fraction;
        target.rotation = quaternions_interpolate(original.rotation, target.rotation, fraction);
        for axis in 0..3 {
            target.translation[axis] =
                original.translation[axis] * inverse + target.translation[axis] * fraction;
        }
    }
}

/// The nodes' matrices in the world, from their orientations and the
/// object's `root` (`animation_graph_node_matrices_from_orientations`):
/// the graph's nodes from the first, each after its parent, in the order
/// the game reaches them.
pub fn node_matrices(
    nodes: &[AnimationNode],
    orientations: &[Orientation],
    root: &NodeMatrix,
) -> Vec<NodeMatrix> {
    let node_count = nodes.len().min(MAX_NODES);
    let mut matrices = vec![NodeMatrix::IDENTITY; node_count];
    if node_count == 0 {
        return matrices;
    }
    let mut queue = [0u16; MAX_NODES];
    let (mut read, mut write) = (0, 1);
    while read != write {
        let index = usize::from(queue[read]);
        read += 1;
        let node = &nodes[index];
        let parent = if index == 0 {
            *root
        } else {
            match node.parent.map(usize::from) {
                Some(parent) if parent < node_count => matrices[parent],
                // (a parent the graph doesn't have is the first node)
                _ => matrices[0],
            }
        };
        let local =
            NodeMatrix::from_orientation(orientations.get(index).unwrap_or(&Orientation::REST));
        matrices[index] = parent.multiply(&local);
        // (only nodes the graph has, and no more than the queue holds: a
        // map's links could go in a loop)
        for link in [node.next_sibling, node.first_child].into_iter().flatten() {
            if usize::from(link) < node_count && write < MAX_NODES {
                queue[write] = link;
                write += 1;
            }
        }
    }
    matrices
}

/// What happened to an animation in a frame (`animation_update_result`).
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Update {
    /// It played on.
    Running,
    /// It reached one of its key frames.
    KeyFrame,
    /// It ends on the next frame.
    WillRestartOnNextFrame,
    /// It ended, and one of its permutations started.
    Restarted,
    /// It reached its end and went back to its loop frame.
    Looped,
}

/// An animation playing: which, and its frame.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq, Hash)]
pub struct AnimationState {
    /// The animation, in its graph's.
    pub animation: u16,
    /// Its frame.
    pub frame: u16,
}

/// One of the permutations from `first` on, chosen by `random` from 0 to 1
/// by their weights (`animation_choose_random_permutation_internal`); a
/// bad list is `first`, or none.
pub fn choose_permutation(
    animations: &[Animation],
    first: Option<u16>,
    random: f32,
) -> Option<u16> {
    let mut index = first;
    let mut count = 0;
    while let Some(at) = index {
        let Some(animation) = animations.get(usize::from(at)) else {
            return first.filter(|&first| usize::from(first) < animations.len());
        };
        if count >= animations.len() {
            return first.filter(|&first| usize::from(first) < animations.len());
        }
        count += 1;
        if random <= animation.normalized_weight {
            break;
        }
        index = animation.next_animation;
    }
    index
}

/// Plays `state` on a frame (`animation_update_internal`): at its end it
/// loops to its loop frame, or one of its permutations is chosen with
/// `random` (drawn only then). An animation the graph doesn't have stays
/// where it is.
pub fn advance(
    animations: &[Animation],
    state: &mut AnimationState,
    random: impl FnOnce() -> f32,
) -> Update {
    let Some(animation) = animations.get(usize::from(state.animation)) else {
        return Update::Running;
    };
    let frame = state.frame.saturating_add(1);
    state.frame = frame;
    if frame >= animation.frame_count {
        if animation.loop_frame > 0 {
            state.frame = animation
                .loop_frame
                .min(animation.frame_count.saturating_sub(1));
            Update::Looped
        } else {
            let chosen = choose_permutation(animations, animation.parent_animation, random());
            // (an animation with no permutations plays again)
            state.animation = chosen.unwrap_or(state.animation);
            state.frame = 0;
            Update::Restarted
        }
    } else if frame + 1 == animation.frame_count && animation.loop_frame == 0 {
        Update::WillRestartOnNextFrame
    } else if i32::from(frame) == i32::from(animation.key_frame)
        || i32::from(frame) == i32::from(animation.second_key_frame)
    {
        Update::KeyFrame
    } else {
        Update::Running
    }
}

/// An object's animation: its graph, its model, and what plays now.
#[derive(Component, Clone, Debug)]
pub struct AnimationPlayer {
    /// The graph its animations are in.
    pub graph: Arc<AnimationGraph>,
    /// The model the animations pose, whose default pose stands in for an
    /// animation made for other nodes.
    pub model: Option<Arc<Model>>,
    /// The animation playing, and its frame.
    pub state: AnimationState,
    /// The nodes' links: the graph's, or the model's when the graph has
    /// none of its own (a vehicle's or scenery's).
    links: Arc<[AnimationNode]>,
    /// The pose it blends from into its animation.
    blend: Option<Blend>,
    /// The animation it goes back to when the one playing ends.
    then: Option<u16>,
}

/// A blend from a pose into the animation playing
/// (`object_start_interpolation`): the pose, and the frame of the blend
/// and its length.
#[derive(Clone, Debug)]
struct Blend {
    from: Vec<Orientation>,
    frame: u16,
    frames: u16,
}

/// The frames a unit blends back into what it did over when an impulse
/// ends.
const RESUME_FRAMES: u16 = 6;

/// The animation an object stands still with: for a unit, its standing
/// seat's (else its first seat's) unarmed (else first) weapon class's
/// idle; for anything else, or a unit whose graph has none, the graph's
/// first.
pub fn standing_animation(graph: &AnimationGraph, unit: bool) -> u16 {
    /// `_unit_weapon_class_animation_idle`.
    const IDLE: usize = 0;
    let idle = || {
        let seat = graph
            .unit_seats
            .iter()
            .find(|seat| seat.label == "stand")
            .or(graph.unit_seats.first())?;
        let class = seat
            .weapon_classes
            .iter()
            .find(|class| class.label == "unarmed")
            .or(seat.weapon_classes.first())?;
        class
            .animations
            .get(IDLE)
            .copied()
            .flatten()
            .filter(|&animation| usize::from(animation) < graph.animations.len())
    };
    if unit {
        idle().unwrap_or(0)
    } else {
        0
    }
}

impl AnimationPlayer {
    /// Plays `animation` of `graph` from its first frame, posing `model`.
    pub fn new(graph: Arc<AnimationGraph>, model: Option<Arc<Model>>, animation: u16) -> Self {
        let links: Arc<[AnimationNode]> = match &model {
            Some(model) if graph.nodes.is_empty() => model
                .nodes
                .iter()
                .map(|node| AnimationNode {
                    name: node.name.clone(),
                    parent: node.parent,
                    first_child: node.first_child,
                    next_sibling: node.next_sibling,
                })
                .collect(),
            _ => graph.nodes.iter().cloned().collect(),
        };
        Self {
            graph,
            model,
            state: AnimationState {
                animation,
                frame: 0,
            },
            links,
            blend: None,
            then: None,
        }
    }

    /// Starts the animation at `frame` instead, if it has that frame
    /// (`unit_custom_animation_at_frame`); false, and from its first, if
    /// not.
    pub fn start_at(&mut self, frame: u16) -> bool {
        let frames = self
            .graph
            .animations
            .get(usize::from(self.state.animation))
            .map_or(0, |a| a.frame_count);
        let valid = frame < frames;
        if valid {
            self.state.frame = frame;
        }
        valid
    }

    /// Blends into the animation from `from` (another player's pose of the
    /// same nodes) over `frames` frames (`object_start_interpolation`).
    pub fn blend_from(&mut self, from: Vec<Orientation>, frames: u16) {
        if from.len() == self.links.len().min(MAX_NODES) && frames > 0 {
            self.blend = Some(Blend {
                from,
                frame: 0,
                frames,
            });
        }
    }

    /// Plays `animation` from its first frame, blending into it from the
    /// pose now over `frames` frames (none for a cut); at its end the
    /// animation playing now plays again, from its first frame, if
    /// `resume`.
    pub fn play(&mut self, animation: u16, frames: u16, resume: bool) {
        let from = self.blended_pose();
        if resume {
            self.then = Some(self.then.unwrap_or(self.state.animation));
        } else {
            self.then = None;
        }
        self.state = AnimationState {
            animation,
            frame: 0,
        };
        self.blend = (frames > 0).then_some(Blend {
            from,
            frame: 0,
            frames,
        });
    }

    /// Whether an animation played to its end goes back to another (an
    /// impulse is playing).
    pub fn resumes(&self) -> bool {
        self.then.is_some()
    }

    /// The pose with the blend into the animation applied.
    pub fn blended_pose(&self) -> Vec<Orientation> {
        let mut nodes = self.pose();
        if let Some(blend) = &self.blend {
            interpolate_orientations(
                &blend.from,
                &mut nodes,
                i16::try_from(blend.frame).unwrap_or(i16::MAX),
                i16::try_from(blend.frames).unwrap_or(i16::MAX),
            );
        }
        nodes
    }

    /// Moves to the next frame: the blend goes on, and an animation that
    /// ended goes back to the one it interrupted.
    fn step(&mut self, random: impl FnOnce() -> f32) {
        let update = advance(&self.graph.animations, &mut self.state, random);
        if let Some(blend) = &mut self.blend {
            blend.frame += 1;
            if blend.frame + 1 >= blend.frames {
                self.blend = None;
            }
        }
        if matches!(update, Update::Restarted | Update::Looped) {
            if let Some(then) = self.then.take() {
                let from = self.blended_pose();
                self.state = AnimationState {
                    animation: then,
                    frame: 0,
                };
                self.blend = Some(Blend {
                    from,
                    frame: 0,
                    frames: RESUME_FRAMES,
                });
            }
        }
    }

    /// The nodes the animation moves, each with its links.
    pub fn nodes(&self) -> &[AnimationNode] {
        &self.links
    }

    /// The nodes' orientations at the animation's frame: a base animation
    /// poses them; a replacement or an overlay goes over the model's
    /// default pose.
    pub fn pose(&self) -> Vec<Orientation> {
        let mut nodes = vec![Orientation::REST; self.links.len().min(MAX_NODES)];
        let model = self.model.as_deref();
        let frame = i16::try_from(self.state.frame).unwrap_or(i16::MAX);
        let animation = self.graph.animations.get(usize::from(self.state.animation));
        match animation {
            Some(animation) if animation.kind == AnimationKind::Base => {
                base_orientations(model, animation, frame, &mut nodes);
            }
            _ => {
                if let Some(model) = model {
                    model_orientations(model, &mut nodes);
                }
                match animation {
                    Some(animation) if animation.kind == AnimationKind::Replacement => {
                        apply_replacement(animation, frame, &mut nodes);
                    }
                    Some(animation) => apply_overlay(animation, frame, &mut nodes),
                    None => {}
                }
            }
        }
        nodes
    }

    /// The nodes' matrices in the world, for an object at `transform`.
    pub fn matrices(&self, transform: &Transform) -> Vec<NodeMatrix> {
        node_matrices(&self.links, &self.blended_pose(), &root_matrix(transform))
    }
}

/// An object's matrix: at its position, facing its forward (x) with its up
/// (z) up, at its scale.
pub fn root_matrix(transform: &Transform) -> NodeMatrix {
    let forward = transform.rotation * Vec3::X;
    let up = transform.rotation * Vec3::Z;
    NodeMatrix {
        scale: transform.scale,
        ..NodeMatrix::from_point_and_vectors(
            transform.position.to_array(),
            forward.to_array(),
            up.to_array(),
        )
    }
}

/// An animated object's nodes in the world, at the last two ticks; the
/// renderer interpolates between them as it does the object.
#[derive(Component, Clone, Debug, Default, PartialEq)]
pub struct NodeMatrices {
    /// At the tick before the last.
    pub previous: Vec<NodeMatrix>,
    /// At the last tick.
    pub current: Vec<NodeMatrix>,
}

/// Plays each object's animation on by a frame and poses its nodes, in the
/// order of the objects' ids, so that the seed is drawn from in the same
/// order on every machine.
pub(crate) fn play_animations(
    mut seed: ResMut<RandomSeed>,
    motions: Option<Res<VehicleMotions>>,
    table: Option<Res<ObjectTable>>,
    order: Query<(Entity, &ObjectId), With<AnimationPlayer>>,
    mut players: Query<(
        &Transform,
        &mut AnimationPlayer,
        &mut NodeMatrices,
        Option<&DevicePose>,
    )>,
) {
    let mut entities: Vec<(Entity, ObjectId)> =
        order.iter().map(|(entity, id)| (entity, *id)).collect();
    entities.sort_by_key(|(_, id)| *id);
    // (a vehicle's overlays over its animation, by its movement)
    let motions: Vec<(Entity, &VehicleMotion)> = match (&motions, &table) {
        (Some(motions), Some(table)) => motions
            .0
            .iter()
            .filter_map(|(handle, motion)| Some((table.entity(*handle)?, motion)))
            .collect(),
        _ => Vec::new(),
    };
    for (entity, _) in entities {
        let Ok((transform, mut player, mut matrices, device)) = players.get_mut(entity) else {
            continue;
        };
        let player = &mut *player;
        player.step(|| seed.next_real());
        let current = match motions.iter().find(|(vehicle, _)| *vehicle == entity) {
            Some((_, motion)) => {
                let mut nodes = player.blended_pose();
                let model_nodes = player
                    .model
                    .as_ref()
                    .map_or(nodes.len(), |model| model.nodes.len());
                apply_vehicle_overlays(&player.graph, model_nodes, motion, &mut nodes);
                node_matrices(&player.links, &nodes, &root_matrix(transform))
            }
            None => match device {
                // (a device's position and power overlays over its animation)
                Some(pose) => {
                    let mut nodes = player.blended_pose();
                    apply_device_overlays(&player.graph, pose, &mut nodes);
                    node_matrices(&player.links, &nodes, &root_matrix(transform))
                }
                None => player.matrices(transform),
            },
        };
        matrices.previous = std::mem::replace(&mut matrices.current, current);
    }
}

#[cfg(test)]
pub(crate) mod tests {
    use super::*;
    use opence_content::{Group, ModelNode, TagRef};

    /// An animation named `name` of `node_count` nodes and one frame.
    pub(crate) fn animation_named(name: &str, node_count: u16) -> Animation {
        Animation {
            name: name.into(),
            node_count,
            ..animation(1)
        }
    }

    /// A graph of `animations` alone.
    pub(crate) fn graph_of(animations: Vec<Animation>) -> AnimationGraph {
        graph(animations)
    }

    fn animation(frame_count: u16) -> Animation {
        Animation {
            name: "a".into(),
            kind: AnimationKind::Base,
            frame_count,
            frame_size: 0,
            frame_info_type: 0,
            node_list_checksum: 0,
            node_count: 2,
            loop_frame: 0,
            weight: 1.0,
            key_frame: -1,
            second_key_frame: -1,
            next_animation: None,
            parent_animation: None,
            normalized_weight: 1.0,
            flags: 0,
            sound: None,
            sound_frame: -1,
            left_foot_frame: -1,
            right_foot_frame: -1,
            frame_info: Vec::new(),
            translation_flags: [0; 2],
            rotation_flags: [0; 2],
            scale_flags: [0; 2],
            compressed_data_offset: 0,
            default_data: Vec::new(),
            data: Vec::new(),
        }
    }

    fn model(checksum: i32, node_count: usize) -> Model {
        let node = ModelNode {
            name: "n".into(),
            parent: None,
            first_child: None,
            next_sibling: None,
            translation: [1.0, 2.0, 3.0],
            rotation: [0.0, 0.0, 0.6, 0.8],
            distance_from_parent: 0.0,
            default_inverse_scale: 1.0,
            default_inverse: [[1.0, 0.0, 0.0], [0.0, 1.0, 0.0], [0.0, 0.0, 1.0], [0.0; 3]],
        };
        Model {
            tag: TagRef {
                group: Group(*b"mod2"),
                path: "test".into(),
            },
            flags: 0,
            node_list_checksum: checksum,
            detail_cutoffs: [0.0; 5],
            base_map_scale: [1.0; 2],
            nodes: vec![node; node_count],
            markers: Vec::new(),
            regions: Vec::new(),
            geometries: Vec::new(),
            shaders: Vec::new(),
        }
    }

    #[test]
    fn an_animation_for_other_nodes_poses_the_model_by_default() {
        // (no data: a frame the animation can't give, or nodes it wasn't
        // made for, is the model's pose)
        let mut playing = animation(4);
        playing.node_list_checksum = 7;
        playing.default_data = vec![0; 2 * 32];
        let expected = Orientation {
            rotation: [0.0, 0.0, 0.6, 0.8],
            translation: [1.0, 2.0, 3.0],
            scale: 1.0,
        };
        for model in [model(8, 2), model(7, 3)] {
            let mut nodes = [Orientation::REST; 2];
            base_orientations(Some(&model), &playing, 0, &mut nodes);
            assert_eq!(nodes, [expected; 2]);
        }
        // made for it, it is the animation's (here its defaults)
        let mut nodes = [expected; 2];
        base_orientations(Some(&model(7, 2)), &playing, 0, &mut nodes);
        assert_ne!(nodes[0], expected);
        // and with no model, the rest pose
        let mut nodes = [expected; 2];
        base_orientations(None, &animation(0), 0, &mut nodes);
        assert_eq!(nodes, [Orientation::REST; 2]);
    }

    #[test]
    fn an_animation_plays_to_its_end_and_starts_again() {
        let mut one = animation(4);
        one.key_frame = 1;
        one.parent_animation = Some(0);
        let animations = [one];
        let mut state = AnimationState {
            animation: 0,
            frame: 0,
        };
        let mut updates = Vec::new();
        for _ in 0..5 {
            updates.push(advance(&animations, &mut state, || 0.5));
        }
        assert_eq!(
            updates,
            [
                Update::KeyFrame,
                Update::Running,
                Update::WillRestartOnNextFrame,
                Update::Restarted,
                Update::KeyFrame
            ]
        );
        assert_eq!(state.frame, 1);
    }

    #[test]
    fn a_looping_animation_goes_back_to_its_loop_frame() {
        let mut looping = animation(3);
        looping.loop_frame = 1;
        let animations = [looping];
        let mut state = AnimationState {
            animation: 0,
            frame: 1,
        };
        let never = || -> f32 { panic!("a loop draws no random number") };
        assert_eq!(advance(&animations, &mut state, never), Update::Running);
        assert_eq!(advance(&animations, &mut state, never), Update::Looped);
        assert_eq!(state.frame, 1);
    }

    #[test]
    fn a_permutation_is_chosen_by_its_weight() {
        // three permutations, 0 to 0.2, to 0.7 and to 1
        let mut animations: Vec<Animation> = (0..3).map(|_| animation(2)).collect();
        for (index, (weight, next)) in [(0.2, Some(1)), (0.7, Some(2)), (1.0, None)]
            .into_iter()
            .enumerate()
        {
            animations[index].normalized_weight = weight;
            animations[index].next_animation = next;
            animations[index].parent_animation = Some(0);
        }
        assert_eq!(choose_permutation(&animations, Some(0), 0.1), Some(0));
        assert_eq!(choose_permutation(&animations, Some(0), 0.2), Some(0));
        assert_eq!(choose_permutation(&animations, Some(0), 0.5), Some(1));
        assert_eq!(choose_permutation(&animations, Some(0), 0.9), Some(2));
        // (one past the last weight runs off the list)
        assert_eq!(choose_permutation(&animations, Some(0), 1.5), None);
        // a list in a loop, or one off the graph, is its first
        animations[2].next_animation = Some(0);
        assert_eq!(choose_permutation(&animations, Some(1), 1.5), Some(1));
        animations[2].next_animation = Some(9);
        assert_eq!(choose_permutation(&animations, Some(1), 1.5), Some(1));
        assert_eq!(choose_permutation(&animations, Some(9), 0.0), None);
        // the state ends on the chosen one
        let mut state = AnimationState {
            animation: 2,
            frame: 1,
        };
        assert_eq!(advance(&animations, &mut state, || 0.5), Update::Restarted);
        assert_eq!(
            state,
            AnimationState {
                animation: 1,
                frame: 0
            }
        );
    }

    #[test]
    fn a_graph_whose_links_loop_ends() {
        // every node its own sibling and child
        let nodes: Vec<AnimationNode> = (0..3)
            .map(|index| AnimationNode {
                name: String::new(),
                parent: Some(0),
                first_child: Some(index),
                next_sibling: Some(index),
            })
            .collect();
        let orientations = [Orientation::REST; 3];
        let matrices = node_matrices(&nodes, &orientations, &NodeMatrix::IDENTITY);
        assert_eq!(matrices, vec![NodeMatrix::IDENTITY; 3]);
    }

    fn graph(animations: Vec<Animation>) -> AnimationGraph {
        AnimationGraph {
            tag: TagRef {
                group: Group(*b"antr"),
                path: "test".into(),
            },
            flags: 0,
            limp_body_node_radius: 0.0,
            nodes: Vec::new(),
            animations,
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
    fn a_unit_stands_in_its_unarmed_idle() {
        use opence_content::{UnitSeat, WeaponClass};
        let class = |label: &str, idle: Option<u16>| WeaponClass {
            label: label.into(),
            animations: vec![idle, Some(0)],
            ..WeaponClass::default()
        };
        let seat = |label: &str, classes: Vec<WeaponClass>| UnitSeat {
            label: label.into(),
            weapon_classes: classes,
            ..UnitSeat::default()
        };
        let mut graph = graph(vec![animation(1); 4]);
        graph.unit_seats = vec![
            seat("crouch", vec![class("unarmed", Some(1))]),
            seat(
                "stand",
                vec![class("pistol", Some(2)), class("unarmed", Some(3))],
            ),
        ];
        assert_eq!(standing_animation(&graph, true), 3);
        // (scenery plays its first, whatever seats its graph has)
        assert_eq!(standing_animation(&graph, false), 0);
        // (no standing seat: the first; no unarmed class: the first)
        graph.unit_seats.remove(1);
        graph.unit_seats[0].weapon_classes[0].label = "rifle".into();
        assert_eq!(standing_animation(&graph, true), 1);
        // (an idle past the graph's animations, or none, is its first)
        graph.unit_seats[0].weapon_classes[0].animations[0] = Some(9);
        assert_eq!(standing_animation(&graph, true), 0);
        graph.unit_seats.clear();
        assert_eq!(standing_animation(&graph, true), 0);
    }

    fn spawned(simulation: &mut crate::Simulation, x: f32) -> Option<crate::objects::ObjectHandle> {
        simulation
            .spawn(crate::objects::ObjectSpawn {
                kind: opence_content::ObjectKind::Vehicle,
                tag: TagRef {
                    group: Group(*b"vehi"),
                    path: "test".into(),
                },
                transform: Transform {
                    position: Vec3::new(x, 0.0, 0.0),
                    ..Default::default()
                },
                name: None,
                permutation: None,
                team: None,
            })
            .ok()
    }

    #[test]
    fn a_graph_without_nodes_poses_its_models() {
        // (a vehicle's graph: the model's two nodes, the second the
        // first's child)
        let mut chain = model(0, 2);
        chain.nodes[0].first_child = Some(1);
        chain.nodes[1].parent = Some(0);
        let player = AnimationPlayer::new(
            Arc::new(graph(vec![animation(3)])),
            Some(Arc::new(chain)),
            0,
        );
        assert_eq!(player.nodes().len(), 2);
        let matrices = player.matrices(&Transform {
            position: Vec3::new(10.0, 0.0, 0.0),
            ..Default::default()
        });
        let child = NodeMatrix::IDENTITY
            .multiply(&NodeMatrix::from_orientation(&player.pose()[0]))
            .multiply(&NodeMatrix::from_orientation(&player.pose()[1]));
        assert_eq!(matrices.len(), 2);
        assert_eq!(
            matrices[1].position,
            [
                child.position[0] + 10.0,
                child.position[1],
                child.position[2]
            ]
        );
    }

    #[test]
    fn the_world_plays_each_objects_animation() {
        let mut second = animation(2);
        second.weight = 1.0;
        let mut first = animation(3);
        first.parent_animation = Some(1);
        let graph = Arc::new(graph(vec![first, second]));
        let model = Arc::new(model(0, 2));
        let run = || {
            let mut simulation = crate::Simulation::empty();
            let handles: Vec<_> = [1.0, 2.0]
                .into_iter()
                .filter_map(|x| spawned(&mut simulation, x))
                .collect();
            for &handle in &handles {
                let player = AnimationPlayer::new(graph.clone(), Some(model.clone()), 0);
                assert!(simulation.animate(handle, player));
            }
            let mut states = Vec::new();
            for _ in 0..4 {
                simulation.step(crate::input::TickInput::default());
                for &handle in &handles {
                    let player = simulation.component::<AnimationPlayer>(handle);
                    states.push(player.map(|player| player.state));
                }
            }
            let mut drawn = Vec::new();
            simulation.visit_drawn(|object| {
                drawn.push((
                    object.id,
                    object.current.position.x,
                    object.nodes.map(|n| n.current.len()),
                ));
            });
            (states, drawn, simulation.world_hash())
        };
        let (states, drawn, hash) = run();
        let state = |animation, frame| Some(AnimationState { animation, frame });
        // frames 1, 2, then the end: its one permutation, then on
        assert_eq!(
            states.iter().step_by(2).copied().collect::<Vec<_>>(),
            vec![state(0, 1), state(0, 2), state(1, 0), state(1, 1)]
        );
        assert_eq!(
            drawn,
            vec![
                (crate::ObjectId(0), 1.0, Some(2)),
                (crate::ObjectId(1), 2.0, Some(2))
            ]
        );
        assert_eq!(run().2, hash);
    }
}
