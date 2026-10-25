//! The mood check: whether the bake keeps the stock lightmaps' mood.
//!
//! The bake lights a level from what its tags say of the light, which is
//! not what the original's radiosity was tuned by, so its light can come
//! out brighter, darker or another colour than the level was made to look.
//! The check compares each baked page's mean light and colour with its
//! stock page's, over the texels the bake's chart covers, as the level
//! shader reads them: the stock page's values as stored, the baked
//! irradiance over pi as a display value ([`shown`]).
//!
//! The original's clusters are not imported, so the check is by page (a
//! page holds the surfaces of one part of the level) rather than by
//! cluster.
//!
//! Each BSP's light is scaled by the exposure that brings it to its stock
//! pages' brightness ([`fitted_exposure`]), as the original's lightmaps
//! were brought to theirs. A BSP keeps its mood ([`BspMood`]) when its
//! pages taken together keep the stock pages' brightness and colour, and
//! most of its texels lie on pages that keep their own.

use opence_content::{Level, Pixels};
use rayon::prelude::*;

use crate::chart::{Chart, EMPTY};
use crate::output::{from_half, BakedPage};
use crate::texel;

/// How far a page's light may be from its stock page's.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct MoodLimits {
    /// The most the brightness may differ by, as a factor either way (2:
    /// one stop).
    pub brightness: f32,
    /// The most each channel's share of the colour may differ by.
    pub hue: f32,
}

/// The limits every stock BSP keeps.
pub const LIMITS: MoodLimits = MoodLimits {
    brightness: 2.0,
    hue: 0.1,
};

/// The brightness below which a page counts as dark: its brightness is
/// compared from there, and its colour not at all (a dark page's colour is
/// mostly noise).
pub const DARK: f32 = 0.02;

/// A page's light against its stock page's.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct PageMood {
    /// The page, in the BSP's lightmaps.
    pub page: u32,
    /// The stock page's mean light (red, green, blue, as stored, 0 to 1).
    pub stock: [f32; 3],
    /// The baked page's mean light, as the shader reads it.
    pub baked: [f32; 3],
    /// How many texels were compared.
    pub texels: u32,
}

impl PageMood {
    /// The baked page's brightness over the stock page's, each counted
    /// from [`DARK`].
    pub fn ratio(&self) -> f32 {
        (brightness(self.baked) + DARK) / (brightness(self.stock) + DARK)
    }

    /// The most a channel's share of the colour differs by (0 when either
    /// page is dark).
    pub fn hue_difference(&self) -> f32 {
        if brightness(self.stock) < DARK || brightness(self.baked) < DARK {
            return 0.0;
        }
        let (stock, baked) = (shares(self.stock), shares(self.baked));
        (0..3)
            .map(|channel| (stock[channel] - baked[channel]).abs())
            .fold(0.0, f32::max)
    }

    /// Whether it keeps within `limits`.
    pub fn within(&self, limits: &MoodLimits) -> bool {
        let ratio = self.ratio();
        ratio <= limits.brightness
            && ratio * limits.brightness >= 1.0
            && self.hue_difference() <= limits.hue
    }
}

/// A colour's brightness as the level shader takes it: its channels' mean.
pub fn brightness(color: [f32; 3]) -> f32 {
    color.iter().sum::<f32>() / 3.0
}

/// Each channel's share of a colour.
fn shares(color: [f32; 3]) -> [f32; 3] {
    let sum = color.iter().sum::<f32>().max(f32::MIN_POSITIVE);
    color.map(|channel| channel / sum)
}

/// The mean of the stock page under `chart`'s covered texels (each read at
/// the same place on the stock page), as stored. None if the page has no
/// stock image the bake can decode, or the chart covers nothing.
pub fn stock_light(level: &Level, bsp: usize, chart: &Chart) -> Option<[f32; 3]> {
    stock_mean(chart, &StockPage::of(level, bsp, chart.page as usize)?)
}

/// The mean of `stock` under `chart`'s covered texels, as stored ([`stock_light`]).
pub fn stock_mean(chart: &Chart, stock: &StockPage) -> Option<[f32; 3]> {
    let stock_at = |x: u32, y: u32| {
        let sx = u64::from(x) * u64::from(stock.width) / u64::from(chart.width.max(1));
        let sy = u64::from(y) * u64::from(stock.height) / u64::from(chart.height.max(1));
        stock.light(sx as u32, sy as u32)
    };
    mean_over(chart, stock_at, |light| light)
}

