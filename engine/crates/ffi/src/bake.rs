//! The bake's scene, for the app's path tracer (OpenCEEngineRender's
//! PathTracer): a BSP's hierarchy, materials and lights as the GPU reads
//! them, and the CPU's reference tracer to check the GPU's against.

use std::sync::Arc;

use opence_bake::{layout, Bvh, Random, Scene, Tracer};

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
        BakeSample {
            irradiance: result.irradiance.to_vec(),
            direction: result.direction.to_vec(),
            occlusion: result.occlusion,
        }
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
}
