//! The test map, `devmap`: a level the engine makes rather than imports. It
//! is a flat square plane, solid to stand on, drawn with a grid of one
//! world unit, with a player start at its middle. Its sky is another
//! level's, when it is made over one, or a generated one: a fog that fades
//! the plane into a blue horizon.

use crate::level::{
    Bsp2dReference, Bsp3dNode, CollisionEdge, CollisionLeaf, CollisionMaterial, CollisionSurface,
    CollisionVertex, EnvironmentVertex, Lightmap, LightmapVertex, Material,
};
use crate::{
    AtmosphericFog, Bitmap, BitmapImage, CollisionBsp, EnvironmentShader, GameTypes, Globals,
    Group, Level, Pixels, PlayerStart, Scenario, Shader, ShaderClass, Sky, StructureBsp, TagRef,
    Texture, TextureFormat,
};

/// The map's name: `--map devmap`, `map devmap`.
pub const NAME: &str = "devmap";
/// Half the plane's side, in world units (so it is 160 across).
pub const HALF_SIZE: f32 = 80.0;
/// The drawn plane's cells along each side (its triangles are kept small,
/// so that the fog and the texture coordinates stay smooth across them).
pub const CELLS: u16 = 32;
/// The grid texture's side, in texels; it repeats each world unit.
pub const GRID_TEXELS: u16 = 64;
/// The physics material the plane is made of (stone).
pub const MATERIAL_TYPE: i16 = 2;

/// The plane's structure BSP's path.
pub const BSP_PATH: &str = r"devmap\devmap";
/// The grid's shader's path.
pub const SHADER_PATH: &str = r"devmap\shaders\grid";
/// The grid's bitmap's path.
pub const BITMAP_PATH: &str = r"devmap\bitmaps\grid";
/// The generated sky's path.
pub const SKY_PATH: &str = r"devmap\sky";

/// The generated sky's horizon colour.
pub const HORIZON: [f32; 3] = [0.42, 0.52, 0.66];

/// A tag of `group` at `path`.
fn tag(group: &[u8; 4], path: &str) -> TagRef {
    TagRef {
        group: Group(*group),
        path: path.into(),
    }
}

/// The grid's shader's tag.
pub fn shader_tag() -> TagRef {
    tag(b"senv", SHADER_PATH)
}

/// The grid's bitmap's tag.
pub fn bitmap_tag() -> TagRef {
    tag(b"bitm", BITMAP_PATH)
}

/// The plane: `half_size` world units each way from the origin at z = 0,
/// drawn as `cells` by `cells` squares, its collision one surface facing
/// up.
pub fn plane(half_size: f32, cells: u16) -> StructureBsp {
    let cells = cells.clamp(1, 255);
    let side = cells + 1;
    let step = 2.0 * half_size / f32::from(cells);
    let mut vertices = Vec::with_capacity(usize::from(side) * usize::from(side));
    for row in 0..side {
        for column in 0..side {
            let x = -half_size + step * f32::from(column);
            let y = -half_size + step * f32::from(row);
            vertices.push(EnvironmentVertex {
                position: [x, y, 0.0],
                normal: [0.0, 0.0, 1.0],
                binormal: [0.0, 1.0, 0.0],
                tangent: [1.0, 0.0, 0.0],
                // (a repeat of the grid each world unit)
                texcoord: [x, -y],
            });
        }
    }
    let mut surfaces = Vec::with_capacity(usize::from(cells) * usize::from(cells) * 2);
    for row in 0..cells {
        for column in 0..cells {
            let corner = row * side + column;
            let (a, b, c, d) = (corner, corner + 1, corner + side + 1, corner + side);
            // (counter-clockwise seen from above)
            surfaces.push([a, b, c]);
            surfaces.push([a, c, d]);
        }
    }
    let lightmap_vertices = vec![
        LightmapVertex {
            incident_direction: [0.0, 0.0, 1.0],
            texcoord: [0.0, 0.0],
        };
        vertices.len()
    ];
    StructureBsp {
        path: BSP_PATH.into(),
        lightmap_bitmap: None,
        world_bounds: [[-half_size, half_size], [-half_size, half_size], [0.0, 0.0]],
        collision: plane_collision(half_size),
        collision_materials: vec![CollisionMaterial {
            shader: Some(shader_tag()),
            material_type: MATERIAL_TYPE,
        }],
        surfaces: surfaces.clone(),
        lightmaps: vec![Lightmap {
            bitmap_index: None,
            materials: vec![Material {
                shader: Some(shader_tag()),
                first_surface: 0,
                surface_count: surfaces.len() as u32,
                vertices,
                lightmap_vertices,
            }],
        }],
    }
}

