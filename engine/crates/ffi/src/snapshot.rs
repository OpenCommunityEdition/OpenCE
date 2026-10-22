//! The per-frame snapshot: what the renderer draws, in `#[repr(C)]` types
//! that cbindgen writes into `opence_snapshot.h`.
//!
//! The simulation publishes a snapshot after each tick; Swift reads the
//! latest one in place. The three slots are a triple buffer: the writer
//! fills its own slot and swaps it with the middle one, and the reader swaps
//! its slot with the middle one when a newer snapshot is there, so neither
//! waits, nothing is copied, and a slot is never written while it is read.

use std::cell::UnsafeCell;
use std::sync::atomic::{AtomicU8, Ordering};

/// The most objects one snapshot holds.
pub const MAX_OBJECTS: usize = 4096;

/// The most animated nodes one snapshot holds, for all its objects.
pub const MAX_NODES: usize = 16384;

/// The most debug lines one snapshot holds.
pub const MAX_DEBUG_LINES: usize = opence_dev::draw::MAX_LINES;

/// The most debug labels one snapshot holds.
pub const MAX_DEBUG_LABELS: usize = opence_dev::draw::MAX_LABELS;

/// The bytes of a debug label's text, its end padded with zeros.
pub const DEBUG_LABEL_BYTES: usize = 48;

/// The most dynamic lights one snapshot holds.
pub const MAX_LIGHTS: usize = 512;

/// The most dynamic decals one snapshot holds.
pub const MAX_DECALS: usize = 2048;

/// The most running effects one snapshot holds.
pub const MAX_EFFECTS: usize = 256;

/// Raised with any change to the snapshot's types, so that the app refuses
/// a library of another layout.
pub const SNAPSHOT_VERSION: u32 = 9;

/// `ObjectSnapshot::flags`: the first-person weapon or hands, posed from
/// the camera and drawn in front of the world.
pub const OBJECT_FIRST_PERSON: u16 = 1;

/// A vector in world units.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct Vec3 {
    /// X.
    pub x: f32,
    /// Y.
    pub y: f32,
    /// Z (up).
    pub z: f32,
}

/// A unit quaternion.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct Quat {
    /// X.
    pub x: f32,
    /// Y.
    pub y: f32,
    /// Z.
    pub z: f32,
    /// W.
    pub w: f32,
}

/// A position, a uniform scale and an orientation.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct Transform {
    /// The position, in world units.
    pub position: Vec3,
    /// The uniform scale.
    pub scale: f32,
    /// The orientation.
    pub rotation: Quat,
}

/// The camera at the last two ticks; the renderer interpolates between them.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct CameraSnapshot {
    /// At the tick before the last.
    pub previous: Transform,
    /// At the last tick.
    pub current: Transform,
    /// The vertical field of view; the horizontal one follows the aspect
    /// ratio.
    pub vertical_fov_radians: f32,
    /// (zero)
    pub reserved: [u32; 3],
}

/// A node's matrix in the world, the game's: a point `p` of the node is
/// `scale * (p.x * forward + p.y * left + p.z * up) + position`.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct NodeMatrix {
    /// Where its x goes.
    pub forward: Vec3,
    /// Where its y goes.
    pub left: Vec3,
    /// Where its z goes.
    pub up: Vec3,
    /// Where its origin goes.
    pub position: Vec3,
    /// How much it scales.
    pub scale: f32,
}

/// An animated node at the last two ticks.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct NodeSnapshot {
    /// At the tick before the last.
    pub previous: NodeMatrix,
    /// At the last tick.
    pub current: NodeMatrix,
}

/// One object to draw.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct ObjectSnapshot {
    /// The object's id, the same from tick to tick.
    pub id: u32,
    /// The index of its render model in the map's content, or `u32::MAX`
    /// for none.
    pub model: u32,
    /// Flags (`OBJECT_FIRST_PERSON`).
    pub flags: u16,
    /// The number of its nodes in the frame's `nodes`; none if it is not
    /// animated.
    pub node_count: u16,
    /// Its first node in the frame's `nodes`.
    pub first_node: u32,
    /// At the tick before the last.
    pub previous: Transform,
    /// At the last tick.
    pub current: Transform,
    /// The radius of its bounding sphere, which its model's level of
    /// detail is chosen by.
    pub bounding_radius: f32,
    /// The permutation each of its model's regions is drawn with.
    pub regions: [u8; 8],
    /// Its four change colours, as `0xFFRRGGBB`.
    pub change_colors: [u32; 4],
}

/// `LightSnapshot::flags`: it is a cone (else it shines all round).
pub const LIGHT_SPOT: u32 = 1;
/// `LightSnapshot::flags`: it makes no highlights.
pub const LIGHT_NO_SPECULAR: u32 = 1 << 1;
/// `LightSnapshot::flags`: it is the first-person view's flashlight, at
/// the camera.
pub const LIGHT_FIRST_PERSON: u32 = 1 << 2;

