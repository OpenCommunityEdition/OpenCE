//! The wind, as the original blows it: each of the BSP's weather palette's
//! entries that names a wind (`wind`) wanders once a tick within the wind's
//! bounds and about the entry's direction, and a point in its air is blown
//! by that, less the wind's local variation, plus a variation of its own
//! drawn from a table of smooth noise the point's place and the time look
//! up.

use opence_content::{Level, PointPhysics, StructureBsp, Wind};
use opence_sim::Vec3;

use super::Random;

/// The most palette entries that blow.
const MAX_WINDS: usize = 32;
/// The noise's samples along each axis, and its control points.
const SAMPLES: usize = 64;
const CONTROL_POINTS: usize = 8;
/// How fast the time moves each axis's noise.
const AXIS_SCALES: [f32; 3] = [0.1, 0.2, 0.07];
/// How far a wind's speed and angles wander in a tick.
const WANDER: f32 = 0.01;

/// A palette entry's wind this tick.
#[derive(Clone, Copy, Debug, Default, PartialEq)]
struct State {
    /// It has a wind.
    valid: bool,
    /// Where its speed is between its wind's bounds, 0 to 1.
    velocity_variance: f32,
    /// Where its pitch and yaw are in its wind's variation, -1 to 1.
    angular_variance: [f32; 2],
    /// Its speed.
    velocity: f32,
    /// How it blows.
    velocity3d: Vec3,
    /// Its wind's local variation's weight and rate, and its damping.
    weight: f32,
    rate: f32,
    damping: f32,
}

/// The value at `t` of the uniform cubic spline through `f0` to `f3`, which
/// are at `t0` and each `h` after.
fn uniform_cubic_spline(f: [Vec3; 4], t0: f32, h: f32, t: f32) -> Vec3 {
    let [f0, mut f1, mut f2, mut f3] = f;
    f3 -= f2;
    f2 -= f1;
    f1 -= f0;
    f3 -= f2;
    f2 -= f1;
    f0 + (f1 + (f2 + (f3 - f2) * ((t - (t0 + 2.0 * h)) / (3.0 * h))) * ((t - (t0 + h)) / (2.0 * h)))
        * ((t - t0) / h)
}

/// `[pitch, yaw]` of `vector`.
fn euler_angles(vector: Vec3) -> [f32; 2] {
    [
        libm::atan2f(
            vector.z,
            libm::sqrtf(vector.x * vector.x + vector.y * vector.y),
        ),
        libm::atan2f(vector.y, vector.x),
    ]
}

/// The unit vector of `[pitch, yaw]`.
fn from_euler_angles([pitch, yaw]: [f32; 2]) -> Vec3 {
    Vec3::new(
        libm::cosf(yaw) * libm::cosf(pitch),
        libm::sinf(yaw) * libm::cosf(pitch),
        libm::sinf(pitch),
    )
}

/// Every palette entry's wind, and the noise they vary by.
#[derive(Clone, Debug)]
pub(super) struct Winds {
    /// Each axis's noise.
    variance: [[Vec3; SAMPLES]; 3],
    states: Vec<State>,
    /// The ticks blown.
    time: u32,
}

impl Default for Winds {
    fn default() -> Self {
        Self::new(&mut Random::default())
    }
}

impl Winds {
    /// Still air, its noise drawn from `random`: random directions at each
    /// control point, and splines between.
    pub(super) fn new(random: &mut Random) -> Self {
        let mut variance = [[Vec3::ZERO; SAMPLES]; 3];
        for point in 0..CONTROL_POINTS {
            for axis in &mut variance {
                axis[point * CONTROL_POINTS] = random.direction();
            }
        }
        for point in 0..CONTROL_POINTS {
            let points = [
                (point + CONTROL_POINTS - 1) % CONTROL_POINTS,
                point,
                (point + 1) % CONTROL_POINTS,
                (point + 2) % CONTROL_POINTS,
            ];
            for sample in 1..CONTROL_POINTS {
                for axis in &mut variance {
                    axis[point * CONTROL_POINTS + sample] = uniform_cubic_spline(
                        points.map(|at| axis[at * CONTROL_POINTS]),
                        point as f32 - 1.0,
                        1.0,
                        sample as f32 * 0.125 + point as f32,
                    );
                }
            }
        }
        Self {
            variance,
            states: Vec::new(),
            time: 0,
        }
    }

