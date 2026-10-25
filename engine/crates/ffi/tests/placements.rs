//! The engine's world after a level starts, against the reference game's:
//! each stock map's placements, as `tools/engine_placement_traces.py`
//! recorded them (to `build/placement_traces`), are made in the same places
//! and facing the same ways, those of the structure BSP a mission's scripts
//! switched to as well. It passes without assets/maps or the traces.

use std::collections::BTreeMap;
use std::path::{Path, PathBuf};

use opence_ffi::{Engine, LevelGeometry};
use serde_json::Value;

type Result<T = ()> = std::result::Result<T, Box<dyn std::error::Error>>;

/// How far a placement may be from the reference's (world units: 1 mm).
const DISTANCE: f64 = 0.001;
/// How far its forward or up may turn from the reference's (0.01°).
const ANGLE: f64 = 0.01;

/// One placed object: its tag, place, forward and up.
#[derive(Clone, Debug)]
struct Placed {
    tag: String,
    position: [f64; 3],
    forward: [f64; 3],
    up: [f64; 3],
    /// Whether it is a vehicle.
    vehicle: bool,
    /// Where it is in the trace or the world (its row or its id).
    at: String,
}

/// A CSV row's cells (a cell in quotes may have commas and doubled quotes).
fn cells(line: &str) -> Vec<String> {
    let mut cells = vec![String::new()];
    let mut quoted = false;
    let mut characters = line.chars().peekable();
    while let Some(character) = characters.next() {
        match (character, quoted) {
            ('"', true) if characters.peek() == Some(&'"') => {
                characters.next();
                if let Some(cell) = cells.last_mut() {
                    cell.push('"');
                }
            }
            ('"', _) => quoted = !quoted,
            (',', false) => cells.push(String::new()),
            (character, _) => {
                if let Some(cell) = cells.last_mut() {
                    cell.push(character);
                }
            }
        }
    }
    cells
}

/// A trace's objects.
fn trace(path: &Path) -> Result<Vec<Placed>> {
    let text = std::fs::read_to_string(path)?;
    let mut lines = text.lines();
    let header = cells(lines.next().ok_or("empty trace")?);
    let column = |name: &str| {
        header
            .iter()
            .position(|cell| cell == name)
            .ok_or(format!("no {name}"))
    };
    let (x, forward, up, tag, kind, block) = (
        column("x")?,
        column("forward_i")?,
        column("up_i")?,
        column("tag")?,
        column("type")?,
        column("block_index")?,
    );
    let mut placed = Vec::new();
    for line in lines {
        let row = cells(line);
        let number =
            |at: usize| -> Result<f64> { Ok(row.get(at).ok_or("short row")?.parse::<f64>()?) };
        let three =
            |at: usize| -> Result<[f64; 3]> { Ok([number(at)?, number(at + 1)?, number(at + 2)?]) };
        placed.push(Placed {
            tag: row.get(tag).ok_or("short row")?.clone(),
            position: three(x)?,
            forward: three(forward)?,
            up: three(up)?,
            // (the game's type 1)
            vehicle: row[kind] == "1",
            at: format!("type {} block {}", row[kind], row[block]),
        });
    }
    Ok(placed)
}

/// `vector` turned by the quaternion `[x, y, z, w]`.
fn rotate([x, y, z, w]: [f64; 4], vector: [f64; 3]) -> [f64; 3] {
    let u = [x, y, z];
    let cross = |a: [f64; 3], b: [f64; 3]| {
        [
            a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0],
        ]
    };
    let t = cross(u, vector).map(|value| 2.0 * value);
    let c = cross(u, t);
    [0, 1, 2].map(|axis| vector[axis] + w * t[axis] + c[axis])
}

