//! The test map, `devmap`: a level the engine makes rather than imports. It
//! is a flat square plane, solid to stand on, drawn with a grid of one
//! world unit, with a player start at its middle. Its sky is another
//! level's, when it is made over one, or a generated one: a fog that fades
//! the plane into a blue horizon.
//!
//! Made over every map's level ([`gathered`]), it holds each definition
//! the maps have, once, and shows each object in a gallery of its kind
//! ([`galleries`]), in rows in front of the start.
//!
//! Behind the start are the test stations ([`STATIONS`]): spots the
//! `station` command runs a test from, with the ramps of the vehicles'
//! course and the physics' slope ([`SOLIDS`]) standing on the plane.

use crate::level::{
    Bsp2dReference, Bsp3dNode, CollisionEdge, CollisionLeaf, CollisionMaterial, CollisionSurface,
    CollisionVertex, EnvironmentVertex, Lightmap, LightmapVertex, Material,
};
use std::collections::HashSet;

use crate::{
    AtmosphericFog, Bitmap, BitmapImage, CollisionBsp, EnvironmentShader, GameTypes, Globals,
    Group, Level, ObjectDefinition, ObjectKind, ObjectPlacement, Pixels, PlayerStart, Scenario,
    Shader, ShaderClass, Sky, StructureBsp, TagRef, Texture, TextureFormat,
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
        cluster_skies: Vec::new(),
        lens_flare_markers: Vec::new(),
        lens_flares: Vec::new(),
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
    Shader::new(
        shader_tag(),
        MATERIAL_TYPE,
        ShaderClass::Environment(Box::new(EnvironmentShader {
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
            self_illumination_flags: 0,
            self_illumination_scale: 0.0,
            self_illumination_map: None,
            specular_flags: 0,
            specular_brightness: 0.0,
            specular_perpendicular_color: [0.0; 3],
            specular_parallel_color: [0.0; 3],
            reflection_flags: 0,
            reflection_kind: 0,
            lightmap_brightness_scale: 0.0,
            reflection_perpendicular_brightness: 0.0,
            reflection_parallel_brightness: 0.0,
            reflection_cube_map: None,
            animation: crate::EnvironmentAnimation::default(),
        })),
    )
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
        indoor_ambient: crate::SkyAmbient::default(),
        outdoor_ambient: crate::SkyAmbient {
            color: HORIZON,
            power: 0.5,
        },
        // (a sun high in the south-west, its light white)
        lights: vec![crate::SkyLight {
            lens_flare: None,
            flags: 0b11,
            color: [1.0; 3],
            power: 1.0,
            test_distance: 0.0,
            direction: [3.9, 0.9],
            diameter: 0.01,
        }],
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

/// Adds to `into` each of `from` whose tag it does not hold yet.
fn merge<T: Clone>(into: &mut Vec<T>, from: &[T], tag: impl Fn(&T) -> &TagRef) {
    let mut held: HashSet<TagRef> = into.iter().map(|item| tag(item).clone()).collect();
    for item in from {
        if held.insert(tag(item).clone()) {
            into.push(item.clone());
        }
    }
}

/// A wedge standing on the plane: a ramp rising from the middle of its
/// foot's edge, `foot`, along `rising` (a unit vector on the plane) for
/// `length`, to a vertical back `height` high, `width` across.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Wedge {
    /// The middle of its foot's edge, on the plane.
    pub foot: [f32; 2],
    /// The way it rises, a unit vector on the plane.
    pub rising: [f32; 2],
    /// How far it rises along, in world units.
    pub length: f32,
    /// How wide it is.
    pub width: f32,
    /// How high its back is.
    pub height: f32,
}

impl Wedge {
    /// Its faces: its slope, its back and its two sides (not its bottom,
    /// which lies on the plane), each a ring of points counter-clockwise
    /// seen from outside.
    pub fn faces(&self) -> [Vec<[f32; 3]>; 4] {
        let [fx, fy] = self.rising;
        // (its left, across the way it rises)
        let [sx, sy] = [-fy, fx];
        let half = 0.5 * self.width;
        let at = |along: f32, across: f32, z: f32| {
            [
                self.foot[0] + fx * along + sx * across,
                self.foot[1] + fy * along + sy * across,
                z,
            ]
        };
        let (length, height) = (self.length, self.height);
        let a = at(0.0, -half, 0.0);
        let b = at(0.0, half, 0.0);
        let c = at(length, half, 0.0);
        let d = at(length, -half, 0.0);
        let c_top = at(length, half, height);
        let d_top = at(length, -half, height);
        [
            vec![a, d_top, c_top, b],
            vec![d, c, c_top, d_top],
            vec![a, d, d_top],
            vec![b, c_top, c],
        ]
    }

