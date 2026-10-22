//! The level's object models made ready for the GPU: their parts' vertices,
//! skinned to two nodes each, in one vertex buffer; their strips as
//! triangle lists in one index buffer; a draw for each part, with the
//! material of its model shader; and the regions' permutations and levels
//! of detail that choose which parts are drawn.

use opence_content::{Model, ModelShaderClass, ModelVertex};

use crate::render_level::{
    finite, scale, BlendFunction, Builder, MaterialKind, RenderDraw, RenderMaterial,
};

/// The bytes of one model vertex, little-endian: position and normal (three
/// floats each), texture coordinates (two floats), the two nodes it moves
/// with (a `u16` each) and how much it moves with each (two floats).
pub const MODEL_VERTEX_STRIDE: usize = 44;

/// The levels of detail a model has, super low to super high.
pub const DETAIL_LEVELS: usize = 5;

/// A model, ready for the GPU.
#[derive(Clone, Debug, PartialEq, uniffi::Record)]
pub struct RenderModel {
    /// The pixel sizes below which each level of detail is not drawn,
    /// super low to super high (below the first, the model is not drawn).
    pub detail_cutoffs: Vec<f32>,
    /// Its nodes.
    pub node_count: u32,
    /// Each node's matrix at the default pose, inverted, which the node's
    /// matrix in the world is multiplied by to skin the vertices: thirteen
    /// floats each (scale; forward, left and up; position).
    pub default_inverses: Vec<f32>,
    /// Its regions.
    pub regions: Vec<RenderRegion>,
    /// Its geometries, which the permutations choose from.
    pub geometries: Vec<RenderGeometry>,
}

/// A region of a model: its permutations, one of which is drawn.
#[derive(Clone, Debug, PartialEq, uniffi::Record)]
pub struct RenderRegion {
    /// Its name.
    pub name: String,
    /// Its permutations.
    pub permutations: Vec<RenderPermutation>,
}

/// A way a region may look.
#[derive(Clone, Debug, PartialEq, uniffi::Record)]
pub struct RenderPermutation {
    /// Its name.
    pub name: String,
    /// Its geometry at each level of detail, super low to super high.
    pub geometries: Vec<Option<u32>>,
}

/// A geometry: the draws of its parts.
#[derive(Clone, Debug, PartialEq, uniffi::Record)]
pub struct RenderGeometry {
    /// Its parts' draws, into the model index buffer.
    pub parts: Vec<RenderDraw>,
}

/// The level of detail a model is drawn at, by the diameter in pixels of
/// its object's bounding sphere, as the game's `render_model` chooses it:
/// the highest whose cutoff the size reaches, or none below the lowest's.
#[uniffi::export]
pub fn model_level_of_detail(detail_cutoffs: Vec<f32>, pixels: f32) -> Option<u32> {
    let lowest = *detail_cutoffs.first()?;
    // (a NaN size is never drawn)
    if pixels.is_nan() || pixels < lowest {
        return None;
    }
    let mut level = detail_cutoffs.len().min(DETAIL_LEVELS) - 1;
    while level > 0 && pixels < detail_cutoffs[level] {
        level -= 1;
    }
    Some(level as u32)
}

/// The diameter in pixels of a sphere of `radius` at `depth` along the
/// view, as the game's `render_frustum_sphere_diameter_in_pixels` gives
/// it, for a view `viewport_height` pixels high seeing `vertical_fov`
/// radians from top to bottom.
#[uniffi::export]
pub fn sphere_diameter_pixels(
    depth: f32,
    radius: f32,
    vertical_fov: f32,
    viewport_height: f32,
) -> f32 {
    let projection = viewport_height * 0.5 / libm::tanf(vertical_fov * 0.5);
    projection / depth.abs().max(0.1) * radius * 2.0
}

/// A triangle strip's triangles, each wound as the strip winds its first:
/// those with a vertex twice (the strip's joins) or past `vertex_count`
/// are left out.
pub fn strip_triangles(strip: &[u16], vertex_count: usize) -> Vec<[u16; 3]> {
    strip
        .windows(3)
        .enumerate()
        .filter_map(|(at, window)| {
            let [a, b, c] = [window[0], window[1], window[2]];
            let triangle = if at % 2 == 0 { [a, b, c] } else { [b, a, c] };
            let distinct = a != b && b != c && a != c;
            let inside = triangle
                .iter()
                .all(|&vertex| usize::from(vertex) < vertex_count);
            (distinct && inside).then_some(triangle)
        })
        .collect()
}

