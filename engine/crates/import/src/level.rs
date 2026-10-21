//! The level: the scenario's structure BSPs, with their collision and
//! lightmapped geometry, and the shaders and bitmaps they are drawn with.
//!
//! A structure BSP is not in the tag data: it has its own part of the
//! file, which the scenario's reference to it gives the offset, size and
//! address of, and every address inside it is relative to that address.

use std::collections::BTreeSet;

use opence_content::level::{
    Bsp2dNode, Bsp2dReference, Bsp3dNode, CollisionEdge, CollisionLeaf, CollisionSurface,
    CollisionVertex,
};
use opence_content::{
    CollisionBsp, CollisionMaterial, EnvironmentVertex, Group, Level, Lightmap, LightmapVertex,
    Material, ObjectKind, ObjectPlacement, PlacementPermutation, PlayerStart, Scenario,
    StructureBsp, TagRef,
};

use crate::cache::Cache;
use crate::error::ImportError;
use crate::reader;
use crate::tags::{Block, Element, GroupTag, Region};

pub(crate) const SCNR: GroupTag = GroupTag::new(b"scnr");
pub(crate) const SBSP: GroupTag = GroupTag::new(b"sbsp");
pub(crate) const SHDR: GroupTag = GroupTag::new(b"shdr");
pub(crate) const BITM: GroupTag = GroupTag::new(b"bitm");

pub(crate) const SCENARIO_SIZE: usize = 0x5B0;
pub(crate) const SCENARIO_BSPS_OFFSET: usize = 0x5A4;
pub(crate) const SCENARIO_BSP_SIZE: usize = 0x20;
const MAX_STRUCTURE_BSPS: usize = 32;
pub(crate) const SCENARIO_PLAYER_STARTS_OFFSET: usize = 0x354;
pub(crate) const PLAYER_START_SIZE: usize = 0x34;
const MAX_PLAYER_STARTS: usize = 256;

pub(crate) const BSP_HEADER_SIZE: usize = 0x18;
pub(crate) const BSP_HEADER_SIGNATURE_OFFSET: usize = 0x14;

pub(crate) const STRUCTURE_BSP_SIZE: usize = 0x288;
pub(crate) const LIGHTMAP_BITMAP_OFFSET: usize = 0x00;
pub(crate) const COLLISION_MATERIALS_OFFSET: usize = 0xA4;
pub(crate) const COLLISION_MATERIAL_SIZE: usize = 0x14;
const MAX_COLLISION_MATERIALS: usize = 512;
pub(crate) const COLLISION_BSPS_OFFSET: usize = 0xB0;
pub(crate) const COLLISION_BSP_SIZE: usize = 0x60;
pub(crate) const WORLD_BOUNDS_OFFSET: usize = 0xC8;
pub(crate) const SURFACES_OFFSET: usize = 0xF8;
pub(crate) const SURFACE_SIZE: usize = 6;
const MAX_SURFACES: usize = 0x20000;
pub(crate) const LIGHTMAPS_OFFSET: usize = 0x104;
pub(crate) const LIGHTMAP_SIZE: usize = 0x20;
const MAX_LIGHTMAPS: usize = 128;
pub(crate) const MATERIALS_OFFSET: usize = 0x14;
pub(crate) const MATERIAL_SIZE: usize = 0x100;
const MAX_MATERIALS: usize = 2048;
const MAX_MATERIAL_SURFACES: usize = 20000;
const MAX_MATERIAL_VERTICES: usize = 64000;
pub(crate) const MATERIAL_FIRST_SURFACE_OFFSET: usize = 0x14;
pub(crate) const MATERIAL_SURFACE_COUNT_OFFSET: usize = 0x18;
pub(crate) const MATERIAL_VERTEX_COUNT_OFFSET: usize = 0xB4;
pub(crate) const MATERIAL_LIGHTMAP_VERTEX_COUNT_OFFSET: usize = 0xC8;
pub(crate) const MATERIAL_UNCOMPRESSED_VERTICES_OFFSET: usize = 0xD8;
pub(crate) const MATERIAL_COMPRESSED_VERTICES_OFFSET: usize = 0xEC;

pub(crate) const UNCOMPRESSED_VERTEX_SIZE: usize = 56;
pub(crate) const UNCOMPRESSED_LIGHTMAP_VERTEX_SIZE: usize = 20;
pub(crate) const COMPRESSED_VERTEX_SIZE: usize = 32;
pub(crate) const COMPRESSED_LIGHTMAP_VERTEX_SIZE: usize = 8;

/// The collision BSP's blocks: offset, element size, most elements.
const BSP3D_NODES: (usize, usize, usize) = (0x00, 12, 131072);
const PLANES: (usize, usize, usize) = (0x0C, 16, 131072);
const LEAVES: (usize, usize, usize) = (0x18, 8, 131072);
const BSP2D_REFERENCES: (usize, usize, usize) = (0x24, 8, 131072);
const BSP2D_NODES: (usize, usize, usize) = (0x30, 20, 131072);
const COLLISION_SURFACES: (usize, usize, usize) = (0x3C, 12, 131072);
const EDGES: (usize, usize, usize) = (0x48, 24, 262144);
const VERTICES: (usize, usize, usize) = (0x54, 16, 131072);

/// A child or a designator whose high bit is set: a leaf, a surface or a
/// flipped plane, in the low 31 bits.
const HIGH_BIT: i32 = i32::MIN;

/// A tag a reference names.
pub(crate) fn tag_ref(cache: &Cache, index: usize) -> TagRef {
    let instance = &cache.tags()[index];
    TagRef {
        group: Group(instance.group.0.to_be_bytes()),
        path: instance.name.clone(),
    }
}

/// The shaders and bitmaps an import reached, to import after it.
#[derive(Default)]
pub(crate) struct Reached {
    pub(crate) shaders: BTreeSet<usize>,
    pub(crate) bitmaps: BTreeSet<usize>,
}

impl Reached {
    /// A reference to a shader, of any class.
    pub(crate) fn shader(
        &mut self,
        element: &Element<'_>,
        at: usize,
        field: &str,
    ) -> Result<Option<TagRef>, ImportError> {
        Ok(element.reference(at, field, &[SHDR])?.map(|index| {
            self.shaders.insert(index);
            tag_ref(element.cache(), index)
        }))
    }

    /// A reference to a bitmap.
    pub(crate) fn bitmap(
        &mut self,
        element: &Element<'_>,
        at: usize,
        field: &str,
    ) -> Result<Option<TagRef>, ImportError> {
        Ok(element.reference(at, field, &[BITM])?.map(|index| {
            self.bitmaps.insert(index);
            tag_ref(element.cache(), index)
        }))
    }
}

