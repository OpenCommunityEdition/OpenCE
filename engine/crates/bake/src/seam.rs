//! The lightmap seams: where two triangles of one surface meet in the level
//! but not on the pages.
//!
//! The stock lightmaps lay a level's surfaces out in charts, and a flat
//! floor is often cut into several: its triangles meet edge to edge in the
//! level, while their lightmap coordinates lie apart (on another chart, or
//! another page). Each side of such an edge is traced at its own texels, so
//! the two sides' light differs by the tracer's noise, and the dilation
//! copies only each side's own points outward; the edge shows as a line
//! across the floor. The original's radiosity lit both sides of a seam
//! alike.
//!
//! [`Seam::find`] finds the seams, [`stitch`] brings the light on their two
//! sides together (fully at the edge, a quarter of the way a texel in, so
//! that no step is left a texel on), and [`SeamStep::measure`] and
//! [`stock_steps`] measure the step left across each, on the baked and the
//! stock pages.

use opence_content::level::StructureBsp;
use rustc_hash::FxHashMap;

use crate::chart::{Atlas, Chart, EMPTY};
use crate::mood::{brightness, shown, StockPage};
use crate::output::{from_half, BakedPage};

/// The least cosine between the two sides' facings for an edge to be a
/// seam (a fold in the surface is not one).
pub const SAME_FACING: f32 = 0.9;
/// How far apart, in baked texels, two sides' lightmap coordinates may be
/// at an edge's end and still be one chart.
const CONTINUOUS: f32 = 0.5;
/// How finely positions are compared, in world units: two corners nearer
/// than this are one.
const WELD: f32 = 1.0 / 256.0;
/// The most points a seam is sampled at.
const MOST_SAMPLES: usize = 4096;

/// One side of a seam: where its edge lies on its page.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Side {
    /// The chart, in the atlas.
    pub chart: u32,
    /// The lightmap coordinates of the edge's two ends, in the seam's order.
    pub ends: [[f32; 2]; 2],
    /// The way into the side's triangle, across the edge, in the page's
    /// texels (unit).
    pub inward: [f32; 2],
}

/// A seam: an edge two triangles of one surface share in the level, whose
/// lightmap coordinates differ.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Seam {
    /// Its two ends in the level.
    pub ends: [[f32; 3]; 2],
    /// The way the surface faces (unit).
    pub normal: [f32; 3],
    /// Its two sides.
    pub sides: [Side; 2],
}

/// The step in light across a seam.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct SeamStep {
    /// The seam.
    pub seam: Seam,
    /// Its length in the level.
    pub length: f32,
    /// The mean step in brightness across it on the baked pages, as the
    /// level shader shows them (0 to 1). None if no sample had a texel on
    /// both sides.
    pub baked: Option<f32>,
    /// The same on the stock pages, as stored.
    pub stock: Option<f32>,
}

/// A triangle's edge, as seen from the triangle.
#[derive(Clone, Copy)]
struct Edge {
    ends: [[f32; 3]; 2],
    normal: [f32; 3],
    side: Side,
}

