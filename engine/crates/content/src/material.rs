//! A surface's material: what the renderer lights, taken from a shader of
//! any class. The shader's own fields stay as the tag has them; this is
//! the one record the lighting reads, in linear colour.
//!
//! The game's shaders are not physically based: an environment or model
//! shader has a diffuse map, a bump map and a cube-map reflection whose
//! brightness and tint are given looking along the surface's normal and
//! across it. Those two become the specular colour at normal and at
//! grazing incidence (a tinted specular takes the place of metalness), and
//! the reflection's kind gives a roughness.

use rkyv::{Archive, Deserialize, Serialize};

use crate::{EnvironmentShader, ModelShaderClass, ShaderClass, TagRef, TransparentChicagoShader};

/// A surface's material.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct SurfaceMaterial {
    /// The diffuse colour's map.
    pub albedo_map: Option<TagRef>,
    /// What the diffuse map is multiplied by (linear).
    pub albedo: [f32; 3],
    /// The bump map, whose heights give the normals.
    pub normal_map: Option<TagRef>,
    /// The bump map's scale in u and v (one: as the base map).
    pub normal_map_scale: f32,
    /// How rough the surface is, from 0 (a mirror) to 1 (no reflection).
    pub roughness: f32,
    /// The specular colour looking along the normal (linear).
    pub specular: [f32; 3],
    /// The specular colour looking across the surface (linear).
    pub specular_grazing: [f32; 3],
    /// The cube map the game reflected in it.
    pub reflection_map: Option<TagRef>,
    /// The map of where it glows.
    pub emission_map: Option<TagRef>,
    /// The colour it glows (linear; black: none).
    pub emission: [f32; 3],
    /// [`SurfaceMaterial::ALPHA_TESTED`] and the other flags.
    pub flags: u16,
}

/// The roughness of a surface with a reflection whose normals come from a
/// bump map (the game's bumped cube map).
const BUMPED_ROUGHNESS: f32 = 0.3;
/// The roughness of a surface with a flat cube-map reflection.
const FLAT_ROUGHNESS: f32 = 0.15;
/// The roughness of a surface whose reflection is the lightmap's light
/// (the game's radiosity reflection).
const RADIOSITY_ROUGHNESS: f32 = 0.6;

impl SurfaceMaterial {
    /// Cut out where the albedo map's alpha is under a half.
    pub const ALPHA_TESTED: u16 = 1 << 0;
    /// Blended with what is behind it.
    pub const BLENDED: u16 = 1 << 1;
    /// Drawn from both sides.
    pub const TWO_SIDED: u16 = 1 << 2;
    /// Drawn over the surface it lies on.
    pub const DECAL: u16 = 1 << 3;
    /// Not lit: its maps are its colour.
    pub const UNLIT: u16 = 1 << 4;

    /// A material that is only a colour map, lit, with no reflection.
    pub fn diffuse(albedo_map: Option<TagRef>, flags: u16) -> Self {
        Self {
            albedo_map,
            albedo: [1.0; 3],
            normal_map: None,
            normal_map_scale: 1.0,
            roughness: 1.0,
            specular: [0.0; 3],
            specular_grazing: [0.0; 3],
            reflection_map: None,
            emission_map: None,
            emission: [0.0; 3],
            flags,
        }
    }

    /// The material of a shader of `class`.
    pub fn of(class: &ShaderClass) -> Self {
        match class {
            ShaderClass::Environment(shader) => environment(shader),
            ShaderClass::Model(shader) => model(shader),
            ShaderClass::TransparentChicago(shader)
            | ShaderClass::TransparentChicagoExtended(shader) => chicago(shader),
            // (classes not imported yet are transparent effects: glass,
            // meters, plasma, water and the generic transparent shader)
            ShaderClass::Other(_) => Self::diffuse(None, Self::BLENDED | Self::UNLIT),
        }
    }

    /// Whether `flag` is set.
    pub fn has(&self, flag: u16) -> bool {
        self.flags & flag != 0
    }
}

/// A display value (the tags' colours are as the screen shows them) as a
/// linear one, by the sRGB curve.
pub fn linear(display: f32) -> f32 {
    let display = if display.is_finite() {
        display.clamp(0.0, 1.0)
    } else {
        0.0
    };
    if display <= 0.04045 {
        display / 12.92
    } else {
        libm::powf((display + 0.055) / 1.055, 2.4)
    }
}

/// A linear value as a display one: the inverse of [`linear`].
pub fn display(linear: f32) -> f32 {
    let linear = if linear.is_finite() {
        linear.clamp(0.0, 1.0)
    } else {
        0.0
    };
    if linear <= 0.003_130_8 {
        linear * 12.92
    } else {
        1.055 * libm::powf(linear, 1.0 / 2.4) - 0.055
    }
}

