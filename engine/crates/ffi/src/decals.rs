//! Decals laid on the level: where the level's runtime decals meet its
//! surfaces, and each one's picture, size, colour and turn, drawn in the
//! original's random order (`decals.c`, `structure_runtime_decals.c`).

use opence_content::{Bitmap, Decal, Level, Light, StructureBsp};
use opence_sim::random::RandomSeed;

/// A decal laid on a surface: a box the renderer projects its picture
/// through onto what is drawn inside it.
#[derive(Clone, Debug, Default, PartialEq, uniffi::Record)]
pub struct RenderDecal {
    /// Its centre, on the surface.
    pub x: f32,
    /// (y)
    pub y: f32,
    /// (z)
    pub z: f32,
    /// The surface's normal: the box's up.
    pub up_x: f32,
    /// (y)
    pub up_y: f32,
    /// (z)
    pub up_z: f32,
    /// The picture's across (its u), along the surface.
    pub forward_x: f32,
    /// (y)
    pub forward_y: f32,
    /// (z)
    pub forward_z: f32,
    /// The picture's down (its v), along the surface.
    pub left_x: f32,
    /// (y)
    pub left_y: f32,
    /// (z)
    pub left_z: f32,
    /// How far it reaches from its centre along forward, least and
    /// greatest (the least negative).
    pub forward_least: f32,
    /// (greatest)
    pub forward_greatest: f32,
    /// ... along left.
    pub left_least: f32,
    /// (greatest)
    pub left_greatest: f32,
    /// How far above and below the surface it reaches.
    pub depth: f32,
    /// Its picture's texture coordinates at the least forward and left.
    pub u0: f32,
    /// (v)
    pub v0: f32,
    /// ... at the greatest.
    pub u1: f32,
    /// (v)
    pub v1: f32,
    /// Its picture, an index into `RenderLevel::textures` (`u32::MAX`:
    /// none, a plain colour).
    pub texture: u32,
    /// Its colour, linear.
    pub red: f32,
    /// (green)
    pub green: f32,
    /// (blue)
    pub blue: f32,
    /// Its brightness (its colour's alpha).
    pub alpha: f32,
    /// How it is blended (`Decal::framebuffer_blend_function`).
    pub blend: u32,
    /// Its layer (`Decal::PRIMARY` and the rest).
    pub layer: u32,
}

/// How far from a runtime decal's position its ray reaches.
const RAY_LENGTH: f32 = 1.0;
/// A decal no deeper than this, above and below its surface.
const LEAST_DEPTH: f32 = 0.05;
/// A dot product this near 0 is a ray along a surface.
const EPSILON: f32 = 1e-4;

type Vector = [f32; 3];

fn add(a: Vector, b: Vector) -> Vector {
    [a[0] + b[0], a[1] + b[1], a[2] + b[2]]
}

fn sub(a: Vector, b: Vector) -> Vector {
    [a[0] - b[0], a[1] - b[1], a[2] - b[2]]
}

fn scale(a: Vector, s: f32) -> Vector {
    [a[0] * s, a[1] * s, a[2] * s]
}

fn dot(a: Vector, b: Vector) -> f32 {
    a[0] * b[0] + a[1] * b[1] + a[2] * b[2]
}

fn cross(a: Vector, b: Vector) -> Vector {
    [
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    ]
}

fn normalize(a: Vector) -> Vector {
    let length = libm::sqrtf(dot(a, a));
    if length > 0.0 {
        scale(a, 1.0 / length)
    } else {
        a
    }
}

/// A vector at right angles to `v` (`perpendicular3d`).
fn perpendicular(v: Vector) -> Vector {
    let [i, j, k] = v.map(f32::abs);
    if i <= j && i <= k {
        [0.0, v[2], -v[1]]
    } else if j <= k {
        [-v[2], 0.0, v[0]]
    } else {
        [v[1], -v[0], 0.0]
    }
}

/// From `lower` to `upper` by the next random number (`real_seed_random_range`).
fn range(seed: &mut RandomSeed, [lower, upper]: [f32; 2]) -> f32 {
    lower + (upper - lower) * seed.next_real()
}

