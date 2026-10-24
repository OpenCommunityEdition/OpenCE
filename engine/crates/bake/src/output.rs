//! What a bake makes and keeps: the lightmap pages, the irradiance probes
//! and the reflection probes, and the cache file they are kept in.
//!
//! A [`BakePlan`] holds the points to trace (the pages' texels, the
//! probes' axes and the reflection probes' texels); the tracer's results
//! for them make a [`BakedLighting`], which is written beside the level's
//! imported pack. The file is named by a key of everything the light
//! depends on, the importer's and the baker's versions among them, so a
//! level whose content or baker has changed is baked again.
//!
//! The light is kept as half floats: a page's texel holds its irradiance
//! and occlusion, and a second texel the direction its light comes from
//! (by eight bits, as `0.5 + 0.5 * direction`) and how much of it does.
//!
//! The probes light what moves, which the sun's shadow maps shadow, so
//! they leave out the sun's light arriving straight at them (its light off
//! the level is kept) and hold instead how much of the sun each sees; the
//! sun is kept beside them, for the renderer to add.

use opence_content::level::Level;
use opence_content::pack::FORMAT_VERSION;

use crate::chart::{Atlas, BakePoint, Chart};
use crate::edge::EdgeSteps;
use crate::filter;
use crate::mood::{self, PageMood, StockPage};
use crate::probe::{self, ProbeGrid, REFLECTION_SIZE};
use crate::scene::Scene;
use crate::seam::{self, Seam, SeamStep};

/// The baker's version: raised with any change to what it makes.
pub const BAKE_VERSION: u32 = 8;
/// The cache file's first eight bytes.
pub const MAGIC: [u8; 8] = *b"OCEBAKE\0";

/// How a level is baked.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct BakeSettings {
    /// Paths for each lightmap texel.
    pub lightmap_samples: u32,
    /// Paths for each probe's axis.
    pub probe_samples: u32,
    /// Paths for each reflection probe's texel.
    pub reflection_samples: u32,
    /// How far an occluder counts for the ambient occlusion.
    pub occlusion_distance: f32,
}

impl Default for BakeSettings {
    fn default() -> Self {
        Self {
            lightmap_samples: 64,
            probe_samples: 256,
            reflection_samples: 16,
            occlusion_distance: 2.0,
        }
    }
}

/// A level's BSP, ready to bake: its scene and the points to trace.
#[derive(Clone, Debug)]
pub struct BakePlan {
    /// The scene.
    pub scene: Scene,
    /// The lightmap pages and their texels' points.
    pub atlas: Atlas,
    /// The irradiance probes.
    pub grid: ProbeGrid,
    /// Where the reflection probes are.
    pub reflections: Vec<[f32; 3]>,
    /// How it is baked.
    pub settings: BakeSettings,
    /// The cache key.
    pub key: u64,
    /// Each chart's stock page's mean light under it, if it has one the
    /// bake can read (for the mood check).
    pub stock: Vec<Option<[f32; 3]>>,
    /// The lightmap seams, which the bake stitches ([`seam::stitch`]).
    pub seams: Vec<Seam>,
    /// Each seam's step on the stock pages, if they can be read (for the
    /// seam check).
    pub stock_seams: Vec<Option<f32>>,
    /// The steps between neighbouring texels on the stock pages (for the
    /// edge check).
    pub stock_edges: EdgeSteps,
}

/// A baked page: its texels' light, row by row from the top.
#[derive(Clone, Debug, PartialEq)]
pub struct BakedPage {
    /// The page, in the BSP's lightmaps.
    pub page: u32,
    /// Its width.
    pub width: u32,
    /// Its height.
    pub height: u32,
    /// Each texel's irradiance and occlusion: four half floats.
    pub irradiance: Vec<u16>,
    /// Each texel's light direction and directionality: four bytes.
    pub directions: Vec<u8>,
}

/// The sun of a BSP's baked light.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct BakedSun {
    /// The direction towards it (unit).
    pub towards: [f32; 3],
    /// Its light on a surface facing it, scaled by the bake's exposure as
    /// the probes' light is.
    pub irradiance: [f32; 3],
}

