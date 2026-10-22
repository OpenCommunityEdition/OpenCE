//! Lights (`ligh`): the dynamic lights objects carry at their markers and
//! effects make, and the functions (an object's outgoing values) that
//! scale them.

use rkyv::{Archive, Deserialize, Serialize};

use crate::TagRef;

/// A light, as its tag defines it. Angles are in radians, times in ticks.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct Light {
    /// Its tag.
    pub tag: TagRef,
    /// Its flags ([`Light::DYNAMIC`] and the rest).
    pub flags: u32,
    /// How far it reaches at its least and greatest intensity, as the
    /// radius modifiers' bounds scale it.
    pub radius: f32,
    /// The radius's scale at intensity 0 and at 1.
    pub radius_modifier: [f32; 2],
    /// The angle from its forward where its cone starts to fall off.
    pub falloff_angle: f32,
    /// The angle from its forward where its cone ends (π: it shines all
    /// round).
    pub cutoff_angle: f32,
    /// The radius its lens flare is seen within.
    pub lens_flare_radius: f32,
    /// The cosine of the falloff angle (worked out by the tools).
    pub cosine_falloff_angle: f32,
    /// The cosine of the cutoff angle (worked out by the tools).
    pub cosine_cutoff_angle: f32,
    /// How much farther than its radius its highlights reach.
    pub specular_radius_multiplier: f32,
    /// The sine of the cutoff angle (worked out by the tools).
    pub sine_cutoff_angle: f32,
    /// How its colour is interpolated ([`Light::HSV`],
    /// [`Light::MORE_COLORS`]).
    pub color_flags: u32,
    /// Its colour at intensity 0, alpha first: alpha says how much of it
    /// a change colour leaves alone.
    pub color_lower: [f32; 4],
    /// Its colour at intensity 1, alpha first.
    pub color_upper: [f32; 4],
    /// The picture its light is projected through.
    pub gel: Option<TagRef>,
    /// Its lens flare.
    pub lens_flare: Option<TagRef>,
    /// How long an effect's light lasts, in ticks.
    pub duration: f32,
    /// The transition function an effect's light fades by.
    pub falloff_function: i16,
}

impl Light {
    /// It lights what is around it (not only a lens flare).
    pub const DYNAMIC: u32 = 1 << 0;
    /// It makes no highlights.
    pub const NO_SPECULAR: u32 = 1 << 1;
    /// It does not light the object it is attached to.
    pub const DONT_LIGHT_OWN_OBJECT: u32 = 1 << 2;
    /// It is larger in the first-person view.
    pub const SUPERSIZE_IN_FIRST_PERSON: u32 = 1 << 3;
    /// It is the first-person view's flashlight.
    pub const FIRST_PERSON_FLASHLIGHT: u32 = 1 << 4;
    /// It does not fade with its unit's active camouflage.
    pub const DONT_FADE_ACTIVE_CAMOUFLAGE: u32 = 1 << 5;
    /// Its colours are interpolated by hue, saturation and value.
    pub const HSV: u32 = 1 << 0;
    /// The hue goes the long way round.
    pub const MORE_COLORS: u32 = 1 << 1;
}

/// One of an object's functions: a value computed each tick from time
/// and the object's incoming values, which its lights, shaders and
/// attachments are scaled by. An index of another value is 0 for none,
/// 1 to 4 for an incoming value (A to D), 5 to 8 for an outgoing one.
#[derive(Clone, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct ObjectFunction {
    /// Its flags ([`ObjectFunction::INVERT`] and the rest).
    pub flags: u32,
    /// One over its period, in seconds (worked out by the tools).
    pub one_over_period: f32,
    /// The value its period is divided by.
    pub scale_period_by: i16,
    /// Its periodic function.
    pub function: i16,
    /// The value its function is multiplied by.
    pub scale_function_by: i16,
    /// The periodic function it wobbles by.
    pub wobble_function: i16,
    /// The wobble's period, in seconds.
    pub wobble_period: f32,
    /// How far it wobbles.
    pub wobble_magnitude: f32,
    /// Above this it is 1, below 0 (0: no square wave).
    pub square_wave_threshold: f32,
    /// The steps it is cut into (fewer than 2: none).
    pub step_count: i16,
    /// The transition function its result is mapped by.
    pub map_to: i16,
    /// The teeth of its sawtooth (fewer than 2: none).
    pub sawtooth_count: i16,
    /// The value added to it.
    pub add: i16,
    /// The value its result is multiplied by.
    pub scale_result_by: i16,
    /// How its bounds are applied: 0 clip, 1 clip and normalize, 2
    /// scale to fit.
    pub bounds_mode: i16,
    /// Its least and greatest values.
    pub bounds: [f32; 2],
    /// The function (0 to 3) that turns it off when off (-1: none).
    pub turn_off_with: i16,
    /// What its result is multiplied by (0: nothing).
    pub scale_by: f32,
    /// One over its bounds' range (worked out by the tools).
    pub reciprocal_bounds_range: f32,
    /// One over its sawtooth count (worked out by the tools).
    pub reciprocal_sawtooth_count: f32,
    /// One over its step count (worked out by the tools).
    pub reciprocal_step_count: f32,
}

impl ObjectFunction {
    /// Its function's value is one less it.
    pub const INVERT: u32 = 1 << 0;
    /// It adds to its last value, wrapping at 1.
    pub const ADDITIVE: u32 = 1 << 1;
    /// It stays on below its lower bound.
    pub const ALWAYS_ACTIVE: u32 = 1 << 2;
}

/// Where an object's four incoming values come from: for each kind of
/// part it has, the mode that part exports into each value (0: none).
/// The object's are exported first, then its type's, whose override.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq, Archive, Serialize, Deserialize)]
pub struct FunctionModes {
    /// The object's own (body vitality and the rest).
    pub object: [i16; 4],
    /// A unit's (bipeds and vehicles: seat power, integrated light power
    /// and the rest).
    pub unit: [i16; 4],
    /// A biped's (flying speed).
    pub biped: [i16; 4],
    /// A weapon's (heat, ammunition, rate of fire and the rest).
    pub weapon: [i16; 4],
}
