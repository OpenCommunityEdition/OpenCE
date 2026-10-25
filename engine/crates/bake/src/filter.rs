//! The traced light with its fireflies held down.
//!
//! A small, bright emitter is found by few of a texel's paths. A rare path
//! that meets it leaves a speck far brighter than the texels about it, and
//! texels that each catch one leave a blotch. The stock lightmaps, baked by
//! radiosity, have neither. Each point's light is held to [`FIREFLY`]
//! times the mean of the points about it on its page that face its way,
//! so that a speck takes its neighbours' light; light that is bright over
//! a patch (a lamp's pool) is kept. The light lost is given back by the
//! BSP's exposure ([`crate::mood::fitted_exposure`]), which is fitted after.

use rayon::prelude::*;

use crate::chart::{Atlas, Chart, EMPTY};

/// How many times brighter than its neighbours' mean a point may be.
pub const FIREFLY: f32 = 4.0;
/// How many texels each way the neighbours are taken from.
pub const REACH: i64 = 2;
/// The least cosine between a neighbour's normal and the point's.
const SAME_FACING: f32 = 0.8;

/// `irradiance` (one to a point of `atlas`) with each point held to
/// [`FIREFLY`] times its neighbours' mean, and a point whose light is not
/// finite given their mean colour. A point no page covers, or one with no
/// neighbour facing its way, is kept as it is.
pub fn hold_fireflies(atlas: &Atlas, irradiance: &[[f32; 4]]) -> Vec<[f32; 4]> {
    // (a point is held at the first texel that shows it, chart by chart:
    // which texel that is is settled first, so that the charts can be held
    // at once, and the light is the same on any number of threads)
    let mut seen = vec![false; irradiance.len()];
    let firsts: Vec<Vec<(i64, i64, usize)>> = atlas
        .charts
        .iter()
        .map(|chart| {
            let (width, height) = (i64::from(chart.width), i64::from(chart.height));
            let mut firsts = Vec::new();
            for y in 0..height {
                for x in 0..width {
                    let Some(&point) = chart.texels.get((y * width + x) as usize) else {
                        continue;
                    };
                    let point = point as usize;
                    if point >= irradiance.len() || seen[point] || atlas.points.get(point).is_none()
                    {
                        continue;
                    }
                    seen[point] = true;
                    firsts.push((x, y, point));
                }
            }
            firsts
        })
        .collect();
    let changes: Vec<Vec<(usize, [f32; 4])>> = atlas
        .charts
        .par_iter()
        .zip(&firsts)
        .map(|(chart, firsts)| {
            firsts
                .iter()
                .filter_map(|&(x, y, point)| {
                    held(atlas, chart, irradiance, x, y, point).map(|light| (point, light))
                })
                .collect()
        })
        .collect();
    let mut held = irradiance.to_vec();
    for (point, light) in changes.into_iter().flatten() {
        held[point] = light;
    }
    held
}

/// The light of `point`, at `x`, `y` on `chart`, held to its neighbours'
/// mean, or none if it is kept as it is.
fn held(
    atlas: &Atlas,
    chart: &Chart,
    irradiance: &[[f32; 4]],
    x: i64,
    y: i64,
    point: usize,
) -> Option<[f32; 4]> {
    let (width, height) = (i64::from(chart.width), i64::from(chart.height));
    let normal = atlas.points.get(point)?.normal;
    let (mut sum, mut colour, mut count) = (0.0, [0.0f32; 3], 0u32);
    let mut counted = Vec::new();
    for ny in (y - REACH).max(0)..(y + REACH + 1).min(height) {
        for nx in (x - REACH).max(0)..(x + REACH + 1).min(width) {
            let Some(&other) = chart.texels.get((ny * width + nx) as usize) else {
                continue;
            };
            if other == EMPTY || other as usize == point || counted.contains(&other) {
                continue;
            }
            let (Some(near), Some(light)) = (
                atlas.points.get(other as usize),
                irradiance.get(other as usize),
            ) else {
                continue;
            };
            if dot(near.normal, normal) < SAME_FACING {
                continue;
            }
            if !finite(*light) {
                continue;
            }
            counted.push(other);
            sum += luminance(*light);
            for channel in 0..3 {
                colour[channel] += light[channel];
            }
            count += 1;
        }
    }
    if count == 0 {
        return None;
    }
    let [r, g, b, a] = irradiance[point];
    if !finite(irradiance[point]) {
        let mean = colour.map(|channel| channel / count as f32);
        return Some([
            mean[0],
            mean[1],
            mean[2],
            if a.is_finite() { a } else { 1.0 },
        ]);
    }
    let limit = FIREFLY * sum / count as f32;
    let own = luminance(irradiance[point]);
    (own > limit).then(|| {
        let scale = limit / own;
        [r * scale, g * scale, b * scale, a]
    })
}