/// A BSP's baked light.
#[derive(Clone, Debug, PartialEq)]
pub struct BakedLighting {
    /// The key it was baked for.
    pub key: u64,
    /// The scale its light was given to keep the stock pages' brightness
    /// ([`BakePlan::exposure`]).
    pub exposure: f32,
    /// The lightmap pages.
    pub pages: Vec<BakedPage>,
    /// The irradiance probes' grid.
    pub grid: ProbeGrid,
    /// The probes' light from each axis, axis by axis (in
    /// [`probe::AXES`]'s order), then probe by probe: four half floats
    /// each, the irradiance without the sun's direct light and the share
    /// of the sun seen ([`crate::trace::Tracer::probe`]).
    pub probes: Vec<u16>,
    /// The sun the probes leave out, if the sky has one.
    pub sun: Option<BakedSun>,
    /// Where the reflection probes are.
    pub reflections: Vec<[f32; 3]>,
    /// Their faces' radiance: probe by probe, face by face, texel by
    /// texel, four half floats each (the last 1).
    pub radiance: Vec<u16>,
}

impl BakePlan {
    /// The plan for `level`'s BSP `bsp`. None if there is no such BSP.
    pub fn of(level: &Level, bsp: usize, settings: BakeSettings) -> Option<Self> {
        let scene = Scene::of(level, bsp)?;
        let atlas = Atlas::of(level, bsp);
        let grid = ProbeGrid::covering(scene.bounds);
        let reflections = probe::reflection_probes(&scene);
        let key = key(&scene, &atlas, &grid, &settings);
        let pages: Vec<Option<StockPage>> = atlas
            .charts
            .iter()
            .map(|chart| StockPage::of(level, bsp, chart.page as usize))
            .collect();
        let stock = atlas
            .charts
            .iter()
            .zip(&pages)
            .map(|(chart, page)| mood::stock_mean(chart, page.as_ref()?))
            .collect();
        let seams = Seam::find(&level.structure_bsps[bsp], &atlas);
        let stock_seams = seam::stock_steps(&atlas, &seams, &pages);
        let stock_edges = EdgeSteps::stock(&atlas, &pages);
        Some(Self {
            scene,
            atlas,
            grid,
            reflections,
            settings,
            key,
            stock,
            seams,
            stock_seams,
            stock_edges,
        })
    }

    /// The steps between neighbouring texels on `light`'s pages and on the
    /// stock pages.
    pub fn edges(&self, light: &BakedLighting) -> (EdgeSteps, EdgeSteps) {
        (EdgeSteps::baked(&self.atlas, &light.pages), self.stock_edges)
    }

    /// The step in light across each seam on `light`'s pages and on the
    /// stock pages.
    pub fn seams(&self, light: &BakedLighting) -> Vec<SeamStep> {
        SeamStep::measure(&self.atlas, &self.seams, &light.pages, &self.stock_seams)
    }

    /// Each baked page of `light` against its stock page, for the pages
    /// with a stock page to compare with.
    pub fn moods(&self, light: &BakedLighting) -> Vec<PageMood> {
        let mut moods = Vec::new();
        for ((chart, stock), page) in self.atlas.charts.iter().zip(&self.stock).zip(&light.pages) {
            if let (Some(stock), Some(baked)) = (stock, mood::baked_light(chart, page)) {
                moods.push(PageMood {
                    page: chart.page,
                    stock: *stock,
                    baked,
                    texels: mood::covered(chart),
                });
            }
        }
        moods
    }

    /// The scale [`Self::finish`] gives the tracer's light, so that the
    /// baked pages keep their stock pages' brightness
    /// ([`mood::fitted_exposure`]).
    pub fn exposure(&self, irradiance: &[[f32; 4]]) -> f32 {
        mood::fitted_exposure(&self.atlas.charts, &self.stock, irradiance)
    }

    /// The probes' points, six to a probe.
    pub fn probe_points(&self) -> Vec<BakePoint> {
        self.grid.points()
    }

    /// The reflection probes' texels' points.
    pub fn reflection_points(&self) -> Vec<BakePoint> {
        probe::reflection_points(&self.reflections)
    }

