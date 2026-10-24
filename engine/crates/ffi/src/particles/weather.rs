//! The weather, as the original fills a box about the camera with it
//! (`weather_particle_systems.c`): the camera's cluster names a palette
//! entry, whose system's kinds each keep particles in a box as wide as the
//! kind's farthest fade, placed in the box's own space so that moving the
//! camera never moves them. Each is drawn in whichever copy of the box,
//! among the camera's and its visible neighbours', puts it in view, faded
//! by its depth, and hidden inside the BSP's weather polyhedra.

use opence_content::{Level, StructureBsp, WeatherParticleType};
use opence_sim::{point_physics, Quat, Vec3};

use super::wind::Air;
use super::{blend, mix, rgba, Pictures, Random, TICK};
use crate::snapshot::{
    ParticleSnapshot, MAX_PARTICLES, PARTICLE_FACING_AXIS, PARTICLE_FACING_SCREEN,
};

/// The most weather particles, of every kind.
const MAX_DROPS: usize = 512;
/// The most kinds a system has.
const MAX_KINDS: usize = 8;
/// The most polyhedra that hide a kind's particles.
const MAX_POLYHEDRA: usize = 8;
/// The most copies of a kind's box drawn.
const MAX_BOXES: usize = 26;
/// How far a particle drifts its own way, a tick.
const DRIFT: f32 = 0.001;
/// How much wider than high the view is taken to be.
const ASPECT: f32 = 2.4;
/// The vertical field of view taken for one that is not given.
const DEFAULT_FOV: f32 = 70.0 * std::f32::consts::PI / 180.0;

/// The camera the weather is drawn for.
#[derive(Clone, Copy, Debug, PartialEq)]
pub(crate) struct ParticleView {
    /// Where it is.
    pub(crate) position: Vec3,
    /// Which way it faces (forward is x, left y, up z).
    pub(crate) rotation: Quat,
    /// Its vertical field of view, in radians.
    pub(crate) vertical_fov: f32,
}

impl ParticleView {
    /// A camera at `position` looking along x.
    #[cfg(test)]
    pub(crate) fn at(position: Vec3) -> Self {
        Self {
            position,
            rotation: Quat::IDENTITY,
            vertical_fov: DEFAULT_FOV,
        }
    }

    /// The planes of its view, their normals facing out (a point inside
    /// lies behind them all), each as its normal and distance: the far
    /// plane at `far`, then left, right, top and bottom.
    fn planes(&self, far: f32) -> [(Vec3, f32); 5] {
        let forward = (self.rotation * Vec3::X).normalize_or_zero();
        let left = (self.rotation * Vec3::Y).normalize_or_zero();
        let up = (self.rotation * Vec3::Z).normalize_or_zero();
        let fov = if self.vertical_fov.is_finite() && self.vertical_fov > 0.0 {
            self.vertical_fov.min(3.0)
        } else {
            DEFAULT_FOV
        };
        let tall = libm::tanf(fov * 0.5);
        let wide = tall * ASPECT;
        let plane = |normal: Vec3| {
            let normal = normal.normalize_or_zero();
            (normal, normal.dot(self.position))
        };
        [
            (forward, forward.dot(self.position + forward * far)),
            plane(left - forward * wide),
            plane(-left - forward * wide),
            plane(up - forward * tall),
            plane(-up - forward * tall),
        ]
    }
}

/// A weather particle, in its box's own space.
#[derive(Clone, Debug, PartialEq)]
struct Drop {
    position: Vec3,
    velocity: Vec3,
    acceleration: Vec3,
    /// Its own way, which it drifts along.
    drift: Vec3,
    radius: f32,
    rotation: f32,
    /// How fast it spins, and which way.
    spin: f32,
    color: [f32; 4],
    /// Sprites a second.
    fps: f32,
    sequence: i32,
    sprite: f32,
}

/// One kind's particles.
#[derive(Clone, Debug, Default)]
struct Kind {
    /// Particles a cubic world unit, chosen when the system began.
    density: f32,
    drops: Vec<Drop>,
}

