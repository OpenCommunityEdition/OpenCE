//! The reference game's vehicle traces in engine/tests/traces (recorded by
//! port/linux/game/movement_trace.c on bloodgulch at 30 Hz, with
//! tools/engine_movement_traces.py): each is whole, and shows what its
//! script of inputs was for, so that the driving and flying controllers are
//! held to traces that drive, turn, brake and leave the ground. Passes
//! without the maps.

use std::error::Error;
use std::path::Path;

type Result = std::result::Result<(), Box<dyn Error>>;

/// The vehicle traces' columns (movement_trace.c's "vehicle trace:" line).
const COLUMNS: [&str; 19] = [
    "tick",
    "x",
    "y",
    "z",
    "vx",
    "vy",
    "vz",
    "forward_i",
    "forward_j",
    "forward_k",
    "up_i",
    "up_j",
    "up_k",
    "wx",
    "wy",
    "wz",
    "speed",
    "turn",
    "airborne_ticks",
];

/// Each script and its ticks.
const SCRIPTS: [(&str, usize); 7] = [
    ("warthog_drive", 150),
    ("warthog_turn", 120),
    ("warthog_brake", 150),
    ("warthog_ramp", 300),
    ("ghost_drive", 150),
    ("ghost_turn", 120),
    ("ghost_brake", 150),
];

/// The Warthog's tag (vehicles\warthog\warthog), a tick's of the game's 30.
const WARTHOG_FORWARD: f32 = 0.255;
const WARTHOG_REVERSE: f32 = 0.1;
const WARTHOG_ACCELERATION: f32 = 0.00275;
const WARTHOG_DECELERATION: f32 = 0.011;

/// A trace's ticks, each its columns' values.
fn read(name: &str) -> std::result::Result<Vec<[f32; 19]>, Box<dyn Error>> {
    let path = Path::new(env!("CARGO_MANIFEST_DIR")).join(format!("../../tests/traces/{name}.csv"));
    let text = std::fs::read_to_string(path)?;
    let mut lines = text.lines();
    let header: Vec<&str> = lines.next().ok_or("no header")?.split(',').collect();
    assert_eq!(header, COLUMNS, "{name}'s columns");
    let mut rows = Vec::new();
    for line in lines {
        let values: Vec<f32> = line
            .split(',')
            .map(str::parse)
            .collect::<std::result::Result<_, _>>()?;
        let row: [f32; 19] = values
            .try_into()
            .map_err(|_| format!("{name}: a row of the wrong length"))?;
        rows.push(row);
    }
    Ok(rows)
}

fn column(name: &str) -> usize {
    COLUMNS
        .iter()
        .position(|column| *column == name)
        .unwrap_or(0)
}

fn heading(row: &[f32; 19]) -> f32 {
    libm::atan2f(row[column("forward_j")], row[column("forward_i")])
}

/// The heading's change from the first tick to the last, unwound.
fn turned(rows: &[[f32; 19]]) -> f32 {
    let pi = std::f32::consts::PI;
    let mut total = 0.0;
    for pair in rows.windows(2) {
        let mut step = heading(&pair[1]) - heading(&pair[0]);
        if step > pi {
            step -= 2.0 * pi;
        } else if step < -pi {
            step += 2.0 * pi;
        }
        total += step;
    }
    total
}

fn travelled(rows: &[[f32; 19]]) -> f32 {
    let (x, y) = (column("x"), column("y"));
    rows.windows(2)
        .map(|pair| libm::hypotf(pair[1][x] - pair[0][x], pair[1][y] - pair[0][y]))
        .sum()
}

#[test]
fn every_vehicle_trace_is_whole() -> Result {
    for (name, ticks) in SCRIPTS {
        let rows = read(name)?;
        assert_eq!(rows.len(), ticks + 1, "{name}'s ticks");
        for (tick, row) in rows.iter().enumerate() {
            assert_eq!(row[0] as usize, tick, "{name}'s tick {tick}");
            assert!(
                row.iter().all(|value| value.is_finite()),
                "{name}'s tick {tick}"
            );
        }
    }
    Ok(())
}

#[test]
fn the_warthogs_speed_follows_its_tag() -> Result {
    let rows = read("warthog_drive")?;
    let speed = column("speed");
    // (forward for three seconds: its acceleration a tick, to its fastest)
    for (tick, row) in rows.iter().enumerate().take(90) {
        let expected = (WARTHOG_ACCELERATION * (tick + 1) as f32).min(WARTHOG_FORWARD);
        assert!(
            (row[speed] - expected).abs() < 1e-4,
            "tick {tick}: {}",
            row[speed]
        );
    }
    // (then let go: its deceleration a tick, to a stop)
    for pair in rows[90..].windows(2) {
        let step = pair[0][speed] - pair[1][speed];
        assert!(
            step.abs() < 1e-6
                || (step - WARTHOG_DECELERATION).abs() < 1e-4
                || pair[1][speed] == 0.0
        );
    }
    assert_eq!(rows[rows.len() - 1][speed], 0.0);
    assert!(travelled(&rows) > 10.0);
    Ok(())
}

#[test]
fn the_warthog_brakes_to_its_fastest_reverse() -> Result {
    let rows = read("warthog_brake")?;
    let speed = column("speed");
    let slowest = rows
        .iter()
        .map(|row| row[speed])
        .fold(f32::INFINITY, f32::min);
    assert!((slowest + WARTHOG_REVERSE).abs() < 1e-4, "{slowest}");
    Ok(())
}

#[test]
fn the_vehicles_turn_as_their_drivers_look() -> Result {
    // (the driver looks a quarter turn left over three seconds)
    let warthog = turned(&read("warthog_turn")?);
    let ghost = turned(&read("ghost_turn")?);
    assert!(warthog.abs() > std::f32::consts::FRAC_PI_2, "{warthog}");
    assert!(ghost > 0.5, "{ghost}");
    Ok(())
}

#[test]
fn the_warthog_leaves_the_ground_over_the_hills() -> Result {
    let rows = read("warthog_ramp")?;
    let airborne = column("airborne_ticks");
    // (three jumps, the last still in the air as the trace ends)
    let jumps = rows.iter().filter(|row| row[airborne] == 1.0).count();
    let longest = rows.iter().map(|row| row[airborne]).fold(0.0, f32::max);
    assert_eq!(jumps, 3);
    assert!(longest >= 20.0, "{longest}");
    Ok(())
}

#[test]
fn the_ghost_drives_and_brakes() -> Result {
    let drive = travelled(&read("ghost_drive")?);
    let brake = travelled(&read("ghost_brake")?);
    assert!(drive > 15.0, "{drive}");
    assert!(brake < drive, "{brake} {drive}");
    Ok(())
}
