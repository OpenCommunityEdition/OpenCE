//! What a level's surfaces are made of, found by a point on them: the
//! material type the game's collision reports with each test
//! (`collision.material_index`, through `get_material_type`), which Jolt's
//! mesh does not carry. The triangles are filed in a grid over x and y; a
//! point takes the material of the nearest triangle within
//! [`SURFACE_REACH`] of it.

use glam::Vec3;

use crate::LevelMesh;

/// How far from a surface a point may be and still be on it, in world
/// units (a contact's point, a ray's hit, a little off from float order).
pub const SURFACE_REACH: f32 = 0.05;
/// The least side of a grid cell, in world units.
const CELL_LEAST: f32 = 2.0;
/// The most cells along x or y: a larger level has larger cells.
const CELLS_MOST: usize = 512;

/// A triangle and what it is made of.
#[derive(Clone, Debug, PartialEq)]
struct Facet {
    corners: [Vec3; 3],
    material: i16,
}

/// A level's triangles by where they are, each with its material type.
#[derive(Clone, Debug, Default, PartialEq)]
pub struct SurfaceMaterials {
    facets: Vec<Facet>,
    origin: [f32; 2],
    cell: f32,
    columns: usize,
    rows: usize,
    /// Each cell's facets, row by row.
    cells: Vec<Vec<u32>>,
}

impl SurfaceMaterials {
    /// The materials of `mesh`'s triangles: each triangle's collision
    /// material, an index into `types` (the BSP's collision materials'
    /// material types). A triangle with none, or an index past `types`,
    /// is of none (-1).
    pub fn new(mesh: &LevelMesh, types: &[i16]) -> SurfaceMaterials {
        let mut facets = Vec::with_capacity(mesh.triangles.len());
        for (index, triangle) in mesh.triangles.iter().enumerate() {
            let Some(corners) = triangle
                .iter()
                .map(|&vertex| mesh.vertices.get(vertex as usize).copied())
                .collect::<Option<Vec<_>>>()
            else {
                continue;
            };
            let material = mesh
                .materials
                .get(index)
                .copied()
                .flatten()
                .and_then(|material| types.get(usize::from(material)).copied())
                .unwrap_or(-1);
            facets.push(Facet {
                corners: [0, 1, 2].map(|corner| Vec3::from_array(corners[corner])),
                material,
            });
        }
        let Some(first) = facets.first().map(|facet| facet.corners[0]) else {
            return SurfaceMaterials::default();
        };
        let (mut least, mut greatest) = (first, first);
        for corner in facets.iter().flat_map(|facet| facet.corners) {
            least = least.min(corner);
            greatest = greatest.max(corner);
        }
        least -= Vec3::splat(SURFACE_REACH);
        greatest += Vec3::splat(SURFACE_REACH);
        let extent = (greatest - least).max_element();
        let cell = (extent / CELLS_MOST as f32).max(CELL_LEAST);
        let columns = cells_along(greatest.x - least.x, cell);
        let rows = cells_along(greatest.y - least.y, cell);
        let mut surfaces = SurfaceMaterials {
            facets,
            origin: [least.x, least.y],
            cell,
            columns,
            rows,
            cells: vec![Vec::new(); columns * rows],
        };
        for (index, facet) in surfaces.facets.iter().enumerate() {
            let low = facet.corners[0].min(facet.corners[1]).min(facet.corners[2])
                - Vec3::splat(SURFACE_REACH);
            let high = facet.corners[0].max(facet.corners[1]).max(facet.corners[2])
                + Vec3::splat(SURFACE_REACH);
            let (column0, row0) = surfaces.cell_of(low);
            let (column1, row1) = surfaces.cell_of(high);
            for row in row0..=row1 {
                for column in column0..=column1 {
                    surfaces.cells[row * columns + column].push(index as u32);
                }
            }
        }
        surfaces
    }