impl Seam {
    /// The seams of `structure` on `atlas`'s charts (laid out from it).
    pub fn find(structure: &StructureBsp, atlas: &Atlas) -> Vec<Self> {
        let mut edges: FxHashMap<[[i64; 3]; 2], Vec<Edge>> = FxHashMap::default();
        for (chart_index, chart) in atlas.charts.iter().enumerate() {
            let Some(lightmap) = structure.lightmaps.get(chart.page as usize) else {
                continue;
            };
            let size = [chart.width as f32, chart.height as f32];
            for material in &lightmap.materials {
                if material.lightmap_vertices.len() != material.vertices.len() {
                    continue;
                }
                let first = material.first_surface as usize;
                let count = material.surface_count as usize;
                let surfaces = structure.surfaces.get(first..first.saturating_add(count));
                for surface in surfaces.unwrap_or_default() {
                    let corner = |at: u16| {
                        let at = usize::from(at);
                        Some((
                            material.vertices.get(at)?,
                            material.lightmap_vertices.get(at)?.texcoord,
                        ))
                    };
                    let [Some(a), Some(b), Some(c)] = surface.map(corner) else {
                        continue;
                    };
                    let corners = [a, b, c];
                    let Some(normal) = normalize(
                        corners
                            .iter()
                            .fold([0.0; 3], |sum, (vertex, _)| add(sum, vertex.normal)),
                    ) else {
                        continue;
                    };
                    for (from, to, third) in [(0, 1, 2), (1, 2, 0), (2, 0, 1)] {
                        let mut ends = [corners[from], corners[to]];
                        let mut key = ends.map(|(vertex, _)| weld(vertex.position));
                        if key[1] < key[0] {
                            ends.swap(0, 1);
                            key.swap(0, 1);
                        }
                        let texels = ends.map(|(_, texcoord)| scale(texcoord, size));
                        let Some(inward) = inward(texels, scale(corners[third].1, size)) else {
                            continue;
                        };
                        if ends
                            .iter()
                            .flat_map(|(v, t)| v.position.iter().chain(t))
                            .any(|v| !v.is_finite())
                        {
                            continue;
                        }
                        edges.entry(key).or_default().push(Edge {
                            ends: ends.map(|(vertex, _)| vertex.position),
                            normal,
                            side: Side {
                                chart: chart_index as u32,
                                ends: ends.map(|(_, texcoord)| texcoord),
                                inward,
                            },
                        });
                    }
                }
            }
        }
        let mut seams: Vec<Self> = edges
            .into_values()
            .filter_map(|edge| {
                let [a, b] = edge.as_slice() else {
                    return None;
                };
                if dot(a.normal, b.normal) < SAME_FACING {
                    return None;
                }
                if a.side.chart == b.side.chart {
                    let chart = &atlas.charts[a.side.chart as usize];
                    let size = [chart.width as f32, chart.height as f32];
                    let apart = (0..2).map(|end| {
                        length2(sub2(
                            scale(a.side.ends[end], size),
                            scale(b.side.ends[end], size),
                        ))
                    });
                    if apart.fold(0.0f32, f32::max) <= CONTINUOUS * CONTINUOUS {
                        return None;
                    }
                }
                Some(Self {
                    ends: a.ends,
                    normal: normalize(add(a.normal, b.normal)).unwrap_or(a.normal),
                    sides: [a.side, b.side],
                })
            })
            .collect();
        // (the map's order is not the same from run to run)
        seams.sort_by(|a, b| {
            a.ends
                .partial_cmp(&b.ends)
                .unwrap_or(std::cmp::Ordering::Equal)
        });
        seams
    }

    /// Its length in the level.
    pub fn length(&self) -> f32 {
        length(sub(self.ends[1], self.ends[0]))
    }

    /// How many points along it to sample `per_texel` times a texel of its
    /// longer side on `charts`.
    fn samples(&self, charts: &[Chart], per_texel: f32) -> usize {
        let longest = self
            .sides
            .iter()
            .map(|side| {
                let chart = &charts[side.chart as usize];
                let size = [chart.width as f32, chart.height as f32];
                length2(sub2(scale(side.ends[1], size), scale(side.ends[0], size))).sqrt()
            })
            .fold(0.0f32, f32::max);
        ((longest * per_texel).ceil() as usize).clamp(1, MOST_SAMPLES)
    }
}

/// The texel of a page `width` by `height` that `side`'s edge crosses at
/// `along` (0 to 1 from its first end), `depth` texels into its triangle.
fn texel_at(side: &Side, width: u32, height: u32, along: f32, depth: f32) -> Option<(u32, u32)> {
    let size = [width as f32, height as f32];
    let [a, b] = side.ends.map(|end| scale(end, size));
    let x = a[0] + (b[0] - a[0]) * along + side.inward[0] * depth;
    let y = a[1] + (b[1] - a[1]) * along + side.inward[1] * depth;
    // (texel centres at whole numbers less a half: a texel holds the
    // coordinates from its corner to the next)
    let (x, y) = (x.floor(), y.floor());
    (x >= 0.0 && y >= 0.0 && x < size[0] && y < size[1]).then_some((x as u32, y as u32))
}

