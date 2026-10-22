//! The transparent shader classes made ready for the GPU: the generic
//! transparent shader as chicago maps, and glass, meters, plasma and water
//! as a [`RenderTransparent`] each, drawn with one blend that multiplies
//! the frame by one colour and adds another (the original drew them in
//! two or three passes, each of which this blend can express in one).

use opence_content::{
    AnimatedColor, EnvironmentAnimation, GlassShader, MeterShader, PlasmaNoise, PlasmaShader,
    TagRef, TextureAnimation, TransparentGenericShader, WaterShader,
};
use opence_sim::periodic::function;

use crate::render_level::{
    finite, scale, BlendFunction, Builder, MaterialKind, RenderChicagoMap, RenderMaterial,
};

/// Which transparent class a [`RenderTransparent`] draws.
#[derive(Clone, Copy, Debug, PartialEq, Eq, uniffi::Enum)]
pub enum TransparentClass {
    /// Glass: the frame tinted, a reflection added, a diffuse map over.
    Glass,
    /// A meter's bar, its gradient by the map's alpha.
    Meter,
    /// Plasma: its noise maps' glow, brighter across the surface.
    Plasma,
    /// Water: what is behind tinted by its base map, a reflection added.
    Water,
}

/// A glass, meter, plasma or water shader's parameters. Colours are in
/// display values (red, green, blue, then a brightness or an opacity).
#[derive(Clone, Debug, PartialEq, uniffi::Record)]
pub struct RenderTransparent {
    /// Its class.
    pub class: TransparentClass,
    /// The class's flags: glass's (bit 3 the bump map's alpha masks the
    /// reflection), a meter's (bit 3 tint mode 2) or water's (bit 0 the
    /// base map's alpha masks the reflection, bit 1 its colour tints
    /// what is behind).
    pub flags: u8,
    /// Its maps, at most four: glass's diffuse, tint, bump and diffuse
    /// detail maps; a meter's map; plasma's two noise maps; water's base
    /// map. Only the map, scale and offset are used.
    pub maps: Vec<RenderChicagoMap>,
    /// The reflection's cube map texture.
    pub reflection_map: Option<u32>,
    /// What the frame behind is multiplied by (glass), or what the whole
    /// is (a meter).
    pub tint: Vec<f32>,
    /// The reflection's or the glow's colour looking along the normal (a
    /// meter: its gradient's start, and its bar's opacity).
    pub perpendicular: Vec<f32>,
    /// The same looking across the surface (a meter: its gradient's end,
    /// and its background's opacity).
    pub parallel: Vec<f32>,
}

/// A transparent map's slot: its texture, scale and offset.
fn slot(builder: &mut Builder<'_>, map: Option<&TagRef>, by: f32) -> RenderChicagoMap {
    RenderChicagoMap {
        map: builder.map(map),
        color_function: 0,
        alpha_function: 0,
        alpha_replicate: false,
        scale: vec![scale(by), scale(by)],
        offset: vec![0.0, 0.0],
    }
}

fn rgb(color: [f32; 3], last: f32) -> Vec<f32> {
    vec![
        finite(color[0]),
        finite(color[1]),
        finite(color[2]),
        finite(last),
    ]
}

/// A colour given as alpha, red, green and blue, as red, green, blue and
/// alpha.
fn argb(color: [f32; 4]) -> Vec<f32> {
    rgb([color[1], color[2], color[3]], color[0])
}

fn transparent(material: RenderTransparent) -> RenderMaterial {
    RenderMaterial {
        kind: MaterialKind::Blended,
        blend: BlendFunction::TintAndAdd,
        transparent: Some(material),
        ..RenderMaterial::plain()
    }
}

/// A noise map's scrolling as a colour swept along its direction (the
/// animation's floats carry it to the shader), once each period.
fn scroll(noise: &PlasmaNoise) -> AnimatedColor {
    AnimatedColor {
        on_color: noise.direction.map(finite),
        off_color: [0.0; 3],
        function: function::SLIDE,
        period: finite(noise.period).max(0.0),
        phase: 0.0,
    }
}

