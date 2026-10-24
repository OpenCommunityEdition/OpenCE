//! What the bake lights: a structure BSP's triangles and their materials,
//! under the sky's lights.
//!
//! The original's radiosity took its light from the same tags: each sky's
//! lights (the sun, the moon) and its ambient colours, and each shader's
//! radiosity power and colour (its lamps). Transparent shaders let light
//! through, so only the structure's opaque surfaces cast shadows here; a
//! transparent surface that glows (most of the levels' light panels) is
//! kept, open (its coverage none), so that rays passing it take its light.
//! The glowing triangles are also listed as emitters, which the tracer
//! samples directly: the levels' lamps are small and bright, and a path
//! finds them by chance too seldom.

use opence_content::material::linear_color;
use opence_content::{Level, Pixels, ShaderClass, Sky, SurfaceMaterial, TagRef};

use crate::texel::mean_color;
use crate::trace::{cross, dot, luminance, sub};

/// The albedo of a surface whose map the bake cannot read (a map in
/// Custom Edition's `bitmaps.map`, or none): a middle grey, linear.
pub const UNKNOWN_ALBEDO: f32 = 0.2;
/// The most light a surface reflects, so that light bounced between two
/// surfaces dies away.
pub const MAX_ALBEDO: f32 = 0.95;
/// The bake's light for each unit of a sky light's power (the sun's, the
/// moon's). The original's radiosity did not work in linear light, so its
/// units are not the bake's: this, [`AMBIENT_EXPOSURE`] and
/// [`EMITTER_EXPOSURE`] are what bring the bake's light to the stock
/// lightmaps' brightness and colour over the stock BSPs (the mood check).
pub const SKY_EXPOSURE: f32 = 1.5;
/// The bake's light for each unit of the sky's ambient power (its dome).
/// At 3.0 the dome lit Blood Gulch's shadowed faces as brightly as its
/// sunlit ones, and the start view came out 21% brighter than stock; at
/// 0.5 it is 8% brighter, and every view is within 10% of stock's mean.
pub const AMBIENT_EXPOSURE: f32 = 0.5;
/// The bake's light for each unit of a shader's radiosity power. Fitted
/// by eye and by measure against Blood Gulch's stock lightmaps from three
/// views: at 2.5 the base's moss and lights washed it green, red and
/// orange where the stock lightmaps show only a faint glow; the difference
/// from stock was least between 0.1 and 0.25. With each glowing surface
/// glowing from its front only, the mood check holds every stock BSP at
/// 0.2, the lamp-lit ones (b40's, c40's) too. Blood Gulch's caves, lit by
/// their fungus, are darker than stock at 0.2, but more power brightens the
/// base's lamps as well, and the BSP's one exposure takes it back from the
/// outdoors (at 1.0 the canyon floor is 14% under stock).
pub const EMITTER_EXPOSURE: f32 = 0.2;

/// A structure BSP lit by its sky.
#[derive(Clone, Debug, PartialEq)]
pub struct Scene {
    /// The triangles that cast shadows and bounce light.
    pub triangles: Vec<SceneTriangle>,
    /// Each lightmap material's light: the BSP's pages' materials in order.
    pub materials: Vec<BakeMaterial>,
    /// The sky's lights.
    pub lights: Vec<DirectionalLight>,
    /// The light of the sky's dome, which rays that leave the level see
    /// (linear radiance: the outdoor ambient colour times its power).
    pub sky: [f32; 3],
    /// The indoor ambient light (linear: its colour times its power).
    pub indoor: [f32; 3],
    /// The triangles' bounds: least x, y and z, then greatest.
    pub bounds: [[f32; 3]; 2],
    /// The glowing triangles ([`Scene::find_emitters`]).
    pub emitters: Vec<Emitter>,
    /// Their power: the sum of each one's brightness times its area.
    pub emitter_power: f32,
}

