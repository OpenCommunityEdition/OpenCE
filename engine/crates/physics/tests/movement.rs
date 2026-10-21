//! The player's movement against the reference game's: each of the traces
//! in engine/tests/traces (recorded by port/linux/game/movement_trace.c on
//! bloodgulch at 30 Hz) is played again at 60 Hz with the same controls,
//! and the biped must move as Halo's did. Passes without
//! assets/maps/bloodgulch.map.

use std::error::Error;
use std::path::Path;

use glam::{Vec2, Vec3};
use opence_content::Level;
use opence_import::{import_level, Cache};
use opence_physics::{LevelMesh, MovementInput, Mover, MoverSettings, World, WorldSettings};

/// Where the traces start, before the biped settles.
const START: Vec3 = Vec3::new(87.1, -158.45, 0.4);
/// Engine ticks in each of the trace's.
const TICKS_PER_ROW: usize = 2;

/// A trace's tick: the biped's state after it.
struct Row {
    position: Vec3,
    yaw: f32,
    airborne: bool,
    crouch: f32,
}

fn read_trace(name: &str) -> Result<Vec<Row>, Box<dyn Error>> {
    let path = Path::new(env!("CARGO_MANIFEST_DIR")).join(format!("../../tests/traces/{name}.csv"));
    let text = std::fs::read_to_string(path)?;
    let mut rows = Vec::new();
    for line in text.lines().skip(1) {
        let fields: Vec<f32> = line.split(',').map(str::parse).collect::<Result<_, _>>()?;
        rows.push(Row {
            position: Vec3::new(fields[1], fields[2], fields[3]),
            yaw: fields[7],
            airborne: fields[8] != 0.0,
            crouch: fields[10],
        });
    }
    Ok(rows)
}

/// The script's controls in the trace's tick `tick` (movement_trace.c's).
fn controls(name: &str, tick: usize, yaw: f32) -> MovementInput {
    let mut input = MovementInput {
        yaw,
        ..MovementInput::default()
    };
    match name {
        "run" if tick < 90 => input.throttle = Vec2::X,
        "strafe" if tick < 60 => input.throttle = Vec2::Y,
        "jump" => input.jump = tick == 15,
        "runjump" => {
            input.throttle = Vec2::X;
            input.jump = tick == 30;
        }
        "crouch" => {
            input.crouch = true;
            if tick < 90 {
                input.throttle = Vec2::X;
            }
        }
        "ramp" => input.throttle = Vec2::X,
        _ => {}
    }
    input
}

/// bloodgulch, its world and its player's biped.
type Bloodgulch = (Level, World, Mover);

/// bloodgulch, its world and the player's biped standing settled at the
/// traces' start; none without the map.
fn bloodgulch() -> Result<Option<Bloodgulch>, Box<dyn Error>> {
    let path = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../assets/maps/bloodgulch.map");
    let Ok(file) = std::fs::read(&path) else {
        return Ok(None); // (no game data here)
    };
    let level = import_level(&Cache::open(file)?)?;
    let settings = WorldSettings::default();
    let mut world = World::new(settings)?;
    let bsp = level.structure_bsps.first().ok_or("no structure bsp")?;
    world.set_level(&LevelMesh::from_collision(&bsp.collision))?;
    let unit = level
        .globals
        .multiplayer_unit
        .as_ref()
        .ok_or("no multiplayer unit")?;
    let biped = level
        .objects
        .iter()
        .find(|object| object.tag == *unit)
        .and_then(|object| object.biped)
        .ok_or("the multiplayer unit is no biped")?;
    let mover_settings = MoverSettings {
        biped,
        player: level.globals.player,
        gravity: -settings.gravity.z,
    };
    let mut mover = Mover::new(&mut world, mover_settings, START)?;
    let still = MovementInput {
        yaw: -0.35657,
        ..MovementInput::default()
    };
    for _ in 0..90 {
        mover.step(&mut world, &still)?;
    }
    Ok(Some((level, world, mover)))
}

/// How far the replay strayed from the trace: the most, across its rows,
/// horizontally and vertically (each from its own start), and the rows
/// whose airborne flag differs.
struct Strayed {
    horizontal: f32,
    vertical: f32,
    airborne_rows: usize,
    crouch: f32,
    peak: f32,
    trace_peak: f32,
}

fn replay(name: &str) -> Result<Option<Strayed>, Box<dyn Error>> {
    let Some((_level, mut world, mut mover)) = bloodgulch()? else {
        return Ok(None);
    };
    let rows = read_trace(name)?;
    let mut strayed = Strayed {
        horizontal: 0.0,
        vertical: 0.0,
        airborne_rows: 0,
        crouch: 0.0,
        peak: f32::MIN,
        trace_peak: f32::MIN,
    };
    // (the fall's biped is lifted before its first row, which it starts at)
    let first = if name == "fall" {
        mover.teleport(&mut world, mover.position + Vec3::new(0.0, 0.0, 3.0))?;
        1
    } else {
        0
    };
    let mut start = mover.position;
    for (tick, row) in rows.iter().enumerate().skip(first) {
        let input = controls(name, tick, row.yaw);
        for _ in 0..TICKS_PER_ROW {
            mover.step(&mut world, &input)?;
        }
        if tick == 0 {
            start = mover.position;
        }
        let ours = mover.position - start;
        let theirs = row.position - rows[0].position;
        if std::env::var_os("MOVEMENT_DUMP").is_some() && tick % 5 == 0 {
            eprintln!(
                "{tick}: ours {ours:.3} v {:.3} n {:.3} air {} | theirs {theirs:.3} air {}",
                mover.velocity, mover.ground_normal, mover.airborne, row.airborne
            );
        }
        strayed.horizontal = strayed
            .horizontal
            .max((ours.truncate() - theirs.truncate()).length());
        strayed.vertical = strayed.vertical.max((ours.z - theirs.z).abs());
        strayed.airborne_rows += usize::from(mover.airborne != row.airborne);
        // (Halo's crouch moves the tick after the control's: its row is the
        // one after ours)
        if let Some(next) = rows.get(tick + 1) {
            strayed.crouch = strayed.crouch.max((mover.crouch - next.crouch).abs());
        }
        strayed.peak = strayed.peak.max(ours.z);
        strayed.trace_peak = strayed.trace_peak.max(theirs.z);
    }
    eprintln!(
        "{name}: {:.4} horizontally, {:.4} vertically, {} rows airborne apart, crouch {:.3}, peak {:.4} (Halo's {:.4})",
        strayed.horizontal,
        strayed.vertical,
        strayed.airborne_rows,
        strayed.crouch,
        strayed.peak,
        strayed.trace_peak
    );
    Ok(Some(strayed))
}

