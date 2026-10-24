//! A structure BSP made ready for the GPU: one vertex buffer, one index
//! buffer, a draw for each of its materials, the materials' parameters and
//! the textures they sample. Built once when a level loads; the app makes
//! its Metal buffers and textures from it and draws it every frame.

use std::collections::HashMap;

use crate::render_model::{model_animation, model_material, RenderModel};
use crate::render_transparent::{plasma_animation, RenderTransparent};

use opence_content::{
    EnvironmentAnimation, EnvironmentShader, Level, Lightmap, Material, Pixels, ShaderClass,
    StructureBsp, TagRef, TextureFormat,
};

/// The bytes of one vertex: position, normal (three floats each), texture
/// coordinates and lightmap coordinates (two each), then tangent, binormal
/// and the incident light's direction (three each), little-endian.
pub const VERTEX_STRIDE: usize = 76;

/// The floats each material's animation takes in
/// [`RenderLevel::animation`].
pub const ANIMATION_FLOATS: usize = 12;

/// How a material is drawn.
#[derive(Clone, Copy, Debug, PartialEq, Eq, uniffi::Enum)]
pub enum MaterialKind {
    /// Opaque, writing depth.
    Opaque,
    /// Opaque where the base map's alpha is at least a half, else not drawn.
    AlphaTested,
    /// Blended with what is behind it, after the opaque draws, far to near.
    Blended,
    /// A shader class not imported yet (or a shader not found): drawn
    /// opaque, plain and lit, so that the geometry is there, except in the
    /// sky.
    Plain,
}

/// A transparent chicago shader's map: one texture stage, combined with
/// the result of the stages before it by the stage before's functions.
#[derive(Clone, Debug, PartialEq, uniffi::Record)]
pub struct RenderChicagoMap {
    /// Its texture.
    pub map: Option<u32>,
    /// How the result so far combines with the next map's colour: 0 the
    /// result, 1 the next map, 2 multiplied, 3 double-multiplied, 4 added,
    /// 5 and 6 added signed, 7 and 8 subtracted (from the result, from the
    /// next map), 9 to 12 blended by the result's or the next map's alpha
    /// (or its inverse).
    pub color_function: u8,
    /// The same for the alpha.
    pub alpha_function: u8,
    /// Its alpha replaces its colour.
    pub alpha_replicate: bool,
    /// Its scale in u and v.
    pub scale: Vec<f32>,
    /// Its offset in u and v.
    pub offset: Vec<f32>,
}

/// How a blended material combines with the frame (a chicago shader's
/// framebuffer blend function).
#[derive(Clone, Copy, Debug, PartialEq, Eq, uniffi::Enum)]
pub enum BlendFunction {
    /// By the material's alpha.
    AlphaBlend,
    /// The frame times the material.
    Multiply,
    /// The frame times twice the material.
    DoubleMultiply,
    /// The frame plus the material.
    Add,
    /// The frame times the material's second colour, plus its first (a
    /// dual-source blend: glass, meters, plasma and water).
    TintAndAdd,
}

impl BlendFunction {
    pub(crate) fn from_chicago(function: i16) -> Self {
        // (subtract, the component minimum and maximum and alpha-multiply
        // add are drawn as the nearest of these)
        match function {
            1 => Self::Multiply,
            2 => Self::DoubleMultiply,
            3 | 4 | 7 => Self::Add,
            5 => Self::Multiply,
            _ => Self::AlphaBlend,
        }
    }
}

/// A material's parameters. Texture indices are into the level's textures;
/// a missing map is drawn as if it changed nothing.
#[derive(Clone, Debug, PartialEq, uniffi::Record)]
pub struct RenderMaterial {
    /// How it is drawn.
    pub kind: MaterialKind,
    /// How it blends, if it is blended.
    pub blend: BlendFunction,
    /// An environment shader's kind: 0 normal (the secondary detail map's
    /// alpha mixes the detail maps), 1 blended and 2 blended base specular
    /// (the base map's alpha mixes them).
    pub environment_kind: u8,
    /// How the detail maps combine with the base: 0 twice their product,
    /// 1 their product, 2 their sum less a half, doubled.
    pub detail_function: u8,
    /// The base map.
    pub base_map: Option<u32>,
    /// The base map's scale in u and v.
    pub base_scale: Vec<f32>,
    /// The base map's offset in u and v.
    pub base_offset: Vec<f32>,
    /// The primary detail map.
    pub primary_detail_map: Option<u32>,
    /// Its scale.
    pub primary_detail_scale: f32,
    /// The secondary detail map.
    pub secondary_detail_map: Option<u32>,
    /// Its scale.
    pub secondary_detail_scale: f32,
    /// A model shader's multipurpose map: in its red the reflection's mask,
    /// in its green the self-illumination's, in its blue the change
    /// colour's and in its alpha an auxiliary mask, any of which the
    /// detail mask may choose.
    pub multipurpose_map: Option<u32>,
    /// A model shader's detail mask: 0 none, then which of the
    /// multipurpose map's channels (or its inverse, odd values from 1)
    /// mask the detail map.
    pub detail_mask: u8,
    /// A model shader's change colour: 0 none, then which of the object's
    /// four change colours tints it (by the multipurpose map's alpha).
    pub change_color_source: u8,
    /// A transparent chicago shader's maps, at most four (none for the
    /// other classes).
    pub chicago_maps: Vec<RenderChicagoMap>,
    /// An environment shader's lighting: its micro detail, bump,
    /// self-illumination, specular and reflection (None for the other
    /// classes).
    pub environment: Option<RenderEnvironment>,
    /// A model shader's self-illumination and reflection (None for the
    /// other classes).
    pub model: Option<RenderModelShader>,
    /// A glass, meter, plasma or water shader's parameters (None for the
    /// other classes).
    pub transparent: Option<RenderTransparent>,
}

impl RenderMaterial {
    /// A material drawn opaque, plain and lit, with no maps: what a
    /// surface with a shader not found is drawn as, so that the geometry
    /// is there.
    pub(crate) fn plain() -> Self {
        Self {
            kind: MaterialKind::Plain,
            blend: BlendFunction::AlphaBlend,
            environment_kind: 0,
            detail_function: 0,
            base_map: None,
            base_scale: vec![1.0, 1.0],
            base_offset: vec![0.0, 0.0],
            primary_detail_map: None,
            primary_detail_scale: 1.0,
            secondary_detail_map: None,
            secondary_detail_scale: 1.0,
            multipurpose_map: None,
            detail_mask: 0,
            change_color_source: 0,
            chicago_maps: Vec::new(),
            environment: None,
            model: None,
            transparent: None,
        }
    }
}

/// What a model shader adds to its base and detail maps, as the original's
/// model combiner adds it. Colours are in display values, as the tag gives
/// them.
#[derive(Clone, Debug, PartialEq, uniffi::Record)]
pub struct RenderModelShader {
    /// The reflection's cube map texture.
    pub reflection_map: Option<u32>,
    /// The detail map applies after the reflection is added (flag bit 0),
    /// not to the base map before it is lit.
    pub detail_after_reflection: bool,
    /// Which of the object's change colours tints the self-illumination
    /// (0: none).
    pub self_illumination_color_source: u8,
    /// The reflection's tint and brightness looking along the normal.
    pub perpendicular: Vec<f32>,
    /// The reflection's tint and brightness looking across the surface.
    pub parallel: Vec<f32>,
    /// The distance at which the reflection is at full strength.
    pub reflection_falloff_distance: f32,
    /// The distance beyond which there is none (0: it never fades).
    pub reflection_cutoff_distance: f32,
}