/// A glowing triangle, which the tracer samples by its power.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Emitter {
    /// Its corners.
    pub corners: [[f32; 3]; 3],
    /// The light it gives, on either side (linear radiance).
    pub emission: [f32; 3],
    /// Its area.
    pub area: f32,
    /// The share of the emitters' power in it and those before it: it is
    /// chosen when a random number up to 1 falls below this and not below
    /// the one before's.
    pub cumulative: f32,
}

/// A triangle of the structure.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct SceneTriangle {
    /// Its corners.
    pub positions: [[f32; 3]; 3],
    /// Its material, in [`Scene::materials`].
    pub material: u32,
}

/// A lightmap material's part in the light.
#[derive(Clone, Debug, PartialEq)]
pub struct BakeMaterial {
    /// Its shader.
    pub shader: Option<TagRef>,
    /// The share of light it reflects, linear, red, green and blue.
    pub albedo: [f32; 3],
    /// The share of its area that is solid (its map's mean alpha, for an
    /// alpha-tested shader; none for a transparent one; else 1).
    pub coverage: f32,
    /// The light it gives (linear radiance: the shader's radiosity colour
    /// times its power).
    pub emission: [f32; 3],
    /// Whether it casts shadows (an opaque environment shader's surfaces).
    pub opaque: bool,
}

impl BakeMaterial {
    /// Whether it gives light.
    pub fn glows(&self) -> bool {
        self.emission.iter().any(|channel| *channel > 0.0)
    }
}

/// A light far away: a sky light.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct DirectionalLight {
    /// The unit vector towards it.
    pub towards: [f32; 3],
    /// The light on a surface facing it (linear: its colour times its
    /// power).
    pub irradiance: [f32; 3],
    /// The cosine of half the angle it covers in the sky (1: a point).
    pub cos_half_angle: f32,
}

impl Scene {
    /// The scene of `level`'s BSP `bsp`, under its first sky (the
    /// original's clusters chose among the skies; this bake does not
    /// import them). None if there is no such BSP.
    pub fn of(level: &Level, bsp: usize) -> Option<Self> {
        let structure = level.structure_bsps.get(bsp)?;
        let mut scene = Self {
            triangles: Vec::new(),
            materials: Vec::new(),
            lights: Vec::new(),
            sky: [0.0; 3],
            indoor: [0.0; 3],
            bounds: [[f32::INFINITY; 3], [f32::NEG_INFINITY; 3]],
            emitters: Vec::new(),
            emitter_power: 0.0,
        };
        if let Some(sky) = level.skies.first() {
            scene.light_by(sky);
        }
        for page in &structure.lightmaps {
            for material in &page.materials {
                let index = scene.materials.len() as u32;
                let baked = bake_material(level, material.shader.as_ref());
                if baked.opaque || baked.glows() {
                    let first = material.first_surface as usize;
                    let count = material.surface_count as usize;
                    let surfaces = structure.surfaces.get(first..first.saturating_add(count));
                    for surface in surfaces.unwrap_or_default() {
                        let corner = |at: u16| material.vertices.get(usize::from(at));
                        if let [Some(a), Some(b), Some(c)] = surface.map(corner) {
                            scene.add([a.position, b.position, c.position], index);
                        }
                    }
                }
                scene.materials.push(baked);
            }
        }
        if scene.triangles.is_empty() {
            scene.bounds = [[0.0; 3]; 2];
        }
        scene.find_emitters();
        Some(scene)
    }

