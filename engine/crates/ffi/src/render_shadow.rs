//! The sun's cascaded shadow maps: where the view's distance is split
//! between the cascades, and each cascade's view from the sun.
//!
//! Each cascade holds a sphere around its slice of the view, so that its
//! size does not change as the camera turns, and its centre moves in whole
//! texels of the map, so that its shadows' edges do not crawl as the
//! camera moves.

/// The cascades the sun's shadows are drawn in.
pub const SHADOW_CASCADES: u32 = 4;

/// How far the cascades reach ahead of the camera, in world units.
pub const SHADOW_DISTANCE: f32 = 64.0;

/// How far towards the sun past a cascade's sphere what casts its shadows
/// may be, in world units.
pub const CASTER_REACH: f32 = 256.0;

/// How much the splits follow the distance's logarithm (one) rather than
/// its even steps (zero).
const LOGARITHMIC_SHARE: f32 = 0.75;

/// One cascade of the sun's shadows.
#[derive(Clone, Debug, PartialEq, uniffi::Record)]
pub struct ShadowCascade {
    /// World to the map's clip space, sixteen floats column by column: x
    /// and y across the map, depth from 0 nearest the sun to 1.
    pub view_projection: Vec<f32>,
    /// The distance ahead of the camera it reaches to.
    pub far: f32,
    /// The width of one of its texels, in world units.
    pub texel: f32,
}

/// The distances ahead of the camera at which each of `count` cascades
/// ends, from `near` to `far`: between even steps and the logarithmic
/// steps that give each cascade the same detail on screen.
#[uniffi::export]
pub fn shadow_splits(near: f32, far: f32, count: u32) -> Vec<f32> {
    let near = if near.is_finite() {
        near.max(1.0e-4)
    } else {
        1.0e-4
    };
    let far = if far.is_finite() { far.max(near) } else { near };
    let count = count.max(1);
    (1..=count)
        .map(|index| {
            let share = index as f32 / count as f32;
            let logarithmic = near * libm::powf(far / near, share);
            let even = near + (far - near) * share;
            LOGARITHMIC_SHARE * logarithmic + (1.0 - LOGARITHMIC_SHARE) * even
        })
        .collect()
}

/// The sun's shadow cascades for a camera at `position` looking along
/// `forward`, seeing `vertical_fov` radians from top to bottom of a view
/// `aspect` times as wide as high, from `near` to `distance` ahead, with
/// the sun towards `towards_sun`, in maps `resolution` texels square.
#[uniffi::export]
#[allow(clippy::too_many_arguments)]
pub fn sun_shadow_cascades(
    position: Vec<f32>,
    forward: Vec<f32>,
    vertical_fov: f32,
    aspect: f32,
    near: f32,
    distance: f32,
    towards_sun: Vec<f32>,
    resolution: u32,
) -> Vec<ShadowCascade> {
    let position = three(&position);
    let forward = normalized(three(&forward)).unwrap_or([1.0, 0.0, 0.0]);
    let Some(towards) = normalized(three(&towards_sun)) else {
        return Vec::new();
    };
    let (across, up) = light_basis(towards);
    let resolution = resolution.max(1) as f32;
    let half_height = libm::tanf(vertical_fov.clamp(0.01, 3.13) * 0.5);
    let half_width = half_height
        * if aspect.is_finite() {
            aspect.max(0.01)
        } else {
            1.0
        };
    // the slice's corners are this many times their distance off its axis
    let spread = half_width * half_width + half_height * half_height;
    let splits = shadow_splits(near, distance, SHADOW_CASCADES);
    let mut from = if near.is_finite() {
        near.max(1.0e-4)
    } else {
        1.0e-4
    };
    splits
        .into_iter()
        .map(|to| {
            // the smallest sphere through the slice's near and far corners,
            // its centre on the view's axis (no further than the far end)
            let along = ((from + to) * 0.5 * (1.0 + spread)).min(to);
            let reach = to - along;
            let radius = libm::sqrtf(reach * reach + to * to * spread);
            // (rounded up, so that rounding does not make it change)
            let radius = libm::ceilf(radius * 16.0) / 16.0;
            from = to;
            let centre = add(position, scaled(forward, along));
            let texel = radius * 2.0 / resolution;
            let snap = |value: f32| libm::floorf(value / texel) * texel;
            let centre_across = snap(dot(centre, across));
            let centre_up = snap(dot(centre, up));
            let nearest = dot(centre, towards) + radius + CASTER_REACH;
            let depth = radius * 2.0 + CASTER_REACH;
            ShadowCascade {
                view_projection: columns([
                    [
                        across[0] / radius,
                        across[1] / radius,
                        across[2] / radius,
                        -centre_across / radius,
                    ],
                    [
                        up[0] / radius,
                        up[1] / radius,
                        up[2] / radius,
                        -centre_up / radius,
                    ],
                    [
                        -towards[0] / depth,
                        -towards[1] / depth,
                        -towards[2] / depth,
                        nearest / depth,
                    ],
                    [0.0, 0.0, 0.0, 1.0],
                ]),
                far: to,
                texel,
            }
        })
        .collect()
}