    /// The material type of the surface at `point` (the nearest within
    /// [`SURFACE_REACH`]; of two as near, the first), or -1 for none.
    pub fn at(&self, point: Vec3) -> i16 {
        if self.cells.is_empty() || !point.is_finite() {
            return -1;
        }
        let (column, row) = self.cell_of(point);
        let mut nearest = (SURFACE_REACH * SURFACE_REACH, -1);
        for &index in &self.cells[row * self.columns + column] {
            let facet = &self.facets[index as usize];
            let distance = closest_point(point, facet.corners).distance_squared(point);
            if distance <= nearest.0 && (distance < nearest.0 || nearest.1 == -1) {
                nearest = (distance, facet.material);
            }
        }
        nearest.1
    }

    /// The cell `point` is in, held to the grid.
    fn cell_of(&self, point: Vec3) -> (usize, usize) {
        let along = |value: f32, origin: f32, count: usize| {
            let cell = libm::floorf((value - origin) / self.cell);
            if cell.is_nan() || cell < 0.0 {
                0
            } else {
                (cell as usize).min(count - 1)
            }
        };
        (
            along(point.x, self.origin[0], self.columns),
            along(point.y, self.origin[1], self.rows),
        )
    }
}

/// How many cells of side `cell` cover `extent`: at least one, at most
/// [`CELLS_MOST`].
fn cells_along(extent: f32, cell: f32) -> usize {
    let cells = libm::ceilf(extent / cell);
    if cells.is_nan() || cells < 1.0 {
        1
    } else {
        (cells as usize).clamp(1, CELLS_MOST)
    }
}

/// The point of triangle `corners` nearest `point` (Ericson's, by the
/// region of the triangle's plane `point` falls in).
fn closest_point(point: Vec3, [a, b, c]: [Vec3; 3]) -> Vec3 {
    let ab = b - a;
    let ac = c - a;
    let ap = point - a;
    let d1 = ab.dot(ap);
    let d2 = ac.dot(ap);
    if d1 <= 0.0 && d2 <= 0.0 {
        return a;
    }
    let bp = point - b;
    let d3 = ab.dot(bp);
    let d4 = ac.dot(bp);
    if d3 >= 0.0 && d4 <= d3 {
        return b;
    }
    let vc = d1 * d4 - d3 * d2;
    if vc <= 0.0 && d1 >= 0.0 && d3 <= 0.0 {
        return a + ab * (d1 / (d1 - d3));
    }
    let cp = point - c;
    let d5 = ab.dot(cp);
    let d6 = ac.dot(cp);
    if d6 >= 0.0 && d5 <= d6 {
        return c;
    }
    let vb = d5 * d2 - d1 * d6;
    if vb <= 0.0 && d2 >= 0.0 && d6 <= 0.0 {
        return a + ac * (d2 / (d2 - d6));
    }
    let va = d3 * d6 - d5 * d4;
    if va <= 0.0 && (d4 - d3) >= 0.0 && (d5 - d6) >= 0.0 {
        return b + (c - b) * ((d4 - d3) / ((d4 - d3) + (d5 - d6)));
    }
    let denominator = va + vb + vc;
    if denominator == 0.0 {
        // (a triangle with no area: its first corner)
        return a;
    }
    a + ab * (vb / denominator) + ac * (vc / denominator)
}

#[cfg(test)]
mod tests {
    use super::*;

    /// Two squares of floor side by side, of materials 0 and 1, and a
    /// wall of material 2 standing at x = 2.
    fn mesh() -> LevelMesh {
        LevelMesh {
            vertices: vec![
                [0.0, 0.0, 0.0],
                [1.0, 0.0, 0.0],
                [1.0, 1.0, 0.0],
                [0.0, 1.0, 0.0],
                [2.0, 0.0, 0.0],
                [2.0, 1.0, 0.0],
                [2.0, 0.0, 1.0],
                [2.0, 1.0, 1.0],
            ],
            triangles: vec![
                [0, 1, 2],
                [0, 2, 3],
                [1, 4, 5],
                [1, 5, 2],
                [4, 5, 7],
                [4, 7, 6],
            ],
            materials: vec![Some(0), Some(0), Some(1), Some(1), Some(2), None],
        }
    }

