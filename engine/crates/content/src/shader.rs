//! Shaders: what a surface is drawn with.

use rkyv::{Archive, Deserialize, Serialize};

use crate::{Group, SurfaceMaterial, TagRef};

/// A shader.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct Shader {
    /// Its tag.
    pub tag: TagRef,
    /// The physics material type of a surface drawn with it.
    pub material_type: i16,
    /// Its class's fields.
    pub class: ShaderClass,
    /// What the lighting reads of it ([`SurfaceMaterial::of`] its class).
    pub material: SurfaceMaterial,
    /// The light it gives and passes, as the original's radiosity took it.
    pub radiosity: ShaderRadiosity,
}

/// What every shader says of its part in the original's radiosity (the
/// shader's own first 0x28 bytes).
#[derive(Clone, Copy, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct ShaderRadiosity {
    /// Its flags: 0 simple parameterization, 1 ignore normals, 2
    /// transparent lit.
    pub flags: u16,
    /// How much light it gives (0: none).
    pub power: f32,
    /// The colour of the light it gives.
    pub color: [f32; 3],
    /// The tint of the light passing through it.
    pub tint: [f32; 3],
}

impl ShaderRadiosity {
    /// The flag that it lets light through, tinted.
    pub const TRANSPARENT_LIT: u16 = 1 << 2;
}

impl Shader {
    /// A shader of `class`, with its material.
    pub fn new(tag: TagRef, material_type: i16, class: ShaderClass) -> Self {
        Self {
            tag,
            material_type,
            material: SurfaceMaterial::of(&class),
            class,
            radiosity: ShaderRadiosity::default(),
        }
    }
}

/// A shader's class, and that class's fields.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub enum ShaderClass {
    /// The structure's opaque surfaces (`senv`).
    Environment(Box<EnvironmentShader>),
    /// Transparent surfaces drawn in the fixed-function stages (`schi`).
    TransparentChicago(TransparentChicagoShader),
    /// Transparent chicago surfaces with a second set of maps for two
    /// texture stages (`scex`).
    TransparentChicagoExtended(TransparentChicagoShader),
    /// An object's model's surfaces (`soso`).
    Model(ModelShaderClass),
    /// Transparent surfaces drawn by the programmable combiner's stages
    /// (`sotr`).
    TransparentGeneric(TransparentGenericShader),
    /// Glass: a tint, a reflection and a diffuse map (`sgla`).
    Glass(Box<GlassShader>),
    /// A meter's bar (`smet`).
    Meter(MeterShader),
    /// Plasma energy: shields and the like (`spla`).
    Plasma(PlasmaShader),
    /// Water: a tint, a reflection and ripples (`swat`).
    Water(WaterShader),
    /// A group that is not a shader class.
    Other(Group),
}

/// An environment shader (`senv`).
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct EnvironmentShader {
    /// Its flags (alpha tested, bump map is specular mask).
    pub flags: u16,
    /// Normal, blended or blended-base-specular.
    pub kind: i16,
    /// The diffuse flags (rescale detail maps, rescale bump map).
    pub diffuse_flags: u16,
    /// The base map.
    pub base_map: Option<TagRef>,
    /// How the detail maps combine with it.
    pub detail_function: i16,
    /// The primary detail map's scale.
    pub primary_detail_scale: f32,
    /// The primary detail map.
    pub primary_detail_map: Option<TagRef>,
    /// The secondary detail map's scale.
    pub secondary_detail_scale: f32,
    /// The secondary detail map.
    pub secondary_detail_map: Option<TagRef>,
    /// How the micro detail map combines.
    pub micro_detail_function: i16,
    /// The micro detail map's scale.
    pub micro_detail_scale: f32,
    /// The micro detail map.
    pub micro_detail_map: Option<TagRef>,
    /// The material's colour.
    pub material_color: [f32; 3],
    /// The bump map's scale.
    pub bump_scale: f32,
    /// The bump map.
    pub bump_map: Option<TagRef>,
    /// The self-illumination flags (unfiltered).
    pub self_illumination_flags: u16,
    /// The self-illumination map's scale.
    pub self_illumination_scale: f32,
    /// The self-illumination map.
    pub self_illumination_map: Option<TagRef>,
    /// The specular flags (overbright, extra shiny, lightmap is specular).
    pub specular_flags: u16,
    /// The specular brightness.
    pub specular_brightness: f32,
    /// The specular colour, looking along the normal.
    pub specular_perpendicular_color: [f32; 3],
    /// The specular colour, looking across it.
    pub specular_parallel_color: [f32; 3],
    /// The reflection's flags.
    pub reflection_flags: u16,
    /// The reflection's kind (bumped cube map, flat cube map, radiosity).
    pub reflection_kind: i16,
    /// How bright the lightmap makes the reflection.
    pub lightmap_brightness_scale: f32,
    /// The reflection's brightness, looking along the normal.
    pub reflection_perpendicular_brightness: f32,
    /// The reflection's brightness, looking across it.
    pub reflection_parallel_brightness: f32,
    /// The reflection's cube map.
    pub reflection_cube_map: Option<TagRef>,
    /// The self-illumination's colours and the base map's scrolling.
    pub animation: EnvironmentAnimation,
}