/// A stock lightmap page, decoded.
#[derive(Clone, Debug, PartialEq)]
pub struct StockPage {
    /// Its width.
    pub width: u32,
    /// Its height.
    pub height: u32,
    /// Its texels, row by row from the top.
    pub texels: Vec<texel::Texel>,
}

impl StockPage {
    /// The stock page `page` of `level`'s BSP `bsp`. None if it has no
    /// image the bake can decode.
    pub fn of(level: &Level, bsp: usize, page: usize) -> Option<Self> {
        let structure = level.structure_bsps.get(bsp)?;
        let index = structure.lightmaps.get(page)?.bitmap_index?;
        let bitmap = level.bitmap(structure.lightmap_bitmap.as_ref()?)?;
        let image = bitmap.images.get(usize::from(index))?;
        let Pixels::Texture(texture) = &image.pixels else {
            return None;
        };
        let [width, height, _] = texture.level_size(0);
        let texels = texel::decode(texture.format, width, height, texture.levels.first()?)?;
        Some(Self {
            width,
            height,
            texels,
        })
    }

    /// The texel at `x`, `y`'s light, as stored (0 to 1).
    pub fn light(&self, x: u32, y: u32) -> Option<[f32; 3]> {
        if x >= self.width {
            return None;
        }
        self.texels
            .get(y as usize * self.width as usize + x as usize)
            .map(|texel| [0, 1, 2].map(|channel| f32::from(texel[channel]) / 255.0))
    }
}

/// The baked light as the level shader shows it: the irradiance over pi
/// (the light a white surface sends back, linear) as a display value, as
/// the stock lightmaps hold theirs, by the sRGB curve carried on past 1.
pub fn shown(irradiance: f32) -> f32 {
    let linear = if irradiance.is_finite() {
        irradiance.max(0.0) / std::f32::consts::PI
    } else {
        0.0
    };
    if linear <= 0.003_130_8 {
        linear * 12.92
    } else {
        1.055 * libm::powf(linear, 1.0 / 2.4) - 0.055
    }
}

/// The mean of `page`'s light under `chart`'s covered texels, as the level
/// shader shows it ([`shown`]). None if the chart covers nothing.
pub fn baked_light(chart: &Chart, page: &BakedPage) -> Option<[f32; 3]> {
    let width = page.width as usize;
    let light_at = |x: u32, y: u32| {
        let at = (y as usize * width + x as usize) * 4;
        let texel = page.irradiance.get(at..at + 3)?;
        Some([0, 1, 2].map(|channel| finite(from_half(texel[channel]))))
    };
    mean_over(chart, light_at, |light| {
        light.map(|channel| shown(channel).min(1.0))
    })
}

/// The most squares of texels ([`BLOCK`]) [`fitted_exposure`] reads
/// (spread evenly over the covered ones), so that fitting a large BSP
/// stays quick.
pub const FIT_TEXELS: usize = 1 << 16;

/// The side, in texels, of the squares whose light the mood check and
/// [`fitted_exposure`] average before showing it. Each texel's light is
/// traced along a few paths (four in the quick bake), so a level lit by
/// small lamps has many texels that no path found and a few that met a
/// lamp; shown texel by texel, each at most white, their mean comes out far
/// darker than the light they average, and the darker the fewer the paths.
/// A square averages its texels' paths first, as the eye takes in a patch
/// of a wall rather than a texel.
pub const BLOCK: u32 = 4;

