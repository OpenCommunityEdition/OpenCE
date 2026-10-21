//! Objects: an object's definition, its model (the Xbox's `mode`, Custom
//! Edition's `mod2`), its animation graph and its collision model.
//!
//! A model's vertices and strips are not in its definition. The Xbox's
//! are in the tag data, which each part's vertex buffer entry and strip
//! address point into; Custom Edition's are in the file's model data,
//! which the tag index's header gives the offset and size of.

use opence_content::{
    Animation, AnimationGraph, AnimationKind, AnimationNode, Attachment, Biped, ChangeColor,
    CollisionModel, CollisionModelMaterial, CollisionNode, CollisionRegion, ColorPermutation,
    MarkerInstance, Model, ModelGeometry, ModelMarker, ModelNode, ModelPart, ModelPermutation,
    ModelRegion, ModelShader, ModelVertex, ObjectDefinition, ObjectKind, PathfindingSphere, TagRef,
    UnitSeat, WeaponClass, WeaponType,
};

use crate::cache::{Cache, CacheKind};
use crate::error::ImportError;
use crate::level::{short_coordinate, tag_ref, unit_vector};
use crate::reader;
use crate::tags::{Element, GroupTag};

pub(crate) const OBJE: GroupTag = GroupTag::new(b"obje");
pub(crate) const MODE: GroupTag = GroupTag::new(b"mode");
pub(crate) const MOD2: GroupTag = GroupTag::new(b"mod2");
pub(crate) const ANTR: GroupTag = GroupTag::new(b"antr");
pub(crate) const COLL: GroupTag = GroupTag::new(b"coll");
const SHDR: GroupTag = GroupTag::new(b"shdr");
const EFFE: GroupTag = GroupTag::new(b"effe");
const BIPD: GroupTag = GroupTag::new(b"bipd");
const UNHI: GroupTag = GroupTag::new(b"unhi");

pub(crate) const OBJECT_SIZE: usize = 0x17C;
/// A biped's whole definition, and where its own part starts (after the
/// object's and the unit's).
const BIPED_SIZE: usize = 0x4F4;
const BIPED_PART: usize = 0x2F0;
/// Where the unit part starts, after the object's.
const UNIT_PART: usize = OBJECT_SIZE;
const NAME_LONGEST: usize = 31;

pub(crate) const MODEL_SIZE: usize = 0xE8;
pub(crate) const MODEL_NODE_SIZE: usize = 0x9C;
pub(crate) const MODEL_MARKER_SIZE: usize = 0x40;
pub(crate) const MARKER_INSTANCE_SIZE: usize = 0x20;
pub(crate) const MODEL_REGION_SIZE: usize = 0x4C;
pub(crate) const MODEL_PERMUTATION_SIZE: usize = 0x58;
pub(crate) const MODEL_GEOMETRY_SIZE: usize = 0x30;
pub(crate) const MODEL_SHADER_SIZE: usize = 0x20;
pub(crate) const XBOX_PART_SIZE: usize = 0x68;
pub(crate) const CE_PART_SIZE: usize = 0x84;
const MAX_NODES: usize = 64;
const MAX_MARKERS: usize = 256;
const MAX_MARKER_INSTANCES: usize = 32;
const MAX_REGIONS: usize = 32;
const MAX_PERMUTATIONS: usize = 32;
const MAX_GEOMETRIES: usize = 256;
const MAX_PARTS: usize = 32;
const MAX_SHADERS: usize = 32;
const MAX_VERTICES: usize = 0xFFFF;
const MAX_LOCAL_NODES: usize = 22;
/// The model's flag that its parts' vertices name local nodes.
const PARTS_HAVE_LOCAL_NODES: u32 = 1 << 1;

const STRIP_TYPE: i16 = 1;
const XBOX_VERTEX_TYPE: i16 = 5;
const CE_VERTEX_TYPE: i16 = 4;
pub(crate) const XBOX_VERTEX_SIZE: usize = 32;
pub(crate) const CE_VERTEX_SIZE: usize = 68;
pub(crate) const VERTEX_BUFFER_SIZE: usize = 12;

/// Where Custom Edition's tag index header says the model data is.
pub(crate) const MODEL_DATA_OFFSET: usize = 0x14;
pub(crate) const INDEX_DATA_OFFSET: usize = 0x1C;
pub(crate) const MODEL_DATA_SIZE: usize = 0x20;

pub(crate) const ANIMATION_GRAPH_SIZE: usize = 0x80;
pub(crate) const OVERLAY_SIZE: usize = 0x14;
pub(crate) const UNIT_SEAT_SIZE: usize = 0x64;
pub(crate) const WEAPON_CLASS_SIZE: usize = 0xBC;
pub(crate) const WEAPON_TYPE_SIZE: usize = 0x3C;
pub(crate) const ANIMATION_NODE_SIZE: usize = 0x40;
pub(crate) const ANIMATION_SIZE: usize = 0xB4;
const MAX_ANIMATIONS: usize = 256;
const MAX_FRAMES: u16 = 2048;
const MAX_SEATS: usize = 64;
const MAX_ANIMATION_DATA: usize = 0x0400_0000;

pub(crate) const COLLISION_MODEL_SIZE: usize = 0x298;
pub(crate) const COLLISION_MATERIAL_SIZE: usize = 0x48;
pub(crate) const COLLISION_REGION_SIZE: usize = 0x54;
pub(crate) const COLLISION_PERMUTATION_SIZE: usize = 0x20;
pub(crate) const PATHFINDING_SPHERE_SIZE: usize = 0x20;
pub(crate) const COLLISION_NODE_SIZE: usize = 0x40;
pub(crate) const COLLISION_BSP_SIZE: usize = 0x60;
const MAX_COLLISION_BLOCK: usize = 64;

/// The kind its type field names.
fn kind(value: i16) -> Option<ObjectKind> {
    usize::try_from(value)
        .ok()
        .and_then(|index| ObjectKind::ALL.get(index).copied())
}

/// An index of `field` into a list of `len`: -1 for none.
pub(crate) fn optional_index(
    element: &Element<'_>,
    at: usize,
    field: &str,
    len: usize,
) -> Result<Option<u16>, ImportError> {
    match element.i16(at, field)? {
        -1 => Ok(None),
        value if value >= 0 && (value as usize) < len => Ok(Some(value as u16)),
        value => Err(element.fault(field, format!("is {value}, past the {len} there are"))),
    }
}

pub(crate) fn vector<const N: usize>(
    element: &Element<'_>,
    at: usize,
    field: &str,
) -> Result<[f32; N], ImportError> {
    let mut value = [0.0; N];
    for (index, component) in value.iter_mut().enumerate() {
        *component = element.f32(at + index * 4, field)?;
    }
    Ok(value)
}

pub(crate) fn reference(
    element: &Element<'_>,
    at: usize,
    field: &str,
    groups: &[GroupTag],
) -> Result<Option<TagRef>, ImportError> {
    Ok(element
        .reference(at, field, groups)?
        .map(|index| tag_ref(element.cache(), index)))
}

fn name(element: &Element<'_>) -> Result<String, ImportError> {
    Ok(element.text(0, "name", NAME_LONGEST)?.to_owned())
}

