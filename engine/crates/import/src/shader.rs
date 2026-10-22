//! Shaders of every class, with their maps.

use opence_content::{
    AnimatedColor, ChicagoMap, EnvironmentAnimation, EnvironmentShader, GenericMap, GenericStage,
    GlassShader, Group, MeterShader, ModelShaderClass, PlasmaNoise, PlasmaShader, Shader,
    ShaderClass, ShaderRadiosity, SurfaceMaterial, TextureAnimation, TransparentChicagoShader,
    TransparentGenericShader, WaterShader,
};

use crate::cache::Cache;
use crate::error::ImportError;
use crate::level::{tag_ref, Reached, SHDR};
use crate::tags::{Element, GroupTag};

pub(crate) const SENV: GroupTag = GroupTag::new(b"senv");
pub(crate) const SCHI: GroupTag = GroupTag::new(b"schi");
pub(crate) const SCEX: GroupTag = GroupTag::new(b"scex");
pub(crate) const SOSO: GroupTag = GroupTag::new(b"soso");
pub(crate) const SOTR: GroupTag = GroupTag::new(b"sotr");
pub(crate) const SGLA: GroupTag = GroupTag::new(b"sgla");
pub(crate) const SMET: GroupTag = GroupTag::new(b"smet");
pub(crate) const SPLA: GroupTag = GroupTag::new(b"spla");
pub(crate) const SWAT: GroupTag = GroupTag::new(b"swat");

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
/// What of each class is read (the offsets the decompilation's
/// rasterizer checks).
pub(crate) const GENERIC_SIZE: usize = 0x6C;
pub(crate) const GENERIC_MAP_SIZE: usize = 0x64;
pub(crate) const GENERIC_STAGE_SIZE: usize = 0x70;
pub(crate) const GLASS_SIZE: usize = 0x17C;
pub(crate) const METER_SIZE: usize = 0xE4;
pub(crate) const PLASMA_SIZE: usize = 0x12C;
pub(crate) const WATER_SIZE: usize = 0xE4;
const MAX_GENERIC_STAGES: usize = 7;
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
        SOTR => GENERIC_SIZE,
        SGLA => GLASS_SIZE,
        SMET => METER_SIZE,
        SPLA => PLASMA_SIZE,
        SWAT => WATER_SIZE,
        _ => SHADER_BASE_SIZE,
    };
    let shader = cache.tag(index, SHDR, size)?;
    let class = match group {
        SENV => ShaderClass::Environment(Box::new(environment(&shader, reached)?)),
        SCHI => ShaderClass::TransparentChicago(chicago(&shader, reached, false)?),
        SCEX => ShaderClass::TransparentChicagoExtended(chicago(&shader, reached, true)?),
        SOSO => ShaderClass::Model(model(&shader, reached)?),
        SOTR => ShaderClass::TransparentGeneric(generic(&shader, reached)?),
        SGLA => ShaderClass::Glass(Box::new(glass(&shader, reached)?)),
        SMET => ShaderClass::Meter(meter(&shader, reached)?),
        SPLA => ShaderClass::Plasma(plasma(&shader, reached)?),
        SWAT => ShaderClass::Water(water(&shader, reached)?),
        _ => ShaderClass::Other(Group(group.0.to_be_bytes())),
    };
    Ok(Shader {
        tag: tag_ref(cache, index),
        material_type: shader.i16(MATERIAL_TYPE_OFFSET, "material type")?,
        material: SurfaceMaterial::of(&class),
        class,
        radiosity: ShaderRadiosity {
            flags: shader.u16(0x00, "flags")?,
            power: shader.f32(0x04, "power")?,
            color: color(&shader, 0x08, "color of emitted light")?,
            tint: color(&shader, 0x14, "tint color")?,
        },
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

fn argb(element: &Element<'_>, at: usize, field: &str) -> Result<[f32; 4], ImportError> {
    Ok([
        element.f32(at, field)?,
        element.f32(at + 4, field)?,
        element.f32(at + 8, field)?,
        element.f32(at + 12, field)?,
    ])
}

fn extra_layers(
    shader: &Element<'_>,
    reached: &mut Reached,
) -> Result<Vec<opence_content::TagRef>, ImportError> {
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
    Ok(extra_layers)
}

/// A transparent generic shader (`shader_transparent_generic`).
fn generic(
    shader: &Element<'_>,
    reached: &mut Reached,
) -> Result<TransparentGenericShader, ImportError> {
    let block = shader.block(0x54, "maps", GENERIC_MAP_SIZE, MAX_CHICAGO_MAPS)?;
    let mut maps = Vec::with_capacity(block.len());
    for map in block.iter() {
        maps.push(GenericMap {
            flags: map.u16(0, "flags")?,
            scale: [map.f32(4, "map u-scale")?, map.f32(8, "map v-scale")?],
            offset: [
                map.f32(0xC, "map u-offset")?,
                map.f32(0x10, "map v-offset")?,
            ],
            rotation: map.f32(0x14, "map rotation")?,
            map: reached.bitmap(&map, 0x1C, "map")?,
        });
    }
    let block = shader.block(0x60, "stages", GENERIC_STAGE_SIZE, MAX_GENERIC_STAGES)?;
    let mut stages = Vec::with_capacity(block.len());
    for stage in block.iter() {
        stages.push(GenericStage {
            flags: stage.u16(0, "flags")?,
            color0: argb(&stage, 0xC, "color0")?,
            color1: argb(&stage, 0x1C, "color1")?,
        });
    }
    Ok(TransparentGenericShader {
        flags: shader.u8(0x29, "flags")?,
        first_map_kind: shader.i16(0x2A, "first map type")?,
        framebuffer_blend_function: shader.i16(0x2C, "framebuffer blend function")?,
        framebuffer_fade_mode: shader.i16(0x2E, "framebuffer fade mode")?,
        extra_layers: extra_layers(shader, reached)?,
        maps,
        stages,
    })
}

/// A glass shader (`shader_transparent_glass`).
fn glass(shader: &Element<'_>, reached: &mut Reached) -> Result<GlassShader, ImportError> {
    Ok(GlassShader {
        flags: shader.u16(0x28, "flags")?,
        tint_color: color(shader, 0x54, "background tint color")?,
        tint_map_scale: shader.f32(0x60, "background tint map scale")?,
        tint_map: reached.bitmap(shader, 0x64, "background tint map")?,
        reflection_kind: shader.i16(0x8A, "reflection type")?,
        perpendicular_color: argb(shader, 0x8C, "perpendicular color")?,
        parallel_color: argb(shader, 0x9C, "parallel color")?,
        reflection_map: reached.bitmap(shader, 0xAC, "reflection map")?,
        bump_map_scale: shader.f32(0xBC, "bump map scale")?,
        bump_map: reached.bitmap(shader, 0xC0, "bump map")?,
        diffuse_map_scale: shader.f32(0x154, "diffuse map scale")?,
        diffuse_map: reached.bitmap(shader, 0x158, "diffuse map")?,
        diffuse_detail_map_scale: shader.f32(0x168, "diffuse detail map scale")?,
        diffuse_detail_map: reached.bitmap(shader, 0x16C, "diffuse detail map")?,
    })
}

/// A meter shader (`shader_transparent_meter`).
fn meter(shader: &Element<'_>, reached: &mut Reached) -> Result<MeterShader, ImportError> {
    Ok(MeterShader {
        flags: shader.u16(0x28, "flags")?,
        map: reached.bitmap(shader, 0x4C, "map")?,
        gradient_min_color: color(shader, 0x7C, "gradient min color")?,
        gradient_max_color: color(shader, 0x88, "gradient max color")?,
        background_color: color(shader, 0x94, "background color")?,
        flash_color: color(shader, 0xA0, "flash color")?,
        tint_color: color(shader, 0xAC, "tint color")?,
        meter_transparency: shader.f32(0xB8, "meter transparency")?,
        background_transparency: shader.f32(0xBC, "background transparency")?,
        value_source: shader.i16(0xDC, "value source")?,
    })
}

fn plasma_noise(
    shader: &Element<'_>,
    at: usize,
    reached: &mut Reached,
) -> Result<PlasmaNoise, ImportError> {
    Ok(PlasmaNoise {
        period: shader.f32(at, "noise map animation period")?,
        direction: color(shader, at + 4, "noise map animation direction")?,
        scale: shader.f32(at + 0x10, "noise map scale")?,
        map: reached.bitmap(shader, at + 0x14, "noise map")?,
    })
}

/// A plasma shader (`shader_transparent_plasma`).
fn plasma(shader: &Element<'_>, reached: &mut Reached) -> Result<PlasmaShader, ImportError> {
    Ok(PlasmaShader {
        intensity_source: shader.i16(0x2C, "intensity source")?,
        intensity_exponent: shader.f32(0x30, "intensity exponent")?,
        offset_amount: shader.f32(0x38, "offset amount")?,
        perpendicular_alpha: shader.f32(0x60, "perpendicular alpha")?,
        perpendicular_color: color(shader, 0x64, "perpendicular tint color")?,
        parallel_alpha: shader.f32(0x70, "parallel alpha")?,
        parallel_color: color(shader, 0x74, "parallel tint color")?,
        color_source: shader.i16(0x80, "tint color source")?,
        primary_animation: plasma_noise(shader, 0xC0, reached)?,
        secondary_animation: plasma_noise(shader, 0x108, reached)?,
    })
}

/// A water shader (`shader_transparent_water`).
fn water(shader: &Element<'_>, reached: &mut Reached) -> Result<WaterShader, ImportError> {
    Ok(WaterShader {
        flags: shader.u16(0x28, "flags")?,
        base_map: reached.bitmap(shader, 0x4C, "base map")?,
        perpendicular_tint: argb(shader, 0x6C, "view perpendicular tint")?,
        parallel_tint: argb(shader, 0x7C, "view parallel tint")?,
        reflection_map: reached.bitmap(shader, 0x9C, "reflection map")?,
        ripple_angle: shader.f32(0xBC, "ripple animation angle")?,
        ripple_velocity: shader.f32(0xC0, "ripple animation velocity")?,
        ripple_scale: shader.f32(0xC4, "ripple scale")?,
        ripple_maps: reached.bitmap(shader, 0xC8, "ripple maps")?,
    })
}

fn chicago(
    shader: &Element<'_>,
    reached: &mut Reached,
    extended: bool,
) -> Result<TransparentChicagoShader, ImportError> {
    let extra_layers = extra_layers(shader, reached)?;
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

#[cfg(test)]
mod tests {
    use super::*;
    use crate::cache::CacheKind;
    use crate::fake::FakeCache;
    use crate::level::tests::{block, empty, put, reference};
    use crate::level::BITM;

    fn f32s(bytes: &mut [u8], at: usize, values: &[f32]) {
        for (index, value) in values.iter().enumerate() {
            put(bytes, at + index * 4, &value.to_le_bytes());
        }
    }

    /// A cache of one bitmap and one shader of `group`, made by `fill`
    /// (given the bitmap's handle), imported.
    fn imported(
        group: GroupTag,
        size: usize,
        fill: impl Fn(&mut FakeCache, &mut Vec<u8>, u32),
    ) -> (Shader, Reached) {
        let mut fake = FakeCache::new(CacheKind::Xbox);
        let bitmap = fake.add_tag(BITM, "fake\\bitmap", &[0; 4]);
        let handle = fake.handle(bitmap);
        let mut bytes = vec![0u8; size];
        fill(&mut fake, &mut bytes, handle);
        let shader = fake.add_tag(group, "fake\\shader", &bytes);
        fake.set_parent(shader, SHDR);
        let cache = match Cache::open(fake.build()) {
            Ok(cache) => cache,
            Err(error) => panic!("{error}"),
        };
        let mut reached = Reached::default();
        match import_shader(&cache, shader, &mut reached) {
            Ok(shader) => (shader, reached),
            Err(error) => panic!("{error}"),
        }
    }

    fn bitmap() -> Option<opence_content::TagRef> {
        Some(opence_content::TagRef {
            group: Group(*b"bitm"),
            path: "fake\\bitmap".to_owned(),
        })
    }

    #[test]
    fn a_generic_shader_reads_its_maps_and_stages() {
        let (shader, reached) = imported(SOTR, GENERIC_SIZE, |fake, bytes, handle| {
            let mut map = vec![0u8; GENERIC_MAP_SIZE];
            put(&mut map, 0, &1u16.to_le_bytes());
            f32s(&mut map, 4, &[2.0, 3.0, 0.25, 0.5, 90.0]);
            reference(&mut map, 0x1C, BITM, handle);
            let maps = fake.add_data(&map);
            let mut stage = vec![0u8; GENERIC_STAGE_SIZE];
            f32s(&mut stage, 0xC, &[1.0, 0.5, 0.25, 0.125]);
            f32s(&mut stage, 0x1C, &[0.0, 1.0, 1.0, 1.0]);
            let stages = fake.add_data(&stage);
            bytes[0x29] = 0b101;
            put(bytes, 0x2C, &3i16.to_le_bytes());
            // it emits light for the radiosity: power 2, orange, tinted
            put(bytes, 0, &4u16.to_le_bytes());
            f32s(bytes, 4, &[2.0, 1.0, 0.5, 0.0, 0.25, 0.5, 0.75]);
            block(bytes, 0x54, 1, maps);
            block(bytes, 0x60, 1, stages);
        });
        let ShaderClass::TransparentGeneric(generic) = &shader.class else {
            panic!("{:?}", shader.class)
        };
        assert_eq!(generic.flags, 0b101);
        assert_eq!(generic.framebuffer_blend_function, 3);
        assert_eq!(
            shader.radiosity,
            ShaderRadiosity {
                flags: ShaderRadiosity::TRANSPARENT_LIT,
                power: 2.0,
                color: [1.0, 0.5, 0.0],
                tint: [0.25, 0.5, 0.75],
            }
        );
        assert_eq!(
            generic.maps,
            [GenericMap {
                flags: 1,
                scale: [2.0, 3.0],
                offset: [0.25, 0.5],
                rotation: 90.0,
                map: bitmap(),
            }]
        );
        assert_eq!(generic.stages[0].color0, [1.0, 0.5, 0.25, 0.125]);
        assert_eq!(generic.stages[0].color1, [0.0, 1.0, 1.0, 1.0]);
        assert_eq!(shader.material.albedo_map, bitmap());
        assert_eq!(reached.bitmaps.len(), 1);
    }

    #[test]
    fn glass_reads_its_tint_reflection_bump_and_diffuse() {
        let (shader, _) = imported(SGLA, GLASS_SIZE, |_, bytes, handle| {
            put(bytes, 0x28, &4u16.to_le_bytes());
            f32s(bytes, 0x54, &[0.5, 0.25, 1.0, 2.0]);
            empty(bytes, 0x64);
            put(bytes, 0x8A, &1i16.to_le_bytes());
            f32s(bytes, 0x8C, &[1.0, 0.5, 0.5, 0.5, 0.25, 1.0, 1.0, 1.0]);
            reference(bytes, 0xAC, BITM, handle);
            f32s(bytes, 0xBC, &[3.0]);
            reference(bytes, 0xC0, BITM, handle);
            f32s(bytes, 0x154, &[4.0]);
            reference(bytes, 0x158, BITM, handle);
            f32s(bytes, 0x168, &[5.0]);
            empty(bytes, 0x16C);
        });
        let ShaderClass::Glass(glass) = &shader.class else {
            panic!("{:?}", shader.class)
        };
        assert_eq!(glass.flags, 4);
        assert_eq!(glass.tint_color, [0.5, 0.25, 1.0]);
        assert_eq!(glass.tint_map_scale, 2.0);
        assert_eq!(glass.tint_map, None);
        assert_eq!(glass.reflection_kind, 1);
        assert_eq!(glass.perpendicular_color, [1.0, 0.5, 0.5, 0.5]);
        assert_eq!(glass.parallel_color, [0.25, 1.0, 1.0, 1.0]);
        assert_eq!(glass.reflection_map, bitmap());
        assert_eq!(
            (glass.bump_map_scale, glass.bump_map.clone()),
            (3.0, bitmap())
        );
        assert_eq!(
            (glass.diffuse_map_scale, glass.diffuse_map.clone()),
            (4.0, bitmap())
        );
        assert_eq!(
            (
                glass.diffuse_detail_map_scale,
                glass.diffuse_detail_map.clone()
            ),
            (5.0, None)
        );
    }

    #[test]
    fn meters_plasma_and_water_read_their_fields() {
        let (shader, _) = imported(SMET, METER_SIZE, |_, bytes, handle| {
            put(bytes, 0x28, &3u16.to_le_bytes());
            reference(bytes, 0x4C, BITM, handle);
            f32s(bytes, 0x7C, &[0.0, 0.0, 1.0, 1.0, 0.0, 0.0]);
            f32s(bytes, 0xAC, &[0.5, 0.5, 0.5, 0.75, 0.25]);
            put(bytes, 0xDC, &2i16.to_le_bytes());
        });
        let ShaderClass::Meter(meter) = &shader.class else {
            panic!("{:?}", shader.class)
        };
        assert_eq!((meter.flags, meter.map.clone()), (3, bitmap()));
        assert_eq!(meter.gradient_min_color, [0.0, 0.0, 1.0]);
        assert_eq!(meter.gradient_max_color, [1.0, 0.0, 0.0]);
        assert_eq!(meter.tint_color, [0.5; 3]);
        assert_eq!(
            (meter.meter_transparency, meter.background_transparency),
            (0.75, 0.25)
        );
        assert_eq!(meter.value_source, 2);

        let (shader, reached) = imported(SPLA, PLASMA_SIZE, |_, bytes, handle| {
            put(bytes, 0x2C, &1i16.to_le_bytes());
            f32s(bytes, 0x30, &[2.0]);
            f32s(bytes, 0x60, &[0.25, 1.0, 0.0, 0.0, 0.5, 0.0, 1.0, 0.0]);
            f32s(bytes, 0xC0, &[3.0, 1.0, 0.0, 0.0, 4.0]);
            reference(bytes, 0xD4, BITM, handle);
            f32s(bytes, 0x108, &[5.0, 0.0, 1.0, 0.0, 6.0]);
            empty(bytes, 0x11C);
        });
        let ShaderClass::Plasma(plasma) = &shader.class else {
            panic!("{:?}", shader.class)
        };
        assert_eq!(
            (plasma.intensity_source, plasma.intensity_exponent),
            (1, 2.0)
        );
        assert_eq!(plasma.perpendicular_alpha, 0.25);
        assert_eq!(plasma.perpendicular_color, [1.0, 0.0, 0.0]);
        assert_eq!(plasma.parallel_alpha, 0.5);
        assert_eq!(plasma.parallel_color, [0.0, 1.0, 0.0]);
        assert_eq!(
            plasma.primary_animation,
            PlasmaNoise {
                period: 3.0,
                direction: [1.0, 0.0, 0.0],
                scale: 4.0,
                map: bitmap()
            }
        );
        assert_eq!(
            plasma.secondary_animation,
            PlasmaNoise {
                period: 5.0,
                direction: [0.0, 1.0, 0.0],
                scale: 6.0,
                map: None
            }
        );
        assert_eq!(reached.bitmaps.len(), 1);

        let (shader, _) = imported(SWAT, WATER_SIZE, |_, bytes, handle| {
            put(bytes, 0x28, &2u16.to_le_bytes());
            reference(bytes, 0x4C, BITM, handle);
            f32s(bytes, 0x6C, &[1.0, 0.0, 0.5, 1.0, 0.5, 1.0, 1.0, 1.0]);
            reference(bytes, 0x9C, BITM, handle);
            f32s(bytes, 0xBC, &[45.0, 2.0, 8.0]);
            reference(bytes, 0xC8, BITM, handle);
        });
        let ShaderClass::Water(water) = &shader.class else {
            panic!("{:?}", shader.class)
        };
        assert_eq!((water.flags, water.base_map.clone()), (2, bitmap()));
        assert_eq!(water.perpendicular_tint, [1.0, 0.0, 0.5, 1.0]);
        assert_eq!(water.parallel_tint, [0.5, 1.0, 1.0, 1.0]);
        assert_eq!(water.reflection_map, bitmap());
        assert_eq!(
            (
                water.ripple_angle,
                water.ripple_velocity,
                water.ripple_scale
            ),
            (45.0, 2.0, 8.0)
        );
        assert_eq!(water.ripple_maps, bitmap());
    }
}