/// A runtime decal's ray: its yaw is in 127ths of a half turn, its pitch
/// in 254ths.
pub(crate) fn runtime_direction(yaw: i8, pitch: i8) -> Vector {
    let yaw = f32::from(yaw) * core::f32::consts::PI / 127.0;
    let pitch = f32::from(pitch) * core::f32::consts::PI / 254.0;
    let (cos_pitch, sin_pitch) = (libm::cosf(pitch), libm::sinf(pitch));
    [
        libm::cosf(yaw) * cos_pitch,
        libm::sinf(yaw) * cos_pitch,
        sin_pitch,
    ]
}

/// The seed a decal at `position` draws from (`decal_new`).
pub(crate) fn seed(position: Vector) -> RandomSeed {
    RandomSeed(position[2].to_bits() ^ position[1].to_bits() ^ position[0].to_bits() ^ 0xDEAD_C0DE)
}

/// Where a decal is laid, and how it is turned on its surface.
#[derive(Clone, Copy, Debug, PartialEq)]
pub(crate) struct Basis {
    pub position: Vector,
    pub forward: Vector,
    pub left: Vector,
    pub up: Vector,
}

/// What a decal shares with the one it hands its geometry on to.
#[derive(Clone, Copy, Debug)]
struct Geometry {
    basis: Basis,
    sequence: usize,
    radius: f32,
}

/// The decal `decal` and those it chains to, laid where a ray along
/// `velocity` met a surface at `position` with `normal`, each drawing its
/// numbers from `seed` (`decal_new_from_collision`).
pub(crate) fn lay(
    level: &Level,
    decal: &Decal,
    position: Vector,
    normal: Vector,
    velocity: Vector,
    seed: &mut RandomSeed,
    texture: &mut dyn FnMut(&Bitmap, u16) -> Option<u32>,
) -> Vec<RenderDecal> {
    let mut laid = Vec::new();
    let mut inherited: Option<Geometry> = None;
    let mut current = Some(decal);
    // (a chain that comes back on itself is cut where it does)
    let mut seen = Vec::new();
    while let Some(decal) = current {
        if seen.contains(&&decal.tag) {
            break;
        }
        seen.push(&decal.tag);
        let bitmap = decal.map.as_ref().and_then(|map| level.bitmap(map));
        let geometry = match inherited {
            Some(geometry) => geometry,
            None => {
                let basis = turn(decal, position, normal, velocity, seed);
                let sequences = bitmap.map_or(0, |bitmap| bitmap.sequences.len());
                let sequence = seed.next_range(0, sequences.min(i16::MAX as usize) as i16);
                Geometry {
                    basis,
                    sequence: usize::try_from(sequence).unwrap_or(0),
                    radius: range(seed, decal.radius),
                }
            }
        };
        let (extent, uv, image) = extent(decal, bitmap, geometry.sequence, geometry.radius);
        let _lifetime = range(seed, decal.lifetime);
        let _decay = range(seed, decal.decay_time);
        let intensity = range(seed, decal.intensity);
        let t = seed.next_real();
        let color = crate::lights::interpolate(
            u32::from(decal.flags >> 1) & (Light::HSV | Light::MORE_COLORS),
            decal.color[0],
            decal.color[1],
            t,
        );
        let texture = match (bitmap, image) {
            (Some(bitmap), Some(image)) => texture(bitmap, image).unwrap_or(u32::MAX),
            _ => u32::MAX,
        };
        let Basis {
            position: centre,
            forward,
            left,
            up,
        } = geometry.basis;
        let reach = extent
            .iter()
            .fold(0.0f32, |reach, value| reach.max(value.abs()));
        laid.push(RenderDecal {
            x: centre[0],
            y: centre[1],
            z: centre[2],
            up_x: up[0],
            up_y: up[1],
            up_z: up[2],
            forward_x: forward[0],
            forward_y: forward[1],
            forward_z: forward[2],
            left_x: left[0],
            left_y: left[1],
            left_z: left[2],
            forward_least: extent[0],
            forward_greatest: extent[1],
            left_least: extent[2],
            left_greatest: extent[3],
            depth: (reach * 0.5).max(LEAST_DEPTH),
            u0: uv[0],
            v0: uv[2],
            u1: uv[1],
            v1: uv[3],
            texture,
            red: color[0],
            green: color[1],
            blue: color[2],
            alpha: intensity,
            blend: u32::try_from(decal.framebuffer_blend_function).unwrap_or(0),
            layer: u32::try_from(decal.layer).unwrap_or(0),
        });
        inherited = (decal.flags & Decal::GEOMETRY_INHERITED != 0).then_some(geometry);
        current = decal.next.as_ref().and_then(|next| level.decal(next));
    }
    laid
}

