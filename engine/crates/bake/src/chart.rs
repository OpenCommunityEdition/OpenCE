//! The lightmap pages' texels, as points to bake the light at.
//!
//! Each page the stock lightmaps have is baked again at twice its size,
//! in the same layout: every triangle's lightmap coordinates are laid on
//! the page, and each texel its triangles cover becomes a point on the
//! surface (where it is, which way it faces). Texels just outside the
//! triangles take a neighbour's point, so that filtering at a chart's edge
//! does not reach into the dark.
//!
//! A BSP shipped without lightmaps has no page layout to bake into; its
//! surfaces are lit from the probe grid instead ([`crate::probe`]).

use opence_content::level::{Level, StructureBsp};

/// How much larger a baked page is than its stock page, each way.
pub const PAGE_SCALE: u32 = 2;
/// The largest baked page, each way.
pub const MAX_PAGE_SIZE: u32 = 2048;
/// How many texels outward the charts are grown.
pub const DILATION: u32 = 2;
/// A texel no point lights.
pub const EMPTY: u32 = u32::MAX;
/// The widest a point's footprint is taken to be (world units), so that a
/// triangle squeezed to a sliver on the page does not spread one texel's
/// light across the level.
pub const MAX_FOOTPRINT: f32 = 1.0;

/// A point on a surface to bake the light at.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct BakePoint {
    /// Where it is.
    pub position: [f32; 3],
    /// The way it faces (unit).
    pub normal: [f32; 3],
    /// How far about it on its surface the texel it lights reaches: the
    /// side of a texel there (world units, at most [`MAX_FOOTPRINT`]). The
    /// tracers start each of its paths at a point within this distance, so
    /// that a texel holds the light over the surface it covers rather than
    /// at its centre, and a shadow's edge falls across texels smoothly
    /// instead of in steps. 0: a point (a probe's).
    pub footprint: f32,
}

/// One baked lightmap page: which point lights each texel.
#[derive(Clone, Debug, PartialEq)]
pub struct Chart {
    /// The page, in the BSP's lightmaps.
    pub page: u32,
    /// Its width in texels.
    pub width: u32,
    /// Its height in texels.
    pub height: u32,
    /// Each texel's point, row by row from the top ([`EMPTY`]: none).
    pub texels: Vec<u32>,
}

/// The BSP's baked pages and the points their texels are lit by.
#[derive(Clone, Debug, Default, PartialEq)]
pub struct Atlas {
    /// The pages that are baked.
    pub charts: Vec<Chart>,
    /// The points, as the charts' texels index them.
    pub points: Vec<BakePoint>,
}

impl Atlas {
    /// The pages of `level`'s BSP `bsp` that have a stock page to follow,
    /// laid out. Empty if there is no such BSP.
    pub fn of(level: &Level, bsp: usize) -> Self {
        let mut atlas = Self::default();
        let Some(structure) = level.structure_bsps.get(bsp) else {
            return atlas;
        };
        for (page, _) in structure.lightmaps.iter().enumerate() {
            if let Some((width, height)) = page_size(level, structure, page) {
                atlas.lay_out(structure, page, width, height);
            }
        }
        atlas
    }

