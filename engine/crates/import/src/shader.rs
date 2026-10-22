//! Shaders: the environment, model and transparent chicago classes, with
//! their maps; the other classes by group alone, for now.

use opence_content::{
    AnimatedColor, ChicagoMap, EnvironmentAnimation, EnvironmentShader, Group, ModelShaderClass,
    Shader, ShaderClass, SurfaceMaterial, TextureAnimation, TransparentChicagoShader,
};

use crate::cache::Cache;
use crate::error::ImportError;
use crate::level::{tag_ref, Reached, SHDR};
use crate::tags::{Element, GroupTag};

pub(crate) const SENV: GroupTag = GroupTag::new(b"senv");
pub(crate) const SCHI: GroupTag = GroupTag::new(b"schi");
pub(crate) const SCEX: GroupTag = GroupTag::new(b"scex");
pub(crate) const SOSO: GroupTag = GroupTag::new(b"soso");

pub(crate) const SHADER_BASE_SIZE: usize = 0x28;
pub(crate) const MATERIAL_TYPE_OFFSET: usize = 0x22;
pub(crate) const ENVIRONMENT_SIZE: usize = 0x344;
/// What of a model shader is read: up to its reflection's cube map.
pub(crate) const MODEL_SIZE: usize = 0x174;
pub(crate) const CHICAGO_SIZE: usize = 0x6C;
pub(crate) const CHICAGO_EXTENDED_SIZE: usize = 0x78;
pub(crate) const CHICAGO_EXTRA_LAYERS_OFFSET: usize = 0x48;
pub(crate) const CHICAGO_MAPS_OFFSET: usize = 0x54;
pub(crate) const CHICAGO_TWO_STAGE_MAPS_OFFSET: usize = 0x60;
pub(crate) const CHICAGO_EXTRA_FLAGS_OFFSET: usize = 0x6C;
pub(crate) const CHICAGO_MAP_SIZE: usize = 0xDC;
pub(crate) const CHICAGO_MAP_BITMAP_OFFSET: usize = 0x6C;
const MAX_CHICAGO_MAPS: usize = 4;
const MAX_EXTRA_LAYERS: usize = 4;

/// Reads a shader of any class, noting the shaders and bitmaps it reaches.
pub(crate) fn import_shader(
    cache: &Cache,
    index: usize,
    reached: &mut Reached,
) -> Result<Shader, ImportError> {
    let group = cache.tags()[index].group;
    let size = match group {
        SENV => ENVIRONMENT_SIZE,
        SCHI => CHICAGO_SIZE,
        SCEX => CHICAGO_EXTENDED_SIZE,
        SOSO => MODEL_SIZE,
        _ => SHADER_BASE_SIZE,
    };
    let shader = cache.tag(index, SHDR, size)?;
    let class = match group {
        SENV => ShaderClass::Environment(Box::new(environment(&shader, reached)?)),
        SCHI => ShaderClass::TransparentChicago(chicago(&shader, reached, false)?),
        SCEX => ShaderClass::TransparentChicagoExtended(chicago(&shader, reached, true)?),
        SOSO => ShaderClass::Model(model(&shader, reached)?),
        _ => ShaderClass::Other(Group(group.0.to_be_bytes())),
    };
    Ok(Shader {
        tag: tag_ref(cache, index),
        material_type: shader.i16(MATERIAL_TYPE_OFFSET, "material type")?,
        material: SurfaceMaterial::of(&class),
        class,
    })
}

fn color(element: &Element<'_>, at: usize, field: &str) -> Result<[f32; 3], ImportError> {
    Ok([
        element.f32(at, field)?,
        element.f32(at + 4, field)?,
        element.f32(at + 8, field)?,
    ])
}