/// What an environment shader adds to its base and detail maps. Colours
/// are in display values, as the tag gives them.
#[derive(Clone, Debug, PartialEq, uniffi::Record)]
pub struct RenderEnvironment {
    /// The micro detail map.
    pub micro_detail_map: Option<u32>,
    /// Its scale.
    pub micro_detail_scale: f32,
    /// How it combines, as the detail function.
    pub micro_detail_function: u8,
    /// The bump map (a normal map), unless it is the specular mask.
    pub bump_map: Option<u32>,
    /// Its scale.
    pub bump_scale: f32,
    /// The bump map's alpha masks the specular instead of bumping.
    pub bump_is_specular_mask: bool,
    /// The bump map that masks the specular, if it does.
    pub specular_mask_map: Option<u32>,
    /// The colour the light is multiplied by.
    pub material_color: Vec<f32>,
    /// The self-illumination map: its red, green and blue channels show
    /// the primary, secondary and plasma colours.
    pub self_illumination_map: Option<u32>,
    /// Its scale.
    pub self_illumination_scale: f32,
    /// Sampled without filtering.
    pub self_illumination_unfiltered: bool,
    /// The plasma's colour where its band is, and where it is not.
    pub plasma_on_color: Vec<f32>,
    /// (as the plasma's on colour)
    pub plasma_off_color: Vec<f32>,
    /// The specular flags: 1 overbright, 2 extra shiny, 4 the lightmap's
    /// brightness is specular.
    pub specular_flags: u8,
    /// The specular brightness.
    pub specular_brightness: f32,
    /// The specular colour, looking along the normal.
    pub specular_perpendicular_color: Vec<f32>,
    /// The specular colour, looking across it.
    pub specular_parallel_color: Vec<f32>,
    /// The reflection's cube map texture.
    pub reflection_map: Option<u32>,
    /// The reflection's kind: 0 bumped cube map, 1 flat cube map, 2
    /// bumped radiosity.
    pub reflection_kind: u8,
    /// How much the lightmap's brightness masks the reflection.
    pub lightmap_brightness_scale: f32,
    /// The reflection's brightness, looking along the normal.
    pub reflection_perpendicular_brightness: f32,
    /// The reflection's brightness, looking across it.
    pub reflection_parallel_brightness: f32,
}

/// A chicago map's function, or the result's (0) for one out of range.
fn chicago_function(function: i16) -> u8 {
    u8::try_from(function)
        .ok()
        .filter(|&function| function <= 12)
        .unwrap_or(0)
}

/// A draw: a material's triangles.
#[derive(Clone, Debug, PartialEq, uniffi::Record)]
pub struct RenderDraw {
    /// Its first index, in the index buffer.
    pub first_index: u32,
    /// How many indices.
    pub index_count: u32,
    /// Its material.
    pub material: u32,
    /// Its lightmap page's texture, if it is lightmapped.
    pub lightmap: Option<u32>,
    /// Its lightmap page, in the BSP's lightmaps, if it is lightmapped
    /// (the page its baked light is in, when the light is baked again).
    pub lightmap_page: Option<u32>,
    /// Lit from its normals, as the game lights a structure BSP shipped
    /// with no lightmaps (its `no_lightmap` mode); otherwise it would be
    /// drawn fullbright.
    pub unbaked: bool,
    /// The middle of its vertices, for drawing blended ones far to near.
    pub center: Vec<f32>,
}

/// A texture's format, as Metal takes it.
#[derive(Clone, Copy, Debug, PartialEq, Eq, uniffi::Enum)]
pub enum RenderTextureFormat {
    /// BC1 (DXT1).
    Bc1,
    /// BC2 (DXT3).
    Bc2,
    /// BC3 (DXT5).
    Bc3,
    /// A byte each of red, green, blue and alpha.
    Rgba8,
}

/// A 2D or cube texture's description; its levels' bytes are fetched one
/// by one.
#[derive(Clone, Debug, PartialEq, Eq, uniffi::Record)]
pub struct RenderTexture {
    /// 1 for a 2D texture, 6 for a cube, each level holding its faces in
    /// turn (+x, -x, +y, -y, +z, -z).
    pub faces: u32,
    /// Its format.
    pub format: RenderTextureFormat,
    /// The first level's width.
    pub width: u32,
    /// The first level's height.
    pub height: u32,
    /// How many levels, largest first.
    pub level_count: u32,
    /// The bytes of one row of the first level (of 4x4 blocks, for BC).
    pub row_bytes: u32,
}

/// A structure BSP, ready for the GPU.
#[derive(Debug, Default)]
pub struct RenderLevel {
    /// The vertices, `VERTEX_STRIDE` bytes each.
    pub vertices: Vec<u8>,
    /// Each triangle's three vertices.
    pub indices: Vec<u32>,
    /// The draws: opaque and alpha-tested first, then blended.
    pub draws: Vec<RenderDraw>,
    /// The materials the draws use.
    pub materials: Vec<RenderMaterial>,
    /// The textures the materials and lightmaps use, with their levels'
    /// bytes.
    pub textures: Vec<(RenderTexture, Vec<Vec<u8>>)>,
    /// The BSP's bounds: x, y and z, each least and greatest.
    pub bounds: [[f32; 2]; 3],
    /// The first player start in the BSP (or in any): its position and
    /// facing.
    pub player_start: Option<([f32; 3], f32)>,
    /// The level's object models' vertices, `MODEL_VERTEX_STRIDE` bytes
    /// each.
    pub model_vertices: Vec<u8>,
    /// Each model triangle's three vertices.
    pub model_indices: Vec<u32>,
    /// The level's object models, as `Level::models` orders them.
    pub models: Vec<RenderModel>,
    /// The level's sky.
    pub sky: Option<crate::render_sky::RenderSky>,
    /// Each material's animation, if it is an environment shader's.
    pub animations: Vec<Option<EnvironmentAnimation>>,
    /// Each of the level's lights' gel, a cube texture (`u32::MAX`: none),
    /// as `Level::lights` orders them.
    pub light_gels: Vec<u32>,
    /// The textures the camera's sprites draw, as `CameraSprite::image`
    /// numbers them (`u32::MAX`: one not loaded, drawn as a soft disk).
    pub camera_textures: Vec<u32>,
    /// The decals the BSP lays when it is loaded, layer by layer.
    pub decals: Vec<crate::decals::RenderDecal>,
    /// The textures the decals laid while the game runs draw, as the
    /// snapshot's decal pictures number them (`u32::MAX`: one not loaded).
    pub decal_textures: Vec<u32>,
    /// The textures the particles, contrails, weather and detail objects
    /// draw, as the snapshot's particle pictures number them (`u32::MAX`:
    /// one not loaded).
    pub particle_textures: Vec<u32>,
    /// The BSP's detail objects, drawn as the snapshot's particles are.
    pub detail_objects: Vec<crate::snapshot::ParticleSnapshot>,
    /// The textures of the images the UI's widgets draw, by bitmap and
    /// image.
    pub ui_textures: HashMap<(TagRef, u16), u32>,
}

/// Gathers the level's textures and materials, each once.
pub(crate) struct Builder<'a> {
    level: &'a Level,
    pub(crate) out: RenderLevel,
    textures: HashMap<(TagRef, u16, u8), Option<u32>>,
    materials: HashMap<TagRef, Option<u32>>,
}