/// The point at `side`'s texel at `along`, `depth` texels in, if it faces
/// the seam's way.
fn point_at(atlas: &Atlas, seam: &Seam, side: &Side, along: f32, depth: f32) -> Option<usize> {
    let chart = atlas.charts.get(side.chart as usize)?;
    let (x, y) = texel_at(side, chart.width, chart.height, along, depth)?;
    let point = *chart.texels.get((y * chart.width + x) as usize)?;
    if point == EMPTY {
        return None;
    }
    let normal = atlas.points.get(point as usize)?.normal;
    (dot(normal, seam.normal) >= SAME_FACING).then_some(point as usize)
}

/// How far into each side [`stitch`] reaches, in texels, and how far it
/// brings each texel's light to the other side's there.
const STITCH: [(f32, f32); 2] = [(0.5, 0.5), (1.5, 0.25)];

/// `irradiance` (one to a point of `atlas`) with the light on each seam's
/// two sides brought together: each point at the edge takes the mean of
/// its light and the other side's, and each a texel in a quarter of the
/// other side's. A point on several seams takes the mean of their other
/// sides, by the most any brings it. The occlusion is kept.
pub fn stitch(atlas: &Atlas, seams: &[Seam], irradiance: &[[f32; 4]]) -> Vec<[f32; 4]> {
    let mut other = vec![([0.0f32; 3], 0u32, 0.0f32); irradiance.len()];
    for seam in seams {
        let samples = seam.samples(&atlas.charts, 2.0);
        for sample in 0..samples {
            let along = (sample as f32 + 0.5) / samples as f32;
            for (depth, pull) in STITCH {
                let [a, b] = seam
                    .sides
                    .map(|side| point_at(atlas, seam, &side, along, depth));
                let (Some(a), Some(b)) = (a, b) else { continue };
                if a == b || a >= irradiance.len() || b >= irradiance.len() {
                    continue;
                }
                if !finite(irradiance[a]) || !finite(irradiance[b]) {
                    continue;
                }
                for (this, that) in [(a, b), (b, a)] {
                    let (sum, count, most) = &mut other[this];
                    for channel in 0..3 {
                        sum[channel] += irradiance[that][channel];
                    }
                    *count += 1;
                    *most = most.max(pull);
                }
            }
        }
    }
    irradiance
        .iter()
        .zip(&other)
        .map(|(&light, &(sum, count, pull))| {
            if count == 0 {
                return light;
            }
            let mut stitched = light;
            for channel in 0..3 {
                let mean = sum[channel] / count as f32;
                stitched[channel] = light[channel] * (1.0 - pull) + mean * pull;
            }
            stitched
        })
        .collect()
}

/// The mean step in brightness across `seam` at its edge (half a texel
/// into each side), `per_texel` samples to a texel of its longer side,
/// each side's brightness given by `light` from its chart index and texel.
fn step(
    seam: &Seam,
    sizes: impl Fn(u32) -> Option<(u32, u32)>,
    light: impl Fn(u32, u32, u32) -> Option<f32>,
    samples: usize,
) -> Option<f32> {
    let (mut sum, mut count) = (0.0f64, 0u32);
    for sample in 0..samples {
        let along = (sample as f32 + 0.5) / samples as f32;
        let sides = seam.sides.map(|side| {
            let (width, height) = sizes(side.chart)?;
            let (x, y) = texel_at(&side, width, height, along, 0.5)?;
            light(side.chart, x, y)
        });
        if let [Some(a), Some(b)] = sides {
            sum += f64::from((a - b).abs());
            count += 1;
        }
    }
    (count > 0).then(|| (sum / f64::from(count)) as f32)
}