    /// The baked light from the tracer's results: for the atlas's points,
    /// each its irradiance and occlusion and its direction and
    /// directionality; for the probes' points, each its irradiance and
    /// occlusion; for the reflection points, each its radiance. The atlas's
    /// fireflies are held down ([`filter::hold_fireflies`]) and its seams
    /// stitched ([`seam::stitch`]), then each is scaled by the BSP's
    /// [`Self::exposure`]. A result that is missing is dark.
    pub fn finish(
        &self,
        irradiance: &[[f32; 4]],
        directions: &[[f32; 4]],
        probes: &[[f32; 4]],
        radiance: &[[f32; 4]],
    ) -> BakedLighting {
        let held = filter::hold_fireflies(&self.atlas, irradiance);
        let held = seam::stitch(&self.atlas, &self.seams, &held);
        let exposure = self.exposure(&held);
        let exposed = |light: &[[f32; 4]]| -> Vec<[f32; 4]> {
            light
                .iter()
                .map(|&[r, g, b, a]| [r * exposure, g * exposure, b * exposure, a])
                .collect()
        };
        let (irradiance, probes, radiance) = (
            &exposed(&held)[..],
            &exposed(probes)[..],
            &exposed(radiance)[..],
        );
        let pages = self
            .atlas
            .charts
            .iter()
            .map(|chart| bake_page(chart, irradiance, directions))
            .collect();
        let count = self.grid.len();
        let mut probe_light = Vec::with_capacity(count * 6 * 4);
        for axis in 0..probe::AXES.len() {
            for index in 0..count {
                let light = probes.get(index * 6 + axis).copied().unwrap_or_default();
                probe_light.extend(light.map(half));
            }
        }
        let texels = self.reflections.len() * 6 * (REFLECTION_SIZE * REFLECTION_SIZE) as usize;
        let mut radiance_light = Vec::with_capacity(texels * 4);
        for index in 0..texels {
            let [r, g, b, _] = radiance.get(index).copied().unwrap_or_default();
            radiance_light.extend([r, g, b, 1.0].map(half));
        }
        BakedLighting {
            key: self.key,
            exposure,
            pages,
            grid: self.grid,
            probes: probe_light,
            sun: self.scene.sun().map(|sun| BakedSun {
                towards: sun.towards,
                irradiance: sun.irradiance.map(|channel| channel * exposure),
            }),
            reflections: self.reflections.clone(),
            radiance: radiance_light,
        }
    }
}

/// A chart's texels' light.
fn bake_page(chart: &Chart, irradiance: &[[f32; 4]], directions: &[[f32; 4]]) -> BakedPage {
    let light = chart.fill(irradiance, [0.0; 4]);
    let direction = chart.fill(directions, [0.0; 4]);
    BakedPage {
        page: chart.page,
        width: chart.width,
        height: chart.height,
        irradiance: light.iter().flat_map(|texel| texel.map(half)).collect(),
        directions: direction
            .iter()
            .flat_map(|[x, y, z, w]| [0.5 + 0.5 * x, 0.5 + 0.5 * y, 0.5 + 0.5 * z, *w].map(unorm8))
            .collect(),
    }
}

/// The cache key of a plan: an FNV-1a hash of the importer's and baker's
/// versions, the settings, the scene, the pages' sizes and the grid.
pub fn key(scene: &Scene, atlas: &Atlas, grid: &ProbeGrid, settings: &BakeSettings) -> u64 {
    let mut hash = Fnv::default();
    hash.u32(FORMAT_VERSION);
    hash.u32(BAKE_VERSION);
    for value in [
        settings.lightmap_samples,
        settings.probe_samples,
        settings.reflection_samples,
        settings.occlusion_distance.to_bits(),
    ] {
        hash.u32(value);
    }
    for triangle in &scene.triangles {
        triangle
            .positions
            .iter()
            .flatten()
            .for_each(|v| hash.f32(*v));
        hash.u32(triangle.material);
    }
    hash.bytes(&scene.material_bytes());
    hash.bytes(&scene.light_bytes());
    scene
        .sky
        .iter()
        .chain(&scene.indoor)
        .for_each(|v| hash.f32(*v));
    for chart in &atlas.charts {
        [chart.page, chart.width, chart.height]
            .into_iter()
            .for_each(|v| hash.u32(v));
    }
    hash.u32(atlas.points.len() as u32);
    grid.origin
        .iter()
        .chain([&grid.spacing])
        .for_each(|v| hash.f32(*v));
    grid.counts.into_iter().for_each(|v| hash.u32(v));
    hash.0
}

/// A 64-bit FNV-1a hash.
struct Fnv(u64);

impl Default for Fnv {
    fn default() -> Self {
        Self(0xcbf2_9ce4_8422_2325)
    }
}

impl Fnv {
    fn bytes(&mut self, bytes: &[u8]) {
        for byte in bytes {
            self.0 ^= u64::from(*byte);
            self.0 = self.0.wrapping_mul(0x0000_0100_0000_01b3);
        }
    }