/// The scale of the baked light that brings its mean brightness over the
/// charts' covered texels, as the level shader shows it (each channel at
/// most white, as an 8-bit lightmap holds it), to the stock pages' mean
/// brightness under the same texels. `irradiance` is the tracer's result
/// for each of the atlas's points; `stock`, each chart's stock mean. The
/// tags' units of light are not the bake's, and no one scale suits every
/// BSP (some are lit by a few small lamps of great power, others by wide
/// panels), so each BSP's exposure follows its stock lightmaps: the bake
/// keeps their overall brightness and changes only where the light falls.
/// 1 when there is nothing to fit to (no stock pages, or no baked light).
pub fn fitted_exposure(
    charts: &[Chart],
    stock: &[Option<[f32; 3]>],
    irradiance: &[[f32; 4]],
) -> f32 {
    // (the squares and the shown light are found in parallel and summed
    // in order, so the exposure is the same on any number of threads)
    let found: Vec<(f32, Squares)> = charts
        .par_iter()
        .zip(stock)
        .filter_map(|(chart, stock)| {
            let stock = (*stock)?;
            let width = chart.width.max(1) as usize;
            let light_at = |x: u32, y: u32| {
                let point = *chart.texels.get(y as usize * width + x as usize)?;
                let light = irradiance.get(point as usize)?;
                Some([0, 1, 2].map(|channel| finite(light[channel])))
            };
            Some((brightness(stock), squares_of(chart, light_at)))
        })
        .collect();
    let (mut target, mut texels) = (0.0f64, 0u64);
    let mut squares = Vec::new();
    for (stock, chart_squares) in found {
        let count: u64 = chart_squares
            .iter()
            .map(|(_, count)| u64::from(*count))
            .sum();
        target += f64::from(stock) * count as f64;
        texels += count;
        squares.extend(chart_squares);
    }
    if texels == 0 {
        return 1.0;
    }
    let target = target / texels as f64;
    let step = squares.len().div_ceil(FIT_TEXELS);
    let sample: Vec<&([f32; 3], u32)> = squares.iter().step_by(step).collect();
    let weight: f64 = sample.iter().map(|(_, count)| f64::from(*count)).sum();
    let shown_mean = |exposure: f32| {
        let terms: Vec<f64> = sample
            .par_iter()
            .map(|(light, count)| {
                let channels = light.map(|channel| shown(channel * exposure).min(1.0));
                f64::from(brightness(channels)) * f64::from(*count)
            })
            .collect();
        let sum: f64 = terms.iter().sum();
        sum / weight
    };
    if shown_mean(libm::exp2f(EXPOSURE_STOPS)) <= 0.0 {
        return 1.0;
    }
    // (the shown mean grows with the exposure: halve the stops between
    // the least and the most)
    let (mut low, mut high) = (-EXPOSURE_STOPS, EXPOSURE_STOPS);
    for _ in 0..24 {
        let middle = (low + high) / 2.0;
        if shown_mean(libm::exp2f(middle)) < target {
            low = middle;
        } else {
            high = middle;
        }
    }
    libm::exp2f((low + high) / 2.0)
}

/// The most stops [`fitted_exposure`] moves the light by, either way.
pub const EXPOSURE_STOPS: f32 = 8.0;

/// The mean over `chart`'s covered texels that have a `value` of each
/// [`BLOCK`] square's mean value as `show` shows it, weighted by the
/// square's texels.
fn mean_over(
    chart: &Chart,
    value: impl Fn(u32, u32) -> Option<[f32; 3]>,
    show: impl Fn([f32; 3]) -> [f32; 3],
) -> Option<[f32; 3]> {
    let mut sum = [0.0f64; 3];
    let mut count = 0u64;
    for (light, texels) in squares_of(chart, value) {
        for (sum, channel) in sum.iter_mut().zip(show(light)) {
            *sum += f64::from(channel) * f64::from(texels);
        }
        count += u64::from(texels);
    }
    (count > 0).then(|| sum.map(|sum| (sum / count as f64) as f32))
}

/// [`BLOCK`] squares' mean values, each with how many texels it has.
type Squares = Vec<([f32; 3], u32)>;

/// Each [`BLOCK`] square of `chart`'s covered texels that have a `value`:
/// its texels' mean value, and how many they are.
fn squares_of(chart: &Chart, value: impl Fn(u32, u32) -> Option<[f32; 3]>) -> Squares {
    let width = chart.width.max(1) as usize;
    let mut squares = Vec::new();
    for top in (0..chart.height).step_by(BLOCK as usize) {
        for left in (0..chart.width).step_by(BLOCK as usize) {
            let mut sum = [0.0f64; 3];
            let mut count = 0u32;
            for y in top..(top + BLOCK).min(chart.height) {
                for x in left..(left + BLOCK).min(chart.width) {
                    if chart.texels.get(y as usize * width + x as usize) == Some(&EMPTY) {
                        continue;
                    }
                    let Some(color) = value(x, y) else { continue };
                    for (sum, channel) in sum.iter_mut().zip(color) {
                        *sum += f64::from(channel);
                    }
                    count += 1;
                }
            }
            if count > 0 {
                squares.push((sum.map(|sum| (sum / f64::from(count)) as f32), count));
            }
        }
    }
    squares
}

