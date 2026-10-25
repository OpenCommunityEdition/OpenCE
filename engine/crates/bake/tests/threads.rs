//! The bake's CPU side splits its work over rayon's threads; its output is
//! the same with one thread as with many, bit for bit, so that a bake
//! cached on one machine is the bake another would make.

use std::error::Error;

use opence_bake::{filter, BakePlan, BakeSettings, BakedLighting};
use opence_import::testing::stock_level_unheld;
use rayon::ThreadPoolBuilder;

/// Light that varies from point to point, with a bright speck every so
/// often and now and then a point the tracer lost.
fn traced(count: usize) -> Vec<[f32; 4]> {
    (0..count)
        .map(|index| {
            let hash = (index as u32).wrapping_mul(2_654_435_761);
            let base = (hash >> 8) as f32 / (1 << 24) as f32;
            match hash % 97 {
                0 => [f32::NAN, base, base, 1.0],
                1..=3 => [base * 60.0, base * 50.0, base * 40.0, 1.0],
                _ => [base * 3.0, base * 2.5, base * 2.0, base],
            }
        })
        .collect()
}

/// `run` in a pool of `threads` threads.
fn with_threads<T: Send>(
    threads: usize,
    run: impl FnOnce() -> T + Send,
) -> Result<T, Box<dyn Error>> {
    Ok(ThreadPoolBuilder::new()
        .num_threads(threads)
        .build()?
        .install(run))
}

#[test]
fn a_bake_finished_on_one_thread_is_the_bake_finished_on_many() -> Result<(), Box<dyn Error>> {
    let Some(level) = stock_level_unheld("bloodgulch")? else {
        return Ok(());
    };
    let plan = BakePlan::of(&level, 0, BakeSettings::default()).ok_or("no plan")?;
    let irradiance = traced(plan.atlas.points.len());
    let directions = vec![[0.0, 0.0, 1.0, 0.5]; plan.atlas.points.len()];
    let probes = traced(plan.grid.len() * 6);
    let radiance = traced(plan.reflection_points().len());
    let finish = || -> BakedLighting { plan.finish(&irradiance, &directions, &probes, &radiance) };
    let one = with_threads(1, finish)?;
    for threads in [2, 8] {
        let many = with_threads(threads, finish)?;
        assert_eq!(
            one.exposure.to_bits(),
            many.exposure.to_bits(),
            "{threads} threads"
        );
        assert!(one == many, "{threads} threads: the pages differ");
    }
    let held = |threads| with_threads(threads, || filter::hold_fireflies(&plan.atlas, &irradiance));
    let (one, many) = (held(1)?, held(8)?);
    assert!(
        one.iter()
            .zip(&many)
            .all(|(a, b)| a.map(f32::to_bits) == b.map(f32::to_bits)),
        "the held light differs"
    );
    Ok(())
}