/// A model shader's fields (`shader_model_properties`, after the shader's
/// own 0x28 bytes; the offsets the decompilation's
/// `rasterizer_xbox_models.c` checks).
fn model(shader: &Element<'_>, reached: &mut Reached) -> Result<ModelShaderClass, ImportError> {
    Ok(ModelShaderClass {
        flags: shader.u16(0x28, "flags")?,
        translucency: shader.f32(0x38, "translucency")?,
        change_color_source: shader.i16(0x4C, "change color source")?,
        map_scale: [
            shader.f32(0x9C, "map u scale")?,
            shader.f32(0xA0, "map v scale")?,
        ],
        base_map: reached.bitmap(shader, 0xA4, "base map")?,
        multipurpose_map: reached.bitmap(shader, 0xBC, "multipurpose map")?,
        detail_function: shader.i16(0xD4, "detail function")?,
        detail_mask: shader.i16(0xD6, "detail mask")?,
        detail_map_scale: shader.f32(0xD8, "detail map scale")?,
        detail_map: reached.bitmap(shader, 0xDC, "detail map")?,
        detail_map_v_scale: shader.f32(0xEC, "detail map v scale")?,
        self_illumination_color: color(shader, 0x84, "self-illumination upper bound")?,
        self_illumination_lower: color(shader, 0x78, "self-illumination lower bound")?,
        self_illumination_flags: shader.u16(0x6C, "self-illumination flags")?,
        self_illumination_color_source: shader.i16(0x70, "self-illumination color source")?,
        self_illumination_function: shader.i16(0x72, "self-illumination function")?,
        self_illumination_period: shader.f32(0x74, "self-illumination period")?,
        u_animation: model_texture_animation(shader, 0xFC, "u animation")?,
        v_animation: model_texture_animation(shader, 0x110, "v animation")?,
        reflection_falloff_distance: shader.f32(0x13C, "reflection falloff distance")?,
        reflection_cutoff_distance: shader.f32(0x140, "reflection cutoff distance")?,
        perpendicular_brightness: shader.f32(0x144, "perpendicular brightness")?,
        perpendicular_tint_color: color(shader, 0x148, "perpendicular tint color")?,
        parallel_brightness: shader.f32(0x154, "parallel brightness")?,
        parallel_tint_color: color(shader, 0x158, "parallel tint color")?,
        reflection_cube_map: reached.bitmap(shader, 0x164, "reflection cube map")?,
    })
}

fn environment(
    shader: &Element<'_>,
    reached: &mut Reached,
) -> Result<EnvironmentShader, ImportError> {
    Ok(EnvironmentShader {
        flags: shader.u16(0x28, "flags")?,
        kind: shader.i16(0x2A, "type")?,
        diffuse_flags: shader.u16(0x6C, "diffuse flags")?,
        base_map: reached.bitmap(shader, 0x88, "base map")?,
        detail_function: shader.i16(0xB0, "detail map function")?,
        primary_detail_scale: shader.f32(0xB4, "primary detail map scale")?,
        primary_detail_map: reached.bitmap(shader, 0xB8, "primary detail map")?,
        secondary_detail_scale: shader.f32(0xC8, "secondary detail map scale")?,
        secondary_detail_map: reached.bitmap(shader, 0xCC, "secondary detail map")?,
        micro_detail_function: shader.i16(0xF4, "micro detail map function")?,
        micro_detail_scale: shader.f32(0xF8, "micro detail map scale")?,
        micro_detail_map: reached.bitmap(shader, 0xFC, "micro detail map")?,
        material_color: color(shader, 0x10C, "material color")?,
        bump_scale: shader.f32(0x124, "bump map scale")?,
        bump_map: reached.bitmap(shader, 0x128, "bump map")?,
        self_illumination_flags: shader.u16(0x180, "self-illumination flags")?,
        self_illumination_scale: shader.f32(0x250, "self-illumination map scale")?,
        self_illumination_map: reached.bitmap(shader, 0x254, "self-illumination map")?,
        specular_flags: shader.u16(0x27C, "specular flags")?,
        specular_brightness: shader.f32(0x290, "specular brightness")?,
        specular_perpendicular_color: color(shader, 0x2A8, "specular perpendicular color")?,
        specular_parallel_color: color(shader, 0x2B4, "specular parallel color")?,
        reflection_flags: shader.u16(0x2D0, "reflection flags")?,
        reflection_kind: shader.i16(0x2D2, "reflection type")?,
        lightmap_brightness_scale: shader.f32(0x2D4, "lightmap brightness scale")?,
        reflection_perpendicular_brightness: shader
            .f32(0x2F4, "reflection perpendicular brightness")?,
        reflection_parallel_brightness: shader.f32(0x2F8, "reflection parallel brightness")?,
        reflection_cube_map: reached.bitmap(shader, 0x324, "reflection cube map")?,
        animation: EnvironmentAnimation {
            primary: animated_color(shader, 0x19C, "primary")?,
            secondary: animated_color(shader, 0x1D8, "secondary")?,
            plasma: animated_color(shader, 0x214, "plasma")?,
            u: texture_animation(shader, 0x150, "u animation")?,
            v: texture_animation(shader, 0x15C, "v animation")?,
        },
    })
}