    /// One tick of BSP `bsp`'s winds: each wanders a step.
    pub(super) fn tick(&mut self, level: &Level, bsp: &StructureBsp, random: &mut Random) {
        self.time = self.time.wrapping_add(1);
        let count = bsp.weather_palette.len().min(MAX_WINDS);
        self.states.resize(count, State::default());
        for (state, entry) in self.states.iter_mut().zip(&bsp.weather_palette) {
            let wind: Option<&Wind> = entry
                .wind_tag
                .as_ref()
                .and_then(|tag| level.winds.iter().find(|wind| &wind.tag == tag));
            let Some(wind) = wind else {
                state.valid = false;
                continue;
            };
            let mut step = || if random.next() < 0.5 { -WANDER } else { WANDER };
            state.velocity_variance = (state.velocity_variance + step()).clamp(0.0, 1.0);
            state.angular_variance[0] = (state.angular_variance[0] + step()).clamp(-1.0, 1.0);
            state.angular_variance[1] = (state.angular_variance[1] + step()).clamp(-1.0, 1.0);
            let [least, most] = wind.velocity;
            state.velocity = (most - least) * state.velocity_variance + least;
            let [x, y, z, magnitude] = entry.wind;
            let [pitch, yaw] = euler_angles(Vec3::new(x, y, z));
            let [area_pitch, area_yaw] = wind.variation_area;
            let direction = [
                pitch + area_pitch * state.angular_variance[0] * 0.5,
                yaw + area_yaw * state.angular_variance[1] * 0.5,
            ];
            state.velocity3d = from_euler_angles(direction) * (magnitude * state.velocity);
            state.weight = wind.local_variation_weight;
            state.rate = wind.local_variation_rate;
            state.damping = wind.damping;
            state.valid = true;
        }
    }

    /// The noise at `position`, at most `most` long along each axis.
    fn variance(&self, position: Vec3, rate: f32, most: f32) -> Vec3 {
        let mut sum = Vec3::ZERO;
        for (axis, (noise, scale)) in self.variance.iter().zip(AXIS_SCALES).enumerate() {
            let key = (scale * self.time as f32 * rate + position[axis]) * 8.0;
            // (to the nearest, half to even, as the original adds 2^23)
            let index = libm::rintf(libm::fabsf(key)) as i64 as usize % SAMPLES;
            sum += noise[index];
        }
        sum * (most / 3.0)
    }

    /// The air of palette entry `entry` at `position`: its wind with only
    /// the noise's share if `simple`, damped by its damping if `damped`;
    /// still for no entry or one without a wind.
    pub(super) fn current(
        &self,
        entry: Option<usize>,
        position: Vec3,
        simple: bool,
        damped: bool,
    ) -> Vec3 {
        let Some(state) = entry
            .and_then(|index| self.states.get(index))
            .filter(|state| state.valid)
        else {
            return Vec3::ZERO;
        };
        let weight = if simple { 0.0 } else { state.weight };
        let variance = self.variance(position, state.rate, state.weight * state.velocity);
        let current = state.velocity3d * (1.0 - weight) + variance;
        if damped {
            current * (1.0 - state.damping)
        } else {
            current
        }
    }
}

/// The air a tick's particles move through: the winds, and the BSP whose
/// clusters say which palette entry blows where.
#[derive(Clone, Copy, Debug)]
pub(super) struct Air<'a> {
    winds: Option<&'a Winds>,
    bsp: Option<&'a StructureBsp>,
}

impl<'a> Air<'a> {
    /// The air of `winds` over `bsp`.
    pub(super) fn new(winds: &'a Winds, bsp: Option<&'a StructureBsp>) -> Self {
        Self {
            winds: Some(winds),
            bsp,
        }
    }

    /// No wind anywhere.
    #[cfg(test)]
    pub(super) fn still() -> Self {
        Self {
            winds: None,
            bsp: None,
        }
    }

    /// The palette entry of the cluster `position` is in.
    pub(super) fn entry_at(&self, position: Vec3) -> Option<usize> {
        let bsp = self.bsp?;
        let leaf = bsp.collision.leaf_at(position.to_array())?;
        let cluster = (*bsp.leaf_clusters.get(leaf as usize)?)?;
        bsp.cluster_weather
            .get(usize::from(cluster))
            .copied()
            .flatten()
            .map(usize::from)
    }

    /// The air of palette entry `entry` at `position`, for a point of
    /// physics flags `flags` (simple and damped wind).
    pub(super) fn of(&self, entry: Option<usize>, position: Vec3, flags: u32) -> Vec3 {
        self.winds.map_or(Vec3::ZERO, |winds| {
            winds.current(
                entry,
                position,
                flags & PointPhysics::SIMPLE_WIND != 0,
                flags & PointPhysics::DAMPED_WIND != 0,
            )
        })
    }