/// The weather about the camera.
#[derive(Clone, Debug, Default)]
pub(super) struct Weather {
    /// The system falling (its index in the level's weather).
    system: Option<usize>,
    /// The palette entry the camera is in, whose wind blows it.
    entry: Option<usize>,
    /// The BSP it falls in.
    bsp: usize,
    kinds: Vec<Kind>,
    /// The particles made, which number them.
    made: u32,
}

/// The width of a kind's box: its farthest fade.
fn box_width(kind: &WeatherParticleType) -> f32 {
    let width = kind.distance_fade[3];
    if width.is_finite() {
        width.max(0.0)
    } else {
        0.0
    }
}

/// How far `value` is from `start` to `end`, 0 to 1; a range of nothing
/// is passed at once.
fn ramp(value: f32, start: f32, end: f32) -> f32 {
    if end > start {
        pin((value - start) / (end - start), 0.0, 1.0)
    } else if value >= start {
        1.0
    } else {
        0.0
    }
}

/// `value` held between `least` and `most`, as the original's `PIN`
/// (which never fails on bounds the wrong way round).
fn pin(value: f32, least: f32, most: f32) -> f32 {
    if value < least {
        least
    } else if value > most {
        most
    } else {
        value
    }
}

/// `point` brought into the box of width `width` from 0.
fn wrapped(point: Vec3, width: f32) -> Vec3 {
    let wrap = |value: f32| {
        let value = libm::fmodf(value, width);
        if value < 0.0 {
            value + width
        } else {
            value
        }
    };
    Vec3::new(wrap(point.x), wrap(point.y), wrap(point.z))
}

/// How many sprites (or images) sequence `sequence` of `bitmap` has, and
/// how many sequences it has.
fn sprites(
    level: &Level,
    bitmap: Option<&opence_content::TagRef>,
    sequence: i32,
) -> (usize, usize) {
    let Some(bitmap) = bitmap.and_then(|tag| level.bitmap(tag)) else {
        return (1, 0);
    };
    let count = usize::try_from(sequence)
        .ok()
        .and_then(|index| bitmap.sequences.get(index))
        .map_or(1, |sequence| {
            if sequence.sprites.is_empty() {
                usize::from(sequence.image_count)
            } else {
                sequence.sprites.len()
            }
        });
    (count.max(1), bitmap.sequences.len())
}

/// The scale the camera's height gives a kind: none below its height
/// fade in, none above its fade out.
fn height_scale(kind: &WeatherParticleType, height: f32) -> f32 {
    let [in_start, in_end, out_start, out_end] = kind.height_fade;
    if in_start == 0.0 && in_end == 0.0 && out_start == 0.0 && out_end == 0.0 {
        return 1.0;
    }
    ramp(height, in_start, in_end) * (1.0 - ramp(height, out_start, out_end))
}

impl Drop {
    /// A new particle of `kind`, the `index`th made, somewhere in its box
    /// of width `width`.
    fn new(
        level: &Level,
        kind: &WeatherParticleType,
        width: f32,
        index: u32,
        random: &mut Random,
    ) -> Self {
        let place = Vec3::new(random.next(), random.next(), random.next()) * width;
        let [least, most, ..] = kind.acceleration;
        let (_, sequences) = sprites(level, kind.bitmap.as_ref(), 0);
        let sequence = random.below(i32::try_from(sequences).unwrap_or(i32::MAX));
        let (count, _) = sprites(level, kind.bitmap.as_ref(), sequence);
        let [from, to] = kind.color.map(rgba);
        let mut color = mix(from, to, random.next());
        color[3] = random.between([from[3], to[3]]);
        let way = if index & 1 == 1 { -1.0 } else { 1.0 };
        Self {
            position: place,
            velocity: Vec3::ZERO,
            acceleration: random.direction() * random.between([least, most]),
            drift: Random(index.wrapping_mul(0x9E37_79B9) | 1).direction(),
            radius: random.between(kind.radius),
            rotation: if kind.flags & WeatherParticleType::RANDOM_ROTATION != 0 {
                std::f32::consts::TAU * random.next()
            } else {
                0.0
            },
            spin: way * random.between(kind.rotation_rate),
            color,
            fps: random.between(kind.animation_rate),
            sequence,
            sprite: random.next() * count as f32,
        }
    }