impl RenderLevel {
    /// The level's BSP `bsp`, or None if it has none of that index.
    pub fn build(level: &Level, bsp: usize) -> Option<Self> {
        let structure = level.structure_bsps.get(bsp)?;
        let mut builder = Builder {
            level,
            out: Self {
                bounds: structure.world_bounds,
                player_start: level
                    .scenario
                    .player_starts
                    .iter()
                    .find(|start| {
                        start
                            .structure_bsp
                            .is_none_or(|index| usize::from(index) == bsp)
                    })
                    .map(|start| (start.position, start.facing)),
                ..Self::default()
            },
            textures: HashMap::new(),
            materials: HashMap::new(),
        };
        let mut blended = Vec::new();
        for (number, lightmap) in structure.lightmaps.iter().enumerate() {
            let page = builder.lightmap_texture(structure, lightmap);
            for material in &lightmap.materials {
                let Some(draw) = builder.draw(structure, material, page, number as u32) else {
                    continue;
                };
                let kind = builder.out.materials[draw.material as usize].kind;
                if kind == MaterialKind::Blended {
                    blended.push(draw);
                } else {
                    builder.out.draws.push(draw);
                }
            }
        }
        builder.out.draws.extend(blended);
        builder.out.sky = crate::render_sky::render_sky(level);
        for light in &level.lights {
            let gel = builder.cube_map(light.gel.as_ref()).unwrap_or(u32::MAX);
            builder.out.light_gels.push(gel);
        }
        for (bitmap, image) in crate::camera::camera_images(level) {
            let texture = builder.texture(&bitmap, image, 1).unwrap_or(u32::MAX);
            builder.out.camera_textures.push(texture);
        }
        for (bitmap, image) in crate::dynamic_decals::decal_images(level) {
            let texture = builder.texture(&bitmap, image, 1).unwrap_or(u32::MAX);
            builder.out.decal_textures.push(texture);
        }
        for (bitmap, image) in crate::particles::particle_images(level) {
            let texture = builder.texture(&bitmap, image, 1).unwrap_or(u32::MAX);
            builder.out.particle_textures.push(texture);
        }
        for (bitmap, image) in crate::ui::ui_images(level) {
            if let Some(texture) = builder.texture(&bitmap, image, 1) {
                builder.out.ui_textures.insert((bitmap, image), texture);
            }
        }
        builder.out.detail_objects = crate::particles::detail_objects(level, structure);
        builder.out.decals = crate::decals::level_decals(level, structure, &mut |bitmap, image| {
            builder.texture(&bitmap.tag, image, 1)
        });
        for model in &level.models {
            let model = builder.model(model);
            builder.out.models.push(model);
        }
        Some(builder.out)
    }
}

impl Builder<'_> {
    /// A bitmap's image as a texture, if it is a 2D texture (or a cube,
    /// for `faces` 6) whose levels are all there.
    fn texture(&mut self, bitmap: &TagRef, image: u16, faces: u8) -> Option<u32> {
        let key = (bitmap.clone(), image, faces);
        if let Some(&index) = self.textures.get(&key) {
            return index;
        }
        let texture = self
            .level
            .bitmap(bitmap)
            .and_then(|bitmap| bitmap.images.get(usize::from(image)))
            .and_then(|image| match &image.pixels {
                Pixels::Texture(texture) => Some(texture),
                Pixels::ResourceMap(_) => None,
            })
            .filter(|texture| {
                texture.depth == 1
                    && texture.faces == faces
                    && !texture.levels.is_empty()
                    && texture
                        .levels
                        .iter()
                        .enumerate()
                        .all(|(level, bytes)| bytes.len() == texture.level_bytes(level))
            });
        let index = texture.map(|texture| {
            let format = match texture.format {
                TextureFormat::Bc1 => RenderTextureFormat::Bc1,
                TextureFormat::Bc2 => RenderTextureFormat::Bc2,
                TextureFormat::Bc3 => RenderTextureFormat::Bc3,
                TextureFormat::Rgba8 => RenderTextureFormat::Rgba8,
            };
            let description = RenderTexture {
                faces: u32::from(faces),
                format,
                width: u32::from(texture.width),
                height: u32::from(texture.height),
                level_count: texture.levels.len() as u32,
                row_bytes: texture.format.row_bytes(u32::from(texture.width)) as u32,
            };
            self.out
                .textures
                .push((description, texture.levels.clone()));
            self.out.textures.len() as u32 - 1
        });
        self.textures.insert(key, index);
        index
    }

    pub(crate) fn map(&mut self, map: Option<&TagRef>) -> Option<u32> {
        map.and_then(|map| self.texture(map, 0, 1))
    }

    pub(crate) fn cube_map(&mut self, map: Option<&TagRef>) -> Option<u32> {
        map.and_then(|map| self.texture(map, 0, 6))
    }

    /// An environment shader's lighting.
    fn environment(&mut self, shader: &EnvironmentShader) -> RenderEnvironment {
        let specular_mask = shader.flags & (1 << 1) != 0;
        let bump = self.map(shader.bump_map.as_ref());
        RenderEnvironment {
            micro_detail_map: self.map(shader.micro_detail_map.as_ref()),
            micro_detail_scale: scale(shader.micro_detail_scale),
            micro_detail_function: shader.micro_detail_function.clamp(0, 2) as u8,
            bump_map: if specular_mask { None } else { bump },
            bump_scale: scale(shader.bump_scale),
            bump_is_specular_mask: specular_mask,
            specular_mask_map: if specular_mask { bump } else { None },
            material_color: color(shader.material_color, 1.0),
            self_illumination_map: self.map(shader.self_illumination_map.as_ref()),
            self_illumination_scale: scale(shader.self_illumination_scale),
            self_illumination_unfiltered: shader.self_illumination_flags & 1 != 0,
            plasma_on_color: color(shader.animation.plasma.on_color, 0.0),
            plasma_off_color: color(shader.animation.plasma.off_color, 0.0),
            specular_flags: (shader.specular_flags & 7) as u8,
            specular_brightness: finite(shader.specular_brightness).max(0.0),
            specular_perpendicular_color: color(shader.specular_perpendicular_color, 0.0),
            specular_parallel_color: color(shader.specular_parallel_color, 0.0),
            reflection_map: self.cube_map(shader.reflection_cube_map.as_ref()),
            reflection_kind: shader.reflection_kind.clamp(0, 2) as u8,
            lightmap_brightness_scale: finite(shader.lightmap_brightness_scale).max(0.0),
            reflection_perpendicular_brightness: finite(shader.reflection_perpendicular_brightness)
                .max(0.0),
            reflection_parallel_brightness: finite(shader.reflection_parallel_brightness).max(0.0),
        }
    }

    fn lightmap_texture(&mut self, structure: &StructureBsp, lightmap: &Lightmap) -> Option<u32> {
        let bitmap = structure.lightmap_bitmap.as_ref()?;
        self.texture(bitmap, lightmap.bitmap_index?, 1)
    }

    /// A shader's material, or None for a surface not drawn (one with no
    /// shader: the sky's portals).
    pub(crate) fn material(&mut self, shader: &TagRef) -> Option<u32> {
        if let Some(&index) = self.materials.get(shader) {
            return index;
        }
        let class = self.level.shader(shader).map(|shader| &shader.class);
        let animation = match class {
            Some(ShaderClass::Environment(environment)) => Some(environment.animation.clone()),
            Some(ShaderClass::Model(model)) => Some(model_animation(model)),
            Some(ShaderClass::Plasma(plasma)) => Some(plasma_animation(plasma)),
            _ => None,
        };
        let material = match class {
            Some(ShaderClass::Environment(environment)) => RenderMaterial {
                kind: if environment.flags & 1 != 0 {
                    MaterialKind::AlphaTested
                } else {
                    MaterialKind::Opaque
                },
                blend: BlendFunction::AlphaBlend,
                environment_kind: environment.kind.clamp(0, 2) as u8,
                detail_function: environment.detail_function.clamp(0, 2) as u8,
                base_map: self.map(environment.base_map.as_ref()),
                base_scale: vec![1.0, 1.0],
                base_offset: vec![0.0, 0.0],
                primary_detail_map: self.map(environment.primary_detail_map.as_ref()),
                primary_detail_scale: scale(environment.primary_detail_scale),
                secondary_detail_map: self.map(environment.secondary_detail_map.as_ref()),
                secondary_detail_scale: scale(environment.secondary_detail_scale),
                multipurpose_map: None,
                detail_mask: 0,
                change_color_source: 0,
                chicago_maps: Vec::new(),
                environment: Some(self.environment(environment)),
                model: None,
                transparent: None,
            },
            Some(
                ShaderClass::TransparentChicago(chicago)
                | ShaderClass::TransparentChicagoExtended(chicago),
            ) => {
                let first = chicago.maps.first();
                let chicago_maps = chicago
                    .maps
                    .iter()
                    .take(4)
                    .map(|map| RenderChicagoMap {
                        map: self.map(map.map.as_ref()),
                        color_function: chicago_function(map.color_function),
                        alpha_function: chicago_function(map.alpha_function),
                        alpha_replicate: map.flags & (1 << 1) != 0,
                        scale: vec![scale(map.scale[0]), scale(map.scale[1])],
                        offset: map.offset.iter().map(|&offset| finite(offset)).collect(),
                    })
                    .collect();
                RenderMaterial {
                    kind: MaterialKind::Blended,
                    blend: BlendFunction::from_chicago(chicago.framebuffer_blend_function),
                    environment_kind: 0,
                    detail_function: 0,
                    base_map: self.map(first.and_then(|map| map.map.as_ref())),
                    base_scale: first.map_or(vec![1.0, 1.0], |map| {
                        vec![scale(map.scale[0]), scale(map.scale[1])]
                    }),
                    base_offset: first.map_or(vec![0.0, 0.0], |map| {
                        map.offset.iter().map(|&offset| finite(offset)).collect()
                    }),
                    primary_detail_map: None,
                    primary_detail_scale: 1.0,
                    secondary_detail_map: None,
                    secondary_detail_scale: 1.0,
                    multipurpose_map: None,
                    detail_mask: 0,
                    change_color_source: 0,
                    chicago_maps,
                    environment: None,
                    model: None,
                    transparent: None,
                }
            }
            Some(ShaderClass::Model(model)) => model_material(self, model),
            Some(ShaderClass::TransparentGeneric(generic)) => self.generic(generic),
            Some(ShaderClass::Glass(glass)) => self.glass(glass),
            Some(ShaderClass::Meter(meter)) => self.meter(meter),
            Some(ShaderClass::Plasma(plasma)) => self.plasma(plasma),
            Some(ShaderClass::Water(water)) => self.water(water),
            // (a group that is not a shader's, or a shader not found: drawn
            // plain and lit, so that the geometry is there)
            Some(ShaderClass::Other(_)) | None => RenderMaterial::plain(),
        };
        self.out.materials.push(material);
        self.out.animations.push(animation);
        let index = Some(self.out.materials.len() as u32 - 1);
        self.materials.insert(shader.clone(), index);
        index
    }

    /// A material's vertices and triangles added to the buffers, and its
    /// draw; None if it is not drawn or has no whole triangles.
    fn draw(
        &mut self,
        structure: &StructureBsp,
        material: &Material,
        lightmap: Option<u32>,
        page: u32,
    ) -> Option<RenderDraw> {
        let index = self.material(material.shader.as_ref()?)?;
        let first = usize::try_from(material.first_surface).ok()?;
        let count = usize::try_from(material.surface_count).ok()?;
        let surfaces = structure.surfaces.get(first..first.checked_add(count)?)?;
        let vertex_count = material.vertices.len();
        let base = u32::try_from(self.out.vertices.len() / VERTEX_STRIDE).ok()?;
        let first_index = self.out.indices.len() as u32;
        for triangle in surfaces {
            // (the importer checks them; a triangle past its vertices is
            // dropped all the same, never drawn from another's)
            if triangle
                .iter()
                .all(|&vertex| usize::from(vertex) < vertex_count)
            {
                self.out
                    .indices
                    .extend(triangle.iter().map(|&vertex| base + u32::from(vertex)));
            }
        }
        let index_count = self.out.indices.len() as u32 - first_index;
        if index_count == 0 {
            return None;
        }
        let lit = material.lightmap_vertices.len() == vertex_count;
        let mut center = [0.0f32; 3];
        for (number, vertex) in material.vertices.iter().enumerate() {
            let (light, incident) = if lit {
                let light = &material.lightmap_vertices[number];
                (light.texcoord, light.incident_direction)
            } else {
                ([0.0, 0.0], [0.0; 3])
            };
            let floats = vertex
                .position
                .iter()
                .chain(&vertex.normal)
                .chain(&vertex.texcoord)
                .chain(&light)
                .chain(&vertex.tangent)
                .chain(&vertex.binormal)
                .chain(&incident);
            for &value in floats {
                self.out.vertices.extend_from_slice(&value.to_le_bytes());
            }
            for (axis, &value) in vertex.position.iter().enumerate() {
                center[axis] += value / vertex_count as f32;
            }
        }
        let kind = self
            .out
            .materials
            .get(index as usize)
            .map(|material| material.kind);
        Some(RenderDraw {
            first_index,
            index_count,
            material: index,
            lightmap: if lit { lightmap } else { None },
            lightmap_page: (lit && lightmap.is_some()).then_some(page),
            unbaked: structure.lightmap_bitmap.is_none()
                && matches!(kind, Some(MaterialKind::Opaque | MaterialKind::AlphaTested)),
            center: center.to_vec(),
        })
    }
}