/// A light's channel, or none (0) if the tracer lost it.
fn finite(channel: f32) -> f32 {
    if channel.is_finite() {
        channel
    } else {
        0.0
    }
}

/// How many of `chart`'s texels are covered.
pub fn covered(chart: &Chart) -> u32 {
    chart.texels.iter().filter(|point| **point != EMPTY).count() as u32
}

/// The most of a BSP's compared texels that may lie on pages outside the
/// limits. The bake lights each surface from the level's tags, and the
/// original's radiosity was tuned by hand, page by page; a page can keep
/// its BSP's mood and still be brighter or darker than its stock page (a
/// lamp the tags make brighter, a corner the original filled in).
pub const FAILING_SHARE: f32 = 0.6;

/// A BSP's light against its stock lightmaps': its pages taken together.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct BspMood {
    /// The pages' light weighted by their texels (`page` is the count of
    /// pages).
    pub whole: PageMood,
    /// The share of the texels on pages outside the limits.
    pub failing: f32,
}

impl BspMood {
    /// The BSP's pages taken together, or None if none was compared.
    pub fn of(pages: &[PageMood], limits: &MoodLimits) -> Option<BspMood> {
        let texels: u64 = pages.iter().map(|page| u64::from(page.texels)).sum();
        if texels == 0 {
            return None;
        }
        let mean = |light: fn(&PageMood) -> [f32; 3]| {
            let mut sum = [0.0f64; 3];
            for page in pages {
                for (sum, channel) in sum.iter_mut().zip(light(page)) {
                    *sum += f64::from(channel) * f64::from(page.texels);
                }
            }
            sum.map(|sum| (sum / texels as f64) as f32)
        };
        let failing: u64 = pages
            .iter()
            .filter(|page| !page.within(limits))
            .map(|page| u64::from(page.texels))
            .sum();
        Some(BspMood {
            whole: PageMood {
                page: pages.len() as u32,
                stock: mean(|page| page.stock),
                baked: mean(|page| page.baked),
                texels: u32::try_from(texels).unwrap_or(u32::MAX),
            },
            failing: (failing as f64 / texels as f64) as f32,
        })
    }