/// Imports the cache's level: its scenario's structure BSPs and every
/// shader and bitmap they reach.
pub fn import_level(cache: &Cache) -> Result<Level, ImportError> {
    let mut reached = Reached::default();
    let scenario = cache.tag(cache.scenario_index(), SCNR, SCENARIO_SIZE)?;
    let bsps = scenario.block(
        SCENARIO_BSPS_OFFSET,
        "structure bsps",
        SCENARIO_BSP_SIZE,
        MAX_STRUCTURE_BSPS,
    )?;
    let mut structure_bsp_tags = Vec::with_capacity(bsps.len());
    let mut structure_bsps = Vec::with_capacity(bsps.len());
    for element in bsps.iter() {
        let index = element
            .reference(0x10, "structure bsp", &[SBSP])?
            .ok_or_else(|| element.fault("structure bsp", "is empty"))?;
        structure_bsp_tags.push(tag_ref(cache, index));
        structure_bsps.push(import_structure_bsp(cache, &element, index, &mut reached)?);
    }

    // (a shader's extra layers are shaders too, reached as each is read)
    let mut shaders = Vec::new();
    let mut imported = BTreeSet::new();
    while let Some(index) = reached
        .shaders
        .iter()
        .copied()
        .find(|index| !imported.contains(index))
    {
        imported.insert(index);
        shaders.push(crate::shader::import_shader(cache, index, &mut reached)?);
    }
    shaders.sort_by(|a, b| a.tag.cmp(&b.tag));

    let mut bitmaps = Vec::new();
    let mut resource_bitmaps = Vec::new();
    for &index in &reached.bitmaps {
        if cache.tags()[index].in_resource_map {
            resource_bitmaps.push(tag_ref(cache, index));
        } else {
            bitmaps.push(crate::bitmap::import_bitmap(cache, index)?);
        }
    }
    bitmaps.sort_by(|a, b| a.tag.cmp(&b.tag));

    let starts = scenario.block(
        SCENARIO_PLAYER_STARTS_OFFSET,
        "player starting locations",
        PLAYER_START_SIZE,
        MAX_PLAYER_STARTS,
    )?;
    let mut player_starts = Vec::with_capacity(starts.len());
    for element in starts.iter() {
        player_starts.push(import_player_start(&element, structure_bsps.len())?);
    }

    let placements = import_placements(&scenario)?;

    let level = Level {
        scenario: Scenario {
            path: scenario.instance().name.clone(),
            structure_bsps: structure_bsp_tags,
            player_starts,
            placements,
        },
        structure_bsps,
        shaders,
        bitmaps,
        resource_bitmaps,
    };
    check_lightmap_pages(&level)?;
    Ok(level)
}

/// A player starting location: its position, facing and BSP (-1 for any).
fn import_player_start(element: &Element<'_>, bsps: usize) -> Result<PlayerStart, ImportError> {
    let mut position = [0.0; 3];
    for (axis, value) in position.iter_mut().enumerate() {
        *value = element.f32(axis * 4, "position")?;
    }
    let facing = element.f32(0x0C, "facing")?;
    if !position
        .iter()
        .chain([&facing])
        .all(|value| value.is_finite())
    {
        return Err(element.fault("position", "is not finite"));
    }
    let team = element.u16(0x10, "team")?;
    let structure_bsp = match element.i16(0x12, "structure bsp")? {
        -1 => None,
        index if index >= 0 && (index as usize) < bsps => Some(index as u16),
        index => {
            return Err(element.fault("structure bsp", format!("is {index}, past the {bsps} BSPs")))
        }
    };
    Ok(PlayerStart {
        position,
        facing,
        team,
        structure_bsp,
    })
}

/// Each kind's placements: its block in the scenario (its palette is the
/// block after it), the size of a placement, the most a scenario has, and
/// whether a permutation follows the object's own fields.
const PLACEMENT_BLOCKS: [(ObjectKind, usize, usize, usize, bool); 9] = [
    (ObjectKind::Scenery, 0x210, 0x48, 2000, true),
    (ObjectKind::Biped, 0x228, 0x78, 1024, true),
    (ObjectKind::Vehicle, 0x240, 0x78, 1024, true),
    (ObjectKind::Equipment, 0x258, 0x28, 1024, false),
    (ObjectKind::Weapon, 0x270, 0x5C, 1024, true),
    (ObjectKind::Machine, 0x294, 0x40, 1024, false),
    (ObjectKind::Control, 0x2AC, 0x40, 1024, false),
    (ObjectKind::LightFixture, 0x2C4, 0x58, 1024, false),
    (ObjectKind::SoundScenery, 0x2DC, 0x28, 1024, false),
];
pub(crate) const SCENARIO_OBJECT_NAMES_OFFSET: usize = 0x204;
pub(crate) const OBJECT_NAME_SIZE: usize = 0x24;
const MAX_OBJECT_NAMES: usize = 1024;
pub(crate) const PALETTE_ENTRY_SIZE: usize = 0x30;
const MAX_PALETTE_ENTRIES: usize = 256;
/// Where a placement's permutation is: after the object's fields.
const PLACEMENT_PERMUTATION_OFFSET: usize = 0x28;

/// The objects the scenario places. A placement whose palette entry is
/// none, past the palette or empty is not placed, as the game skips it; a
/// name past the names is no name, as the game ignores it.
fn import_placements(scenario: &Element<'_>) -> Result<Vec<ObjectPlacement>, ImportError> {
    let cache = scenario.cache();
    let names = scenario.block(
        SCENARIO_OBJECT_NAMES_OFFSET,
        "object names",
        OBJECT_NAME_SIZE,
        MAX_OBJECT_NAMES,
    )?;
    let names = names
        .iter()
        .map(|name| name.text(0, "object name", 32).map(str::to_owned))
        .collect::<Result<Vec<_>, _>>()?;
    let mut placements = Vec::new();
    for (kind, at, size, longest, has_permutation) in PLACEMENT_BLOCKS {
        let group = GroupTag::new(&kind.group());
        let palette =
            scenario.block(at + 12, "palette", PALETTE_ENTRY_SIZE, MAX_PALETTE_ENTRIES)?;
        let palette = palette
            .iter()
            .map(|entry| {
                entry
                    .reference(0, "palette entry", &[group])
                    .map(|index| index.map(|index| tag_ref(cache, index)))
            })
            .collect::<Result<Vec<_>, _>>()?;
        for element in scenario.block(at, "placements", size, longest)?.iter() {
            let tag = usize::try_from(element.i16(0, "palette index")?)
                .ok()
                .and_then(|index| palette.get(index).cloned().flatten());
            let Some(tag) = tag else {
                continue;
            };
            let name = usize::try_from(element.i16(2, "name index")?)
                .ok()
                .and_then(|index| names.get(index).cloned());
            let mut position = [0.0; 3];
            let mut rotation = [0.0; 3];
            for axis in 0..3 {
                position[axis] = element.f32(0x08 + axis * 4, "position")?;
                rotation[axis] = element.f32(0x14 + axis * 4, "rotation")?;
            }
            let permutation = if has_permutation {
                let mut change_colors = [0; 4];
                for (index, color) in change_colors.iter_mut().enumerate() {
                    *color =
                        element.u32(PLACEMENT_PERMUTATION_OFFSET + index * 4, "change color")?;
                }
                let mut region_permutations = [0; 8];
                region_permutations.copy_from_slice(element.slice(
                    PLACEMENT_PERMUTATION_OFFSET + 0x10,
                    8,
                    "region permutations",
                )?);
                Some(PlacementPermutation {
                    change_colors,
                    region_permutations,
                })
            } else {
                None
            };
            placements.push(ObjectPlacement {
                kind,
                tag,
                name,
                placement_flags: element.u16(4, "placement flags")?,
                position,
                rotation,
                structure_bsps: element.u16(0x20, "structure bsps")?,
                permutation,
            });
        }
    }
    Ok(placements)
}