/// What animates an environment shader: the self-illumination map's three
/// channels, each a colour between two that a periodic function sweeps,
/// and the maps' scrolling in u and v.
#[derive(Clone, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct EnvironmentAnimation {
    /// The red channel's colour.
    pub primary: AnimatedColor,
    /// The green channel's colour.
    pub secondary: AnimatedColor,
    /// The blue channel's colour, banded by the alpha channel.
    pub plasma: AnimatedColor,
    /// The scrolling in u.
    pub u: TextureAnimation,
    /// The scrolling in v.
    pub v: TextureAnimation,
}

/// A colour swept from `off_color` to `on_color` by a periodic function.
#[derive(Clone, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct AnimatedColor {
    /// The colour at one.
    pub on_color: [f32; 3],
    /// The colour at zero.
    pub off_color: [f32; 3],
    /// The periodic function (`opence_sim::periodic::function`).
    pub function: i16,
    /// Its period in seconds.
    pub period: f32,
    /// Its phase in seconds.
    pub phase: f32,
}

/// A map's scrolling: a periodic function over a period, times a scale.
#[derive(Clone, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
pub struct TextureAnimation {
    /// The periodic function.
    pub function: i16,
    /// Its period in seconds.
    pub period: f32,
    /// How far it scrolls, in map widths.
    pub scale: f32,
    /// Where in its period it starts (0 for environment shaders).
    pub phase: f32,
}

/// A model shader (`soso`): a base map, a multipurpose map whose channels
/// mask the specular reflection, self-illumination, change colour and
/// detail, and a detail map.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct ModelShaderClass {
    /// Its flags (detail after reflection, two-sided, not alpha tested,
    /// alpha-blended decal, true atmospheric fog, disable two-sided culling).
    pub flags: u16,
    /// How translucent it is.
    pub translucency: f32,
    /// Which of the object's change colours tints it (0: none).
    pub change_color_source: i16,
    /// The base map's scale in u and v (0: one).
    pub map_scale: [f32; 2],
    /// The base map.
    pub base_map: Option<TagRef>,
    /// The multipurpose map.
    pub multipurpose_map: Option<TagRef>,
    /// How the detail map combines with the base.
    pub detail_function: i16,
    /// Which of the multipurpose map's channels masks the detail map.
    pub detail_mask: i16,
    /// The detail map's scale in u, and in v when that is zero.
    pub detail_map_scale: f32,
    /// The detail map.
    pub detail_map: Option<TagRef>,
    /// The detail map's scale in v (0: as in u).
    pub detail_map_v_scale: f32,
    /// The colour it glows where its multipurpose map says (the top of
    /// its self-illumination animation's range).
    pub self_illumination_color: [f32; 3],
    /// The bottom of its self-illumination animation's range.
    pub self_illumination_lower: [f32; 3],
    /// Its self-illumination flags (bit 0: no random phase).
    pub self_illumination_flags: u16,
    /// Which of the object's change colours tints its self-illumination
    /// (0: none).
    pub self_illumination_color_source: i16,
    /// The periodic function that animates its self-illumination.
    pub self_illumination_function: i16,
    /// That function's period in seconds.
    pub self_illumination_period: f32,
    /// The maps' scrolling in u.
    pub u_animation: TextureAnimation,
    /// The maps' scrolling in v.
    pub v_animation: TextureAnimation,
    /// The distance at which the reflection is at full strength.
    pub reflection_falloff_distance: f32,
    /// The distance beyond which there is no reflection (0: always).
    pub reflection_cutoff_distance: f32,
    /// The reflection's brightness looking along the normal.
    pub perpendicular_brightness: f32,
    /// The reflection's tint looking along the normal.
    pub perpendicular_tint_color: [f32; 3],
    /// The reflection's brightness looking across the surface.
    pub parallel_brightness: f32,
    /// The reflection's tint looking across the surface.
    pub parallel_tint_color: [f32; 3],
    /// The reflection's cube map.
    pub reflection_cube_map: Option<TagRef>,
}

