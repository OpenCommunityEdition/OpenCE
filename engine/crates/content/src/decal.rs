//! Decals (`deca`): pictures laid on the level's surfaces, placed by the
//! level (its BSPs' runtime decals) or by impacts and explosions.

use rkyv::{Archive, Deserialize, Serialize};

use crate::TagRef;

/// A decal: the picture, how large, how long it lasts and how it is
/// blended over what is under it.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct Decal {
    /// Its tag.
    pub tag: TagRef,
    /// Its flags ([`Decal::GEOMETRY_INHERITED`] and the rest).
    pub flags: u16,
    /// What it is: 0 a scratch, 1 a splatter, 2 a burn, 3 a painted sign.
    pub kind: i16,
    /// Its layer ([`Decal::PRIMARY`] and the rest): the layers are drawn
    /// in order.
    pub layer: i16,
    /// The decal made with it, under or over it.
    pub next: Option<TagRef>,
    /// Its radius, least and greatest, in world units.
    pub radius: [f32; 2],
    /// Its brightness (the alpha of its colour), least and greatest.
    pub intensity: [f32; 2],
    /// The colours its colour is chosen between.
    pub color: [[f32; 3]; 2],
    /// The frame a looping animation starts again from.
    pub animation_loop_frame: i16,
    /// The ticks each frame of its animation lasts.
    pub animation_speed: i16,
    /// How long it lasts, least and greatest, in seconds (0: for ever).
    pub lifetime: [f32; 2],
    /// How long it fades over at the end of its life, least and greatest.
    pub decay_time: [f32; 2],
    /// How it is blended: 0 alpha blend, 1 multiply, 2 double multiply,
    /// 3 add, 4 subtract, 5 least, 6 greatest, 7 alpha multiply add.
    pub framebuffer_blend_function: i16,
    /// Its picture.
    pub map: Option<TagRef>,
    /// The longest side of its bitmap's sprites, in texels (worked out by
    /// the tools): a sprite of that size is the decal's radius.
    pub maximum_sprite_extent: f32,
}

impl Decal {
    /// The decal after it in the chain is laid as this one is.
    pub const GEOMETRY_INHERITED: u16 = 1 << 0;
    /// Its colour is interpolated by hue, saturation and value.
    pub const COLOR_HSV: u16 = 1 << 1;
    /// ... the long way round the hues.
    pub const COLOR_MORE_COLORS: u16 = 1 << 2;
    /// It is not turned at random (it faces the way it was thrown).
    pub const NO_RANDOM_ROTATION: u16 = 1 << 3;
    /// It is made only on water.
    pub const WATER_EFFECT: u16 = 1 << 4;
    /// Its animation loops.
    pub const ANIMATION_LOOP: u16 = 1 << 7;
    /// It keeps its picture's shape.
    pub const PRESERVE_ASPECT: u16 = 1 << 8;

    /// The first layer.
    pub const PRIMARY: i16 = 0;
    /// The layer over it.
    pub const SECONDARY: i16 = 1;
    /// Light (drawn over the others).
    pub const LIGHT: i16 = 2;
    /// Alpha tested: drawn where the picture is more than half opaque.
    pub const ALPHA_TESTED: i16 = 3;
    /// Water's.
    pub const WATER: i16 = 4;
}

/// A decal a structure BSP places (`structure_runtime_decal`): laid where
/// a ray from its position along its direction meets the level.
#[derive(Clone, Copy, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct LevelDecal {
    /// Where the ray starts.
    pub position: [f32; 3],
    /// Its decal, an index into the scenario's decal palette.
    pub palette_index: u8,
    /// The ray's yaw, in units of a 127th of a half turn.
    pub yaw: i8,
    /// Its pitch, in units of a 254th of a half turn.
    pub pitch: i8,
}
