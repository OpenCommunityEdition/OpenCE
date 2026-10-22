//! The bake's scene, for the app's path tracer (OpenCEEngineRender's
//! PathTracer): a BSP's hierarchy, materials and lights as the GPU reads
//! them, and the CPU's reference tracer to check the GPU's against; and
//! a level's bake: the points to trace, and the light they make, kept in
//! the cache folder.

use std::sync::Arc;

use opence_bake::mood::BspMood;
use opence_bake::output::BAKE_VERSION;
use opence_bake::{
    chart, layout, BakePlan, BakeSettings, BakedLighting, Bvh, Random, Scene, Tracer,
};

use crate::{EngineError, LevelGeometry};

/// A structure BSP (or a test layout) ready to bake.
#[derive(uniffi::Object)]
pub struct BakeScene {
    scene: Scene,
    bvh: Bvh,
}

/// The light found at one point.
#[derive(Clone, Debug, PartialEq, uniffi::Record)]
pub struct BakeSample {
    /// Red, green and blue, linear.
    pub irradiance: Vec<f32>,
    /// The direction most of it comes from (unit, or zeros).
    pub direction: Vec<f32>,
    /// How much of the light comes from that direction, 0 to 1.
    pub directionality: f32,
    /// The open share of the hemisphere.
    pub occlusion: f32,
}

/// What a tracer must find at a test layout's origin.
#[derive(Clone, Copy, Debug, PartialEq, uniffi::Record)]
pub struct BakeExpected {
    /// The irradiance, each channel.
    pub irradiance: f32,
    /// The open share of the hemisphere.
    pub occlusion: f32,
}

impl BakeScene {
    fn new(scene: Scene) -> Arc<Self> {
        let bvh = Bvh::build(&scene.triangles);
        Arc::new(Self { scene, bvh })
    }
}

/// The test layouts' names (opence_bake's `layout`).
#[uniffi::export]
pub fn bake_layout_names() -> Vec<String> {
    layout::NAMES
        .iter()
        .map(|name| (*name).to_owned())
        .collect()
}

/// The occlusion distance the test layouts are traced with.
#[uniffi::export]
pub fn bake_layout_occlusion_distance() -> f32 {
    layout::OCCLUSION_DISTANCE
}

/// What a tracer must find at the origin of the test layout `name`.
#[uniffi::export]
pub fn bake_layout_expected(name: String) -> Option<BakeExpected> {
    layout::layout(&name).map(|(_, expected)| BakeExpected {
        irradiance: expected.irradiance,
        occlusion: expected.occlusion,
    })
}

#[uniffi::export]
impl BakeScene {
    /// The test layout `name`, traced at the origin facing up.
    #[uniffi::constructor]
    pub fn layout(name: String) -> Result<Arc<Self>, EngineError> {
        let (scene, _) = layout::layout(&name).ok_or_else(|| EngineError::Load {
            message: format!("no bake layout {name}"),
        })?;
        Ok(Self::new(scene))
    }

    /// The hierarchy's nodes, 32 bytes each.
    pub fn node_bytes(&self) -> Vec<u8> {
        self.bvh.node_bytes()
    }

    /// The hierarchy's triangles, 48 bytes each.
    pub fn triangle_bytes(&self) -> Vec<u8> {
        self.bvh.triangle_bytes()
    }

    /// The materials, 32 bytes each.
    pub fn material_bytes(&self) -> Vec<u8> {
        self.scene.material_bytes()
    }

    /// The sky's lights, 32 bytes each.
    pub fn light_bytes(&self) -> Vec<u8> {
        self.scene.light_bytes()
    }

    /// The glowing triangles, 64 bytes each.
    pub fn emitter_bytes(&self) -> Vec<u8> {
        self.scene.emitter_bytes()
    }

    /// The sky dome's light: red, green and blue, linear.
    pub fn sky(&self) -> Vec<f32> {
        self.scene.sky.to_vec()
    }

    /// The triangles' bounds: least x, y and z, then greatest.
    pub fn bounds(&self) -> Vec<f32> {
        self.scene.bounds.concat()
    }

