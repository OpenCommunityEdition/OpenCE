//! The bake's two costs: building the triangles' hierarchy and tracing a
//! texel's light, in the furnace layout (`cargo bench -p opence_bake`).

use divan::Bencher;
use opence_bake::layout::{layout, OCCLUSION_DISTANCE};
use opence_bake::{Bvh, Random, Tracer};

fn main() {
    divan::main();
}

#[divan::bench]
fn the_hierarchy_is_built(bencher: Bencher) {
    let Some((scene, _)) = layout("furnace") else {
        return;
    };
    bencher.bench(|| Bvh::build(divan::black_box(&scene.triangles)));
}

#[divan::bench(args = [16, 64])]
fn a_texel_is_traced(bencher: Bencher, samples: u32) {
    let Some((scene, _)) = layout("furnace") else {
        return;
    };
    let bvh = Bvh::build(&scene.triangles);
    let tracer = Tracer {
        scene: &scene,
        bvh: &bvh,
        occlusion_distance: OCCLUSION_DISTANCE,
    };
    let mut random = Random::new(7);
    bencher.bench_local(|| tracer.irradiance([0.0; 3], [0.0, 0.0, 1.0], samples, &mut random));
}