/// Each lightmap's page must be in its BSP's lightmap bitmap (when the
/// bitmap is in the map, not in Custom Edition's `bitmaps.map`).
fn check_lightmap_pages(level: &Level) -> Result<(), ImportError> {
    for bsp in &level.structure_bsps {
        let pages = bsp
            .lightmap_bitmap
            .as_ref()
            .and_then(|tag| level.bitmap(tag))
            .map(|bitmap| bitmap.images.len());
        for (index, lightmap) in bsp.lightmaps.iter().enumerate() {
            let Some(page) = lightmap.bitmap_index else {
                continue;
            };
            let reason = match pages {
                Some(pages) if usize::from(page) >= pages => {
                    format!("is {page}, past the lightmap bitmap's {pages} images")
                }
                None if bsp.lightmap_bitmap.is_none() => {
                    format!("is {page}, but the BSP has no lightmap bitmap")
                }
                _ => continue,
            };
            return Err(ImportError::Tag {
                tag: bsp.path.clone(),
                group: SBSP,
                field: format!("lightmaps[{index}].bitmap"),
                reason,
            });
        }
    }
    Ok(())
}

/// Reads the structure BSP a scenario's reference names, from its part of
/// the file.
fn import_structure_bsp(
    cache: &Cache,
    reference: &Element<'_>,
    index: usize,
    reached: &mut Reached,
) -> Result<StructureBsp, ImportError> {
    let offset = reference.u32(0, "file offset")? as usize;
    let size = reference.u32(4, "size")? as usize;
    let address = reference.u32(8, "address")?;
    let bytes = reader::slice(cache.file(), offset, size)
        .map_err(|fault| reference.fault("size", fault))?;
    let region = Region::new(bytes, address);
    let header = cache.tag_in(index, SBSP, region, address, BSP_HEADER_SIZE)?;
    let signature = GroupTag(header.u32(BSP_HEADER_SIGNATURE_OFFSET, "header's signature")?);
    if signature != SBSP {
        return Err(header.fault("header's signature", format!("is {signature}, not sbsp")));
    }
    let bsp = cache.tag_in(
        index,
        SBSP,
        region,
        header.u32(0, "header's structure bsp")?,
        STRUCTURE_BSP_SIZE,
    )?;

    let lightmap_bitmap = reached.bitmap(&bsp, LIGHTMAP_BITMAP_OFFSET, "lightmap bitmap")?;
    let mut world_bounds = [[0.0; 2]; 3];
    for (axis, bounds) in world_bounds.iter_mut().enumerate() {
        for (end, bound) in bounds.iter_mut().enumerate() {
            *bound = bsp.f32(WORLD_BOUNDS_OFFSET + (axis * 2 + end) * 4, "world bounds")?;
        }
    }

    let materials = bsp.block(
        COLLISION_MATERIALS_OFFSET,
        "collision materials",
        COLLISION_MATERIAL_SIZE,
        MAX_COLLISION_MATERIALS,
    )?;
    let mut collision_materials = Vec::with_capacity(materials.len());
    for material in materials.iter() {
        collision_materials.push(CollisionMaterial {
            shader: reached.shader(&material, 0, "shader")?,
            material_type: material.i16(0x12, "material type")?,
        });
    }

    let collisions = bsp.block(
        COLLISION_BSPS_OFFSET,
        "collision bsp",
        COLLISION_BSP_SIZE,
        1,
    )?;
    let collision = collisions
        .get(0)
        .ok_or_else(|| bsp.fault("collision bsp", "is empty"))?;
    let collision = import_collision(&collision, collision_materials.len())?;

    let surfaces_block = bsp.block(SURFACES_OFFSET, "surfaces", SURFACE_SIZE, MAX_SURFACES)?;
    let mut surfaces = Vec::with_capacity(surfaces_block.len());
    for surface in surfaces_block.iter() {
        surfaces.push([
            surface.u16(0, "vertex")?,
            surface.u16(2, "vertex")?,
            surface.u16(4, "vertex")?,
        ]);
    }

    let lightmaps_block = bsp.block(LIGHTMAPS_OFFSET, "lightmaps", LIGHTMAP_SIZE, MAX_LIGHTMAPS)?;
    let mut lightmaps = Vec::with_capacity(lightmaps_block.len());
    for lightmap in lightmaps_block.iter() {
        let bitmap_index = lightmap.i16(0, "bitmap")?;
        let bitmap_index = match bitmap_index {
            -1 => None,
            index => Some(
                u16::try_from(index)
                    .map_err(|_| lightmap.fault("bitmap", format!("is {index}")))?,
            ),
        };
        let materials_block =
            lightmap.block(MATERIALS_OFFSET, "materials", MATERIAL_SIZE, MAX_MATERIALS)?;
        let mut materials = Vec::with_capacity(materials_block.len());
        for material in materials_block.iter() {
            materials.push(import_material(&material, &surfaces, reached)?);
        }
        lightmaps.push(Lightmap {
            bitmap_index,
            materials,
        });
    }

    Ok(StructureBsp {
        path: bsp.instance().name.clone(),
        lightmap_bitmap,
        world_bounds,
        collision,
        collision_materials,
        surfaces,
        lightmaps,
    })
}

fn count(
    element: &Element<'_>,
    at: usize,
    field: &str,
    longest: usize,
) -> Result<usize, ImportError> {
    let value = element.i32(at, field)?;
    usize::try_from(value)
        .ok()
        .filter(|&value| value <= longest)
        .ok_or_else(|| element.fault(field, format!("is {value}, not 0 to {longest}")))
}