    /// The triangles in the hierarchy.
    pub fn triangle_count(&self) -> u32 {
        self.bvh.triangles.len() as u32
    }

    /// The CPU's light at `point` (x, y, z) on a surface facing `normal`,
    /// from `samples` paths seeded by `seed`.
    pub fn reference(
        &self,
        point: Vec<f32>,
        normal: Vec<f32>,
        samples: u32,
        seed: u64,
        occlusion_distance: f32,
    ) -> BakeSample {
        let three =
            |values: &[f32]| std::array::from_fn(|axis| values.get(axis).copied().unwrap_or(0.0));
        let tracer = Tracer {
            scene: &self.scene,
            bvh: &self.bvh,
            occlusion_distance,
        };
        let result = tracer.irradiance(
            three(&point),
            three(&normal),
            samples,
            &mut Random::new(seed),
        );
        sample(result)
    }

    /// The light at an irradiance probe's `point` from the axis `normal`,
    /// by the CPU's tracer, without the sky's light `sun` arriving straight
    /// from it: its `occlusion` is the share of the sun seen.
    pub fn reference_probe(
        &self,
        point: Vec<f32>,
        normal: Vec<f32>,
        samples: u32,
        seed: u64,
        sun: u32,
    ) -> BakeSample {
        let three =
            |values: &[f32]| std::array::from_fn(|axis| values.get(axis).copied().unwrap_or(0.0));
        let tracer = Tracer {
            scene: &self.scene,
            bvh: &self.bvh,
            occlusion_distance: 0.0,
        };
        sample(tracer.probe(
            three(&point),
            three(&normal),
            samples,
            sun as usize,
            &mut Random::new(seed),
        ))
    }

    /// The sky's brightest light (the sun), which the probes leave out: its
    /// index in `light_bytes`.
    pub fn sun_light(&self) -> Option<u32> {
        self.scene.sun_index().map(|index| index as u32)
    }
}

/// A tracer's result as the app reads it.
fn sample(result: opence_bake::trace::Irradiance) -> BakeSample {
    BakeSample {
        irradiance: result.irradiance.to_vec(),
        direction: result.direction.to_vec(),
        directionality: result.directionality,
        occlusion: result.occlusion,
    }
}

/// A level's BSP, ready to bake: the points the app's path tracer
/// traces, and how.
#[derive(uniffi::Object)]
pub struct LevelBake {
    plan: BakePlan,
    scene: Arc<BakeScene>,
    name: String,
    bsp: usize,
}

/// A baked lightmap page.
#[derive(Clone, Debug, PartialEq, uniffi::Record)]
pub struct BakedLightPage {
    /// The page, in the BSP's lightmaps (as a draw's `lightmap_page`).
    pub page: u32,
    /// Its width.
    pub width: u32,
    /// Its height.
    pub height: u32,
    /// Its texels' irradiance and occlusion: four half floats each, rows
    /// from the top.
    pub irradiance: Vec<u8>,
    /// Its texels' light direction (`0.5 + 0.5 * direction`) and
    /// directionality: four bytes each.
    pub directions: Vec<u8>,
}

/// A baked page's light against its stock page's (the mood check).
#[derive(Clone, Debug, PartialEq, uniffi::Record)]
pub struct BakeMood {
    /// The page, in the BSP's lightmaps.
    pub page: u32,
    /// The stock page's mean light: red, green, blue, as stored.
    pub stock: Vec<f32>,
    /// The baked page's, as the level shader reads it.
    pub baked: Vec<f32>,
    /// How many texels were compared.
    pub texels: u32,
    /// The baked brightness over the stock (each from the dark floor).
    pub ratio: f32,
    /// The most a channel's share of the colour differs by.
    pub hue_difference: f32,
    /// Whether it keeps within the limits every stock BSP keeps.
    pub within: bool,
}