    /// Its height over the point `x`, `y` of the plane (0 off it).
    pub fn height_at(&self, x: f32, y: f32) -> f32 {
        let [dx, dy] = [x - self.foot[0], y - self.foot[1]];
        let along = dx * self.rising[0] + dy * self.rising[1];
        let across = -dx * self.rising[1] + dy * self.rising[0];
        if (0.0..=self.length).contains(&along) && across.abs() <= 0.5 * self.width {
            self.height * along / self.length
        } else {
            0.0
        }
    }
}

/// The vehicles' course: a loop, driven from its first corner to each in
/// turn and back.
pub const COURSE: [[f32; 2]; 4] = [[-30.0, 10.0], [-30.0, 70.0], [-70.0, 70.0], [-70.0, 10.0]];

/// The course's ramps, one on each of its long sides, rising the way it
/// is driven.
pub const RAMPS: [Wedge; 2] = [
    Wedge {
        foot: [-30.0, 30.0],
        rising: [0.0, 1.0],
        length: 8.0,
        width: 7.0,
        height: 1.5,
    },
    Wedge {
        foot: [-70.0, 50.0],
        rising: [0.0, -1.0],
        length: 8.0,
        width: 7.0,
        height: 1.5,
    },
];

/// The physics station's slope, which the crates fall onto and slide
/// down toward the station.
pub const SLOPE: Wedge = Wedge {
    foot: [-30.0, -40.0],
    rising: [-1.0, 0.0],
    length: 24.0,
    width: 14.0,
    height: 8.0,
};

/// Every solid on the plane: the ramps and the slope.
pub const SOLIDS: [Wedge; 3] = [RAMPS[0], RAMPS[1], SLOPE];

/// A test station: where the `station` command puts the player, and the
/// way it faces (radians from +x, turning left).
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Station {
    /// Its name: `station <name>`.
    pub name: &'static str,
    /// Where the player stands.
    pub at: [f32; 2],
    /// The way the player faces.
    pub facing: f32,
    /// What it tests.
    pub help: &'static str,
}

/// The stations, behind the start (along -x), clear of the galleries.
pub const STATIONS: [Station; 5] = [
    Station {
        name: "weapons",
        at: [-8.0, -15.0],
        facing: core::f32::consts::PI,
        help: "fires each weapon at targets: its rate of fire, damage and heat",
    },
    Station {
        name: "vehicles",
        at: COURSE[0],
        facing: core::f32::consts::FRAC_PI_2,
        help: "drives each vehicle round the course and over its ramps",
    },
    Station {
        name: "animation",
        at: [-8.0, -62.0],
        facing: core::f32::consts::PI,
        help: "plays each animation of a biped's graph in turn, named",
    },
    Station {
        name: "doors",
        at: [-8.0, 45.0],
        facing: core::f32::consts::PI,
        help: "moves every device through its positions",
    },
    Station {
        name: "physics",
        at: [-22.0, -40.0],
        facing: core::f32::consts::PI,
        help: "drops a pile of crates onto the slope",
    },
];

/// The station named `name`.
pub fn station(name: &str) -> Option<&'static Station> {
    STATIONS.iter().find(|station| station.name == name)
}

/// A face's plane: its unit normal (from its ring, counter-clockwise seen
/// from outside) and its distance from the origin along it.
fn face_plane(ring: &[[f32; 3]]) -> [f32; 4] {
    let mut normal = [0.0f32; 3];
    for (at, a) in ring.iter().enumerate() {
        let b = ring[(at + 1) % ring.len()];
        normal[0] += (a[1] - b[1]) * (a[2] + b[2]);
        normal[1] += (a[2] - b[2]) * (a[0] + b[0]);
        normal[2] += (a[0] - b[0]) * (a[1] + b[1]);
    }
    let length = libm::sqrtf(normal.iter().map(|value| value * value).sum());
    let [x, y, z] = if length > 0.0 {
        normal.map(|value| value / length)
    } else {
        [0.0, 0.0, 1.0]
    };
    let distance = ring.first().map_or(0.0, |p| x * p[0] + y * p[1] + z * p[2]);
    [x, y, z, distance]
}