impl SeamStep {
    /// The step across each of `seams` on the baked `pages` (one to a chart
    /// of `atlas`), with each's step on the stock pages (`stock`, from
    /// [`stock_steps`]).
    pub fn measure(
        atlas: &Atlas,
        seams: &[Seam],
        pages: &[BakedPage],
        stock: &[Option<f32>],
    ) -> Vec<Self> {
        seams
            .iter()
            .enumerate()
            .map(|(index, seam)| {
                let baked = step(
                    seam,
                    |chart| {
                        pages
                            .get(chart as usize)
                            .map(|page| (page.width, page.height))
                    },
                    |chart, x, y| {
                        let chart_texels = &atlas.charts.get(chart as usize)?.texels;
                        let page = pages.get(chart as usize)?;
                        let at = (y * page.width + x) as usize;
                        if chart_texels.get(at) == Some(&EMPTY) {
                            return None;
                        }
                        let texel = page.irradiance.get(at * 4..at * 4 + 3)?;
                        let color = [0, 1, 2].map(|channel| {
                            let value = from_half(texel[channel]);
                            shown(if value.is_finite() { value } else { 0.0 }).min(1.0)
                        });
                        Some(brightness(color))
                    },
                    seam.samples(&atlas.charts, 1.0),
                );
                Self {
                    seam: *seam,
                    length: seam.length(),
                    baked,
                    stock: stock.get(index).copied().flatten(),
                }
            })
            .collect()
    }
}

/// The step across each of `seams` on the `stock` pages (one to a chart of
/// `atlas`, if it has one), as stored.
pub fn stock_steps(atlas: &Atlas, seams: &[Seam], stock: &[Option<StockPage>]) -> Vec<Option<f32>> {
    seams
        .iter()
        .map(|seam| {
            step(
                seam,
                |chart| {
                    let page = stock.get(chart as usize)?.as_ref()?;
                    Some((page.width, page.height))
                },
                |chart, x, y| {
                    let page = stock.get(chart as usize)?.as_ref()?;
                    page.light(x, y).map(brightness)
                },
                seam.samples(&atlas.charts, 1.0),
            )
        })
        .collect()
}

/// `position` on the weld's grid.
fn weld(position: [f32; 3]) -> [i64; 3] {
    position.map(|value| (value / WELD).round() as i64)
}

/// The way from the edge `ends` into its triangle, towards `third`, across
/// the edge (unit). None if the triangle has no area on the page.
fn inward(ends: [[f32; 2]; 2], third: [f32; 2]) -> Option<[f32; 2]> {
    let along = sub2(ends[1], ends[0]);
    let to_third = sub2(third, ends[0]);
    let squared = length2(along);
    if squared <= f32::MIN_POSITIVE {
        return None;
    }
    let share = (to_third[0] * along[0] + to_third[1] * along[1]) / squared;
    let across = [
        to_third[0] - along[0] * share,
        to_third[1] - along[1] * share,
    ];
    let length = length2(across).sqrt();
    (length > 1e-6 && length.is_finite()).then(|| [across[0] / length, across[1] / length])
}

fn finite(light: [f32; 4]) -> bool {
    light[..3].iter().all(|channel| channel.is_finite())
}

fn scale(texcoord: [f32; 2], size: [f32; 2]) -> [f32; 2] {
    [texcoord[0] * size[0], texcoord[1] * size[1]]
}

fn sub2(a: [f32; 2], b: [f32; 2]) -> [f32; 2] {
    [a[0] - b[0], a[1] - b[1]]
}

fn length2(a: [f32; 2]) -> f32 {
    a[0] * a[0] + a[1] * a[1]
}

fn add(a: [f32; 3], b: [f32; 3]) -> [f32; 3] {
    [a[0] + b[0], a[1] + b[1], a[2] + b[2]]
}

fn sub(a: [f32; 3], b: [f32; 3]) -> [f32; 3] {
    [a[0] - b[0], a[1] - b[1], a[2] - b[2]]
}

fn dot(a: [f32; 3], b: [f32; 3]) -> f32 {
    a[0] * b[0] + a[1] * b[1] + a[2] * b[2]
}

fn length(a: [f32; 3]) -> f32 {
    dot(a, a).sqrt()
}

