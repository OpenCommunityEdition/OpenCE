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
                points.push(BakePoint { position, normal });
            }
        }
        points
    }
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
