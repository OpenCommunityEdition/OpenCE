//! Every animation a graph lists, on every map, resolves: its index is one
//! of the graph's animations, it has frames, it is the kind its list asks
//! for, and a base animation of a graph with nodes moves each of them.
//! These are the animations the animation stand and play reach through
//! the lists (`seat_and_weapon`, `damage_animation`, the first-person,
//! weapon, vehicle and device lists). Skipped without assets/maps.

use std::error::Error;
use std::path::Path;

use opence_content::{AnimationGraph, AnimationKind};
use opence_import::{import_animation_graph, Cache, GroupTag};
use opence_sim::animation_graph::{
    fits, seat_and_weapon, ListEntry, DEVICE, FIRST_PERSON_WEAPON, UNIT_SEAT, VEHICLE, WEAPON,
    WEAPON_CLASS, WEAPON_TYPE,
};

/// What a sweep of one graph found.
#[derive(Default)]
struct Sweep {
    listed: usize,
    failures: Vec<String>,
}

impl Sweep {
    /// Checks each animation of `list` against `entries` (a damage list
    /// has no entries: any kind fills it).
    fn list(
        &mut self,
        graph: &AnimationGraph,
        name: &str,
        list: &[Option<u16>],
        entries: &[ListEntry],
    ) {
        for (index, animation) in list.iter().enumerate() {
            let Some(animation) = *animation else {
                continue;
            };
            self.listed += 1;
            let slot = entries
                .get(index)
                .map_or(String::from("?"), |entry| entry.name.to_owned());
            let Some(found) = graph.animations.get(usize::from(animation)) else {
                self.failures.push(format!(
                    "{name} '{slot}': animation {animation} of {}",
                    graph.animations.len()
                ));
                continue;
            };
            let kind = entries.get(index).and_then(|entry| entry.kind);
            // (a weapon's or vehicle's graph has no nodes: its model's are
            // the animation's)
            let nodes = if graph.nodes.is_empty() {
                usize::from(found.node_count)
            } else {
                graph.nodes.len()
            };
            if !fits(found, kind, nodes) {
                self.failures.push(format!(
                    "{name} '{slot}' -> '{}': {:?} of {} nodes and {} frames, {kind:?} of {nodes} asked",
                    found.name, found.kind, found.node_count, found.frame_count
                ));
            }
        }
    }
}

#[test]
fn every_listed_animation_resolves_on_every_map() -> Result<(), Box<dyn Error>> {
    let maps = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../assets/maps");
    let Ok(entries) = std::fs::read_dir(&maps) else {
        return Ok(()); // (no game data here)
    };
    let mut paths: Vec<_> = entries
        .filter_map(Result::ok)
        .map(|entry| entry.path())
        .filter(|path| {
            path.extension().is_some_and(|e| e == "map")
                && path.file_stem().is_some_and(|stem| stem != "ui")
        })
        .collect();
    paths.sort();
    let antr = GroupTag::new(b"antr");
    let (mut graphs, mut listed, mut unarmed) = (0, 0, 0);
    let mut failures = Vec::new();
    for path in &paths {
        let cache = Cache::open(std::fs::read(path)?)?;
        let map = path.file_stem().and_then(|s| s.to_str()).unwrap_or("?");
        for (index, tag) in cache.tags().iter().enumerate() {
            if !tag.is(antr) {
                continue;
            }
            let graph = import_animation_graph(&cache, index)?;
            let mut sweep = Sweep::default();
            for seat in &graph.unit_seats {
                let at = format!("seat '{}'", seat.label);
                sweep.list(&graph, &at, &seat.animations, &UNIT_SEAT);
                for class in &seat.weapon_classes {
                    let at = format!("{at} class '{}'", class.label);
                    sweep.list(&graph, &at, &class.animations, &WEAPON_CLASS);
                    for weapon_type in &class.weapon_types {
                        let at = format!("{at} type '{}'", weapon_type.label);
                        sweep.list(&graph, &at, &weapon_type.animations, &WEAPON_TYPE);
                    }
                }
            }
            if graph
                .unit_seats
                .iter()
                .flat_map(|seat| &seat.weapon_classes)
                .any(|class| !class.weapon_types.is_empty())
            {
                // (a unit with weapon types stands in one of them; a
                // first-person or cinematic graph may have seats and classes
                // of none)
                if seat_and_weapon(&graph, None, Some("unarmed")).is_some()
                    || seat_and_weapon(&graph, None, None).is_some()
                {
                    unarmed += 1;
                } else {
                    failures.push(format!("{map} {}: no seat has a weapon", tag.name));
                }
            }
            sweep.list(&graph, "damage", &graph.unit_damage, &[]);
            sweep.list(&graph, "weapon", &graph.weapon, &WEAPON);
            sweep.list(
                &graph,
                "first person",
                &graph.first_person_weapon,
                &FIRST_PERSON_WEAPON,
            );
            sweep.list(&graph, "device", &graph.device, &DEVICE);
            if let Some(vehicle) = &graph.vehicle {
                sweep.list(&graph, "vehicle", &vehicle.animations, &VEHICLE);
                for (index, suspension) in vehicle.suspensions.iter().enumerate() {
                    if let Some(animation) = suspension.animation {
                        let name = format!("suspension {index}");
                        sweep.list(
                            &graph,
                            &name,
                            &[Some(animation)],
                            &[ListEntry {
                                name: "suspension",
                                kind: Some(AnimationKind::Overlay),
                            }],
                        );
                    }
                }
            }
            graphs += 1;
            listed += sweep.listed;
            failures.extend(
                sweep
                    .failures
                    .into_iter()
                    .map(|failure| format!("{map} {}: {failure}", tag.name)),
            );
        }
    }
    if paths.is_empty() {
        return Ok(());
    }
    println!(
        "{} maps, {graphs} graphs, {listed} listed animations, {unarmed} units with a seat, {} failures",
        paths.len(),
        failures.len()
    );
    assert!(graphs > 0 && listed > 0, "{graphs} graphs, {listed} listed");
    assert!(
        failures.is_empty(),
        "{} failures:\n{}",
        failures.len(),
        failures.join("\n")
    );
    Ok(())
}
