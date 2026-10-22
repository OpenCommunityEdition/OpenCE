//! The path tracer: the light arriving at a point of the structure.
//!
//! This is the reference the GPU's kernel is checked against, and what the
//! bake falls back to without a GPU. From a point on a surface it sends
//! cosine-distributed rays: a ray that leaves the level sees the sky's
//! dome; one that meets a surface takes its emission and, by its albedo,
//! the light that surface receives in turn, sampled again the same way
//! (Russian roulette ends the path). The sky's lights are sampled directly
//! at every point on the path (next-event estimation), within the cone
//! each covers, so that their shadows are soft by their size.

use crate::bvh::{Bvh, Hit, Ray};
use crate::scene::Scene;

/// How far from a surface a ray starts, so that it does not meet the
/// surface it leaves (world units: about three millimetres).
pub const SURFACE_OFFSET: f32 = 1e-3;
/// How far a ray goes.
pub const RAY_LENGTH: f32 = 1.0e5;
/// The bounces after which Russian roulette may end a path.
const SURE_BOUNCES: u32 = 2;
/// The most bounces a path takes.
const MAX_BOUNCES: u32 = 16;
/// The times a ray may pass through alpha-tested holes before it is taken
/// to be stopped.
const MAX_PASSES: u32 = 8;

/// The light arriving at a point, from the hemisphere about its normal.
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct Irradiance {
    /// The light on the surface (linear, red, green and blue).
    pub irradiance: [f32; 3],
    /// The direction most of that light comes from (unit, or zero when
    /// there is none): what a normal map is lit by.
    pub direction: [f32; 3],
    /// The share of the hemisphere, by cosine, open to the sky within the
    /// occlusion distance (1: open).
    pub occlusion: f32,
}

/// A small random number generator (PCG32), seeded per texel so that a
/// bake is the same every time.
#[derive(Clone, Debug)]
pub struct Random {
    state: u64,
}

impl Random {
    /// The generator for `seed`.
    pub fn new(seed: u64) -> Self {
        let mut random = Self { state: 0 };
        random.next_u32();
        random.state = random.state.wrapping_add(seed);
        random.next_u32();
        random
    }

    /// The next 32 random bits.
    pub fn next_u32(&mut self) -> u32 {
        let state = self.state;
        self.state = state
            .wrapping_mul(6_364_136_223_846_793_005)
            .wrapping_add(1_442_695_040_888_963_407);
        let shifted = (((state >> 18) ^ state) >> 27) as u32;
        shifted.rotate_right((state >> 59) as u32)
    }

    /// A number from 0 up to 1.
    pub fn next_f32(&mut self) -> f32 {
        (self.next_u32() >> 8) as f32 / (1u32 << 24) as f32
    }
}

/// A scene and its hierarchy, ready to trace.
#[derive(Clone, Copy, Debug)]
pub struct Tracer<'a> {
    /// The scene.
    pub scene: &'a Scene,
    /// Its triangles' hierarchy (built from `scene.triangles`).
    pub bvh: &'a Bvh,
    /// How far an occluder counts for [`Irradiance::occlusion`].
    pub occlusion_distance: f32,
}