    /// Lists the triangles whose material glows as emitters, with the
    /// share of the power each holds.
    pub fn find_emitters(&mut self) {
        self.emitters.clear();
        let mut power = 0.0f64;
        for triangle in &self.triangles {
            let Some(material) = self.materials.get(triangle.material as usize) else {
                continue;
            };
            let brightness = luminance(material.emission);
            let [a, b, c] = triangle.positions;
            let normal = cross(sub(b, a), sub(c, a));
            let area = 0.5 * dot(normal, normal).sqrt();
            if !(brightness > 0.0 && area > 0.0 && (brightness * area).is_finite()) {
                continue;
            }
            power += f64::from(brightness * area);
            self.emitters.push(Emitter {
                corners: triangle.positions,
                emission: material.emission,
                area,
                cumulative: power as f32,
            });
        }
        self.emitter_power = power as f32;
        if power > 0.0 {
            let mut sum = 0.0f64;
            for emitter in &mut self.emitters {
                sum += f64::from(luminance(emitter.emission) * emitter.area);
                emitter.cumulative = (sum / power) as f32;
            }
            if let Some(last) = self.emitters.last_mut() {
                last.cumulative = 1.0;
            }
        }
    }

    /// The chance, per unit of area, that sampling the emitters picks a
    /// point of a surface of `material` (0 for one that does not glow).
    pub fn emitter_density(&self, material: &BakeMaterial) -> f32 {
        let brightness = luminance(material.emission);
        if self.emitter_power > 0.0 && brightness > 0.0 {
            brightness / self.emitter_power
        } else {
            0.0
        }
    }

    /// The emitters as the GPU reads them: each its corners, the first
    /// with its cumulative share and the third with its density
    /// ([`Scene::emitter_density`]), then its emission and a word of
    /// padding (64 bytes).
    pub fn emitter_bytes(&self) -> Vec<u8> {
        let mut bytes = Vec::with_capacity(self.emitters.len() * 64);
        for emitter in &self.emitters {
            let [a, b, c] = emitter.corners;
            let density = if self.emitter_power > 0.0 {
                luminance(emitter.emission) / self.emitter_power
            } else {
                0.0
            };
            put_words(&mut bytes, a, emitter.cumulative);
            put_words(&mut bytes, b, emitter.area);
            put_words(&mut bytes, c, density);
            put_words(&mut bytes, emitter.emission, 0.0);
        }
        bytes
    }

    /// The brightest of the sky's lights: the sun (or the moon).
    pub fn sun(&self) -> Option<&DirectionalLight> {
        self.lights.get(self.sun_index()?)
    }

    /// The sun's index in [`Self::lights`].
    pub fn sun_index(&self) -> Option<usize> {
        let brightness = |light: &DirectionalLight| light.irradiance.iter().sum::<f32>();
        self.lights
            .iter()
            .enumerate()
            .max_by(|(_, a), (_, b)| brightness(a).total_cmp(&brightness(b)))
            .map(|(index, _)| index)
    }

    /// The materials as the GPU reads them: each its albedo and
    /// coverage, then its emission and its emitter density
    /// ([`Scene::emitter_density`]), little-endian.
    pub fn material_bytes(&self) -> Vec<u8> {
        let mut bytes = Vec::with_capacity(self.materials.len() * 32);
        for material in &self.materials {
            put_words(&mut bytes, material.albedo, material.coverage);
            put_words(
                &mut bytes,
                material.emission,
                self.emitter_density(material),
            );
        }
        bytes
    }

    /// The lights as the GPU reads them: each the way towards it and its
    /// cone's cosine, then its irradiance and a word of padding.
    pub fn light_bytes(&self) -> Vec<u8> {
        let mut bytes = Vec::with_capacity(self.lights.len() * 32);
        for light in &self.lights {
            put_words(&mut bytes, light.towards, light.cos_half_angle);
            put_words(&mut bytes, light.irradiance, 0.0);
        }
        bytes
    }