/// The plane's collision: one square surface on the plane z = 0, facing
/// up, in the leaf above it (below is solid).
fn plane_collision(half_size: f32) -> CollisionBsp {
    let corners = [
        [-half_size, -half_size, 0.0],
        [half_size, -half_size, 0.0],
        [half_size, half_size, 0.0],
        [-half_size, half_size, 0.0],
    ];
    CollisionBsp {
        // (behind the plane: none, all ones; in front: leaf 0)
        bsp3d_nodes: vec![Bsp3dNode {
            plane: 0,
            children: [-1, i32::MIN],
        }],
        planes: vec![[0.0, 0.0, 1.0, 0.0]],
        leaves: vec![CollisionLeaf {
            flags: 0,
            bsp2d_reference_count: 1,
            first_bsp2d_reference: 0,
        }],
        // (the 2D tree is surface 0 alone)
        bsp2d_references: vec![Bsp2dReference {
            plane: 0,
            node: i32::MIN,
        }],
        bsp2d_nodes: Vec::new(),
        surfaces: vec![CollisionSurface {
            plane: 0,
            first_edge: 0,
            flags: 0,
            breakable_surface: 0,
            material: Some(0),
        }],
        edges: (0..4u32)
            .map(|edge| CollisionEdge {
                vertices: [edge, (edge + 1) % 4],
                edges: [(edge + 1) % 4, (edge + 1) % 4],
                surfaces: [Some(0), None],
            })
            .collect(),
        vertices: corners
            .iter()
            .enumerate()
            .map(|(edge, &point)| CollisionVertex {
                point,
                first_edge: edge as u32,
            })
            .collect(),
    }
}

/// The grid texel at `x`, `y` of a level `size` texels across: a line
/// along the cell's edges over a dark grey, a brighter one along two of
/// them so that the cells' corners show.
fn grid_texel(x: u16, y: u16, size: u16) -> [u8; 4] {
    let line = (size / 32).max(1);
    let edge = |at: u16| at < line || at >= size - line;
    match (edge(x), edge(y)) {
        (true, true) => [0xe0, 0xe0, 0xe0, 0xff],
        (true, false) | (false, true) => [0xa8, 0xa8, 0xa8, 0xff],
        (false, false) => [0x50, 0x52, 0x56, 0xff],
    }
}

/// The grid's bitmap: one RGBA image `GRID_TEXELS` square with its
/// mipmaps, each drawn at its own size (so that the lines stay as wide
/// down to the smallest).
pub fn grid_bitmap() -> Bitmap {
    let mut levels = Vec::new();
    let mut size = GRID_TEXELS;
    loop {
        let mut texels = Vec::with_capacity(usize::from(size) * usize::from(size) * 4);
        for y in 0..size {
            for x in 0..size {
                texels.extend_from_slice(&grid_texel(x, y, size));
            }
        }
        levels.push(texels);
        if size == 1 {
            break;
        }
        size /= 2;
    }
    let mipmap_count = levels.len() as u16 - 1;
    Bitmap {
        tag: bitmap_tag(),
        kind: 0,
        format: 0,
        usage: 1,
        flags: 0,
        bump_height: 0.0,
        sequences: Vec::new(),
        images: vec![BitmapImage {
            width: GRID_TEXELS,
            height: GRID_TEXELS,
            depth: 1,
            kind: 0,
            format: 0,
            flags: 1,
            registration_point: [0, 0],
            mipmap_count,
            pixels: Pixels::Texture(Texture {
                format: TextureFormat::Rgba8,
                width: GRID_TEXELS,
                height: GRID_TEXELS,
                depth: 1,
                faces: 1,
                levels,
            }),
        }],
    }
}

/// The grid's shader: an opaque environment shader whose base map is the
/// grid.
pub fn grid_shader() -> Shader {
    Shader {
        tag: shader_tag(),
        material_type: MATERIAL_TYPE,
        class: ShaderClass::Environment(Box::new(EnvironmentShader {
            flags: 0,
            kind: 0,
            diffuse_flags: 0,
            base_map: Some(bitmap_tag()),
            detail_function: 0,
            primary_detail_scale: 0.0,
            primary_detail_map: None,
            secondary_detail_scale: 0.0,
            secondary_detail_map: None,
            micro_detail_function: 0,
            micro_detail_scale: 0.0,
            micro_detail_map: None,
            material_color: [1.0; 3],
            bump_scale: 0.0,
            bump_map: None,
            self_illumination_scale: 0.0,
            self_illumination_map: None,
            specular_brightness: 0.0,
            specular_perpendicular_color: [0.0; 3],
            specular_parallel_color: [0.0; 3],
            reflection_flags: 0,
            reflection_kind: 0,
            lightmap_brightness_scale: 0.0,
            reflection_perpendicular_brightness: 0.0,
            reflection_parallel_brightness: 0.0,
            reflection_cube_map: None,
        })),
    }
}