    /// The page `page` of `structure` laid out at `width` by `height`.
    pub fn lay_out(&mut self, structure: &StructureBsp, page: usize, width: u32, height: u32) {
        let Some(lightmap) = structure.lightmaps.get(page) else {
            return;
        };
        let mut chart = Chart {
            page: page as u32,
            width,
            height,
            texels: vec![EMPTY; width as usize * height as usize],
        };
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
                        material.lightmap_vertices.get(at)?,
                    ))
                };
                if let [Some(a), Some(b), Some(c)] = surface.map(corner) {
                    let corners = [a, b, c].map(|(vertex, light)| Corner {
                        position: vertex.position,
                        normal: vertex.normal,
                        texcoord: light.texcoord,
                    });
                    self.rasterize(&mut chart, corners);
                }
            }
        }
        chart.dilate(DILATION);
        self.charts.push(chart);
    }

    /// Claims the texels whose centres `corners`' triangle covers (the
    /// first triangle to cover a texel keeps it), or the texel at its
    /// middle if it covers no centre.
    fn rasterize(&mut self, chart: &mut Chart, corners: [Corner; 3]) {
        let (width, height) = (chart.width as f32, chart.height as f32);
        // (texel centres at whole numbers)
        let at = corners.map(|corner| {
            [
                corner.texcoord[0] * width - 0.5,
                corner.texcoord[1] * height - 0.5,
            ]
        });
        if at.iter().flatten().any(|value| !value.is_finite())
            || corners.iter().any(|corner| {
                corner
                    .position
                    .iter()
                    .chain(&corner.normal)
                    .any(|v| !v.is_finite())
            })
        {
            return;
        }
        let area = edge(at[0], at[1], at[2]);
        let footprint = footprint(&corners, area);
        let mut claimed = false;
        if area.abs() > f32::EPSILON {
            let low = |axis: usize| at.iter().map(|p| p[axis]).fold(f32::INFINITY, f32::min);
            let high = |axis: usize| at.iter().map(|p| p[axis]).fold(f32::NEG_INFINITY, f32::max);
            let x0 = libm::ceilf(low(0)).max(0.0) as u32;
            let y0 = libm::ceilf(low(1)).max(0.0) as u32;
            let x1 = (libm::floorf(high(0)).min(width - 1.0)).max(-1.0) as i64;
            let y1 = (libm::floorf(high(1)).min(height - 1.0)).max(-1.0) as i64;
            for y in i64::from(y0)..=y1 {
                for x in i64::from(x0)..=x1 {
                    let p = [x as f32, y as f32];
                    let weights = [
                        edge(at[1], at[2], p) / area,
                        edge(at[2], at[0], p) / area,
                        edge(at[0], at[1], p) / area,
                    ];
                    if weights.iter().all(|weight| *weight >= -1e-5) {
                        claimed |=
                            self.claim(chart, x as u32, y as u32, &corners, weights, footprint);
                    }
                }
            }
        }
        if !claimed {
            let middle = |axis: usize| (at[0][axis] + at[1][axis] + at[2][axis]) / 3.0;
            let (x, y) = (libm::roundf(middle(0)), libm::roundf(middle(1)));
            if (0.0..width).contains(&x) && (0.0..height).contains(&y) {
                self.claim(
                    chart,
                    x as u32,
                    y as u32,
                    &corners,
                    [1.0 / 3.0; 3],
                    footprint,
                );
            }
        }
    }

    /// Gives the texel at `x`, `y` a point at `weights` between `corners`
    /// reaching `footprint` about it, if it has none; whether it did.
    fn claim(
        &mut self,
        chart: &mut Chart,
        x: u32,
        y: u32,
        corners: &[Corner; 3],
        weights: [f32; 3],
        footprint: f32,
    ) -> bool {
        let index = y as usize * chart.width as usize + x as usize;
        match chart.texels.get_mut(index) {
            Some(texel) if *texel == EMPTY => {
                let blend = |field: fn(&Corner) -> [f32; 3]| -> [f32; 3] {
                    std::array::from_fn(|axis| {
                        corners
                            .iter()
                            .zip(weights)
                            .map(|(corner, weight)| field(corner)[axis] * weight)
                            .sum()
                    })
                };
                let normal = blend(|corner| corner.normal);
                let length = normal.iter().map(|v| v * v).sum::<f32>().sqrt();
                let normal = if length > 0.0 {
                    normal.map(|v| v / length)
                } else {
                    [0.0, 0.0, 1.0]
                };
                *texel = self.points.len() as u32;
                self.points.push(BakePoint {
                    position: blend(|corner| corner.position),
                    normal,
                    footprint,
                });
                true
            }
            _ => false,
        }
    }

    /// The points as the GPU reads them: each its position and its
    /// footprint, then its normal and a word of padding, little-endian.
    pub fn point_bytes(&self) -> Vec<u8> {
        point_bytes(&self.points)
    }
}

impl Chart {
    /// Grows the charts outward by `passes` texels: each empty texel next
    /// to a lit one (of the eight about it) takes the first such
    /// neighbour's point.
    pub fn dilate(&mut self, passes: u32) {
        let (width, height) = (self.width as i64, self.height as i64);
        for _ in 0..passes {
            let before = self.texels.clone();
            for y in 0..height {
                for x in 0..width {
                    let index = (y * width + x) as usize;
                    if before[index] != EMPTY {
                        continue;
                    }
                    let neighbour = NEIGHBOURS.iter().find_map(|(dx, dy)| {
                        let (nx, ny) = (x + dx, y + dy);
                        if nx < 0 || ny < 0 || nx >= width || ny >= height {
                            return None;
                        }
                        Some(before[(ny * width + nx) as usize]).filter(|point| *point != EMPTY)
                    });
                    if let Some(point) = neighbour {
                        self.texels[index] = point;
                    }
                }
            }
        }
    }

    /// The page's texels each given its point's value from `values` (as
    /// many as the atlas's points), `empty` where none lights it.
    pub fn fill<T: Copy>(&self, values: &[T], empty: T) -> Vec<T> {
        self.texels
            .iter()
            .map(|point| values.get(*point as usize).copied().unwrap_or(empty))
            .collect()
    }
}

/// The eight texels about one: the four sharing an edge first.
const NEIGHBOURS: [(i64, i64); 8] = [
    (-1, 0),
    (1, 0),
    (0, -1),
    (0, 1),
    (-1, -1),
    (1, -1),
    (-1, 1),
    (1, 1),
];