    /// One tick: it animates, spins, turns its acceleration, moves in the
    /// air of palette entry `entry`, drifts its own way and wraps.
    fn tick(
        &mut self,
        level: &Level,
        kind: &WeatherParticleType,
        width: f32,
        (air, entry): (&Air<'_>, Option<usize>),
        random: &mut Random,
    ) {
        let (count, _) = sprites(level, kind.bitmap.as_ref(), self.sequence);
        self.sprite = libm::fmodf(self.sprite + self.fps * TICK, count as f32);
        self.rotation += self.spin * TICK;
        let [least, most, turning, change] = kind.acceleration;
        if least != 0.0 || most != 0.0 {
            let magnitude = self.acceleration.length();
            let along = self.acceleration.normalize_or_zero();
            let magnitude = pin(magnitude + random.between([-change, change]), least, most);
            self.acceleration =
                (random.direction() * turning + along * (1.0 - turning)) * magnitude;
            self.velocity += self.acceleration * TICK;
        }
        if let Some(physics) = &kind.physics {
            let wind = air.of(entry, self.position, physics.flags);
            point_physics::update(
                physics,
                &mut self.position,
                &mut self.velocity,
                self.radius,
                TICK,
                (false, wind),
                |_, _| None,
            );
        } else {
            self.position += self.velocity * TICK;
        }
        self.position += self.drift * (DRIFT * TICK * 30.0);
        self.position = wrapped(self.position, width);
    }
}

impl Weather {
    /// One tick on BSP `bsp` about `camera`: the system of the camera's
    /// cluster begins (or ends), each kind's count follows its density and
    /// the camera's height, and its particles move in `air`.
    pub(super) fn tick(
        &mut self,
        level: &Level,
        (bsp, structure): (usize, &StructureBsp),
        air: &Air<'_>,
        camera: Vec3,
        random: &mut Random,
    ) {
        let entry = air.entry_at(camera);
        let system = entry
            .and_then(|index| structure.weather_palette.get(index))
            .and_then(|palette| palette.system.as_ref())
            .and_then(|tag| level.weather.iter().position(|system| &system.tag == tag));
        self.entry = entry;
        if system != self.system || bsp != self.bsp {
            self.system = system;
            self.bsp = bsp;
            self.kinds = system
                .and_then(|index| level.weather.get(index))
                .map_or(&[][..], |system| system.types.as_slice())
                .iter()
                .take(MAX_KINDS)
                .map(|kind| Kind {
                    density: random.between(kind.density),
                    drops: Vec::new(),
                })
                .collect();
        }
        let Some(definition) = self.system.and_then(|index| level.weather.get(index)) else {
            return;
        };
        let mut alive: usize = self.kinds.iter().map(|kind| kind.drops.len()).sum();
        for (kind, state) in definition.types.iter().zip(&mut self.kinds) {
            let width = box_width(kind);
            let scale = height_scale(kind, camera.z);
            // (`as` takes what is not a number to 0, as the original's
            // count is held to at least 0)
            let target = (state.density * width * width * width * scale) as usize;
            while state.drops.len() > target {
                state.drops.pop();
                alive -= 1;
            }
            while state.drops.len() < target && alive < MAX_DROPS {
                state
                    .drops
                    .push(Drop::new(level, kind, width, self.made, random));
                self.made = self.made.wrapping_add(1);
                alive += 1;
            }
            for drop in &mut state.drops {
                drop.tick(level, kind, width, (air, self.entry), random);
            }
        }
    }