/// How a decal is turned on its surface: at random, or facing the way it
/// was thrown.
fn turn(
    decal: &Decal,
    position: Vector,
    normal: Vector,
    velocity: Vector,
    seed: &mut RandomSeed,
) -> Basis {
    let (cosine, sine, tangent, bitangent) =
        if decal.flags & Decal::NO_RANDOM_ROTATION != 0 && dot(normal, velocity) < -EPSILON {
            let tangent = cross(normal, velocity);
            (-1.0, 0.0, tangent, cross(normal, tangent))
        } else {
            let angle = range(seed, [0.0, 2.0 * core::f32::consts::PI]);
            let tangent = perpendicular(normal);
            (
                libm::cosf(angle),
                libm::sinf(angle),
                tangent,
                cross(normal, tangent),
            )
        };
    let (tangent, bitangent) = (normalize(tangent), normalize(bitangent));
    Basis {
        position,
        forward: sub(scale(bitangent, cosine), scale(tangent, sine)),
        left: add(scale(tangent, cosine), scale(bitangent, sine)),
        up: normal,
    }
}

/// A decal's reach from its centre (forward least and greatest, left
/// least and greatest), its texture coordinates (u least and greatest, v
/// least and greatest) and the bitmap image it is drawn from.
fn extent(
    decal: &Decal,
    bitmap: Option<&Bitmap>,
    sequence: usize,
    radius: f32,
) -> ([f32; 4], [f32; 4], Option<u16>) {
    let sequence = bitmap.and_then(|bitmap| bitmap.sequences.get(sequence));
    let preserve = decal.flags & Decal::PRESERVE_ASPECT != 0;
    if let (Some(bitmap), Some(sprite)) = (bitmap, sequence.and_then(|s| s.sprites.first())) {
        if let Some(image) = bitmap.images.get(usize::from(sprite.image)) {
            let (width, height) = (f32::from(image.width), f32::from(image.height));
            let [left, right, top, bottom] = sprite.bounds;
            let [x, y] = sprite.registration_point;
            let extent = if decal.maximum_sprite_extent > 0.0 {
                radius / decal.maximum_sprite_extent
            } else {
                0.0
            };
            let aspect = if preserve && bottom > top && width > 0.0 {
                ((right - left) / (bottom - top)) * (height / width)
            } else {
                1.0
            };
            let (across, down) = (width * extent, height * extent * aspect);
            return (
                [
                    -x * across,
                    (right - x - left) * across,
                    -y * down,
                    (bottom - y - top) * down,
                ],
                [left, right, top, bottom],
                Some(sprite.image),
            );
        }
    }
    let image = sequence
        .and_then(|sequence| sequence.first_image)
        .or(bitmap.filter(|bitmap| !bitmap.images.is_empty()).map(|_| 0));
    let aspect = match (preserve, bitmap, image) {
        (true, Some(bitmap), Some(image)) => bitmap
            .images
            .get(usize::from(image))
            .filter(|image| image.width > 0)
            .map_or(1.0, |image| {
                f32::from(image.height) / f32::from(image.width)
            }),
        _ => 1.0,
    };
    (
        [-radius, radius, -aspect * radius, aspect * radius],
        [0.0, 1.0, 0.0, 1.0],
        image,
    )
}

/// A triangle of the BSP, with its front's normal.
struct Triangle {
    corners: [Vector; 3],
    normal: Vector,
    least: Vector,
    greatest: Vector,
}