    fn u32(&mut self, value: u32) {
        self.bytes(&value.to_le_bytes());
    }

    fn f32(&mut self, value: f32) {
        self.u32(value.to_bits());
    }
}

/// `value` as a half float, rounded to the nearest (ties to even); too
/// large, the largest half; not a number, zero.
pub fn half(value: f32) -> u16 {
    if value.is_nan() {
        return 0;
    }
    let bits = value.to_bits();
    let sign = ((bits >> 16) & 0x8000) as u16;
    let magnitude = value.abs();
    if magnitude >= 65520.0 {
        return sign | 0x7bff;
    }
    let exponent = ((bits >> 23) & 0xff) as i32 - 127;
    let mantissa = bits & 0x007f_ffff;
    if exponent < -24 {
        return sign;
    }
    let (shift, base) = if exponent < -14 {
        // a subnormal half: the implicit one becomes explicit
        (
            (-exponent - 14 + 13) as u32,
            (mantissa | 0x0080_0000) as u64,
        )
    } else {
        (13, u64::from(mantissa) | (((exponent + 15) as u64) << 23))
    };
    let half_up = 1u64 << (shift - 1);
    let rest = base & ((1u64 << shift) - 1);
    let mut rounded = base >> shift;
    if rest > half_up || (rest == half_up && rounded & 1 == 1) {
        rounded += 1;
    }
    sign | rounded as u16
}

/// A half float's value.
pub fn from_half(bits: u16) -> f32 {
    let sign = if bits & 0x8000 != 0 { -1.0 } else { 1.0 };
    let exponent = i32::from((bits >> 10) & 0x1f);
    let mantissa = f32::from(bits & 0x3ff);
    sign * match exponent {
        0 => mantissa * libm::exp2f(-24.0),
        31 => f32::INFINITY,
        _ => (1.0 + mantissa / 1024.0) * libm::exp2f((exponent - 15) as f32),
    }
}

/// `value` (0 to 1) in eight bits.
fn unorm8(value: f32) -> u8 {
    if value.is_finite() {
        libm::roundf(value.clamp(0.0, 1.0) * 255.0) as u8
    } else {
        0
    }
}

/// Why a cache file could not be read.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum CacheError {
    /// It is not a bake.
    Magic,
    /// It is another baker's.
    Version(u32),
    /// It ends early, or its counts do not fit.
    Truncated,
}

impl core::fmt::Display for CacheError {
    fn fmt(&self, formatter: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self {
            Self::Magic => write!(formatter, "not a baked light file"),
            Self::Version(version) => {
                write!(formatter, "a bake of version {version}, not {BAKE_VERSION}")
            }
            Self::Truncated => write!(formatter, "a baked light file that ends early"),
        }
    }
}

impl std::error::Error for CacheError {}

impl BakedLighting {
    /// The file it is kept in.
    pub fn encode(&self) -> Vec<u8> {
        let mut bytes = Vec::new();
        bytes.extend_from_slice(&MAGIC);
        put_u32(&mut bytes, BAKE_VERSION);
        bytes.extend_from_slice(&self.key.to_le_bytes());
        bytes.extend_from_slice(&self.exposure.to_le_bytes());
        put_u32(&mut bytes, self.pages.len() as u32);
        for page in &self.pages {
            for value in [page.page, page.width, page.height] {
                put_u32(&mut bytes, value);
            }
            put_halves(&mut bytes, &page.irradiance);
            put_u32(&mut bytes, page.directions.len() as u32);
            bytes.extend_from_slice(&page.directions);
        }
        let grid = &self.grid;
        for value in grid.origin.iter().chain([&grid.spacing]) {
            bytes.extend_from_slice(&value.to_le_bytes());
        }
        for count in grid.counts {
            put_u32(&mut bytes, count);
        }
        put_halves(&mut bytes, &self.probes);
        put_u32(&mut bytes, u32::from(self.sun.is_some()));
        if let Some(sun) = self.sun {
            for value in sun.towards.iter().chain(&sun.irradiance) {
                bytes.extend_from_slice(&value.to_le_bytes());
            }
        }
        put_u32(&mut bytes, self.reflections.len() as u32);
        for value in self.reflections.iter().flatten() {
            bytes.extend_from_slice(&value.to_le_bytes());
        }
        put_halves(&mut bytes, &self.radiance);
        bytes
    }

