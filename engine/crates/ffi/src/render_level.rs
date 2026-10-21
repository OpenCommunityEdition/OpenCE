//! A structure BSP made ready for the GPU: one vertex buffer, one index
//! buffer, a draw for each of its materials, the materials' parameters and
//! the textures they sample. Built once when a level loads; the app makes
//! its Metal buffers and textures from it and draws it every frame.

use std::collections::HashMap;

use opence_content::{
    Level, Lightmap, Material, Pixels, ShaderClass, StructureBsp, TagRef, TextureFormat,
};

/// The bytes of one vertex: position, normal (three floats each), texture
/// coordinates and lightmap coordinates (two each), little-endian.
pub const VERTEX_STRIDE: usize = 40;

/// How a material is drawn.
#[derive(Clone, Copy, Debug, PartialEq, Eq, uniffi::Enum)]
pub enum MaterialKind {
    /// Opaque, writing depth.
    Opaque,
    /// Opaque where the base map's alpha is at least a half, else not drawn.
    AlphaTested,
    /// Blended with what is behind it, after the opaque draws, far to near.
    Blended,
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
}

impl BlendFunction {
    fn from_chicago(function: i16) -> Self {
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

/// A 2D texture's description; its levels' bytes are fetched one by one.
#[derive(Clone, Debug, PartialEq, Eq, uniffi::Record)]
pub struct RenderTexture {
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
}

/// Gathers the level's textures and materials, each once.
struct Builder<'a> {
    level: &'a Level,
    out: RenderLevel,
    textures: HashMap<(TagRef, u16), Option<u32>>,
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
        for lightmap in &structure.lightmaps {
            let page = builder.lightmap_texture(structure, lightmap);
            for material in &lightmap.materials {
                let Some(draw) = builder.draw(structure, material, page) else {
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
        Some(builder.out)
    }
}

impl Builder<'_> {
    /// A bitmap's image as a texture, if it is a 2D texture whose levels
    /// are all there.
    fn texture(&mut self, bitmap: &TagRef, image: u16) -> Option<u32> {
        let key = (bitmap.clone(), image);
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
                    && texture.faces == 1
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

    fn map(&mut self, map: Option<&TagRef>) -> Option<u32> {
        map.and_then(|map| self.texture(map, 0))
    }

    fn lightmap_texture(&mut self, structure: &StructureBsp, lightmap: &Lightmap) -> Option<u32> {
        let bitmap = structure.lightmap_bitmap.as_ref()?;
        self.texture(bitmap, lightmap.bitmap_index?)
    }

    /// A shader's material, or None for a surface not drawn (one with no
    /// shader: the sky's portals).
    fn material(&mut self, shader: &TagRef) -> Option<u32> {
        if let Some(&index) = self.materials.get(shader) {
            return index;
        }
        let class = self.level.shader(shader).map(|shader| &shader.class);
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
            },
            Some(
                ShaderClass::TransparentChicago(chicago)
                | ShaderClass::TransparentChicagoExtended(chicago),
            ) => {
                let first = chicago.maps.first();
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
                }
            }
            // (a class not imported yet, or a shader not found: drawn plain
            // and lit, so that the geometry is there)
            Some(ShaderClass::Other(_)) | None => RenderMaterial {
                kind: MaterialKind::Opaque,
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
            },
        };
        self.out.materials.push(material);
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
            let light = if lit {
                material.lightmap_vertices[number].texcoord
            } else {
                [0.0, 0.0]
            };
            let floats = vertex
                .position
                .iter()
                .chain(&vertex.normal)
                .chain(&vertex.texcoord)
                .chain(&light);
            for &value in floats {
                self.out.vertices.extend_from_slice(&value.to_le_bytes());
            }
            for (axis, &value) in vertex.position.iter().enumerate() {
                center[axis] += value / vertex_count as f32;
            }
        }
        Some(RenderDraw {
            first_index,
            index_count,
            material: index,
            lightmap: if lit { lightmap } else { None },
            center: center.to_vec(),
        })
    }
}

fn finite(value: f32) -> f32 {
    if value.is_finite() {
        value
    } else {
        0.0
    }
}

/// A map's scale: a zero (left unset in the tag) or nonsense is one.
fn scale(value: f32) -> f32 {
    if value.is_finite() && value != 0.0 {
        value
    } else {
        1.0
    }
}

#[cfg(test)]
mod tests {
    use super::*;
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

    fn bitmap(path: &str, images: usize, levels: usize) -> Bitmap {
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
        Shader {
            tag: tag(b"senv", path),
            material_type: 0,
            class: ShaderClass::Environment(Box::new(EnvironmentShader {
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
                self_illumination_scale: 0.0,
                self_illumination_map: None,
                specular_brightness: 0.0,
                specular_perpendicular_color: [0.0; 3],
                specular_parallel_color: [0.0; 3],
                reflection_flags: 0,
                reflection_kind: 0,
                lightmap_brightness_scale: 0.0,
                reflection_perpendicular_brightness: 0.0,
                reflection_parallel_brightness: 0.0,
                reflection_cube_map: None,
            })),
        }
    }

    fn chicago(path: &str) -> Shader {
        Shader {
            tag: tag(b"schi", path),
            material_type: 0,
            class: ShaderClass::TransparentChicago(TransparentChicagoShader {
                numeric_counter_limit: 0,
                flags: 0,
                first_map_kind: 0,
                framebuffer_blend_function: 3,
                framebuffer_fade_mode: 0,
                framebuffer_fade_source: 0,
                extra_layers: Vec::new(),
                maps: vec![ChicagoMap {
                    flags: 0,
                    color_function: 0,
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
        }
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
    fn level() -> Level {
        let materials = vec![
            material(Some(tag(b"schi", "glass")), 0, 1, true),
            material(Some(tag(b"senv", "ground")), 0, 1, true),
            material(None, 0, 1, true),
            material(Some(tag(b"senv", "fence")), 0, 1, false),
            material(Some(tag(b"senv", "ground")), 1, 2, true),
            material(Some(tag(b"senv", "ground")), 3, 100, true),
        ];
        Level {
            scenario: Scenario {
                path: "test".into(),
                structure_bsps: Vec::new(),
                player_starts: Vec::new(),
                placements: Vec::new(),
            },
            structure_bsps: vec![StructureBsp {
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
    fn vertices_carry_position_normal_and_both_coordinates() {
        let built = built();
        let float = |vertex: usize, at: usize| {
            let start = vertex * VERTEX_STRIDE + at * 4;
            let mut bytes = [0u8; 4];
            bytes.copy_from_slice(&built.vertices[start..start + 4]);
            f32::from_le_bytes(bytes)
        };
        // the first vertex drawn (the chicago's, in the page's order): its
        // second vertex, at (2, 0, 1)
        let floats: Vec<f32> = (0..10).map(|at| float(1, at)).collect();
        assert_eq!(floats, [2.0, 0.0, 1.0, 0.0, 0.0, 1.0, 2.0, 0.0, 1.0, 0.25]);
        // the unlit cutout's have no lightmap coordinates, nor its draw a page
        let fence = &built.draws[1];
        assert_eq!(fence.lightmap, None);
        // (the page's texture is the first made)
        assert_eq!(built.draws[0].lightmap, Some(0));
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
}