/// An object's definition: what every kind has, its kind checked against
/// its group.
pub fn import_object(cache: &Cache, index: usize) -> Result<ObjectDefinition, ImportError> {
    let object = cache.tag(index, OBJE, OBJECT_SIZE)?;
    let value = object.i16(0, "type")?;
    let kind = kind(value).ok_or_else(|| object.fault("type", format!("is {value}")))?;
    let group = GroupTag::new(&kind.group());
    if object.instance().group != group {
        return Err(object.fault("type", format!("is {kind:?}, whose group is {group}")));
    }
    let models = match cache.header().kind {
        CacheKind::Xbox => [MODE],
        CacheKind::CustomEdition => [MOD2],
    };

    let attachments_block = object.block(0x140, "attachments", 0x48, 64)?;
    let mut attachments = Vec::with_capacity(attachments_block.len());
    for attachment in attachments_block.iter() {
        attachments.push(Attachment {
            tag: reference(&attachment, 0, "type", &[])?,
            marker: attachment.text(0x10, "marker", NAME_LONGEST)?.to_owned(),
        });
    }

    let colors = object.block(0x164, "change colors", 0x2C, 4)?;
    let mut change_colors = Vec::with_capacity(colors.len());
    for color in colors.iter() {
        let permutations_block = color.block(0x20, "permutations", 0x1C, 32)?;
        let mut permutations = Vec::with_capacity(permutations_block.len());
        for permutation in permutations_block.iter() {
            permutations.push(ColorPermutation {
                weight: permutation.f32(0, "weight")?,
                lower: vector(&permutation, 0x04, "color lower bound")?,
                upper: vector(&permutation, 0x10, "color upper bound")?,
            });
        }
        change_colors.push(ChangeColor {
            darken_by: color.i16(0, "darken by")?,
            scale_by: color.i16(2, "scale by")?,
            scale_flags: color.u32(0x04, "scale flags")?,
            lower: vector(&color, 0x08, "color lower bound")?,
            upper: vector(&color, 0x14, "color upper bound")?,
            permutations,
        });
    }

    Ok(ObjectDefinition {
        tag: tag_ref(cache, index),
        kind,
        flags: object.u16(0x02, "flags")?,
        bounding_radius: object.f32(0x04, "bounding radius")?,
        bounding_offset: vector(&object, 0x08, "bounding offset")?,
        acceleration_scale: object.f32(0x20, "acceleration scale")?,
        render_bounding_radius: object.f32(0x104, "render bounding radius")?,
        model: reference(&object, 0x28, "model", &models)?,
        animation_graph: reference(&object, 0x38, "animation graph", &[ANTR])?,
        collision_model: reference(&object, 0x70, "collision model", &[COLL])?,
        physics: reference(&object, 0x80, "physics", &[crate::vehicle::PHYS])?,
        modifier_shader: reference(&object, 0x90, "modifier shader", &[SHDR])?,
        creation_effect: reference(&object, 0xA0, "creation effect", &[EFFE])?,
        attachments,
        change_colors,
        unit_hud: match kind {
            ObjectKind::Biped => unit_hud(cache, index, BIPD, BIPED_SIZE)?,
            ObjectKind::Vehicle => unit_hud(
                cache,
                index,
                crate::vehicle::VEHI,
                crate::vehicle::VEHICLE_SIZE,
            )?,
            _ => None,
        },
        biped: match kind {
            ObjectKind::Biped => Some(import_biped(cache, index)?),
            _ => None,
        },
        weapon: match kind {
            ObjectKind::Weapon => Some(crate::weapon::import_weapon(cache, index)?),
            _ => None,
        },
        projectile: match kind {
            ObjectKind::Projectile => Some(crate::weapon::import_projectile(cache, index)?),
            _ => None,
        },
        equipment: match kind {
            ObjectKind::Equipment => Some(crate::weapon::import_equipment(cache, index)?),
            _ => None,
        },
        vehicle: match kind {
            ObjectKind::Vehicle => Some(crate::vehicle::import_vehicle(cache, index)?),
            _ => None,
        },
    })
}

/// A unit's first HUD (its unit part's new HUD interfaces), its tag of
/// `group` and `size`.
fn unit_hud(
    cache: &Cache,
    index: usize,
    group: GroupTag,
    size: usize,
) -> Result<Option<TagRef>, ImportError> {
    let unit = cache.tag(index, group, size)?;
    let huds = unit.block(UNIT_PART + 0x12C, "new hud interfaces", 0x30, 2)?;
    match huds.get(0) {
        Some(hud) => reference(&hud, 0, "unit hud interface", &[UNHI]),
        None => Ok(None),
    }
}

/// A biped's movement and stance, from its own part of its definition.
fn import_biped(cache: &Cache, index: usize) -> Result<Biped, ImportError> {
    let biped = cache.tag(index, BIPD, BIPED_SIZE)?;
    let f = |at: usize, field: &str| biped.f32(BIPED_PART + at, field);
    Ok(Biped {
        flags: biped.u32(BIPED_PART + 0x04, "biped flags")?,
        moving_turning_speed: f(0x00, "moving turning speed")?,
        stationary_turning_threshold: f(0x08, "stationary turning threshold")?,
        maximum_slope_angle: f(0x68, "maximum slope angle")?,
        downhill_falloff_angle: f(0x6C, "downhill falloff angle")?,
        downhill_cutoff_angle: f(0x70, "downhill cutoff angle")?,
        downhill_velocity_scale: f(0x74, "downhill velocity scale")?,
        uphill_falloff_angle: f(0x78, "uphill falloff angle")?,
        uphill_cutoff_angle: f(0x7C, "uphill cutoff angle")?,
        uphill_velocity_scale: f(0x80, "uphill velocity scale")?,
        // (the tools that built the map left it a tick's, of the game's 30)
        jump_velocity: f(0xC4, "jump velocity")? * 30.0,
        maximum_soft_landing_time: f(0xE4, "maximum soft landing time")?,
        maximum_hard_landing_time: f(0xE8, "maximum hard landing time")?,
        minimum_soft_landing_velocity: f(0xEC, "minimum soft landing velocity")?,
        minimum_hard_landing_velocity: f(0xF0, "minimum hard landing velocity")?,
        maximum_hard_landing_velocity: f(0xF4, "maximum hard landing velocity")?,
        death_hard_landing_velocity: f(0xF8, "death hard landing velocity")?,
        standing_camera_height: f(0x110, "standing camera height")?,
        crouching_camera_height: f(0x114, "crouching camera height")?,
        crouch_transition_time: f(0x118, "crouch transition time")?,
        // (the unit part's, before the biped's: a second's already)
        grenade_velocity: biped.f32(UNIT_PART + 0x144, "grenade velocity")?,
        collision_height_standing: f(0x134, "standing collision height")?,
        collision_height_crouching: f(0x138, "crouching collision height")?,
        collision_radius: f(0x13C, "collision radius")?,
        minimum_normal_k: f(0x1E0, "minimum normal k")?,
        downhill_k0: f(0x1E4, "downhill k0")?,
        downhill_k1: f(0x1E8, "downhill k1")?,
        uphill_k0: f(0x1EC, "uphill k0")?,
        uphill_k1: f(0x1F0, "uphill k1")?,
    })
}

/// Custom Edition's model data: the vertices, then from its index data
/// offset the strips.
struct ModelData<'a> {
    vertices: &'a [u8],
    indices: &'a [u8],
    index_offset: usize,
}

fn model_data(cache: &Cache) -> Result<ModelData<'_>, ImportError> {
    let fault = |field: &'static str, reason: String| ImportError::Header { field, reason };
    let header = cache.tag_data();
    let read = |at: usize, field: &'static str| {
        reader::u32(header, at).map_err(|error| fault(field, error.to_string()))
    };
    let offset = read(MODEL_DATA_OFFSET, "model data offset")? as usize;
    let index_offset = read(INDEX_DATA_OFFSET, "index data offset")? as usize;
    let size = read(MODEL_DATA_SIZE, "model data size")? as usize;
    let bytes = reader::slice(cache.file(), offset, size)
        .map_err(|error| fault("model data size", error.to_string()))?;
    if index_offset > size {
        return Err(fault(
            "index data offset",
            format!("is {index_offset:#x}, past the {size:#x} bytes of model data"),
        ));
    }
    let (vertices, indices) = bytes.split_at(index_offset);
    Ok(ModelData {
        vertices,
        indices,
        index_offset,
    })
}

