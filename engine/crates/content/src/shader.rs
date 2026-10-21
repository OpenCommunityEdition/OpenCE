//! Shaders: what a surface is drawn with.

use crate::{Group, TagRef};

/// A shader.
#[derive(Clone, Debug, PartialEq)]
pub struct Shader {
    /// Its tag.
    pub tag: TagRef,
    /// The physics material type of a surface drawn with it.
    pub material_type: i16,
    /// Its class's fields.
    pub class: ShaderClass,
}

/// A shader's class, and that class's fields.
#[derive(Clone, Debug, PartialEq)]
pub enum ShaderClass {
    /// The structure's opaque surfaces (`senv`).
    Environment(Box<EnvironmentShader>),
    /// Transparent surfaces drawn in the fixed-function stages (`schi`).
    TransparentChicago(TransparentChicagoShader),
    /// Transparent chicago surfaces with a second set of maps for two
    /// texture stages (`scex`).
    TransparentChicagoExtended(TransparentChicagoShader),
    /// A class not imported yet (`soso`, `sotr`, `sgla`, `smet`, `spla`,
    /// `swat`).
    Other(Group),
}

/// An environment shader (`senv`).
#[derive(Clone, Debug, PartialEq)]
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
    /// The self-illumination map's scale.
    pub self_illumination_scale: f32,
    /// The self-illumination map.
    pub self_illumination_map: Option<TagRef>,
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
}

/// A transparent chicago shader (`schi`, `scex`).
#[derive(Clone, Debug, PartialEq)]
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
#[derive(Clone, Debug, PartialEq)]
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