/// A vertex's bytes, its texture coordinates scaled by the model's base
/// map scale. A node past the model's, or none, is the first node with no
/// weight; a vertex with no node it may move with moves with the first.
fn push_vertex(out: &mut Vec<u8>, vertex: &ModelVertex, uv_scale: [f32; 2], node_count: usize) {
    let mut nodes = [0u16; 2];
    let mut weights = [0.0f32; 2];
    for slot in 0..2 {
        if let Some(node) = vertex.nodes[slot].filter(|&node| usize::from(node) < node_count) {
            nodes[slot] = node;
            weights[slot] = finite(vertex.weights[slot]).clamp(0.0, 1.0);
        }
    }
    if weights[0] + weights[1] <= 0.0 {
        weights = [1.0, 0.0];
    }
    let texcoord = [
        vertex.texcoord[0] * uv_scale[0],
        vertex.texcoord[1] * uv_scale[1],
    ];
    for value in vertex
        .position
        .iter()
        .chain(&vertex.normal)
        .chain(&texcoord)
        .map(|&value| finite(value))
    {
        out.extend_from_slice(&value.to_le_bytes());
    }
    for node in nodes {
        out.extend_from_slice(&node.to_le_bytes());
    }
    for weight in weights {
        out.extend_from_slice(&weight.to_le_bytes());
    }
}

/// A model shader's material: alpha-tested by its base map unless it is
/// not, blended if it is an alpha-blended decal.
pub(crate) fn model_material(
    builder: &mut Builder<'_>,
    shader: &ModelShaderClass,
) -> RenderMaterial {
    let kind = if shader.flags & (1 << 3) != 0 {
        MaterialKind::Blended
    } else if shader.flags & (1 << 2) != 0 {
        MaterialKind::Opaque
    } else {
        MaterialKind::AlphaTested
    };
    RenderMaterial {
        kind,
        blend: BlendFunction::AlphaBlend,
        environment_kind: 0,
        detail_function: shader.detail_function.clamp(0, 2) as u8,
        base_map: builder.map(shader.base_map.as_ref()),
        base_scale: vec![scale(shader.map_scale[0]), scale(shader.map_scale[1])],
        base_offset: vec![0.0, 0.0],
        primary_detail_map: builder.map(shader.detail_map.as_ref()),
        primary_detail_scale: scale(shader.detail_map_scale),
        secondary_detail_map: None,
        secondary_detail_scale: 1.0,
        multipurpose_map: builder.map(shader.multipurpose_map.as_ref()),
        detail_mask: shader.detail_mask.clamp(0, 9) as u8,
        change_color_source: shader.change_color_source.clamp(0, 4) as u8,
        chicago_maps: Vec::new(),
        environment: None,
    }
}

