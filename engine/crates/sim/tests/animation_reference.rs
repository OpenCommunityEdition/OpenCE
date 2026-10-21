//! The engine's animation against the game's own code: the reference
//! (`animation_reference.txt`) is what the decompilation's functions give on
//! made-up animations, written by `tools/engine_animation_reference.py`.
//! Every value must be the same to the bit.

use std::collections::HashMap;
use std::error::Error;

use opence_content::{Animation, AnimationKind, AnimationNode};
use opence_sim::animation::{
    apply_overlay, apply_replacement, base_orientations, interpolate_orientations, node_matrices,
    NodeMatrix, Orientation,
};

const REFERENCE: &str = include_str!("animation_reference.txt");

type Result<T> = std::result::Result<T, Box<dyn Error>>;

fn words(fields: &[&str]) -> Result<Vec<u32>> {
    Ok(fields
        .iter()
        .map(|word| u32::from_str_radix(word, 16))
        .collect::<std::result::Result<_, _>>()?)
}

fn hex(text: Option<&&str>) -> Result<Vec<u8>> {
    let text = text.copied().unwrap_or("");
    Ok((0..text.len())
        .step_by(2)
        .map(|at| u8::from_str_radix(&text[at..at + 2], 16))
        .collect::<std::result::Result<_, _>>()?)
}

fn orientations(words: &[u32]) -> Vec<Orientation> {
    words
        .as_chunks::<8>()
        .0
        .iter()
        .map(|node| {
            let f = |index: usize| f32::from_bits(node[index]);
            Orientation {
                rotation: [f(0), f(1), f(2), f(3)],
                translation: [f(4), f(5), f(6)],
                scale: f(7),
            }
        })
        .collect()
}

fn orientation_bits(nodes: &[Orientation]) -> Vec<u32> {
    nodes
        .iter()
        .flat_map(|node| {
            node.rotation
                .into_iter()
                .chain(node.translation)
                .chain([node.scale])
                .map(f32::to_bits)
        })
        .collect()
}

fn matrix_bits(matrices: &[NodeMatrix]) -> Vec<u32> {
    matrices
        .iter()
        .flat_map(|matrix| {
            matrix
                .forward
                .into_iter()
                .chain(matrix.left)
                .chain(matrix.up)
                .chain(matrix.position)
                .chain([matrix.scale])
                .map(f32::to_bits)
        })
        .collect()
}

fn animation(fields: &[&str]) -> Result<Animation> {
    let number = |index: usize| -> Result<u32> {
        let field = fields.get(index).ok_or("a short animation line")?;
        Ok(field.parse::<i64>()? as u32)
    };
    Ok(Animation {
        name: (*fields.first().ok_or("no name")?).into(),
        kind: AnimationKind::Base,
        frame_count: number(2)? as u16,
        frame_size: number(3)? as u16,
        frame_info_type: 0,
        node_list_checksum: 0,
        node_count: number(4)? as u16,
        loop_frame: 0,
        weight: 1.0,
        key_frame: -1,
        second_key_frame: -1,
        next_animation: None,
        parent_animation: None,
        normalized_weight: 1.0,
        flags: number(5)? as u16,
        sound: None,
        sound_frame: -1,
        left_foot_frame: -1,
        right_foot_frame: -1,
        frame_info: Vec::new(),
        rotation_flags: [number(7)?, 0],
        translation_flags: [number(8)?, 0],
        scale_flags: [number(9)?, 0],
        compressed_data_offset: number(6)?,
        default_data: Vec::new(),
        data: Vec::new(),
    })
}

fn link(field: &str) -> Result<Option<u16>> {
    let index: i32 = field.parse()?;
    Ok(u16::try_from(index).ok())
}

fn check(what: &str, found: &[u32], expected: &[u32]) -> Result<()> {
    if found != expected {
        let at = found
            .iter()
            .zip(expected)
            .position(|(a, b)| a != b)
            .unwrap_or(found.len().min(expected.len()));
        return Err(format!(
            "{what}: word {at} is {:08x?}, the game's is {:08x?} ({} words, the game's {})",
            found.get(at),
            expected.get(at),
            found.len(),
            expected.len()
        )
        .into());
    }
    Ok(())
}