/// A lightmap's material: its triangles, which must be in the BSP's
/// surfaces and index its own vertices, and those vertices.
fn import_material(
    material: &Element<'_>,
    surfaces: &[[u16; 3]],
    reached: &mut Reached,
) -> Result<Material, ImportError> {
    let shader = reached.shader(material, 0, "shader")?;
    let first_surface = count(
        material,
        MATERIAL_FIRST_SURFACE_OFFSET,
        "first surface",
        MAX_SURFACES,
    )?;
    let surface_count = count(
        material,
        MATERIAL_SURFACE_COUNT_OFFSET,
        "surface count",
        MAX_MATERIAL_SURFACES,
    )?;
    let triangles = surfaces
        .get(first_surface..first_surface + surface_count)
        .ok_or_else(|| {
            material.fault(
                "surface count",
                format!(
                    "is {surface_count} from {first_surface}, past the BSP's {} surfaces",
                    surfaces.len()
                ),
            )
        })?;
    let vertex_count = count(
        material,
        MATERIAL_VERTEX_COUNT_OFFSET,
        "vertex count",
        MAX_MATERIAL_VERTICES,
    )?;
    let lightmap_vertex_count = count(
        material,
        MATERIAL_LIGHTMAP_VERTEX_COUNT_OFFSET,
        "lightmap vertex count",
        MAX_MATERIAL_VERTICES,
    )?;
    if lightmap_vertex_count != 0 && lightmap_vertex_count != vertex_count {
        return Err(material.fault(
            "lightmap vertex count",
            format!("is {lightmap_vertex_count}, not 0 or the {vertex_count} vertices"),
        ));
    }
    if let Some(index) = triangles
        .iter()
        .flatten()
        .find(|&&index| usize::from(index) >= vertex_count)
    {
        return Err(material.fault(
            "surfaces",
            format!("index vertex {index}, past the material's {vertex_count}"),
        ));
    }

    let longest =
        MAX_MATERIAL_VERTICES * (UNCOMPRESSED_VERTEX_SIZE + UNCOMPRESSED_LIGHTMAP_VERTEX_SIZE);
    let uncompressed = material.data(
        MATERIAL_UNCOMPRESSED_VERTICES_OFFSET,
        "uncompressed vertices",
        longest,
    )?;
    let compressed = material.data(
        MATERIAL_COMPRESSED_VERTICES_OFFSET,
        "compressed vertices",
        longest,
    )?;
    // (Halo PC's caches keep the uncompressed vertices, the Xbox's the
    // compressed ones)
    let (vertices, lightmap_vertices) = if !uncompressed.is_empty() || compressed.is_empty() {
        let need = vertex_count * UNCOMPRESSED_VERTEX_SIZE
            + lightmap_vertex_count * UNCOMPRESSED_LIGHTMAP_VERTEX_SIZE;
        if uncompressed.len() < need {
            return Err(material.fault(
                "uncompressed vertices",
                format!(
                    "are {} bytes, fewer than the {need} of the vertices",
                    uncompressed.len()
                ),
            ));
        }
        let (environment, lightmap) =
            uncompressed.split_at(vertex_count * UNCOMPRESSED_VERTEX_SIZE);
        (
            decode(
                material,
                "uncompressed vertices",
                environment,
                UNCOMPRESSED_VERTEX_SIZE,
                uncompressed_vertex,
            )?,
            decode(
                material,
                "uncompressed vertices",
                &lightmap[..lightmap_vertex_count * UNCOMPRESSED_LIGHTMAP_VERTEX_SIZE],
                UNCOMPRESSED_LIGHTMAP_VERTEX_SIZE,
                uncompressed_lightmap_vertex,
            )?,
        )
    } else {
        let need = vertex_count * COMPRESSED_VERTEX_SIZE
            + lightmap_vertex_count * COMPRESSED_LIGHTMAP_VERTEX_SIZE;
        if compressed.len() < need {
            return Err(material.fault(
                "compressed vertices",
                format!(
                    "are {} bytes, fewer than the {need} of the vertices",
                    compressed.len()
                ),
            ));
        }
        let (environment, lightmap) = compressed.split_at(vertex_count * COMPRESSED_VERTEX_SIZE);
        (
            decode(
                material,
                "compressed vertices",
                environment,
                COMPRESSED_VERTEX_SIZE,
                compressed_vertex,
            )?,
            decode(
                material,
                "compressed vertices",
                &lightmap[..lightmap_vertex_count * COMPRESSED_LIGHTMAP_VERTEX_SIZE],
                COMPRESSED_LIGHTMAP_VERTEX_SIZE,
                compressed_lightmap_vertex,
            )?,
        )
    };

    Ok(Material {
        shader,
        first_surface: first_surface as u32,
        surface_count: surface_count as u32,
        vertices,
        lightmap_vertices,
    })
}

/// Decodes each `size` bytes; a vertex with a coordinate that is not
/// finite is refused.
fn decode<T>(
    material: &Element<'_>,
    field: &str,
    bytes: &[u8],
    size: usize,
    vertex: fn(&[u8]) -> Option<T>,
) -> Result<Vec<T>, ImportError> {
    bytes
        .chunks_exact(size)
        .enumerate()
        .map(|(index, chunk)| {
            vertex(chunk).ok_or_else(|| {
                material.fault(field, format!("has vertex {index}, which is not finite"))
            })
        })
        .collect()
}

fn f32_at(bytes: &[u8], at: usize) -> Option<f32> {
    reader::f32(bytes, at)
        .ok()
        .filter(|value| value.is_finite())
}

fn vector3(bytes: &[u8], at: usize) -> Option<[f32; 3]> {
    Some([
        f32_at(bytes, at)?,
        f32_at(bytes, at + 4)?,
        f32_at(bytes, at + 8)?,
    ])
}

fn vector2(bytes: &[u8], at: usize) -> Option<[f32; 2]> {
    Some([f32_at(bytes, at)?, f32_at(bytes, at + 4)?])
}

fn uncompressed_vertex(bytes: &[u8]) -> Option<EnvironmentVertex> {
    Some(EnvironmentVertex {
        position: vector3(bytes, 0)?,
        normal: vector3(bytes, 12)?,
        binormal: vector3(bytes, 24)?,
        tangent: vector3(bytes, 36)?,
        texcoord: vector2(bytes, 48)?,
    })
}

fn uncompressed_lightmap_vertex(bytes: &[u8]) -> Option<LightmapVertex> {
    Some(LightmapVertex {
        incident_direction: vector3(bytes, 0)?,
        texcoord: vector2(bytes, 12)?,
    })
}

fn compressed_vertex(bytes: &[u8]) -> Option<EnvironmentVertex> {
    Some(EnvironmentVertex {
        position: vector3(bytes, 0)?,
        normal: unit_vector(reader::u32(bytes, 12).ok()?),
        binormal: unit_vector(reader::u32(bytes, 16).ok()?),
        tangent: unit_vector(reader::u32(bytes, 20).ok()?),
        texcoord: vector2(bytes, 24)?,
    })
}

fn compressed_lightmap_vertex(bytes: &[u8]) -> Option<LightmapVertex> {
    Some(LightmapVertex {
        incident_direction: unit_vector(reader::u32(bytes, 0).ok()?),
        texcoord: [
            short_coordinate(reader::i16(bytes, 4).ok()?),
            short_coordinate(reader::i16(bytes, 6).ok()?),
        ],
    })
}

/// A unit vector packed in 11, 11 and 10 bits, decoded as the game does
/// (`uncompress_int32_to_real_vector3d`): each signed field n is
/// (2n + 1) / (2^bits - 1), in the same float operations.
pub(crate) fn unit_vector(compressed: u32) -> [f32; 3] {
    let i = (compressed << 21) as i32 as f32;
    let j = ((compressed >> 11) << 21) as i32 as f32;
    let k = ((compressed >> 22) << 22) as i32 as f32;
    [
        (i * (1.0 / 1_048_576.0) + 1.0) * (1.0 / 2047.0),
        (j * (1.0 / 1_048_576.0) + 1.0) * (1.0 / 2047.0),
        (k * (1.0 / 2_097_152.0) + 1.0) * (1.0 / 1023.0),
    ]
}

/// A texture coordinate packed in a signed short: (2n + 1) / 65535.
pub(crate) fn short_coordinate(value: i16) -> f32 {
    (f32::from(value) * 2.0 + 1.0) * (1.0 / 65535.0)
}