impl Builder<'_> {
    /// A model's vertices and triangles added to the model buffers, and
    /// its draws, regions and levels of detail.
    pub(crate) fn model(&mut self, model: &Model) -> RenderModel {
        let node_count = model.nodes.len();
        let uv_scale = [
            scale(model.base_map_scale[0]),
            scale(model.base_map_scale[1]),
        ];
        let mut geometries = Vec::with_capacity(model.geometries.len());
        for geometry in &model.geometries {
            let mut parts = Vec::with_capacity(geometry.parts.len());
            for part in &geometry.parts {
                let Some(material) = model
                    .shaders
                    .get(usize::from(part.shader))
                    .and_then(|shader| shader.shader.as_ref())
                    .and_then(|shader| self.material(shader))
                else {
                    continue;
                };
                let triangles = strip_triangles(&part.strip, part.vertices.len());
                let Ok(base) = u32::try_from(self.out.model_vertices.len() / MODEL_VERTEX_STRIDE)
                else {
                    continue;
                };
                if triangles.is_empty() {
                    continue;
                }
                let first_index = self.out.model_indices.len() as u32;
                self.out.model_indices.extend(
                    triangles
                        .iter()
                        .flatten()
                        .map(|&vertex| base + u32::from(vertex)),
                );
                for vertex in &part.vertices {
                    push_vertex(&mut self.out.model_vertices, vertex, uv_scale, node_count);
                }
                parts.push(RenderDraw {
                    first_index,
                    index_count: (triangles.len() * 3) as u32,
                    material,
                    lightmap: None,
                    unbaked: false,
                    center: part.centroid.iter().map(|&value| finite(value)).collect(),
                });
            }
            geometries.push(RenderGeometry { parts });
        }
        let geometry_count = geometries.len();
        let regions = model
            .regions
            .iter()
            .map(|region| RenderRegion {
                name: region.name.clone(),
                permutations: region
                    .permutations
                    .iter()
                    .map(|permutation| RenderPermutation {
                        name: permutation.name.clone(),
                        geometries: permutation
                            .geometries
                            .iter()
                            .map(|geometry| {
                                geometry
                                    .map(u32::from)
                                    .filter(|&index| (index as usize) < geometry_count)
                            })
                            .collect(),
                    })
                    .collect(),
            })
            .collect();
        let default_inverses = model
            .nodes
            .iter()
            .flat_map(|node| {
                std::iter::once(node.default_inverse_scale)
                    .chain(node.default_inverse.iter().flatten().copied())
                    .map(finite)
            })
            .collect();
        RenderModel {
            detail_cutoffs: model
                .detail_cutoffs
                .iter()
                .map(|&cutoff| finite(cutoff))
                .collect(),
            node_count: node_count as u32,
            default_inverses,
            regions,
            geometries,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn a_strips_joins_and_strays_are_left_out() {
        // (0 1 2, then 2 1 3 wound back; 2 3 3 and 3 3 4 are the join)
        let strip = [0, 1, 2, 3, 3, 4, 4, 5, 6, 9];
        assert_eq!(
            strip_triangles(&strip, 7),
            vec![[0, 1, 2], [2, 1, 3], [4, 5, 6]]
        );
        assert!(strip_triangles(&[0, 1], 2).is_empty());
    }

    #[test]
    fn the_level_of_detail_is_the_highest_the_size_reaches() {
        let cutoffs = vec![10.0, 20.0, 40.0, 80.0, 160.0];
        assert_eq!(model_level_of_detail(cutoffs.clone(), 5.0), None);
        assert_eq!(model_level_of_detail(cutoffs.clone(), 10.0), Some(0));
        assert_eq!(model_level_of_detail(cutoffs.clone(), 39.9), Some(1));
        assert_eq!(model_level_of_detail(cutoffs.clone(), 80.0), Some(3));
        assert_eq!(model_level_of_detail(cutoffs.clone(), 1e9), Some(4));
        assert_eq!(model_level_of_detail(cutoffs, f32::NAN), None);
        // (cutoffs of zero, as most models have for their lowest levels)
        assert_eq!(model_level_of_detail(vec![0.0; 5], 0.0), Some(4));
        assert_eq!(model_level_of_detail(Vec::new(), 100.0), None);
    }

    #[test]
    fn a_nearer_sphere_is_more_pixels() {
        let fov = std::f32::consts::FRAC_PI_2;
        // (at a depth equal to the radius, with a 90 degree view, the
        // sphere is as high as the view)
        let full = sphere_diameter_pixels(1.0, 1.0, fov, 1080.0);
        assert!((full - 1080.0).abs() < 0.01, "{full}");
        assert!(sphere_diameter_pixels(10.0, 1.0, fov, 1080.0) < full);
        assert_eq!(
            sphere_diameter_pixels(-10.0, 1.0, fov, 1080.0),
            sphere_diameter_pixels(10.0, 1.0, fov, 1080.0)
        );
        // (a sphere at the eye is as near as a tenth of a unit)
        assert_eq!(
            sphere_diameter_pixels(0.0, 1.0, fov, 1080.0),
            sphere_diameter_pixels(0.1, 1.0, fov, 1080.0)
        );
    }

    #[test]
    fn a_vertex_moves_with_nodes_the_model_has() {
        let vertex = ModelVertex {
            position: [1.0, 2.0, 3.0],
            normal: [0.0, 0.0, 1.0],
            binormal: [0.0; 3],
            tangent: [0.0; 3],
            texcoord: [0.5, 0.25],
            nodes: [Some(7), Some(1)],
            weights: [0.75, 0.25],
        };
        let mut out = Vec::new();
        push_vertex(&mut out, &vertex, [2.0, 4.0], 4);
        assert_eq!(out.len(), MODEL_VERTEX_STRIDE);
        let float = |out: &[u8], at: usize| {
            f32::from_le_bytes([out[at], out[at + 1], out[at + 2], out[at + 3]])
        };
        let node = |out: &[u8], at: usize| u16::from_le_bytes([out[at], out[at + 1]]);
        assert_eq!((float(&out, 24), float(&out, 28)), (1.0, 1.0));
        // (node 7 is past the four: its weight goes)
        assert_eq!((node(&out, 32), node(&out, 34)), (0, 1));
        assert_eq!((float(&out, 36), float(&out, 40)), (0.0, 0.25));

        out.clear();
        push_vertex(
            &mut out,
            &ModelVertex {
                nodes: [None, None],
                ..vertex
            },
            [1.0, 1.0],
            4,
        );
        assert_eq!((float(&out, 36), float(&out, 40)), (1.0, 0.0));
    }
}