#[test]
fn the_engines_animation_is_the_games() -> Result<()> {
    let mut animations: HashMap<String, Animation> = HashMap::new();
    let mut poses: HashMap<String, Vec<Orientation>> = HashMap::new();
    let mut graphs: HashMap<String, Vec<AnimationNode>> = HashMap::new();
    let mut checked = 0;

    for line in REFERENCE.lines().filter(|line| !line.starts_with('#')) {
        let fields: Vec<&str> = line.split_whitespace().collect();
        let Some((&record, rest)) = fields.split_first() else {
            continue;
        };
        let named = |name: &str| -> Result<Animation> {
            animations
                .get(name)
                .cloned()
                .ok_or_else(|| format!("no animation {name}").into())
        };
        let pose = |name: &str| -> Result<Vec<Orientation>> {
            poses
                .get(name)
                .cloned()
                .ok_or_else(|| format!("no pose {name}").into())
        };
        match record {
            "pose" => {
                poses.insert(rest[0].into(), orientations(&words(&rest[1..])?));
            }
            "animation" => {
                animations.insert(rest[0].into(), animation(rest)?);
            }
            "data" | "default" => {
                let bytes = hex(rest.get(1))?;
                let animation = animations
                    .get_mut(rest[0])
                    .ok_or("data before its animation")?;
                if record == "data" {
                    animation.data = bytes;
                } else {
                    animation.default_data = bytes;
                }
            }
            "base" => {
                let animation = named(rest[0])?;
                let frame: i16 = rest[1].parse()?;
                let mut nodes = vec![Orientation::REST; usize::from(animation.node_count)];
                base_orientations(None, &animation, frame, &mut nodes);
                check(line, &orientation_bits(&nodes), &words(&rest[2..])?)?;
                checked += 1;
            }
            "replace" | "overlay" => {
                let mut animation = named(rest[0])?;
                let mut nodes = pose(rest[1])?;
                let frame: i16 = rest[2].parse()?;
                if record == "replace" {
                    animation.kind = AnimationKind::Replacement;
                    apply_replacement(&animation, frame, &mut nodes);
                } else {
                    animation.kind = AnimationKind::Overlay;
                    apply_overlay(&animation, frame, &mut nodes);
                }
                check(line, &orientation_bits(&nodes), &words(&rest[3..])?)?;
                checked += 1;
            }
            "blend" => {
                let original = pose(rest[0])?;
                let mut target = pose(rest[1])?;
                let frame: i16 = rest[2].parse()?;
                let frame_count: i16 = rest[3].parse()?;
                interpolate_orientations(&original, &mut target, frame, frame_count);
                check(line, &orientation_bits(&target), &words(&rest[4..])?)?;
                checked += 1;
            }
            "graph" => {
                let nodes = rest[1..]
                    .iter()
                    .map(|node| -> Result<AnimationNode> {
                        let links: Vec<&str> = node.split(',').collect();
                        Ok(AnimationNode {
                            name: String::new(),
                            parent: link(links[0])?,
                            first_child: link(links[1])?,
                            next_sibling: link(links[2])?,
                        })
                    })
                    .collect::<Result<_>>()?;
                graphs.insert(rest[0].into(), nodes);
            }
            "matrices" => {
                let graph = graphs.get(rest[0]).ok_or("no graph")?;
                let animation = named(rest[1])?;
                let frame: i16 = rest[2].parse()?;
                let real = |index: usize| -> Result<f32> { Ok(rest[index].parse()?) };
                let root = NodeMatrix::from_point_and_vectors(
                    [real(3)?, real(4)?, real(5)?],
                    [real(6)?, real(7)?, real(8)?],
                    [real(9)?, real(10)?, real(11)?],
                );
                let mut nodes = vec![Orientation::REST; graph.len()];
                base_orientations(None, &animation, frame, &mut nodes);
                let matrices = node_matrices(graph, &nodes, &root);
                check(line, &matrix_bits(&matrices), &words(&rest[12..])?)?;
                checked += 1;
            }
            other => return Err(format!("a record the test does not know: {other}").into()),
        }
    }
    // (every line the reference has was checked: a reference cut short
    // would not pass)
    assert_eq!(checked, 54 + 18 + 18 + 4 + 1);
    Ok(())
}
