//! The light in the level's air: irradiance probes and reflection probes.
//!
//! The irradiance probes are a grid over the BSP's bounds. Each holds the
//! light arriving from each of the six axes (an ambient cube), which lights
//! what moves through the level and the surfaces of a BSP shipped without
//! lightmaps. The reflection probes are small cube maps of what is seen
//! from points spread over the level's geometry, which reflections fall
//! back to.
//!
//! The original chose a level's light by its clusters, which this engine
//! does not import; the reflection probes are placed by a coarse grid of
//! the geometry instead.

use std::collections::BTreeSet;

use crate::chart::BakePoint;
use crate::scene::Scene;

/// The most irradiance probes in a grid.
pub const MAX_PROBES: u32 = 65_536;
/// The least distance between probes (world units: about three metres).
pub const MIN_SPACING: f32 = 1.0;
/// The six axes each probe is lit from, in order: +x, -x, +y, -y, +z, -z.
pub const AXES: [[f32; 3]; 6] = [
    [1.0, 0.0, 0.0],
    [-1.0, 0.0, 0.0],
    [0.0, 1.0, 0.0],
    [0.0, -1.0, 0.0],
    [0.0, 0.0, 1.0],
    [0.0, 0.0, -1.0],
];
/// The most reflection probes.
pub const MAX_REFLECTIONS: usize = 256;
/// A reflection probe's faces' size, each way.
pub const REFLECTION_SIZE: u32 = 16;
/// How many cells, along the bounds' longest side, reflection probes are
/// placed in at first.
const REFLECTION_CELLS: f32 = 16.0;

/// The irradiance probes' grid.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct ProbeGrid {
    /// The first probe's position (the bounds' least corner).
    pub origin: [f32; 3],
    /// The distance between neighbouring probes.
    pub spacing: f32,
    /// How many probes each way.
    pub counts: [u32; 3],
}

impl ProbeGrid {
    /// A grid covering `bounds` (least, then greatest corner), of at most
    /// [`MAX_PROBES`] probes no closer than [`MIN_SPACING`].
    pub fn covering(bounds: [[f32; 3]; 2]) -> Self {
        let [low, high] = bounds;
        let extent: [f32; 3] = std::array::from_fn(|axis| {
            let side = high[axis] - low[axis];
            if side.is_finite() {
                side.max(0.0)
            } else {
                0.0
            }
        });
        let origin = low.map(|value| if value.is_finite() { value } else { 0.0 });
        let volume: f32 = extent.iter().map(|side| side.max(MIN_SPACING)).product();
        let mut spacing = libm::cbrtf(volume / MAX_PROBES as f32).max(MIN_SPACING);
        loop {
            let counts = extent.map(|side| (libm::ceilf(side / spacing) as u32).saturating_add(1));
            let total = counts
                .iter()
                .map(|count| u64::from(*count))
                .product::<u64>();
            if total <= u64::from(MAX_PROBES) {
                return Self {
                    origin,
                    spacing,
                    counts,
                };
            }
            spacing *= 1.25;
        }
    }

    /// How many probes.
    pub fn len(&self) -> usize {
        self.counts.iter().map(|count| *count as usize).product()
    }

    /// Whether there are none.
    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }

    /// The probe `index`'s position (x fastest, then y, then z).
    pub fn position(&self, index: usize) -> [f32; 3] {
        let [nx, ny, _] = self.counts.map(|count| count.max(1) as usize);
        let at = [index % nx, (index / nx) % ny, index / (nx * ny)];
        std::array::from_fn(|axis| self.origin[axis] + at[axis] as f32 * self.spacing)
    }

    /// The points to bake: each probe's six axes in [`AXES`]'s order.
    pub fn points(&self) -> Vec<BakePoint> {
        let mut points = Vec::with_capacity(self.len() * AXES.len());
        for index in 0..self.len() {
            let position = self.position(index);
            for normal in AXES {
                points.push(BakePoint {
                    position,
                    normal,
                    footprint: 0.0,
                });
            }
        }
        points
    }
}