impl RenderLevel {
    /// Every material's animation at `seconds` of game time,
    /// [`ANIMATION_FLOATS`] each: the primary colour and the plasma's
    /// value, the secondary colour and a zero, and the maps' offset in u
    /// and v and two zeros (all zeros for a material that does not
    /// animate).
    pub fn animation(&self, seconds: f32) -> Vec<f32> {
        let mut out = Vec::with_capacity(self.materials.len() * ANIMATION_FLOATS);
        for index in 0..self.materials.len() {
            let Some(Some(animation)) = self.animations.get(index) else {
                out.extend([0.0; ANIMATION_FLOATS]);
                continue;
            };
            let swept = |color: &opence_content::AnimatedColor| {
                let value = opence_sim::periodic::animate(
                    color.function,
                    seconds,
                    color.period,
                    color.phase,
                );
                let on = self::color(color.on_color, 0.0);
                let off = self::color(color.off_color, 0.0);
                (
                    [0, 1, 2].map(|at| off[at] * (1.0 - value) + on[at] * value),
                    value,
                )
            };
            let scroll = |scroll: &opence_content::TextureAnimation| {
                let value = opence_sim::periodic::animate(
                    scroll.function,
                    seconds,
                    scroll.period,
                    scroll.phase,
                );
                finite(value * finite(scroll.scale))
            };
            let (primary, _) = swept(&animation.primary);
            let (secondary, _) = swept(&animation.secondary);
            let (_, plasma) = swept(&animation.plasma);
            out.extend(primary);
            out.push(plasma);
            out.extend(secondary);
            out.push(0.0);
            out.extend([scroll(&animation.u), scroll(&animation.v), 0.0, 0.0]);
        }
        out
    }
}

/// A colour's channels, each finite (else `otherwise`).
fn color(color: [f32; 3], otherwise: f32) -> Vec<f32> {
    color
        .iter()
        .map(|&channel| {
            if channel.is_finite() {
                channel
            } else {
                otherwise
            }
        })
        .collect()
}

pub(crate) fn finite(value: f32) -> f32 {
    if value.is_finite() {
        value
    } else {
        0.0
    }
}

/// A map's scale: a zero (left unset in the tag) or nonsense is one.
pub(crate) fn scale(value: f32) -> f32 {
    if value.is_finite() && value != 0.0 {
        value
    } else {
        1.0
    }
}

#[cfg(test)]
pub(crate) mod tests {
    use super::*;
    use crate::render_transparent::TransparentClass;
    use opence_content::{
        Bitmap, BitmapImage, ChicagoMap, CollisionBsp, EnvironmentShader, EnvironmentVertex, Group,
        LightmapVertex, Scenario, Shader, Texture, TransparentChicagoShader,
    };

    fn tag(group: &[u8; 4], path: &str) -> TagRef {
        TagRef {
            group: Group(*group),
            path: path.into(),
        }
    }