/// The BSP's drawn triangles, each facing the way its vertices' normals
/// do, sorted by their least x.
fn triangles(structure: &StructureBsp) -> Vec<Triangle> {
    let mut triangles = Vec::new();
    for material in structure.lightmaps.iter().flat_map(|page| &page.materials) {
        let first = material.first_surface as usize;
        let count = material.surface_count as usize;
        for surface in structure.surfaces.iter().skip(first).take(count) {
            let Some(vertices) = surface
                .iter()
                .map(|&index| material.vertices.get(usize::from(index)))
                .collect::<Option<Vec<_>>>()
            else {
                continue;
            };
            let corners = [0, 1, 2].map(|i| vertices[i].position);
            let mut normal = normalize(cross(
                sub(corners[1], corners[0]),
                sub(corners[2], corners[0]),
            ));
            let shaded = add(
                add(vertices[0].normal, vertices[1].normal),
                vertices[2].normal,
            );
            if dot(normal, shaded) < 0.0 {
                normal = scale(normal, -1.0);
            }
            let least = [0, 1, 2].map(|axis| corners.iter().fold(f32::MAX, |m, c| m.min(c[axis])));
            let greatest =
                [0, 1, 2].map(|axis| corners.iter().fold(f32::MIN, |m, c| m.max(c[axis])));
            triangles.push(Triangle {
                corners,
                normal,
                least,
                greatest,
            });
        }
    }
    triangles.sort_by(|a, b| a.least[0].total_cmp(&b.least[0]));
    triangles
}

/// How far along `direction` from `origin` the ray meets the front of
/// `triangle`, if it does.
fn hit(triangle: &Triangle, origin: Vector, direction: Vector) -> Option<f32> {
    if dot(triangle.normal, direction) >= -EPSILON {
        return None;
    }
    let [a, b, c] = triangle.corners;
    let (e1, e2) = (sub(b, a), sub(c, a));
    let p = cross(direction, e2);
    let determinant = dot(e1, p);
    if determinant.abs() < 1e-12 {
        return None;
    }
    let inverse = 1.0 / determinant;
    let s = sub(origin, a);
    let u = dot(s, p) * inverse;
    if !(0.0..=1.0).contains(&u) {
        return None;
    }
    let q = cross(s, e1);
    let v = dot(direction, q) * inverse;
    if v < 0.0 || u + v > 1.0 {
        return None;
    }
    Some(dot(e2, q) * inverse)
}

/// Where each of `rays` (an origin and a unit direction, `RAY_LENGTH`
/// long) first meets a front of the BSP, and that surface's normal.
fn cast(structure: &StructureBsp, rays: &[(Vector, Vector)]) -> Vec<Option<(Vector, Vector)>> {
    let triangles = triangles(structure);
    rays.iter()
        .map(|&(origin, direction)| {
            let end = add(origin, scale(direction, RAY_LENGTH));
            let least = [0, 1, 2].map(|axis| origin[axis].min(end[axis]));
            let greatest = [0, 1, 2].map(|axis| origin[axis].max(end[axis]));
            // (the triangles are sorted by their least x: none past the
            // ray's greatest x can meet it)
            let past = triangles.partition_point(|triangle| triangle.least[0] <= greatest[0]);
            triangles[..past]
                .iter()
                .filter(|triangle| {
                    (0..3).all(|axis| {
                        triangle.least[axis] <= greatest[axis]
                            && triangle.greatest[axis] >= least[axis]
                    })
                })
                .filter_map(|triangle| {
                    hit(triangle, origin, direction)
                        .filter(|t| (0.0..=RAY_LENGTH).contains(t))
                        .map(|t| (t, triangle.normal))
                })
                .min_by(|a, b| a.0.total_cmp(&b.0))
                .map(|(t, normal)| (add(origin, scale(direction, t)), normal))
        })
        .collect()
}

/// The decals the level's BSP `structure` lays when it is loaded, layer by
/// layer and in its order within each, their pictures made textures by
/// `texture`.
pub(crate) fn level_decals(
    level: &Level,
    structure: &StructureBsp,
    texture: &mut dyn FnMut(&Bitmap, u16) -> Option<u32>,
) -> Vec<RenderDecal> {
    let rays: Vec<(Vector, Vector)> = structure
        .decals
        .iter()
        .map(|decal| (decal.position, runtime_direction(decal.yaw, decal.pitch)))
        .collect();
    let hits = cast(structure, &rays);
    let mut laid = Vec::new();
    for ((runtime, &(origin, direction)), hit) in structure.decals.iter().zip(&rays).zip(hits) {
        let Some(decal) = level
            .scenario
            .decal_palette
            .get(usize::from(runtime.palette_index))
            .and_then(Option::as_ref)
            .and_then(|tag| level.decal(tag))
        else {
            continue;
        };
        if decal.flags & Decal::WATER_EFFECT != 0 {
            continue;
        }
        // (a decal whose ray meets nothing is laid where it starts,
        // facing back along it)
        let (position, normal) = hit.unwrap_or((origin, scale(direction, -1.0)));
        let mut seed = seed(runtime.position);
        laid.extend(lay(
            level, decal, position, normal, direction, &mut seed, texture,
        ));
    }
    laid.sort_by_key(|decal| decal.layer);
    laid
}