/// The collision BSP's eight blocks, each index in them checked against
/// the block it indexes.
pub(crate) fn import_collision(
    bsp: &Element<'_>,
    material_count: usize,
) -> Result<CollisionBsp, ImportError> {
    let block = |(at, size, longest): (usize, usize, usize), field: &str| {
        bsp.block(at, field, size, longest)
    };
    let nodes = block(BSP3D_NODES, "bsp3d nodes")?;
    let planes = block(PLANES, "planes")?;
    let leaves = block(LEAVES, "leaves")?;
    let references = block(BSP2D_REFERENCES, "bsp2d references")?;
    let nodes2d = block(BSP2D_NODES, "bsp2d nodes")?;
    let surfaces = block(COLLISION_SURFACES, "surfaces")?;
    let edges = block(EDGES, "edges")?;
    let vertices = block(VERTICES, "vertices")?;

    let index = |element: &Element<'_>, at: usize, field: &str, of: &Block<'_>| {
        let value = element.i32(at, field)?;
        u32::try_from(value)
            .ok()
            .filter(|&value| (value as usize) < of.len())
            .ok_or_else(|| {
                element.fault(
                    field,
                    format!("is {value}, past the {} there are", of.len()),
                )
            })
    };
    let optional = |element: &Element<'_>, at: usize, field: &str, of: &Block<'_>| {
        if element.i32(at, field)? == -1 {
            Ok(None)
        } else {
            index(element, at, field, of).map(Some)
        }
    };
    let plane = |element: &Element<'_>, at: usize| {
        let designator = element.i32(at, "plane")?;
        if ((designator & !HIGH_BIT) as usize) < planes.len() {
            Ok(designator)
        } else {
            Err(element.fault(
                "plane",
                format!("is {designator:#x}, past the {} planes", planes.len()),
            ))
        }
    };
    // (a 2D child: a node, or a surface in the low 31 bits)
    let child2d = |element: &Element<'_>, at: usize, field: &str| {
        let child = element.i32(at, field)?;
        let fits = if child >= 0 {
            (child as usize) < nodes2d.len()
        } else {
            ((child & !HIGH_BIT) as usize) < surfaces.len()
        };
        if fits {
            Ok(child)
        } else {
            Err(element.fault(field, format!("is {child:#x}, not a bsp2d node or surface")))
        }
    };

    let mut collision = CollisionBsp::default();
    for node in nodes.iter() {
        let mut children = [0; 2];
        for (side, child) in children.iter_mut().enumerate() {
            let value = node.i32(4 + side * 4, "child")?;
            let fits = if value >= 0 {
                (value as usize) < nodes.len()
            } else {
                value == -1 || ((value & !HIGH_BIT) as usize) < leaves.len()
            };
            if !fits {
                return Err(node.fault("child", format!("is {value:#x}, not a node or leaf")));
            }
            *child = value;
        }
        collision.bsp3d_nodes.push(Bsp3dNode {
            plane: plane(&node, 0)?,
            children,
        });
    }
    for element in planes.iter() {
        collision.planes.push([
            element.f32(0, "i")?,
            element.f32(4, "j")?,
            element.f32(8, "k")?,
            element.f32(12, "d")?,
        ]);
    }
    for leaf in leaves.iter() {
        let count = leaf.i16(2, "bsp2d reference count")?;
        let first = leaf.i32(4, "first bsp2d reference")?;
        // (an empty leaf's first reference is -1)
        let fits = (count == 0 && first == -1)
            || (count >= 0
                && first >= 0
                && (first as usize).saturating_add(count as usize) <= references.len());
        if !fits {
            return Err(leaf.fault(
                "bsp2d references",
                format!(
                    "are {count} from {first}, past the {} there are",
                    references.len()
                ),
            ));
        }
        collision.leaves.push(CollisionLeaf {
            flags: leaf.u16(0, "flags")?,
            bsp2d_reference_count: count as u16,
            first_bsp2d_reference: first.max(0) as u32,
        });
    }
    for reference in references.iter() {
        collision.bsp2d_references.push(Bsp2dReference {
            plane: plane(&reference, 0)?,
            node: child2d(&reference, 4, "node")?,
        });
    }
    for node in nodes2d.iter() {
        collision.bsp2d_nodes.push(Bsp2dNode {
            plane: [node.f32(0, "i")?, node.f32(4, "j")?, node.f32(8, "d")?],
            children: [child2d(&node, 12, "child")?, child2d(&node, 16, "child")?],
        });
    }
    for surface in surfaces.iter() {
        let material = match surface.i16(10, "material")? {
            -1 => None,
            value if value >= 0 && (value as usize) < material_count => Some(value as u16),
            value => {
                return Err(surface.fault(
                    "material",
                    format!("is {value}, past the {material_count} collision materials"),
                ))
            }
        };
        collision.surfaces.push(CollisionSurface {
            plane: plane(&surface, 0)?,
            first_edge: index(&surface, 4, "first edge", &edges)?,
            flags: surface.u8(8, "flags")?,
            breakable_surface: surface.u8(9, "breakable surface")?,
            material,
        });
    }
    for edge in edges.iter() {
        collision.edges.push(CollisionEdge {
            vertices: [
                index(&edge, 0, "start vertex", &vertices)?,
                index(&edge, 4, "end vertex", &vertices)?,
            ],
            edges: [
                index(&edge, 8, "forward edge", &edges)?,
                index(&edge, 12, "reverse edge", &edges)?,
            ],
            surfaces: [
                optional(&edge, 16, "left surface", &surfaces)?,
                optional(&edge, 20, "right surface", &surfaces)?,
            ],
        });
    }
    for vertex in vertices.iter() {
        collision.vertices.push(CollisionVertex {
            point: [
                vertex.f32(0, "x")?,
                vertex.f32(4, "y")?,
                vertex.f32(8, "z")?,
            ],
            first_edge: index(&vertex, 12, "first edge", &edges)?,
        });
    }
    Ok(collision)
}

#[cfg(test)]
pub(crate) mod tests {
    use super::*;
    use crate::bitmap::{IMAGES_OFFSET, IMAGE_SIZE, SEQUENCES_OFFSET};
    use crate::cache::CacheKind;
    use crate::fake::FakeCache;
    use crate::shader::{SCHI, SENV};
    use opence_content::{Pixels, ShaderClass, Texture, TextureFormat};

    const BSP_BASE: u32 = 0x4000_0000;

    pub(crate) fn put(bytes: &mut [u8], at: usize, value: &[u8]) {
        bytes[at..at + value.len()].copy_from_slice(value);
    }

    pub(crate) fn block(bytes: &mut [u8], at: usize, count: usize, address: u32) {
        put(bytes, at, &(count as i32).to_le_bytes());
        put(bytes, at + 4, &address.to_le_bytes());
    }

    pub(crate) fn reference(bytes: &mut [u8], at: usize, group: GroupTag, handle: u32) {
        put(bytes, at, &group.0.to_le_bytes());
        put(bytes, at + 12, &handle.to_le_bytes());
    }

    pub(crate) fn empty(bytes: &mut [u8], at: usize) {
        reference(bytes, at, GroupTag::NONE, u32::MAX);
    }

    /// A BSP's bytes, placed at `BSP_BASE`: elements go after the header
    /// and the structure BSP as they are added.
    struct BspBytes(Vec<u8>);

    impl BspBytes {
        fn add(&mut self, bytes: &[u8]) -> u32 {
            let address = BSP_BASE + self.0.len() as u32;
            self.0.extend_from_slice(bytes);
            address
        }
    }

    /// What to break in the level.
    #[derive(Default)]
    pub(crate) struct Breaks {
        bsp_signature: bool,
        triangle_past_vertices: bool,
        lightmap_page_past_images: bool,
        pixels_past_file: bool,
        node_child_past_leaves: bool,
    }