/// The run's and the strafe's distances, within a tenth of a unit of
/// Halo's after seconds of running (its 30 Hz steps start and stop a
/// little differently).
fn check_ground(name: &str) -> Result<(), Box<dyn Error>> {
    let Some(strayed) = replay(name)? else {
        return Ok(());
    };
    assert!(strayed.horizontal < 0.1, "{name}: {}", strayed.horizontal);
    assert!(strayed.vertical < 0.05, "{name}: {}", strayed.vertical);
    assert!(
        strayed.airborne_rows <= 2,
        "{name}: {}",
        strayed.airborne_rows
    );
    Ok(())
}

#[test]
fn running_matches_halo() -> Result<(), Box<dyn Error>> {
    check_ground("run")
}

#[test]
fn strafing_matches_halo() -> Result<(), Box<dyn Error>> {
    check_ground("strafe")
}

#[test]
fn crouching_matches_halo() -> Result<(), Box<dyn Error>> {
    let Some(strayed) = replay("crouch")? else {
        return Ok(());
    };
    assert!(strayed.horizontal < 0.1, "{}", strayed.horizontal);
    assert!(strayed.crouch < 0.1, "{}", strayed.crouch);
    Ok(())
}

/// The ramp trace is not a reference: the recorded biped's yaw swings a
/// fifth of a radian each tick and turns half a circle, not the quarter
/// turn movement_trace.c's facing delta asks for, so its path cannot be
/// played again. The biped turns as the script meant instead, a quarter
/// turn over six seconds from the traces' start, across bloodgulch's
/// slopes: head on into a bank too steep to stand on, which stops it as a
/// wall does, then along the bank's walkable slope. It never leaves the
/// ground, gets past the bank, and on the slopes, climbing at the speed
/// the slope scale leaves it (0.9 wu/s up the flank), never stalls.
#[test]
fn running_across_the_slopes() -> Result<(), Box<dyn Error>> {
    let Some((level, mut world, mut mover)) = bloodgulch()? else {
        return Ok(());
    };
    let seconds = 6.0;
    let ticks = (seconds * 60.0) as usize;
    let mut path = 0.0;
    let mut airborne = 0;
    let mut slowest_on_slopes = f32::MAX;
    let mut on_slope = 0;
    for tick in 0..ticks {
        let input = MovementInput {
            yaw: -0.35657 + std::f32::consts::FRAC_PI_2 * tick as f32 / ticks as f32,
            throttle: Vec2::X,
            ..MovementInput::default()
        };
        let before = mover.position;
        mover.step(&mut world, &input)?;
        let moved = (mover.position - before).length();
        path += moved;
        airborne += usize::from(mover.airborne);
        // (a slope's: the bank's flank, k 0.78, against the floor's 1; from
        // a tenth of a second on it, past the step onto it)
        on_slope = if mover.ground_normal.z < 0.95 {
            on_slope + 1
        } else {
            0
        };
        if on_slope > 6 {
            slowest_on_slopes = slowest_on_slopes.min(moved * 60.0);
        }
    }
    let speed = level.globals.player.run_forward;
    eprintln!("slopes: {path:.3} along the path, slowest on slopes {slowest_on_slopes:.3} wu/s, {airborne} ticks airborne");
    assert_eq!(airborne, 0);
    assert!(path > 0.6 * speed * seconds, "{path} against {speed}");
    assert!(slowest_on_slopes < f32::MAX, "never on a slope");
    assert!(slowest_on_slopes > 0.3 * speed, "{slowest_on_slopes}");
    Ok(())
}

#[test]
fn jumping_matches_halo() -> Result<(), Box<dyn Error>> {
    for name in ["jump", "runjump"] {
        let Some(strayed) = replay(name)? else {
            return Ok(());
        };
        assert!(
            (strayed.peak - strayed.trace_peak).abs() < 0.04,
            "{name}: {} against {}",
            strayed.peak,
            strayed.trace_peak
        );
        assert!(strayed.vertical < 0.06, "{name}: {}", strayed.vertical);
        assert!(
            strayed.airborne_rows <= 3,
            "{name}: {}",
            strayed.airborne_rows
        );
    }
    Ok(())
}

#[test]
fn falling_matches_halo() -> Result<(), Box<dyn Error>> {
    let Some(strayed) = replay("fall")? else {
        return Ok(());
    };
    assert!(strayed.vertical < 0.06, "{}", strayed.vertical);
    assert!(strayed.airborne_rows <= 2, "{}", strayed.airborne_rows);
    Ok(())
}
