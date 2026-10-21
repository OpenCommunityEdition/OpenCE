//! A level: its scenario, its structure BSPs and what they are drawn with.

use rkyv::{Archive, Deserialize, Serialize};

use crate::{AnimationGraph, Bitmap, Model, ObjectDefinition, ObjectKind, Shader, TagRef};

/// Everything a level's structure is drawn and collided with.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct Level {
    /// The scenario.
    pub scenario: Scenario,
    /// Its structure BSPs, in the scenario's order.
    pub structure_bsps: Vec<StructureBsp>,
    /// The definitions of the objects the scenario places, by path.
    pub objects: Vec<ObjectDefinition>,
    /// Their models, by path.
    pub models: Vec<Model>,
    /// Their animation graphs, by path.
    pub animation_graphs: Vec<AnimationGraph>,
    /// Every shader the BSPs and models use, and the shaders those layer
    /// on, by path.
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

    /// An object definition, by its tag.
    pub fn object(&self, tag: &TagRef) -> Option<&ObjectDefinition> {
        self.objects.iter().find(|object| object.tag == *tag)
    }

    /// A model's index in [`Level::models`], by its tag.
    pub fn model_index(&self, tag: &TagRef) -> Option<usize> {
        self.models.iter().position(|model| model.tag == *tag)
    }

    /// An animation graph, by its tag.
    pub fn animation_graph(&self, tag: &TagRef) -> Option<&AnimationGraph> {
        self.animation_graphs.iter().find(|graph| graph.tag == *tag)
    }

    /// A bitmap, by its tag.
    pub fn bitmap(&self, tag: &TagRef) -> Option<&Bitmap> {
        self.bitmaps.iter().find(|bitmap| bitmap.tag == *tag)
    }
}

/// A scenario, as far as its structure goes.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct Scenario {
    /// Its path.
    pub path: String,
    /// Its structure BSPs.
    pub structure_bsps: Vec<TagRef>,
    /// Where players start.
    pub player_starts: Vec<PlayerStart>,
    /// The objects it places, kind by kind in the scenario's order.
    pub placements: Vec<ObjectPlacement>,
}

/// An object the scenario places.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct ObjectPlacement {
    /// Its kind: the scenario's block it is in.
    pub kind: ObjectKind,
    /// Its object tag, from the kind's palette.
    pub tag: TagRef,
    /// Its name, from the scenario's object names.
    pub name: Option<String>,
    /// When it is placed: bit 0, not automatically; bits 1 to 3, not on
    /// easy, normal or hard.
    pub placement_flags: u16,
    /// Where it is.
    pub position: [f32; 3],
    /// Its yaw, pitch and roll, in radians.
    pub rotation: [f32; 3],
    /// The structure BSPs it is in, a bit each.
    pub structure_bsps: u16,
    /// Its colors and regions' permutations (scenery, units and weapons).
    pub permutation: Option<PlacementPermutation>,
}

impl ObjectPlacement {
    /// Whether it is placed when the level starts.
    pub fn is_automatic(&self) -> bool {
        self.placement_flags & 1 == 0
    }
}

/// A placed object's appearance.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Archive, Serialize, Deserialize)]
pub struct PlacementPermutation {
    /// Its four change colors, each 0xAARRGGBB (zero: the object's own).
    pub change_colors: [u32; 4],
    /// The permutation of each of its model's regions.
    pub region_permutations: [u8; 8],
}

/// A place a player starts, as the scenario gives it.
#[derive(Clone, Copy, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct PlayerStart {
    /// Where its feet are.
    pub position: [f32; 3],
    /// The way it faces: radians left from +x.
    pub facing: f32,
    /// The team it is for (multiplayer).
    pub team: u16,
    /// The structure BSP it is in (none: any).
    pub structure_bsp: Option<u16>,
}

/// A structure BSP: the level's fixed geometry, its lightmaps and its
/// collision.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
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
#[derive(Clone, Debug, PartialEq, Eq, Archive, Serialize, Deserialize)]
pub struct CollisionMaterial {
    /// The shader drawn there.
    pub shader: Option<TagRef>,
    /// The physics material type (what a footstep or a bullet does).
    pub material_type: i16,
}

/// The geometry that shares one lightmap page.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct Lightmap {
    /// The page's index in the BSP's lightmap bitmap, if it has one.
    pub bitmap_index: Option<u16>,
    /// Its materials.
    pub materials: Vec<Material>,
}

/// The triangles of a lightmap page that share a shader.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
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
#[derive(Clone, Copy, Debug, PartialEq, Archive, Serialize, Deserialize)]
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
#[derive(Clone, Copy, Debug, PartialEq, Archive, Serialize, Deserialize)]
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
#[derive(Clone, Debug, Default, PartialEq, Archive, Serialize, Deserialize)]
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
#[derive(Clone, Copy, Debug, PartialEq, Eq, Archive, Serialize, Deserialize)]
pub struct Bsp3dNode {
    /// Its plane designator.
    pub plane: i32,
    /// Behind and in front of the plane.
    pub children: [i32; 2],
}

/// A leaf of a collision BSP.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Archive, Serialize, Deserialize)]
pub struct CollisionLeaf {
    /// Its flags.
    pub flags: u16,
    /// How many 2D references it has.
    pub bsp2d_reference_count: u16,
    /// Its first 2D reference.
    pub first_bsp2d_reference: u32,
}

/// A 2D reference of a collision BSP: a plane and the 2D tree in it.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Archive, Serialize, Deserialize)]
pub struct Bsp2dReference {
    /// Its plane designator.
    pub plane: i32,
    /// Its tree's root.
    pub node: i32,
}

/// A 2D node of a collision BSP.
#[derive(Clone, Copy, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct Bsp2dNode {
    /// Its line: normal, then distance.
    pub plane: [f32; 3],
    /// Behind and in front of the line.
    pub children: [i32; 2],
}

/// A surface of a collision BSP.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Archive, Serialize, Deserialize)]
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
#[derive(Clone, Copy, Debug, PartialEq, Eq, Archive, Serialize, Deserialize)]
pub struct CollisionEdge {
    /// Its start and end.
    pub vertices: [u32; 2],
    /// The next edge of each of its surfaces.
    pub edges: [u32; 2],
    /// The surfaces on each side, if any.
    pub surfaces: [Option<u32>; 2],
}

/// A vertex of a collision BSP.
#[derive(Clone, Copy, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct CollisionVertex {
    /// Where it is.
    pub point: [f32; 3],
    /// One of its edges.
    pub first_edge: u32,
}