/// A model: its skeleton, markers, regions, and its geometries' parts
/// with their vertices and strips, every index checked.
pub fn import_model(cache: &Cache, index: usize) -> Result<Model, ImportError> {
    let kind = cache.header().kind;
    let group = match kind {
        CacheKind::Xbox => MODE,
        CacheKind::CustomEdition => MOD2,
    };
    let model = cache.tag(index, group, MODEL_SIZE)?;
    let flags = model.u32(0, "flags")?;

    let nodes_block = model.block(0xB8, "nodes", MODEL_NODE_SIZE, MAX_NODES)?;
    let node_count = nodes_block.len();
    let mut nodes = Vec::with_capacity(node_count);
    for node in nodes_block.iter() {
        nodes.push(ModelNode {
            name: name(&node)?,
            next_sibling: optional_index(&node, 0x20, "next sibling node", node_count)?,
            first_child: optional_index(&node, 0x22, "first child node", node_count)?,
            parent: optional_index(&node, 0x24, "parent node", node_count)?,
            translation: vector(&node, 0x28, "default translation")?,
            rotation: vector(&node, 0x34, "default rotation")?,
            distance_from_parent: node.f32(0x44, "node distance from parent")?,
            default_inverse_scale: node.f32(0x68, "default inverse matrix")?,
            default_inverse: [
                vector(&node, 0x6C, "default inverse matrix")?,
                vector(&node, 0x78, "default inverse matrix")?,
                vector(&node, 0x84, "default inverse matrix")?,
                vector(&node, 0x90, "default inverse matrix")?,
            ],
        });
    }

    let shaders_block = model.block(0xDC, "shaders", MODEL_SHADER_SIZE, MAX_SHADERS)?;
    let mut shaders = Vec::with_capacity(shaders_block.len());
    for shader in shaders_block.iter() {
        shaders.push(ModelShader {
            shader: reference(&shader, 0, "shader", &[SHDR])?,
            permutation: shader.i16(0x10, "permutation")?,
        });
    }

    let data = match kind {
        CacheKind::Xbox => None,
        CacheKind::CustomEdition => Some(model_data(cache)?),
    };
    let geometries_block = model.block(0xD0, "geometries", MODEL_GEOMETRY_SIZE, MAX_GEOMETRIES)?;
    let mut geometries = Vec::with_capacity(geometries_block.len());
    for geometry in geometries_block.iter() {
        let part_size = match kind {
            CacheKind::Xbox => XBOX_PART_SIZE,
            CacheKind::CustomEdition => CE_PART_SIZE,
        };
        let parts_block = geometry.block(0x24, "parts", part_size, MAX_PARTS)?;
        let mut parts = Vec::with_capacity(parts_block.len());
        for part in parts_block.iter() {
            let context = PartContext {
                parts: parts_block.len(),
                nodes: node_count,
                shaders: shaders.len(),
                local_nodes: flags & PARTS_HAVE_LOCAL_NODES != 0,
            };
            parts.push(import_part(&part, &context, data.as_ref())?);
        }
        geometries.push(ModelGeometry { parts });
    }

    let regions_block = model.block(0xC4, "regions", MODEL_REGION_SIZE, MAX_REGIONS)?;
    let mut regions = Vec::with_capacity(regions_block.len());
    for region in regions_block.iter() {
        let permutations_block = region.block(
            0x40,
            "permutations",
            MODEL_PERMUTATION_SIZE,
            MAX_PERMUTATIONS,
        )?;
        let mut permutations = Vec::with_capacity(permutations_block.len());
        for permutation in permutations_block.iter() {
            let mut levels = [None; 5];
            for (level, geometry) in levels.iter_mut().enumerate() {
                *geometry =
                    optional_index(&permutation, 0x40 + level * 2, "geometry", geometries.len())?;
            }
            permutations.push(ModelPermutation {
                name: name(&permutation)?,
                flags: permutation.u32(0x20, "flags")?,
                geometries: levels,
            });
        }
        regions.push(ModelRegion {
            name: name(&region)?,
            permutations,
        });
    }

    let markers_block = model.block(0xAC, "markers", MODEL_MARKER_SIZE, MAX_MARKERS)?;
    let mut markers = Vec::with_capacity(markers_block.len());
    for marker in markers_block.iter() {
        let instances_block = marker.block(
            0x34,
            "instances",
            MARKER_INSTANCE_SIZE,
            MAX_MARKER_INSTANCES,
        )?;
        let mut instances = Vec::with_capacity(instances_block.len());
        for instance in instances_block.iter() {
            let region = instance.u8(0, "region")?;
            let permutation = instance.u8(1, "permutation")?;
            let node = instance.u8(2, "node")?;
            let permutations = regions
                .get(usize::from(region))
                .map(|region| region.permutations.len())
                .ok_or_else(|| {
                    instance.fault(
                        "region",
                        format!("is {region}, past the {} regions", regions.len()),
                    )
                })?;
            if usize::from(permutation) >= permutations {
                return Err(instance.fault(
                    "permutation",
                    format!("is {permutation}, past the region's {permutations}"),
                ));
            }
            if usize::from(node) >= node_count {
                return Err(
                    instance.fault("node", format!("is {node}, past the {node_count} nodes"))
                );
            }
            instances.push(MarkerInstance {
                region,
                permutation,
                node,
                translation: vector(&instance, 0x04, "translation")?,
                rotation: vector(&instance, 0x10, "rotation")?,
            });
        }
        markers.push(ModelMarker {
            name: name(&marker)?,
            instances,
        });
    }

    let mut detail_cutoffs = [0.0; 5];
    for (level, cutoff) in detail_cutoffs.iter_mut().enumerate() {
        *cutoff = model.f32(0x08 + level * 4, "detail cutoff")?;
    }
    Ok(Model {
        tag: tag_ref(cache, index),
        flags,
        node_list_checksum: model.i32(4, "node list checksum")?,
        detail_cutoffs,
        base_map_scale: vector(&model, 0x30, "base map scale")?,
        nodes,
        markers,
        regions,
        geometries,
        shaders,
    })
}

/// What a part's indices are checked against.
struct PartContext {
    parts: usize,
    nodes: usize,
    shaders: usize,
    local_nodes: bool,
}

/// A part: its strip and vertices, from the tag data (the Xbox's) or the
/// model data (Custom Edition's).
fn import_part(
    part: &Element<'_>,
    context: &PartContext,
    data: Option<&ModelData<'_>>,
) -> Result<ModelPart, ImportError> {
    let shader = optional_index(part, 0x04, "shader index", context.shaders)?
        .ok_or_else(|| part.fault("shader index", "is none"))?;
    for (at, field) in [(0x06, "prev part"), (0x07, "next part")] {
        let value = part.u8(at, field)? as i8;
        if value != -1 && (value < 0 || value as usize >= context.parts) {
            return Err(part.fault(
                field,
                format!("is {value}, past the {} parts", context.parts),
            ));
        }
    }
    optional_index(part, 0x08, "centroid primary node", context.nodes)?;
    optional_index(part, 0x0A, "centroid secondary node", context.nodes)?;
    let strip_type = part.i16(0x44, "triangle buffer type")?;
    if strip_type != STRIP_TYPE {
        return Err(part.fault(
            "triangle buffer type",
            format!("is {strip_type}, not a strip"),
        ));
    }
    let triangles = part.i32(0x48, "strip triangle count")?;
    let strip_count = usize::try_from(triangles)
        .ok()
        .filter(|&count| (1..=MAX_VERTICES).contains(&count))
        .ok_or_else(|| part.fault("strip triangle count", format!("is {triangles}")))?
        + 2;
    let vertex_count = part.i32(0x58, "vertex count")?;
    let vertex_count = usize::try_from(vertex_count)
        .ok()
        .filter(|&count| (1..=MAX_VERTICES).contains(&count))
        .ok_or_else(|| part.fault("vertex count", format!("is {vertex_count}")))?;
    let vertex_type = part.i16(0x54, "vertex buffer type")?;

    let (strip_bytes, vertices) = match data {
        None => {
            if vertex_type != XBOX_VERTEX_TYPE {
                return Err(part.fault("vertex buffer type", format!("is {vertex_type}")));
            }
            let region = part.cache().tag_region();
            let strip = region
                .get(part.u32(0x4C, "strip")?, strip_count * 2)
                .map_err(|fault| part.fault("strip", fault))?;
            let buffer = region
                .get(part.u32(0x64, "vertex buffer")?, VERTEX_BUFFER_SIZE)
                .map_err(|fault| part.fault("vertex buffer", fault))?;
            let address =
                reader::u32(buffer, 4).map_err(|fault| part.fault("vertex buffer", fault))?;
            let bytes = region
                .get(address, vertex_count * XBOX_VERTEX_SIZE)
                .map_err(|fault| part.fault("vertex buffer", fault))?;
            let mut vertices = Vec::with_capacity(vertex_count);
            for (index, chunk) in bytes.as_chunks::<XBOX_VERTEX_SIZE>().0.iter().enumerate() {
                vertices.push(xbox_vertex(chunk).ok_or_else(|| {
                    part.fault("vertices", format!("has vertex {index}, which is not finite or names a node that is not a third"))
                })?);
            }
            (strip, vertices)
        }
        Some(data) => {
            if vertex_type != CE_VERTEX_TYPE {
                return Err(part.fault("vertex buffer type", format!("is {vertex_type}")));
            }
            let strip_offset = part.u32(0x4C, "strip offset")? as usize;
            if !(data.index_offset + strip_offset).is_multiple_of(2) {
                return Err(part.fault("strip offset", "is not aligned"));
            }
            let strip = reader::slice(data.indices, strip_offset, strip_count * 2)
                .map_err(|fault| part.fault("strip offset", fault))?;
            let vertex_offset = part.u32(0x64, "vertex offset")? as usize;
            if !vertex_offset.is_multiple_of(4) {
                return Err(part.fault("vertex offset", "is not aligned"));
            }
            let bytes = reader::slice(data.vertices, vertex_offset, vertex_count * CE_VERTEX_SIZE)
                .map_err(|fault| part.fault("vertex offset", fault))?;
            let local_count = usize::from(part.u8(0x6B, "local node count")?);
            if local_count > MAX_LOCAL_NODES {
                return Err(part.fault("local node count", format!("is {local_count}")));
            }
            let local = part.slice(0x6C, MAX_LOCAL_NODES, "local node indices")?;
            let local = &local[..local_count];
            if context.local_nodes {
                if local.is_empty() {
                    return Err(part.fault("local node count", "is 0"));
                }
                if let Some(node) = local
                    .iter()
                    .find(|&&node| usize::from(node) >= context.nodes)
                {
                    return Err(part.fault(
                        "local node indices",
                        format!("name node {node}, past the {} nodes", context.nodes),
                    ));
                }
            }
            let mut vertices = Vec::with_capacity(vertex_count);
            for (index, chunk) in bytes.as_chunks::<CE_VERTEX_SIZE>().0.iter().enumerate() {
                let mut vertex = ce_vertex(chunk).ok_or_else(|| {
                    part.fault(
                        "vertices",
                        format!("has vertex {index}, which is not finite"),
                    )
                })?;
                if context.local_nodes {
                    for node in vertex.nodes.iter_mut().flatten() {
                        *node = local.get(usize::from(*node)).map(|&node| u16::from(node)).ok_or_else(|| {
                            part.fault(
                                "vertices",
                                format!("has vertex {index}, whose node {node} is past the {local_count} local nodes"),
                            )
                        })?;
                    }
                }
                vertices.push(vertex);
            }
            (strip, vertices)
        }
    };
    if let Some((index, vertex)) = vertices.iter().enumerate().find(|(_, vertex)| {
        vertex
            .nodes
            .iter()
            .flatten()
            .any(|&node| usize::from(node) >= context.nodes)
    }) {
        return Err(part.fault(
            "vertices",
            format!(
                "has vertex {index}, whose nodes {:?} are past the {} nodes",
                vertex.nodes, context.nodes
            ),
        ));
    }
    let strip: Vec<u16> = strip_bytes
        .as_chunks::<2>()
        .0
        .iter()
        .map(|&pair| u16::from_le_bytes(pair))
        .collect();
    if let Some(index) = strip
        .iter()
        .find(|&&index| usize::from(index) >= vertex_count)
    {
        return Err(part.fault(
            "strip",
            format!("indexes vertex {index}, past the part's {vertex_count}"),
        ));
    }
    Ok(ModelPart {
        flags: part.u32(0, "flags")?,
        shader,
        centroid: vector(part, 0x14, "centroid")?,
        vertices,
        strip,
    })
}