/// A transparent generic shader (`sotr`): its maps, combined by its
/// stages.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct TransparentGenericShader {
    /// Its flags (as a chicago shader's: alpha tested, decal, two-sided,
    /// first map in screen space).
    pub flags: u8,
    /// How the first map is projected (2D, cube map, ...).
    pub first_map_kind: i16,
    /// How it blends with the frame buffer (as a chicago shader's).
    pub framebuffer_blend_function: i16,
    /// How it fades.
    pub framebuffer_fade_mode: i16,
    /// The shaders drawn over it.
    pub extra_layers: Vec<TagRef>,
    /// Its maps, at most four.
    pub maps: Vec<GenericMap>,
    /// Its stages' constant colours, at most seven.
    pub stages: Vec<GenericStage>,
}

/// A map of a transparent generic shader.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct GenericMap {
    /// Its flags (unfiltered, clamped u, clamped v).
    pub flags: u16,
    /// Its scale in u and v.
    pub scale: [f32; 2],
    /// Its offset in u and v.
    pub offset: [f32; 2],
    /// Its rotation.
    pub rotation: f32,
    /// The bitmap.
    pub map: Option<TagRef>,
}

/// A stage of a transparent generic shader: its two constant colours
/// (alpha, red, green, blue, as the tag has them).
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct GenericStage {
    /// Its flags.
    pub flags: u16,
    /// The first constant colour.
    pub color0: [f32; 4],
    /// The second.
    pub color1: [f32; 4],
}

/// A glass shader (`sgla`): the frame behind it tinted, then a cube map's
/// reflection added, then a diffuse map blended over.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct GlassShader {
    /// Its flags (alpha tested, decal, two-sided, bump map is the
    /// specular mask).
    pub flags: u16,
    /// The colour the frame behind it is multiplied by.
    pub tint_color: [f32; 3],
    /// The tint map's scale.
    pub tint_map_scale: f32,
    /// The tint map.
    pub tint_map: Option<TagRef>,
    /// The reflection's kind: 0 bumped cube map, 1 flat cube map, 2
    /// dynamic mirror.
    pub reflection_kind: i16,
    /// The reflection's colour and brightness looking along the normal
    /// (alpha, red, green, blue).
    pub perpendicular_color: [f32; 4],
    /// The same looking across the surface.
    pub parallel_color: [f32; 4],
    /// The reflection's cube map.
    pub reflection_map: Option<TagRef>,
    /// The bump map's scale.
    pub bump_map_scale: f32,
    /// The bump map.
    pub bump_map: Option<TagRef>,
    /// The diffuse map's scale.
    pub diffuse_map_scale: f32,
    /// The diffuse map.
    pub diffuse_map: Option<TagRef>,
    /// The diffuse detail map's scale.
    pub diffuse_detail_map_scale: f32,
    /// The diffuse detail map.
    pub diffuse_detail_map: Option<TagRef>,
}

/// A meter shader (`smet`): a bar filled to a value, whose map's alpha is
/// where along the bar each texel is.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct MeterShader {
    /// Its flags (decal, two-sided, flash colour is negative, tint mode
    /// 2, unfiltered).
    pub flags: u16,
    /// The map.
    pub map: Option<TagRef>,
    /// The bar's colour at its start.
    pub gradient_min_color: [f32; 3],
    /// Its colour at its end.
    pub gradient_max_color: [f32; 3],
    /// The colour beyond the value.
    pub background_color: [f32; 3],
    /// The colour it flashes.
    pub flash_color: [f32; 3],
    /// The colour the whole is multiplied by.
    pub tint_color: [f32; 3],
    /// The bar's opacity (tint mode 2).
    pub meter_transparency: f32,
    /// The background's opacity (tint mode 2).
    pub background_transparency: f32,
    /// Which of the object's functions is the value (0: none, full).
    pub value_source: i16,
}