    /// Whether the BSP keeps its mood: its whole light within `limits`,
    /// and at most [`FAILING_SHARE`] of its texels on pages outside them.
    pub fn within(&self, limits: &MoodLimits) -> bool {
        self.whole.within(limits) && self.failing <= FAILING_SHARE
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::output::half;

    fn mood(stock: [f32; 3], baked: [f32; 3]) -> PageMood {
        PageMood {
            page: 0,
            stock,
            baked,
            texels: 1,
        }
    }

    #[test]
    fn the_limits_allow_a_stop_and_a_little_colour() {
        assert!(mood([0.5; 3], [0.5; 3]).within(&LIMITS));
        assert!((mood([0.5; 3], [0.5; 3]).ratio() - 1.0).abs() < 1e-6);
        assert!(mood([0.4; 3], [0.7; 3]).within(&LIMITS));
        assert!(!mood([0.2; 3], [0.7; 3]).within(&LIMITS));
        assert!(!mood([0.7; 3], [0.2; 3]).within(&LIMITS));
        // the same brightness, but blue instead of grey
        let blue = mood([0.5; 3], [0.3, 0.4, 0.8]);
        assert!(blue.hue_difference() > LIMITS.hue);
        assert!(!blue.within(&LIMITS));
        // a little warmer is fine
        assert!(mood([0.5; 3], [0.55, 0.5, 0.45]).within(&LIMITS));
        // dark pages are compared from the dark floor, and not by colour
        let dark = mood([0.0; 3], [0.01, 0.0, 0.0]);
        assert_eq!(dark.hue_difference(), 0.0);
        assert!(dark.within(&LIMITS));
        assert!(!mood([0.0; 3], [0.1; 3]).within(&LIMITS));
    }

    #[test]
    fn a_bsp_keeps_its_mood_by_its_whole_light_and_most_of_its_texels() {
        let page = |stock: f32, baked: f32, texels: u32| PageMood {
            texels,
            ..mood([stock; 3], [baked; 3])
        };
        assert!(BspMood::of(&[], &LIMITS).is_none());
        // one page of four is too bright: the BSP keeps its mood
        let bsp = BspMood::of(&[page(0.5, 0.5, 3), page(0.2, 0.7, 1)], &LIMITS)
            .unwrap_or_else(|| panic!("no mood"));
        assert_eq!(bsp.whole.page, 2);
        assert_eq!(bsp.whole.texels, 4);
        assert!((bsp.whole.stock[0] - 0.425).abs() < 1e-6);
        assert!((bsp.whole.baked[0] - 0.55).abs() < 1e-6);
        assert!((bsp.failing - 0.25).abs() < 1e-6);
        assert!(bsp.within(&LIMITS));
        // too bright and too dark pages that even out still fail it
        let bsp = BspMood::of(
            &[page(0.5, 0.5, 1), page(0.2, 0.7, 2), page(0.7, 0.2, 2)],
            &LIMITS,
        )
        .unwrap_or_else(|| panic!("no mood"));
        assert!(bsp.whole.within(&LIMITS));
        assert!((bsp.failing - 0.8).abs() < 1e-6);
        assert!(!bsp.within(&LIMITS));
        // as does a whole BSP too bright
        let bsp = BspMood::of(&[page(0.2, 0.7, 1)], &LIMITS).unwrap_or_else(|| panic!("no mood"));
        assert!(!bsp.within(&LIMITS));
    }

    fn chart(width: u32, height: u32, covered: &[usize]) -> Chart {
        let mut texels = vec![EMPTY; (width * height) as usize];
        for (point, at) in covered.iter().enumerate() {
            texels[*at] = point as u32;
        }
        Chart {
            page: 0,
            width,
            height,
            texels,
        }
    }

    #[test]
    fn the_baked_mean_is_over_the_covered_texels() {
        // (the two covered texels in squares of their own)
        let chart = chart(8, 1, &[0, 7]);
        assert_eq!(covered(&chart), 2);
        let pi = std::f32::consts::PI;
        let texel = |value: f32| [value, value, value, 1.0].map(half);
        // (pi: white, shown as 1; 0: black)
        let mut light = texel(pi).to_vec();
        for _ in 1..7 {
            light.extend(texel(100.0));
        }
        light.extend(texel(0.0));
        let page = BakedPage {
            page: 0,
            width: 8,
            height: 1,
            irradiance: light,
            directions: vec![0; 32],
        };
        let mean = baked_light(&chart, &page).unwrap_or_else(|| panic!("no mean"));
        for channel in mean {
            assert!((channel - 0.5).abs() < 1e-2, "{mean:?}");
        }
        assert_eq!(baked_light(&self::chart(8, 1, &[]), &page), None);
        // a page shorter than its chart counts what it has
        let short = BakedPage {
            irradiance: texel(pi).to_vec(),
            ..page
        };
        let mean = baked_light(&chart, &short).unwrap_or_else(|| panic!("no mean"));
        assert!((mean[0] - 1.0).abs() < 1e-2);
    }

    #[test]
    fn a_square_is_averaged_before_it_is_shown() {
        // two texels of one square, one that met a lamp and one that met
        // nothing: their light is white on average, though each shown
        // alone would be black and clipped white
        let chart = chart(2, 2, &[0, 3]);
        let pi = std::f32::consts::PI;
        let texel = |value: f32| [value, value, value, 1.0].map(half);
        let page = BakedPage {
            page: 0,
            width: 2,
            height: 2,
            irradiance: [texel(2.0 * pi), texel(0.0), texel(0.0), texel(0.0)].concat(),
            directions: vec![0; 16],
        };
        let mean = baked_light(&chart, &page).unwrap_or_else(|| panic!("no mean"));
        assert!((mean[0] - 1.0).abs() < 1e-2, "{mean:?}");
        let irradiance = [[2.0 * pi, 2.0 * pi, 2.0 * pi, 1.0], [0.0, 0.0, 0.0, 1.0]];
        let chart = self::chart(2, 1, &[0, 1]);
        let exposure = fitted_exposure(&[chart], &[Some([1.0; 3])], &irradiance);
        assert!((exposure - 1.0).abs() < 0.02, "{exposure}");
    }

    #[test]
    fn the_exposure_brings_the_light_to_the_stock_brightness() {
        let pi = std::f32::consts::PI;
        let charts = [chart(2, 1, &[0, 1]), chart(1, 1, &[0])];
        // (the second chart's point is the third, lit by its stock page
        // only if it has one)
        let mut second = charts[1].clone();
        second.texels = vec![2];
        let charts = [charts[0].clone(), second];
        let irradiance = [[pi; 4], [pi; 4], [pi; 4]];
        // white light under stock pages of a quarter: shown at a quarter
        let quarter = fitted_exposure(&charts, &[Some([0.25; 3]), Some([0.25; 3])], &irradiance);
        let mean = shown(pi * quarter);
        assert!((mean - 0.25).abs() < 1e-3, "{quarter} {mean}");
        // dim light is brought up, within the limit of stops
        let dim = [[pi / 16.0; 4]; 3];
        let up = fitted_exposure(&charts, &[Some([1.0; 3]), None], &dim);
        assert!((shown(pi / 16.0 * up) - 1.0).abs() < 1e-3, "{up}");
        let dark = [[1e-9; 4]; 3];
        let most = fitted_exposure(&charts, &[Some([1.0; 3]), None], &dark);
        assert!((most - libm::exp2f(EXPOSURE_STOPS)).abs() < 1.0, "{most}");
        // nothing to fit to: no stock pages, or no light
        assert_eq!(fitted_exposure(&charts, &[None, None], &irradiance), 1.0);
        assert_eq!(
            fitted_exposure(&charts, &[Some([0.5; 3]); 2], &[[0.0; 4]; 3]),
            1.0
        );
        assert_eq!(fitted_exposure(&[], &[], &[]), 1.0);
    }

    #[test]
    fn baked_light_is_shown_as_a_display_value() {
        let pi = std::f32::consts::PI;
        assert!((shown(pi) - 1.0).abs() < 1e-5);
        assert_eq!(shown(0.0), 0.0);
        assert_eq!(shown(-1.0), 0.0);
        assert_eq!(shown(f32::NAN), 0.0);
        // a fifth of white's light is about half as bright on the screen
        let fifth = shown(0.2 * pi);
        assert!((fifth - opence_content::material::display(0.2)).abs() < 1e-5);
        assert!(fifth > 0.45 && fifth < 0.5, "{fifth}");
        // the dimmest light, on the curve's straight part
        assert!((shown(0.001 * pi) - 0.01292).abs() < 1e-6);
        // and past white, brighter still
        assert!(shown(4.0 * pi) > 1.5);
    }

    #[test]
    fn the_stock_mean_reads_the_stock_page_under_the_chart() {
        let mut level = opence_content::devmap::level(None);
        // the devmap has no stock lightmaps
        assert_eq!(stock_light(&level, 0, &chart(2, 2, &[0])), None);
        assert_eq!(stock_light(&level, 5, &chart(2, 2, &[0])), None);
        // a 2 by 2 stock page under a 4 by 4 chart: each stock texel under
        // four of the chart's
        let tag = opence_content::TagRef {
            group: opence_content::Group(*b"bitm"),
            path: "lightmaps".into(),
        };
        let mut bitmap = level.bitmaps[0].clone();
        bitmap.tag = tag.clone();
        bitmap.images.truncate(1);
        bitmap.images[0].width = 2;
        bitmap.images[0].height = 2;
        bitmap.images[0].pixels = Pixels::Texture(opence_content::Texture {
            format: opence_content::TextureFormat::Rgba8,
            width: 2,
            height: 2,
            depth: 1,
            faces: 1,
            levels: vec![[[255, 0, 0, 255], [0; 4], [0; 4], [0, 0, 255, 255]].concat()],
        });
        level.bitmaps.push(bitmap);
        let structure = &mut level.structure_bsps[0];
        structure.lightmap_bitmap = Some(tag);
        structure.lightmaps[0].bitmap_index = Some(0);
        // the chart covers the top left quarter and one texel of the
        // bottom right: three red texels and one blue
        let quarter = chart(4, 4, &[0, 1, 4, 15]);
        let mean = stock_light(&level, 0, &quarter).unwrap_or_else(|| panic!("no mean"));
        assert_eq!(mean, [0.75, 0.0, 0.25]);
        assert_eq!(stock_light(&level, 0, &chart(4, 4, &[])), None);
    }
}