/// A dynamic light: a weapon's, a projectile's, an effect's, the
/// flashlight, a piece of scenery's.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct LightSnapshot {
    /// Where it is at the last tick.
    pub position: Vec3,
    /// How far it reaches.
    pub radius: f32,
    /// Where it was at the tick before the last.
    pub previous: Vec3,
    /// The cosine of the angle from `forward` its cone ends at.
    pub cosine_cutoff: f32,
    /// The way it faces (its gel's x).
    pub forward: Vec3,
    /// The cosine of the angle from `forward` its cone starts to fall off
    /// at.
    pub cosine_falloff: f32,
    /// Its gel's up.
    pub up: Vec3,
    /// How much farther than its radius its highlights reach.
    pub specular_radius: f32,
    /// Its colour, linear.
    pub color: Vec3,
    /// Flags (`LIGHT_SPOT` and the rest).
    pub flags: u32,
    /// Its tag's index in the map's lights, which its gel is found by.
    pub light: u32,
    /// What made it, the same from frame to frame (its object's id and
    /// attachment, or its effect's number), which keeps its shadow.
    pub source: u32,
}

/// `DecalSnapshot::modes`' picture when it has none: drawn as its colour.
pub const DECAL_NO_PICTURE: u32 = u32::MAX;
/// `DecalSnapshot::modes`' surface: the level's.
pub const DECAL_ON_LEVEL: u32 = 1;
/// `DecalSnapshot::modes`' surface: an object's.
pub const DECAL_ON_OBJECT: u32 = 2;

/// A dynamic decal: a mark an impact or an explosion left, laid as the
/// level's decals are (laid out as the app's decal shader reads it).
#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct DecalSnapshot {
    /// Its centre, then how far above and below its surface it reaches.
    pub center: [f32; 4],
    /// The way it faces forward on its surface, then its least reach that
    /// way.
    pub forward: [f32; 4],
    /// The way to its left, then its least reach that way.
    pub left: [f32; 4],
    /// Its surface's normal, then 0.
    pub up: [f32; 4],
    /// Its greatest reach forward and left, then 0, 0.
    pub reach: [f32; 4],
    /// Its picture's texture coordinates: u0, v0, u1, v1.
    pub uv: [f32; 4],
    /// Its colour, linear, then its opacity.
    pub color: [f32; 4],
    /// Its framebuffer blend function, its layer, its picture (an index in
    /// the level's decal pictures, or `DECAL_NO_PICTURE`) and the surface
    /// it is laid on (`DECAL_ON_LEVEL` or `DECAL_ON_OBJECT`).
    pub modes: [u32; 4],
}

/// An effect running: where it is and which of its events it is in.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct EffectSnapshot {
    /// Where it is, then the seconds it has run.
    pub position: [f32; 4],
    /// The way it faces, then 0.
    pub forward: [f32; 4],
    /// Its number, its event's index, the slot of the object it is on
    /// (`u32::MAX`: none) and 1 if its event is skipped.
    pub ids: [u32; 4],
}

/// A debug line, drawn after the world.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct DebugLine {
    /// Its start.
    pub from: Vec3,
    /// Its end.
    pub to: Vec3,
    /// Its colour: red, green, blue and alpha a byte each from the top.
    pub color: u32,
}

/// A debug label at a point, drawn after the world.
#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct DebugLabel {
    /// Where it is.
    pub at: Vec3,
    /// Its colour, as a line's.
    pub color: u32,
    /// Its text in UTF-8, its end padded with zeros.
    pub text: [u8; DEBUG_LABEL_BYTES],
}

impl Default for DebugLabel {
    fn default() -> Self {
        Self {
            at: Vec3::default(),
            color: 0,
            text: [0; DEBUG_LABEL_BYTES],
        }
    }
}

/// One frame of the world.
#[repr(C)]
#[derive(Debug)]
pub struct FrameSnapshot {
    /// `SNAPSHOT_VERSION`.
    pub version: u32,
    /// The number of objects in `objects`.
    pub object_count: u32,
    /// The tick this snapshot was taken after.
    pub tick: u64,
    /// How far the display's time is between `previous` (0) and `current`
    /// (1), when the snapshot was published.
    pub alpha: f32,
    /// The number of nodes in `nodes`.
    pub node_count: u32,
    /// The camera.
    pub camera: CameraSnapshot,
    /// The objects: `object_count` of them, in the same slot as this
    /// header, valid as long as the snapshot is.
    pub objects: *const ObjectSnapshot,
    /// The animated objects' nodes: `node_count` of them, each object's
    /// together, in the same slot.
    pub nodes: *const NodeSnapshot,
    /// The number of lines in `debug_lines`.
    pub debug_line_count: u32,
    /// The number of labels in `debug_labels`.
    pub debug_label_count: u32,
    /// The debug lines, in the same slot.
    pub debug_lines: *const DebugLine,
    /// The debug labels, in the same slot.
    pub debug_labels: *const DebugLabel,
    /// The number of lights in `lights`.
    pub light_count: u32,
    /// The number of decals in `decals`.
    pub decal_count: u32,
    /// The dynamic lights, in the same slot.
    pub lights: *const LightSnapshot,
    /// The dynamic decals, layer by layer and picture by picture, in the
    /// same slot.
    pub decals: *const DecalSnapshot,
    /// The effects running, oldest first, in the same slot.
    pub effects: *const EffectSnapshot,
    /// The number of effects in `effects`.
    pub effect_count: u32,
}