/// A BSP's baked light against its stock lightmaps', its pages taken
/// together (the mood check).
#[derive(Clone, Debug, PartialEq, uniffi::Record)]
pub struct BakeBspMood {
    /// The stock pages' light, weighted by their texels.
    pub stock: Vec<f32>,
    /// The baked pages', as the level shader reads them.
    pub baked: Vec<f32>,
    /// The baked brightness over the stock (each from the dark floor).
    pub ratio: f32,
    /// The most a channel's share of the colour differs by.
    pub hue_difference: f32,
    /// The share of the texels on pages outside the limits.
    pub failing: f32,
    /// Whether the BSP keeps its mood.
    pub within: bool,
}

/// A BSP's baked light.
#[derive(uniffi::Object)]
pub struct BakedLight {
    light: BakedLighting,
}

/// Float4s from `bytes`, little-endian.
fn float4s(bytes: &[u8]) -> Vec<[f32; 4]> {
    bytes
        .as_chunks::<16>()
        .0
        .iter()
        .map(|chunk| {
            std::array::from_fn(|lane| {
                let at = lane * 4;
                f32::from_le_bytes([chunk[at], chunk[at + 1], chunk[at + 2], chunk[at + 3]])
            })
        })
        .collect()
}

/// Half floats as little-endian bytes.
fn half_bytes(halves: &[u16]) -> Vec<u8> {
    halves.iter().flat_map(|half| half.to_le_bytes()).collect()
}

#[uniffi::export]
impl LevelBake {
    /// The scene to trace.
    pub fn scene(&self) -> Arc<BakeScene> {
        Arc::clone(&self.scene)
    }

    /// The lightmap texels' points, 32 bytes each.
    pub fn lightmap_point_bytes(&self) -> Vec<u8> {
        self.plan.atlas.point_bytes()
    }

    /// The probes' points (six axes each), 32 bytes each.
    pub fn probe_point_bytes(&self) -> Vec<u8> {
        chart::point_bytes(&self.plan.probe_points())
    }

    /// The reflection probes' texels' points (each looking along its
    /// normal), 32 bytes each.
    pub fn reflection_point_bytes(&self) -> Vec<u8> {
        chart::point_bytes(&self.plan.reflection_points())
    }

    /// Paths for each lightmap texel.
    pub fn lightmap_samples(&self) -> u32 {
        self.plan.settings.lightmap_samples
    }

    /// Paths for each probe's axis.
    pub fn probe_samples(&self) -> u32 {
        self.plan.settings.probe_samples
    }

    /// Paths for each reflection texel.
    pub fn reflection_samples(&self) -> u32 {
        self.plan.settings.reflection_samples
    }

    /// How far an occluder counts for the ambient occlusion.
    pub fn occlusion_distance(&self) -> f32 {
        self.plan.settings.occlusion_distance
    }

    /// The cache file's name: the map's, the BSP's and the bake's key.
    pub fn cache_file_name(&self) -> String {
        BakedLighting::file_name(&self.name, self.bsp, self.plan.key)
    }

    /// The baker's version.
    pub fn version(&self) -> u32 {
        BAKE_VERSION
    }

    /// The light from the tracer's results, each a float4 (16 bytes): the
    /// lightmap texels' irradiance and their directions, the probes'
    /// irradiance and the reflection texels' radiance.
    pub fn finish(
        &self,
        irradiance: Vec<u8>,
        directions: Vec<u8>,
        probes: Vec<u8>,
        radiance: Vec<u8>,
    ) -> Arc<BakedLight> {
        Arc::new(BakedLight {
            light: self.plan.finish(
                &float4s(&irradiance),
                &float4s(&directions),
                &float4s(&probes),
                &float4s(&radiance),
            ),
        })
    }

    /// Each baked page of `light` against its stock page, for the pages
    /// that have one.
    pub fn moods(&self, light: Arc<BakedLight>) -> Vec<BakeMood> {
        self.plan
            .moods(&light.light)
            .iter()
            .map(|mood| BakeMood {
                page: mood.page,
                stock: mood.stock.to_vec(),
                baked: mood.baked.to_vec(),
                texels: mood.texels,
                ratio: mood.ratio(),
                hue_difference: mood.hue_difference(),
                within: mood.within(&opence_bake::mood::LIMITS),
            })
            .collect()
    }