#[cfg(test)]
mod tests {
    use super::*;
    use opence_content::{
        BitmapImage, EnvironmentVertex, LevelDecal, Lightmap, Material, Sequence, Sprite, TagRef,
        Texture, TextureFormat,
    };

    fn near(a: f32, b: f32) -> bool {
        (a - b).abs() < 1e-4
    }

    fn tag(group: &[u8; 4], path: &str) -> TagRef {
        TagRef {
            group: opence_content::Group(*group),
            path: path.into(),
        }
    }

    fn decal(path: &str) -> Decal {
        Decal {
            tag: tag(b"deca", path),
            flags: 0,
            kind: 0,
            layer: Decal::PRIMARY,
            next: None,
            radius: [0.5, 0.5],
            intensity: [1.0, 1.0],
            color: [[1.0, 0.5, 0.25], [1.0, 0.5, 0.25]],
            animation_loop_frame: 0,
            animation_speed: 1,
            lifetime: [0.0, 0.0],
            decay_time: [0.0, 0.0],
            framebuffer_blend_function: 1,
            map: None,
            maximum_sprite_extent: 0.0,
        }
    }

    fn vertex(position: Vector) -> EnvironmentVertex {
        EnvironmentVertex {
            position,
            normal: [0.0, 0.0, 1.0],
            binormal: [0.0, 1.0, 0.0],
            tangent: [1.0, 0.0, 0.0],
            texcoord: [0.0, 0.0],
        }
    }

    /// A floor at z 0 from -10 to 10, and the decals laid on it.
    fn floor(decals: Vec<LevelDecal>) -> StructureBsp {
        let mut bsp = crate::render_level::tests::level().structure_bsps.remove(0);
        bsp.surfaces = vec![[0, 1, 2], [0, 2, 3]];
        bsp.lightmaps = vec![Lightmap {
            bitmap_index: None,
            materials: vec![Material {
                shader: None,
                first_surface: 0,
                surface_count: 2,
                vertices: vec![
                    vertex([-10.0, -10.0, 0.0]),
                    vertex([10.0, -10.0, 0.0]),
                    vertex([10.0, 10.0, 0.0]),
                    vertex([-10.0, 10.0, 0.0]),
                ],
                lightmap_vertices: Vec::new(),
            }],
        }];
        bsp.decals = decals;
        bsp
    }

    fn level_with(decals: Vec<Decal>, runtime: Vec<LevelDecal>) -> Level {
        let mut level = crate::render_level::tests::level();
        level.scenario.decal_palette = decals.iter().map(|d| Some(d.tag.clone())).collect();
        level.decals = decals;
        level.structure_bsps = vec![floor(runtime)];
        level
    }

    fn down(x: f32, y: f32, palette_index: u8) -> LevelDecal {
        LevelDecal {
            position: [x, y, 0.5],
            palette_index,
            yaw: 0,
            pitch: -127,
        }
    }

    fn laid(level: &Level) -> Vec<RenderDecal> {
        level_decals(level, &level.structure_bsps[0], &mut |_, _| None)
    }

    #[test]
    fn the_ray_points_by_the_yaw_and_pitch() {
        let [x, y, z] = runtime_direction(0, -127);
        assert!(near(x, 0.0) && near(y, 0.0) && near(z, -1.0));
        let [x, y, z] = runtime_direction(127 / 2, 0);
        assert!(near(x, libm::cosf(core::f32::consts::PI * 63.0 / 127.0)));
        assert!(y > 0.99 && near(z, 0.0));
    }