/// Adds `solids` to `bsp`: each face a collision surface of its own ring
/// (in the plane's leaf, which the movement and physics do not read: they
/// collide with the surfaces), and drawn with the grid as a second
/// material.
pub fn add_solids(bsp: &mut StructureBsp, solids: &[Wedge]) {
    let mut vertices = Vec::new();
    let mut triangles: Vec<[u16; 3]> = Vec::new();
    let mut top = bsp.world_bounds[2][1];
    for solid in solids {
        top = top.max(solid.height);
        for ring in solid.faces() {
            let plane = face_plane(&ring);
            let collision = &mut bsp.collision;
            let (Ok(plane_index), Ok(surface), Ok(first_edge), Ok(first_vertex)) = (
                i32::try_from(collision.planes.len()),
                u32::try_from(collision.surfaces.len()),
                u32::try_from(collision.edges.len()),
                u32::try_from(collision.vertices.len()),
            ) else {
                return;
            };
            collision.planes.push(plane);
            collision.surfaces.push(CollisionSurface {
                plane: plane_index,
                first_edge,
                flags: 0,
                breakable_surface: 0,
                material: Some(0),
            });
            let count = ring.len() as u32;
            for (at, &point) in ring.iter().enumerate() {
                let at = at as u32;
                let next = first_edge + (at + 1) % count;
                collision.edges.push(CollisionEdge {
                    vertices: [first_vertex + at, first_vertex + (at + 1) % count],
                    edges: [next, next],
                    surfaces: [Some(surface), None],
                });
                collision.vertices.push(CollisionVertex {
                    point,
                    first_edge: first_edge + at,
                });
            }
            // (drawn as a fan, the grid laid across it from above, or from
            // the side for a wall)
            let Ok(base) = u16::try_from(vertices.len()) else {
                return;
            };
            let normal = [plane[0], plane[1], plane[2]];
            for &position in &ring {
                let texcoord = if normal[2].abs() > 0.5 {
                    [position[0], -position[1]]
                } else {
                    [position[0] + position[1], -position[2]]
                };
                vertices.push(EnvironmentVertex {
                    position,
                    normal,
                    binormal: [0.0, 0.0, 1.0],
                    tangent: [-normal[1], normal[0], 0.0],
                    texcoord,
                });
            }
            for at in 1..ring.len().saturating_sub(1) as u16 {
                triangles.push([base, base + at, base + at + 1]);
            }
        }
    }
    bsp.world_bounds[2][1] = top;
    let Some(lightmap) = bsp.lightmaps.first_mut() else {
        return;
    };
    let lightmap_vertices = vec![
        LightmapVertex {
            incident_direction: [0.0, 0.0, 1.0],
            texcoord: [0.0, 0.0],
        };
        vertices.len()
    ];
    lightmap.materials.push(Material {
        shader: Some(shader_tag()),
        first_surface: bsp.surfaces.len() as u32,
        surface_count: triangles.len() as u32,
        vertices,
        lightmap_vertices,
    });
    bsp.surfaces.extend(triangles);
}

/// The ground: the plane, with the stations' solids on it.
pub fn ground(half_size: f32, cells: u16) -> StructureBsp {
    let mut bsp = plane(half_size, cells);
    add_solids(&mut bsp, &SOLIDS);
    bsp
}