    /// The BSP's pages of `light` taken together against its stock pages
    /// (None if no page has a stock page).
    pub fn bsp_mood(&self, light: Arc<BakedLight>) -> Option<BakeBspMood> {
        let limits = opence_bake::mood::LIMITS;
        BspMood::of(&self.plan.moods(&light.light), &limits).map(|mood| BakeBspMood {
            stock: mood.whole.stock.to_vec(),
            baked: mood.whole.baked.to_vec(),
            ratio: mood.whole.ratio(),
            hue_difference: mood.whole.hue_difference(),
            failing: mood.failing,
            within: mood.within(&limits),
        })
    }

    /// The BSP, in the level's.
    pub fn bsp(&self) -> u32 {
        self.bsp as u32
    }

    /// The light kept in a cache file, if it is whole and baked for this
    /// plan.
    pub fn read(&self, bytes: Vec<u8>) -> Option<Arc<BakedLight>> {
        BakedLighting::decode(&bytes)
            .ok()
            .filter(|light| light.key == self.plan.key)
            .map(|light| Arc::new(BakedLight { light }))
    }
}

#[uniffi::export]
impl BakedLight {
    /// The cache file.
    pub fn encode(&self) -> Vec<u8> {
        self.light.encode()
    }

    /// The scale the light was given to keep the stock pages' brightness.
    pub fn exposure(&self) -> f32 {
        self.light.exposure
    }

    /// The baked lightmap pages.
    pub fn pages(&self) -> Vec<BakedLightPage> {
        self.light
            .pages
            .iter()
            .map(|page| BakedLightPage {
                page: page.page,
                width: page.width,
                height: page.height,
                irradiance: half_bytes(&page.irradiance),
                directions: page.directions.clone(),
            })
            .collect()
    }

    /// The probe grid's first probe: x, y and z.
    pub fn probe_origin(&self) -> Vec<f32> {
        self.light.grid.origin.to_vec()
    }

    /// The distance between probes.
    pub fn probe_spacing(&self) -> f32 {
        self.light.grid.spacing
    }

    /// How many probes each way.
    pub fn probe_counts(&self) -> Vec<u32> {
        self.light.grid.counts.to_vec()
    }

    /// The probes' light from the axis `axis` (+x, -x, +y, -y, +z, -z):
    /// four half floats a probe, x fastest, as a 3D texture's texels.
    pub fn probe_bytes(&self, axis: u32) -> Vec<u8> {
        let size = self.light.grid.len() * 4;
        let start = axis as usize * size;
        half_bytes(
            self.light
                .probes
                .get(start..start + size)
                .unwrap_or_default(),
        )
    }

    /// The direction towards the sun the probes leave out (empty without
    /// one).
    pub fn sun_towards(&self) -> Vec<f32> {
        self.light
            .sun
            .map(|sun| sun.towards.to_vec())
            .unwrap_or_default()
    }

    /// The sun's light on a surface facing it, exposed as the probes'
    /// light is (empty without one).
    pub fn sun_irradiance(&self) -> Vec<f32> {
        self.light
            .sun
            .map(|sun| sun.irradiance.to_vec())
            .unwrap_or_default()
    }

    /// The probes' light at `point` on a surface facing `normal`: red,
    /// green and blue (without the sun's direct light), and the share of
    /// the sun seen. The renderer's model shader samples the same.
    pub fn light_at(&self, point: Vec<f32>, normal: Vec<f32>) -> Vec<f32> {
        let three =
            |values: &[f32]| std::array::from_fn(|axis| values.get(axis).copied().unwrap_or(0.0));
        opence_bake::probe::light_at(
            &self.light.grid,
            &self.light.probes,
            three(&point),
            three(&normal),
        )
        .to_vec()
    }