/// The light at `point` on a surface facing `normal` (unit) from the
/// probes of `grid` (`probes` as [`crate::output::BakedLighting::probes`]
/// holds them): red, green and blue, and the sun's visibility. Each axis's
/// light is interpolated between the eight probes around the point
/// (clamped to the grid, as the renderer's 3D textures sample them) and
/// the six are weighed by how much the normal faces each (an ambient
/// cube). Dark when the grid or the probes are missing.
pub fn light_at(grid: &ProbeGrid, probes: &[u16], point: [f32; 3], normal: [f32; 3]) -> [f32; 4] {
    let count = grid.len();
    if count == 0 || probes.len() != count * AXES.len() * 4 || grid.spacing <= 0.0 {
        return [0.0; 4];
    }
    let length = (normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]).sqrt();
    let n = if length > 0.0 {
        normal.map(|value| value / length)
    } else {
        [0.0, 0.0, 1.0]
    };
    // (each axis's lower probe, and how far the point is towards the next)
    let cell: [(usize, usize, f32); 3] = std::array::from_fn(|axis| {
        let last = grid.counts[axis].max(1) - 1;
        let at = ((point[axis] - grid.origin[axis]) / grid.spacing).clamp(0.0, last as f32);
        let low = (at.floor() as u32).min(last);
        (low as usize, (low + 1).min(last) as usize, at - low as f32)
    });
    let [nx, ny, _] = grid.counts.map(|count| count.max(1) as usize);
    let mut light = [0.0f32; 4];
    for (axis, weight) in n.iter().map(|value| value * value).enumerate() {
        let side = axis * 2 + usize::from(n[axis] < 0.0);
        for corner in 0..8 {
            let pick = |along: usize| {
                let (low, high, t) = cell[along];
                if corner >> along & 1 == 1 {
                    (high, t)
                } else {
                    (low, 1.0 - t)
                }
            };
            let ((x, wx), (y, wy), (z, wz)) = (pick(0), pick(1), pick(2));
            let share = weight * wx * wy * wz;
            if share == 0.0 {
                continue;
            }
            let at = (side * count + x + nx * (y + ny * z)) * 4;
            for (channel, total) in light.iter_mut().enumerate() {
                *total += share * crate::output::from_half(probes[at + channel]);
            }
        }
    }
    light
}

/// The reflection probe of `reflections` nearest `point`, if there are
/// any.
pub fn nearest_reflection(reflections: &[[f32; 3]], point: [f32; 3]) -> Option<usize> {
    let distance = |probe: &[f32; 3]| -> f32 {
        (0..3)
            .map(|axis| (probe[axis] - point[axis]) * (probe[axis] - point[axis]))
            .sum()
    };
    reflections
        .iter()
        .enumerate()
        .min_by(|(_, a), (_, b)| distance(a).total_cmp(&distance(b)))
        .map(|(index, _)| index)
}

/// Where the reflection probes go: the middles of the cells of a coarse
/// grid over `scene` that hold a triangle's middle, at most
/// [`MAX_REFLECTIONS`] (the cells grow until they fit).
pub fn reflection_probes(scene: &Scene) -> Vec<[f32; 3]> {
    if scene.triangles.is_empty() {
        return Vec::new();
    }
    let [low, high] = scene.bounds;
    let longest = (0..3)
        .map(|axis| high[axis] - low[axis])
        .fold(0.0f32, f32::max);
    let mut cell = (longest / REFLECTION_CELLS).max(MIN_SPACING);
    loop {
        let mut cells = BTreeSet::new();
        for triangle in &scene.triangles {
            let middle: [f32; 3] = std::array::from_fn(|axis| {
                triangle
                    .positions
                    .iter()
                    .map(|corner| corner[axis])
                    .sum::<f32>()
                    / 3.0
            });
            let at: [i64; 3] =
                std::array::from_fn(|axis| libm::floorf((middle[axis] - low[axis]) / cell) as i64);
            cells.insert(at);
        }
        if cells.len() <= MAX_REFLECTIONS {
            return cells
                .into_iter()
                .map(|at| {
                    std::array::from_fn(|axis| {
                        (low[axis] + (at[axis] as f32 + 0.5) * cell).clamp(low[axis], high[axis])
                    })
                })
                .collect();
        }
        cell *= 1.5;
    }
}