/// The sizes and offsets of the snapshot's types, as Rust laid them out;
/// the app checks them against the header it was compiled with.
#[derive(uniffi::Record, Clone, Copy, Debug, PartialEq, Eq)]
pub struct SnapshotLayout {
    /// `SNAPSHOT_VERSION`.
    pub version: u32,
    /// `MAX_OBJECTS`.
    pub max_objects: u32,
    /// `MAX_NODES`.
    pub max_nodes: u32,
    /// The size of `FrameSnapshot`.
    pub frame_size: u32,
    /// The offset of `FrameSnapshot::tick`.
    pub frame_tick_offset: u32,
    /// The offset of `FrameSnapshot::alpha`.
    pub frame_alpha_offset: u32,
    /// The offset of `FrameSnapshot::camera`.
    pub frame_camera_offset: u32,
    /// The offset of `FrameSnapshot::objects`.
    pub frame_objects_offset: u32,
    /// The offset of `FrameSnapshot::nodes`.
    pub frame_nodes_offset: u32,
    /// The size of `CameraSnapshot`.
    pub camera_size: u32,
    /// The size of `ObjectSnapshot`.
    pub object_size: u32,
    /// The offset of `ObjectSnapshot::current`.
    pub object_current_offset: u32,
    /// The offset of `ObjectSnapshot::first_node`.
    pub object_first_node_offset: u32,
    /// The offset of `ObjectSnapshot::bounding_radius`.
    pub object_bounding_radius_offset: u32,
    /// The size of `Transform`.
    pub transform_size: u32,
    /// The size of `NodeSnapshot`.
    pub node_size: u32,
    /// `MAX_DEBUG_LINES`.
    pub max_debug_lines: u32,
    /// `MAX_DEBUG_LABELS`.
    pub max_debug_labels: u32,
    /// The offset of `FrameSnapshot::debug_lines`.
    pub frame_debug_lines_offset: u32,
    /// The offset of `FrameSnapshot::debug_labels`.
    pub frame_debug_labels_offset: u32,
    /// The size of `DebugLine`.
    pub debug_line_size: u32,
    /// The size of `DebugLabel`.
    pub debug_label_size: u32,
    /// `MAX_LIGHTS`.
    pub max_lights: u32,
    /// The offset of `FrameSnapshot::lights`.
    pub frame_lights_offset: u32,
    /// The size of `LightSnapshot`.
    pub light_size: u32,
    /// `MAX_DECALS`.
    pub max_decals: u32,
    /// The offset of `FrameSnapshot::decals`.
    pub frame_decals_offset: u32,
    /// The size of `DecalSnapshot`.
    pub decal_size: u32,
    /// `MAX_EFFECTS`.
    pub max_effects: u32,
    /// The offset of `FrameSnapshot::effects`.
    pub frame_effects_offset: u32,
    /// The size of `EffectSnapshot`.
    pub effect_size: u32,
}

/// The layout of the snapshot's types.
#[uniffi::export]
pub fn snapshot_layout() -> SnapshotLayout {
    use std::mem::{offset_of, size_of};
    // (all far below u32::MAX)
    let n = |value: usize| value as u32;
    SnapshotLayout {
        version: SNAPSHOT_VERSION,
        max_objects: n(MAX_OBJECTS),
        max_nodes: n(MAX_NODES),
        frame_size: n(size_of::<FrameSnapshot>()),
        frame_tick_offset: n(offset_of!(FrameSnapshot, tick)),
        frame_alpha_offset: n(offset_of!(FrameSnapshot, alpha)),
        frame_camera_offset: n(offset_of!(FrameSnapshot, camera)),
        frame_objects_offset: n(offset_of!(FrameSnapshot, objects)),
        frame_nodes_offset: n(offset_of!(FrameSnapshot, nodes)),
        camera_size: n(size_of::<CameraSnapshot>()),
        object_size: n(size_of::<ObjectSnapshot>()),
        object_current_offset: n(offset_of!(ObjectSnapshot, current)),
        object_first_node_offset: n(offset_of!(ObjectSnapshot, first_node)),
        object_bounding_radius_offset: n(offset_of!(ObjectSnapshot, bounding_radius)),
        transform_size: n(size_of::<Transform>()),
        node_size: n(size_of::<NodeSnapshot>()),
        max_debug_lines: n(MAX_DEBUG_LINES),
        max_debug_labels: n(MAX_DEBUG_LABELS),
        frame_debug_lines_offset: n(offset_of!(FrameSnapshot, debug_lines)),
        frame_debug_labels_offset: n(offset_of!(FrameSnapshot, debug_labels)),
        debug_line_size: n(size_of::<DebugLine>()),
        debug_label_size: n(size_of::<DebugLabel>()),
        max_lights: n(MAX_LIGHTS),
        frame_lights_offset: n(offset_of!(FrameSnapshot, lights)),
        light_size: n(size_of::<LightSnapshot>()),
        max_decals: n(MAX_DECALS),
        frame_decals_offset: n(offset_of!(FrameSnapshot, decals)),
        decal_size: n(size_of::<DecalSnapshot>()),
        max_effects: n(MAX_EFFECTS),
        frame_effects_offset: n(offset_of!(FrameSnapshot, effects)),
        effect_size: n(size_of::<EffectSnapshot>()),
    }
}