    /// The reflection probe nearest `point` (an index into
    /// `reflection_positions`' probes), if there are any.
    pub fn nearest_reflection(&self, point: Vec<f32>) -> Option<u32> {
        let three: [f32; 3] = std::array::from_fn(|axis| point.get(axis).copied().unwrap_or(0.0));
        opence_bake::probe::nearest_reflection(&self.light.reflections, three)
            .map(|index| index as u32)
    }

    /// The reflection probes' positions: x, y and z each.
    pub fn reflection_positions(&self) -> Vec<f32> {
        self.light.reflections.iter().flatten().copied().collect()
    }

    /// A reflection probe's faces' size, each way.
    pub fn reflection_size(&self) -> u32 {
        opence_bake::probe::REFLECTION_SIZE
    }

    /// The reflection probes' faces: four half floats a texel, probe by
    /// probe, face by face (+x, -x, +y, -y, +z, -z), rows from the top.
    pub fn reflection_bytes(&self) -> Vec<u8> {
        half_bytes(&self.light.radiance)
    }
}

#[uniffi::export]
impl LevelGeometry {
    /// The scene the bake lights for this level's BSP.
    pub fn bake_scene(&self) -> Result<Arc<BakeScene>, EngineError> {
        let bsp = self.bsp;
        Scene::of(&self.content, bsp)
            .map(BakeScene::new)
            .ok_or(EngineError::NoBsp { index: bsp as u32 })
    }

    /// The bake of this level's BSP, at the default settings; or, when
    /// `quick`, at few samples (for tests).
    pub fn bake_plan(&self, quick: bool) -> Result<Arc<LevelBake>, EngineError> {
        self.bake_plan_of(self.bsp, quick)
    }

    /// The bakes of every BSP of the level, as `bake_plan`'s.
    pub fn bake_plans(&self, quick: bool) -> Vec<Arc<LevelBake>> {
        (0..self.content.structure_bsps.len())
            .filter_map(|bsp| self.bake_plan_of(bsp, quick).ok())
            .collect()
    }
}