/// A triangle's corner on the page.
#[derive(Clone, Copy)]
struct Corner {
    position: [f32; 3],
    normal: [f32; 3],
    texcoord: [f32; 2],
}

/// The side of a texel on the surface of `corners`' triangle, whose area
/// on the page is half `area` texels (0 if either area is nothing).
fn footprint(corners: &[Corner; 3], area: f32) -> f32 {
    let [a, b, c] = corners.map(|corner| corner.position);
    let ab: [f32; 3] = std::array::from_fn(|axis| b[axis] - a[axis]);
    let ac: [f32; 3] = std::array::from_fn(|axis| c[axis] - a[axis]);
    let cross = [
        ab[1] * ac[2] - ab[2] * ac[1],
        ab[2] * ac[0] - ab[0] * ac[2],
        ab[0] * ac[1] - ab[1] * ac[0],
    ];
    // (both areas doubled)
    let world = cross.iter().map(|v| v * v).sum::<f32>().sqrt();
    let side = (world / area.abs()).sqrt();
    if side.is_finite() {
        side.min(MAX_FOOTPRINT)
    } else {
        0.0
    }
}

/// Twice the signed area of the triangle `a`, `b`, `p`.
fn edge(a: [f32; 2], b: [f32; 2], p: [f32; 2]) -> f32 {
    (b[0] - a[0]) * (p[1] - a[1]) - (b[1] - a[1]) * (p[0] - a[0])
}

/// The baked size of the page `page` of `structure`: twice its stock
/// page's, at most [`MAX_PAGE_SIZE`]. None if it has no stock page.
pub fn page_size(level: &Level, structure: &StructureBsp, page: usize) -> Option<(u32, u32)> {
    let index = structure.lightmaps.get(page)?.bitmap_index?;
    let bitmap = level.bitmap(structure.lightmap_bitmap.as_ref()?)?;
    let image = bitmap.images.get(usize::from(index))?;
    let size = |stock: u16| (u32::from(stock) * PAGE_SCALE).clamp(1, MAX_PAGE_SIZE);
    (image.width > 0 && image.height > 0).then(|| (size(image.width), size(image.height)))
}

/// `points` as the GPU reads them (32 bytes each).
pub fn point_bytes(points: &[BakePoint]) -> Vec<u8> {
    let mut bytes = Vec::with_capacity(points.len() * 32);
    for point in points {
        for (three, fourth) in [(point.position, point.footprint), (point.normal, 0.0)] {
            for value in three.into_iter().chain([fourth]) {
                bytes.extend_from_slice(&value.to_le_bytes());
            }
        }
    }
    bytes
}

#[cfg(test)]
mod tests {
    use super::*;
    use opence_content::level::{EnvironmentVertex, Lightmap, LightmapVertex, Material};

    /// A BSP of one page: a right triangle over the page's lower left
    /// half, on the plane z = 0 from (0, 0) to (8, 8).
    fn triangle_bsp(texcoords: [[f32; 2]; 3]) -> StructureBsp {
        let mut structure = opence_content::devmap::plane(4.0, 1);
        let positions = [[0.0, 0.0, 0.0], [8.0, 0.0, 0.0], [0.0, 8.0, 0.0]];
        let vertex = |at: usize| EnvironmentVertex {
            position: positions[at],
            normal: [0.0, 0.0, 1.0],
            binormal: [0.0; 3],
            tangent: [0.0; 3],
            texcoord: [0.0; 2],
        };
        structure.surfaces = vec![[0, 1, 2]];
        structure.lightmaps = vec![Lightmap {
            bitmap_index: Some(0),
            materials: vec![Material {
                shader: None,
                first_surface: 0,
                surface_count: 1,
                vertices: (0..3).map(vertex).collect(),
                lightmap_vertices: texcoords
                    .iter()
                    .map(|texcoord| LightmapVertex {
                        incident_direction: [0.0, 0.0, 1.0],
                        texcoord: *texcoord,
                    })
                    .collect(),
            }],
        }];
        structure
    }