    #[test]
    fn a_runtime_decal_is_laid_where_its_ray_meets_the_floor_facing_up() {
        let level = level_with(vec![decal("scorch")], vec![down(1.0, 2.0, 0)]);
        let laid = laid(&level);
        assert_eq!(laid.len(), 1);
        let decal = &laid[0];
        assert!(near(decal.x, 1.0) && near(decal.y, 2.0) && near(decal.z, 0.0));
        assert!(near(decal.up_z, 1.0));
        // (turned at random about the normal, flat on the floor)
        assert!(near(decal.forward_z, 0.0) && near(decal.left_z, 0.0));
        let forward = [decal.forward_x, decal.forward_y, decal.forward_z];
        let left = [decal.left_x, decal.left_y, decal.left_z];
        assert!(near(dot(forward, forward), 1.0) && near(dot(forward, left), 0.0));
        assert_eq!(
            [
                decal.forward_least,
                decal.forward_greatest,
                decal.left_least,
                decal.left_greatest
            ],
            [-0.5, 0.5, -0.5, 0.5]
        );
        assert_eq!(
            (decal.u0, decal.v0, decal.u1, decal.v1),
            (0.0, 0.0, 1.0, 1.0)
        );
        assert_eq!(
            (decal.red, decal.green, decal.blue, decal.alpha),
            (1.0, 0.5, 0.25, 1.0)
        );
        assert_eq!((decal.blend, decal.layer, decal.texture), (1, 0, u32::MAX));
        // (the same position draws the same turn)
        assert_eq!(laid, self::laid(&level));
    }

    #[test]
    fn a_ray_that_misses_or_meets_the_back_lays_where_it_starts() {
        // (looking up from under the floor: it meets the floor's back)
        let mut up = down(0.0, 0.0, 0);
        up.position[2] = -0.5;
        up.pitch = 127;
        let far = down(30.0, 0.0, 0);
        let level = level_with(vec![decal("scorch")], vec![up, far]);
        let laid = laid(&level);
        assert_eq!(laid.len(), 2);
        assert!(near(laid[0].z, -0.5) && near(laid[0].up_z, -1.0));
        assert!(near(laid[1].x, 30.0) && near(laid[1].z, 0.5) && near(laid[1].up_z, 1.0));
    }

    #[test]
    fn chained_decals_follow_and_layers_are_drawn_in_order() {
        let mut light = decal("light");
        light.layer = Decal::LIGHT;
        light.radius = [2.0, 2.0];
        let mut inherit = decal("inherit");
        inherit.layer = Decal::SECONDARY;
        inherit.flags = Decal::GEOMETRY_INHERITED;
        inherit.radius = [0.25, 0.75];
        inherit.next = Some(light.tag.clone());
        let mut first = decal("first");
        first.next = Some(inherit.tag.clone());
        // (and a chain back to the start stops)
        light.next = Some(first.tag.clone());
        let mut water = decal("water");
        water.flags = Decal::WATER_EFFECT;
        let level = level_with(
            vec![first, inherit, light, water],
            vec![down(0.0, 0.0, 0), down(0.0, 0.0, 3), down(0.0, 0.0, 9)],
        );
        let laid = laid(&level);
        assert_eq!(
            laid.iter().map(|d| d.layer).collect::<Vec<_>>(),
            [0, 1, 2],
            "the chain was not followed, or not in its layers"
        );
        // (the decal after one that hands on its geometry is laid as it is)
        assert_eq!(laid[2].forward_greatest, laid[1].forward_greatest);
        assert_eq!(
            [laid[2].forward_x, laid[2].forward_y],
            [laid[1].forward_x, laid[1].forward_y]
        );
        assert!((0.25..=0.75).contains(&laid[1].forward_greatest));
        assert_ne!(laid[1].forward_greatest, 0.5);
    }