    /// A level of one BSP: one triangle in one lightmap's one material, an
    /// environment shader, a chicago shader layered on it, a bitmap of
    /// two images for the lightmap and a two-pixel collision BSP.
    pub(crate) fn level(kind: CacheKind, breaks: &Breaks) -> FakeCache {
        let mut fake = FakeCache::new(kind);

        // the bitmap: two 2x1 A8 images, their pixels in the file
        let pixels = fake.add_file_data(&[1, 2, 3, 4]);
        let mut images = vec![0u8; IMAGE_SIZE * 2];
        for (index, image) in images.chunks_mut(IMAGE_SIZE).enumerate() {
            put(image, 0, &GroupTag::new(b"bitm").0.to_le_bytes());
            put(image, 4, &2u16.to_le_bytes());
            put(image, 6, &1u16.to_le_bytes());
            put(image, 8, &1u16.to_le_bytes());
            let offset = pixels + index as u32 * 2;
            let offset = if breaks.pixels_past_file {
                offset + 0x10_0000
            } else {
                offset
            };
            put(image, 0x18, &offset.to_le_bytes());
            put(image, 0x1C, &2u32.to_le_bytes());
        }
        let images = fake.add_data(&images);
        let mut sequence = vec![0u8; 0x40];
        put(&mut sequence, 0, b"lightmaps");
        put(&mut sequence, 0x22, &2i16.to_le_bytes());
        let sequences = fake.add_data(&sequence);
        let mut bitmap = vec![0u8; crate::bitmap::BITMAP_SIZE];
        put(&mut bitmap, 0, &0i16.to_le_bytes());
        block(&mut bitmap, SEQUENCES_OFFSET, 1, sequences);
        block(&mut bitmap, IMAGES_OFFSET, 2, images);
        let bitmap = fake.add_tag(BITM, "levels\\fake\\bitmaps\\lightmap", &bitmap);
        let bitmap_handle = fake.handle(bitmap);

        // a chicago shader with one map of the bitmap, and an environment
        // shader with that bitmap as its base map and the chicago shader
        // reached through a collision material
        let mut map = vec![0u8; crate::shader::CHICAGO_MAP_SIZE];
        put(&mut map, 0x54, &1.0f32.to_le_bytes());
        put(&mut map, 0x58, &2.0f32.to_le_bytes());
        reference(
            &mut map,
            crate::shader::CHICAGO_MAP_BITMAP_OFFSET,
            BITM,
            bitmap_handle,
        );
        let map = fake.add_data(&map);
        let mut chicago = vec![0u8; crate::shader::CHICAGO_SIZE];
        put(&mut chicago, 0x22, &3i16.to_le_bytes());
        put(&mut chicago, 0x2C, &1i16.to_le_bytes());
        block(&mut chicago, crate::shader::CHICAGO_MAPS_OFFSET, 1, map);
        let chicago = fake.add_tag(SCHI, "levels\\fake\\shaders\\glass", &chicago);
        fake.set_parent(chicago, SHDR);
        let chicago_handle = fake.handle(chicago);

        let mut environment = vec![0u8; crate::shader::ENVIRONMENT_SIZE];
        put(&mut environment, 0x22, &7i16.to_le_bytes());
        for at in [0xB8, 0xCC, 0xFC, 0x128, 0x254, 0x324] {
            empty(&mut environment, at);
        }
        reference(&mut environment, 0x88, BITM, bitmap_handle);
        put(&mut environment, 0x10C, &0.5f32.to_le_bytes());
        let environment = fake.add_tag(SENV, "levels\\fake\\shaders\\ground", &environment);
        fake.set_parent(environment, SHDR);
        let environment_handle = fake.handle(environment);

        // the BSP
        let mut bsp = BspBytes(vec![0u8; BSP_HEADER_SIZE + STRUCTURE_BSP_SIZE]);
        let structure = BSP_HEADER_SIZE;
        put(&mut bsp.0, 0, &(BSP_BASE + structure as u32).to_le_bytes());
        let signature = if breaks.bsp_signature { BITM } else { SBSP };
        put(
            &mut bsp.0,
            BSP_HEADER_SIGNATURE_OFFSET,
            &signature.0.to_le_bytes(),
        );

        let mut material = vec![0u8; COLLISION_MATERIAL_SIZE];
        reference(&mut material, 0, SCHI, chicago_handle);
        put(&mut material, 0x12, &4i16.to_le_bytes());
        let collision_materials = bsp.add(&material);

        // (one node over one leaf, one surface of one edge between two
        // vertices)
        let leaf_child = if breaks.node_child_past_leaves {
            HIGH_BIT | 1
        } else {
            HIGH_BIT
        };
        let mut node = vec![0u8; 12];
        put(&mut node, 4, &(-1i32).to_le_bytes());
        put(&mut node, 8, &leaf_child.to_le_bytes());
        let nodes = bsp.add(&node);
        let planes = bsp.add(&[0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x80, 0x3f, 0, 0, 0, 0]);
        let mut leaf = vec![0u8; 8];
        put(&mut leaf, 4, &(-1i32).to_le_bytes());
        let leaves = bsp.add(&leaf);
        let mut surface = vec![0u8; 12];
        put(&mut surface, 10, &0i16.to_le_bytes());
        let collision_surfaces = bsp.add(&surface);
        let mut edge = vec![0u8; 24];
        put(&mut edge, 4, &1u32.to_le_bytes());
        put(&mut edge, 20, &(-1i32).to_le_bytes());
        let edges = bsp.add(&edge);
        let vertices = bsp.add(&[0u8; 32]);
        let mut collision = vec![0u8; COLLISION_BSP_SIZE];
        block(&mut collision, BSP3D_NODES.0, 1, nodes);
        block(&mut collision, PLANES.0, 1, planes);
        block(&mut collision, LEAVES.0, 1, leaves);
        block(&mut collision, COLLISION_SURFACES.0, 1, collision_surfaces);
        block(&mut collision, EDGES.0, 1, edges);
        block(&mut collision, VERTICES.0, 2, vertices);
        let collision = bsp.add(&collision);

        let last = if breaks.triangle_past_vertices {
            3u16
        } else {
            2
        };
        let mut triangle = Vec::new();
        for index in [0u16, 1, last] {
            triangle.extend_from_slice(&index.to_le_bytes());
        }
        let surfaces = bsp.add(&triangle);

        let mut vertex_bytes = Vec::new();
        for index in 0..3u8 {
            let floats = [
                f32::from(index),
                0.0,
                0.0, // position
                0.0,
                0.0,
                1.0, // normal
                0.0,
                1.0,
                0.0, // binormal
                1.0,
                0.0,
                0.0, // tangent
                0.25,
                0.75, // texcoord
            ];
            for float in floats {
                vertex_bytes.extend_from_slice(&float.to_le_bytes());
            }
        }
        for _ in 0..3 {
            for float in [0.0f32, 0.0, 1.0, 0.5, 0.5] {
                vertex_bytes.extend_from_slice(&float.to_le_bytes());
            }
        }
        let vertex_data = bsp.add(&vertex_bytes);
        let mut material = vec![0u8; MATERIAL_SIZE];
        reference(&mut material, 0, SENV, environment_handle);
        put(
            &mut material,
            MATERIAL_SURFACE_COUNT_OFFSET,
            &1i32.to_le_bytes(),
        );
        put(
            &mut material,
            MATERIAL_VERTEX_COUNT_OFFSET,
            &3i32.to_le_bytes(),
        );
        put(
            &mut material,
            MATERIAL_LIGHTMAP_VERTEX_COUNT_OFFSET,
            &3i32.to_le_bytes(),
        );
        let data = MATERIAL_UNCOMPRESSED_VERTICES_OFFSET;
        put(
            &mut material,
            data,
            &(vertex_bytes.len() as i32).to_le_bytes(),
        );
        put(&mut material, data + 12, &vertex_data.to_le_bytes());
        let materials = bsp.add(&material);
        let mut lightmap = vec![0u8; LIGHTMAP_SIZE];
        let page = if breaks.lightmap_page_past_images {
            2i16
        } else {
            1
        };
        put(&mut lightmap, 0, &page.to_le_bytes());
        block(&mut lightmap, MATERIALS_OFFSET, 1, materials);
        let lightmaps = bsp.add(&lightmap);

        let structure_bsp = &mut bsp.0[structure..structure + STRUCTURE_BSP_SIZE];
        reference(structure_bsp, LIGHTMAP_BITMAP_OFFSET, BITM, bitmap_handle);
        block(
            structure_bsp,
            COLLISION_MATERIALS_OFFSET,
            1,
            collision_materials,
        );
        block(structure_bsp, COLLISION_BSPS_OFFSET, 1, collision);
        for (at, bound) in [-1.0f32, 1.0, -2.0, 2.0, -3.0, 3.0].iter().enumerate() {
            put(
                structure_bsp,
                WORLD_BOUNDS_OFFSET + at * 4,
                &bound.to_le_bytes(),
            );
        }
        block(structure_bsp, SURFACES_OFFSET, 1, surfaces);
        block(structure_bsp, LIGHTMAPS_OFFSET, 1, lightmaps);

        let bsp_size = bsp.0.len() as u32;
        let bsp_offset = fake.add_file_data(&bsp.0);
        let bsp_tag = fake.add_tag(SBSP, "levels\\fake\\fake", &[0u8; 4]);
        let mut element = vec![0u8; SCENARIO_BSP_SIZE];
        put(&mut element, 0, &bsp_offset.to_le_bytes());
        put(&mut element, 4, &bsp_size.to_le_bytes());
        put(&mut element, 8, &BSP_BASE.to_le_bytes());
        reference(&mut element, 0x10, SBSP, fake.handle(bsp_tag));
        let element = fake.add_data(&element);
        let mut scenario = vec![0u8; SCENARIO_SIZE];
        block(&mut scenario, SCENARIO_BSPS_OFFSET, 1, element);
        // one player start in the BSP, facing +y
        let mut start = vec![0u8; PLAYER_START_SIZE];
        for (at, value) in [(0, 1.0f32), (4, 2.0), (8, 3.0), (0x0C, 1.5)] {
            put(&mut start, at, &value.to_le_bytes());
        }
        put(&mut start, 0x10, &2u16.to_le_bytes());
        let start = fake.add_data(&start);
        block(&mut scenario, SCENARIO_PLAYER_STARTS_OFFSET, 1, start);
        place_objects(&mut fake, &mut scenario);
        fake.set_scenario_data(&scenario);
        fake
    }