    #[test]
    fn a_point_takes_the_material_of_the_surface_it_is_on() {
        let surfaces = SurfaceMaterials::new(&mesh(), &[7, 9, 3]);
        assert_eq!(surfaces.at(Vec3::new(0.5, 0.5, 0.0)), 7);
        assert_eq!(surfaces.at(Vec3::new(1.5, 0.5, 0.01)), 9);
        assert_eq!(surfaces.at(Vec3::new(1.99, 0.8, 0.5)), 3);
        // (the wall's other triangle has none)
        assert_eq!(surfaces.at(Vec3::new(2.0, 0.2, 0.9)), -1);
        // (off every surface)
        assert_eq!(surfaces.at(Vec3::new(0.5, 0.5, 0.5)), -1);
        assert_eq!(surfaces.at(Vec3::new(-5.0, 0.5, 0.0)), -1);
        assert_eq!(surfaces.at(Vec3::NAN), -1);
    }

    #[test]
    fn a_material_past_the_types_is_none() {
        let surfaces = SurfaceMaterials::new(&mesh(), &[7]);
        assert_eq!(surfaces.at(Vec3::new(1.5, 0.5, 0.0)), -1);
        assert_eq!(
            SurfaceMaterials::new(&LevelMesh::default(), &[]).at(Vec3::ZERO),
            -1
        );
    }

    #[test]
    fn of_two_surfaces_the_nearer_is_taken() {
        // (where floor and wall meet, a point just above the floor)
        let surfaces = SurfaceMaterials::new(&mesh(), &[7, 9, 3]);
        assert_eq!(surfaces.at(Vec3::new(1.98, 0.5, 0.005)), 9);
        assert_eq!(surfaces.at(Vec3::new(1.995, 0.5, 0.03)), 3);
    }

    #[test]
    fn the_closest_point_is_found_in_every_region() {
        let triangle = [Vec3::ZERO, Vec3::X, Vec3::Y];
        let cases = [
            (Vec3::new(-1.0, -1.0, 0.0), Vec3::ZERO),
            (Vec3::new(2.0, -0.5, 0.0), Vec3::X),
            (Vec3::new(-0.5, 2.0, 0.0), Vec3::Y),
            (Vec3::new(0.5, -1.0, 0.0), Vec3::new(0.5, 0.0, 0.0)),
            (Vec3::new(-1.0, 0.5, 0.0), Vec3::new(0.0, 0.5, 0.0)),
            (Vec3::new(1.0, 1.0, 0.0), Vec3::new(0.5, 0.5, 0.0)),
            (Vec3::new(0.25, 0.25, 3.0), Vec3::new(0.25, 0.25, 0.0)),
        ];
        for (point, nearest) in cases {
            assert!(
                closest_point(point, triangle).distance(nearest) < 1e-6,
                "{point}"
            );
        }
        assert_eq!(closest_point(Vec3::ONE, [Vec3::X; 3]), Vec3::X);
    }

    #[test]
    fn a_large_level_keeps_its_grid_small() {
        let mesh = LevelMesh {
            vertices: vec![[0.0, 0.0, 0.0], [5000.0, 0.0, 0.0], [0.0, 5000.0, 0.0]],
            triangles: vec![[0, 1, 2]],
            materials: vec![Some(0)],
        };
        let surfaces = SurfaceMaterials::new(&mesh, &[4]);
        assert!(surfaces.columns <= CELLS_MOST && surfaces.rows <= CELLS_MOST);
        assert_eq!(surfaces.at(Vec3::new(100.0, 100.0, 0.0)), 4);
        assert_eq!(surfaces.at(Vec3::new(4000.0, 4000.0, 0.0)), -1);
    }
}