/// The engine's objects once `map` starts at normal, as the reference
/// game's co-op and multiplayer games start, and, if the reference's
/// scripts switched to the structure BSP `bsp`, those made as the engine
/// switches to it too (each object once, whether it is there still).
fn world(map: &Path, cache: &Path, bsp: u32) -> Result<Vec<Placed>> {
    let geometry = LevelGeometry::load(
        map.to_string_lossy().into_owned(),
        cache.to_string_lossy().into_owned(),
        0,
    )?;
    let engine = Engine::new();
    engine.start_level(geometry, 1)?;
    let mut placed = objects(&engine)?;
    if bsp != 0 {
        let switched = engine.run_command(format!("bsp {bsp}"));
        if !switched.ok {
            return Err(switched.text.into());
        }
        // (a deleted object's index is used again, so an object is the
        // same one only if its tag and place are the same too)
        let before: Vec<(String, String, [u64; 3])> = placed.iter().map(identity).collect();
        placed.extend(
            objects(&engine)?
                .into_iter()
                .filter(|object| !before.contains(&identity(object))),
        );
    }
    Ok(placed)
}

/// An object of the engine's, by its id, tag and place.
fn identity(object: &Placed) -> (String, String, [u64; 3]) {
    (
        object.at.clone(),
        object.tag.clone(),
        object.position.map(f64::to_bits),
    )
}

/// The engine's objects now.
fn objects(engine: &Engine) -> Result<Vec<Placed>> {
    let listed = engine.run_command("objects".into());
    let listed: Value = serde_json::from_str(&listed.json)?;
    let mut placed = Vec::new();
    for object in listed.as_array().ok_or("no objects")? {
        let id = object["id"].as_i64().ok_or("no id")?;
        let shown = engine.run_command(format!("inspect {id}"));
        let shown: Value = serde_json::from_str(&shown.json)?;
        let number = |value: &Value| value.as_f64().unwrap_or(f64::NAN);
        let position = &shown["position"];
        let rotation = &shown["rotation"];
        let quaternion = [0, 1, 2, 3].map(|at| number(&rotation[at]));
        placed.push(Placed {
            tag: shown["tag"].as_str().unwrap_or("").to_owned(),
            position: [
                number(&position[0]),
                number(&position[1]),
                number(&position[2]),
            ],
            forward: rotate(quaternion, [1.0, 0.0, 0.0]),
            up: rotate(quaternion, [0.0, 0.0, 1.0]),
            vehicle: shown["kind"] == "vehicle",
            at: format!("object {id} {}", shown["kind"].as_str().unwrap_or("?")),
        });
    }
    Ok(placed)
}

fn distance(a: [f64; 3], b: [f64; 3]) -> f64 {
    libm::sqrt(
        (0..3)
            .map(|axis| (a[axis] - b[axis]) * (a[axis] - b[axis]))
            .sum::<f64>(),
    )
}

/// The angle between two unit vectors, in degrees (from their chord:
/// acos is coarse near 0).
fn angle(a: [f64; 3], b: [f64; 3]) -> f64 {
    2.0 * (distance(a, b) / 2.0).clamp(0.0, 1.0).asin().to_degrees()
}

/// Each way the world differs from the trace: an object it lacks or has
/// over, or one placed or turned differently.
fn differences(map: &str, expected: &[Placed], got: &[Placed]) -> Vec<String> {
    let mut left: BTreeMap<&str, Vec<&Placed>> = BTreeMap::new();
    for object in got {
        left.entry(object.tag.as_str()).or_default().push(object);
    }
    let mut found = Vec::new();
    for want in expected {
        let candidates = left.entry(want.tag.as_str()).or_default();
        let nearest = candidates
            .iter()
            .enumerate()
            .min_by(|(_, a), (_, b)| {
                distance(a.position, want.position).total_cmp(&distance(b.position, want.position))
            })
            .map(|(index, _)| index);
        let Some(nearest) = nearest else {
            found.push(format!("{map}: {} {} is missing", want.at, want.tag));
            continue;
        };
        let have = candidates.swap_remove(nearest);
        let off = distance(have.position, want.position);
        let forward = angle(have.forward, want.forward);
        let up = angle(have.up, want.up);
        if off > DISTANCE || forward > ANGLE || up > ANGLE {
            found.push(format!(
                "{map}: {} {} ({}): {off:.4} away {:?} for {:?}, forward {forward:.3}° up {up:.3}° off",
                want.at, want.tag, have.at, have.position, want.position
            ));
        }
    }
    for (tag, extra) in left {
        for have in extra {
            found.push(format!("{map}: {} {tag} is not the reference's", have.at));
        }
    }
    found
}

