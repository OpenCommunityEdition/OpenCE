//! A unit's pose blended from a base animation of all its nodes, on
//! bloodgulch's graph with the most nodes (`cargo bench -p opence_sim`; nothing without
//! assets/maps).

use divan::Bencher;
use opence_content::{AnimationGraph, AnimationKind};
use opence_import::{import_animation_graph, Cache, GroupTag};
use opence_sim::animation::AnimationState;
use opence_sim::animation_blend::{blend_pose, ObjectLayers, UnitLayers};

fn main() {
    divan::main();
}

/// Bloodgulch's animation graph with the most nodes (a biped's).
fn biped() -> Option<AnimationGraph> {
    let map = opence_import::testing::maps_folder().join("bloodgulch.map");
    let cache = Cache::open(std::fs::read(map).ok()?).ok()?;
    let antr = GroupTag::new(b"antr");
    cache
        .tags()
        .iter()
        .enumerate()
        .filter(|(_, tag)| tag.is(antr))
        .filter_map(|(index, _)| import_animation_graph(&cache, index).ok())
        .filter(|graph| !graph.animations.is_empty())
        .max_by_key(|graph| graph.nodes.len())
}

#[divan::bench]
fn a_bipeds_pose_is_blended(bencher: Bencher) {
    let Some(graph) = biped() else {
        return;
    };
    // (its first base animation of all its nodes, half way through)
    let Some((animation, found)) = graph.animations.iter().enumerate().find(|(_, animation)| {
        animation.kind == AnimationKind::Base
            && usize::from(animation.node_count) == graph.nodes.len()
            && animation.frame_count > 1
    }) else {
        return;
    };
    let state = AnimationState {
        animation: u16::try_from(animation).unwrap_or_default(),
        frame: found.frame_count / 2,
    };
    let layers = ObjectLayers {
        base: Some(state),
        ..ObjectLayers::default()
    };
    let unit = UnitLayers::default();
    assert_eq!(
        blend_pose(&graph, None, &layers, Some(&unit)).len(),
        graph.nodes.len()
    );
    bencher.bench(|| blend_pose(&graph, None, divan::black_box(&layers), Some(&unit)));
}