    /// The frame's weather particles, seen from `view`.
    pub(super) fn gather(
        &self,
        level: &Level,
        pictures: &Pictures,
        view: &ParticleView,
        out: &mut Vec<ParticleSnapshot>,
    ) {
        let Some(definition) = self.system.and_then(|index| level.weather.get(index)) else {
            return;
        };
        let polyhedra = level
            .structure_bsps
            .get(self.bsp)
            .map_or(&[][..], |bsp| bsp.weather_polyhedra.as_slice());
        let camera = view.position;
        let forward = (view.rotation * Vec3::X).normalize_or_zero();
        for (kind, state) in definition.types.iter().zip(&self.kinds) {
            let width = box_width(kind);
            if state.drops.is_empty() || width <= 0.0 {
                continue;
            }
            let planes = view.planes(width);
            let boxes = boxes(&planes, camera, width);
            let hiding: Vec<_> = polyhedra
                .iter()
                .filter(|polyhedron| {
                    let reach = width + polyhedron.radius;
                    Vec3::from(polyhedron.center).distance_squared(camera) < reach * reach
                })
                .take(MAX_POLYHEDRA)
                .collect();
            let [near, full, start, end] = kind.distance_fade;
            let far = end.min(width);
            for drop in &state.drops {
                if out.len() >= MAX_PARTICLES {
                    return;
                }
                let Some(position) = boxes
                    .iter()
                    .map(|corner| *corner + drop.position)
                    .find(|&point| planes.iter().all(|&(n, d)| n.dot(point) - d < 0.0))
                else {
                    continue;
                };
                let depth = (position - camera).dot(forward);
                if !(depth > near && depth < far) {
                    continue;
                }
                let hidden = hiding.iter().any(|polyhedron| {
                    polyhedron
                        .planes
                        .iter()
                        .all(|&[x, y, z, d]| Vec3::new(x, y, z).dot(position) - d >= 0.0)
                });
                if hidden {
                    continue;
                }
                let fade = ramp(depth, near, full) * (1.0 - ramp(depth, start, far));
                let source = if kind.direction_source == 1 {
                    drop.acceleration
                } else {
                    drop.velocity
                };
                let along = source.normalize_or_zero();
                let (facing, axis) = match (kind.render_mode, along == Vec3::ZERO) {
                    (0, _) => (PARTICLE_FACING_SCREEN, along),
                    (_, true) => (PARTICLE_FACING_AXIS, Vec3::Z),
                    (_, false) => (PARTICLE_FACING_AXIS, along),
                };
                let frame = pictures.frame(
                    level,
                    kind.bitmap.as_ref(),
                    drop.sequence,
                    drop.sprite as usize,
                );
                let [r, g, b, a] = drop.color;
                out.push(ParticleSnapshot {
                    position: [position.x, position.y, position.z, drop.radius],
                    axis: [axis.x, axis.y, axis.z, drop.rotation],
                    color: [r, g, b, a * fade],
                    uv: frame.uv,
                    shape: [drop.radius, 0.0, 0.0, 0.0],
                    modes: [
                        frame.picture,
                        blend(kind.shader.framebuffer_blend_function),
                        facing,
                        0,
                    ],
                });
            }
        }
    }
}

/// The corners of the copies of a box of width `width` that are drawn:
/// the one `camera` is in, then those of its neighbours inside `planes`.
fn boxes(planes: &[(Vec3, f32); 5], camera: Vec3, width: f32) -> Vec<Vec3> {
    let home = camera - wrapped(camera, width);
    let mut boxes = vec![home];
    let offsets = [-width, 0.0, width];
    for x in offsets {
        for y in offsets {
            for z in offsets {
                let corner = home + Vec3::new(x, y, z);
                if (x, y, z) == (0.0, 0.0, 0.0) || boxes.len() >= MAX_BOXES {
                    continue;
                }
                if cube_visible(planes, corner, width) {
                    boxes.push(corner);
                }
            }
        }
    }
    boxes
}

/// Whether some of the cube from `corner`, `width` wide, is behind every
/// one of `planes`.
fn cube_visible(planes: &[(Vec3, f32); 5], corner: Vec3, width: f32) -> bool {
    planes.iter().all(|&(normal, distance)| {
        // (its corner nearest the inside)
        let nearest = corner
            + Vec3::new(
                if normal.x < 0.0 { width } else { 0.0 },
                if normal.y < 0.0 { width } else { 0.0 },
                if normal.z < 0.0 { width } else { 0.0 },
            );
        normal.dot(nearest) - distance < 0.0
    })
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::particles::wind::Winds;
    use opence_content::{
        Group, PointPhysics, TagRef, WeatherPalette, WeatherPolyhedron, WeatherSystem,
    };

    fn tag(name: &str) -> TagRef {
        TagRef {
            group: Group(*b"rain"),
            path: name.into(),
        }
    }

    fn kind(density: f32, width: f32) -> WeatherParticleType {
        WeatherParticleType {
            flags: 0,
            distance_fade: [0.0, 0.0, width * 0.5, width],
            density: [density, density],
            physics: None,
            acceleration: [0.0; 4],
            radius: [0.1, 0.1],
            animation_rate: [0.0, 0.0],
            rotation_rate: [0.0, 0.0],
            color: [[1.0; 4], [1.0; 4]],
            bitmap: None,
            render_mode: 0,
            shader: Default::default(),
            height_fade: [0.0; 4],
            direction_source: 0,
            one_over_sprite_width: 1.0,
        }
    }

    /// The devmap with `kinds` falling in every cluster, and a point in
    /// its air.
    fn raining(kinds: Vec<WeatherParticleType>) -> (Level, Vec3) {
        let mut level = opence_content::devmap::level(None);
        level.weather = vec![WeatherSystem {
            tag: tag("rain"),
            types: kinds,
        }];
        let bsp = &mut level.structure_bsps[0];
        bsp.weather_palette = vec![WeatherPalette {
            system: Some(tag("rain")),
            wind_tag: None,
            wind: [1.0, 0.0, 0.0, 0.0],
        }];
        bsp.cluster_weather = vec![Some(0)];
        bsp.leaf_clusters = vec![Some(0); bsp.collision.leaves.len()];
        let camera = [1.0, -1.0, 10.0, -10.0]
            .into_iter()
            .map(|z| Vec3::new(0.5, 0.5, z))
            .find(|point| bsp.collision.leaf_at(point.to_array()).is_some());
        let Some(camera) = camera else {
            panic!("the devmap has no leaf");
        };
        (level, camera)
    }

    fn run(level: &Level, weather: &mut Weather, camera: Vec3, ticks: usize) {
        let winds = Winds::default();
        let air = Air::new(&winds, level.structure_bsps.first());
        let mut random = Random::default();
        for _ in 0..ticks {
            weather.tick(
                level,
                (0, &level.structure_bsps[0]),
                &air,
                camera,
                &mut random,
            );
        }
    }

    #[test]
    fn a_kinds_count_is_its_density_through_its_box_and_held_to_the_pool() {
        let (level, camera) = raining(vec![kind(0.25, 4.0), kind(100.0, 4.0)]);
        let mut weather = Weather::default();
        run(&level, &mut weather, camera, 1);
        assert_eq!(weather.kinds[0].drops.len(), 16, "0.25 a unit through 4³");
        assert_eq!(
            weather.kinds[1].drops.len(),
            MAX_DROPS - 16,
            "the rest of the pool"
        );
        let width = 4.0;
        for drop in weather.kinds.iter().flat_map(|kind| &kind.drops) {
            assert!(drop.position.cmpge(Vec3::ZERO).all(), "{}", drop.position);
            assert!(
                drop.position.cmplt(Vec3::splat(width)).all(),
                "{}",
                drop.position
            );
        }
    }

    #[test]
    fn the_cameras_height_fades_a_kind_in_and_out() {
        let mut faded = kind(1.0, 4.0);
        faded.height_fade = [0.0, 10.0, 20.0, 30.0];
        assert_eq!(height_scale(&faded, -1.0), 0.0);
        assert!((height_scale(&faded, 5.0) - 0.5).abs() < 1e-6);
        assert_eq!(height_scale(&faded, 15.0), 1.0);
        assert!((height_scale(&faded, 25.0) - 0.5).abs() < 1e-6);
        assert_eq!(height_scale(&faded, 31.0), 0.0);
        assert_eq!(height_scale(&kind(1.0, 4.0), 1000.0), 1.0, "no fade given");
    }

    #[test]
    fn a_wind_blows_the_weather_of_its_palette_entry() {
        let mut blown = kind(1.0, 4.0);
        blown.physics = Some(PointPhysics {
            flags: 0,
            mass_over_radius_cubed: 0.0,
            water_buoyancy: 0.0,
            air_buoyancy: 0.0,
            density: 0.0,
            air_friction: 1000.0,
            water_friction: 0.0,
            contact_friction: 0.0,
            elasticity: 0.0,
        });
        let (mut level, camera) = raining(vec![blown]);
        level.winds = vec![opence_content::Wind {
            tag: tag("breeze"),
            velocity: [3.0, 3.0],
            variation_area: [0.0, 0.0],
            local_variation_weight: 0.0,
            local_variation_rate: 0.0,
            damping: 0.0,
        }];
        level.structure_bsps[0].weather_palette[0].wind = [1.0, 0.0, 0.0, 1.0];
        level.structure_bsps[0].weather_palette[0].wind_tag = Some(tag("breeze"));
        let mut winds = Winds::default();
        let mut random = Random::default();
        let mut weather = Weather::default();
        for _ in 0..30 {
            winds.tick(&level, &level.structure_bsps[0], &mut random);
            let air = Air::new(&winds, level.structure_bsps.first());
            weather.tick(
                &level,
                (0, &level.structure_bsps[0]),
                &air,
                camera,
                &mut random,
            );
        }
        let Some(drop) = weather.kinds[0].drops.first() else {
            panic!("no weather");
        };
        assert!(drop.velocity.x > 2.5, "{}", drop.velocity);
        assert!(drop.velocity.y.abs() < 0.1, "{}", drop.velocity);
    }

    #[test]
    fn weather_is_drawn_ahead_faded_by_depth_and_not_inside_a_polyhedron() {
        let width = 8.0;
        let (mut level, camera) = raining(vec![kind(1.0, width)]);
        let mut weather = Weather::default();
        run(&level, &mut weather, camera, 1);
        let view = ParticleView::at(camera);
        let pictures = Pictures::new(&level);
        let mut out = Vec::new();
        weather.gather(&level, &pictures, &view, &mut out);
        assert!(!out.is_empty());
        for sprite in &out {
            let [x, y, z, _] = sprite.position;
            let depth = x - camera.x;
            assert!(depth > 0.0 && depth < width, "{depth}");
            let alpha = sprite.color[3];
            if depth > width * 0.75 {
                assert!(alpha < 0.5 + 1e-4, "{depth} {alpha}");
            }
            let _ = (y, z);
        }
        // (a polyhedron about everything ahead hides it all)
        let ahead = camera + Vec3::X * width * 0.5;
        level.structure_bsps[0].weather_polyhedra = vec![WeatherPolyhedron {
            center: ahead.to_array(),
            radius: width * 2.0,
            planes: vec![
                [1.0, 0.0, 0.0, camera.x - 1.0],
                [-1.0, 0.0, 0.0, -(camera.x + width * 2.0)],
            ],
        }];
        weather.gather(&level, &pictures, &view, &mut out);
        let before = out.len();
        out.clear();
        weather.gather(&level, &pictures, &view, &mut out);
        assert!(out.is_empty(), "{before} drawn inside");
    }

    #[test]
    fn only_the_boxes_in_view_are_drawn() {
        let view = ParticleView::at(Vec3::new(0.5, 0.5, 0.5));
        let planes = view.planes(4.0);
        let drawn = boxes(&planes, view.position, 4.0);
        assert_eq!(drawn[0], Vec3::ZERO);
        assert!(drawn.len() > 1 && drawn.len() <= MAX_BOXES);
        assert!(drawn.iter().all(|corner| corner.x >= 0.0), "{drawn:?}");
    }
}