/// One slot of the triple buffer: the header and the objects and nodes it
/// points to.
#[repr(C)]
struct Slot {
    frame: FrameSnapshot,
    objects: [ObjectSnapshot; MAX_OBJECTS],
    nodes: [NodeSnapshot; MAX_NODES],
    debug_lines: [DebugLine; MAX_DEBUG_LINES],
    debug_labels: [DebugLabel; MAX_DEBUG_LABELS],
    lights: [LightSnapshot; MAX_LIGHTS],
    decals: [DecalSnapshot; MAX_DECALS],
    effects: [EffectSnapshot; MAX_EFFECTS],
}

impl Slot {
    fn new() -> Box<Self> {
        // SAFETY: every field of Slot is a number, a pointer or an array of
        // them, for which all zero bits are a valid value (a null pointer).
        let mut slot = unsafe { Box::<Self>::new_zeroed().assume_init() };
        slot.frame.version = SNAPSHOT_VERSION;
        // (the slot is boxed and never moves, so the pointer stays valid)
        slot.frame.objects = slot.objects.as_ptr();
        slot.frame.nodes = slot.nodes.as_ptr();
        slot.frame.debug_lines = slot.debug_lines.as_ptr();
        slot.frame.debug_labels = slot.debug_labels.as_ptr();
        slot.frame.lights = slot.lights.as_ptr();
        slot.frame.decals = slot.decals.as_ptr();
        slot.frame.effects = slot.effects.as_ptr();
        slot
    }
}

/// The bit of `middle` set when it holds a snapshot the reader has not
/// taken.
const FRESH: u8 = 0b100;
const INDEX: u8 = 0b011;

/// The three slots and the index of the middle one.
pub struct SnapshotBuffers {
    slots: [UnsafeCell<Box<Slot>>; 3],
    middle: AtomicU8,
    /// The reader's slot; only the reader touches it.
    reader: AtomicU8,
}

// SAFETY: the writer only touches the slot whose index it holds, the reader
// only the one in `reader`, and the indices are exchanged through `middle`
// with acquire-release swaps, so no slot is ever reached from two threads at
// once. The `objects` and `nodes` pointers inside each slot only point into
// that slot.
unsafe impl Sync for SnapshotBuffers {}
// SAFETY: as for Sync; the boxes are owned by the buffers.
unsafe impl Send for SnapshotBuffers {}

impl SnapshotBuffers {
    /// Three empty slots: the writer starts on slot 0, the middle is 1 and
    /// the reader holds 2, so a read before any publish sees an empty frame.
    pub fn new() -> (Self, SnapshotWriter) {
        let buffers = Self {
            slots: [
                UnsafeCell::new(Slot::new()),
                UnsafeCell::new(Slot::new()),
                UnsafeCell::new(Slot::new()),
            ],
            middle: AtomicU8::new(1),
            reader: AtomicU8::new(2),
        };
        (buffers, SnapshotWriter { slot: 0 })
    }

    /// The newest published snapshot. Only one thread may read at a time,
    /// and the snapshot stays valid until that thread acquires again.
    fn acquire(&self) -> *const FrameSnapshot {
        let mut reader = self.reader.load(Ordering::Relaxed);
        if self.middle.load(Ordering::Acquire) & FRESH != 0 {
            reader = self.middle.swap(reader, Ordering::AcqRel) & INDEX;
            self.reader.store(reader, Ordering::Relaxed);
        }
        // SAFETY: the reader's slot is not touched by the writer (see Sync).
        let slot = unsafe { &*self.slots[usize::from(reader)].get() };
        &slot.frame
    }
}

/// The writer's side: fills its slot, then publishes it.
pub struct SnapshotWriter {
    slot: u8,
}