    pub(crate) fn bitmap(path: &str, images: usize, levels: usize) -> Bitmap {
        let texture = Texture {
            format: TextureFormat::Rgba8,
            width: 4,
            height: 4,
            depth: 1,
            faces: 1,
            levels: (0..levels)
                .map(|level| vec![0x80; (4usize >> level).max(1).pow(2) * 4])
                .collect(),
        };
        Bitmap {
            tag: tag(b"bitm", path),
            kind: 0,
            format: 0,
            usage: 0,
            flags: 0,
            bump_height: 0.0,
            sequences: Vec::new(),
            images: (0..images)
                .map(|_| BitmapImage {
                    width: 4,
                    height: 4,
                    depth: 1,
                    kind: 0,
                    format: 0,
                    flags: 0,
                    registration_point: [0, 0],
                    mipmap_count: levels as u16 - 1,
                    pixels: Pixels::Texture(texture.clone()),
                })
                .collect(),
        }
    }

    fn environment(path: &str, flags: u16) -> Shader {
        Shader::new(
            tag(b"senv", path),
            0,
            ShaderClass::Environment(Box::new(EnvironmentShader {
                flags,
                kind: 1,
                diffuse_flags: 0,
                base_map: Some(tag(b"bitm", "ground")),
                detail_function: 1,
                primary_detail_scale: 8.0,
                primary_detail_map: Some(tag(b"bitm", "detail")),
                secondary_detail_scale: 0.0,
                secondary_detail_map: Some(tag(b"bitm", "missing")),
                micro_detail_function: 0,
                micro_detail_scale: 0.0,
                micro_detail_map: None,
                material_color: [1.0; 3],
                bump_scale: 0.0,
                bump_map: None,
                self_illumination_flags: 0,
                self_illumination_scale: 0.0,
                self_illumination_map: None,
                specular_flags: 0,
                specular_brightness: 0.0,
                specular_perpendicular_color: [0.0; 3],
                specular_parallel_color: [0.0; 3],
                reflection_flags: 0,
                reflection_kind: 0,
                lightmap_brightness_scale: 0.0,
                reflection_perpendicular_brightness: 0.0,
                reflection_parallel_brightness: 0.0,
                reflection_cube_map: None,
                animation: opence_content::EnvironmentAnimation::default(),
            })),
        )
    }

    fn chicago(path: &str) -> Shader {
        Shader::new(
            tag(b"schi", path),
            0,
            ShaderClass::TransparentChicago(TransparentChicagoShader {
                numeric_counter_limit: 0,
                flags: 0,
                first_map_kind: 0,
                framebuffer_blend_function: 3,
                framebuffer_fade_mode: 0,
                framebuffer_fade_source: 0,
                extra_layers: Vec::new(),
                maps: vec![ChicagoMap {
                    flags: 1 << 1,
                    color_function: 40,
                    alpha_function: 0,
                    scale: [2.0, 0.0],
                    offset: [0.5, f32::NAN],
                    rotation: 0.0,
                    mipmap_bias: 0.0,
                    map: Some(tag(b"bitm", "ground")),
                }],
                two_stage_maps: Vec::new(),
                extra_flags: 0,
            }),
        )
    }

    fn vertex(x: f32, y: f32) -> EnvironmentVertex {
        EnvironmentVertex {
            position: [x, y, 1.0],
            normal: [0.0, 0.0, 1.0],
            binormal: [0.0, 1.0, 0.0],
            tangent: [1.0, 0.0, 0.0],
            texcoord: [x, y],
        }
    }

    fn material(shader: Option<TagRef>, first: u32, count: u32, lit: bool) -> Material {
        let vertices = vec![vertex(0.0, 0.0), vertex(2.0, 0.0), vertex(0.0, 2.0)];
        let lightmap_vertices = if lit {
            vertices
                .iter()
                .map(|vertex| LightmapVertex {
                    incident_direction: [0.0, 0.0, 1.0],
                    texcoord: [vertex.position[0] / 2.0, 0.25],
                })
                .collect()
        } else {
            Vec::new()
        };
        Material {
            shader,
            first_surface: first,
            surface_count: count,
            vertices,
            lightmap_vertices,
        }
    }

    /// A BSP of one lightmap page: a chicago triangle, a ground triangle,
    /// a sky portal, a cutout triangle (unlit), and a triangle past its
    /// vertices beside a whole one.
    pub(crate) fn level() -> Level {
        let materials = vec![
            material(Some(tag(b"schi", "glass")), 0, 1, true),
            material(Some(tag(b"senv", "ground")), 0, 1, true),
            material(None, 0, 1, true),
            material(Some(tag(b"senv", "fence")), 0, 1, false),
            material(Some(tag(b"senv", "ground")), 1, 2, true),
            material(Some(tag(b"senv", "ground")), 3, 100, true),
        ];
        Level {
            glows: Vec::new(),
            decals: Vec::new(),
            lens_flares: Vec::new(),
            light_volumes: Vec::new(),
            scenario: Scenario {
                path: "test".into(),
                structure_bsps: Vec::new(),
                player_starts: Vec::new(),
                placements: Vec::new(),
                netgame_flags: Vec::new(),
                netgame_equipment: Vec::new(),
                scripts: Default::default(),
                campaign: Default::default(),
                decal_palette: Vec::new(),
            },
            structure_bsps: vec![StructureBsp {
                cluster_skies: Vec::new(),
                weather_palette: Vec::new(),
                cluster_weather: Vec::new(),
                leaf_clusters: Vec::new(),
                detail_objects: Vec::new(),
                background_sounds: Vec::new(),
                sound_environments: Vec::new(),
                cluster_sounds: Vec::new(),
                cluster_sound_distances: Vec::new(),
                cluster_water: Vec::new(),
                vehicle_floor: 0.0,
                vehicle_ceiling: 0.0,
                decals: Vec::new(),
                lens_flare_markers: Vec::new(),
                lens_flares: Vec::new(),
                path: "test".into(),
                lightmap_bitmap: Some(tag(b"bitm", "lightmaps")),
                world_bounds: [[-1.0, 1.0], [-2.0, 2.0], [0.0, 3.0]],
                collision: CollisionBsp::default(),
                collision_materials: Vec::new(),
                surfaces: vec![[0, 1, 2], [2, 1, 9], [0, 2, 1]],
                lightmaps: vec![Lightmap {
                    bitmap_index: Some(1),
                    materials,
                }],
            }],
            objects: Vec::new(),
            models: Vec::new(),
            animation_graphs: Vec::new(),
            shaders: vec![
                environment("ground", 0),
                environment("fence", 1),
                chicago("glass"),
            ],
            bitmaps: vec![
                bitmap("ground", 1, 3),
                bitmap("detail", 1, 1),
                bitmap("lightmaps", 2, 1),
            ],
            resource_bitmaps: Vec::new(),
            skies: Vec::new(),
            globals: Default::default(),
            damage_effects: Vec::new(),
            collision_models: Vec::new(),
            physics: Vec::new(),
            effects: Vec::new(),
            particles: Vec::new(),
            contrails: Vec::new(),
            particle_systems: Vec::new(),
            weather: Vec::new(),
            detail_object_collections: Vec::new(),
            lights: Vec::new(),
            unit_huds: Vec::new(),
            weapon_huds: Vec::new(),
            grenade_huds: Vec::new(),
            hud_digits: None,
            hud_globals: None,
            ui: Default::default(),
            sounds: Vec::new(),
            looping_sounds: Vec::new(),
            material_effects: Vec::new(),
        }
    }

    fn built() -> RenderLevel {
        match RenderLevel::build(&level(), 0) {
            Some(built) => built,
            None => panic!("the level has a BSP 0"),
        }
    }

    #[test]
    fn each_drawn_material_is_a_draw_and_the_blended_come_last() {
        let built = built();
        let kinds: Vec<_> = built
            .draws
            .iter()
            .map(|draw| built.materials[draw.material as usize].kind)
            .collect();
        // (the sky portal is not drawn, and the material whose surfaces
        // run past the BSP's has none)
        assert_eq!(
            kinds,
            [
                MaterialKind::Opaque,
                MaterialKind::AlphaTested,
                MaterialKind::Opaque,
                MaterialKind::Blended,
            ]
        );
        assert_eq!(built.bounds, [[-1.0, 1.0], [-2.0, 2.0], [0.0, 3.0]]);
        assert!(RenderLevel::build(&level(), 1).is_none());
    }