/// The way the texel `x`, `y` of cube face `face` (+x, -x, +y, -y, +z, -z,
/// as Metal orders them) looks, for faces `size` texels each way (unit).
pub fn cube_direction(face: u32, x: u32, y: u32, size: u32) -> [f32; 3] {
    let size = size.max(1) as f32;
    let s = 2.0 * (x as f32 + 0.5) / size - 1.0;
    let t = 2.0 * (y as f32 + 0.5) / size - 1.0;
    let direction = match face {
        0 => [1.0, -t, -s],
        1 => [-1.0, -t, s],
        2 => [s, 1.0, t],
        3 => [s, -1.0, -t],
        4 => [s, -t, 1.0],
        _ => [-s, -t, -1.0],
    };
    let length = direction.iter().map(|v| v * v).sum::<f32>().sqrt();
    direction.map(|v| v / length)
}

/// The points to bake the reflection probes at `probes` by: each probe's
/// six faces' texels, row by row, as a position and the way it looks.
pub fn reflection_points(probes: &[[f32; 3]]) -> Vec<BakePoint> {
    let size = REFLECTION_SIZE;
    let mut points = Vec::with_capacity(probes.len() * 6 * (size * size) as usize);
    for position in probes {
        for face in 0..6 {
            for y in 0..size {
                for x in 0..size {
                    points.push(BakePoint {
                        position: *position,
                        normal: cube_direction(face, x, y, size),
                        footprint: 0.0,
                    });
                }
            }
        }
    }
    points
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::layout;

    #[test]
    fn the_light_between_probes_is_interpolated_and_weighed_by_the_normal() {
        use crate::output::half;
        let grid = ProbeGrid {
            origin: [0.0; 3],
            spacing: 2.0,
            counts: [2, 1, 1],
        };
        // (+x lit 1 at the first probe and 3 at the second; -z lit 2, its
        // sun seen half; the other axes dark)
        let mut probes = vec![0u16; grid.len() * 6 * 4];
        let mut set = |axis: usize, index: usize, light: [f32; 4]| {
            for (channel, value) in light.into_iter().enumerate() {
                probes[(axis * grid.len() + index) * 4 + channel] = half(value);
            }
        };
        set(0, 0, [1.0, 1.0, 1.0, 1.0]);
        set(0, 1, [3.0, 3.0, 3.0, 1.0]);
        set(5, 0, [2.0, 0.0, 0.0, 0.5]);
        set(5, 1, [2.0, 0.0, 0.0, 0.5]);
        let at = |point, normal| light_at(&grid, &probes, point, normal);
        assert_eq!(at([0.0; 3], [1.0, 0.0, 0.0]), [1.0, 1.0, 1.0, 1.0]);
        assert_eq!(at([1.0, 0.0, 0.0], [2.0, 0.0, 0.0]), [2.0, 2.0, 2.0, 1.0]);
        // (outside the grid: its nearest probes)
        assert_eq!(at([9.0, -4.0, 7.0], [1.0, 0.0, 0.0]), [3.0, 3.0, 3.0, 1.0]);
        assert_eq!(at([-9.0, 0.0, 0.0], [0.0, 0.0, -1.0]), [2.0, 0.0, 0.0, 0.5]);
        // (facing +x and -z alike: half of each)
        let s = core::f32::consts::FRAC_1_SQRT_2;
        let light = at([0.0; 3], [s, 0.0, -s]);
        for (got, want) in light.iter().zip([1.5, 0.5, 0.5, 0.75]) {
            assert!((got - want).abs() < 1e-3, "{light:?}");
        }
        // (facing a dark axis, or no probes)
        assert_eq!(at([0.0; 3], [0.0, 1.0, 0.0]), [0.0; 4]);
        assert_eq!(light_at(&grid, &[], [0.0; 3], [1.0, 0.0, 0.0]), [0.0; 4]);
        // (no normal: facing up)
        assert_eq!(at([0.0; 3], [0.0; 3]), [0.0; 4]);
    }

    #[test]
    fn the_nearest_reflection_probe_is_found() {
        let probes = [[0.0, 0.0, 0.0], [10.0, 0.0, 0.0], [0.0, 10.0, 5.0]];
        assert_eq!(nearest_reflection(&probes, [1.0, 1.0, 1.0]), Some(0));
        assert_eq!(nearest_reflection(&probes, [7.0, -3.0, 0.0]), Some(1));
        assert_eq!(nearest_reflection(&probes, [0.0, 50.0, 0.0]), Some(2));
        assert_eq!(nearest_reflection(&[], [0.0; 3]), None);
    }

    #[test]
    fn the_grid_covers_the_bounds() {
        for bounds in [
            [[-1.0, -2.0, -3.0], [1.0, 2.0, 3.0]],
            [[0.0; 3], [0.0; 3]],
            [[-500.0, -300.0, -20.0], [400.0, 350.0, 90.0]],
            [[0.0, 0.0, 0.0], [5000.0, 1.0, 1.0]],
        ] {
            let grid = ProbeGrid::covering(bounds);
            assert!(grid.len() as u32 <= MAX_PROBES, "{grid:?}");
            assert!(!grid.is_empty());
            assert!(grid.spacing >= MIN_SPACING);
            assert_eq!(grid.position(0), bounds[0]);
            let last = grid.position(grid.len() - 1);
            for axis in 0..3 {
                assert!(last[axis] >= bounds[1][axis] - 1e-3, "{grid:?}");
                assert!(last[axis] < bounds[1][axis] + grid.spacing + 1e-3);
            }
            assert_eq!(grid.points().len(), grid.len() * 6);
        }
        let small = ProbeGrid::covering([[-1.0, -2.0, -3.0], [1.0, 2.0, 3.0]]);
        assert_eq!((small.spacing, small.counts), (1.0, [3, 5, 7]));
        // (x fastest)
        assert_eq!(small.position(1), [0.0, -2.0, -3.0]);
        assert_eq!(small.position(3), [-1.0, -1.0, -3.0]);
        assert_eq!(small.points()[1].normal, AXES[1]);
        let broken = ProbeGrid::covering([[f32::NAN; 3], [f32::INFINITY; 3]]);
        assert_eq!(broken.counts, [1, 1, 1]);
    }

    #[test]
    fn reflection_probes_are_where_the_geometry_is() {
        let (furnace, _) = layout::layout("furnace").unwrap_or_else(|| panic!("no furnace"));
        let probes = reflection_probes(&furnace);
        assert!(!probes.is_empty());
        assert!(probes.len() <= MAX_REFLECTIONS);
        for probe in &probes {
            assert!(
                probe.iter().all(|value| (-1.0..=1.0).contains(value)),
                "{probe:?}"
            );
        }
        let (open, _) = layout::layout("open").unwrap_or_else(|| panic!("no open"));
        assert!(reflection_probes(&open).is_empty());
        // a wide floor of many triangles fits in the most probes, and none
        // is far from the floor
        let mut floor = Vec::new();
        for x in 0..64 {
            for y in 0..64 {
                floor.extend(layout::quad(
                    [x as f32 * 10.0, y as f32 * 10.0, 0.0],
                    [10.0, 0.0, 0.0],
                    [0.0, 10.0, 0.0],
                ));
            }
        }
        let floor = layout::scene(floor, vec![layout::material(0.5, 0.0, 1.0)], 1.0);
        let probes = reflection_probes(&floor);
        assert!(
            probes.len() <= MAX_REFLECTIONS && probes.len() > 64,
            "{}",
            probes.len()
        );
        assert!(probes.iter().all(|probe| probe[2] == 0.0));
        assert_eq!(
            reflection_points(&probes).len(),
            probes.len() * 6 * (REFLECTION_SIZE * REFLECTION_SIZE) as usize
        );
    }

    #[test]
    fn cube_faces_look_along_their_axes() {
        for (face, axis) in AXES.iter().enumerate() {
            let middle = cube_direction(face as u32, 7, 7, 16);
            let along = middle.iter().zip(axis).map(|(a, b)| a * b).sum::<f32>();
            assert!(along > 0.99, "{face}: {middle:?}");
            let corner = cube_direction(face as u32, 0, 0, 16);
            let length = corner.iter().map(|v| v * v).sum::<f32>();
            assert!((length - 1.0).abs() < 1e-5);
        }
        // the +z face's first texel is up and to the left (-x, +y)
        let corner = cube_direction(4, 0, 0, 16);
        assert!(corner[0] < 0.0 && corner[1] > 0.0, "{corner:?}");
    }
}
