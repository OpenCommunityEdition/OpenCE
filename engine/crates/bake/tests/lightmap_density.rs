//! The lightmaps' density on Blood Gulch: each surface's size in the level
//! for each texel of its lightmap page, stock and baked, swept by shader
//! (the far hills, the cliffs, the ground, the bases' insides). No baked
//! texel is larger than its stock texel, and the texels the bake spreads
//! its paths over cover what they light. Skipped without assets/maps.

use std::collections::BTreeMap;
use std::error::Error;
use std::path::Path;

use opence_bake::chart::{page_size, MAX_FOOTPRINT};
use opence_content::level::{Level, StructureBsp};
use opence_import::{import_level, Cache};

type Result<T = ()> = std::result::Result<T, Box<dyn Error>>;

fn level(name: &str) -> Result<Option<Level>> {
    let map =
        Path::new(env!("CARGO_MANIFEST_DIR")).join(format!("../../../assets/maps/{name}.map"));
    let Ok(file) = std::fs::read(&map) else {
        return Ok(None); // (no game data here)
    };
    Ok(Some(import_level(&Cache::open(file)?)?))
}

/// A lightmapped triangle: its shader, its area in the level and the side
/// of a stock and of a baked texel on it (world units).
#[derive(Debug)]
struct Lit {
    shader: String,
    area: f32,
    stock: f32,
    baked: f32,
}

fn cross_length(a: [f32; 3], b: [f32; 3], c: [f32; 3]) -> f32 {
    let u: [f32; 3] = std::array::from_fn(|axis| b[axis] - a[axis]);
    let v: [f32; 3] = std::array::from_fn(|axis| c[axis] - a[axis]);
    let cross = [
        u[1] * v[2] - u[2] * v[1],
        u[2] * v[0] - u[0] * v[2],
        u[0] * v[1] - u[1] * v[0],
    ];
    cross.iter().map(|value| value * value).sum::<f32>().sqrt()
}

/// Every lightmapped triangle of `structure`.
fn triangles(level: &Level, structure: &StructureBsp) -> Vec<Lit> {
    let mut lit = Vec::new();
    let Some(bitmap) = structure
        .lightmap_bitmap
        .as_ref()
        .and_then(|tag| level.bitmap(tag))
    else {
        return lit;
    };
    for (page, lightmap) in structure.lightmaps.iter().enumerate() {
        let Some(image) = lightmap
            .bitmap_index
            .and_then(|index| bitmap.images.get(usize::from(index)))
        else {
            continue;
        };
        let Some((width, height)) = page_size(level, structure, page) else {
            continue;
        };
        let stock = f32::from(image.width) * f32::from(image.height);
        let baked = width as f32 * height as f32;
        for material in &lightmap.materials {
            if material.lightmap_vertices.len() != material.vertices.len() {
                continue;
            }
            let shader = material
                .shader
                .as_ref()
                .map_or_else(|| "(none)".to_owned(), |tag| tag.path.clone());
            let first = material.first_surface as usize;
            let surfaces = structure
                .surfaces
                .get(first..first.saturating_add(material.surface_count as usize))
                .unwrap_or_default();
            for surface in surfaces {
                let corner = |at: u16| {
                    let at = usize::from(at);
                    Some((
                        material.vertices.get(at)?.position,
                        material.lightmap_vertices.get(at)?.texcoord,
                    ))
                };
                let [Some(a), Some(b), Some(c)] = surface.map(corner) else {
                    continue;
                };
                let world = cross_length(a.0, b.0, c.0) * 0.5;
                let page = cross_length(
                    [a.1[0], a.1[1], 0.0],
                    [b.1[0], b.1[1], 0.0],
                    [c.1[0], c.1[1], 0.0],
                ) * 0.5;
                if world > 0.0 && page > 0.0 && world.is_finite() && page.is_finite() {
                    lit.push(Lit {
                        shader: shader.clone(),
                        area: world,
                        stock: (world / (page * stock)).sqrt(),
                        baked: (world / (page * baked)).sqrt(),
                    });
                }
            }
        }
    }
    lit
}

/// The side of texel that `share` of `lit`'s area has a texel no larger
/// than.
fn quantile(lit: &[&Lit], share: f32, side: fn(&Lit) -> f32) -> f32 {
    let mut sorted: Vec<(f32, f32)> = lit.iter().map(|lit| (side(lit), lit.area)).collect();
    sorted.sort_by(|a, b| a.0.total_cmp(&b.0));
    let total: f32 = sorted.iter().map(|(_, area)| area).sum();
    let mut seen = 0.0;
    for (side, area) in sorted {
        seen += area;
        if seen >= total * share {
            return side;
        }
    }
    0.0
}

#[test]
fn no_baked_texel_is_larger_than_stock_and_the_sweep_by_shader() -> Result {
    let Some(level) = level("bloodgulch")? else {
        return Ok(());
    };
    for structure in &level.structure_bsps {
        let lit = triangles(&level, structure);
        assert!(lit.len() > 1000, "{} lit triangles", lit.len());
        let mut shaders: BTreeMap<&str, Vec<&Lit>> = BTreeMap::new();
        for triangle in &lit {
            shaders.entry(&triangle.shader).or_default().push(triangle);
        }
        let all: Vec<&Lit> = lit.iter().collect();
        let total: f32 = lit.iter().map(|lit| lit.area).sum();
        println!(
            "{}: {} triangles, {:.0} square units; stock texel median {:.3}, p95 {:.3}; baked median {:.3}, p95 {:.3}",
            structure.path,
            lit.len(),
            total,
            quantile(&all, 0.5, |lit| lit.stock),
            quantile(&all, 0.95, |lit| lit.stock),
            quantile(&all, 0.5, |lit| lit.baked),
            quantile(&all, 0.95, |lit| lit.baked),
        );
        println!("shader | triangles | share of area | stock median | stock p95 | stock max | baked median | baked p95 | over the footprint");
        let mut rows: Vec<(&str, Vec<&Lit>)> = shaders.into_iter().collect();
        rows.sort_by(|a, b| {
            let area = |lit: &[&Lit]| lit.iter().map(|lit| lit.area).sum::<f32>();
            area(&b.1).total_cmp(&area(&a.1))
        });
        for (shader, lit) in &rows {
            let area: f32 = lit.iter().map(|lit| lit.area).sum();
            let over: f32 = lit
                .iter()
                .filter(|lit| lit.baked > MAX_FOOTPRINT)
                .map(|lit| lit.area)
                .sum();
            println!(
                "{shader} | {} | {:.1}% | {:.3} | {:.3} | {:.3} | {:.3} | {:.3} | {:.1}%",
                lit.len(),
                area / total * 100.0,
                quantile(lit, 0.5, |lit| lit.stock),
                quantile(lit, 0.95, |lit| lit.stock),
                lit.iter().map(|lit| lit.stock).fold(0.0, f32::max),
                quantile(lit, 0.5, |lit| lit.baked),
                quantile(lit, 0.95, |lit| lit.baked),
                over / area * 100.0,
            );
        }
        let larger: Vec<&&Lit> = all
            .iter()
            .filter(|lit| lit.baked > lit.stock * 1.0001)
            .collect();
        assert!(
            larger.is_empty(),
            "{} baked texels larger than stock: {:?}",
            larger.len(),
            larger.first()
        );
    }
    Ok(())
}