    #[test]
    fn indices_point_at_their_own_materials_vertices() {
        let built = built();
        assert_eq!(built.vertices.len() % VERTEX_STRIDE, 0);
        let vertex_count = (built.vertices.len() / VERTEX_STRIDE) as u32;
        assert_eq!(vertex_count, 3 * 4);
        assert!(built.indices.iter().all(|&index| index < vertex_count));
        // the ground's second draw: of its two triangles, the one past its
        // vertices is dropped
        let second = &built.draws[2];
        assert_eq!(second.index_count, 3);
        let triangle = &built.indices[second.first_index as usize..][..3];
        assert_eq!(
            triangle.iter().map(|index| index % 3).collect::<Vec<_>>(),
            [0, 2, 1]
        );
    }

    #[test]
    fn a_bsp_shipped_without_lightmaps_lights_its_opaque_surfaces_from_their_normals() {
        // with its lightmaps, nothing is lit from its normals
        assert!(built().draws.iter().all(|draw| !draw.unbaked));
        let mut level = level();
        level.structure_bsps[0].lightmap_bitmap = None;
        let Some(built) = RenderLevel::build(&level, 0) else {
            panic!("the level has a BSP 0");
        };
        let unbaked: Vec<_> = built
            .draws
            .iter()
            .map(|draw| (built.materials[draw.material as usize].kind, draw.unbaked))
            .collect();
        // (the chicago glass is drawn unlit, as the game draws it)
        assert_eq!(
            unbaked,
            [
                (MaterialKind::Opaque, true),
                (MaterialKind::AlphaTested, true),
                (MaterialKind::Opaque, true),
                (MaterialKind::Blended, false),
            ]
        );
        assert!(built.draws.iter().all(|draw| draw.lightmap.is_none()));
    }

    #[test]
    fn vertices_carry_position_normal_both_coordinates_the_frame_and_the_light() {
        let built = built();
        let float = |vertex: usize, at: usize| {
            let start = vertex * VERTEX_STRIDE + at * 4;
            let mut bytes = [0u8; 4];
            bytes.copy_from_slice(&built.vertices[start..start + 4]);
            f32::from_le_bytes(bytes)
        };
        // the first vertex drawn (the chicago's, in the page's order): its
        // second vertex, at (2, 0, 1)
        let floats: Vec<f32> = (0..19).map(|at| float(1, at)).collect();
        assert_eq!(
            floats,
            [
                2.0, 0.0, 1.0, 0.0, 0.0, 1.0, 2.0, 0.0, 1.0, 0.25, // (then the tangent frame)
                1.0, 0.0, 0.0, 0.0, 1.0, 0.0, // (and the incident light)
                0.0, 0.0, 1.0,
            ]
        );
        // the unlit cutout's have no lightmap coordinates, nor its draw a page
        let fence = &built.draws[1];
        assert_eq!(fence.lightmap, None);
        // (the page's texture is the first made)
        assert_eq!(built.draws[0].lightmap, Some(0));
        assert_eq!(built.draws[0].lightmap_page, Some(0));
        assert_eq!(fence.lightmap_page, None);
        let middle = [2.0 / 3.0, 2.0 / 3.0, 1.0];
        assert!(fence
            .center
            .iter()
            .zip(middle)
            .all(|(a, b)| (a - b).abs() < 1e-6));
    }

    #[test]
    fn materials_name_their_maps_and_textures_are_made_once() {
        let built = built();
        let blended = &built.materials[built.draws[3].material as usize];
        assert_eq!(blended.blend, BlendFunction::Add);
        assert_eq!(blended.base_scale, vec![2.0, 1.0]);
        assert_eq!(blended.base_offset, vec![0.5, 0.0]);
        // its map's stage, its function out of range taken as the result's
        assert_eq!(blended.chicago_maps.len(), 1);
        assert_eq!(blended.chicago_maps[0].map, blended.base_map);
        assert_eq!(blended.chicago_maps[0].color_function, 0);
        assert!(blended.chicago_maps[0].alpha_replicate);
        let ground = &built.materials[built.draws[0].material as usize];
        assert_eq!(ground.environment_kind, 1);
        assert_eq!(ground.detail_function, 1);
        assert_eq!(ground.primary_detail_scale, 8.0);
        // a map the level does not have is none, its scale one
        assert_eq!(ground.secondary_detail_map, None);
        assert_eq!(ground.secondary_detail_scale, 1.0);
        assert_eq!(ground.base_map, blended.base_map);
        // the lightmap page, ground and detail
        assert_eq!(built.textures.len(), 3);
        let (ground_texture, levels) = &built.textures[ground.base_map.unwrap_or(0) as usize];
        assert_eq!(ground_texture.level_count, 3);
        assert_eq!(ground_texture.row_bytes, 16);
        assert_eq!(levels[2].len(), 4);
        assert_eq!(built.materials.len(), 3);
    }

    #[test]
    fn a_texture_missing_bytes_is_left_out() {
        let mut level = level();
        if let Pixels::Texture(texture) = &mut level.bitmaps[0].images[0].pixels {
            texture.levels[1].pop();
        }
        let built = RenderLevel::build(&level, 0).unwrap_or_default();
        let ground = &built.materials[built.draws[0].material as usize];
        assert_eq!(ground.base_map, None);
        assert!(ground.primary_detail_map.is_some());
    }

    fn cube(path: &str) -> Bitmap {
        let mut cube = bitmap(path, 1, 1);
        if let Pixels::Texture(texture) = &mut cube.images[0].pixels {
            texture.faces = 6;
            texture.levels = vec![vec![0x40; 4 * 4 * 4 * 6]];
        }
        cube
    }

    #[test]
    fn each_light_names_its_gel_cube() {
        let mut level = level();
        level.bitmaps.push(cube("gel"));
        let light = |gel: Option<TagRef>| opence_content::Light {
            tag: tag(b"ligh", "flashlight"),
            flags: opence_content::Light::DYNAMIC,
            radius: 4.0,
            radius_modifier: [1.0, 1.0],
            falloff_angle: 0.5,
            cutoff_angle: 0.6,
            lens_flare_radius: 0.0,
            cosine_falloff_angle: 0.0,
            cosine_cutoff_angle: 0.0,
            specular_radius_multiplier: 1.0,
            sine_cutoff_angle: 0.0,
            color_flags: 0,
            color_lower: [1.0; 4],
            color_upper: [1.0; 4],
            gel,
            lens_flare: None,
            duration: 0.0,
            falloff_function: 0,
        };
        level.lights = vec![light(Some(tag(b"bitm", "gel"))), light(None)];
        let built = RenderLevel::build(&level, 0).unwrap_or_default();
        assert_eq!(built.light_gels.len(), 2);
        let gel = built.light_gels[0];
        assert_eq!(built.textures[gel as usize].0.faces, 6);
        assert_eq!(built.light_gels[1], u32::MAX);
    }

    /// The level, its ground shader bumped, self-illuminated, reflecting
    /// and animated.
    fn lit_level(flags: u16) -> Level {
        use opence_content::{AnimatedColor, TextureAnimation};
        use opence_sim::periodic::function;
        let mut level = level();
        level.bitmaps.push(cube("sky cube"));
        let mut shader = environment("ground", flags);
        if let ShaderClass::Environment(environment) = &mut shader.class {
            environment.bump_map = Some(tag(b"bitm", "detail"));
            environment.bump_scale = 4.0;
            environment.self_illumination_map = Some(tag(b"bitm", "detail"));
            environment.self_illumination_flags = 1;
            environment.specular_flags = 0xFF;
            environment.specular_brightness = f32::NAN;
            environment.reflection_cube_map = Some(tag(b"bitm", "sky cube"));
            environment.reflection_kind = 9;
            environment.material_color = [0.5, f32::INFINITY, 1.0];
            environment.animation.primary = AnimatedColor {
                on_color: [1.0, 0.0, 0.0],
                off_color: [0.0, 0.0, 1.0],
                function: function::ONE,
                period: 1.0,
                phase: 0.0,
            };
            environment.animation.secondary = AnimatedColor {
                on_color: [1.0, 1.0, 1.0],
                off_color: [0.0, 0.5, 0.0],
                function: function::ZERO,
                period: 0.0,
                phase: 0.0,
            };
            environment.animation.plasma.function = function::ONE;
            environment.animation.plasma.on_color = [0.25, f32::NAN, 1.0];
            environment.animation.u = TextureAnimation {
                function: function::ONE,
                period: 2.0,
                scale: 0.5,
                phase: 0.0,
            };
        }
        level.shaders[0] = shader;
        level
    }