    /// Two object names; a scenery palette of a rock and an empty entry,
    /// with three placements (the rock, named and colored; one of the
    /// empty entry; one past the palette); and a weapon-less equipment
    /// palette of one, placed once.
    fn place_objects(fake: &mut FakeCache, scenario: &mut [u8]) {
        let mut names = vec![0u8; OBJECT_NAME_SIZE * 2];
        put(&mut names, 0, b"rock");
        put(&mut names, OBJECT_NAME_SIZE, b"other");
        let names = fake.add_data(&names);
        block(scenario, SCENARIO_OBJECT_NAMES_OFFSET, 2, names);

        let rock = fake.add_tag(GroupTag::new(b"scen"), "scenery\\rock", &[0u8; 4]);
        let mut palette = vec![0u8; PALETTE_ENTRY_SIZE * 2];
        reference(&mut palette, 0, GroupTag::new(b"scen"), fake.handle(rock));
        empty(&mut palette, PALETTE_ENTRY_SIZE);
        let palette = fake.add_data(&palette);
        block(scenario, 0x210 + 12, 2, palette);
        let mut scenery = vec![0u8; 0x48 * 3];
        for (index, placement) in scenery.chunks_mut(0x48).enumerate() {
            put(placement, 0, &(index as i16).to_le_bytes());
            put(placement, 2, &(-1i16).to_le_bytes());
        }
        put(&mut scenery, 2, &1i16.to_le_bytes());
        put(&mut scenery, 4, &(1u16 << 2).to_le_bytes());
        for (at, value) in [
            (0x08, 4.0f32),
            (0x0C, 5.0),
            (0x10, 6.0),
            (0x14, 0.5),
            (0x18, -0.25),
        ] {
            put(&mut scenery, at, &value.to_le_bytes());
        }
        put(&mut scenery, 0x20, &1u16.to_le_bytes());
        put(&mut scenery, 0x28, &0xFF10_2030u32.to_le_bytes());
        put(&mut scenery, 0x38, &[0, 3]);
        let scenery = fake.add_data(&scenery);
        block(scenario, 0x210, 3, scenery);

        let pack = fake.add_tag(GroupTag::new(b"eqip"), "powerups\\pack", &[0u8; 4]);
        let mut palette = vec![0u8; PALETTE_ENTRY_SIZE];
        reference(&mut palette, 0, GroupTag::new(b"eqip"), fake.handle(pack));
        let palette = fake.add_data(&palette);
        block(scenario, 0x258 + 12, 1, palette);
        let mut equipment = vec![0u8; 0x28];
        put(&mut equipment, 2, &7i16.to_le_bytes());
        put(&mut equipment, 4, &1u16.to_le_bytes());
        let equipment = fake.add_data(&equipment);
        block(scenario, 0x258, 1, equipment);
    }

    fn import(kind: CacheKind, breaks: &Breaks) -> Result<Level, String> {
        let cache = Cache::open(level(kind, breaks).build()).map_err(|error| error.to_string())?;
        import_level(&cache).map_err(|error| error.to_string())
    }