    /// The light kept in `bytes`, checked: its sizes must agree.
    pub fn decode(bytes: &[u8]) -> Result<Self, CacheError> {
        let mut reader = Reader { bytes, at: 0 };
        if reader.take(8)? != MAGIC {
            return Err(CacheError::Magic);
        }
        let version = reader.u32()?;
        if version != BAKE_VERSION {
            return Err(CacheError::Version(version));
        }
        let key = u64::from_le_bytes(
            reader
                .take(8)?
                .try_into()
                .map_err(|_| CacheError::Truncated)?,
        );
        let exposure = reader.f32()?;
        let page_count = reader.u32()?;
        let mut pages = Vec::new();
        for _ in 0..page_count {
            let (page, width, height) = (reader.u32()?, reader.u32()?, reader.u32()?);
            let texels = (width as usize)
                .checked_mul(height as usize)
                .ok_or(CacheError::Truncated)?;
            let irradiance = reader.halves()?;
            let length = reader.u32()? as usize;
            let directions = reader.take(length)?.to_vec();
            if irradiance.len() != texels * 4 || directions.len() != texels * 4 {
                return Err(CacheError::Truncated);
            }
            pages.push(BakedPage {
                page,
                width,
                height,
                irradiance,
                directions,
            });
        }
        let origin = [reader.f32()?, reader.f32()?, reader.f32()?];
        let spacing = reader.f32()?;
        let counts = [reader.u32()?, reader.u32()?, reader.u32()?];
        let grid = ProbeGrid {
            origin,
            spacing,
            counts,
        };
        let probes = reader.halves()?;
        if counts.iter().any(|count| *count > probe::MAX_PROBES)
            || probes.len() != grid.len() * 6 * 4
        {
            return Err(CacheError::Truncated);
        }
        let sun = match reader.u32()? {
            0 => None,
            1 => Some(BakedSun {
                towards: [reader.f32()?, reader.f32()?, reader.f32()?],
                irradiance: [reader.f32()?, reader.f32()?, reader.f32()?],
            }),
            _ => return Err(CacheError::Truncated),
        };
        let reflection_count = reader.u32()? as usize;
        if reflection_count > probe::MAX_REFLECTIONS {
            return Err(CacheError::Truncated);
        }
        let mut reflections = Vec::with_capacity(reflection_count);
        for _ in 0..reflection_count {
            reflections.push([reader.f32()?, reader.f32()?, reader.f32()?]);
        }
        let radiance = reader.halves()?;
        let texels = reflection_count * 6 * (REFLECTION_SIZE * REFLECTION_SIZE) as usize;
        if radiance.len() != texels * 4 || reader.at != bytes.len() {
            return Err(CacheError::Truncated);
        }
        Ok(Self {
            key,
            exposure,
            pages,
            grid,
            probes,
            sun,
            reflections,
            radiance,
        })
    }

    /// The cache file's name for `level`'s BSP `bsp` baked by `key`.
    pub fn file_name(name: &str, bsp: usize, key: u64) -> String {
        format!("{name}.bsp{bsp}.{key:016x}.bake")
    }
}

fn put_u32(bytes: &mut Vec<u8>, value: u32) {
    bytes.extend_from_slice(&value.to_le_bytes());
}

fn put_halves(bytes: &mut Vec<u8>, halves: &[u16]) {
    put_u32(bytes, halves.len() as u32);
    for value in halves {
        bytes.extend_from_slice(&value.to_le_bytes());
    }
}

/// A bounds-checked reader of a cache file.
struct Reader<'a> {
    bytes: &'a [u8],
    at: usize,
}