/// The object list is full (`MAX_OBJECTS`), or the node list
/// (`MAX_NODES`).
#[derive(Debug, PartialEq, Eq, thiserror::Error)]
#[error("the snapshot holds at most {MAX_OBJECTS} objects and {MAX_NODES} nodes")]
pub struct SnapshotFull;

/// A snapshot being filled.
pub struct FrameBuilder<'a> {
    slot: &'a mut Slot,
}

impl FrameBuilder<'_> {
    /// Sets the camera.
    pub fn camera(&mut self, camera: CameraSnapshot) {
        self.slot.frame.camera = camera;
    }

    /// Adds an object.
    pub fn push_object(&mut self, object: ObjectSnapshot) -> Result<(), SnapshotFull> {
        self.push_animated(object, &[])
    }

    /// Adds an object with its nodes, which set its `first_node` and
    /// `node_count`. Neither is added if both don't fit.
    pub fn push_animated(
        &mut self,
        mut object: ObjectSnapshot,
        nodes: &[NodeSnapshot],
    ) -> Result<(), SnapshotFull> {
        let count = self.slot.frame.object_count as usize;
        let first = self.slot.frame.node_count as usize;
        let node_count = u16::try_from(nodes.len()).map_err(|_| SnapshotFull)?;
        if count >= MAX_OBJECTS || first + nodes.len() > MAX_NODES {
            return Err(SnapshotFull);
        }
        self.slot.nodes[first..first + nodes.len()].copy_from_slice(nodes);
        object.first_node = first as u32;
        object.node_count = node_count;
        self.slot.objects[count] = object;
        self.slot.frame.object_count += 1;
        self.slot.frame.node_count += u32::from(node_count);
        Ok(())
    }

    /// Adds `lights`, as many as fit.
    pub fn lights(&mut self, lights: &[LightSnapshot]) {
        let count = lights.len().min(MAX_LIGHTS);
        self.slot.lights[..count].copy_from_slice(&lights[..count]);
        self.slot.frame.light_count = count as u32;
    }

    /// Adds `decals`, as many as fit.
    pub fn decals(&mut self, decals: &[DecalSnapshot]) {
        let count = decals.len().min(MAX_DECALS);
        self.slot.decals[..count].copy_from_slice(&decals[..count]);
        self.slot.frame.decal_count = count as u32;
    }

    /// Adds `effects`, as many as fit.
    pub fn effects(&mut self, effects: &[EffectSnapshot]) {
        let count = effects.len().min(MAX_EFFECTS);
        self.slot.effects[..count].copy_from_slice(&effects[..count]);
        self.slot.frame.effect_count = count as u32;
    }

    /// Adds `draw`'s lines and labels, as many as fit.
    pub fn debug(&mut self, draw: &opence_dev::draw::DebugDraw) {
        let vector = |[x, y, z]: [f32; 3]| Vec3 { x, y, z };
        let frame = &mut self.slot.frame;
        for line in draw.lines() {
            let at = frame.debug_line_count as usize;
            if at >= MAX_DEBUG_LINES {
                break;
            }
            self.slot.debug_lines[at] = DebugLine {
                from: vector(line.from),
                to: vector(line.to),
                color: line.color.0,
            };
            frame.debug_line_count += 1;
        }
        for label in draw.labels() {
            let at = frame.debug_label_count as usize;
            if at >= MAX_DEBUG_LABELS {
                break;
            }
            let mut text = [0; DEBUG_LABEL_BYTES];
            // (the store cuts labels shorter than this, at a character)
            let bytes = label.text.as_bytes();
            let length = bytes.len().min(DEBUG_LABEL_BYTES - 1);
            text[..length].copy_from_slice(&bytes[..length]);
            self.slot.debug_labels[at] = DebugLabel {
                at: vector(label.at),
                color: label.color.0,
                text,
            };
            frame.debug_label_count += 1;
        }
    }
}