fn normalize(a: [f32; 3]) -> Option<[f32; 3]> {
    let length = length(a);
    (length > 1e-6 && length.is_finite()).then(|| a.map(|value| value / length))
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::output::half;
    use opence_content::level::{EnvironmentVertex, Lightmap, LightmapVertex, Material};

    /// A BSP of one page holding a quad from (0, 0) to (8, 8) on z = 0, cut
    /// along its diagonal into two triangles: the corners `positions` with
    /// their `texcoords` and `normals`, and the `surfaces`.
    fn bsp(
        positions: &[[f32; 3]],
        normals: &[[f32; 3]],
        texcoords: &[[f32; 2]],
        surfaces: Vec<[u16; 3]>,
    ) -> StructureBsp {
        let mut structure = opence_content::devmap::plane(4.0, 1);
        let count = surfaces.len() as u32;
        structure.surfaces = surfaces;
        structure.lightmaps = vec![Lightmap {
            bitmap_index: Some(0),
            materials: vec![Material {
                shader: None,
                first_surface: 0,
                surface_count: count,
                vertices: positions
                    .iter()
                    .zip(normals)
                    .map(|(&position, &normal)| EnvironmentVertex {
                        position,
                        normal,
                        binormal: [0.0; 3],
                        tangent: [0.0; 3],
                        texcoord: [0.0; 2],
                    })
                    .collect(),
                lightmap_vertices: texcoords
                    .iter()
                    .map(|&texcoord| LightmapVertex {
                        incident_direction: [0.0, 0.0, 1.0],
                        texcoord,
                    })
                    .collect(),
            }],
        }];
        structure
    }

    const UP: [f32; 3] = [0.0, 0.0, 1.0];

    /// The quad with each triangle on its own island of the page: the
    /// first on the left half, the second on the right.
    fn cut_quad() -> StructureBsp {
        bsp(
            &[
                [0.0, 0.0, 0.0],
                [8.0, 0.0, 0.0],
                [8.0, 8.0, 0.0],
                [0.0, 0.0, 0.0],
                [8.0, 8.0, 0.0],
                [0.0, 8.0, 0.0],
            ],
            &[UP; 6],
            &[
                [0.05, 0.05],
                [0.45, 0.05],
                [0.45, 0.45],
                [0.55, 0.05],
                [0.95, 0.45],
                [0.55, 0.45],
            ],
            vec![[0, 1, 2], [3, 4, 5]],
        )
    }

    fn laid_out(structure: &StructureBsp) -> Atlas {
        let mut atlas = Atlas::default();
        atlas.lay_out(structure, 0, 32, 32);
        atlas
    }

    #[test]
    fn an_edge_whose_sides_lie_apart_on_the_page_is_a_seam() {
        let structure = cut_quad();
        let atlas = laid_out(&structure);
        let seams = Seam::find(&structure, &atlas);
        assert_eq!(seams.len(), 1, "{seams:?}");
        let seam = &seams[0];
        assert_eq!(seam.ends, [[0.0, 0.0, 0.0], [8.0, 8.0, 0.0]]);
        assert_eq!(seam.normal, UP);
        assert!((seam.length() - 8.0 * 2f32.sqrt()).abs() < 1e-4);
        // each side's way in leads away from the diagonal, into its own
        // triangle: the first's towards (0.45, 0.05), the second's towards
        // (0.55, 0.45)
        let [first, second] = seam.sides;
        assert_eq!(first.ends, [[0.05, 0.05], [0.45, 0.45]]);
        assert!(first.inward[0] > 0.0 && first.inward[1] < 0.0, "{first:?}");
        assert!(
            second.inward[0] < 0.0 && second.inward[1] > 0.0,
            "{second:?}"
        );
    }

    #[test]
    fn an_edge_laid_out_in_one_piece_is_not_a_seam() {
        let positions = [
            [0.0, 0.0, 0.0],
            [8.0, 0.0, 0.0],
            [8.0, 8.0, 0.0],
            [0.0, 8.0, 0.0],
        ];
        let texcoords = positions.map(|[x, y, _]| [0.05 + x / 9.0, 0.05 + y / 9.0]);
        let structure = bsp(&positions, &[UP; 4], &texcoords, vec![[0, 1, 2], [0, 2, 3]]);
        assert!(Seam::find(&structure, &laid_out(&structure)).is_empty());
    }

    #[test]
    fn a_fold_is_not_a_seam() {
        // the second triangle stands up from the diagonal
        let mut structure = cut_quad();
        let material = &mut structure.lightmaps[0].materials[0];
        material.vertices[5].position = [4.0, 4.0, 8.0];
        let side = [
            core::f32::consts::FRAC_1_SQRT_2,
            -core::f32::consts::FRAC_1_SQRT_2,
            0.0,
        ];
        for vertex in &mut material.vertices[3..] {
            vertex.normal = side;
        }
        assert!(Seam::find(&structure, &laid_out(&structure)).is_empty());
    }

    /// The light of `atlas`'s points: `left` on the first triangle's, `right`
    /// on the second's.
    fn two_sided(atlas: &Atlas, left: f32, right: f32) -> Vec<[f32; 4]> {
        atlas
            .points
            .iter()
            .map(|point| {
                let value = if point.position[0] > point.position[1] {
                    left
                } else {
                    right
                };
                [value, value, value, 1.0]
            })
            .collect()
    }

    fn page(atlas: &Atlas, light: &[[f32; 4]]) -> BakedPage {
        let chart = &atlas.charts[0];
        BakedPage {
            page: 0,
            width: chart.width,
            height: chart.height,
            irradiance: chart
                .fill(light, [0.0; 4])
                .iter()
                .flat_map(|t| t.map(half))
                .collect(),
            directions: Vec::new(),
        }
    }

    #[test]
    fn stitching_closes_the_step_across_a_seam() {
        let structure = cut_quad();
        let atlas = laid_out(&structure);
        let seams = Seam::find(&structure, &atlas);
        let light = two_sided(&atlas, 1.0, 3.0);
        let before = SeamStep::measure(&atlas, &seams, &[page(&atlas, &light)], &[None]);
        let stitched = stitch(&atlas, &seams, &light);
        let after = SeamStep::measure(&atlas, &seams, &[page(&atlas, &stitched)], &[None]);
        let (Some(before), Some(after)) = (before[0].baked, after[0].baked) else {
            panic!("the seam's step was not measured: {before:?} {after:?}");
        };
        assert!(before > 0.1, "{before}");
        assert!(after < before * 0.05, "{after} against {before}");
        // the points away from the seam keep their light, and the ones a
        // texel in are brought a quarter of the way
        assert!(stitched.iter().any(|t| t[0] == 1.0) && stitched.iter().any(|t| t[0] == 3.0));
        assert!(
            stitched.iter().any(|t| (t[0] - 1.5).abs() < 1e-4),
            "a texel in"
        );
        assert!(
            stitched.iter().all(|t| t[3] == 1.0),
            "the occlusion is kept"
        );
    }

    #[test]
    fn an_even_light_is_kept_and_a_lost_one_is_not_spread() {
        let structure = cut_quad();
        let atlas = laid_out(&structure);
        let seams = Seam::find(&structure, &atlas);
        let even = two_sided(&atlas, 2.0, 2.0);
        assert_eq!(stitch(&atlas, &seams, &even), even);
        let all_lost: Vec<[f32; 4]> = vec![[f32::NAN, 0.0, 0.0, 1.0]; even.len()];
        assert!(stitch(&atlas, &seams, &all_lost)
            .iter()
            .all(|t| t[0].is_nan()));
    }

    #[test]
    fn the_stock_step_is_read_from_the_stock_page() {
        let structure = cut_quad();
        let atlas = laid_out(&structure);
        let seams = Seam::find(&structure, &atlas);
        // a stock page dark on its left half and white on its right
        let texels = (0..16 * 16)
            .map(|at| {
                if at % 16 < 8 {
                    [0, 0, 0, 255]
                } else {
                    [255, 255, 255, 255]
                }
            })
            .collect();
        let stock = StockPage {
            width: 16,
            height: 16,
            texels,
        };
        let steps = stock_steps(&atlas, &seams, &[Some(stock)]);
        assert!(
            steps[0].is_some_and(|step| (step - 1.0).abs() < 1e-4),
            "{steps:?}"
        );
        assert_eq!(stock_steps(&atlas, &seams, &[None]), vec![None]);
    }
}