fn luminance([r, g, b, _]: [f32; 4]) -> f32 {
    0.2126 * r + 0.7152 * g + 0.0722 * b
}

fn finite(light: [f32; 4]) -> bool {
    light[..3].iter().all(|channel| channel.is_finite())
}

fn dot(a: [f32; 3], b: [f32; 3]) -> f32 {
    a[0] * b[0] + a[1] * b[1] + a[2] * b[2]
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::chart::{BakePoint, Chart};

    fn atlas(width: u32, height: u32, normals: &[[f32; 3]]) -> Atlas {
        Atlas {
            charts: vec![Chart {
                page: 0,
                width,
                height,
                texels: (0..width * height).collect(),
            }],
            points: normals
                .iter()
                .map(|&normal| BakePoint {
                    position: [0.0; 3],
                    normal,
                    footprint: 0.0,
                })
                .collect(),
        }
    }

    const UP: [f32; 3] = [0.0, 0.0, 1.0];
    const SIDE: [f32; 3] = [1.0, 0.0, 0.0];

    #[test]
    fn a_speck_takes_its_neighbours_light() {
        let atlas = atlas(5, 5, &[UP; 25]);
        let mut light = vec![[1.0, 1.0, 1.0, 1.0]; 25];
        light[12] = [100.0, 100.0, 100.0, 0.5];
        let held = hold_fireflies(&atlas, &light);
        assert!((held[12][0] - FIREFLY).abs() < 1e-4, "{:?}", held[12]);
        assert_eq!(held[12][3], 0.5, "the occlusion is kept");
        assert!(held
            .iter()
            .enumerate()
            .all(|(i, t)| i == 12 || *t == light[i]));
    }

    #[test]
    fn a_bright_patch_is_kept() {
        let atlas = atlas(5, 5, &[UP; 25]);
        let light: Vec<[f32; 4]> = (0..25)
            .map(|i| {
                if i % 5 < 3 {
                    [20.0, 20.0, 20.0, 1.0]
                } else {
                    [1.0, 1.0, 1.0, 1.0]
                }
            })
            .collect();
        assert_eq!(hold_fireflies(&atlas, &light), light);
    }

    #[test]
    fn a_neighbour_facing_away_is_not_counted() {
        // a lit wall's texel beside a dark floor's: the floor faces away
        let mut normals = [SIDE; 9];
        normals[4] = UP;
        normals[1] = UP;
        let atlas = atlas(3, 3, &normals);
        let mut light = vec![[0.0, 0.0, 0.0, 1.0]; 9];
        light[4] = [10.0, 10.0, 10.0, 1.0];
        light[1] = [8.0, 8.0, 8.0, 1.0];
        let held = hold_fireflies(&atlas, &light);
        assert_eq!(held[4], light[4]);
    }

    #[test]
    fn a_point_on_two_texels_is_held_once() {
        // a dilated texel repeats its neighbour's point
        let mut atlas = atlas(3, 1, &[UP; 3]);
        atlas.charts[0].texels = vec![0, 0, 1];
        let light = vec![[10.0, 10.0, 10.0, 1.0], [1.0, 1.0, 1.0, 1.0], [0.0; 4]];
        let held = hold_fireflies(&atlas, &light);
        assert!((held[0][0] - FIREFLY).abs() < 1e-4, "{:?}", held[0]);
    }

    #[test]
    fn a_non_finite_light_takes_its_neighbours_colour() {
        let atlas = atlas(3, 1, &[UP; 3]);
        let light = vec![
            [1.0, 0.0, 0.0, 1.0],
            [f32::NAN, f32::INFINITY, 0.0, 1.0],
            [3.0, 0.0, 0.0, 1.0],
        ];
        let held = hold_fireflies(&atlas, &light);
        assert_eq!(held[1], [2.0, 0.0, 0.0, 1.0]);
    }
}