    fn lit(flags: u16) -> (RenderLevel, RenderEnvironment) {
        let Some(built) = RenderLevel::build(&lit_level(flags), 0) else {
            panic!("the level has a BSP 0");
        };
        let ground = built.materials[built.draws[0].material as usize].clone();
        let Some(environment) = ground.environment else {
            panic!("the ground is an environment shader");
        };
        (built, environment)
    }

    #[test]
    fn an_environment_shader_carries_its_bump_self_illumination_and_reflection() {
        let (built, environment) = lit(0);
        assert!(environment.bump_map.is_some());
        assert_eq!(environment.bump_scale, 4.0);
        assert!(!environment.bump_is_specular_mask);
        assert_eq!(environment.specular_mask_map, None);
        assert_eq!(environment.self_illumination_map, environment.bump_map);
        assert!(environment.self_illumination_unfiltered);
        assert_eq!(environment.plasma_on_color, vec![0.25, 0.0, 1.0]);
        assert_eq!(environment.plasma_off_color, vec![0.0; 3]);
        assert_eq!(environment.specular_flags, 7);
        assert_eq!(environment.specular_brightness, 0.0);
        assert_eq!(environment.reflection_kind, 2);
        assert_eq!(environment.material_color, vec![0.5, 1.0, 1.0]);
        // the cube is a texture of six faces, made apart from the 2D maps
        let Some(cube) = environment.reflection_map else {
            panic!("the cube map is a texture");
        };
        assert_eq!(built.textures[cube as usize].0.faces, 6);
        assert!(built
            .textures
            .iter()
            .enumerate()
            .all(|(index, (texture, _))| (texture.faces == 6) == (index as u32 == cube)));
        // the other classes have none
        assert!(built.materials[built.draws[3].material as usize]
            .environment
            .is_none());
    }

    #[test]
    fn a_model_shader_carries_its_reflection_and_animates_its_self_illumination() {
        use opence_content::{ModelShaderClass, TextureAnimation};
        use opence_sim::periodic::function;
        let mut level = lit_level(0);
        level.shaders[0].class = ShaderClass::Model(ModelShaderClass {
            flags: 1 | 1 << 2,
            translucency: 0.0,
            change_color_source: 7,
            map_scale: [0.0; 2],
            base_map: Some(tag(b"bitm", "ground")),
            multipurpose_map: Some(tag(b"bitm", "detail")),
            detail_function: 1,
            detail_mask: 3,
            detail_map_scale: 0.0,
            detail_map: None,
            detail_map_v_scale: 0.0,
            self_illumination_color: [1.0, 0.0, 0.0],
            self_illumination_lower: [0.0, 0.0, 1.0],
            self_illumination_flags: 0,
            self_illumination_color_source: 2,
            self_illumination_function: function::ONE,
            self_illumination_period: 1.0,
            u_animation: TextureAnimation {
                function: function::ONE,
                period: 2.0,
                scale: 0.25,
                phase: 0.5,
            },
            v_animation: TextureAnimation::default(),
            reflection_falloff_distance: f32::NAN,
            reflection_cutoff_distance: 3.0,
            perpendicular_brightness: 0.5,
            perpendicular_tint_color: [1.0, f32::INFINITY, 0.0],
            parallel_brightness: -1.0,
            parallel_tint_color: [0.25; 3],
            reflection_cube_map: Some(tag(b"bitm", "sky cube")),
        });
        let Some(built) = RenderLevel::build(&level, 0) else {
            panic!("the level has a BSP 0");
        };
        let index = built.draws[0].material as usize;
        let material = &built.materials[index];
        assert_eq!(material.kind, MaterialKind::Opaque);
        assert_eq!(material.change_color_source, 4);
        assert_eq!(material.detail_mask, 3);
        let Some(model) = material.model.as_ref() else {
            panic!("a model shader has its model parameters");
        };
        assert!(model.detail_after_reflection);
        assert_eq!(model.self_illumination_color_source, 2);
        assert_eq!(model.perpendicular, vec![1.0, 0.0, 0.0, 0.5]);
        assert_eq!(model.parallel, vec![0.25, 0.25, 0.25, 0.0]);
        assert_eq!(model.reflection_falloff_distance, 0.0);
        assert_eq!(model.reflection_cutoff_distance, 3.0);
        let Some(cube) = model.reflection_map else {
            panic!("the cube map is a texture");
        };
        assert_eq!(built.textures[cube as usize].0.faces, 6);
        // the self-illumination at its upper bound (the function is one),
        // and the scroll's phase counted
        let animation = built.animation(0.0);
        let at = index * ANIMATION_FLOATS;
        assert_eq!(&animation[at..at + 3], &[1.0, 0.0, 0.0]);
        assert_eq!(animation[at + 8], 0.25);
        assert!(built.materials[built.draws[3].material as usize]
            .model
            .is_none());
    }

    #[test]
    fn a_bump_map_that_is_the_specular_mask_does_not_bump() {
        let (_, environment) = lit(1 << 1);
        assert_eq!(environment.bump_map, None);
        assert!(environment.bump_is_specular_mask);
        assert!(environment.specular_mask_map.is_some());
    }

    #[test]
    fn a_cube_is_not_a_2d_map_nor_a_2d_map_a_cube() {
        let mut level = lit_level(0);
        if let ShaderClass::Environment(environment) = &mut level.shaders[0].class {
            environment.base_map = Some(tag(b"bitm", "sky cube"));
            environment.reflection_cube_map = Some(tag(b"bitm", "ground"));
        }
        let Some(built) = RenderLevel::build(&level, 0) else {
            panic!("the level has a BSP 0");
        };
        let ground = &built.materials[built.draws[0].material as usize];
        assert_eq!(ground.base_map, None);
        assert_eq!(
            ground.environment.as_ref().map(|lit| lit.reflection_map),
            Some(None)
        );
    }

    #[test]
    fn materials_animate_their_self_illumination_and_scrolling() {
        let (built, _) = lit(0);
        let animation = built.animation(3.25);
        assert_eq!(animation.len(), built.materials.len() * ANIMATION_FLOATS);
        let ground = built.draws[0].material as usize;
        let floats = &animation[ground * ANIMATION_FLOATS..][..ANIMATION_FLOATS];
        // primary on (its function one), plasma one, secondary off (zero),
        // scrolled half a map in u (one times its scale) and none in v
        assert_eq!(
            floats,
            [1.0, 0.0, 0.0, 1.0, 0.0, 0.5, 0.0, 0.0, 0.5, 0.0, 0.0, 0.0]
        );
        // a material that does not animate is all zeros
        let glass = built.draws[3].material as usize;
        assert!(animation[glass * ANIMATION_FLOATS..][..ANIMATION_FLOATS]
            .iter()
            .all(|&value| value == 0.0));
    }

    /// The level with its ground's first draw drawn with `shader` instead.
    fn with_shader(shader: Shader) -> RenderLevel {
        let mut level = level();
        let path = shader.tag.clone();
        level.shaders.push(shader);
        level.structure_bsps[0].lightmaps[0].materials[1].shader = Some(path);
        match RenderLevel::build(&level, 0) {
            Some(built) => built,
            None => panic!("the level has a BSP 0"),
        }
    }