/// Two directions across the sun's light, at right angles to it and to
/// each other, which do not change as the camera moves.
fn light_basis(towards: [f32; 3]) -> ([f32; 3], [f32; 3]) {
    // (the world's up, unless the sun is nearly overhead)
    let reference = if towards[2].abs() < 0.99 {
        [0.0, 0.0, 1.0]
    } else {
        [1.0, 0.0, 0.0]
    };
    let across = normalized(cross(reference, towards)).unwrap_or([0.0, 1.0, 0.0]);
    (across, cross(towards, across))
}

/// The matrix of `rows`, column by column.
fn columns(rows: [[f32; 4]; 4]) -> Vec<f32> {
    (0..4)
        .flat_map(|column| rows.iter().map(move |row| row[column]))
        .collect()
}

fn three(values: &[f32]) -> [f32; 3] {
    std::array::from_fn(|axis| {
        values
            .get(axis)
            .copied()
            .filter(|value| value.is_finite())
            .unwrap_or(0.0)
    })
}

fn dot(a: [f32; 3], b: [f32; 3]) -> f32 {
    a[0] * b[0] + a[1] * b[1] + a[2] * b[2]
}

fn cross(a: [f32; 3], b: [f32; 3]) -> [f32; 3] {
    [
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    ]
}

fn add(a: [f32; 3], b: [f32; 3]) -> [f32; 3] {
    [a[0] + b[0], a[1] + b[1], a[2] + b[2]]
}

fn scaled(a: [f32; 3], by: f32) -> [f32; 3] {
    [a[0] * by, a[1] * by, a[2] * by]
}

fn normalized(a: [f32; 3]) -> Option<[f32; 3]> {
    let length = libm::sqrtf(dot(a, a));
    (length > 1.0e-6).then(|| scaled(a, 1.0 / length))
}

#[cfg(test)]
mod tests {
    use super::*;

    const SUN: [f32; 3] = [0.3, -0.4, 0.866];

    fn cascades(position: [f32; 3], forward: [f32; 3]) -> Vec<ShadowCascade> {
        sun_shadow_cascades(
            position.to_vec(),
            forward.to_vec(),
            1.2,
            16.0 / 9.0,
            0.01,
            SHADOW_DISTANCE,
            SUN.to_vec(),
            2048,
        )
    }

    /// `point` in the cascade's clip space.
    fn project(cascade: &ShadowCascade, point: [f32; 3]) -> [f32; 3] {
        let m = &cascade.view_projection;
        std::array::from_fn(|row| {
            m[row] * point[0] + m[4 + row] * point[1] + m[8 + row] * point[2] + m[12 + row]
        })
    }

    #[test]
    fn the_splits_rise_from_near_to_far() {
        let splits = shadow_splits(0.01, 64.0, 4);
        assert_eq!(splits.len(), 4);
        assert!(splits[0] > 0.01);
        for pair in splits.windows(2) {
            assert!(pair[1] > pair[0], "{splits:?}");
        }
        assert!((splits[3] - 64.0).abs() < 1.0e-3);
        // nearer than even steps: the near cascades are the detailed ones
        assert!(splits[0] < 16.0);
    }