    /// The sky's lights and ambient colours.
    fn light_by(&mut self, sky: &Sky) {
        let ambient = |color: [f32; 3], power: f32| {
            let power = if power.is_finite() {
                power.max(0.0)
            } else {
                0.0
            };
            linear_color(color).map(|channel| channel * power * AMBIENT_EXPOSURE)
        };
        self.sky = ambient(sky.outdoor_ambient.color, sky.outdoor_ambient.power);
        self.indoor = ambient(sky.indoor_ambient.color, sky.indoor_ambient.power);
        for light in &sky.lights {
            let irradiance = light.radiance().map(|channel| channel * SKY_EXPOSURE);
            if irradiance.iter().all(|&channel| channel <= 0.0) {
                continue;
            }
            let diameter = if light.diameter.is_finite() {
                light.diameter
            } else {
                0.0
            };
            self.lights.push(DirectionalLight {
                towards: light.towards(),
                irradiance,
                cos_half_angle: libm::cosf(diameter.clamp(0.0, std::f32::consts::PI) / 2.0),
            });
        }
    }

    /// A triangle, unless it is degenerate or not finite.
    fn add(&mut self, positions: [[f32; 3]; 3], material: u32) {
        if positions.iter().flatten().any(|value| !value.is_finite()) {
            return;
        }
        let [a, b, c] = positions;
        let edge = |to: [f32; 3]| [to[0] - a[0], to[1] - a[1], to[2] - a[2]];
        let (u, v) = (edge(b), edge(c));
        let cross = [
            u[1] * v[2] - u[2] * v[1],
            u[2] * v[0] - u[0] * v[2],
            u[0] * v[1] - u[1] * v[0],
        ];
        if cross.iter().map(|value| value * value).sum::<f32>() <= 0.0 {
            return;
        }
        for corner in positions {
            for (axis, value) in corner.into_iter().enumerate() {
                self.bounds[0][axis] = self.bounds[0][axis].min(value);
                self.bounds[1][axis] = self.bounds[1][axis].max(value);
            }
        }
        self.triangles.push(SceneTriangle {
            positions,
            material,
        });
    }
}

/// The light of the material whose shader is `tag`.
fn bake_material(level: &Level, tag: Option<&TagRef>) -> BakeMaterial {
    let shader = tag.and_then(|tag| level.shader(tag));
    let Some(shader) = shader else {
        // (a surface with no shader the bake can find is a grey wall)
        return BakeMaterial {
            shader: tag.cloned(),
            albedo: [UNKNOWN_ALBEDO; 3],
            coverage: 1.0,
            emission: [0.0; 3],
            opaque: true,
        };
    };
    let surface = &shader.material;
    let map = surface
        .albedo_map
        .as_ref()
        .and_then(|tag| level.bitmap(tag))
        .and_then(|bitmap| bitmap.images.first())
        .and_then(|image| match &image.pixels {
            Pixels::Texture(texture) => mean_color(texture),
            Pixels::ResourceMap(_) => None,
        })
        .unwrap_or([UNKNOWN_ALBEDO, UNKNOWN_ALBEDO, UNKNOWN_ALBEDO, 1.0]);
    let albedo = std::array::from_fn(|channel| {
        opence_content::material::display(surface.albedo[channel] * map[channel])
            .clamp(0.0, MAX_ALBEDO)
    });
    let opaque = matches!(
        shader.class,
        ShaderClass::Environment(_) | ShaderClass::Model(_)
    );
    let radiosity = &shader.radiosity;
    let power = if radiosity.power.is_finite() {
        radiosity.power.max(0.0)
    } else {
        0.0
    };
    BakeMaterial {
        shader: tag.cloned(),
        albedo,
        coverage: if !opaque {
            0.0
        } else if surface.has(SurfaceMaterial::ALPHA_TESTED) {
            map[3].clamp(0.0, 1.0)
        } else {
            1.0
        },
        emission: linear_color(radiosity.color).map(|channel| channel * power * EMITTER_EXPOSURE),
        opaque,
    }
}

