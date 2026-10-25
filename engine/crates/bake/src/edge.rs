//! The steps in light between neighbouring texels of one surface: the
//! stairs a shadow's edge is drawn in when each texel holds the light at
//! one point (the edge check).
//!
//! Two texels side by side on a page are neighbours on a surface when
//! their points face the same way and lie within a texel or two of each
//! other in the level. A step between them is hard when the light, as the
//! level shader shows it, changes by more than [`HARD_STEP`] of the range
//! from one to the next. A shadow's edge falling across texels smoothly
//! has few hard steps; one drawn texel by texel has a row of them.

use crate::chart::{Atlas, BakePoint, Chart, EMPTY};
use crate::mood::{brightness, shown, StockPage};
use crate::output::{from_half, BakedPage};

/// The step in shown brightness (0 to 1) between neighbours that counts
/// as hard.
pub const HARD_STEP: f32 = 0.25;
/// How far apart, in their texels' footprints, two points may be and
/// still be neighbours on a surface.
const NEIGHBOURS_WITHIN: f32 = 2.5;

/// The steps between neighbouring texels of a set of pages.
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct EdgeSteps {
    /// How many pairs of neighbours there are.
    pub pairs: u32,
    /// How many of them step by more than [`HARD_STEP`].
    pub hard: u32,
    /// The mean step.
    pub mean: f32,
}

impl EdgeSteps {
    /// The share of the pairs that step hard (0 if there are none).
    pub fn hard_share(&self) -> f32 {
        if self.pairs > 0 {
            self.hard as f32 / self.pairs as f32
        } else {
            0.0
        }
    }

    /// The steps on the baked `pages` (one to a chart of `atlas`).
    pub fn baked(atlas: &Atlas, pages: &[BakedPage]) -> Self {
        let mut steps = Tally::default();
        for (chart, page) in atlas.charts.iter().zip(pages) {
            let light = |x: u32, y: u32| -> Option<f32> {
                let at = (y * page.width + x) as usize;
                let texel = page.irradiance.get(at * 4..at * 4 + 3)?;
                let color = [0, 1, 2].map(|channel| shown(from_half(texel[channel])).min(1.0));
                Some(brightness(color))
            };
            steps.page(atlas, chart, 1, light);
        }
        steps.finish()
    }

    /// The steps on the `stock` pages (one to a chart of `atlas`, if it has
    /// one), as stored: each stock texel's surface is the baked chart's at
    /// its middle.
    pub fn stock(atlas: &Atlas, stock: &[Option<StockPage>]) -> Self {
        let mut steps = Tally::default();
        for (chart, page) in atlas.charts.iter().zip(stock) {
            let Some(page) = page else { continue };
            if page.width == 0 || chart.width % page.width != 0 {
                continue;
            }
            let scale = chart.width / page.width;
            if chart.height != page.height * scale {
                continue;
            }
            steps.page(atlas, chart, scale, |x, y| page.light(x, y).map(brightness));
        }
        steps.finish()
    }
}

/// The running count of [`EdgeSteps`].
#[derive(Default)]
struct Tally {
    pairs: u32,
    hard: u32,
    sum: f64,
}

impl Tally {
    /// Counts the pairs of a page `scale` times smaller each way than
    /// `chart`, whose light at a texel is `light`.
    fn page(
        &mut self,
        atlas: &Atlas,
        chart: &Chart,
        scale: u32,
        light: impl Fn(u32, u32) -> Option<f32>,
    ) {
        let (width, height) = (chart.width / scale, chart.height / scale);
        let point = |x: u32, y: u32| -> Option<&BakePoint> {
            let middle = (y * scale + scale / 2) * chart.width + x * scale + scale / 2;
            let index = *chart.texels.get(middle as usize)?;
            (index != EMPTY)
                .then(|| atlas.points.get(index as usize))
                .flatten()
        };
        for y in 0..height {
            for x in 0..width {
                let Some(here) = point(x, y) else { continue };
                for (nx, ny) in [(x + 1, y), (x, y + 1)] {
                    if nx >= width || ny >= height {
                        continue;
                    }
                    let Some(there) = point(nx, ny) else { continue };
                    if !neighbours(here, there, scale) {
                        continue;
                    }
                    if let (Some(a), Some(b)) = (light(x, y), light(nx, ny)) {
                        let step = (a - b).abs();
                        if !step.is_finite() {
                            continue;
                        }
                        self.pairs += 1;
                        self.hard += u32::from(step > HARD_STEP);
                        self.sum += f64::from(step);
                    }
                }
            }
        }
    }