    #[test]
    fn a_fake_level_imports_into_typed_content() {
        for kind in [CacheKind::Xbox, CacheKind::CustomEdition] {
            let level = match import(kind, &Breaks::default()) {
                Ok(level) => level,
                Err(error) => panic!("{kind:?}: {error}"),
            };
            assert_eq!(level.scenario.structure_bsps.len(), 1);
            assert_eq!(
                level.scenario.player_starts,
                [PlayerStart {
                    position: [1.0, 2.0, 3.0],
                    facing: 1.5,
                    team: 2,
                    structure_bsp: Some(0),
                }]
            );
            assert_eq!(
                level.scenario.placements,
                [
                    ObjectPlacement {
                        kind: ObjectKind::Scenery,
                        tag: TagRef {
                            group: Group(*b"scen"),
                            path: "scenery\\rock".into(),
                        },
                        name: Some("other".into()),
                        placement_flags: 1 << 2,
                        position: [4.0, 5.0, 6.0],
                        rotation: [0.5, -0.25, 0.0],
                        structure_bsps: 1,
                        permutation: Some(PlacementPermutation {
                            change_colors: [0xFF10_2030, 0, 0, 0],
                            region_permutations: [0, 3, 0, 0, 0, 0, 0, 0],
                        }),
                    },
                    ObjectPlacement {
                        kind: ObjectKind::Equipment,
                        tag: TagRef {
                            group: Group(*b"eqip"),
                            path: "powerups\\pack".into(),
                        },
                        name: None,
                        placement_flags: 1,
                        position: [0.0; 3],
                        rotation: [0.0; 3],
                        structure_bsps: 0,
                        permutation: None,
                    },
                ]
            );
            assert!(level.scenario.placements[0].is_automatic());
            assert!(!level.scenario.placements[1].is_automatic());
            let bsp = &level.structure_bsps[0];
            assert_eq!(bsp.world_bounds, [[-1.0, 1.0], [-2.0, 2.0], [-3.0, 3.0]]);
            assert_eq!(bsp.surfaces, [[0, 1, 2]]);
            assert_eq!(bsp.collision.bsp3d_nodes[0].children, [-1, HIGH_BIT]);
            assert_eq!(bsp.collision.planes[0], [0.0, 0.0, 1.0, 0.0]);
            assert_eq!(bsp.collision.edges[0].surfaces, [Some(0), None]);
            assert_eq!(bsp.collision_materials[0].material_type, 4);
            let lightmap = &bsp.lightmaps[0];
            assert_eq!(lightmap.bitmap_index, Some(1));
            let material = &lightmap.materials[0];
            assert_eq!(material.vertices.len(), 3);
            assert_eq!(material.vertices[2].position, [2.0, 0.0, 0.0]);
            assert_eq!(material.vertices[2].texcoord, [0.25, 0.75]);
            assert_eq!(material.lightmap_vertices[0].texcoord, [0.5, 0.5]);

            // (sorted by tag: the environment shader before the chicago)
            assert_eq!(level.shaders.len(), 2);
            let ground = material.shader.as_ref().and_then(|tag| level.shader(tag));
            let Some(ShaderClass::Environment(ground)) = ground.map(|shader| &shader.class) else {
                panic!("{:?}", level.shaders)
            };
            assert_eq!(ground.material_color[0], 0.5);
            assert_eq!(ground.base_map, bsp.lightmap_bitmap);
            assert_eq!(ground.bump_map, None);
            let glass = bsp.collision_materials[0]
                .shader
                .as_ref()
                .and_then(|tag| level.shader(tag));
            let Some(glass) = glass else { panic!() };
            assert_eq!(glass.material_type, 3);
            let ShaderClass::TransparentChicago(glass) = &glass.class else {
                panic!("{glass:?}")
            };
            assert_eq!(glass.framebuffer_blend_function, 1);
            assert_eq!(glass.maps[0].scale, [1.0, 2.0]);

            assert_eq!(level.bitmaps.len(), 1);
            let bitmap = &level.bitmaps[0];
            assert_eq!(bitmap.sequences[0].name, "lightmaps");
            assert_eq!(bitmap.sequences[0].image_count, 2);
            assert_eq!(bitmap.images[1].width, 2);
            // (A8 widened to white with its alpha, and a mipmap made)
            let texture = |first: u8, second: u8, mip: u8| {
                Pixels::Texture(Texture {
                    format: TextureFormat::Rgba8,
                    width: 2,
                    height: 1,
                    depth: 1,
                    faces: 1,
                    levels: vec![
                        vec![255, 255, 255, first, 255, 255, 255, second],
                        vec![255, 255, 255, mip],
                    ],
                })
            };
            assert_eq!(bitmap.images[0].pixels, texture(1, 2, 2));
            assert_eq!(bitmap.images[1].pixels, texture(3, 4, 4));
        }
    }

    #[test]
    fn a_broken_level_is_refused_with_the_tag_and_field() {
        let cases = [
            (
                Breaks {
                    bsp_signature: true,
                    ..Breaks::default()
                },
                "header's signature is bitm, not sbsp",
            ),
            (
                Breaks {
                    triangle_past_vertices: true,
                    ..Breaks::default()
                },
                "surfaces index vertex 3, past the material's 3",
            ),
            (
                Breaks {
                    lightmap_page_past_images: true,
                    ..Breaks::default()
                },
                "lightmaps[0].bitmap is 2, past the lightmap bitmap's 2 images",
            ),
            (
                Breaks {
                    pixels_past_file: true,
                    ..Breaks::default()
                },
                "bitmaps[0].pixels",
            ),
            (
                Breaks {
                    node_child_past_leaves: true,
                    ..Breaks::default()
                },
                "child is 0x80000001, not a node or leaf",
            ),
        ];
        for kind in [CacheKind::Xbox, CacheKind::CustomEdition] {
            for (breaks, expected) in &cases {
                match import(kind, breaks) {
                    Ok(_) => panic!("{kind:?}: {expected}: imported"),
                    Err(error) => assert!(error.contains(expected), "{kind:?}: {error}"),
                }
            }
        }
    }

    pub(crate) fn stock_map(name: &str) -> Option<Cache> {
        let path = std::path::Path::new(env!("CARGO_MANIFEST_DIR"))
            .join("../../../assets/maps")
            .join(format!("{name}.map"));
        let Ok(file) = std::fs::read(&path) else {
            eprintln!("skipped: {} is not here", path.display());
            return None;
        };
        match Cache::open(file) {
            Ok(cache) => Some(cache),
            Err(error) => panic!("{name}: {error}"),
        }
    }

    #[test]
    fn bloodgulch_imports_when_the_maps_are_here() {
        let Some(cache) = stock_map("bloodgulch") else {
            return;
        };
        let level = match import_level(&cache) {
            Ok(level) => level,
            Err(error) => panic!("{error}"),
        };
        assert_eq!(level.structure_bsps.len(), 1);
        let placed = |kind| {
            level
                .scenario
                .placements
                .iter()
                .filter(|placement| placement.kind == kind)
                .count()
        };
        // (bipeds, vehicles, ..., scenery at 6, ..., sound scenery)
        assert_eq!(
            ObjectKind::ALL.map(placed),
            [0, 8, 0, 0, 0, 0, 50, 0, 0, 0, 0, 2]
        );
        let warthog = &level.scenario.placements[50];
        assert_eq!(warthog.tag.path, "vehicles\\warthog\\warthog");
        assert_eq!(warthog.position, [29.137, -90.8342, 0.286439]);
        assert!(warthog.permutation.is_some());
        let teams = level.scenario.player_starts.iter().map(|start| start.team);
        assert_eq!(teams.max(), Some(3));
        let bsp = &level.structure_bsps[0];
        assert_eq!(bsp.lightmaps.len(), 16);
        assert_eq!(bsp.surfaces.len(), 5503);
        let material = &bsp.lightmaps[0].materials[0];
        assert_eq!(material.vertices.len(), 807);
        assert_eq!(material.lightmap_vertices.len(), 807);
        for vertex in &material.vertices {
            let length = vertex.normal.iter().map(|c| c * c).sum::<f32>();
            assert!((length - 1.0).abs() < 0.01, "{vertex:?}");
        }
        assert!(!bsp.collision.bsp3d_nodes.is_empty());
        assert!(level.shaders.len() > 10);
        assert!(level.bitmaps.len() > 10);
        assert!(bsp
            .lightmap_bitmap
            .as_ref()
            .and_then(|tag| level.bitmap(tag))
            .is_some());
    }

    /// Every stock map, in release builds (`cargo test --release -- --ignored`).
    #[test]
    #[ignore]
    fn every_stock_map_imports() {
        let directory =
            std::path::Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../assets/maps");
        let Ok(entries) = std::fs::read_dir(&directory) else {
            return;
        };
        let mut names: Vec<String> = entries
            .filter_map(|entry| entry.ok())
            .filter_map(|entry| entry.file_name().into_string().ok())
            .filter_map(|name| name.strip_suffix(".map").map(str::to_owned))
            .filter(|name| name != "ui")
            .collect();
        names.sort();
        let mut failures = Vec::new();
        for name in names {
            let Some(cache) = stock_map(&name) else {
                continue;
            };
            match import_level(&cache) {
                Ok(level) => eprintln!(
                    "{name}: {} bsps, {} shaders, {} bitmaps",
                    level.structure_bsps.len(),
                    level.shaders.len(),
                    level.bitmaps.len()
                ),
                Err(error) => failures.push(format!("{name}: {error}")),
            }
        }
        assert!(failures.is_empty(), "{failures:#?}");
    }
}