/// A display colour as a linear one.
pub fn linear_color(display: [f32; 3]) -> [f32; 3] {
    display.map(linear)
}

/// A brightness as a factor: finite and not negative.
fn brightness(value: f32) -> f32 {
    if value.is_finite() {
        value.max(0.0)
    } else {
        0.0
    }
}

/// A map scale: zero (unset) or not finite is one.
fn map_scale(value: f32) -> f32 {
    if value.is_finite() && value != 0.0 {
        value
    } else {
        1.0
    }
}

fn scaled(color: [f32; 3], by: f32) -> [f32; 3] {
    linear_color(color).map(|channel| channel * brightness(by))
}

fn environment(shader: &EnvironmentShader) -> SurfaceMaterial {
    let reflected = shader.reflection_cube_map.is_some();
    // (the reflection kinds: 0 bumped cube map, 1 flat cube map, 2
    // radiosity)
    let roughness = match shader.reflection_kind {
        _ if !reflected && shader.reflection_kind != 2 => 1.0,
        0 => BUMPED_ROUGHNESS,
        1 => FLAT_ROUGHNESS,
        _ => RADIOSITY_ROUGHNESS,
    };
    let (specular, specular_grazing) = if roughness < 1.0 {
        (
            scaled(
                shader.specular_perpendicular_color,
                shader.reflection_perpendicular_brightness,
            ),
            scaled(
                shader.specular_parallel_color,
                shader.reflection_parallel_brightness,
            ),
        )
    } else {
        ([0.0; 3], [0.0; 3])
    };
    SurfaceMaterial {
        albedo_map: shader.base_map.clone(),
        // (the original's final combiner multiplies the lightmap by the
        // material colour before the base map multiplies the frame)
        albedo: if shader
            .material_color
            .iter()
            .all(|channel| channel.is_finite())
        {
            linear_color(shader.material_color)
        } else {
            [1.0; 3]
        },
        normal_map: shader.bump_map.clone(),
        normal_map_scale: map_scale(shader.bump_scale),
        roughness,
        specular,
        specular_grazing,
        reflection_map: shader.reflection_cube_map.clone(),
        emission_map: shader.self_illumination_map.clone(),
        emission: if shader.self_illumination_map.is_some() {
            [1.0; 3]
        } else {
            [0.0; 3]
        },
        flags: if shader.flags & 1 != 0 {
            SurfaceMaterial::ALPHA_TESTED
        } else {
            0
        },
    }
}

fn model(shader: &ModelShaderClass) -> SurfaceMaterial {
    let reflected = shader.reflection_cube_map.is_some();
    // (the model flags: 1 two-sided, 2 not alpha tested, 3 alpha-blended
    // decal)
    let mut flags = 0;
    if shader.flags & (1 << 1) != 0 {
        flags |= SurfaceMaterial::TWO_SIDED;
    }
    if shader.flags & (1 << 3) != 0 {
        flags |= SurfaceMaterial::BLENDED | SurfaceMaterial::DECAL;
    } else if shader.flags & (1 << 2) == 0 {
        flags |= SurfaceMaterial::ALPHA_TESTED;
    }
    SurfaceMaterial {
        albedo_map: shader.base_map.clone(),
        albedo: [1.0; 3],
        normal_map: None,
        normal_map_scale: 1.0,
        roughness: if reflected { BUMPED_ROUGHNESS } else { 1.0 },
        specular: if reflected {
            scaled(
                shader.perpendicular_tint_color,
                shader.perpendicular_brightness,
            )
        } else {
            [0.0; 3]
        },
        specular_grazing: if reflected {
            scaled(shader.parallel_tint_color, shader.parallel_brightness)
        } else {
            [0.0; 3]
        },
        reflection_map: shader.reflection_cube_map.clone(),
        // (the multipurpose map's self-illumination channel masks it)
        emission_map: shader.multipurpose_map.clone(),
        emission: linear_color(shader.self_illumination_color),
        flags,
    }
}