/// A plasma shader's animation: its two noise maps' scrolling in the
/// primary and secondary colours.
pub(crate) fn plasma_animation(shader: &PlasmaShader) -> EnvironmentAnimation {
    let still = AnimatedColor {
        function: function::ZERO,
        ..AnimatedColor::default()
    };
    let none = TextureAnimation {
        function: function::ZERO,
        ..TextureAnimation::default()
    };
    EnvironmentAnimation {
        primary: scroll(&shader.primary_animation),
        secondary: scroll(&shader.secondary_animation),
        plasma: still,
        u: none.clone(),
        v: none,
    }
}

impl Builder<'_> {
    /// A transparent generic shader: its maps, each multiplying the ones
    /// before (its stages' combiners are not read), blended with the
    /// frame by its blend function.
    pub(crate) fn generic(&mut self, shader: &TransparentGenericShader) -> RenderMaterial {
        let chicago_maps: Vec<_> = shader
            .maps
            .iter()
            .take(4)
            .map(|map| RenderChicagoMap {
                map: self.map(map.map.as_ref()),
                color_function: 2,
                alpha_function: 2,
                alpha_replicate: false,
                scale: vec![scale(map.scale[0]), scale(map.scale[1])],
                offset: map.offset.iter().map(|&offset| finite(offset)).collect(),
            })
            .collect();
        RenderMaterial {
            kind: MaterialKind::Blended,
            blend: BlendFunction::from_chicago(shader.framebuffer_blend_function),
            base_map: chicago_maps.first().and_then(|map| map.map),
            chicago_maps,
            ..RenderMaterial::plain()
        }
    }

    pub(crate) fn glass(&mut self, shader: &GlassShader) -> RenderMaterial {
        let maps = vec![
            slot(self, shader.diffuse_map.as_ref(), shader.diffuse_map_scale),
            slot(self, shader.tint_map.as_ref(), shader.tint_map_scale),
            slot(self, shader.bump_map.as_ref(), shader.bump_map_scale),
            slot(
                self,
                shader.diffuse_detail_map.as_ref(),
                shader.diffuse_detail_map_scale,
            ),
        ];
        transparent(RenderTransparent {
            class: TransparentClass::Glass,
            flags: (shader.flags & 0xFF) as u8,
            maps,
            reflection_map: self.cube_map(shader.reflection_map.as_ref()),
            tint: rgb(shader.tint_color, 1.0),
            perpendicular: argb(shader.perpendicular_color),
            parallel: argb(shader.parallel_color),
        })
    }

    pub(crate) fn meter(&mut self, shader: &MeterShader) -> RenderMaterial {
        let maps = vec![slot(self, shader.map.as_ref(), 1.0)];
        transparent(RenderTransparent {
            class: TransparentClass::Meter,
            flags: (shader.flags & 0xFF) as u8,
            maps,
            reflection_map: None,
            tint: rgb(shader.tint_color, 1.0),
            perpendicular: rgb(shader.gradient_min_color, shader.meter_transparency),
            parallel: rgb(shader.gradient_max_color, shader.background_transparency),
        })
    }

    pub(crate) fn plasma(&mut self, shader: &PlasmaShader) -> RenderMaterial {
        let maps = vec![
            slot(
                self,
                shader.primary_animation.map.as_ref(),
                shader.primary_animation.scale,
            ),
            slot(
                self,
                shader.secondary_animation.map.as_ref(),
                shader.secondary_animation.scale,
            ),
        ];
        transparent(RenderTransparent {
            class: TransparentClass::Plasma,
            flags: 0,
            maps,
            reflection_map: None,
            tint: vec![1.0; 4],
            perpendicular: rgb(shader.perpendicular_color, shader.perpendicular_alpha),
            parallel: rgb(shader.parallel_color, shader.parallel_alpha),
        })
    }

    pub(crate) fn water(&mut self, shader: &WaterShader) -> RenderMaterial {
        let maps = vec![slot(self, shader.base_map.as_ref(), 1.0)];
        transparent(RenderTransparent {
            class: TransparentClass::Water,
            flags: (shader.flags & 0xFF) as u8,
            maps,
            reflection_map: self.cube_map(shader.reflection_map.as_ref()),
            tint: vec![1.0; 4],
            perpendicular: argb(shader.perpendicular_tint),
            parallel: argb(shader.parallel_tint),
        })
    }
}