    /// The air at `position`, of its cluster's palette entry.
    pub(super) fn at(&self, position: Vec3, flags: u32) -> Vec3 {
        if self.winds.is_none() {
            return Vec3::ZERO;
        }
        self.of(self.entry_at(position), position, flags)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use opence_content::{Group, TagRef, WeatherPalette};

    fn wind_tag() -> TagRef {
        TagRef {
            group: Group(*b"wind"),
            path: "breeze".into(),
        }
    }

    fn breeze(weight: f32, damping: f32) -> Wind {
        Wind {
            tag: wind_tag(),
            velocity: [1.0, 1.0],
            variation_area: [0.0, 0.0],
            local_variation_weight: weight,
            local_variation_rate: 1.0,
            damping,
        }
    }

    fn blowing(wind: Wind, direction: [f32; 3], magnitude: f32) -> (Level, StructureBsp) {
        let mut level = opence_content::devmap::level(None);
        level.winds.push(wind);
        let mut bsp = level.structure_bsps[0].clone();
        let [x, y, z] = direction;
        bsp.weather_palette = vec![
            WeatherPalette {
                system: None,
                wind_tag: Some(wind_tag()),
                wind: [x, y, z, magnitude],
            },
            WeatherPalette {
                system: None,
                wind_tag: None,
                wind: [1.0, 0.0, 0.0, 9.0],
            },
        ];
        (level, bsp)
    }

    #[test]
    fn the_noise_passes_through_its_control_points_smoothly() {
        let winds = Winds::new(&mut Random::default());
        for axis in &winds.variance {
            for point in 0..CONTROL_POINTS {
                let at = axis[point * CONTROL_POINTS];
                assert!((at.length() - 1.0).abs() < 1e-4, "a direction at {point}");
                // (the sample after a control point is near it)
                let next = axis[point * CONTROL_POINTS + 1];
                assert!(next.distance(at) < 1.0, "smooth after {point}");
            }
        }
        let f = [Vec3::X, Vec3::Y, Vec3::Z, Vec3::ONE];
        assert!(uniform_cubic_spline(f, -1.0, 1.0, 0.0).distance(Vec3::Y) < 1e-5);
        assert!(uniform_cubic_spline(f, -1.0, 1.0, 1.0).distance(Vec3::Z) < 1e-5);
    }

    #[test]
    fn a_palette_entry_blows_its_way_at_its_winds_speed_and_one_without_is_still() {
        let (level, bsp) = blowing(breeze(0.0, 0.0), [0.0, 2.0, 0.0], 3.0);
        let mut winds = Winds::new(&mut Random::default());
        winds.tick(&level, &bsp, &mut Random::default());
        let blown = winds.current(Some(0), Vec3::ZERO, false, false);
        assert!(blown.distance(Vec3::new(0.0, 3.0, 0.0)) < 1e-4, "{blown:?}");
        assert_eq!(winds.current(Some(1), Vec3::ZERO, false, false), Vec3::ZERO);
        assert_eq!(winds.current(Some(7), Vec3::ZERO, false, false), Vec3::ZERO);
        assert_eq!(winds.current(None, Vec3::ZERO, false, false), Vec3::ZERO);
    }

    #[test]
    fn the_speed_wanders_within_its_bounds() {
        let mut wind = breeze(0.0, 0.0);
        wind.velocity = [1.0, 2.0];
        let (level, bsp) = blowing(wind, [1.0, 0.0, 0.0], 1.0);
        let mut winds = Winds::new(&mut Random::default());
        let mut random = Random::default();
        for _ in 0..500 {
            winds.tick(&level, &bsp, &mut random);
            let speed = winds.current(Some(0), Vec3::ZERO, false, false).length();
            assert!((1.0 - 1e-4..=2.0 + 1e-4).contains(&speed), "{speed}");
        }
    }

    #[test]
    fn local_variation_varies_by_place_and_simple_or_damped_air_less() {
        let (level, bsp) = blowing(breeze(0.5, 0.25), [1.0, 0.0, 0.0], 2.0);
        let mut winds = Winds::new(&mut Random::default());
        winds.tick(&level, &bsp, &mut Random::default());
        let here = winds.current(Some(0), Vec3::ZERO, false, false);
        let there = winds.current(Some(0), Vec3::new(0.3, 0.6, 0.2), false, false);
        assert!(here.distance(there) > 1e-3, "{here:?} {there:?}");
        // (each of the three samples is a unit at most, so the noise is at
        // most the weight times the speed)
        let base = Vec3::new(2.0, 0.0, 0.0) * 0.5;
        assert!(here.distance(base) <= 0.5 * 2.0 * 1.0 + 1e-4);
        let simple = winds.current(Some(0), Vec3::ZERO, true, false);
        assert!((simple - here).distance(base) < 1e-4, "the whole wind");
        let damped = winds.current(Some(0), Vec3::ZERO, false, true);
        assert!(damped.distance(here * 0.75) < 1e-4);
    }

    #[test]
    fn the_air_maps_point_physics_flags_and_still_air_is_still() {
        let (level, bsp) = blowing(breeze(0.0, 0.5), [1.0, 0.0, 0.0], 2.0);
        let mut winds = Winds::new(&mut Random::default());
        winds.tick(&level, &bsp, &mut Random::default());
        let air = Air::new(&winds, Some(&bsp));
        let open = air.of(Some(0), Vec3::ZERO, 0);
        let damped = air.of(Some(0), Vec3::ZERO, PointPhysics::DAMPED_WIND);
        assert!(damped.distance(open * 0.5) < 1e-4);
        assert_eq!(Air::still().at(Vec3::ZERO, 0), Vec3::ZERO);
        assert_eq!(Air::new(&winds, None).entry_at(Vec3::ZERO), None);
    }
}