/// One level holding every definition of `levels`, each once: a tag the
/// levels share is the first's, with its own data. The globals, the HUD's
/// globals and the skies are the first level's that has a player's biped
/// (or the first's); the scenario and BSPs are theirs too, for
/// [`level`] to replace.
pub fn gathered(levels: &[Level]) -> Option<Level> {
    let base = levels
        .iter()
        .position(|level| {
            level.globals.player_unit.is_some() || level.globals.multiplayer_unit.is_some()
        })
        .unwrap_or(0);
    let mut gathered = levels.get(base)?.clone();
    for (at, level) in levels.iter().enumerate() {
        if at == base {
            continue;
        }
        merge(&mut gathered.objects, &level.objects, |item| &item.tag);
        merge(&mut gathered.models, &level.models, |item| &item.tag);
        merge(
            &mut gathered.animation_graphs,
            &level.animation_graphs,
            |item| &item.tag,
        );
        merge(&mut gathered.shaders, &level.shaders, |item| &item.tag);
        merge(&mut gathered.bitmaps, &level.bitmaps, |item| &item.tag);
        merge(
            &mut gathered.resource_bitmaps,
            &level.resource_bitmaps,
            |item| item,
        );
        merge(
            &mut gathered.damage_effects,
            &level.damage_effects,
            |item| &item.tag,
        );
        merge(
            &mut gathered.collision_models,
            &level.collision_models,
            |item| &item.tag,
        );
        merge(&mut gathered.physics, &level.physics, |item| &item.tag);
        merge(&mut gathered.effects, &level.effects, |item| &item.tag);
        merge(&mut gathered.unit_huds, &level.unit_huds, |item| &item.tag);
        merge(&mut gathered.weapon_huds, &level.weapon_huds, |item| {
            &item.tag
        });
        merge(&mut gathered.grenade_huds, &level.grenade_huds, |item| {
            &item.tag
        });
        if gathered.hud_digits.is_none() {
            gathered.hud_digits.clone_from(&level.hud_digits);
        }
        if gathered.hud_globals.is_none() {
            gathered.hud_globals.clone_from(&level.hud_globals);
        }
    }
    Some(gathered)
}

/// The galleries, in order from the start: each one's heading and the
/// kinds it shows. (Placeholders and sound scenery are not seen.)
pub const GALLERIES: [(&str, &[ObjectKind]); 7] = [
    ("weapons", &[ObjectKind::Weapon]),
    ("vehicles", &[ObjectKind::Vehicle]),
    ("bipeds", &[ObjectKind::Biped]),
    (
        "devices",
        &[
            ObjectKind::Machine,
            ObjectKind::Control,
            ObjectKind::LightFixture,
        ],
    ),
    ("scenery", &[ObjectKind::Scenery, ObjectKind::Garbage]),
    ("equipment", &[ObjectKind::Equipment]),
    ("projectiles", &[ObjectKind::Projectile]),
];

/// How far in front of the start the first gallery's first row is.
pub const GALLERY_START: f32 = 6.0;
/// The space kept clear at the plane's edges.
pub const GALLERY_EDGE: f32 = 2.0;
/// The space between two objects' slots, and between two rows.
pub const SLOT_MARGIN: f32 = 1.0;
/// The space between two galleries.
pub const GALLERY_GAP: f32 = 4.0;
/// The smallest radius a slot is given.
pub const MIN_SLOT_RADIUS: f32 = 0.25;
/// The height of a gallery's heading.
pub const HEADING_HEIGHT: f32 = 3.0;