    fn finish(self) -> EdgeSteps {
        EdgeSteps {
            pairs: self.pairs,
            hard: self.hard,
            mean: if self.pairs > 0 {
                (self.sum / f64::from(self.pairs)) as f32
            } else {
                0.0
            },
        }
    }
}

/// Whether `a` and `b`, the points of texels side by side on a page
/// `scale` times coarser than their chart, are neighbours on one surface.
fn neighbours(a: &BakePoint, b: &BakePoint, scale: u32) -> bool {
    if a == b {
        return false;
    }
    let facing: f32 = (0..3).map(|axis| a.normal[axis] * b.normal[axis]).sum();
    let apart: f32 = (0..3)
        .map(|axis| {
            let along = a.position[axis] - b.position[axis];
            along * along
        })
        .sum::<f32>()
        .sqrt();
    let reach = a.footprint.max(b.footprint) * scale as f32 * NEIGHBOURS_WITHIN;
    facing > 0.9 && reach > 0.0 && apart <= reach
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::chart::Chart;

    /// A page of `width` by 1 texels on the plane z = 0, a unit apart.
    fn row(width: u32) -> Atlas {
        Atlas {
            charts: vec![Chart {
                page: 0,
                width,
                height: 1,
                texels: (0..width).collect(),
            }],
            points: (0..width)
                .map(|x| BakePoint {
                    position: [x as f32, 0.0, 0.0],
                    normal: [0.0, 0.0, 1.0],
                    footprint: 1.0,
                })
                .collect(),
        }
    }

    fn page(light: &[f32]) -> BakedPage {
        BakedPage {
            page: 0,
            width: light.len() as u32,
            height: 1,
            irradiance: light
                .iter()
                .flat_map(|value| {
                    let half = crate::output::half(*value);
                    [half, half, half, 0]
                })
                .collect(),
            directions: vec![0; light.len() * 4],
        }
    }

    #[test]
    fn a_shadow_drawn_in_steps_has_hard_steps_and_a_smooth_one_has_none() {
        let atlas = row(8);
        let pi = core::f32::consts::PI;
        let stepped = EdgeSteps::baked(&atlas, &[page(&[pi, pi, pi, pi, 0.0, 0.0, 0.0, 0.0])]);
        assert_eq!((stepped.pairs, stepped.hard), (7, 1));
        let ramp: Vec<f32> = (0..8)
            .map(|x| pi * libm::powf(1.0 - x as f32 / 7.0, 2.2))
            .collect();
        let smooth = EdgeSteps::baked(&atlas, &[page(&ramp)]);
        assert_eq!((smooth.pairs, smooth.hard), (7, 0));
        assert!(smooth.mean > 0.1 && smooth.mean < 0.2, "{smooth:?}");
        assert!(stepped.hard_share() > smooth.hard_share());
    }

    #[test]
    fn texels_of_other_surfaces_are_not_neighbours() {
        let mut atlas = row(4);
        // (a wall's texel beside the floor's, and one far off on the page)
        atlas.points[1].normal = [1.0, 0.0, 0.0];
        atlas.points[3].position = [50.0, 0.0, 0.0];
        let steps = EdgeSteps::baked(&atlas, &[page(&[1.0, 0.0, 1.0, 0.0])]);
        assert_eq!(steps.pairs, 0);
        assert_eq!(steps.hard_share(), 0.0);
        // (a texel the chart was dilated into shares its neighbour's point)
        let mut dilated = row(2);
        dilated.charts[0].texels = vec![0, 0];
        assert_eq!(EdgeSteps::baked(&dilated, &[page(&[1.0, 0.0])]).pairs, 0);
    }
}