    /// The material drawn by the ground's first draw (the level's second
    /// material: the chicago glass is the first).
    fn second(built: &RenderLevel) -> &RenderMaterial {
        &built.materials[1]
    }

    fn generic_map(map: &str) -> opence_content::GenericMap {
        opence_content::GenericMap {
            flags: 0,
            scale: [2.0, 0.0],
            offset: [0.25, f32::NAN],
            rotation: 0.0,
            map: Some(tag(b"bitm", map)),
        }
    }

    #[test]
    fn a_generic_shaders_maps_multiply_and_blend_as_it_says() {
        let built = with_shader(Shader::new(
            tag(b"sotr", "generic"),
            0,
            ShaderClass::TransparentGeneric(opence_content::TransparentGenericShader {
                flags: 0,
                first_map_kind: 0,
                framebuffer_blend_function: 1,
                framebuffer_fade_mode: 0,
                extra_layers: Vec::new(),
                maps: vec![generic_map("ground"), generic_map("missing")],
                stages: Vec::new(),
            }),
        ));
        let material = second(&built);
        assert_eq!(material.kind, MaterialKind::Blended);
        assert_eq!(material.blend, BlendFunction::Multiply);
        assert_eq!(material.chicago_maps.len(), 2);
        let first = &material.chicago_maps[0];
        assert!(first.map.is_some());
        assert!(material.chicago_maps[1].map.is_none());
        assert_eq!((first.color_function, first.alpha_function), (2, 2));
        assert_eq!(first.scale, [2.0, 1.0]);
        assert_eq!(first.offset, [0.25, 0.0]);
        assert_eq!(material.base_map, first.map);
        assert!(material.transparent.is_none());
    }

    #[test]
    fn glass_tints_the_frame_and_adds_its_reflection() {
        let built = with_shader(Shader::new(
            tag(b"sgla", "glass"),
            0,
            ShaderClass::Glass(Box::new(opence_content::GlassShader {
                flags: 1 << 3,
                tint_color: [0.5, 0.25, 1.0],
                tint_map_scale: 0.0,
                tint_map: None,
                reflection_kind: 0,
                perpendicular_color: [0.5, 0.1, 0.2, 0.3],
                parallel_color: [1.0, 0.4, 0.5, f32::NAN],
                // (a two-dimensional bitmap is not a cube)
                reflection_map: Some(tag(b"bitm", "ground")),
                bump_map_scale: 4.0,
                bump_map: Some(tag(b"bitm", "detail")),
                diffuse_map_scale: 2.0,
                diffuse_map: Some(tag(b"bitm", "ground")),
                diffuse_detail_map_scale: 0.0,
                diffuse_detail_map: None,
            })),
        ));
        let material = second(&built);
        assert_eq!(material.kind, MaterialKind::Blended);
        assert_eq!(material.blend, BlendFunction::TintAndAdd);
        let Some(glass) = &material.transparent else {
            panic!("glass is transparent")
        };
        assert_eq!(glass.class, TransparentClass::Glass);
        assert_eq!(glass.flags, 8);
        assert_eq!(glass.maps.len(), 4);
        assert!(glass.maps[0].map.is_some() && glass.maps[2].map.is_some());
        assert!(glass.maps[1].map.is_none() && glass.maps[3].map.is_none());
        assert_eq!(glass.maps[0].scale, [2.0, 2.0]);
        assert_eq!(glass.maps[1].scale, [1.0, 1.0]);
        assert_eq!(glass.reflection_map, None);
        assert_eq!(glass.tint, [0.5, 0.25, 1.0, 1.0]);
        assert_eq!(glass.perpendicular, [0.1, 0.2, 0.3, 0.5]);
        assert_eq!(glass.parallel, [0.4, 0.5, 0.0, 1.0]);
    }

    #[test]
    fn meters_and_water_are_tinted_and_added() {
        let meter = with_shader(Shader::new(
            tag(b"smet", "meter"),
            0,
            ShaderClass::Meter(opence_content::MeterShader {
                flags: 0x103,
                map: Some(tag(b"bitm", "ground")),
                gradient_min_color: [1.0, 0.0, 0.0],
                gradient_max_color: [0.0, 1.0, 0.0],
                background_color: [0.0; 3],
                flash_color: [0.0; 3],
                tint_color: [1.0, 1.0, 0.5],
                meter_transparency: 0.75,
                background_transparency: 0.25,
                value_source: 0,
            }),
        ));
        let Some(bar) = &second(&meter).transparent else {
            panic!("a meter is transparent")
        };
        assert_eq!(bar.class, TransparentClass::Meter);
        assert_eq!(bar.flags, 3);
        assert_eq!(bar.perpendicular, [1.0, 0.0, 0.0, 0.75]);
        assert_eq!(bar.parallel, [0.0, 1.0, 0.0, 0.25]);
        assert_eq!(bar.tint, [1.0, 1.0, 0.5, 1.0]);

        let water = with_shader(Shader::new(
            tag(b"swat", "water"),
            0,
            ShaderClass::Water(opence_content::WaterShader {
                flags: 3,
                base_map: Some(tag(b"bitm", "ground")),
                perpendicular_tint: [0.2, 0.0, 0.5, 1.0],
                parallel_tint: [1.0, 1.0, 1.0, 1.0],
                reflection_map: None,
                ripple_angle: 0.0,
                ripple_velocity: 0.0,
                ripple_scale: 0.0,
                ripple_maps: None,
            }),
        ));
        let material = second(&water);
        assert_eq!(material.blend, BlendFunction::TintAndAdd);
        let Some(surface) = &material.transparent else {
            panic!("water is transparent")
        };
        assert_eq!(surface.class, TransparentClass::Water);
        assert_eq!(surface.flags, 3);
        assert_eq!(surface.maps.len(), 1);
        assert!(surface.maps[0].map.is_some());
        assert_eq!(surface.perpendicular, [0.0, 0.5, 1.0, 0.2]);
    }

    #[test]
    fn plasma_glows_and_its_noise_scrolls_along_its_directions() {
        let noise = |direction: [f32; 3], period: f32| opence_content::PlasmaNoise {
            period,
            direction,
            scale: 0.0,
            map: Some(tag(b"bitm", "detail")),
        };
        let built = with_shader(Shader::new(
            tag(b"spla", "plasma"),
            0,
            ShaderClass::Plasma(opence_content::PlasmaShader {
                intensity_source: 0,
                intensity_exponent: 1.0,
                offset_amount: 0.0,
                perpendicular_alpha: 0.5,
                perpendicular_color: [0.0, 0.0, 1.0],
                parallel_alpha: 1.0,
                parallel_color: [1.0, 0.0, 0.0],
                color_source: 0,
                primary_animation: noise([1.0, 2.0, 0.0], 4.0),
                secondary_animation: noise([-1.0, 0.0, 0.0], f32::NAN),
            }),
        ));
        let index = built
            .materials
            .iter()
            .position(|material| material.transparent.is_some());
        let Some(index) = index else {
            panic!("plasma is transparent")
        };
        let Some(plasma) = &built.materials[index].transparent else {
            unreachable!()
        };
        assert_eq!(plasma.class, TransparentClass::Plasma);
        assert_eq!(plasma.maps.len(), 2);
        assert_eq!(plasma.perpendicular, [0.0, 0.0, 1.0, 0.5]);
        assert_eq!(plasma.parallel, [1.0, 0.0, 0.0, 1.0]);
        let floats = built.animation(1.0);
        let at = index * ANIMATION_FLOATS;
        let slid =
            opence_sim::periodic::animate(opence_sim::periodic::function::SLIDE, 1.0, 4.0, 0.0);
        assert!((floats[at] - slid).abs() < 1e-5, "{floats:?}");
        assert!((floats[at + 1] - 2.0 * slid).abs() < 1e-5);
        // (a period that is not a number is none)
        assert!(floats[at + 4] <= 0.0 && floats[at + 4].is_finite());
        assert_eq!(floats[at + 3], 0.0);
    }
}