/// The galleries laid out: the placements, where each gallery's heading
/// is, and how large the plane must be to hold them.
#[derive(Clone, Debug, PartialEq)]
pub struct Galleries {
    /// One placement of each object shown, named by its tag's path.
    pub placements: Vec<ObjectPlacement>,
    /// Each gallery that shows something: its heading and where it is.
    pub headings: Vec<(&'static str, [f32; 3])>,
    /// Half the side of a plane that holds them all.
    pub half_size: f32,
    /// The objects too wide for a row, which are not shown.
    pub left_out: Vec<TagRef>,
}

/// A slot's radius: the object's bounding sphere's.
fn slot_radius(object: &ObjectDefinition) -> f32 {
    if object.bounding_radius.is_finite() {
        object.bounding_radius.max(MIN_SLOT_RADIUS)
    } else {
        MIN_SLOT_RADIUS
    }
}

/// Lays out `objects` in their galleries, on a plane of at least
/// `HALF_SIZE`: each gallery's objects in rows across the plane (along y),
/// the rows and galleries one after another away from the start (along
/// +x), each object in a slot of its bounding sphere's width, in its tag's
/// order, facing the start. Weapons, vehicles, equipment and projectiles
/// rest on their spheres; the others stand at their origins.
pub fn galleries(objects: &[ObjectDefinition]) -> Galleries {
    let half_width = HALF_SIZE - GALLERY_EDGE;
    let mut placements = Vec::new();
    let mut left_out = Vec::new();
    let mut row_x = GALLERY_START;
    for (_, kinds) in GALLERIES {
        let mut shown: Vec<&ObjectDefinition> = objects
            .iter()
            .filter(|object| kinds.contains(&object.kind))
            .collect();
        shown.sort_by(|a, b| a.tag.cmp(&b.tag));
        let mut y = -half_width;
        let mut depth: f32 = 0.0;
        for object in shown {
            let radius = slot_radius(object);
            if 2.0 * radius > 2.0 * half_width {
                left_out.push(object.tag.clone());
                continue;
            }
            if y + 2.0 * radius > half_width {
                row_x += depth + SLOT_MARGIN;
                y = -half_width;
                depth = 0.0;
            }
            let finite = object.bounding_offset.iter().all(|value| value.is_finite());
            let [offset_x, offset_y, offset_z] = if finite {
                object.bounding_offset
            } else {
                [0.0; 3]
            };
            // (turned half round to face the start, the sphere's offset from
            // the origin is reversed: the origin is the slot's middle plus it)
            let center = [row_x + radius, y + radius];
            let resting = matches!(
                object.kind,
                ObjectKind::Weapon
                    | ObjectKind::Vehicle
                    | ObjectKind::Equipment
                    | ObjectKind::Projectile
            );
            let z = if resting {
                (radius - offset_z).max(0.0)
            } else {
                0.0
            };
            placements.push(ObjectPlacement {
                kind: object.kind,
                tag: object.tag.clone(),
                name: Some(object.tag.path.clone()),
                placement_flags: 0,
                position: [center[0] + offset_x, center[1] + offset_y, z],
                rotation: [core::f32::consts::PI, 0.0, 0.0],
                structure_bsps: 1,
                permutation: None,
                device: None,
            });
            y += 2.0 * radius + SLOT_MARGIN;
            depth = depth.max(2.0 * radius);
        }
        row_x += depth + GALLERY_GAP;
    }
    Galleries {
        headings: headings(&placements),
        placements,
        half_size: HALF_SIZE.max(row_x + GALLERY_EDGE),
        left_out,
    }
}

/// Each gallery's heading that `placements` show something of, over the
/// middle of its first row (its objects nearest the start).
pub fn headings(placements: &[ObjectPlacement]) -> Vec<(&'static str, [f32; 3])> {
    GALLERIES
        .iter()
        .filter_map(|(heading, kinds)| {
            let nearest = placements
                .iter()
                .filter(|placement| kinds.contains(&placement.kind))
                .map(|placement| placement.position[0])
                .reduce(f32::min)?;
            Some((*heading, [nearest, 0.0, HEADING_HEIGHT]))
        })
        .collect()
}

/// The devmap: the plane and its player start. Made over `donor`, it has
/// the donor's definitions (its objects, its globals and the player's
/// biped, its HUD) and the donor's sky; without one, it has nothing but
/// the plane and the generated sky.
pub fn level(donor: Option<&Level>) -> Level {
    let mut level = match donor {
        Some(donor) => donor.clone(),
        None => Level {
            glows: Vec::new(),
            lens_flares: Vec::new(),
            light_volumes: Vec::new(),
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
            lights: Vec::new(),
            unit_huds: Vec::new(),
            weapon_huds: Vec::new(),
            grenade_huds: Vec::new(),
            hud_digits: None,
            hud_globals: None,
        },
    };
    level.scenario = scenario();
    level.structure_bsps = vec![ground(HALF_SIZE, CELLS)];
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

/// The devmap made over every one of `levels` ([`gathered`]), with each
/// object they define in its gallery, on a plane large enough for them.
/// None if there are no levels.
pub fn gallery_level(levels: &[Level]) -> Option<(Level, Galleries)> {
    let mut level = level(Some(&gathered(levels)?));
    let laid_out = galleries(&level.objects);
    level.scenario.placements.clone_from(&laid_out.placements);
    if laid_out.half_size > HALF_SIZE {
        // (as many cells to the unit as the plane of `HALF_SIZE` has)
        #[allow(clippy::cast_possible_truncation, clippy::cast_sign_loss)]
        let cells = (f32::from(CELLS) * laid_out.half_size / HALF_SIZE).ceil() as u16;
        level.structure_bsps = vec![ground(laid_out.half_size, cells)];
    }
    Some((level, laid_out))
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn a_wedges_faces_face_outward_and_close_it() {
        let wedge = SLOPE;
        let faces = wedge.faces();
        // the slope faces up and back toward its foot, the back along its
        // rise, the sides across it (rising along -x, its left is -y)
        let planes: Vec<[f32; 4]> = faces.iter().map(|ring| face_plane(ring)).collect();
        assert!(planes[0][2] > 0.0 && planes[0][0] > 0.0, "{:?}", planes[0]);
        assert!((planes[1][0] + 1.0).abs() < 1e-6, "{:?}", planes[1]);
        assert!((planes[2][1] - 1.0).abs() < 1e-6, "{:?}", planes[2]);
        assert!((planes[3][1] + 1.0).abs() < 1e-6, "{:?}", planes[3]);
        // every point of a face lies on its plane, and the middle of the
        // wedge is behind every face
        let middle = [-30.0 - 16.0, -40.0, 2.0];
        for (ring, plane) in faces.iter().zip(&planes) {
            for point in ring {
                let along = plane[0] * point[0] + plane[1] * point[1] + plane[2] * point[2];
                assert!((along - plane[3]).abs() < 1e-3, "{point:?} {plane:?}");
            }
            let inside = plane[0] * middle[0] + plane[1] * middle[1] + plane[2] * middle[2];
            assert!(inside < plane[3], "{plane:?}");
        }
        // its height rises along it, and is nothing off it
        assert_eq!(wedge.height_at(-30.0, -40.0), 0.0);
        assert!((wedge.height_at(-42.0, -40.0) - 4.0).abs() < 1e-4);
        assert!((wedge.height_at(-54.0, -46.0) - 8.0).abs() < 1e-4);
        assert_eq!(wedge.height_at(-42.0, -48.0), 0.0);
        assert_eq!(wedge.height_at(-20.0, -40.0), 0.0);
    }

    #[test]
    fn a_degenerate_face_faces_up() {
        assert_eq!(face_plane(&[[1.0, 2.0, 3.0]; 3]), [0.0, 0.0, 1.0, 3.0]);
        assert_eq!(face_plane(&[]), [0.0, 0.0, 1.0, 0.0]);
    }

    #[test]
    fn the_ground_has_the_solids_as_rings_and_triangles() {
        let bsp = ground(HALF_SIZE, CELLS);
        let collision = &bsp.collision;
        // the plane, then four faces each
        assert_eq!(collision.surfaces.len(), 1 + 4 * SOLIDS.len());
        assert_eq!(collision.planes.len(), collision.surfaces.len());
        assert_eq!(bsp.world_bounds[2], [0.0, SLOPE.height]);
        // each surface's ring follows its edges back to where it began
        for (index, surface) in collision.surfaces.iter().enumerate() {
            let mut edge = surface.first_edge;
            let mut count = 0;
            loop {
                let at = &collision.edges[edge as usize];
                assert_eq!(at.surfaces[0], Some(index as u32));
                edge = at.edges[0];
                count += 1;
                if edge == surface.first_edge || count > 8 {
                    break;
                }
            }
            assert!((3..=4).contains(&count), "{index}: {count}");
        }
        // drawn as a second material whose triangles index its vertices
        let materials = &bsp.lightmaps[0].materials;
        assert_eq!(materials.len(), 2);
        let solids = &materials[1];
        assert_eq!(solids.vertices.len(), 14 * SOLIDS.len());
        assert_eq!(solids.lightmap_vertices.len(), solids.vertices.len());
        assert_eq!(solids.surface_count, 6 * SOLIDS.len() as u32);
        assert_eq!(
            solids.first_surface + solids.surface_count,
            bsp.surfaces.len() as u32
        );
        assert!(bsp.surfaces[solids.first_surface as usize..]
            .iter()
            .flatten()
            .all(|&index| usize::from(index) < solids.vertices.len()));
    }

    #[test]
    fn the_stations_stand_clear_of_the_galleries_on_the_plane() {
        for spot in &STATIONS {
            assert!(spot.at[0] < GALLERY_START, "{}", spot.name);
            assert!(spot.at.iter().all(|value| value.abs() < HALF_SIZE));
            assert_eq!(station(spot.name), Some(spot));
        }
        assert_eq!(station("nothing"), None);
        for solid in &SOLIDS {
            for ring in solid.faces() {
                assert!(ring.iter().all(|p| p[0] < 0.0 && p[1].abs() < HALF_SIZE));
            }
        }
        // the ramps lie on the course's sides
        assert!((RAMPS[0].foot[0] - COURSE[0][0]).abs() < 1e-6);
        assert!((RAMPS[1].foot[0] - COURSE[2][0]).abs() < 1e-6);
    }

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

    fn object(kind: ObjectKind, path: &str, radius: f32) -> ObjectDefinition {
        ObjectDefinition {
            widgets: Vec::new(),
            tag: tag(&kind.group(), path),
            kind,
            flags: 0,
            bounding_radius: radius,
            bounding_offset: [0.0, 0.0, 0.0],
            acceleration_scale: 1.0,
            render_bounding_radius: radius,
            model: None,
            animation_graph: None,
            collision_model: None,
            physics: None,
            modifier_shader: None,
            creation_effect: None,
            function_modes: Default::default(),
            functions: Vec::new(),
            attachments: Vec::new(),
            change_colors: Vec::new(),
            unit_hud: None,
            unit_melee_damage: None,
            biped: None,
            weapon: None,
            projectile: None,
            equipment: None,
            vehicle: None,
        }
    }

    #[test]
    fn definitions_are_gathered_once_each_keeping_their_own_data() {
        assert!(gathered(&[]).is_none());
        let mut first = level(None);
        first.scenario.path = "first".into();
        first.objects = vec![object(ObjectKind::Weapon, "rifle", 0.5)];
        let mut second = level(None);
        second.scenario.path = "second".into();
        second.globals.player_unit = Some(tag(b"bipd", "cyborg"));
        second.objects = vec![
            object(ObjectKind::Weapon, "rifle", 9.0),
            object(ObjectKind::Vehicle, "hog", 3.0),
        ];
        let mut third = level(None);
        third.objects = vec![
            object(ObjectKind::Weapon, "rifle", 7.0),
            object(ObjectKind::Biped, "elite", 1.0),
            object(ObjectKind::Vehicle, "hog", 4.0),
        ];
        let mut digits_bitmap = grid_bitmap();
        digits_bitmap.tag = tag(b"bitm", "digits");
        third.bitmaps.push(digits_bitmap);
        third.resource_bitmaps = vec![tag(b"bitm", "far")];
        let all = gathered(&[first, second, third]).unwrap_or_else(|| panic!("none gathered"));
        // (the base is the level with a player's biped)
        assert_eq!(all.scenario.path, "second");
        assert_eq!(all.globals.player_unit, Some(tag(b"bipd", "cyborg")));
        let radii: Vec<(&str, f32)> = all
            .objects
            .iter()
            .map(|object| (object.tag.path.as_str(), object.bounding_radius))
            .collect();
        assert_eq!(radii, vec![("rifle", 9.0), ("hog", 3.0), ("elite", 1.0)]);
        assert_eq!(
            all.bitmaps
                .iter()
                .filter(|bitmap| bitmap.tag == bitmap_tag())
                .count(),
            1
        );
        assert!(all.bitmap(&tag(b"bitm", "digits")).is_some());
        assert_eq!(all.resource_bitmaps, vec![tag(b"bitm", "far")]);
    }

    #[test]
    fn without_a_player_the_first_level_is_the_base() {
        let mut first = level(None);
        first.scenario.path = "first".into();
        let gathered = gathered(&[first, level(None)]).unwrap_or_else(|| panic!("none"));
        assert_eq!(gathered.scenario.path, "first");
    }

    /// Whether the slots of `galleries` (their bounding circles, centred
    /// where the layout put them) overlap, or leave the plane.
    fn check_layout(objects: &[ObjectDefinition], laid_out: &Galleries) {
        let circle = |placement: &ObjectPlacement| {
            let object = objects
                .iter()
                .find(|object| object.tag == placement.tag)
                .unwrap_or_else(|| panic!("{:?} is not defined", placement.tag));
            let radius = slot_radius(object);
            // (the layout's offset, taken back)
            let [x, y, _] = placement.position;
            (
                [x - object.bounding_offset[0], y - object.bounding_offset[1]],
                radius,
            )
        };
        let circles: Vec<_> = laid_out.placements.iter().map(circle).collect();
        let edge = laid_out.half_size - GALLERY_EDGE + 1e-3;
        for (at, ([x, y], radius)) in circles.iter().enumerate() {
            assert!(
                x - radius >= GALLERY_START - 1e-3,
                "{at} is behind the start"
            );
            assert!(
                x + radius <= edge && y + radius <= edge && y - radius >= -edge,
                "{at} is off the plane"
            );
            for ([other_x, other_y], other_radius) in &circles[at + 1..] {
                let (dx, dy) = (x - other_x, y - other_y);
                let apart = radius + other_radius + SLOT_MARGIN - 1e-3;
                assert!(dx * dx + dy * dy >= apart * apart, "{at} overlaps");
            }
        }
    }

    #[test]
    fn the_galleries_hold_each_object_without_overlap() {
        let mut objects = Vec::new();
        for at in 0..60 {
            let radius = 0.1 + (at % 7) as f32 * 0.6;
            objects.push(object(
                ObjectKind::Weapon,
                &format!("weapon {at:02}"),
                radius,
            ));
            objects.push(object(
                ObjectKind::Scenery,
                &format!("tree {at:02}"),
                radius * 2.0,
            ));
        }
        let mut offset = object(ObjectKind::Vehicle, "offset", 2.0);
        offset.bounding_offset = [0.5, -0.25, 1.0];
        objects.push(offset);
        objects.push(object(ObjectKind::Machine, "door", 1.5));
        objects.push(object(ObjectKind::Placeholder, "unseen", 1.0));
        objects.push(object(ObjectKind::Scenery, "too wide", 100.0));
        objects.push(object(ObjectKind::Projectile, "odd", f32::NAN));
        let laid_out = galleries(&objects);
        assert_eq!(laid_out.placements.len(), 60 * 2 + 3);
        assert_eq!(laid_out.left_out, vec![tag(b"scen", "too wide")]);
        assert!(laid_out.half_size > HALF_SIZE, "{}", laid_out.half_size);
        check_layout(&objects, &laid_out);
        // the galleries in their order, each heading where its first row is
        let names: Vec<&str> = laid_out.headings.iter().map(|(name, _)| *name).collect();
        assert_eq!(
            names,
            vec!["weapons", "vehicles", "devices", "scenery", "projectiles"]
        );
        let first_weapon = &laid_out.placements[0];
        assert_eq!(first_weapon.name.as_deref(), Some("weapon 00"));
        assert_eq!(
            laid_out.headings[0].1,
            [first_weapon.position[0], 0.0, HEADING_HEIGHT]
        );
        assert_eq!(headings(&[]), Vec::new());
        let vehicle = laid_out
            .placements
            .iter()
            .find(|placement| placement.kind == ObjectKind::Vehicle)
            .unwrap_or_else(|| panic!("no vehicle"));
        // (resting on its sphere, lifted by its offset's height)
        assert!((vehicle.position[2] - 1.0).abs() < 1e-6);
        assert!((vehicle.rotation[0] - core::f32::consts::PI).abs() < 1e-6);
        let door = laid_out
            .placements
            .iter()
            .find(|placement| placement.kind == ObjectKind::Machine)
            .unwrap_or_else(|| panic!("no door"));
        assert_eq!(door.position[2], 0.0);
        assert!(laid_out.headings[1].1[0] > laid_out.headings[0].1[0]);
    }

    #[test]
    fn a_few_objects_fit_the_smallest_plane() {
        let objects = vec![object(ObjectKind::Biped, "elite", 0.8)];
        let laid_out = galleries(&objects);
        assert_eq!(laid_out.half_size, HALF_SIZE);
        assert_eq!(laid_out.headings.len(), 1);
        check_layout(&objects, &laid_out);
    }

    #[test]
    fn the_gallery_level_places_its_objects_on_a_plane_that_holds_them() {
        assert!(gallery_level(&[]).is_none());
        let mut small = level(None);
        small.objects = vec![object(ObjectKind::Weapon, "rifle", 0.5)];
        let (made, laid_out) = gallery_level(&[small]).unwrap_or_else(|| panic!("none"));
        assert_eq!(made.scenario.placements, laid_out.placements);
        assert_eq!(
            made.structure_bsps[0].world_bounds[0],
            [-HALF_SIZE, HALF_SIZE]
        );

        let mut large = level(None);
        large.objects = (0..40)
            .map(|at| object(ObjectKind::Scenery, &format!("rock {at}"), 10.0))
            .collect();
        let (made, laid_out) = gallery_level(&[large]).unwrap_or_else(|| panic!("none"));
        assert_eq!(made.scenario.placements.len(), 40);
        let half = laid_out.half_size;
        assert!(half > HALF_SIZE);
        assert_eq!(made.structure_bsps[0].world_bounds[0], [-half, half]);
        assert!(
            made.structure_bsps[0].lightmaps[0].materials[0]
                .vertices
                .len()
                > 33 * 33
        );
    }
}