impl<'a> Reader<'a> {
    fn take(&mut self, count: usize) -> Result<&'a [u8], CacheError> {
        let end = self.at.checked_add(count).ok_or(CacheError::Truncated)?;
        let taken = self.bytes.get(self.at..end).ok_or(CacheError::Truncated)?;
        self.at = end;
        Ok(taken)
    }

    fn u32(&mut self) -> Result<u32, CacheError> {
        let word = self.take(4)?;
        Ok(u32::from_le_bytes([word[0], word[1], word[2], word[3]]))
    }

    fn f32(&mut self) -> Result<f32, CacheError> {
        self.u32().map(f32::from_bits)
    }

    fn halves(&mut self) -> Result<Vec<u16>, CacheError> {
        let count = self.u32()? as usize;
        let bytes = self.take(count.checked_mul(2).ok_or(CacheError::Truncated)?)?;
        Ok(bytes
            .as_chunks::<2>()
            .0
            .iter()
            .map(|pair| u16::from_le_bytes(*pair))
            .collect())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn devmap_plan(settings: BakeSettings) -> BakePlan {
        let level = opence_content::devmap::level(None);
        BakePlan::of(&level, 0, settings).unwrap_or_else(|| panic!("no BSP"))
    }

    #[test]
    fn pages_are_compared_with_their_stock_pages() {
        let mut plan = devmap_plan(BakeSettings::default());
        assert!(plan.stock.is_empty());
        let light = plan.finish(&[], &[], &[], &[]);
        assert!(plan.moods(&light).is_empty());
        // two charts of one texel each, the second with no stock page
        for page in 0..2 {
            plan.atlas.charts.push(Chart {
                page,
                width: 1,
                height: 1,
                texels: vec![page],
            });
        }
        plan.stock = vec![Some([0.25; 3]), None];
        let pi = std::f32::consts::PI;
        let light = plan.finish(&[[pi, pi, pi, 1.0], [pi; 4]], &[], &[], &[]);
        let moods = plan.moods(&light);
        assert_eq!(moods.len(), 1);
        assert_eq!(
            (moods[0].page, moods[0].stock, moods[0].texels),
            (0, [0.25; 3], 1)
        );
        // (the light of pi shows as 1, so the exposure brings it down to
        // the stock page's quarter, and the light is kept so exposed)
        assert!((moods[0].baked[0] - 0.25).abs() < 1e-3, "{:?}", moods[0]);
        assert!(
            light.exposure > 0.0 && light.exposure < 1.0,
            "{}",
            light.exposure
        );
        let back =
            BakedLighting::decode(&light.encode()).unwrap_or_else(|error| panic!("{error:?}"));
        assert_eq!(back.exposure, light.exposure);
    }

    #[test]
    fn halves_round_to_the_nearest() {
        for value in [0.0, 1.0, -2.5, 0.333_333, 1e-5, 6.0e-8, 1000.0, 65504.0] {
            let back = from_half(half(value));
            assert!(
                (back - value).abs() <= value.abs() * 1e-3 + 6e-8,
                "{value}: {back}"
            );
        }
        assert_eq!(half(1.0), 0x3c00);
        assert_eq!(half(-2.0), 0xc000);
        assert_eq!(half(1e9), 0x7bff);
        assert_eq!(half(f32::NAN), 0);
        assert_eq!(half(1e-9), 0);
        // (1 + 1/2048 is halfway between halves: to the even one, 1)
        assert_eq!(half(1.0 + 1.0 / 2048.0), 0x3c00);
        assert_eq!(half(1.0 + 3.0 / 2048.0), 0x3c02);
        assert_eq!(from_half(0x7c00), f32::INFINITY);
    }

    #[test]
    fn the_key_changes_with_what_the_light_depends_on() {
        let plan = devmap_plan(BakeSettings::default());
        let again = devmap_plan(BakeSettings::default());
        assert_eq!(plan.key, again.key);
        let fewer = devmap_plan(BakeSettings {
            lightmap_samples: 8,
            ..BakeSettings::default()
        });
        assert_ne!(plan.key, fewer.key);
        let mut darker = plan.scene.clone();
        darker.sky = [0.0; 3];
        assert_ne!(
            plan.key,
            key(&darker, &plan.atlas, &plan.grid, &plan.settings)
        );
        // (the versions are hashed first: a hash without them differs)
        let mut hash = Fnv::default();
        hash.u32(FORMAT_VERSION + 1);
        assert_ne!(hash.0, Fnv::default().0);
        assert!(BakedLighting::file_name("devmap", 0, plan.key).ends_with(".bake"));
        assert_ne!(
            BakedLighting::file_name("devmap", 0, plan.key),
            BakedLighting::file_name("devmap", 0, fewer.key)
        );
    }

    #[test]
    fn a_bake_is_kept_and_read_back() {
        let mut level = opence_content::devmap::level(None);
        // a page to bake into
        let structure = &mut level.structure_bsps[0];
        for (index, light) in structure.lightmaps[0].materials[0]
            .lightmap_vertices
            .iter_mut()
            .enumerate()
        {
            light.texcoord = [(index % 7) as f32 / 6.0, (index / 7 % 7) as f32 / 6.0];
        }
        let structure = level.structure_bsps[0].clone();
        let mut plan =
            BakePlan::of(&level, 0, BakeSettings::default()).unwrap_or_else(|| panic!("no BSP"));
        plan.atlas.lay_out(&structure, 0, 16, 16);
        assert!(!plan.atlas.points.is_empty());
        let lit = |count: usize, value: f32| vec![[value, value * 2.0, value * 3.0, 0.5]; count];
        let points = plan.atlas.points.len();
        // (a sun, brighter than the devmap's, to keep beside the probes)
        plan.scene.lights.push(crate::scene::DirectionalLight {
            towards: [0.0, 0.6, 0.8],
            irradiance: [200.0, 100.0, 50.0],
            cos_half_angle: 0.9999,
        });
        let baked = plan.finish(
            &lit(points, 1.0),
            &vec![[0.0, 0.0, 1.0, 0.75]; points],
            &lit(plan.probe_points().len(), 0.25),
            &lit(plan.reflection_points().len(), 4.0),
        );
        assert_eq!(baked.pages.len(), 1);
        let page = &baked.pages[0];
        assert_eq!((page.width, page.height), (16, 16));
        assert_eq!(page.irradiance.len(), 16 * 16 * 4);
        // a lit texel: its light and occlusion, and its direction up
        let lit_texel = plan.atlas.charts[0]
            .texels
            .iter()
            .position(|texel| *texel != crate::chart::EMPTY)
            .unwrap_or_else(|| panic!("no texel"));
        let texel = &page.irradiance[lit_texel * 4..lit_texel * 4 + 4];
        assert_eq!(
            texel.iter().map(|h| from_half(*h)).collect::<Vec<_>>(),
            [1.0, 2.0, 3.0, 0.5]
        );
        assert_eq!(
            &page.directions[lit_texel * 4..lit_texel * 4 + 4],
            &[128, 128, 255, 191]
        );
        // the probes, axis by axis
        assert_eq!(baked.probes.len(), plan.grid.len() * 24);
        assert_eq!(from_half(baked.probes[1]), 0.5);
        assert_eq!(from_half(baked.radiance[2]), 12.0);
        assert_eq!(from_half(baked.radiance[3]), 1.0);
        let sun = baked.sun.unwrap_or_else(|| panic!("no sun"));
        assert_eq!(sun.towards, [0.0, 0.6, 0.8]);
        assert_eq!(
            sun.irradiance,
            [200.0, 100.0, 50.0].map(|c| c * baked.exposure)
        );
        let bytes = baked.encode();
        assert_eq!(BakedLighting::decode(&bytes), Ok(baked.clone()));
        // a broken file is refused, whatever its length
        for cut in [0, 7, 12, 30, bytes.len() / 2, bytes.len() - 1] {
            assert!(BakedLighting::decode(&bytes[..cut]).is_err(), "{cut}");
        }
        let mut longer = bytes.clone();
        longer.push(0);
        assert_eq!(BakedLighting::decode(&longer), Err(CacheError::Truncated));
        let mut other = bytes.clone();
        other[0] = b'X';
        assert_eq!(BakedLighting::decode(&other), Err(CacheError::Magic));
        other = bytes.clone();
        other[8] = 99;
        assert_eq!(BakedLighting::decode(&other), Err(CacheError::Version(99)));
        assert!(CacheError::Version(99).to_string().contains("99"));
        assert!(!CacheError::Magic.to_string().is_empty());
        assert!(!CacheError::Truncated.to_string().is_empty());
        // (and without a sun, or with a broken flag)
        let sunless = BakedLighting {
            sun: None,
            ..baked.clone()
        };
        let sunless_bytes = sunless.encode();
        assert_eq!(BakedLighting::decode(&sunless_bytes), Ok(sunless));
        // (the flag, then the reflection probes' count and places, then
        // their radiance's count and halves)
        let flag = sunless_bytes.len()
            - 4
            - (4 + baked.reflections.len() * 12)
            - (4 + baked.radiance.len() * 2);
        let mut broken = sunless_bytes.clone();
        broken[flag] = 2;
        assert_eq!(BakedLighting::decode(&broken), Err(CacheError::Truncated));
        // missing results are dark
        let dark = plan.finish(&[], &[], &[], &[]);
        assert!(dark.probes.iter().all(|half| *half == 0));
    }
}