/// A plasma shader (`spla`): two scrolling noise maps whose difference
/// glows, brighter across the surface than along its normal.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct PlasmaShader {
    /// Which of the object's functions is its intensity (0: always full).
    pub intensity_source: i16,
    /// The exponent of its intensity.
    pub intensity_exponent: f32,
    /// How far it is pushed out along the normal.
    pub offset_amount: f32,
    /// Its opacity looking along the normal.
    pub perpendicular_alpha: f32,
    /// Its colour looking along the normal.
    pub perpendicular_color: [f32; 3],
    /// Its opacity looking across the surface.
    pub parallel_alpha: f32,
    /// Its colour looking across the surface.
    pub parallel_color: [f32; 3],
    /// Which of the object's change colours tints it (0: none).
    pub color_source: i16,
    /// The first noise map's period, direction and scale.
    pub primary_animation: PlasmaNoise,
    /// The second's.
    pub secondary_animation: PlasmaNoise,
}

/// A plasma shader's noise map and its scrolling.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct PlasmaNoise {
    /// The seconds it takes to scroll once.
    pub period: f32,
    /// The direction it scrolls in.
    pub direction: [f32; 3],
    /// Its scale.
    pub scale: f32,
    /// The map.
    pub map: Option<TagRef>,
}

/// A water shader (`swat`).
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct WaterShader {
    /// Its flags (the base map's alpha masks the reflection, its colour
    /// tints what is behind, atmospheric fog, drawn before the fog).
    pub flags: u16,
    /// The base map.
    pub base_map: Option<TagRef>,
    /// The reflection's tint looking along the normal (alpha, red,
    /// green, blue).
    pub perpendicular_tint: [f32; 4],
    /// The same looking across the surface.
    pub parallel_tint: [f32; 4],
    /// The reflection's cube map.
    pub reflection_map: Option<TagRef>,
    /// The direction the ripples move in, in degrees.
    pub ripple_angle: f32,
    /// How fast they move.
    pub ripple_velocity: f32,
    /// Their scale.
    pub ripple_scale: f32,
    /// The ripples' maps.
    pub ripple_maps: Option<TagRef>,
}

/// A transparent chicago shader (`schi`, `scex`).
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct TransparentChicagoShader {
    /// The numeric counter's limit.
    pub numeric_counter_limit: u8,
    /// Its flags (alpha tested, decal, two-sided, first map in screen space).
    pub flags: u8,
    /// How the first map is projected (2D, cube map, ...).
    pub first_map_kind: i16,
    /// How it blends with the frame buffer.
    pub framebuffer_blend_function: i16,
    /// How it fades.
    pub framebuffer_fade_mode: i16,
    /// What fades it.
    pub framebuffer_fade_source: i16,
    /// The shaders drawn over it.
    pub extra_layers: Vec<TagRef>,
    /// Its maps, one for each texture stage.
    pub maps: Vec<ChicagoMap>,
    /// The maps used when only two texture stages are (`scex` only).
    pub two_stage_maps: Vec<ChicagoMap>,
    /// The extended shader's flags (`scex` only).
    pub extra_flags: u32,
}

/// A map of a chicago shader.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct ChicagoMap {
    /// Its flags (unfiltered, alpha replicate, clamped u and v).
    pub flags: u16,
    /// How its colour combines with the stage before.
    pub color_function: i16,
    /// How its alpha combines with the stage before.
    pub alpha_function: i16,
    /// Its scale in u and v.
    pub scale: [f32; 2],
    /// Its offset in u and v.
    pub offset: [f32; 2],
    /// Its rotation.
    pub rotation: f32,
    /// Its mipmap bias.
    pub mipmap_bias: f32,
    /// The bitmap.
    pub map: Option<TagRef>,
}
