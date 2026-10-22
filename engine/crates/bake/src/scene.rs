//! What the bake lights: a structure BSP's triangles and their materials,
//! under the sky's lights.
//!
//! The original's radiosity took its light from the same tags: each sky's
//! lights (the sun, the moon) and its ambient colours, and each shader's
//! radiosity power and colour (its lamps). Transparent shaders let light
//! through, so only the structure's opaque surfaces cast shadows here.

use opence_content::material::linear_color;
use opence_content::{Level, Pixels, ShaderClass, Sky, SurfaceMaterial, TagRef};

use crate::texel::mean_color;

/// The albedo of a surface whose map the bake cannot read (a map in
/// Custom Edition's `bitmaps.map`, or none): a middle grey, linear.
pub const UNKNOWN_ALBEDO: f32 = 0.2;
/// The most light a surface reflects, so that light bounced between two
/// surfaces dies away.
pub const MAX_ALBEDO: f32 = 0.95;

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
    /// alpha-tested shader; else 1).
    pub coverage: f32,
    /// The light it gives (linear radiance: the shader's radiosity colour
    /// times its power).
    pub emission: [f32; 3],
    /// Whether it casts shadows (an opaque environment shader's surfaces).
    pub opaque: bool,
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
        };
        if let Some(sky) = level.skies.first() {
            scene.light_by(sky);
        }
        for page in &structure.lightmaps {
            for material in &page.materials {
                let index = scene.materials.len() as u32;
                let baked = bake_material(level, material.shader.as_ref());
                if baked.opaque {
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
        Some(scene)
    }

    /// The brightest of the sky's lights: the sun (or the moon).
    pub fn sun(&self) -> Option<&DirectionalLight> {
        self.lights.iter().max_by(|a, b| {
            let brightness = |light: &DirectionalLight| light.irradiance.iter().sum::<f32>();
            brightness(a).total_cmp(&brightness(b))
        })
    }

    /// The sky's lights and ambient colours.
    fn light_by(&mut self, sky: &Sky) {
        let ambient = |color: [f32; 3], power: f32| {
            let power = if power.is_finite() {
                power.max(0.0)
            } else {
                0.0
            };
            linear_color(color).map(|channel| channel * power)
        };
        self.sky = ambient(sky.outdoor_ambient.color, sky.outdoor_ambient.power);
        self.indoor = ambient(sky.indoor_ambient.color, sky.indoor_ambient.power);
        for light in &sky.lights {
            let irradiance = light.radiance();
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
        (surface.albedo[channel] * map[channel]).clamp(0.0, MAX_ALBEDO)
    });
    let radiosity = &shader.radiosity;
    let power = if radiosity.power.is_finite() {
        radiosity.power.max(0.0)
    } else {
        0.0
    };
    BakeMaterial {
        shader: tag.cloned(),
        albedo,
        coverage: if surface.has(SurfaceMaterial::ALPHA_TESTED) {
            map[3].clamp(0.0, 1.0)
        } else {
            1.0
        },
        emission: linear_color(radiosity.color).map(|channel| channel * power),
        opaque: matches!(
            shader.class,
            ShaderClass::Environment(_) | ShaderClass::Model(_)
        ),
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
            linear_color(outdoor.color).map(|channel| channel * outdoor.power)
        );
        assert_eq!(scene.indoor, [0.0; 3]);
        let sun = scene.sun().unwrap_or_else(|| panic!("no sun"));
        assert_eq!(sun.towards, sky.lights[0].towards());
        assert!(sun.towards[2] > 0.0, "the devmap's sun is up");
        assert_eq!(sun.irradiance, [1.0; 3]);
        assert!(sun.cos_half_angle < 1.0 && sun.cos_half_angle > 0.99);
    }

    #[test]
    fn the_brightest_light_is_the_sun_and_dark_lights_are_left_out() {
        let mut level = devmap::level(None);
        let light = |power: f32, yaw: f32| SkyLight {
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
        assert_eq!(sun.irradiance, [3.0; 3]);
        assert_eq!(sun.cos_half_angle, 1.0);
        assert_eq!(scene.indoor, [2.0, linear(0.5) * 2.0, 0.0]);
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
        assert_eq!(scene.materials[0].emission, [4.0, linear(0.5) * 4.0, 0.0]);
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
}