impl Tracer<'_> {
    /// The light at `point` on a surface facing `normal` (unit), from
    /// `samples` paths.
    pub fn irradiance(
        &self,
        point: [f32; 3],
        normal: [f32; 3],
        samples: u32,
        random: &mut Random,
    ) -> Irradiance {
        let origin = add(point, scale(normal, SURFACE_OFFSET));
        let mut total = [0.0f64; 3];
        let mut weighted = [0.0f32; 3];
        let mut open = 0u32;
        let samples = samples.max(1);
        for _ in 0..samples {
            let direct = self.direct(origin, normal, random);
            for (towards, irradiance) in direct {
                accumulate(&mut total, &mut weighted, towards, irradiance);
            }
            let direction = cosine_direction(normal, random);
            let ray = Ray {
                origin,
                direction,
                t_max: RAY_LENGTH,
            };
            let (radiance, first) = self.radiance(&ray, random);
            if first.is_none_or(|t| t > self.occlusion_distance) {
                open += 1;
            }
            // (cosine sampling: the irradiance is pi times the radiance)
            let irradiance = scale(radiance, core::f32::consts::PI);
            accumulate(&mut total, &mut weighted, direction, irradiance);
        }
        let count = f64::from(samples);
        let length = dot(weighted, weighted).sqrt();
        Irradiance {
            irradiance: total.map(|sum| (sum / count) as f32),
            direction: if length > 0.0 {
                scale(weighted, 1.0 / length)
            } else {
                [0.0; 3]
            },
            occlusion: open as f32 / samples as f32,
        }
    }

    /// The sky's lights on a surface at `origin` facing `normal`: each
    /// light's sampled direction and the light it gives there (zero when
    /// shadowed).
    fn direct(
        &self,
        origin: [f32; 3],
        normal: [f32; 3],
        random: &mut Random,
    ) -> Vec<([f32; 3], [f32; 3])> {
        let mut lights = Vec::with_capacity(self.scene.lights.len());
        for light in &self.scene.lights {
            let towards = cone_direction(light.towards, light.cos_half_angle, random);
            let facing = dot(normal, towards);
            if facing <= 0.0 || light.irradiance.iter().all(|channel| *channel <= 0.0) {
                continue;
            }
            let ray = Ray {
                origin,
                direction: towards,
                t_max: RAY_LENGTH,
            };
            if self.blocked(&ray, random) {
                continue;
            }
            lights.push((towards, scale(light.irradiance, facing)));
        }
        lights
    }

    /// The light coming back along `ray`, and how far it went to the first
    /// surface it met (None: it left the level).
    fn radiance(&self, ray: &Ray, random: &mut Random) -> ([f32; 3], Option<f32>) {
        let mut ray = *ray;
        let mut light = [0.0f32; 3];
        let mut throughput = [1.0f32; 3];
        let mut first = None;
        for bounce in 0..MAX_BOUNCES {
            let Some((hit, point, normal, material)) = self.meet(&ray, random) else {
                light = add(light, mul(throughput, self.scene.sky));
                break;
            };
            if bounce == 0 {
                first = Some(hit.t);
            }
            light = add(light, mul(throughput, material.emission));
            let origin = add(point, scale(normal, SURFACE_OFFSET));
            // (a Lambertian surface: radiance is albedo over pi times its
            // irradiance; the next ray's cosine sampling cancels the pi)
            let reflected = scale(material.albedo, core::f32::consts::FRAC_1_PI);
            for (_, irradiance) in self.direct(origin, normal, random) {
                light = add(light, mul(throughput, mul(reflected, irradiance)));
            }
            throughput = mul(throughput, material.albedo);
            if bounce >= SURE_BOUNCES {
                let survive = throughput.iter().copied().fold(0.0f32, f32::max).min(1.0);
                if random.next_f32() >= survive {
                    break;
                }
                throughput = scale(throughput, 1.0 / survive);
            }
            if throughput.iter().all(|channel| *channel <= 0.0) {
                break;
            }
            ray = Ray {
                origin,
                direction: cosine_direction(normal, random),
                t_max: RAY_LENGTH,
            };
        }
        (light, first)
    }

    /// The first solid surface `ray` meets: the hit, the point, the
    /// normal facing back along the ray, and its material. A ray passes
    /// through an alpha-tested surface by the share of it that is open.
    fn meet(
        &self,
        ray: &Ray,
        random: &mut Random,
    ) -> Option<(Hit, [f32; 3], [f32; 3], &crate::scene::BakeMaterial)> {
        let mut ray = *ray;
        let mut travelled = 0.0;
        for _ in 0..MAX_PASSES {
            let hit = self.bvh.nearest(&ray)?;
            let triangle = self.bvh.triangles.get(hit.triangle as usize)?;
            let material = self.scene.materials.get(triangle.material as usize)?;
            let point = add(ray.origin, scale(ray.direction, hit.t));
            let mut normal = normalize(cross(triangle.ab, triangle.ac));
            if dot(normal, ray.direction) > 0.0 {
                normal = scale(normal, -1.0);
            }
            if random.next_f32() < material.coverage {
                let hit = Hit {
                    t: travelled + hit.t,
                    ..hit
                };
                return Some((hit, point, normal, material));
            }
            travelled += hit.t;
            ray.origin = add(point, scale(ray.direction, SURFACE_OFFSET));
            ray.t_max -= hit.t;
        }
        None
    }

    /// Whether `ray` is stopped before it leaves the level (passing
    /// through alpha-tested surfaces by their open share).
    fn blocked(&self, ray: &Ray, random: &mut Random) -> bool {
        self.meet(ray, random).is_some()
    }
}

