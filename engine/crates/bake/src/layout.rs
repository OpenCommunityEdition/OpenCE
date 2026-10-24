//! Scenes whose light is known, for checking a path tracer: the CPU's in
//! this crate's tests, the GPU's in the app's.
//!
//! Each is traced at the origin, on a surface facing up (+Z). A
//! triangle's front is against its winding's cross product, as the stock
//! BSPs' are, and a glowing surface glows from its front only.

use crate::scene::{BakeMaterial, DirectionalLight, Scene, SceneTriangle};

/// A layout's name and what a tracer must find at the origin.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Expected {
    /// The irradiance, each channel.
    pub irradiance: f32,
    /// The open share of the hemisphere.
    pub occlusion: f32,
}

/// The layouts by name.
pub const NAMES: [&str; 9] = [
    "furnace", "open", "wall", "sun", "roof", "screen", "lamp", "bulb", "back",
];

/// The occlusion distance the layouts are traced with.
pub const OCCLUSION_DISTANCE: f32 = 10.0;

/// The layout `name` and what a tracer finds at its origin; None if there
/// is no such layout.
pub fn layout(name: &str) -> Option<(Scene, Expected)> {
    let pi = core::f32::consts::PI;
    Some(match name {
        // a closed box whose every wall gives 1 and reflects a half: its
        // light is 1 / (1 - a) everywhere, so pi / (1 - a) on any surface
        "furnace" => (
            scene(cube(), vec![material(0.5, 1.0, 1.0)], 100.0),
            Expected {
                irradiance: 2.0 * pi,
                occlusion: 0.0,
            },
        ),
        // nothing under a sky of 2: pi times it
        "open" => (
            scene(Vec::new(), Vec::new(), 2.0),
            Expected {
                irradiance: 2.0 * pi,
                occlusion: 1.0,
            },
        ),
        // a black wall, far wider and higher than the occlusion distance,
        // just beside the origin: half the hemisphere, by cosine
        "wall" => (
            scene(
                quad([0.01, -1e3, -1.0], [0.0, 2e3, 0.0], [0.0, 0.0, 1e3]),
                vec![material(0.0, 0.0, 1.0)],
                1.0,
            ),
            Expected {
                irradiance: 0.5 * pi,
                occlusion: 0.5,
            },
        ),
        // the sun at 45 degrees, giving 3: 3 cos 45
        "sun" => {
            let mut open = scene(Vec::new(), Vec::new(), 0.0);
            open.lights.push(sun());
            (
                open,
                Expected {
                    irradiance: 3.0 * core::f32::consts::FRAC_1_SQRT_2,
                    occlusion: 1.0,
                },
            )
        }
        // the sun over a black roof: dark
        "roof" => {
            let mut roofed = scene(
                quad([-1e3, -1e3, 1.0], [2e3, 0.0, 0.0], [0.0, 2e3, 0.0]),
                vec![material(0.0, 0.0, 1.0)],
                0.0,
            );
            roofed.lights.push(sun());
            (
                roofed,
                Expected {
                    irradiance: 0.0,
                    occlusion: 0.0,
                },
            )
        }
        // a black alpha-tested roof a quarter solid, under a sky of 1
        "screen" => (
            scene(
                quad([-1e3, -1e3, 1.0], [2e3, 0.0, 0.0], [0.0, 2e3, 0.0]),
                vec![material(0.0, 0.0, 0.25)],
                1.0,
            ),
            Expected {
                irradiance: 0.75 * pi,
                occlusion: 0.75,
            },
        ),
        // a transparent roof that glows 1, under a sky of 1: the rays
        // pass it, taking its light, so pi times 2, and nothing near
        "lamp" => {
            let mut lamp = material(0.0, 1.0, 0.0);
            lamp.opaque = false;
            (
                scene(
                    quad([-1e3, -1e3, 1.0], [2e3, 0.0, 0.0], [0.0, 2e3, 0.0]),
                    vec![lamp],
                    1.0,
                ),
                Expected {
                    irradiance: 2.0 * pi,
                    occlusion: 1.0,
                },
            )
        }
        // a black square a hundredth across, a unit above, glowing ten
        // thousand: its light is its radiance times its solid angle,
        // 1e4 * 1e-4 (a path would find it once in thirty thousand)
        "bulb" => (
            scene(
                quad([-0.005, -0.005, 1.0], [0.01, 0.0, 0.0], [0.0, 0.01, 0.0]),
                vec![material(0.0, 1e4, 1.0)],
                0.0,
            ),
            Expected {
                irradiance: 1.0,
                occlusion: 1.0,
            },
        ),
        // the bulb turned to glow up, away from the origin, as a glowing
        // decal's back faces the wall it lies on: nothing
        "back" => (
            scene(
                quad([-0.005, -0.005, 1.0], [0.0, 0.01, 0.0], [0.01, 0.0, 0.0]),
                vec![material(0.0, 1e4, 1.0)],
                0.0,
            ),
            Expected {
                irradiance: 0.0,
                occlusion: 1.0,
            },
        ),
        _ => return None,
    })
}