fn f32_at(bytes: &[u8], at: usize) -> Option<f32> {
    reader::f32(bytes, at)
        .ok()
        .filter(|value| value.is_finite())
}

fn vector_at<const N: usize>(bytes: &[u8], at: usize) -> Option<[f32; N]> {
    let mut value = [0.0; N];
    for (index, component) in value.iter_mut().enumerate() {
        *component = f32_at(bytes, at + index * 4)?;
    }
    Some(value)
}

/// The Xbox's compressed model vertex (as `rasterizer_geometry.c` reads
/// it): normals packed, coordinates in shorts, nodes stored three times
/// over (negative for none), and the first node's weight in a short.
fn xbox_vertex(bytes: &[u8]) -> Option<ModelVertex> {
    let node = |at: usize| -> Option<Option<u16>> {
        let value = i16::from(*bytes.get(at)? as i8);
        if value % 3 != 0 {
            return None;
        }
        Some(u16::try_from(value / 3).ok())
    };
    let weight = (f32::from(reader::i16(bytes, 30).ok()?) / 32767.0).clamp(0.0, 1.0);
    Some(ModelVertex {
        position: vector_at(bytes, 0)?,
        normal: unit_vector(reader::u32(bytes, 12).ok()?),
        binormal: unit_vector(reader::u32(bytes, 16).ok()?),
        tangent: unit_vector(reader::u32(bytes, 20).ok()?),
        texcoord: [
            short_coordinate(reader::i16(bytes, 24).ok()?),
            short_coordinate(reader::i16(bytes, 26).ok()?),
        ],
        nodes: [node(28)?, node(29)?],
        weights: [weight, 1.0 - weight],
    })
}

/// Custom Edition's uncompressed model vertex: every field in floats, and
/// its nodes in shorts (-1 for none).
fn ce_vertex(bytes: &[u8]) -> Option<ModelVertex> {
    let node = |at: usize| -> Option<Option<u16>> {
        match reader::i16(bytes, at).ok()? {
            -1 => Some(None),
            value => u16::try_from(value).ok().map(Some),
        }
    };
    Some(ModelVertex {
        position: vector_at(bytes, 0)?,
        normal: vector_at(bytes, 12)?,
        binormal: vector_at(bytes, 24)?,
        tangent: vector_at(bytes, 36)?,
        texcoord: vector_at(bytes, 48)?,
        nodes: [node(56)?, node(58)?],
        weights: vector_at(bytes, 60)?,
    })
}

/// A block of animation indices (shorts): -1, or one out of range, is
/// none (as the port's loader repairs Custom Edition's).
fn animation_list(
    element: &Element<'_>,
    at: usize,
    field: &str,
    animations: usize,
) -> Result<Vec<Option<u16>>, ImportError> {
    let block = element.block(at, field, 2, MAX_ANIMATIONS)?;
    block
        .iter()
        .map(|entry| Ok(index_or_none(entry.i16(0, "animation")?, animations)))
        .collect()
}

fn index_or_none(value: i16, len: usize) -> Option<u16> {
    u16::try_from(value)
        .ok()
        .filter(|&value| usize::from(value) < len)
}

/// An animation graph: its nodes, its animations' frames as stored, and
/// the overlays and seats that name them.
pub fn import_animation_graph(cache: &Cache, index: usize) -> Result<AnimationGraph, ImportError> {
    let graph = cache.tag(index, ANTR, ANIMATION_GRAPH_SIZE)?;
    let nodes_block = graph.block(0x68, "nodes", ANIMATION_NODE_SIZE, MAX_NODES)?;
    let node_count = nodes_block.len();
    let mut nodes = Vec::with_capacity(node_count);
    for node in nodes_block.iter() {
        // (links out of range are none: the port's loader repairs them)
        let link = |at: usize, field: &str| {
            Ok::<_, ImportError>(index_or_none(node.i16(at, field)?, node_count))
        };
        nodes.push(AnimationNode {
            name: name(&node)?,
            next_sibling: link(0x20, "next sibling node")?,
            first_child: link(0x22, "first child node")?,
            parent: link(0x24, "parent node")?,
        });
    }

    let animations_block = graph.block(0x74, "animations", ANIMATION_SIZE, MAX_ANIMATIONS)?;
    let animation_count = animations_block.len();
    let sounds = graph
        .block(0x54, "sound references", 0x14, MAX_ANIMATIONS)?
        .len();
    let mut animations = Vec::with_capacity(animation_count);
    for animation in animations_block.iter() {
        animations.push(import_animation(&animation, animation_count, sounds)?);
    }

    let overlays_block = graph.block(0x00, "object overlays", OVERLAY_SIZE, MAX_ANIMATIONS)?;
    let mut object_overlays = Vec::with_capacity(overlays_block.len());
    for overlay in overlays_block.iter() {
        object_overlays.push(index_or_none(overlay.i16(0, "animation")?, animation_count));
    }

    let seats_block = graph.block(0x0C, "units", UNIT_SEAT_SIZE, MAX_SEATS)?;
    let mut unit_seats = Vec::with_capacity(seats_block.len());
    for seat in seats_block.iter() {
        let classes_block = seat.block(0x58, "weapons", WEAPON_CLASS_SIZE, MAX_SEATS)?;
        let mut weapon_classes = Vec::with_capacity(classes_block.len());
        for class in classes_block.iter() {
            let types_block = class.block(0xB0, "weapon types", WEAPON_TYPE_SIZE, MAX_SEATS)?;
            let mut weapon_types = Vec::with_capacity(types_block.len());
            for weapon_type in types_block.iter() {
                weapon_types.push(WeaponType {
                    label: weapon_type.text(0, "label", NAME_LONGEST)?.to_owned(),
                    animations: animation_list(&weapon_type, 0x30, "animations", animation_count)?,
                });
            }
            weapon_classes.push(WeaponClass {
                label: class.text(0, "label", NAME_LONGEST)?.to_owned(),
                animations: animation_list(&class, 0x98, "animations", animation_count)?,
                weapon_types,
            });
        }
        unit_seats.push(UnitSeat {
            label: seat.text(0, "label", NAME_LONGEST)?.to_owned(),
            animations: animation_list(&seat, 0x40, "animations", animation_count)?,
            weapon_classes,
        });
    }

    // (the hands and weapon the player sees: the first entry's animations)
    let first_person = graph.block(0x48, "first person weapons", 0x1C, 1)?;
    let first_person_weapon = match first_person.get(0) {
        Some(weapon) => animation_list(&weapon, 0x10, "animations", animation_count)?,
        None => Vec::new(),
    };

    Ok(AnimationGraph {
        tag: tag_ref(cache, index),
        flags: graph.u16(0x64, "flags")?,
        limp_body_node_radius: graph.f32(0x60, "limp body node radius")?,
        nodes,
        animations,
        object_overlays,
        unit_seats,
        first_person_weapon,
    })
}