    #[test]
    fn a_triangle_covers_its_texels_with_its_surface() {
        let structure = triangle_bsp([[0.0, 0.0], [1.0, 0.0], [0.0, 1.0]]);
        let mut atlas = Atlas::default();
        atlas.lay_out(&structure, 0, 8, 8);
        let chart = &atlas.charts[0];
        assert_eq!((chart.page, chart.width, chart.height), (0, 8, 8));
        // the centres on or below the diagonal x + y <= 7: 36 of them
        assert_eq!(atlas.points.len(), 36);
        for y in 0..8u32 {
            for x in 0..8u32 {
                let texel = chart.texels[(y * 8 + x) as usize];
                // (the dilation reaches two texels on, diagonally too)
                assert_eq!(texel != EMPTY, x + y <= 11, "{x}, {y}");
                if x + y <= 7 {
                    let point = atlas.points[texel as usize];
                    // (the page's texel centre, on the 8-unit triangle)
                    assert!(
                        (point.position[0] - (x as f32 + 0.5)).abs() < 1e-4,
                        "{point:?}"
                    );
                    assert!(
                        (point.position[1] - (y as f32 + 0.5)).abs() < 1e-4,
                        "{point:?}"
                    );
                    assert_eq!(point.normal, [0.0, 0.0, 1.0]);
                    // (a texel is a unit square of the surface)
                    assert!((point.footprint - 1.0).abs() < 1e-4, "{point:?}");
                }
            }
        }
        let bytes = atlas.point_bytes();
        assert_eq!(bytes.len(), 36 * 32);
        // (the footprint in the position's fourth word)
        let mut word = [0; 4];
        word.copy_from_slice(&bytes[12..16]);
        let fourth = f32::from_le_bytes(word);
        assert!((fourth - 1.0).abs() < 1e-4);
        let lit: Vec<f32> = chart.fill(&[1.0; 36], 0.0);
        assert_eq!(lit.iter().filter(|value| **value == 1.0).count(), 64 - 6);
    }

    #[test]
    fn the_dilation_reaches_only_so_far() {
        // a small triangle in the corner of a large page
        let structure = triangle_bsp([[0.0, 0.0], [0.125, 0.0], [0.0, 0.125]]);
        let mut atlas = Atlas::default();
        atlas.lay_out(&structure, 0, 32, 32);
        let chart = &atlas.charts[0];
        let at = |x: usize, y: usize| chart.texels[y * 32 + x];
        assert_ne!(at(0, 0), EMPTY);
        // the triangle reaches x + y <= 3, and two texels beyond
        assert_ne!(at(5, 0), EMPTY);
        assert_eq!(at(6, 0), EMPTY);
        assert_eq!(at(20, 20), EMPTY);
        assert_eq!(
            chart.fill(
                &atlas.points,
                BakePoint {
                    position: [-1.0; 3],
                    normal: [0.0; 3],
                    footprint: 0.0,
                }
            )[20 * 32 + 20]
                .position,
            [-1.0; 3]
        );
    }

    #[test]
    fn a_triangle_too_small_for_a_centre_takes_the_texel_at_its_middle() {
        let tiny = 0.01;
        let structure = triangle_bsp([[0.5, 0.5], [0.5 + tiny, 0.5], [0.5, 0.5 + tiny]]);
        let mut atlas = Atlas::default();
        atlas.lay_out(&structure, 0, 8, 8);
        assert_eq!(atlas.points.len(), 1);
        // (texel centres at whole numbers: 0.5 * 8 - 0.5 = 3.5 rounds to 4)
        assert_eq!(atlas.charts[0].texels[4 * 8 + 4], 0);
    }

    #[test]
    fn broken_triangles_and_pages_are_left_out() {
        let mut structure = triangle_bsp([[0.0, 0.0], [f32::NAN, 0.0], [0.0, 1.0]]);
        let mut atlas = Atlas::default();
        atlas.lay_out(&structure, 0, 8, 8);
        assert!(atlas.points.is_empty());
        atlas.lay_out(&structure, 3, 8, 8);
        assert_eq!(atlas.charts.len(), 1);
        // a material without lightmap coordinates is not on the page
        structure.lightmaps[0].materials[0]
            .lightmap_vertices
            .clear();
        let mut atlas = Atlas::default();
        atlas.lay_out(&structure, 0, 8, 8);
        assert!(atlas.points.is_empty());
    }

    #[test]
    fn pages_follow_the_stock_pages_sizes() {
        let mut level = opence_content::devmap::level(None);
        // the devmap has no stock lightmaps
        assert_eq!(Atlas::of(&level, 0), Atlas::default());
        assert_eq!(Atlas::of(&level, 9), Atlas::default());
        let tag = opence_content::TagRef {
            group: opence_content::Group(*b"bitm"),
            path: "lightmaps".into(),
        };
        let mut bitmap = level.bitmaps[0].clone();
        bitmap.tag = tag.clone();
        bitmap.images.truncate(1);
        bitmap.images[0].width = 16;
        bitmap.images[0].height = 1500;
        level.bitmaps.push(bitmap);
        let structure = &mut level.structure_bsps[0];
        structure.lightmap_bitmap = Some(tag);
        structure.lightmaps[0].bitmap_index = Some(0);
        let structure = &level.structure_bsps[0];
        assert_eq!(page_size(&level, structure, 0), Some((32, MAX_PAGE_SIZE)));
        assert_eq!(page_size(&level, structure, 1), None);
        let atlas = Atlas::of(&level, 0);
        assert_eq!(atlas.charts.len(), 1);
    }
}