impl SnapshotWriter {
    /// Fills the writer's slot with `fill` and publishes it.
    pub fn publish<R>(
        &mut self,
        buffers: &SnapshotBuffers,
        tick: u64,
        alpha: f32,
        fill: impl FnOnce(&mut FrameBuilder<'_>) -> R,
    ) -> R {
        // SAFETY: the writer's slot is reached by no one else until it is
        // swapped into the middle below (see Sync), and the writer is not
        // shared (publish takes it by &mut).
        let slot = unsafe { &mut **buffers.slots[usize::from(self.slot)].get() };
        slot.frame.tick = tick;
        slot.frame.alpha = alpha;
        slot.frame.object_count = 0;
        slot.frame.node_count = 0;
        slot.frame.debug_line_count = 0;
        slot.frame.debug_label_count = 0;
        slot.frame.light_count = 0;
        slot.frame.decal_count = 0;
        slot.frame.effect_count = 0;
        let result = fill(&mut FrameBuilder { slot });
        self.slot = buffers.middle.swap(self.slot | FRESH, Ordering::AcqRel) & INDEX;
        result
    }
}

/// The newest snapshot of `buffers` (the address `Engine::snapshot_buffers`
/// gives). One thread reads at a time; the snapshot is valid until that
/// thread calls this again or the engine is dropped.
///
/// # Safety
/// `buffers` must be the address an `Engine` that is still alive gave.
#[no_mangle]
pub unsafe extern "C" fn opence_snapshot_acquire(
    buffers: *const SnapshotBuffers,
) -> *const FrameSnapshot {
    // SAFETY: the caller passes the live engine's buffers (above).
    match unsafe { buffers.as_ref() } {
        Some(buffers) => buffers.acquire(),
        None => std::ptr::null(),
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::mem::{align_of, offset_of, size_of};

    #[test]
    fn the_layout_is_fixed() {
        // (a change here is a change to the contract: raise SNAPSHOT_VERSION)
        assert_eq!(size_of::<Vec3>(), 12);
        assert_eq!(size_of::<Quat>(), 16);
        assert_eq!(size_of::<Transform>(), 32);
        assert_eq!(size_of::<CameraSnapshot>(), 80);
        assert_eq!(size_of::<ObjectSnapshot>(), 108);
        assert_eq!(offset_of!(ObjectSnapshot, bounding_radius), 80);
        assert_eq!(offset_of!(ObjectSnapshot, regions), 84);
        assert_eq!(offset_of!(ObjectSnapshot, change_colors), 92);
        assert_eq!(offset_of!(ObjectSnapshot, first_node), 12);
        assert_eq!(offset_of!(ObjectSnapshot, current), 48);
        assert_eq!(size_of::<NodeMatrix>(), 52);
        assert_eq!(size_of::<NodeSnapshot>(), 104);
        assert_eq!(offset_of!(FrameSnapshot, tick), 8);
        assert_eq!(offset_of!(FrameSnapshot, alpha), 16);
        assert_eq!(offset_of!(FrameSnapshot, camera), 24);
        assert_eq!(offset_of!(FrameSnapshot, node_count), 20);
        assert_eq!(offset_of!(FrameSnapshot, objects), 104);
        assert_eq!(offset_of!(FrameSnapshot, nodes), 112);
        assert_eq!(offset_of!(FrameSnapshot, debug_line_count), 120);
        assert_eq!(offset_of!(FrameSnapshot, debug_label_count), 124);
        assert_eq!(offset_of!(FrameSnapshot, debug_lines), 128);
        assert_eq!(offset_of!(FrameSnapshot, debug_labels), 136);
        assert_eq!(offset_of!(FrameSnapshot, light_count), 144);
        assert_eq!(offset_of!(FrameSnapshot, decal_count), 148);
        assert_eq!(offset_of!(FrameSnapshot, lights), 152);
        assert_eq!(offset_of!(FrameSnapshot, decals), 160);
        assert_eq!(offset_of!(FrameSnapshot, effects), 168);
        assert_eq!(offset_of!(FrameSnapshot, effect_count), 176);
        assert_eq!(size_of::<FrameSnapshot>(), 184);
        assert_eq!(size_of::<EffectSnapshot>(), 48);
        assert_eq!(offset_of!(EffectSnapshot, ids), 32);
        assert_eq!(size_of::<DecalSnapshot>(), 128);
        assert_eq!(offset_of!(DecalSnapshot, modes), 112);
        assert_eq!(size_of::<LightSnapshot>(), 88);
        assert_eq!(offset_of!(LightSnapshot, color), 64);
        assert_eq!(offset_of!(LightSnapshot, source), 84);
        assert_eq!(align_of::<FrameSnapshot>(), 8);
        assert_eq!(size_of::<DebugLine>(), 28);
        assert_eq!(size_of::<DebugLabel>(), 64);
        assert_eq!(offset_of!(DebugLabel, text), 16);
        assert_eq!(SNAPSHOT_VERSION, 9);
    }

    #[test]
    fn the_layout_record_reports_it() {
        let layout = snapshot_layout();
        assert_eq!(layout.frame_size, 184);
        assert_eq!(layout.frame_effects_offset, 168);
        assert_eq!(layout.effect_size, 48);
        assert_eq!(layout.max_effects, 256);
        assert_eq!(MAX_EFFECTS, opence_sim::effects::MAX_EFFECTS);
        assert_eq!(layout.frame_decals_offset, 160);
        assert_eq!(layout.decal_size, 128);
        assert_eq!(layout.max_decals, 2048);
        assert_eq!(layout.frame_lights_offset, 152);
        assert_eq!(layout.light_size, 88);
        assert_eq!(layout.max_lights, 512);
        assert_eq!(layout.frame_debug_lines_offset, 128);
        assert_eq!(layout.frame_debug_labels_offset, 136);
        assert_eq!(layout.debug_line_size, 28);
        assert_eq!(layout.debug_label_size, 64);
        assert_eq!(layout.max_debug_lines, 32768);
        assert_eq!(layout.max_debug_labels, 256);
        assert_eq!(layout.frame_objects_offset, 104);
        assert_eq!(layout.frame_nodes_offset, 112);
        assert_eq!(layout.object_size, 108);
        assert_eq!(layout.object_bounding_radius_offset, 80);
        assert_eq!(layout.object_first_node_offset, 12);
        assert_eq!(layout.node_size, 104);
        assert_eq!(layout.max_objects, 4096);
        assert_eq!(layout.max_nodes, 16384);
    }

    fn read(buffers: &SnapshotBuffers) -> &FrameSnapshot {
        // SAFETY: the buffers outlive the borrow and this thread is the
        // only reader.
        unsafe { &*opence_snapshot_acquire(buffers) }
    }

    fn objects(frame: &FrameSnapshot) -> &[ObjectSnapshot] {
        // SAFETY: objects points at object_count entries in the same slot.
        unsafe { std::slice::from_raw_parts(frame.objects, frame.object_count as usize) }
    }

    #[test]
    fn a_read_before_any_publish_is_empty() {
        let (buffers, _writer) = SnapshotBuffers::new();
        let frame = read(&buffers);
        assert_eq!(
            (frame.version, frame.tick, frame.object_count),
            (SNAPSHOT_VERSION, 0, 0)
        );
    }

    #[test]
    fn the_reader_sees_the_newest_publish() {
        let (buffers, mut writer) = SnapshotBuffers::new();
        for tick in 1..=3 {
            writer
                .publish(&buffers, tick, 0.5, |frame| {
                    frame.push_object(ObjectSnapshot {
                        id: tick as u32,
                        ..Default::default()
                    })
                })
                .unwrap_or_else(|_| panic!("one object fits"));
        }
        let frame = read(&buffers);
        assert_eq!(frame.tick, 3);
        assert_eq!(frame.alpha, 0.5);
        assert_eq!(
            objects(frame).iter().map(|o| o.id).collect::<Vec<_>>(),
            vec![3]
        );
    }

    #[test]
    fn a_held_snapshot_is_not_overwritten() {
        let (buffers, mut writer) = SnapshotBuffers::new();
        writer.publish(&buffers, 1, 0.0, |_| ());
        let held = read(&buffers) as *const FrameSnapshot;
        for tick in 2..10 {
            writer.publish(&buffers, tick, 0.0, |_| ());
        }
        // SAFETY: the held slot is still the reader's: it has not acquired
        // again.
        assert_eq!(unsafe { (*held).tick }, 1);
        assert_eq!(read(&buffers).tick, 9);
    }

    #[test]
    fn the_object_list_stops_at_its_capacity() {
        let (buffers, mut writer) = SnapshotBuffers::new();
        let result = writer.publish(&buffers, 1, 0.0, |frame| {
            for _ in 0..MAX_OBJECTS {
                frame.push_object(ObjectSnapshot::default())?;
            }
            frame.push_object(ObjectSnapshot::default())
        });
        assert_eq!(result, Err(SnapshotFull));
        assert_eq!(read(&buffers).object_count as usize, MAX_OBJECTS);
    }

    fn nodes(frame: &FrameSnapshot) -> &[NodeSnapshot] {
        // SAFETY: nodes points at node_count entries in the same slot.
        unsafe { std::slice::from_raw_parts(frame.nodes, frame.node_count as usize) }
    }

    #[test]
    fn an_animated_objects_nodes_follow_each_other() {
        let (buffers, mut writer) = SnapshotBuffers::new();
        let node = |x: f32| NodeSnapshot {
            current: NodeMatrix {
                position: Vec3 { x, y: 0.0, z: 0.0 },
                ..Default::default()
            },
            ..Default::default()
        };
        let result = writer.publish(&buffers, 1, 0.0, |frame| {
            frame.push_animated(ObjectSnapshot::default(), &[node(1.0), node(2.0)])?;
            frame.push_object(ObjectSnapshot::default())?;
            frame.push_animated(ObjectSnapshot::default(), &[node(3.0)])?;
            // (too many nodes: neither the object nor its nodes)
            frame.push_animated(ObjectSnapshot::default(), &vec![node(4.0); MAX_NODES])
        });
        assert_eq!(result, Err(SnapshotFull));
        let frame = read(&buffers);
        let ranges: Vec<_> = objects(frame)
            .iter()
            .map(|o| (o.first_node, o.node_count))
            .collect();
        assert_eq!(ranges, vec![(0, 2), (2, 0), (2, 1)]);
        let xs: Vec<f32> = nodes(frame).iter().map(|n| n.current.position.x).collect();
        assert_eq!(xs, vec![1.0, 2.0, 3.0]);
    }

    #[test]
    fn the_decals_are_copied_up_to_their_capacity_and_cleared() {
        let (buffers, mut writer) = SnapshotBuffers::new();
        let decal = DecalSnapshot {
            modes: [1, 0, DECAL_NO_PICTURE, DECAL_ON_OBJECT],
            ..Default::default()
        };
        writer.publish(&buffers, 1, 0.0, |frame| {
            frame.decals(&vec![decal; MAX_DECALS + 3]);
        });
        let frame = read(&buffers);
        assert_eq!(frame.decal_count as usize, MAX_DECALS);
        // SAFETY: decals points at decal_count entries in the same slot.
        assert_eq!(unsafe { (*frame.decals).modes[3] }, DECAL_ON_OBJECT);
        writer.publish(&buffers, 2, 0.0, |_| {});
        assert_eq!(read(&buffers).decal_count, 0);
    }

    #[test]
    fn the_effects_are_copied_up_to_their_capacity_and_cleared() {
        let (buffers, mut writer) = SnapshotBuffers::new();
        let effect = EffectSnapshot {
            ids: [7, 1, u32::MAX, 0],
            ..Default::default()
        };
        writer.publish(&buffers, 1, 0.0, |frame| {
            frame.effects(&vec![effect; MAX_EFFECTS + 3]);
        });
        let frame = read(&buffers);
        assert_eq!(frame.effect_count as usize, MAX_EFFECTS);
        // SAFETY: effects points at effect_count entries in the same slot.
        assert_eq!(unsafe { (*frame.effects).ids[0] }, 7);
        writer.publish(&buffers, 2, 0.0, |_| {});
        assert_eq!(read(&buffers).effect_count, 0);
    }

    #[test]
    fn the_debug_shapes_are_copied_and_cleared() {
        use opence_dev::draw::{Color, DebugDraw, Layer};
        let (buffers, mut writer) = SnapshotBuffers::new();
        let mut draw = DebugDraw::new();
        draw.line(
            Layer::General,
            [1.0, 2.0, 3.0],
            [4.0, 5.0, 6.0],
            Color::RED,
            1,
        );
        draw.text(Layer::General, [7.0, 8.0, 9.0], "warthog", Color::GREEN, 1);
        writer.publish(&buffers, 1, 0.0, |frame| frame.debug(&draw));
        let frame = read(&buffers);
        assert_eq!((frame.debug_line_count, frame.debug_label_count), (1, 1));
        // SAFETY: the pointers point at the counts' entries in the slot.
        let (line, label) = unsafe { (*frame.debug_lines, *frame.debug_labels) };
        assert_eq!(
            line.to,
            Vec3 {
                x: 4.0,
                y: 5.0,
                z: 6.0
            }
        );
        assert_eq!(line.color, Color::RED.0);
        assert_eq!(&label.text[..8], b"warthog\0");
        assert_eq!(label.at.z, 9.0);
        // (the next frame starts without them)
        writer.publish(&buffers, 2, 0.0, |_| ());
        let frame = read(&buffers);
        assert_eq!((frame.debug_line_count, frame.debug_label_count), (0, 0));
    }

    #[test]
    fn the_lights_are_copied_up_to_their_capacity() {
        let (buffers, mut writer) = SnapshotBuffers::new();
        let light = |radius| LightSnapshot {
            radius,
            ..Default::default()
        };
        writer.publish(&buffers, 1, 0.0, |frame| {
            frame.lights(&vec![light(2.0); MAX_LIGHTS + 3]);
        });
        let frame = read(&buffers);
        assert_eq!(frame.light_count as usize, MAX_LIGHTS);
        // SAFETY: lights points at light_count entries in the same slot.
        assert_eq!(unsafe { (*frame.lights).radius }, 2.0);
        writer.publish(&buffers, 2, 0.0, |_| ());
        assert_eq!(read(&buffers).light_count, 0);
    }

    #[test]
    fn a_null_address_reads_nothing() {
        // SAFETY: null is handled.
        assert!(unsafe { opence_snapshot_acquire(std::ptr::null()) }.is_null());
    }

    #[test]
    fn a_reader_on_another_thread_never_sees_a_torn_frame() {
        let (buffers, mut writer) = SnapshotBuffers::new();
        let buffers = std::sync::Arc::new(buffers);
        let reader = {
            let buffers = buffers.clone();
            std::thread::spawn(move || {
                let mut last = 0;
                while last < 20_000 {
                    let frame = read(&buffers);
                    // every object of a frame carries its tick
                    assert!(objects(frame).iter().all(|o| u64::from(o.id) == frame.tick));
                    assert!(frame.tick >= last);
                    last = frame.tick;
                }
            })
        };
        for tick in 1..=20_000u64 {
            writer.publish(&buffers, tick, 0.0, |frame| {
                for _ in 0..8 {
                    let _ = frame.push_object(ObjectSnapshot {
                        id: tick as u32,
                        ..Default::default()
                    });
                }
            });
        }
        assert!(reader.join().is_ok());
    }
}