/// An animation: its fields checked, its frames as stored.
fn import_animation(
    animation: &Element<'_>,
    animations: usize,
    sounds: usize,
) -> Result<Animation, ImportError> {
    let kind = match animation.i16(0x20, "type")? {
        0 => AnimationKind::Base,
        1 => AnimationKind::Overlay,
        2 => AnimationKind::Replacement,
        value => return Err(animation.fault("type", format!("is {value}"))),
    };
    let unsigned = |at: usize, field: &str, longest: u16| {
        let value = animation.i16(at, field)?;
        u16::try_from(value)
            .ok()
            .filter(|&value| value <= longest)
            .ok_or_else(|| animation.fault(field, format!("is {value}, not 0 to {longest}")))
    };
    let frame_count = unsigned(0x22, "frame count", MAX_FRAMES)?;
    let frame_size = unsigned(0x24, "frame size", i16::MAX as u16)?;
    let node_count = unsigned(0x2C, "node count", MAX_NODES as u16)?;
    // (kept as stored: some are the frame count, which the game clamps to
    // the last frame when it loops, and 0 means it does not loop)
    let loop_frame = unsigned(0x2E, "loop frame index", MAX_FRAMES)?;
    let flags = animation.u16(0x3A, "flags")?;
    let data = animation
        .data(0xA0, "frame data", MAX_ANIMATION_DATA)?
        .to_vec();
    let compressed_data_offset = animation.u32(0x88, "offset to compressed data")?;
    if flags & 1 != 0 {
        if compressed_data_offset as usize > data.len() {
            return Err(animation.fault(
                "offset to compressed data",
                format!(
                    "is {compressed_data_offset}, past the {} bytes of frame data",
                    data.len()
                ),
            ));
        }
    } else if data.len() < usize::from(frame_count) * usize::from(frame_size) {
        return Err(animation.fault(
            "frame data",
            format!(
                "is {} bytes, fewer than {frame_count} frames of {frame_size}",
                data.len()
            ),
        ));
    }
    let flag_pair = |at: usize, field: &str| -> Result<[u32; 2], ImportError> {
        Ok([animation.u32(at, field)?, animation.u32(at + 4, field)?])
    };
    Ok(Animation {
        name: name(animation)?,
        kind,
        frame_count,
        frame_size,
        frame_info_type: animation.i16(0x26, "frame info type")?,
        node_list_checksum: animation.i32(0x28, "node list checksum")?,
        node_count,
        loop_frame,
        weight: animation.f32(0x30, "weight")?,
        key_frame: animation.i16(0x34, "key frame index")?,
        second_key_frame: animation.i16(0x36, "second key frame index")?,
        next_animation: optional_index(animation, 0x38, "next animation", animations)?,
        // (worked out when the map was built; a bad one is none, as the
        // game's permutation choice treats it)
        parent_animation: index_or_none(animation.i16(0x42, "parent animation")?, animations),
        normalized_weight: animation.f32(0x44, "normalized weight")?,
        flags,
        sound: optional_index(animation, 0x3C, "sound", sounds)?,
        sound_frame: animation.i16(0x3E, "sound frame index")?,
        left_foot_frame: animation.u8(0x40, "left foot frame index")? as i8,
        right_foot_frame: animation.u8(0x41, "right foot frame index")? as i8,
        frame_info: animation
            .data(0x48, "frame info", MAX_ANIMATION_DATA)?
            .to_vec(),
        translation_flags: flag_pair(0x5C, "node trans flag data")?,
        rotation_flags: flag_pair(0x6C, "node rotation flag data")?,
        scale_flags: flag_pair(0x7C, "node scale flag data")?,
        compressed_data_offset,
        default_data: animation
            .data(0x8C, "default data", MAX_ANIMATION_DATA)?
            .to_vec(),
        data,
    })
}

