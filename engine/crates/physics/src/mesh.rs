//! A level's collision mesh: a collision BSP's surfaces cut into
//! triangles, as port/linux/game/physics_jolt.c cuts them for the host.
//! Each surface is a convex ring of edges; its fan of triangles is wound to
//! face as its plane does, and a two-sided surface has both windings.

use opence_content::CollisionBsp;

/// A surface's flag: it is solid from both sides.
const SURFACE_TWO_SIDED: u8 = 1 << 0;
/// The most edges a surface's ring may have: a longer ring is taken as
/// broken (an edge that leads back to a surface's other edges never
/// closes).
const RING_MAX: usize = 64;

/// Triangles in world units, three vertex indices each.
#[derive(Clone, Debug, Default, PartialEq)]
pub struct LevelMesh {
    /// The vertices.
    pub vertices: Vec<[f32; 3]>,
    /// The triangles, wound counter-clockwise seen from their front.
    pub triangles: Vec<[u32; 3]>,
    /// Each triangle's surface's collision material (an index into its
    /// structure BSP's collision materials), if it has one.
    pub materials: Vec<Option<u16>>,
}

impl LevelMesh {
    /// A collision BSP's surfaces as triangles. A surface whose ring is
    /// broken or names what the BSP does not have is left out.
    pub fn from_collision(bsp: &CollisionBsp) -> LevelMesh {
        let mut mesh = LevelMesh::default();
        mesh.append(bsp);
        mesh
    }

    /// Adds a collision BSP's surfaces (another structure BSP's, or an
    /// object's) to the mesh.
    pub fn append(&mut self, bsp: &CollisionBsp) {
        let Ok(base) = u32::try_from(self.vertices.len()) else {
            return;
        };
        self.vertices
            .extend(bsp.vertices.iter().map(|vertex| vertex.point));
        let mut ring = Vec::with_capacity(RING_MAX);
        for (index, surface) in bsp.surfaces.iter().enumerate() {
            if !surface_ring(bsp, index, &mut ring) || ring.len() < 3 {
                continue;
            }
            let plane_index = (surface.plane & i32::MAX) as usize;
            let Some(plane) = bsp.planes.get(plane_index) else {
                continue;
            };
            // the ring's normal (Newell's), against its plane's
            let mut normal = [0.0f32; 3];
            for (point, &a) in ring.iter().enumerate() {
                let a = bsp.vertices[a as usize].point;
                let b = bsp.vertices[ring[(point + 1) % ring.len()] as usize].point;
                normal[0] += (a[1] - b[1]) * (a[2] + b[2]);
                normal[1] += (a[2] - b[2]) * (a[0] + b[0]);
                normal[2] += (a[0] - b[0]) * (a[1] + b[1]);
            }
            let facing = normal[0] * plane[0] + normal[1] * plane[1] + normal[2] * plane[2];
            let flipped = (facing < 0.0) != (surface.plane < 0);
            let sides = if surface.flags & SURFACE_TWO_SIDED != 0 {
                2
            } else {
                1
            };
            for side in 0..sides {
                let reverse = flipped != (side == 1);
                for point in 1..ring.len() - 1 {
                    let (b, c) = if reverse {
                        (point + 1, point)
                    } else {
                        (point, point + 1)
                    };
                    self.triangles
                        .push([base + ring[0], base + ring[b], base + ring[c]]);
                    self.materials.push(surface.material);
                }
            }
        }
    }

    /// Whether it has no triangle.
    pub fn is_empty(&self) -> bool {
        self.triangles.is_empty()
    }
}

/// The vertex indices of a surface's ring, in its order, into `ring`;
/// false if the ring is broken or names a vertex the BSP does not have.
fn surface_ring(bsp: &CollisionBsp, surface: usize, ring: &mut Vec<u32>) -> bool {
    ring.clear();
    let Some(first) = bsp.surfaces.get(surface).map(|surface| surface.first_edge) else {
        return false;
    };
    let surface = surface as u32;
    let mut edge_index = first;
    loop {
        let Some(edge) = bsp.edges.get(edge_index as usize) else {
            return false;
        };
        if ring.len() >= RING_MAX {
            return false;
        }
        let reverse = usize::from(edge.surfaces[1] == Some(surface));
        let vertex = edge.vertices[reverse];
        if vertex as usize >= bsp.vertices.len() {
            return false;
        }
        ring.push(vertex);
        edge_index = edge.edges[reverse];
        if edge_index == first {
            return true;
        }
    }
}