/// Adds a sample's light to the total, and its direction weighted by its
/// brightness.
fn accumulate(
    total: &mut [f64; 3],
    weighted: &mut [f32; 3],
    direction: [f32; 3],
    irradiance: [f32; 3],
) {
    for (sum, channel) in total.iter_mut().zip(irradiance) {
        *sum += f64::from(channel);
    }
    *weighted = add(*weighted, scale(direction, luminance(irradiance)));
}

/// The brightness of a linear colour (Rec. 709).
pub fn luminance(color: [f32; 3]) -> f32 {
    0.2126 * color[0] + 0.7152 * color[1] + 0.0722 * color[2]
}

/// A direction about `normal`, distributed by the cosine to it.
pub fn cosine_direction(normal: [f32; 3], random: &mut Random) -> [f32; 3] {
    let (u, v) = (random.next_f32(), random.next_f32());
    let radius = u.sqrt();
    let angle = 2.0 * core::f32::consts::PI * v;
    let (x, y) = (radius * libm::cosf(angle), radius * libm::sinf(angle));
    let z = (1.0 - u).max(0.0).sqrt();
    let [tangent, bitangent] = basis(normal);
    normalize(add(
        add(scale(tangent, x), scale(bitangent, y)),
        scale(normal, z),
    ))
}

/// A direction within the cone about `axis` whose half angle's cosine is
/// `cos_half_angle`, evenly over its solid angle.
pub fn cone_direction(axis: [f32; 3], cos_half_angle: f32, random: &mut Random) -> [f32; 3] {
    let cos_half_angle = cos_half_angle.clamp(-1.0, 1.0);
    if cos_half_angle >= 1.0 {
        return axis;
    }
    let (u, v) = (random.next_f32(), random.next_f32());
    let cos_theta = 1.0 - u * (1.0 - cos_half_angle);
    let sin_theta = (1.0 - cos_theta * cos_theta).max(0.0).sqrt();
    let angle = 2.0 * core::f32::consts::PI * v;
    let [tangent, bitangent] = basis(axis);
    normalize(add(
        add(
            scale(tangent, sin_theta * libm::cosf(angle)),
            scale(bitangent, sin_theta * libm::sinf(angle)),
        ),
        scale(axis, cos_theta),
    ))
}

/// Two unit vectors at right angles to `normal` and each other (Duff and
/// others' branchless basis).
fn basis(normal: [f32; 3]) -> [[f32; 3]; 2] {
    let sign = 1.0f32.copysign(normal[2]);
    let a = -1.0 / (sign + normal[2]);
    let b = normal[0] * normal[1] * a;
    [
        [
            1.0 + sign * normal[0] * normal[0] * a,
            sign * b,
            -sign * normal[0],
        ],
        [b, sign + normal[1] * normal[1] * a, -normal[1]],
    ]
}

fn add(a: [f32; 3], b: [f32; 3]) -> [f32; 3] {
    std::array::from_fn(|axis| a[axis] + b[axis])
}

fn mul(a: [f32; 3], b: [f32; 3]) -> [f32; 3] {
    std::array::from_fn(|axis| a[axis] * b[axis])
}