/// A collision model: its materials, regions, pathfinding shapes, and its
/// nodes' BSPs, whose surfaces name its materials.
pub fn import_collision_model(cache: &Cache, index: usize) -> Result<CollisionModel, ImportError> {
    let collision = cache.tag(index, COLL, COLLISION_MODEL_SIZE)?;
    let materials_block = collision.block(
        0x234,
        "materials",
        COLLISION_MATERIAL_SIZE,
        MAX_COLLISION_BLOCK,
    )?;
    let mut materials = Vec::with_capacity(materials_block.len());
    for material in materials_block.iter() {
        materials.push(CollisionModelMaterial {
            name: name(&material)?,
            flags: material.u32(0x20, "flags")?,
            material_type: material.i16(0x24, "material type")?,
            shield_leak_percentage: material.f32(0x28, "shield leak percentage")?,
            shield_damage_multiplier: material.f32(0x2C, "shield damage multiplier")?,
            body_damage_multiplier: material.f32(0x3C, "body damage multiplier")?,
        });
    }

    let regions_block =
        collision.block(0x240, "regions", COLLISION_REGION_SIZE, MAX_COLLISION_BLOCK)?;
    let mut regions = Vec::with_capacity(regions_block.len());
    for region in regions_block.iter() {
        let permutations = region
            .block(
                0x48,
                "permutations",
                COLLISION_PERMUTATION_SIZE,
                MAX_COLLISION_BLOCK,
            )?
            .iter()
            .map(|permutation| name(&permutation))
            .collect::<Result<_, _>>()?;
        regions.push(CollisionRegion {
            name: name(&region)?,
            flags: region.u32(0x20, "flags")?,
            damage_threshold: region.f32(0x28, "damage threshold")?,
            permutations,
        });
    }

    let nodes_block = collision.block(0x28C, "nodes", COLLISION_NODE_SIZE, MAX_NODES)?;
    let node_count = nodes_block.len();
    let mut nodes = Vec::with_capacity(node_count);
    for node in nodes_block.iter() {
        let bsps_block = node.block(0x34, "bsps", COLLISION_BSP_SIZE, MAX_COLLISION_BLOCK)?;
        let mut bsps = Vec::with_capacity(bsps_block.len());
        for bsp in bsps_block.iter() {
            bsps.push(crate::level::import_collision(&bsp, materials.len())?);
        }
        nodes.push(CollisionNode {
            name: name(&node)?,
            region: optional_index(&node, 0x20, "region", regions.len())?,
            parent: optional_index(&node, 0x22, "parent node", node_count)?,
            next_sibling: optional_index(&node, 0x24, "next sibling node", node_count)?,
            first_child: optional_index(&node, 0x26, "first child node", node_count)?,
            bsps,
        });
    }

    let spheres_block = collision.block(
        0x280,
        "pathfinding spheres",
        PATHFINDING_SPHERE_SIZE,
        MAX_COLLISION_BLOCK,
    )?;
    let mut pathfinding_spheres = Vec::with_capacity(spheres_block.len());
    for sphere in spheres_block.iter() {
        pathfinding_spheres.push(PathfindingSphere {
            node: optional_index(&sphere, 0, "node", node_count)?,
            center: vector(&sphere, 0x10, "center")?,
            radius: sphere.f32(0x1C, "radius")?,
        });
    }

    let mut pathfinding_box = [[0.0; 2]; 3];
    for (axis, bounds) in pathfinding_box.iter_mut().enumerate() {
        for (end, bound) in bounds.iter_mut().enumerate() {
            *bound = collision.f32(0x268 + (axis * 2 + end) * 4, "pathfinding box")?;
        }
    }
    Ok(CollisionModel {
        tag: tag_ref(cache, index),
        flags: collision.u32(0x00, "flags")?,
        indirect_damage_material: optional_index(
            &collision,
            0x04,
            "indirect damage material",
            materials.len(),
        )?,
        maximum_body_vitality: collision.f32(0x08, "maximum body vitality")?,
        friendly_damage_resistance: collision.f32(0x44, "friendly damage resistance")?,
        body_damaged_threshold: collision.f32(0x94, "body damaged threshold")?,
        body_destroyed_threshold: collision.f32(0xB8, "body destroyed threshold")?,
        maximum_shield_vitality: collision.f32(0xCC, "maximum shield vitality")?,
        shield_material_type: collision.i16(0xD2, "shield material type")?,
        shield_failure_function: collision.i16(0xEC, "shield failure function")?,
        shield_failure_threshold: collision.f32(0xF0, "shield failure threshold")?,
        failing_shield_leak_fraction: collision.f32(0xF4, "failing shield leak fraction")?,
        minimum_stun_damage: collision.f32(0x108, "minimum stun damage")?,
        stun_time: collision.f32(0x10C, "stun time")?,
        recharge_time: collision.f32(0x110, "recharge time")?,
        shield_damaged_threshold: collision.f32(0x184, "shield damaged threshold")?,
        materials,
        regions,
        pathfinding_box,
        pathfinding_spheres,
        nodes,
    })
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::fake::FakeCache;
    use crate::level::tests::{block, empty, put, reference, stock_map};
    use std::error::Error;

    type Result<T = ()> = std::result::Result<T, Box<dyn Error>>;

    /// What a fake model breaks.
    #[derive(Clone, Copy, PartialEq)]
    enum Broken {
        Nothing,
        StripPastVertices,
        VertexBuffer,
        TooManyNodes,
        LocalNodePastNodes,
    }

    fn shorts(values: &[i16]) -> Vec<u8> {
        values
            .iter()
            .flat_map(|value| value.to_le_bytes())
            .collect()
    }

    fn floats(values: &[f32]) -> Vec<u8> {
        values
            .iter()
            .flat_map(|value| value.to_le_bytes())
            .collect()
    }

    const POSITIONS: [[f32; 3]; 3] = [[0.0, 0.0, 0.0], [1.0, 0.0, 0.0], [0.0, 1.0, 0.0]];
    const NORMAL: u32 = 0x1234_5678;

    /// A named element of `size` with its node links (if it has them) none.
    fn named(size: usize, name: &str) -> Vec<u8> {
        let mut bytes = vec![0; size];
        put(&mut bytes, 0, name.as_bytes());
        bytes
    }

    /// A model of two nodes (the second the first's child), one region and
    /// permutation, one geometry of one part of a triangle, a shader and a
    /// marker on the second node.
    fn model_cache(kind: CacheKind, broken: Broken) -> Result<(Cache, usize)> {
        let mut fake = FakeCache::new(kind);
        let last = if broken == Broken::StripPastVertices {
            3
        } else {
            2
        };
        let strip = shorts(&[0, 1, last]);
        let mut part = vec![
            0u8;
            if kind == CacheKind::Xbox {
                XBOX_PART_SIZE
            } else {
                CE_PART_SIZE
            }
        ];
        put(&mut part, 0x06, &[0xFF, 0xFF]);
        put(&mut part, 0x08, &shorts(&[-1, -1]));
        put(&mut part, 0x14, &floats(&[0.25, 0.25, 0.0]));
        put(&mut part, 0x44, &STRIP_TYPE.to_le_bytes());
        put(&mut part, 0x48, &1i32.to_le_bytes());
        put(&mut part, 0x58, &3i32.to_le_bytes());
        match kind {
            CacheKind::Xbox => {
                let mut vertices = Vec::new();
                for (index, position) in POSITIONS.iter().enumerate() {
                    vertices.extend(floats(position));
                    vertices.extend(NORMAL.to_le_bytes().repeat(3));
                    vertices.extend(shorts(&[index as i16 * 1000, -500]));
                    // (node 0, and none, each stored three times over)
                    vertices.extend([0, (-3i8) as u8]);
                    vertices.extend(16384i16.to_le_bytes());
                }
                let vertices = fake.add_data(&vertices);
                let mut buffer = vec![0u8; VERTEX_BUFFER_SIZE];
                let address = if broken == Broken::VertexBuffer {
                    0x1000
                } else {
                    vertices
                };
                put(&mut buffer, 4, &address.to_le_bytes());
                let buffer = fake.add_data(&buffer);
                let strip = fake.add_data(&strip);
                put(&mut part, 0x4C, &strip.to_le_bytes());
                put(&mut part, 0x54, &XBOX_VERTEX_TYPE.to_le_bytes());
                put(&mut part, 0x64, &buffer.to_le_bytes());
            }
            CacheKind::CustomEdition => {
                let mut data = Vec::new();
                for position in POSITIONS {
                    data.extend(floats(&position));
                    data.extend(floats(&[0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0]));
                    data.extend(floats(&[0.5, 0.25]));
                    data.extend(shorts(&[0, -1]));
                    data.extend(floats(&[1.0, 0.0]));
                }
                let index_offset = data.len() as u32;
                data.extend(&strip);
                let offset = fake.add_file_data(&data);
                fake.header_fields = vec![
                    (MODEL_DATA_OFFSET, offset),
                    (INDEX_DATA_OFFSET, index_offset),
                    (MODEL_DATA_SIZE, data.len() as u32),
                ];
                put(&mut part, 0x54, &CE_VERTEX_TYPE.to_le_bytes());
                // (the vertices' local node 0 is the model's node 1)
                part[0x6B] = 1;
                part[0x6C] = if broken == Broken::LocalNodePastNodes {
                    2
                } else {
                    1
                };
            }
        }
        let parts = fake.add_data(&part);
        let mut geometry = vec![0u8; MODEL_GEOMETRY_SIZE];
        block(&mut geometry, 0x24, 1, parts);
        let geometries = fake.add_data(&geometry);

        let mut nodes = named(MODEL_NODE_SIZE, "root");
        put(&mut nodes, 0x20, &shorts(&[-1, 1, -1]));
        put(&mut nodes, 0x34, &floats(&[0.0, 0.0, 0.0, 1.0]));
        let mut head = named(MODEL_NODE_SIZE, "head");
        put(&mut head, 0x20, &shorts(&[-1, -1, 0]));
        put(
            &mut head,
            0x28,
            &floats(&[0.0, 0.0, 0.5, 0.0, 0.0, 0.0, 1.0]),
        );
        nodes.extend(head);
        let nodes = fake.add_data(&nodes);

        let mut permutation = named(MODEL_PERMUTATION_SIZE, "base");
        put(&mut permutation, 0x40, &shorts(&[0, 0, 0, -1, -1]));
        let permutations = fake.add_data(&permutation);
        let mut region = named(MODEL_REGION_SIZE, "body");
        block(&mut region, 0x40, 1, permutations);
        let regions = fake.add_data(&region);

        let mut instance = vec![0u8; MARKER_INSTANCE_SIZE];
        put(&mut instance, 0, &[0, 0, 1]);
        put(&mut instance, 0x10, &floats(&[0.0, 0.0, 0.0, 1.0]));
        let instances = fake.add_data(&instance);
        let mut marker = named(MODEL_MARKER_SIZE, "head");
        block(&mut marker, 0x34, 1, instances);
        let markers = fake.add_data(&marker);

        let mut shader = vec![0u8; MODEL_SHADER_SIZE];
        empty(&mut shader, 0);
        let shaders = fake.add_data(&shader);

        let mut model = vec![0u8; MODEL_SIZE];
        if kind == CacheKind::CustomEdition {
            put(&mut model, 0, &PARTS_HAVE_LOCAL_NODES.to_le_bytes());
        }
        put(&mut model, 0x30, &floats(&[1.0, 1.0]));
        let node_count = if broken == Broken::TooManyNodes {
            MAX_NODES + 1
        } else {
            2
        };
        block(&mut model, 0xAC, 1, markers);
        block(&mut model, 0xB8, node_count, nodes);
        block(&mut model, 0xC4, 1, regions);
        block(&mut model, 0xD0, 1, geometries);
        block(&mut model, 0xDC, 1, shaders);
        let group = if kind == CacheKind::Xbox { MODE } else { MOD2 };
        let index = fake.add_tag(group, "characters\\fake\\fake", &model);
        Ok((Cache::open(fake.build())?, index))
    }

    #[test]
    fn an_xbox_model_has_its_compressed_vertices() -> Result {
        let (cache, index) = model_cache(CacheKind::Xbox, Broken::Nothing)?;
        let model = import_model(&cache, index)?;
        assert_eq!(model.nodes.len(), 2);
        assert_eq!(model.nodes[0].first_child, Some(1));
        assert_eq!(model.nodes[1].parent, Some(0));
        assert_eq!(model.nodes[1].translation, [0.0, 0.0, 0.5]);
        assert_eq!(
            model.regions[0].permutations[0].geometries,
            [Some(0), Some(0), Some(0), None, None]
        );
        assert_eq!(model.markers[0].instances[0].node, 1);
        assert_eq!(model.shaders[0].shader, None);
        let part = &model.geometries[0].parts[0];
        assert_eq!(part.strip, [0, 1, 2]);
        assert_eq!(part.vertices.len(), 3);
        for (index, vertex) in part.vertices.iter().enumerate() {
            assert_eq!(vertex.position, POSITIONS[index]);
            assert_eq!(vertex.normal, unit_vector(NORMAL));
            assert_eq!(
                vertex.texcoord,
                [
                    short_coordinate(index as i16 * 1000),
                    short_coordinate(-500)
                ]
            );
            assert_eq!(vertex.nodes, [Some(0), None]);
            assert_eq!(vertex.weights, [16384.0 / 32767.0, 1.0 - 16384.0 / 32767.0]);
        }
        Ok(())
    }

    #[test]
    fn a_custom_edition_model_has_its_vertices_from_the_model_data() -> Result {
        let (cache, index) = model_cache(CacheKind::CustomEdition, Broken::Nothing)?;
        let model = import_model(&cache, index)?;
        let part = &model.geometries[0].parts[0];
        assert_eq!(part.strip, [0, 1, 2]);
        assert_eq!(part.centroid, [0.25, 0.25, 0.0]);
        for (index, vertex) in part.vertices.iter().enumerate() {
            assert_eq!(vertex.position, POSITIONS[index]);
            assert_eq!(vertex.normal, [0.0, 0.0, 1.0]);
            assert_eq!(vertex.texcoord, [0.5, 0.25]);
            // (local node 0, the model's node 1)
            assert_eq!(vertex.nodes, [Some(1), None]);
            assert_eq!(vertex.weights, [1.0, 0.0]);
        }
        Ok(())
    }

    #[test]
    fn broken_models_are_refused() -> Result {
        for (kind, broken, field) in [
            (CacheKind::Xbox, Broken::StripPastVertices, "strip"),
            (CacheKind::CustomEdition, Broken::StripPastVertices, "strip"),
            (CacheKind::Xbox, Broken::VertexBuffer, "vertex buffer"),
            (CacheKind::Xbox, Broken::TooManyNodes, "nodes"),
            (
                CacheKind::CustomEdition,
                Broken::LocalNodePastNodes,
                "local node indices",
            ),
        ] {
            let (cache, index) = model_cache(kind, broken)?;
            match import_model(&cache, index) {
                Err(ImportError::Tag { field: refused, .. }) => {
                    assert!(refused.ends_with(field), "{kind:?}: {refused}, not {field}");
                }
                other => panic!("{kind:?}: {other:?}"),
            }
        }
        Ok(())
    }

    #[test]
    fn a_custom_edition_model_needs_its_model_data() -> Result {
        let mut fake = FakeCache::new(CacheKind::CustomEdition);
        fake.header_fields = vec![(MODEL_DATA_OFFSET, 0x10_0000), (MODEL_DATA_SIZE, 16)];
        let mut model = vec![0u8; MODEL_SIZE];
        block(&mut model, 0xD0, 0, 0);
        let index = fake.add_tag(MOD2, "fake", &model);
        let cache = Cache::open(fake.build())?;
        assert!(matches!(
            import_model(&cache, index),
            Err(ImportError::Header {
                field: "model data size",
                ..
            })
        ));
        Ok(())
    }

    /// A scenery's definition (or one whose type is `kind`) naming a model.
    fn object_cache(kind: i16) -> Result<(Cache, usize)> {
        let mut fake = FakeCache::new(CacheKind::Xbox);
        let model = fake.add_tag(MODE, "scenery\\rock\\rock", &[0; 4]);
        let mut permutation = vec![0u8; 0x1C];
        put(
            &mut permutation,
            0,
            &floats(&[1.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6]),
        );
        let permutations = fake.add_data(&permutation);
        let mut color = vec![0u8; 0x2C];
        put(&mut color, 0, &shorts(&[1, 2]));
        block(&mut color, 0x20, 1, permutations);
        let colors = fake.add_data(&color);
        let mut attachment = vec![0u8; 0x48];
        empty(&mut attachment, 0);
        put(&mut attachment, 0x10, b"smoke");
        let attachments = fake.add_data(&attachment);

        let mut object = vec![0u8; OBJECT_SIZE];
        put(&mut object, 0, &kind.to_le_bytes());
        put(&mut object, 0x04, &floats(&[2.0, 0.0, 0.0, 1.0]));
        reference(&mut object, 0x28, MODE, fake.handle(model));
        for at in [0x38, 0x70, 0x80, 0x90, 0xA0] {
            empty(&mut object, at);
        }
        put(&mut object, 0x104, &3.0f32.to_le_bytes());
        block(&mut object, 0x140, 1, attachments);
        block(&mut object, 0x164, 1, colors);
        let index = fake.add_tag(GroupTag::new(b"scen"), "scenery\\rock\\rock", &object);
        fake.set_parent(index, OBJE);
        Ok((Cache::open(fake.build())?, index))
    }

    #[test]
    fn an_object_has_its_kind_tags_and_colors() -> Result {
        let (cache, index) = object_cache(6)?;
        let object = import_object(&cache, index)?;
        assert_eq!(object.kind, ObjectKind::Scenery);
        assert_eq!(object.bounding_radius, 2.0);
        assert_eq!(object.bounding_offset, [0.0, 0.0, 1.0]);
        assert_eq!(object.render_bounding_radius, 3.0);
        assert_eq!(
            object.model.map(|model| model.path),
            Some("scenery\\rock\\rock".to_owned())
        );
        assert_eq!(object.animation_graph, None);
        assert_eq!(object.attachments[0].marker, "smoke");
        let color = &object.change_colors[0];
        assert_eq!([color.darken_by, color.scale_by], [1, 2]);
        assert_eq!(color.permutations[0].weight, 1.0);
        assert_eq!(color.permutations[0].upper, [0.4, 0.5, 0.6]);
        Ok(())
    }

    #[test]
    fn an_object_whose_type_is_not_its_group_is_refused() -> Result {
        for kind in [0, 12, -1] {
            let (cache, index) = object_cache(kind)?;
            assert!(
                matches!(import_object(&cache, index), Err(ImportError::Tag { ref field, .. }) if field == "type"),
                "{kind}"
            );
        }
        Ok(())
    }

    fn animation(
        name: &str,
        frames: i16,
        size: i16,
        flags: u16,
        data: u32,
        data_size: usize,
    ) -> Vec<u8> {
        let mut bytes = named(ANIMATION_SIZE, name);
        put(&mut bytes, 0x22, &shorts(&[frames, size]));
        put(&mut bytes, 0x2C, &shorts(&[1]));
        put(&mut bytes, 0x38, &shorts(&[-1]));
        put(&mut bytes, 0x3A, &flags.to_le_bytes());
        put(&mut bytes, 0x3C, &shorts(&[-1]));
        put(&mut bytes, 0xA0, &(data_size as i32).to_le_bytes());
        put(&mut bytes, 0xAC, &data.to_le_bytes());
        bytes
    }

    fn graph_cache(frame_data: usize) -> Result<(Cache, usize)> {
        let mut fake = FakeCache::new(CacheKind::Xbox);
        let mut node = named(ANIMATION_NODE_SIZE, "root");
        // (a parent past the nodes, which is repaired to none)
        put(&mut node, 0x20, &shorts(&[-1, -1, 5]));
        let nodes = fake.add_data(&node);
        let frames = fake.add_data(&[7; 8]);
        let mut animations = animation("idle", 2, 4, 0, frames, frame_data);
        put(&mut animations, 0x38, &shorts(&[1]));
        animations.extend(animation("aim", 3, 4, 1, frames, 4));
        let animations = fake.add_data(&animations);
        let overlays: Vec<u8> = [1i16, 7]
            .iter()
            .flat_map(|animation| {
                let mut overlay = vec![0u8; OVERLAY_SIZE];
                put(&mut overlay, 0, &animation.to_le_bytes());
                overlay
            })
            .collect();
        let overlays = fake.add_data(&overlays);
        let seat_animations = fake.add_data(&shorts(&[0, -1, 9]));
        let mut seat = named(UNIT_SEAT_SIZE, "driver");
        block(&mut seat, 0x40, 3, seat_animations);
        let seats = fake.add_data(&seat);

        let mut graph = vec![0u8; ANIMATION_GRAPH_SIZE];
        block(&mut graph, 0x00, 2, overlays);
        block(&mut graph, 0x0C, 1, seats);
        put(&mut graph, 0x60, &0.5f32.to_le_bytes());
        block(&mut graph, 0x68, 1, nodes);
        block(&mut graph, 0x74, 2, animations);
        let index = fake.add_tag(ANTR, "characters\\fake\\fake", &graph);
        Ok((Cache::open(fake.build())?, index))
    }

    #[test]
    fn an_animation_graph_has_its_animations_and_repairs_its_indices() -> Result {
        let (cache, index) = graph_cache(8)?;
        let graph = import_animation_graph(&cache, index)?;
        assert_eq!(graph.limp_body_node_radius, 0.5);
        assert_eq!(graph.nodes[0].name, "root");
        assert_eq!(graph.nodes[0].parent, None);
        assert_eq!(graph.object_overlays, [Some(1), None]);
        assert_eq!(graph.unit_seats[0].label, "driver");
        assert_eq!(graph.unit_seats[0].animations, [Some(0), None, None]);
        let [idle, aim] = graph.animations.as_slice() else {
            panic!("{} animations", graph.animations.len());
        };
        assert_eq!(
            (idle.kind, idle.frame_count, idle.frame_size),
            (AnimationKind::Base, 2, 4)
        );
        assert_eq!(idle.next_animation, Some(1));
        assert_eq!(idle.data, [7; 8]);
        assert!(!idle.is_compressed());
        assert!(aim.is_compressed());
        assert_eq!(aim.data, [7; 4]);
        Ok(())
    }

    #[test]
    fn an_animation_short_of_its_frames_is_refused() -> Result {
        let (cache, index) = graph_cache(6)?;
        assert!(matches!(
            import_animation_graph(&cache, index),
            Err(ImportError::Tag { ref field, .. }) if field.ends_with("frame data")
        ));
        Ok(())
    }

    fn collision_cache(region: i16) -> Result<(Cache, usize)> {
        let mut fake = FakeCache::new(CacheKind::Xbox);
        let mut material = named(COLLISION_MATERIAL_SIZE, "flesh");
        put(&mut material, 0x24, &shorts(&[3]));
        put(&mut material, 0x2C, &2.0f32.to_le_bytes());
        put(&mut material, 0x3C, &1.5f32.to_le_bytes());
        let materials = fake.add_data(&material);
        let mut permutations = named(COLLISION_PERMUTATION_SIZE, "base");
        permutations.extend(named(COLLISION_PERMUTATION_SIZE, "damaged"));
        let permutations = fake.add_data(&permutations);
        let mut collision_region = named(COLLISION_REGION_SIZE, "body");
        put(&mut collision_region, 0x20, &2u32.to_le_bytes());
        put(&mut collision_region, 0x28, &50.0f32.to_le_bytes());
        block(&mut collision_region, 0x48, 2, permutations);
        let regions = fake.add_data(&collision_region);
        let mut node = named(COLLISION_NODE_SIZE, "root");
        put(&mut node, 0x20, &shorts(&[region, -1, -1, -1]));
        let nodes = fake.add_data(&node);
        let mut sphere = vec![0u8; PATHFINDING_SPHERE_SIZE];
        put(&mut sphere, 0x10, &floats(&[0.0, 0.0, 0.5, 0.25]));
        let spheres = fake.add_data(&sphere);

        let mut collision = vec![0u8; COLLISION_MODEL_SIZE];
        put(&mut collision, 0x04, &shorts(&[-1]));
        put(&mut collision, 0x08, &100.0f32.to_le_bytes());
        put(&mut collision, 0xCC, &75.0f32.to_le_bytes());
        put(&mut collision, 0x108, &floats(&[5.0, 1.0, 2.0]));
        block(&mut collision, 0x234, 1, materials);
        block(&mut collision, 0x240, 1, regions);
        put(
            &mut collision,
            0x268,
            &floats(&[-1.0, 1.0, -2.0, 2.0, 0.0, 3.0]),
        );
        block(&mut collision, 0x280, 1, spheres);
        block(&mut collision, 0x28C, 1, nodes);
        let index = fake.add_tag(COLL, "characters\\fake\\fake", &collision);
        Ok((Cache::open(fake.build())?, index))
    }

    #[test]
    fn a_collision_model_has_its_materials_regions_and_nodes() -> Result {
        let (cache, index) = collision_cache(0)?;
        let collision = import_collision_model(&cache, index)?;
        assert_eq!(collision.maximum_body_vitality, 100.0);
        assert_eq!(collision.maximum_shield_vitality, 75.0);
        assert_eq!(collision.indirect_damage_material, None);
        assert_eq!(
            (
                collision.minimum_stun_damage,
                collision.stun_time,
                collision.recharge_time
            ),
            (5.0, 1.0, 2.0)
        );
        assert_eq!(collision.regions[0].flags, 2);
        assert_eq!(collision.regions[0].damage_threshold, 50.0);
        assert_eq!(collision.node_material(0), None);
        assert_eq!(collision.materials[0].name, "flesh");
        assert_eq!(collision.materials[0].material_type, 3);
        assert_eq!(collision.materials[0].shield_damage_multiplier, 2.0);
        assert_eq!(collision.materials[0].body_damage_multiplier, 1.5);
        assert_eq!(collision.regions[0].permutations, ["base", "damaged"]);
        assert_eq!(
            collision.pathfinding_box,
            [[-1.0, 1.0], [-2.0, 2.0], [0.0, 3.0]]
        );
        assert_eq!(collision.pathfinding_spheres[0].node, Some(0));
        assert_eq!(collision.pathfinding_spheres[0].radius, 0.25);
        assert_eq!(collision.nodes[0].region, Some(0));
        assert_eq!(collision.nodes[0].parent, None);
        assert!(collision.nodes[0].bsps.is_empty());
        assert!(import_collision_model(&collision_cache(1)?.0, index).is_err());
        Ok(())
    }

    #[test]
    fn bloodgulchs_objects_import_when_the_maps_are_here() -> Result {
        let Some(cache) = stock_map("bloodgulch") else {
            return Ok(());
        };
        let mut counts = [0usize; 4];
        for (index, tag) in cache.tags().iter().enumerate() {
            let label = format!("{} '{}'", tag.group, tag.name);
            if tag.is(OBJE) {
                import_object(&cache, index).map_err(|error| format!("{label}: {error}"))?;
                counts[0] += 1;
            } else if tag.is(MODE) {
                let model =
                    import_model(&cache, index).map_err(|error| format!("{label}: {error}"))?;
                assert!(!model.nodes.is_empty(), "{label}");
                counts[1] += 1;
            } else if tag.is(ANTR) {
                import_animation_graph(&cache, index)
                    .map_err(|error| format!("{label}: {error}"))?;
                counts[2] += 1;
            } else if tag.is(COLL) {
                import_collision_model(&cache, index)
                    .map_err(|error| format!("{label}: {error}"))?;
                counts[3] += 1;
            }
        }
        assert!(counts.iter().all(|&count| count > 0), "{counts:?}");

        let cyborg = "characters\\cyborg\\cyborg";
        let biped = cache
            .find(GroupTag::new(b"bipd"), cyborg)
            .ok_or("no cyborg")?;
        let object = import_object(&cache, biped)?;
        assert_eq!(object.kind, ObjectKind::Biped);
        let movement = object.biped.ok_or("no biped part")?;
        eprintln!("cyborg: {movement:?}");
        assert!(movement.flags & Biped::USES_PLAYER_PHYSICS != 0);
        assert!(movement.jump_velocity > 1.0 && movement.jump_velocity < 4.0);
        assert!(movement.minimum_normal_k > 0.0 && movement.minimum_normal_k < 1.0);
        let globals = crate::globals::import_globals(&cache)?;
        eprintln!("globals: {globals:?}");
        assert!((globals.player.run_acceleration - 0.32 * 30.0).abs() < 0.5);
        assert!(globals.multiplayer_unit.is_some());
        let graph = cache.find(ANTR, cyborg).ok_or("no cyborg graph")?;
        let graph = import_animation_graph(&cache, graph)?;
        assert_eq!(
            (
                graph.object_overlays.len(),
                graph.unit_seats.len(),
                graph.nodes.len(),
                graph.animations.len()
            ),
            (0, 24, 19, 195)
        );
        let collision = cache.find(COLL, cyborg).ok_or("no cyborg collision")?;
        let collision = import_collision_model(&cache, collision)?;
        assert_eq!(
            (
                collision.materials.len(),
                collision.regions.len(),
                collision.pathfinding_spheres.len(),
                collision.nodes.len()
            ),
            (4, 1, 1, 19)
        );
        let model = cache.find(MODE, cyborg).ok_or("no cyborg model")?;
        let model = import_model(&cache, model)?;
        assert_eq!(model.nodes.len(), 19);
        let part = &model.geometries[0].parts[0];
        assert_eq!(&part.strip[..4], [0, 1, 2, 3]);
        Ok(())
    }
}