/// The generated sky: no model, the horizon's colour behind everything,
/// and a fog that fades the plane into it toward its edges.
pub fn generated_sky() -> Sky {
    Sky {
        tag: tag(b"sky ", SKY_PATH),
        model: None,
        outdoor_fog: AtmosphericFog {
            color: HORIZON,
            maximum_density: 1.0,
            start_distance: 20.0,
            opaque_distance: HALF_SIZE * 2.5,
        },
        indoor_fog: AtmosphericFog {
            color: HORIZON,
            maximum_density: 0.0,
            start_distance: 0.0,
            opaque_distance: 0.0,
        },
    }
}

/// The game types a start is for: all of them.
const ALL_GAME_TYPES: GameTypes = GameTypes([12, 0, 0, 0]);

/// The devmap's scenario: the plane, and one player start at its middle,
/// facing along +x. It places nothing.
fn scenario() -> Scenario {
    Scenario {
        path: NAME.into(),
        structure_bsps: vec![tag(b"sbsp", BSP_PATH)],
        player_starts: vec![PlayerStart {
            position: [0.0, 0.0, 0.0],
            facing: 0.0,
            team: 0,
            structure_bsp: Some(0),
            game_types: ALL_GAME_TYPES,
        }],
        placements: Vec::new(),
        netgame_flags: Vec::new(),
        netgame_equipment: Vec::new(),
        scripts: Default::default(),
        campaign: Default::default(),
    }
}