fn chicago(shader: &TransparentChicagoShader) -> SurfaceMaterial {
    // (the chicago flags: 0 alpha tested, 1 decal, 2 two-sided)
    let mut flags = SurfaceMaterial::BLENDED | SurfaceMaterial::UNLIT;
    for (bit, flag) in [
        (0, SurfaceMaterial::ALPHA_TESTED),
        (1, SurfaceMaterial::DECAL),
        (2, SurfaceMaterial::TWO_SIDED),
    ] {
        if shader.flags & (1 << bit) != 0 {
            flags |= flag;
        }
    }
    SurfaceMaterial::diffuse(shader.maps.first().and_then(|map| map.map.clone()), flags)
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{ChicagoMap, Group, TextureAnimation};

    fn tag(group: &[u8; 4], path: &str) -> TagRef {
        TagRef {
            group: Group(*group),
            path: path.into(),
        }
    }

    fn close(a: f32, b: f32) -> bool {
        (a - b).abs() < 1e-4
    }

    fn environment_shader() -> EnvironmentShader {
        EnvironmentShader {
            flags: 0,
            kind: 0,
            diffuse_flags: 0,
            base_map: Some(tag(b"bitm", "base")),
            detail_function: 0,
            primary_detail_scale: 0.0,
            primary_detail_map: None,
            secondary_detail_scale: 0.0,
            secondary_detail_map: None,
            micro_detail_function: 0,
            micro_detail_scale: 0.0,
            micro_detail_map: None,
            material_color: [0.5; 3],
            bump_scale: 0.0,
            bump_map: None,
            self_illumination_flags: 0,
            self_illumination_scale: 0.0,
            self_illumination_map: None,
            specular_flags: 0,
            specular_brightness: 1.0,
            specular_perpendicular_color: [1.0, 0.5, 0.0],
            specular_parallel_color: [1.0; 3],
            reflection_flags: 0,
            reflection_kind: 0,
            lightmap_brightness_scale: 1.0,
            reflection_perpendicular_brightness: 0.5,
            reflection_parallel_brightness: 1.0,
            reflection_cube_map: None,
            animation: crate::EnvironmentAnimation::default(),
        }
    }

    fn model_shader() -> ModelShaderClass {
        ModelShaderClass {
            flags: 0,
            translucency: 0.0,
            change_color_source: 0,
            map_scale: [0.0; 2],
            base_map: Some(tag(b"bitm", "base")),
            multipurpose_map: Some(tag(b"bitm", "multipurpose")),
            detail_function: 0,
            detail_mask: 0,
            detail_map_scale: 0.0,
            detail_map: None,
            detail_map_v_scale: 0.0,
            self_illumination_color: [0.0; 3],
            self_illumination_lower: [0.0; 3],
            self_illumination_flags: 0,
            self_illumination_color_source: 0,
            self_illumination_function: 0,
            self_illumination_period: 0.0,
            u_animation: TextureAnimation::default(),
            v_animation: TextureAnimation::default(),
            reflection_falloff_distance: 0.0,
            reflection_cutoff_distance: 0.0,
            perpendicular_brightness: 0.25,
            perpendicular_tint_color: [1.0; 3],
            parallel_brightness: 1.0,
            parallel_tint_color: [0.5; 3],
            reflection_cube_map: None,
        }
    }

    #[test]
    fn display_values_become_linear_by_the_srgb_curve() {
        assert_eq!(linear(0.0), 0.0);
        assert!(close(linear(1.0), 1.0));
        assert!(close(linear(0.5), 0.214_041));
        assert!(close(linear(0.04), 0.04 / 12.92));
        assert_eq!(linear(-1.0), 0.0);
        assert_eq!(linear(f32::NAN), 0.0);
        assert!(close(linear(2.0), 1.0));
        for step in 0..=20 {
            let value = step as f32 / 20.0;
            assert!(close(display(linear(value)), value), "{value}");
        }
        assert_eq!(display(f32::INFINITY), 0.0);
        assert!(close(display(0.001), 0.012_92));
    }

    #[test]
    fn an_environment_shader_with_a_broken_material_colour_is_white() {
        let mut shader = environment_shader();
        shader.material_color = [f32::NAN, 0.5, 0.5];
        let material = SurfaceMaterial::of(&ShaderClass::Environment(Box::new(shader)));
        assert_eq!(material.albedo, [1.0; 3]);
    }

    #[test]
    fn an_environment_shader_without_a_reflection_is_diffuse() {
        let material =
            SurfaceMaterial::of(&ShaderClass::Environment(Box::new(environment_shader())));
        assert_eq!(material.albedo_map, Some(tag(b"bitm", "base")));
        // (the material colour, 0.5 for display, in linear)
        assert!(material
            .albedo
            .iter()
            .all(|&channel| close(channel, 0.214_041)));
        assert_eq!(material.roughness, 1.0);
        assert_eq!(material.specular, [0.0; 3]);
        assert_eq!(material.normal_map_scale, 1.0);
        assert_eq!(material.emission, [0.0; 3]);
        assert_eq!(material.flags, 0);
    }

    #[test]
    fn an_environment_shader_reflection_becomes_its_specular() {
        let mut shader = environment_shader();
        shader.flags = 1;
        shader.bump_map = Some(tag(b"bitm", "bump"));
        shader.bump_scale = 4.0;
        shader.self_illumination_map = Some(tag(b"bitm", "glow"));
        shader.reflection_cube_map = Some(tag(b"bitm", "cube"));
        let material = SurfaceMaterial::of(&ShaderClass::Environment(Box::new(shader.clone())));
        assert_eq!(material.roughness, BUMPED_ROUGHNESS);
        assert!(close(material.specular[0], 0.5));
        assert!(close(material.specular[1], 0.5 * 0.214_041));
        assert_eq!(material.specular[2], 0.0);
        assert!(close(material.specular_grazing[1], 1.0));
        assert_eq!(material.normal_map, Some(tag(b"bitm", "bump")));
        assert_eq!(material.normal_map_scale, 4.0);
        assert_eq!(material.reflection_map, Some(tag(b"bitm", "cube")));
        assert_eq!(material.emission, [1.0; 3]);
        assert!(material.has(SurfaceMaterial::ALPHA_TESTED));
        assert!(!material.has(SurfaceMaterial::BLENDED));

        shader.reflection_kind = 1;
        assert_eq!(
            SurfaceMaterial::of(&ShaderClass::Environment(Box::new(shader.clone()))).roughness,
            FLAT_ROUGHNESS
        );
        // (a radiosity reflection needs no cube map)
        shader.reflection_kind = 2;
        shader.reflection_cube_map = None;
        shader.reflection_perpendicular_brightness = f32::NAN;
        let radiosity = SurfaceMaterial::of(&ShaderClass::Environment(Box::new(shader)));
        assert_eq!(radiosity.roughness, RADIOSITY_ROUGHNESS);
        assert_eq!(radiosity.specular, [0.0; 3]);
    }

    #[test]
    fn a_model_shader_takes_its_reflection_flags_and_glow() {
        let plain = SurfaceMaterial::of(&ShaderClass::Model(model_shader()));
        assert_eq!(plain.roughness, 1.0);
        assert_eq!(plain.specular_grazing, [0.0; 3]);
        assert_eq!(plain.flags, SurfaceMaterial::ALPHA_TESTED);
        assert_eq!(plain.emission_map, Some(tag(b"bitm", "multipurpose")));

        let mut shader = model_shader();
        shader.flags = (1 << 1) | (1 << 2);
        shader.self_illumination_color = [1.0, 0.5, 0.0];
        shader.reflection_cube_map = Some(tag(b"bitm", "cube"));
        let shiny = SurfaceMaterial::of(&ShaderClass::Model(shader.clone()));
        assert_eq!(shiny.flags, SurfaceMaterial::TWO_SIDED);
        assert_eq!(shiny.roughness, BUMPED_ROUGHNESS);
        assert!(close(shiny.specular[0], 0.25));
        assert!(close(shiny.specular_grazing[0], 0.214_041));
        assert!(close(shiny.emission[1], 0.214_041));

        shader.flags = 1 << 3;
        let decal = SurfaceMaterial::of(&ShaderClass::Model(shader));
        assert_eq!(
            decal.flags,
            SurfaceMaterial::BLENDED | SurfaceMaterial::DECAL
        );
    }

    #[test]
    fn transparent_shaders_are_blended_and_unlit() {
        let map = ChicagoMap {
            flags: 0,
            color_function: 0,
            alpha_function: 0,
            scale: [1.0; 2],
            offset: [0.0; 2],
            rotation: 0.0,
            mipmap_bias: 0.0,
            map: Some(tag(b"bitm", "first")),
        };
        let chicago = TransparentChicagoShader {
            numeric_counter_limit: 0,
            flags: 0b111,
            first_map_kind: 0,
            framebuffer_blend_function: 0,
            framebuffer_fade_mode: 0,
            framebuffer_fade_source: 0,
            extra_layers: Vec::new(),
            maps: vec![map],
            two_stage_maps: Vec::new(),
            extra_flags: 0,
        };
        let material = SurfaceMaterial::of(&ShaderClass::TransparentChicagoExtended(chicago));
        assert_eq!(material.albedo_map, Some(tag(b"bitm", "first")));
        assert_eq!(material.flags, 0b11111);

        let other = SurfaceMaterial::of(&ShaderClass::Other(Group(*b"sgla")));
        assert_eq!(
            other.flags,
            SurfaceMaterial::BLENDED | SurfaceMaterial::UNLIT
        );
        assert_eq!(other.albedo_map, None);
    }
}
