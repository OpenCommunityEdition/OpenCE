//! What the camera sees of lights: lens flares (`lens`), at the level's
//! markers, at lights and at the sky's lights; and the widgets objects
//! carry at their markers, light volumes (`mgs2`) and glows (`glw!`).
//! Angles are in radians, colours alpha last.

use rkyv::{Archive, Deserialize, Serialize};

use crate::TagRef;

/// A lens flare: the corona and the reflections drawn where a light is
/// seen, faded by distance and by how much of it is hidden.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct LensFlare {
    /// Its tag.
    pub tag: TagRef,
    /// The cosine of the angle from its forward where it starts to fade
    /// (worked out by the tools).
    pub cosine_falloff_angle: f32,
    /// The cosine of the angle where it is gone.
    pub cosine_cutoff_angle: f32,
    /// The radius of the disk its occlusion test samples, in world units.
    pub occlusion_radius: f32,
    /// Where the disk is moved: 0 towards the viewer, 1 along the
    /// marker's forward, 2 nowhere.
    pub occlusion_offset_direction: i16,
    /// The distance it is at its brightest within.
    pub near_fade_distance: f32,
    /// The distance it is gone beyond (0: it does not fade).
    pub far_fade_distance: f32,
    /// The bitmap its reflections are images of.
    pub bitmap: Option<TagRef>,
    /// Its flags ([`LensFlare::SUN`]).
    pub flags: u16,
    /// The function the corona turns by: 0 none, 1 by the screen's
    /// position, 2 by the screen's angle.
    pub corona_rotation_function: i16,
    /// What that function is multiplied by, in degrees.
    pub corona_rotation_function_scale: f32,
    /// The corona's scale, across and up.
    pub corona_radius_scale: [f32; 2],
    /// Its reflections; the first is the corona.
    pub reflections: Vec<LensFlareReflection>,
}

impl LensFlare {
    /// It is the sun's.
    pub const SUN: u16 = 1 << 0;
}

/// One image of a lens flare, along the line through the screen's centre.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct LensFlareReflection {
    /// Its flags ([`LensFlareReflection::ROTATE_FROM_CENTER`] and the
    /// rest).
    pub flags: u16,
    /// Its image in the flare's bitmap.
    pub bitmap_index: i16,
    /// How far along the line it is: 0 at the light, 1 at the screen's
    /// centre, 2 opposite.
    pub offset: f32,
    /// Its turn, in degrees.
    pub rotation_offset: f32,
    /// Its radius at the light's least and greatest intensity.
    pub radius: [f32; 2],
    /// Its brightness at the least and greatest intensity.
    pub brightness: [f32; 2],
    /// The angle that scales its brightness: 0 none, 1 the light's
    /// direction, 2 from the light to the camera, 3 the camera's.
    pub brightness_scale_function: i16,
    /// Its tint (all zero: the light's colour).
    pub tint: [f32; 4],
    /// The colours it is animated between.
    pub animation_color: [[f32; 4]; 2],
    /// How they are interpolated (the colour flags of [`crate::Light`]).
    pub animation_flags: u16,
    /// The periodic function it is animated by (0 and 1: not animated).
    pub animation_function: i16,
    /// The animation's period, in seconds.
    pub animation_period: f32,
    /// The animation's phase, in seconds.
    pub animation_phase: f32,
}

impl LensFlareReflection {
    /// It turns with its angle round the screen's centre.
    pub const ROTATE_FROM_CENTER: u16 = 1 << 0;
    /// Its radius is in screen units (it is the same size at any
    /// distance).
    pub const RADIUS_NOT_SCALED_BY_DISTANCE: u16 = 1 << 1;
    /// It shrinks as the light is hidden.
    pub const RADIUS_SCALED_BY_OCCLUSION: u16 = 1 << 2;
    /// It is hidden by the first-person weapon.
    pub const ZBUFFER: u16 = 1 << 3;
}

/// A lens flare placed in a structure BSP.
#[derive(Clone, Copy, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct LensFlareMarker {
    /// Where it is.
    pub position: [f32; 3],
    /// The way it faces (a unit vector).
    pub direction: [f32; 3],
    /// Its flare, in the BSP's flares.
    pub lens_flare: u8,
}

/// A light volume: a row of sprites along a marker's forward, like a
/// beam of light, faded as it is seen end on or far off.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct LightVolume {
    /// Its tag.
    pub tag: TagRef,
    /// The marker it is drawn at.
    pub marker: String,
    /// The distance it is at its brightest within.
    pub near_fade_distance: f32,
    /// The distance it is gone beyond (0: it does not fade).
    pub far_fade_distance: f32,
    /// Its brightness seen side on.
    pub perpendicular_brightness_scale: f32,
    /// Its brightness seen end on.
    pub parallel_brightness_scale: f32,
    /// The object's function that scales it (0: none, 1 to 4: A to D).
    pub brightness_scale_source: i16,
    /// The bitmap of its sprites.
    pub bitmap: Option<TagRef>,
    /// The sequence of that bitmap it uses.
    pub sequence_index: i16,
    /// How many sprites it is drawn with.
    pub count: i16,
    /// Its shape (the first frame is used).
    pub frames: Vec<LightVolumeFrame>,
}

/// The shape of a light volume: from its first sprite (hither) to its
/// last (yon), each value interpolated by its exponent.
#[derive(Clone, Copy, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct LightVolumeFrame {
    /// How far from the marker it starts.
    pub offset_from_marker: f32,
    /// The exponent its sprites are spaced by.
    pub offset_exponent: f32,
    /// How long it is.
    pub length: f32,
    /// The radius of its first and last sprites.
    pub radius: [f32; 2],
    /// The exponent the radius is interpolated by.
    pub radius_exponent: f32,
    /// The colour of its first and last sprites.
    pub color: [[f32; 4]; 2],
    /// The exponent the colour is interpolated by.
    pub color_exponent: f32,
    /// The exponent the brightness (alpha) is interpolated by.
    pub brightness_exponent: f32,
}

/// A glow: particles turning round the line through the markers of one
/// name.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct Glow {
    /// Its tag.
    pub tag: TagRef,
    /// The markers it is drawn along.
    pub marker: String,
    /// How many particles it has.
    pub number_of_particles: i16,
    /// Its flags.
    pub flags: u32,
    /// How fast the particles turn round the line, in radians a second.
    pub effect_rotational_velocity: f32,
    /// How far the particles are from the line, least and greatest.
    pub distance_to_object: [f32; 2],
    /// The particles' radius, least and greatest.
    pub particle_size: [f32; 2],
    /// Their colour, from the least to the greatest.
    pub color: [[f32; 4]; 2],
    /// The share of the line, at either end, the particles fade over.
    pub percentage_edge_fade: f32,
    /// The particles' picture.
    pub texture: Option<TagRef>,
}