/// The devmap: the plane and its player start. Made over `donor`, it has
/// the donor's definitions (its objects, its globals and the player's
/// biped, its HUD) and the donor's sky; without one, it has nothing but
/// the plane and the generated sky.
pub fn level(donor: Option<&Level>) -> Level {
    let mut level = match donor {
        Some(donor) => donor.clone(),
        None => Level {
            scenario: scenario(),
            structure_bsps: Vec::new(),
            objects: Vec::new(),
            models: Vec::new(),
            animation_graphs: Vec::new(),
            shaders: Vec::new(),
            bitmaps: Vec::new(),
            resource_bitmaps: Vec::new(),
            skies: Vec::new(),
            globals: Globals::default(),
            damage_effects: Vec::new(),
            collision_models: Vec::new(),
            physics: Vec::new(),
            effects: Vec::new(),
            unit_huds: Vec::new(),
            weapon_huds: Vec::new(),
            grenade_huds: Vec::new(),
            hud_digits: None,
            hud_globals: None,
        },
    };
    level.scenario = scenario();
    level.structure_bsps = vec![plane(HALF_SIZE, CELLS)];
    // (the donor's sky outside, if it has one; its others are of its BSPs)
    level.skies.truncate(1);
    if level.skies.is_empty() {
        level.skies.push(generated_sky());
    }
    level.shaders.retain(|shader| shader.tag != shader_tag());
    level.shaders.push(grid_shader());
    level.bitmaps.retain(|bitmap| bitmap.tag != bitmap_tag());
    level.bitmaps.push(grid_bitmap());
    level
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn the_plane_is_drawn_as_its_cells() {
        let bsp = plane(10.0, 4);
        let material = &bsp.lightmaps[0].materials[0];
        assert_eq!(material.vertices.len(), 25);
        assert_eq!(material.lightmap_vertices.len(), 25);
        assert_eq!(bsp.surfaces.len(), 32);
        assert_eq!(material.surface_count, 32);
        assert!(bsp
            .surfaces
            .iter()
            .flatten()
            .all(|&index| usize::from(index) < material.vertices.len()));
        // every triangle faces up, and they cover the square
        let mut area = 0.0;
        for triangle in &bsp.surfaces {
            let [a, b, c] = triangle.map(|index| material.vertices[usize::from(index)].position);
            let cross = (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0]);
            assert!(cross > 0.0, "{triangle:?}");
            area += cross / 2.0;
        }
        assert!((area - 400.0).abs() < 1e-3, "{area}");
        assert_eq!(bsp.world_bounds, [[-10.0, 10.0], [-10.0, 10.0], [0.0, 0.0]]);
        // (the grid repeats each world unit)
        let corner = material.vertices[24];
        assert_eq!(corner.position, [10.0, 10.0, 0.0]);
        assert_eq!(corner.texcoord, [10.0, -10.0]);
    }

    #[test]
    fn the_cells_are_kept_to_what_the_indices_hold() {
        let bsp = plane(1.0, 0);
        assert_eq!(bsp.surfaces.len(), 2);
        let bsp = plane(1.0, 1000);
        assert_eq!(bsp.lightmaps[0].materials[0].vertices.len(), 256 * 256);
    }

    #[test]
    fn the_collision_is_one_square_ring_facing_up() {
        let bsp = plane(5.0, 2).collision;
        assert_eq!(bsp.surfaces.len(), 1);
        assert_eq!(bsp.planes, vec![[0.0, 0.0, 1.0, 0.0]]);
        // the ring, from the surface's first edge
        let mut ring = Vec::new();
        let mut edge = bsp.surfaces[0].first_edge;
        loop {
            let at = &bsp.edges[edge as usize];
            assert_eq!(at.surfaces[0], Some(0));
            ring.push(bsp.vertices[at.vertices[0] as usize].point);
            edge = at.edges[0];
            if edge == bsp.surfaces[0].first_edge {
                break;
            }
        }
        assert_eq!(
            ring,
            vec![
                [-5.0, -5.0, 0.0],
                [5.0, -5.0, 0.0],
                [5.0, 5.0, 0.0],
                [-5.0, 5.0, 0.0]
            ]
        );
        assert_eq!(bsp.leaves[0].bsp2d_reference_count, 1);
        assert_eq!(bsp.bsp2d_references[0].node & i32::MAX, 0);
    }

    #[test]
    fn the_grid_has_every_mipmap() {
        let bitmap = grid_bitmap();
        let image = &bitmap.images[0];
        let Pixels::Texture(texture) = &image.pixels else {
            panic!("the grid is in the resource map");
        };
        assert_eq!(texture.levels.len(), 7);
        assert_eq!(usize::from(image.mipmap_count) + 1, texture.levels.len());
        for (at, bytes) in texture.levels.iter().enumerate() {
            assert_eq!(bytes.len(), texture.level_bytes(at), "level {at}");
        }
        // a line at the edge, the cell's grey inside
        let first = &texture.levels[0];
        assert_eq!(&first[..4], &[0xe0, 0xe0, 0xe0, 0xff]);
        let middle = (32 * 64 + 32) * 4;
        assert_eq!(&first[middle..middle + 4], &[0x50, 0x52, 0x56, 0xff]);
        assert_eq!(grid_texel(5, 0, 64), [0xa8, 0xa8, 0xa8, 0xff]);
    }

    #[test]
    fn a_level_of_its_own_has_the_plane_and_the_generated_sky() {
        let level = level(None);
        assert_eq!(level.scenario.path, NAME);
        assert_eq!(level.structure_bsps.len(), 1);
        assert_eq!(level.scenario.player_starts.len(), 1);
        assert_eq!(level.scenario.player_starts[0].position, [0.0; 3]);
        assert!(level.scenario.placements.is_empty());
        assert_eq!(level.skies, vec![generated_sky()]);
        assert!(level.shader(&shader_tag()).is_some());
        assert!(level.bitmap(&bitmap_tag()).is_some());
        assert!(level.objects.is_empty());
        let fog = level.skies[0].outdoor_fog;
        assert!(fog.maximum_density > 0.0 && fog.opaque_distance > fog.start_distance);
    }

    #[test]
    fn a_level_over_a_donor_keeps_its_definitions_and_its_sky() {
        let mut donor = level(None);
        donor.scenario.path = "donor".into();
        let mut donor_sky = generated_sky();
        donor_sky.tag = tag(b"sky ", "donor sky");
        let mut other_sky = donor_sky.clone();
        other_sky.tag = tag(b"sky ", "inside");
        donor.skies = vec![donor_sky.clone(), other_sky];
        donor.globals.player_unit = Some(tag(b"bipd", "cyborg"));
        donor.structure_bsps.push(plane(1.0, 1));
        let made = level(Some(&donor));
        assert_eq!(made.scenario.path, NAME);
        assert_eq!(made.structure_bsps.len(), 1);
        assert_eq!(made.skies, vec![donor_sky]);
        assert_eq!(made.globals.player_unit, Some(tag(b"bipd", "cyborg")));
        // (the grid once, though the donor had it already)
        assert_eq!(
            made.shaders
                .iter()
                .filter(|shader| shader.tag == shader_tag())
                .count(),
            1
        );
        assert_eq!(
            made.bitmaps
                .iter()
                .filter(|bitmap| bitmap.tag == bitmap_tag())
                .count(),
            1
        );
    }
}
