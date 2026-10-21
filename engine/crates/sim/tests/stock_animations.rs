//! Every animation of Blood Gulch's graphs, played at every frame, poses its
//! nodes: finite, each rotation of length one, each translation inside the
//! level, as they are not when a frame is read from the wrong place. Skipped
//! without assets/maps/bloodgulch.map.

use std::error::Error;
use std::path::Path;

use opence_content::AnimationKind;
use opence_import::{import_animation_graph, Cache, GroupTag};
use opence_sim::animation::{
    apply_overlay, apply_replacement, base_orientations, node_matrices, NodeMatrix, Orientation,
};

#[test]
fn blood_gulchs_animations_pose_their_nodes() -> Result<(), Box<dyn Error>> {
    let path = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../assets/maps/bloodgulch.map");
    let Ok(file) = std::fs::read(&path) else {
        return Ok(()); // (no game data here)
    };
    let cache = Cache::open(file)?;
    let antr = GroupTag::new(b"antr");
    let (mut animations, mut frames, mut still) = (0, 0, Vec::new());
    for (index, tag) in cache.tags().iter().enumerate() {
        if !tag.is(antr) {
            continue;
        }
        let graph = import_animation_graph(&cache, index)?;
        for animation in &graph.animations {
            let label = format!("{} '{}'", tag.name, animation.name);
            let mut moved = false;
            for frame in 0..animation.frame_count as i16 {
                // (a vehicle's or scenery's graph has no nodes of its own: its
                // model's are the animation's)
                let count = graph.nodes.len().max(usize::from(animation.node_count));
                let mut nodes = vec![Orientation::REST; count];
                match animation.kind {
                    AnimationKind::Base => base_orientations(None, animation, frame, &mut nodes),
                    AnimationKind::Replacement => apply_replacement(animation, frame, &mut nodes),
                    AnimationKind::Overlay => apply_overlay(animation, frame, &mut nodes),
                }
                for (node, orientation) in nodes.iter().enumerate() {
                    let [i, j, k, w] = orientation.rotation;
                    let length = (i * i + j * j + k * k + w * w).sqrt();
                    assert!(
                        (length - 1.0).abs() < 0.01,
                        "{label} frame {frame} node {node}: rotation {:?}",
                        orientation.rotation
                    );
                    assert!(
                        orientation
                            .translation
                            .iter()
                            .chain([&orientation.scale])
                            .all(|value| value.is_finite() && value.abs() < 100.0),
                        "{label} frame {frame} node {node}: {orientation:?}"
                    );
                }
                moved |= nodes.iter().any(|node| *node != Orientation::REST);
                let matrices = node_matrices(&graph.nodes, &nodes, &NodeMatrix::IDENTITY);
                assert!(matrices
                    .iter()
                    .all(|matrix| matrix.position.iter().all(|value| value.is_finite())));
                frames += 1;
            }
            if !moved && animation.frame_count > 0 {
                still.push(label);
            }
            animations += 1;
        }
    }
    assert!(
        animations > 300 && frames > 10_000,
        "{animations} animations, {frames} frames"
    );
    // (a frame the reader refused is the rest pose: few animations hold
    // every node still at every frame)
    assert!(
        still.len() * 50 < animations,
        "{} still: {still:?}",
        still.len()
    );
    Ok(())
}