impl LevelGeometry {
    /// The bake of the level's BSP `bsp`.
    fn bake_plan_of(&self, bsp: usize, quick: bool) -> Result<Arc<LevelBake>, EngineError> {
        let settings = if quick {
            BakeSettings {
                lightmap_samples: 4,
                probe_samples: 4,
                reflection_samples: 1,
                ..BakeSettings::default()
            }
        } else {
            BakeSettings::default()
        };
        let plan = BakePlan::of(&self.content, bsp, settings)
            .ok_or(EngineError::NoBsp { index: bsp as u32 })?;
        let scene = BakeScene::new(plan.scene.clone());
        Ok(Arc::new(LevelBake {
            plan,
            scene,
            name: self.name.clone(),
            bsp,
        }))
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn a_layout_is_laid_out_and_traced() {
        assert_eq!(bake_layout_names().len(), layout::NAMES.len());
        let scene = BakeScene::layout("furnace".into()).unwrap_or_else(|error| panic!("{error}"));
        assert_eq!(scene.triangle_count(), 12);
        assert_eq!(scene.triangle_bytes().len(), 12 * 48);
        assert_eq!(scene.node_bytes().len() % 32, 0);
        assert_eq!(scene.material_bytes().len(), 32);
        assert!(scene.light_bytes().is_empty());
        assert_eq!(scene.sky(), vec![100.0; 3]);
        assert_eq!(scene.bounds(), vec![-1.0, -1.0, -1.0, 1.0, 1.0, 1.0]);
        let expected =
            bake_layout_expected("furnace".into()).unwrap_or_else(|| panic!("no furnace"));
        let sample = scene.reference(
            vec![0.0; 3],
            vec![0.0, 0.0, 1.0],
            1024,
            1,
            bake_layout_occlusion_distance(),
        );
        assert!((sample.irradiance[0] - expected.irradiance).abs() < expected.irradiance * 0.06);
        assert_eq!(sample.occlusion, expected.occlusion);
        assert!(BakeScene::layout("nowhere".into()).is_err());
        assert_eq!(bake_layout_expected("nowhere".into()), None);
    }

    #[test]
    fn a_level_s_scene_is_its_bsp_s() {
        let geometry =
            LevelGeometry::devmap(None, String::new()).unwrap_or_else(|error| panic!("{error}"));
        let scene = geometry
            .bake_scene()
            .unwrap_or_else(|error| panic!("{error}"));
        assert!(scene.triangle_count() > 0);
        assert!(!scene.light_bytes().is_empty());
    }

    #[test]
    fn a_level_is_baked_and_kept() {
        let geometry =
            LevelGeometry::devmap(None, String::new()).unwrap_or_else(|error| panic!("{error}"));
        let bake = geometry
            .bake_plan(true)
            .unwrap_or_else(|error| panic!("{error}"));
        assert_eq!(bake.lightmap_samples(), 4);
        assert!(bake.probe_samples() > 0 && bake.reflection_samples() > 0);
        assert!(bake.occlusion_distance() > 0.0);
        assert_eq!(bake.version(), BAKE_VERSION);
        assert!(bake.scene().triangle_count() > 0);
        // (the devmap has no stock pages: its light is the probes')
        assert!(bake.lightmap_point_bytes().is_empty());
        let probes = bake.probe_point_bytes();
        let reflections = bake.reflection_point_bytes();
        assert_eq!(probes.len() % (32 * 6), 0);
        assert!(!reflections.is_empty());
        let full = geometry
            .bake_plan(false)
            .unwrap_or_else(|error| panic!("{error}"));
        assert_ne!(full.cache_file_name(), bake.cache_file_name());
        assert!(bake.cache_file_name().starts_with("devmap.bsp0."));
        let lit = |count: usize, value: f32| -> Vec<u8> {
            (0..count * 4).flat_map(|_| value.to_le_bytes()).collect()
        };
        let light = bake.finish(
            Vec::new(),
            Vec::new(),
            lit(probes.len() / 32, 1.0),
            lit(reflections.len() / 32, 2.0),
        );
        assert!(light.pages().is_empty());
        let counts = light.probe_counts();
        let probe_count = counts.iter().product::<u32>() as usize;
        assert_eq!(light.probe_bytes(0).len(), probe_count * 8);
        assert_eq!(light.probe_bytes(5)[..2], [0x00, 0x3c]);
        assert!(light.probe_bytes(6).is_empty());
        assert_eq!(light.probe_origin().len(), 3);
        assert!(light.probe_spacing() >= 1.0);
        // (every probe lit 1 from every axis, its sun seen whole)
        let origin = light.probe_origin();
        assert_eq!(
            light.light_at(origin.clone(), vec![0.0, 0.0, 1.0]),
            [1.0; 4]
        );
        let sun = bake.scene().sun_light().unwrap_or_else(|| panic!("no sun"));
        assert_eq!(light.sun_towards().len(), 3);
        assert_eq!(light.sun_irradiance().len(), 3);
        let reference =
            bake.scene()
                .reference_probe(origin.clone(), vec![0.0, 0.0, 1.0], 8, 1, sun);
        assert!((0.0..=1.0).contains(&reference.occlusion));
        let positions = light.reflection_positions();
        let first = positions[..3].to_vec();
        assert_eq!(light.nearest_reflection(first), Some(0));
        let size = light.reflection_size() as usize;
        assert_eq!(
            light.reflection_bytes().len(),
            positions.len() / 3 * 6 * size * size * 8
        );
        let file = light.encode();
        let back = bake
            .read(file.clone())
            .unwrap_or_else(|| panic!("not read back"));
        assert_eq!(back.encode(), file);
        // another plan's file is not this one's
        assert!(full.read(file).is_none());
        assert!(bake.read(vec![1, 2, 3]).is_none());
        // the devmap has no stock pages to compare with, and one BSP
        assert!(bake.moods(Arc::clone(&light)).is_empty());
        assert!(bake.bsp_mood(Arc::clone(&light)).is_none());
        assert_eq!(bake.bsp(), 0);
        let plans = geometry.bake_plans(true);
        assert_eq!(plans.len(), 1);
        assert_eq!(plans[0].cache_file_name(), bake.cache_file_name());
    }
}