/// Three floats and a fourth, little-endian.
fn put_words(bytes: &mut Vec<u8>, three: [f32; 3], fourth: f32) {
    for value in three.into_iter().chain([fourth]) {
        bytes.extend_from_slice(&value.to_le_bytes());
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use opence_content::devmap;
    use opence_content::material::linear;
    use opence_content::{Lightmap, Material, ShaderRadiosity, SkyAmbient, SkyLight};

    fn devmap_scene() -> (Level, Scene) {
        let level = devmap::level(None);
        let scene = Scene::of(&level, 0).unwrap_or_else(|| panic!("no BSP"));
        (level, scene)
    }

    #[test]
    fn the_devmap_s_ground_is_its_triangles() {
        let (level, scene) = devmap_scene();
        let bsp = &level.structure_bsps[0];
        let surfaces: u32 = bsp
            .lightmaps
            .iter()
            .flat_map(|page| &page.materials)
            .map(|material| material.surface_count)
            .sum();
        assert!(surfaces > 0);
        assert_eq!(scene.triangles.len(), surfaces as usize);
        let half = devmap::HALF_SIZE;
        assert!(scene.bounds[0][0] <= -half + 1.0 && scene.bounds[1][0] >= half - 1.0);
        // its grid is lit and opaque, and gives no light
        let grid = &scene.materials[0];
        assert!(grid.opaque);
        assert_eq!(grid.emission, [0.0; 3]);
        assert!(grid
            .albedo
            .iter()
            .all(|&channel| channel > 0.0 && channel <= MAX_ALBEDO));
        assert!(Scene::of(&level, 1).is_none());
    }

    #[test]
    fn the_sky_s_lights_and_ambient_light_are_read() {
        let (level, scene) = devmap_scene();
        let sky = &level.skies[0];
        let outdoor = sky.outdoor_ambient;
        assert_eq!(
            scene.sky,
            linear_color(outdoor.color).map(|channel| channel * outdoor.power * AMBIENT_EXPOSURE)
        );
        assert_eq!(scene.indoor, [0.0; 3]);
        let sun = scene.sun().unwrap_or_else(|| panic!("no sun"));
        assert_eq!(sun.towards, sky.lights[0].towards());
        assert!(sun.towards[2] > 0.0, "the devmap's sun is up");
        assert_eq!(sun.irradiance, [SKY_EXPOSURE; 3]);
        assert!(sun.cos_half_angle < 1.0 && sun.cos_half_angle > 0.99);
    }

    #[test]
    fn the_brightest_light_is_the_sun_and_dark_lights_are_left_out() {
        let mut level = devmap::level(None);
        let light = |power: f32, yaw: f32| SkyLight {
            lens_flare: None,
            flags: 0,
            color: [1.0, 1.0, 1.0],
            power,
            test_distance: 0.0,
            direction: [yaw, 0.5],
            diameter: f32::NAN,
        };
        level.skies[0].lights = vec![light(0.5, 0.0), light(3.0, 1.0), light(0.0, 2.0)];
        level.skies[0].indoor_ambient = SkyAmbient {
            color: [1.0, 0.5, 0.0],
            power: 2.0,
        };
        let scene = Scene::of(&level, 0).unwrap_or_else(|| panic!("no BSP"));
        assert_eq!(scene.lights.len(), 2);
        let sun = scene.sun().unwrap_or_else(|| panic!("no sun"));
        assert_eq!(sun.irradiance, [3.0 * SKY_EXPOSURE; 3]);
        assert_eq!(sun.cos_half_angle, 1.0);
        assert_eq!(
            scene.indoor,
            [
                2.0 * AMBIENT_EXPOSURE,
                linear(0.5) * 2.0 * AMBIENT_EXPOSURE,
                0.0
            ]
        );
        level.skies.clear();
        let scene = Scene::of(&level, 0).unwrap_or_else(|| panic!("no BSP"));
        assert!(scene.sun().is_none());
        assert_eq!(scene.sky, [0.0; 3]);
    }

    #[test]
    fn a_shader_s_radiosity_makes_it_glow() {
        let mut level = devmap::level(None);
        let shader = level
            .shaders
            .iter_mut()
            .find(|shader| shader.tag == devmap::shader_tag())
            .unwrap_or_else(|| panic!("no grid shader"));
        shader.radiosity = ShaderRadiosity {
            flags: 0,
            power: 4.0,
            color: [1.0, 0.5, 0.0],
            tint: [1.0; 3],
        };
        let scene = Scene::of(&level, 0).unwrap_or_else(|| panic!("no BSP"));
        assert_eq!(
            scene.materials[0].emission,
            [
                4.0 * EMITTER_EXPOSURE,
                linear(0.5) * 4.0 * EMITTER_EXPOSURE,
                0.0
            ]
        );
    }

    #[test]
    fn transparent_and_unknown_shaders() {
        let mut level = devmap::level(None);
        let grid = level.structure_bsps[0].lightmaps[0].materials[0].clone();
        // the same triangles once with a water shader and once with none
        let mut water = devmap::grid_shader();
        water.tag.path = "devmap\\water".into();
        water.class = ShaderClass::Other(opence_content::Group(*b"swat"));
        level.shaders.push(water.clone());
        let lit = |shader: Option<TagRef>| Material {
            shader,
            ..grid.clone()
        };
        level.structure_bsps[0].lightmaps = vec![Lightmap {
            bitmap_index: None,
            materials: vec![lit(Some(water.tag.clone())), lit(None)],
        }];
        let scene = Scene::of(&level, 0).unwrap_or_else(|| panic!("no BSP"));
        assert!(!scene.materials[0].opaque);
        assert!(scene.materials[1].opaque);
        assert_eq!(scene.materials[1].albedo, [UNKNOWN_ALBEDO; 3]);
        assert!(scene
            .triangles
            .iter()
            .all(|triangle| triangle.material == 1));
        assert_eq!(scene.triangles.len(), grid.surface_count as usize);
        assert_eq!(scene.materials[0].coverage, 0.0);
        // a transparent shader that glows is kept, open
        let glowing = level
            .shaders
            .iter_mut()
            .find(|shader| shader.tag == water.tag)
            .unwrap_or_else(|| panic!("no water"));
        glowing.radiosity.power = 2.0;
        glowing.radiosity.color = [1.0; 3];
        let scene = Scene::of(&level, 0).unwrap_or_else(|| panic!("no BSP"));
        assert!(scene.materials[0].glows() && !scene.materials[0].opaque);
        assert_eq!(scene.materials[0].coverage, 0.0);
        assert!(!scene.materials[1].glows());
        assert_eq!(scene.triangles.len(), 2 * grid.surface_count as usize);
    }

    #[test]
    fn degenerate_and_broken_triangles_are_left_out() {
        let mut level = devmap::level(None);
        let bsp = &mut level.structure_bsps[0];
        let material = &mut bsp.lightmaps[0].materials[0];
        let first = material.first_surface as usize;
        let [a, b, _] = bsp.surfaces[first].map(usize::from);
        material.vertices[b].position = material.vertices[a].position;
        let far = usize::from(bsp.surfaces[first + 1][2]);
        material.vertices[far].position[0] = f32::NAN;
        let whole = devmap_scene().1.triangles.len();
        let scene = Scene::of(&level, 0).unwrap_or_else(|| panic!("no BSP"));
        // (the corners are shared: each broken one breaks a few triangles)
        let left = scene.triangles.len();
        assert!(left < whole && left + 12 >= whole, "{left} of {whole}");
        // (surfaces past the BSP's end: the material's are not read)
        level.structure_bsps[0].lightmaps[0].materials[0].surface_count += 1_000_000;
        let scene = Scene::of(&level, 0).unwrap_or_else(|| panic!("no BSP"));
        let surfaces = level.structure_bsps[0].lightmaps[0].materials[0].surface_count;
        assert_eq!(
            scene.triangles.len(),
            whole - (surfaces - 1_000_000) as usize
        );
        level.structure_bsps[0].lightmaps.clear();
        let scene = Scene::of(&level, 0).unwrap_or_else(|| panic!("no BSP"));
        assert_eq!(scene.bounds, [[0.0; 3]; 2]);
    }

    #[test]
    fn materials_and_lights_are_laid_out_for_the_gpu() {
        let (_, scene) = devmap_scene();
        let materials = scene.material_bytes();
        assert_eq!(materials.len(), scene.materials.len() * 32);
        let word = |bytes: &[u8], at: usize| {
            f32::from_le_bytes([bytes[at], bytes[at + 1], bytes[at + 2], bytes[at + 3]])
        };
        assert_eq!(word(&materials, 0), scene.materials[0].albedo[0]);
        assert_eq!(word(&materials, 12), scene.materials[0].coverage);
        assert_eq!(word(&materials, 16), scene.materials[0].emission[0]);
        let lights = scene.light_bytes();
        assert_eq!(lights.len(), scene.lights.len() * 32);
        assert!(!lights.is_empty());
        assert_eq!(word(&lights, 8), scene.lights[0].towards[2]);
        assert_eq!(word(&lights, 12), scene.lights[0].cos_half_angle);
        assert_eq!(word(&lights, 20), scene.lights[0].irradiance[1]);
    }

    #[test]
    fn glowing_triangles_are_emitters_by_their_power() {
        use crate::layout::{material, quad, scene};
        // a dark square, one glowing 1 and one glowing 3 of the same size
        let mut triangles = quad([0.0; 3], [1.0, 0.0, 0.0], [0.0, 1.0, 0.0]);
        for (offset, index) in [(2.0, 1), (4.0, 2)] {
            let mut lamp = quad([offset, 0.0, 0.0], [1.0, 0.0, 0.0], [0.0, 1.0, 0.0]);
            lamp.iter_mut()
                .for_each(|triangle| triangle.material = index);
            triangles.extend(lamp);
        }
        let materials = vec![
            material(0.5, 0.0, 1.0),
            material(0.5, 1.0, 1.0),
            material(0.5, 3.0, 1.0),
        ];
        let lit = scene(triangles, materials, 0.0);
        assert_eq!(lit.emitters.len(), 4);
        assert!((lit.emitter_power - 4.0).abs() < 1e-5);
        assert!((lit.emitters[0].area - 0.5).abs() < 1e-6);
        let shares: Vec<f32> = lit.emitters.iter().map(|e| e.cumulative).collect();
        for (share, expected) in shares.iter().zip([0.125, 0.25, 0.625, 1.0]) {
            assert!((share - expected).abs() < 1e-6, "{shares:?}");
        }
        assert_eq!(lit.emitter_density(&lit.materials[0]), 0.0);
        assert!((lit.emitter_density(&lit.materials[2]) - 0.75).abs() < 1e-6);
        let bytes = lit.emitter_bytes();
        assert_eq!(bytes.len(), 4 * 64);
        let word = |at: usize| {
            f32::from_le_bytes([bytes[at], bytes[at + 1], bytes[at + 2], bytes[at + 3]])
        };
        assert_eq!(word(12), 0.125);
        assert_eq!(word(28), 0.5);
        assert!((word(64 * 3 + 44) - 0.75).abs() < 1e-6);
        assert_eq!(word(64 * 3 + 48), 3.0);
        let materials = lit.material_bytes();
        let word = |at: usize| {
            f32::from_le_bytes([
                materials[at],
                materials[at + 1],
                materials[at + 2],
                materials[at + 3],
            ])
        };
        assert!((word(64 + 28) - 0.75).abs() < 1e-6);
        // nothing glows: no emitters
        let dark = scene(
            quad([0.0; 3], [1.0, 0.0, 0.0], [0.0, 1.0, 0.0]),
            vec![material(0.5, 0.0, 1.0)],
            1.0,
        );
        assert!(dark.emitters.is_empty() && dark.emitter_bytes().is_empty());
        assert_eq!(dark.emitter_density(&dark.materials[0]), 0.0);
    }
}