/// A self-illumination colour: on and off colours, then a function, a
/// period and a phase.
fn animated_color(
    shader: &Element<'_>,
    at: usize,
    field: &str,
) -> Result<AnimatedColor, ImportError> {
    Ok(AnimatedColor {
        on_color: color(shader, at, field)?,
        off_color: color(shader, at + 0xC, field)?,
        function: shader.i16(at + 0x18, field)?,
        period: shader.f32(at + 0x1C, field)?,
        phase: shader.f32(at + 0x20, field)?,
    })
}

/// A map's scrolling: a function, a period and a scale.
fn texture_animation(
    shader: &Element<'_>,
    at: usize,
    field: &str,
) -> Result<TextureAnimation, ImportError> {
    Ok(TextureAnimation {
        function: shader.i16(at, field)?,
        period: shader.f32(at + 4, field)?,
        scale: shader.f32(at + 8, field)?,
        phase: 0.0,
    })
}

/// A model shader's scrolling (`shader_texture_animation_function`): an
/// object function that scales it (not read: objects have none yet), a
/// periodic function, a period, a phase and a scale.
fn model_texture_animation(
    shader: &Element<'_>,
    at: usize,
    field: &str,
) -> Result<TextureAnimation, ImportError> {
    Ok(TextureAnimation {
        function: shader.i16(at + 2, field)?,
        period: shader.f32(at + 4, field)?,
        phase: shader.f32(at + 8, field)?,
        scale: shader.f32(at + 0xC, field)?,
    })
}

fn chicago(
    shader: &Element<'_>,
    reached: &mut Reached,
    extended: bool,
) -> Result<TransparentChicagoShader, ImportError> {
    let layers = shader.block(
        CHICAGO_EXTRA_LAYERS_OFFSET,
        "extra layers",
        crate::tags::REFERENCE_SIZE,
        MAX_EXTRA_LAYERS,
    )?;
    let mut extra_layers = Vec::with_capacity(layers.len());
    for layer in layers.iter() {
        if let Some(tag) = reached.shader(&layer, 0, "shader")? {
            extra_layers.push(tag);
        }
    }
    let maps = chicago_maps(shader, CHICAGO_MAPS_OFFSET, "maps", reached)?;
    let (two_stage_maps, extra_flags) = if extended {
        (
            chicago_maps(
                shader,
                CHICAGO_TWO_STAGE_MAPS_OFFSET,
                "2-stage maps",
                reached,
            )?,
            shader.u32(CHICAGO_EXTRA_FLAGS_OFFSET, "extra flags")?,
        )
    } else {
        (Vec::new(), 0)
    };
    Ok(TransparentChicagoShader {
        numeric_counter_limit: shader.u8(0x28, "numeric counter limit")?,
        flags: shader.u8(0x29, "flags")?,
        first_map_kind: shader.i16(0x2A, "first map type")?,
        framebuffer_blend_function: shader.i16(0x2C, "framebuffer blend function")?,
        framebuffer_fade_mode: shader.i16(0x2E, "framebuffer fade mode")?,
        framebuffer_fade_source: shader.i16(0x30, "framebuffer fade source")?,
        extra_layers,
        maps,
        two_stage_maps,
        extra_flags,
    })
}

fn chicago_maps(
    shader: &Element<'_>,
    at: usize,
    field: &str,
    reached: &mut Reached,
) -> Result<Vec<ChicagoMap>, ImportError> {
    let block = shader.block(at, field, CHICAGO_MAP_SIZE, MAX_CHICAGO_MAPS)?;
    let mut maps = Vec::with_capacity(block.len());
    for map in block.iter() {
        maps.push(ChicagoMap {
            flags: map.u16(0, "flags")?,
            color_function: map.i16(0x2C, "color function")?,
            alpha_function: map.i16(0x2E, "alpha function")?,
            scale: [map.f32(0x54, "map u-scale")?, map.f32(0x58, "map v-scale")?],
            offset: [
                map.f32(0x5C, "map u-offset")?,
                map.f32(0x60, "map v-offset")?,
            ],
            rotation: map.f32(0x64, "map rotation")?,
            mipmap_bias: map.f32(0x68, "mipmap bias")?,
            map: reached.bitmap(&map, CHICAGO_MAP_BITMAP_OFFSET, "map")?,
        });
    }
    Ok(maps)
}
