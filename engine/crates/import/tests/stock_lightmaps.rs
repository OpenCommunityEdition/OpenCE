//! Every stock map's lightmaps: each BSP's pages import with light in them,
//! and each lit surface has its lightmap page and coordinates; the six BSPs
//! the game shipped with no lightmaps are the only ones without. Skipped
//! without assets/maps.

use std::error::Error;
use std::path::Path;

use opence_content::{Pixels, ShaderClass};
use opence_import::{import_level, Cache};

/// The BSPs the game shipped with no lightmaps (drawn lit from their
/// normals, as the game's no_lightmap mode draws them).
const UNBAKED: [&str; 6] = [
    "b40 bsp 6",
    "b40 bsp 7",
    "b40 bsp 12",
    "c40 bsp 3",
    "c40 bsp 4",
    "d40 bsp 9",
];

/// What is wrong with a map's lightmaps, one line each, and its BSPs with
/// none.
fn faults(
    name: &str,
    file: Vec<u8>,
    unbaked: &mut Vec<String>,
) -> Result<Vec<String>, Box<dyn Error>> {
    let level = import_level(&Cache::open(file)?)?;
    let mut faults = Vec::new();
    for (index, bsp) in level.structure_bsps.iter().enumerate() {
        let at = format!("{name} bsp {index}");
        if bsp.lightmap_bitmap.is_none() {
            unbaked.push(at.clone());
        }
        let bitmap = bsp
            .lightmap_bitmap
            .as_ref()
            .and_then(|tag| level.bitmap(tag));
        for (number, lightmap) in bsp.lightmaps.iter().enumerate() {
            let Some(page) = lightmap.bitmap_index else {
                continue;
            };
            let Some(bitmap) = bitmap else {
                faults.push(format!("{at}: page {page} but no lightmap bitmap"));
                break;
            };
            let texture = bitmap
                .images
                .get(usize::from(page))
                .map(|image| &image.pixels);
            let Some(Pixels::Texture(texture)) = texture else {
                faults.push(format!(
                    "{at}: lightmap {number}'s page {page} is not a texture"
                ));
                continue;
            };
            let lit = texture
                .levels
                .first()
                .is_some_and(|level| level.iter().any(|&byte| byte != 0));
            if !lit {
                faults.push(format!("{at}: page {page} is black"));
            }
            for (which, material) in lightmap.materials.iter().enumerate() {
                let environment = material
                    .shader
                    .as_ref()
                    .and_then(|shader| level.shader(shader))
                    .is_some_and(|shader| matches!(shader.class, ShaderClass::Environment(_)));
                if environment
                    && !material.vertices.is_empty()
                    && material.lightmap_vertices.len() != material.vertices.len()
                {
                    faults.push(format!(
                        "{at}: lightmap {number} material {which}: {} of {} vertices lit",
                        material.lightmap_vertices.len(),
                        material.vertices.len()
                    ));
                }
            }
        }
    }
    Ok(faults)
}

#[test]
fn every_stock_bsp_is_lightmapped() -> Result<(), Box<dyn Error>> {
    let maps = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../assets/maps");
    let Ok(entries) = std::fs::read_dir(&maps) else {
        return Ok(()); // (no game data here)
    };
    let mut names: Vec<_> = entries
        .filter_map(|entry| entry.ok().map(|entry| entry.path()))
        .filter(|path| path.extension().is_some_and(|extension| extension == "map"))
        .filter(|path| path.file_stem().is_some_and(|stem| stem != "ui"))
        .collect();
    names.sort();
    let mut all = Vec::new();
    let mut unbaked = Vec::new();
    for path in names {
        let name = path
            .file_stem()
            .map(|stem| stem.to_string_lossy().into_owned())
            .unwrap_or_default();
        all.extend(faults(&name, std::fs::read(&path)?, &mut unbaked)?);
    }
    assert!(all.is_empty(), "{} faults:\n{}", all.len(), all.join("\n"));
    if unbaked.iter().any(|at| at.starts_with("b40 ")) {
        assert_eq!(unbaked, UNBAKED);
    }
    Ok(())
}