fn scale(a: [f32; 3], by: f32) -> [f32; 3] {
    a.map(|axis| axis * by)
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

fn normalize(a: [f32; 3]) -> [f32; 3] {
    let length = dot(a, a).sqrt();
    if length > 0.0 {
        scale(a, 1.0 / length)
    } else {
        a
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::layout::{layout, material, quad, scene, OCCLUSION_DISTANCE};
    use crate::scene::DirectionalLight;

    fn trace(scene: &Scene, normal: [f32; 3]) -> Irradiance {
        let bvh = Bvh::build(&scene.triangles);
        Tracer {
            scene,
            bvh: &bvh,
            occlusion_distance: OCCLUSION_DISTANCE,
        }
        .irradiance([0.0; 3], normal, 4096, &mut Random::new(7))
    }

    fn traced(name: &str) -> (Irradiance, crate::layout::Expected) {
        let (scene, expected) = layout(name).unwrap_or_else(|| panic!("no layout {name}"));
        (trace(&scene, [0.0, 0.0, 1.0]), expected)
    }

    #[test]
    fn every_layout_converges_to_its_known_light() {
        for name in crate::layout::NAMES {
            let (result, expected) = traced(name);
            for channel in result.irradiance {
                let error = (channel - expected.irradiance).abs();
                assert!(
                    error <= expected.irradiance * 0.04 + 1e-3,
                    "{name}: {channel} {expected:?}"
                );
            }
            assert!(
                (result.occlusion - expected.occlusion).abs() < 0.03,
                "{name}: {result:?}"
            );
        }
    }

    #[test]
    fn the_light_s_direction_is_where_it_comes_from() {
        assert!(traced("open").0.direction[2] > 0.95);
        // (from the open side, away from the wall)
        assert!(traced("wall").0.direction[0] < -0.3);
        let half = core::f32::consts::FRAC_1_SQRT_2;
        assert!(dot(traced("sun").0.direction, [half, 0.0, half]) > 0.99);
        assert_eq!(traced("roof").0.direction, [0.0; 3]);
        // a light below the surface gives nothing
        let (mut below, _) = layout("sun").unwrap_or_else(|| panic!("no sun"));
        below.lights[0].towards = [0.0, 0.0, -1.0];
        assert_eq!(trace(&below, [0.0, 0.0, 1.0]).irradiance, [0.0; 3]);
    }

    #[test]
    fn light_bounces_off_a_lit_floor_onto_a_ceiling() {
        // a floor reflecting a half in a sun of 2, under a point facing
        // down: the floor's radiance is 0.5 * 2 / pi, over the whole
        // hemisphere pi times that
        let floor = quad([-1e3, -1e3, -1.0], [2e3, 0.0, 0.0], [0.0, 2e3, 0.0]);
        let mut lit = scene(floor, vec![material(0.5, 0.0, 1.0)], 0.0);
        lit.lights.push(DirectionalLight {
            towards: [0.0, 0.0, 1.0],
            irradiance: [2.0; 3],
            cos_half_angle: 1.0,
        });
        let result = trace(&lit, [0.0, 0.0, -1.0]);
        assert!(
            (result.irradiance[2] - 1.0).abs() < 0.02,
            "{:?}",
            result.irradiance
        );
        assert!(result.direction[2] < -0.95);
    }

    #[test]
    fn sampling_stays_in_its_hemisphere_and_cone() {
        let mut random = Random::new(1);
        let normal = normalize([0.3, -0.2, -0.9]);
        let mut mean_cosine = 0.0;
        for _ in 0..4096 {
            let direction = cosine_direction(normal, &mut random);
            assert!((dot(direction, direction) - 1.0).abs() < 1e-4);
            assert!(dot(direction, normal) >= -1e-4);
            mean_cosine += dot(direction, normal) / 4096.0;
            let cone = cone_direction(normal, 0.95, &mut random);
            assert!(dot(cone, normal) >= 0.95 - 1e-4);
        }
        // (the mean cosine of a cosine distribution is two thirds)
        assert!((mean_cosine - 2.0 / 3.0).abs() < 0.02);
        assert_eq!(cone_direction(normal, 1.0, &mut random), normal);
        let [tangent, bitangent] = basis([0.0, 0.0, -1.0]);
        assert!(dot(tangent, bitangent).abs() < 1e-6);
        assert_eq!(luminance([1.0; 3]), 1.0);
        assert_eq!(normalize([0.0; 3]), [0.0; 3]);
    }

    #[test]
    fn the_random_numbers_are_the_same_for_a_seed() {
        let mut a = Random::new(42);
        let mut b = Random::new(42);
        let mut c = Random::new(43);
        let first: Vec<u32> = (0..8).map(|_| a.next_u32()).collect();
        assert_eq!(first, (0..8).map(|_| b.next_u32()).collect::<Vec<_>>());
        assert_ne!(first, (0..8).map(|_| c.next_u32()).collect::<Vec<_>>());
        for _ in 0..1000 {
            assert!((0.0..1.0).contains(&a.next_f32()));
        }
    }
}