/// Whether the stock map `name` is one of the campaign's (a10 to d40).
fn is_campaign(name: &str) -> bool {
    let name = name.as_bytes();
    name.len() == 3 && (b'a'..=b'd').contains(&name[0]) && name[1..].iter().all(u8::is_ascii_digit)
}

#[test]
fn every_stock_maps_placements_match_the_reference_games() -> Result {
    let root = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../..");
    let maps = root.join("assets/maps");
    let traces = root.join("build/placement_traces");
    let cache = PathBuf::from(env!("CARGO_TARGET_TMPDIR")).join("placements_cache");
    std::fs::create_dir_all(&cache)?;
    let Ok(listed) = std::fs::read_dir(&traces) else {
        return Ok(()); // (no traces here)
    };
    let mut names: Vec<String> = listed
        .filter_map(|entry| entry.ok())
        .filter_map(|entry| {
            entry
                .file_name()
                .to_str()?
                .strip_suffix(".csv")
                .map(str::to_owned)
        })
        .collect();
    names.sort();
    let mut found = Vec::new();
    let mut compared = 0;
    for name in names {
        let map = maps.join(format!("{name}.map"));
        if !map.exists() {
            continue;
        }
        let mut expected = trace(&traces.join(format!("{name}.csv")))?;
        // (the structure BSP the reference was in at the end; 0 if the
        // trace has none)
        let bsp = std::fs::read_to_string(traces.join(format!("{name}.bsp")))
            .ok()
            .and_then(|text| text.trim().parse().ok())
            .unwrap_or(0);
        let mut got = world(&map, &cache, bsp)?;
        if !is_campaign(&name) {
            // (a multiplayer game places the vehicles of its gametype's
            // vehicle set, which the reference's traces were recorded in
            // slayer; the engine's level starts with no gametype)
            expected.retain(|placed| !placed.vehicle);
            got.retain(|placed| !placed.vehicle);
        }
        compared += expected.len();
        found.extend(differences(&name, &expected, &got));
    }
    assert!(
        found.is_empty(),
        "{} differences in {compared} placements:\n{}",
        found.len(),
        found.join("\n")
    );
    Ok(())
}

#[test]
fn the_comparison_finds_each_difference() {
    let placed = |tag: &str, x: f64, yaw: f64| Placed {
        tag: tag.into(),
        position: [x, 0.0, 0.0],
        forward: [libm::cos(yaw), libm::sin(yaw), 0.0],
        up: [0.0, 0.0, 1.0],
        vehicle: false,
        at: "here".into(),
    };
    let expected = [
        placed("a", 0.0, 0.0),
        placed("a", 5.0, 0.0),
        placed("b", 1.0, 1.0),
        placed("c", 0.0, 0.0),
    ];
    let same = [
        placed("a", 5.0, 0.0),
        placed("a", 0.0, 0.0),
        placed("b", 1.0, 1.0),
        placed("c", 0.0, 0.0),
    ];
    assert!(differences("m", &expected, &same).is_empty());
    assert!(is_campaign("a10") && is_campaign("d40"));
    assert!(!is_campaign("bloodgulch") && !is_campaign("e10") && !is_campaign("a1"));
    let got = [
        placed("a", 5.0005, 0.0),
        placed("a", 0.01, 0.0),
        placed("b", 1.0, 1.01),
        placed("d", 0.0, 0.0),
    ];
    let found = differences("m", &expected, &got);
    assert_eq!(found.len(), 4, "{found:#?}");
    assert!(found.iter().any(|line| line.contains("c is missing")));
    assert!(found
        .iter()
        .any(|line| line.contains("d is not the reference's")));
    assert!(found.iter().any(|line| line.contains("0.0100 away")));
    assert!(found.iter().any(|line| line.contains("forward 0.573°")));
    assert_eq!(cells("1,\"a, \"\"b\"\"\",c"), ["1", "a, \"b\"", "c"]);
    assert!((angle([1.0, 0.0, 0.0], [-1.0, 0.0, 0.0]) - 180.0).abs() < 1e-9);
    assert!(angle([1.0, 0.0, 0.0], [1.0, 0.0, 0.0]).abs() < 1e-12);
}