    #[test]
    fn a_decal_that_faces_the_way_it_was_thrown_is_not_turned() {
        let mut aimed = decal("aimed");
        aimed.flags = Decal::NO_RANDOM_ROTATION;
        let level = level_with(vec![aimed.clone()], Vec::new());
        let mut seed = RandomSeed(1);
        let before = seed;
        let velocity = normalize([1.0, 0.0, -1.0]);
        let laid = lay(
            &level,
            &aimed,
            [0.0; 3],
            [0.0, 0.0, 1.0],
            velocity,
            &mut seed,
            &mut |_, _| None,
        );
        // (forward is -bitangent: n × (n × v), against the throw's
        // horizontal)
        assert!(near(laid[0].forward_x, 1.0) && near(laid[0].forward_y, 0.0));
        let mut random = before;
        let turned = lay(
            &level,
            &decal("plain"),
            [0.0; 3],
            [0.0, 0.0, 1.0],
            velocity,
            &mut random,
            &mut |_, _| None,
        );
        assert_ne!(seed, random, "an aimed decal drew its turn");
        assert_eq!(turned.len(), 1);
    }

    fn image(width: u16, height: u16) -> BitmapImage {
        BitmapImage {
            width,
            height,
            depth: 1,
            kind: 0,
            format: 0,
            flags: 0,
            registration_point: [0, 0],
            mipmap_count: 0,
            pixels: opence_content::Pixels::Texture(Texture {
                format: TextureFormat::Rgba8,
                width,
                height,
                depth: 1,
                faces: 1,
                levels: vec![vec![0; usize::from(width) * usize::from(height) * 4]],
            }),
        }
    }

    #[test]
    fn a_sprite_decal_is_sized_by_its_sprite_and_its_picture_textured() {
        let mut sheet = crate::render_level::tests::bitmap("sheet", 0, 1);
        sheet.images = vec![image(64, 32), image(128, 128)];
        sheet.sequences = vec![Sequence {
            name: String::new(),
            first_image: Some(0),
            image_count: 1,
            sprites: vec![Sprite {
                image: 1,
                bounds: [0.25, 0.75, 0.0, 0.5],
                registration_point: [0.25, 0.25],
            }],
        }];
        let mut splat = decal("splat");
        splat.map = Some(sheet.tag.clone());
        splat.maximum_sprite_extent = 64.0;
        let mut level = level_with(vec![splat], vec![down(0.0, 0.0, 0)]);
        level.bitmaps.push(sheet);
        let mut asked = Vec::new();
        let laid = level_decals(&level, &level.structure_bsps[0], &mut |bitmap, image| {
            asked.push((bitmap.tag.path.clone(), image));
            Some(7)
        });
        assert_eq!(asked, [("sheet".to_string(), 1)]);
        let decal = &laid[0];
        assert_eq!(decal.texture, 7);
        // (a 64 texel sprite is the radius, half a unit: a texel is 1/128)
        let across = 128.0 * 0.5 / 64.0;
        assert!(near(decal.forward_least, -0.25 * across));
        assert!(near(decal.forward_greatest, 0.25 * across));
        assert!(near(decal.left_least, -0.25 * across));
        assert!(near(decal.left_greatest, 0.25 * across));
        assert_eq!(
            (decal.u0, decal.u1, decal.v0, decal.v1),
            (0.25, 0.75, 0.0, 0.5)
        );
    }

    #[test]
    fn a_plain_decal_keeps_its_pictures_shape_if_it_says_to() {
        let mut picture = crate::render_level::tests::bitmap("picture", 0, 1);
        picture.images = vec![image(64, 32)];
        let mut sign = decal("sign");
        sign.flags = Decal::PRESERVE_ASPECT;
        sign.map = Some(picture.tag.clone());
        let mut level = level_with(vec![sign], vec![down(0.0, 0.0, 0)]);
        level.bitmaps.push(picture);
        let laid = level_decals(&level, &level.structure_bsps[0], &mut |_, image| {
            Some(u32::from(image))
        });
        assert_eq!(laid[0].texture, 0);
        assert_eq!([laid[0].left_least, laid[0].left_greatest], [-0.25, 0.25]);
        assert_eq!(laid[0].depth, 0.25);
    }

    #[test]
    fn a_perpendicular_is_at_right_angles() {
        for v in [
            [1.0, 0.0, 0.0],
            [0.0, 1.0, 0.0],
            [0.0, 0.0, 1.0],
            [0.3, -0.8, 0.5],
        ] {
            let p = perpendicular(v);
            assert!(near(dot(p, v), 0.0) && dot(p, p) > 0.1, "{v:?}");
        }
        assert_eq!(normalize([0.0; 3]), [0.0; 3]);
    }
}