/// The sun of `sun` and `roof`: up at 45 degrees, giving 3.
fn sun() -> DirectionalLight {
    let half = core::f32::consts::FRAC_1_SQRT_2;
    DirectionalLight {
        towards: [half, 0.0, half],
        irradiance: [3.0; 3],
        cos_half_angle: 0.9999,
    }
}

/// A grey material.
pub fn material(albedo: f32, emission: f32, coverage: f32) -> BakeMaterial {
    BakeMaterial {
        shader: None,
        albedo: [albedo; 3],
        coverage,
        emission: [emission; 3],
        opaque: true,
    }
}

/// A scene of `triangles` of `materials` under an even sky of `sky`.
pub fn scene(triangles: Vec<SceneTriangle>, materials: Vec<BakeMaterial>, sky: f32) -> Scene {
    let mut bounds = [[f32::INFINITY; 3], [f32::NEG_INFINITY; 3]];
    for triangle in &triangles {
        for corner in triangle.positions {
            for (axis, value) in corner.into_iter().enumerate() {
                bounds[0][axis] = bounds[0][axis].min(value);
                bounds[1][axis] = bounds[1][axis].max(value);
            }
        }
    }
    if triangles.is_empty() {
        bounds = [[0.0; 3]; 2];
    }
    let mut scene = Scene {
        triangles,
        materials,
        lights: Vec::new(),
        sky: [sky; 3],
        indoor: [0.0; 3],
        bounds,
        emitters: Vec::new(),
        emitter_power: 0.0,
    };
    scene.find_emitters();
    scene
}

/// The two triangles of the rectangle from `corner` along `u` and `v`, of
/// material 0.
pub fn quad(corner: [f32; 3], u: [f32; 3], v: [f32; 3]) -> Vec<SceneTriangle> {
    let add =
        |a: [f32; 3], b: [f32; 3]| -> [f32; 3] { std::array::from_fn(|axis| a[axis] + b[axis]) };
    let (b, c, d) = (add(corner, u), add(add(corner, u), v), add(corner, v));
    vec![
        SceneTriangle {
            positions: [corner, b, c],
            material: 0,
        },
        SceneTriangle {
            positions: [corner, c, d],
            material: 0,
        },
    ]
}

/// A closed cube of side 2 about the origin, of material 0, facing in.
pub fn cube() -> Vec<SceneTriangle> {
    let mut triangles = Vec::new();
    for axis in 0..3 {
        let (u, v) = ((axis + 1) % 3, (axis + 2) % 3);
        for side in [-1.0, 1.0] {
            let mut corner = [-1.0; 3];
            corner[axis] = side;
            let (mut along_u, mut along_v) = ([0.0; 3], [0.0; 3]);
            along_u[u] = 2.0;
            along_v[v] = 2.0;
            // (the cross product of u and v points along the axis, so the
            // low side is wound the other way to face in too)
            if side < 0.0 {
                triangles.extend(quad(corner, along_v, along_u));
            } else {
                triangles.extend(quad(corner, along_u, along_v));
            }
        }
    }
    triangles
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn every_name_is_a_layout() {
        for name in NAMES {
            assert!(layout(name).is_some(), "{name}");
        }
        assert!(layout("nowhere").is_none());
        let (furnace, _) = layout("furnace").unwrap_or_else(|| panic!("no furnace"));
        assert_eq!(furnace.triangles.len(), 12);
        assert_eq!(furnace.bounds, [[-1.0; 3], [1.0; 3]]);
    }
}
