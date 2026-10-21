//! A level: its scenario, its structure BSPs and what they are drawn with.

use crate::{Bitmap, Shader, TagRef};

/// Everything a level's structure is drawn and collided with.
#[derive(Clone, Debug, PartialEq)]
pub struct Level {
    /// The scenario.
    pub scenario: Scenario,
    /// Its structure BSPs, in the scenario's order.
    pub structure_bsps: Vec<StructureBsp>,
    /// Every shader the BSPs use, and the shaders those layer on, by path.
    pub shaders: Vec<Shader>,
    /// Every bitmap the BSPs and shaders use, by path.
    pub bitmaps: Vec<Bitmap>,
    /// Bitmaps whose definitions are in Custom Edition's `bitmaps.map`,
    /// not in the map: read with the shared resource maps.
    pub resource_bitmaps: Vec<TagRef>,
}

impl Level {
    /// A shader, by its tag.
    pub fn shader(&self, tag: &TagRef) -> Option<&Shader> {
        self.shaders.iter().find(|shader| shader.tag == *tag)
    }

    /// A bitmap, by its tag.
    pub fn bitmap(&self, tag: &TagRef) -> Option<&Bitmap> {
        self.bitmaps.iter().find(|bitmap| bitmap.tag == *tag)
    }
}

/// A scenario, as far as its structure goes.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Scenario {
    /// Its path.
    pub path: String,
    /// Its structure BSPs.
    pub structure_bsps: Vec<TagRef>,
}

/// A structure BSP: the level's fixed geometry, its lightmaps and its
/// collision.
#[derive(Clone, Debug, PartialEq)]
pub struct StructureBsp {
    /// Its path.
    pub path: String,
    /// The bitmap its lightmaps' pages are in.
    pub lightmap_bitmap: Option<TagRef>,
    /// Its bounds: x, y and z, each least and greatest.
    pub world_bounds: [[f32; 2]; 3],
    /// Its collision.
    pub collision: CollisionBsp,
    /// What each collision surface is made of.
    pub collision_materials: Vec<CollisionMaterial>,
    /// Triangles, as indices into their material's vertices.
    pub surfaces: Vec<[u16; 3]>,
    /// Its geometry, grouped by lightmap page.
    pub lightmaps: Vec<Lightmap>,
}

/// What a collision surface is made of.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct CollisionMaterial {
    /// The shader drawn there.
    pub shader: Option<TagRef>,
    /// The physics material type (what a footstep or a bullet does).
    pub material_type: i16,
}

/// The geometry that shares one lightmap page.
#[derive(Clone, Debug, PartialEq)]
pub struct Lightmap {
    /// The page's index in the BSP's lightmap bitmap, if it has one.
    pub bitmap_index: Option<u16>,
    /// Its materials.
    pub materials: Vec<Material>,
}

/// The triangles of a lightmap page that share a shader.
#[derive(Clone, Debug, PartialEq)]
pub struct Material {
    /// The shader.
    pub shader: Option<TagRef>,
    /// Its first triangle, in the BSP's surfaces.
    pub first_surface: u32,
    /// How many triangles, from the first.
    pub surface_count: u32,
    /// Its vertices, which its triangles index.
    pub vertices: Vec<EnvironmentVertex>,
    /// Each vertex's lightmap coordinates and light direction, if it is
    /// lightmapped (as many as the vertices).
    pub lightmap_vertices: Vec<LightmapVertex>,
}

/// A vertex of the structure.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct EnvironmentVertex {
    /// Where it is.
    pub position: [f32; 3],
    /// Its normal.
    pub normal: [f32; 3],
    /// Its binormal.
    pub binormal: [f32; 3],
    /// Its tangent.
    pub tangent: [f32; 3],
    /// Its texture coordinates.
    pub texcoord: [f32; 2],
}

/// A vertex's light: where on the lightmap page it is, and the direction
/// most of its light comes from.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct LightmapVertex {
    /// The incident light's direction.
    pub incident_direction: [f32; 3],
    /// Where on the page it is.
    pub texcoord: [f32; 2],
}

/// A collision BSP, in the game's encoding, its indices checked.
///
/// A 3D node's child is a node if it is not negative; otherwise its low
/// 31 bits are a leaf, all ones for none. A 2D node's child, and a 2D
/// reference's node, are a node if not negative, otherwise a surface in
/// the low 31 bits. A plane designator's high bit flips the plane.
#[derive(Clone, Debug, Default, PartialEq)]
pub struct CollisionBsp {
    /// The 3D nodes.
    pub bsp3d_nodes: Vec<Bsp3dNode>,
    /// The planes: normal, then distance.
    pub planes: Vec<[f32; 4]>,
    /// The leaves.
    pub leaves: Vec<CollisionLeaf>,
    /// The 2D references.
    pub bsp2d_references: Vec<Bsp2dReference>,
    /// The 2D nodes.
    pub bsp2d_nodes: Vec<Bsp2dNode>,
    /// The surfaces.
    pub surfaces: Vec<CollisionSurface>,
    /// The edges.
    pub edges: Vec<CollisionEdge>,
    /// The vertices.
    pub vertices: Vec<CollisionVertex>,
}

/// A 3D node of a collision BSP.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Bsp3dNode {
    /// Its plane designator.
    pub plane: i32,
    /// Behind and in front of the plane.
    pub children: [i32; 2],
}

/// A leaf of a collision BSP.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct CollisionLeaf {
    /// Its flags.
    pub flags: u16,
    /// How many 2D references it has.
    pub bsp2d_reference_count: u16,
    /// Its first 2D reference.
    pub first_bsp2d_reference: u32,
}

/// A 2D reference of a collision BSP: a plane and the 2D tree in it.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Bsp2dReference {
    /// Its plane designator.
    pub plane: i32,
    /// Its tree's root.
    pub node: i32,
}

/// A 2D node of a collision BSP.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Bsp2dNode {
    /// Its line: normal, then distance.
    pub plane: [f32; 3],
    /// Behind and in front of the line.
    pub children: [i32; 2],
}

/// A surface of a collision BSP.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct CollisionSurface {
    /// Its plane designator.
    pub plane: i32,
    /// One of its edges.
    pub first_edge: u32,
    /// Its flags (two-sided, invisible, climbable, breakable).
    pub flags: u8,
    /// Its breakable surface, if it is breakable.
    pub breakable_surface: u8,
    /// Its material, in the BSP's collision materials.
    pub material: Option<u16>,
}

/// An edge of a collision BSP.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct CollisionEdge {
    /// Its start and end.
    pub vertices: [u32; 2],
    /// The next edge of each of its surfaces.
    pub edges: [u32; 2],
    /// The surfaces on each side, if any.
    pub surfaces: [Option<u32>; 2],
}

/// A vertex of a collision BSP.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct CollisionVertex {
    /// Where it is.
    pub point: [f32; 3],
    /// One of its edges.
    pub first_edge: u32,
}