    #[test]
    fn the_splits_survive_nonsense() {
        assert_eq!(shadow_splits(f32::NAN, f32::INFINITY, 0).len(), 1);
        let splits = shadow_splits(5.0, 1.0, 2);
        assert!(splits.iter().all(|split| (split - 5.0).abs() < 1.0e-3));
    }

    #[test]
    fn each_cascade_holds_its_slice_of_the_view() {
        let made = cascades([10.0, 20.0, 5.0], [1.0, 0.0, 0.0]);
        assert_eq!(made.len(), SHADOW_CASCADES as usize);
        let half_height = libm::tanf(0.6);
        let half_width = half_height * 16.0 / 9.0;
        let mut from = 0.01;
        for cascade in &made {
            for distance in [from, cascade.far] {
                for (side, rise) in [(-1.0, -1.0), (-1.0, 1.0), (1.0, -1.0), (1.0, 1.0)] {
                    let corner = [
                        10.0 + distance,
                        20.0 + side * half_width * distance,
                        5.0 + rise * half_height * distance,
                    ];
                    let clip = project(cascade, corner);
                    assert!(
                        clip[0].abs() <= 1.0 + 2.0e-3 && clip[1].abs() <= 1.0 + 2.0e-3,
                        "{clip:?}"
                    );
                    assert!((0.0..=1.0).contains(&clip[2]), "{clip:?}");
                }
            }
            from = cascade.far;
        }
    }

    #[test]
    fn what_is_nearer_the_sun_is_nearer_in_the_map() {
        let made = cascades([0.0, 0.0, 0.0], [1.0, 0.0, 0.0]);
        let ground = project(&made[0], [1.0, 0.0, 0.0]);
        let above = project(&made[0], add([1.0, 0.0, 0.0], scaled(SUN, 0.5)));
        assert!(above[2] < ground[2]);
        assert!((above[0] - ground[0]).abs() < 1.0e-4);
    }

    #[test]
    fn moving_the_camera_moves_the_maps_by_whole_texels() {
        let before = cascades([10.0, 20.0, 5.0], [1.0, 0.0, 0.0]);
        let after = cascades([10.37, 20.21, 5.13], [1.0, 0.0, 0.0]);
        let point = [12.0, 21.0, 4.0];
        for (old, new) in before.iter().zip(&after) {
            assert_eq!(old.texel, new.texel);
            let (a, b) = (project(old, point), project(new, point));
            for axis in 0..2 {
                // a texel is two over the resolution of clip space
                let texels = (b[axis] - a[axis]) * 2048.0 / 2.0;
                assert!((texels - texels.round()).abs() < 0.02, "{texels}");
            }
        }
    }

    #[test]
    fn turning_the_camera_keeps_the_cascades_size() {
        let ahead = cascades([0.0, 0.0, 0.0], [1.0, 0.0, 0.0]);
        let turned = cascades([0.0, 0.0, 0.0], [0.2, 0.9, -0.3]);
        for (a, b) in ahead.iter().zip(&turned) {
            assert_eq!(a.texel, b.texel);
            assert_eq!(a.far, b.far);
        }
        assert!(ahead.windows(2).all(|pair| pair[1].texel > pair[0].texel));
    }

    #[test]
    fn no_sun_is_no_cascades() {
        let made = sun_shadow_cascades(
            vec![0.0; 3],
            vec![1.0, 0.0, 0.0],
            1.2,
            1.0,
            0.01,
            64.0,
            Vec::new(),
            1024,
        );
        assert!(made.is_empty());
    }

    #[test]
    fn an_overhead_sun_has_a_basis() {
        let (across, up) = light_basis([0.0, 0.0, 1.0]);
        assert!(dot(across, up).abs() < 1.0e-6);
        assert!((dot(across, across) - 1.0).abs() < 1.0e-5);
        assert!(dot(up, [0.0, 0.0, 1.0]).abs() < 1.0e-6);
    }
}
