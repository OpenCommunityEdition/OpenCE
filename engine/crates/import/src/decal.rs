//! Decals (`deca`): the pictures the level lays on its surfaces, and the
//! decals each chains to.

use std::collections::BTreeSet;

use opence_content::{Decal, LevelDecal, StructureBsp, TagRef};

use crate::cache::Cache;
use crate::error::ImportError;
use crate::level::{tag_index, tag_ref, Reached};
use crate::tags::{Element, GroupTag};

pub(crate) const DECA: GroupTag = GroupTag::new(b"deca");
pub(crate) const DECAL_SIZE: usize = 0x10C;
/// The scenario's decal palette (`scenario.decal_palette`).
pub(crate) const SCENARIO_DECAL_PALETTE_OFFSET: usize = 0x3B4;
pub(crate) const DECAL_PALETTE_SIZE: usize = 0x10;
const MAX_DECAL_PALETTE: usize = 128;
/// A BSP's runtime decals (`structure_bsp.runtime_decals`).
pub(crate) const BSP_RUNTIME_DECALS_OFFSET: usize = 0x258;
pub(crate) const BSP_RUNTIME_DECAL_SIZE: usize = 0x10;
const MAX_BSP_RUNTIME_DECALS: usize = 6144;
/// The longest chain followed (the original stops at none: a chain that
/// comes back on itself is cut where it does).
const MAX_CHAIN: usize = 16;

fn pair(element: &Element<'_>, at: usize, field: &str) -> Result<[f32; 2], ImportError> {
    Ok([element.f32(at, field)?, element.f32(at + 4, field)?])
}

fn rgb(element: &Element<'_>, at: usize, field: &str) -> Result<[f32; 3], ImportError> {
    Ok([
        element.f32(at, field)?,
        element.f32(at + 4, field)?,
        element.f32(at + 8, field)?,
    ])
}

/// A decal tag, its bitmap reached.
pub(crate) fn import_decal(
    cache: &Cache,
    index: usize,
    reached: &mut Reached,
) -> Result<Decal, ImportError> {
    let decal = cache.tag(index, DECA, DECAL_SIZE)?;
    Ok(Decal {
        tag: tag_ref(cache, index),
        flags: decal.u16(0x00, "flags")?,
        kind: decal.i16(0x02, "type")?,
        layer: decal.i16(0x04, "layer")?,
        next: crate::object::reference(&decal, 0x08, "next decal in chain", &[DECA])?,
        radius: pair(&decal, 0x18, "radius")?,
        intensity: pair(&decal, 0x2C, "intensity")?,
        color: [
            rgb(&decal, 0x34, "color lower bounds")?,
            rgb(&decal, 0x40, "color upper bounds")?,
        ],
        animation_loop_frame: decal.i16(0x58, "animation loop frame")?,
        animation_speed: decal.i16(0x5A, "animation speed")?,
        lifetime: pair(&decal, 0x78, "lifetime")?,
        decay_time: pair(&decal, 0x80, "decay time")?,
        framebuffer_blend_function: decal.i16(0xC0, "framebuffer blend function")?,
        map: reached.bitmap(&decal, 0xD8, "map")?,
        maximum_sprite_extent: decal.f32(0xFC, "maximum sprite extent")?,
    })
}

/// The scenario's decal palette.
pub(crate) fn decal_palette(scenario: &Element<'_>) -> Result<Vec<Option<TagRef>>, ImportError> {
    let block = scenario.block(
        SCENARIO_DECAL_PALETTE_OFFSET,
        "decal palette",
        DECAL_PALETTE_SIZE,
        MAX_DECAL_PALETTE,
    )?;
    let mut palette = Vec::with_capacity(block.len());
    for element in block.iter() {
        palette.push(crate::object::reference(&element, 0, "reference", &[DECA])?);
    }
    Ok(palette)
}

/// A BSP's runtime decals.
pub(crate) fn runtime_decals(bsp: &Element<'_>) -> Result<Vec<LevelDecal>, ImportError> {
    let block = bsp.block(
        BSP_RUNTIME_DECALS_OFFSET,
        "runtime decals",
        BSP_RUNTIME_DECAL_SIZE,
        MAX_BSP_RUNTIME_DECALS,
    )?;
    let mut decals = Vec::with_capacity(block.len());
    for element in block.iter() {
        decals.push(LevelDecal {
            position: [
                element.f32(0x0, "position")?,
                element.f32(0x4, "position")?,
                element.f32(0x8, "position")?,
            ],
            palette_index: element.u8(0xC, "decal type")?,
            yaw: element.u8(0xE, "yaw")? as i8,
            pitch: element.u8(0xF, "pitch")? as i8,
        });
    }
    Ok(decals)
}

/// Every decal the palette names and the decals they chain to, each once,
/// by path; their bitmaps reached. (The palette is imported whole: the
/// BSPs' decals index it.)
pub(crate) fn import_decals(
    cache: &Cache,
    palette: &[Option<TagRef>],
    structure_bsps: &[StructureBsp],
    reached: &mut Reached,
) -> Result<Vec<Decal>, ImportError> {
    let mut decals: Vec<Decal> = Vec::new();
    let mut imported: BTreeSet<TagRef> = BTreeSet::new();
    let used = structure_bsps.iter().any(|bsp| !bsp.decals.is_empty());
    for start in palette.iter().flatten().filter(|_| used) {
        let mut tag = Some(start.clone());
        for _ in 0..MAX_CHAIN {
            let Some(current) = tag.take() else { break };
            if !imported.insert(current.clone()) {
                break;
            }
            let Some(index) = tag_index(cache, &current) else {
                break;
            };
            let decal = import_decal(cache, index, reached)?;
            tag = decal.next.clone();
            decals.push(decal);
        }
    }
    decals.sort_by(|a, b| a.tag.path.cmp(&b.tag.path));
    Ok(decals)
}
