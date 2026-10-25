//! How many allocations a frame makes, counted by the engine's allocator
//! (`stat allocations`): a number on an empty world, and on devgulch's AI
//! station no more than its budget (that part needs assets/maps).

use std::path::Path;

use opence_ffi::{Engine, LevelGeometry};
use serde_json::Value;

type Result<T = ()> = std::result::Result<T, Box<dyn std::error::Error>>;

/// The most allocations a frame of devgulch's AI station may make, on
/// average: 823 a frame at first, 708 once the lights kept their rest
/// poses, 151 once the simulation kept its queries' states (see the
/// rust_tools report); the budget is a little over the worst frame, 185.
const BUDGET: f64 = 200.0;

fn run(engine: &Engine, line: &str) -> Result<Value> {
    let output = engine.run_command(line.into());
    if !output.ok {
        return Err(format!("{line}: {}", output.text).into());
    }
    Ok(serde_json::from_str(&output.json)?)
}

/// The last whole frame's allocations and their bytes, by the command.
fn last_frame(engine: &Engine) -> Result<(f64, f64)> {
    let shown = run(engine, "stat allocations")?;
    let count = shown["allocations"].as_f64().ok_or("no allocations")?;
    let bytes = shown["bytes"].as_f64().ok_or("no bytes")?;
    Ok((count, bytes))
}

/// The last whole frame's allocations, read without a command (which
/// publishes a frame of its own for the panel).
#[allow(clippy::cast_precision_loss)]
fn counted() -> (f64, f64) {
    let frame = opence_ffi::allocations::last_frame();
    (frame.allocations as f64, frame.bytes as f64)
}

#[test]
fn an_empty_worlds_frames_are_counted() -> Result {
    let engine = Engine::new();
    engine.start_empty_world();
    for _ in 0..3 {
        engine.advance(1.0 / 60.0)?;
    }
    let output = engine.run_command("stat allocations".into());
    assert!(output.ok, "{}", output.text);
    assert!(output.text.contains("allocations"), "{}", output.text);
    let (count, bytes) = last_frame(&engine)?;
    assert!(count >= 0.0 && bytes >= 0.0);
    Ok(())
}

#[test]
fn a_frame_of_the_ai_station_keeps_to_its_budget() -> Result {
    let maps = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../assets/maps");
    let names = ["bloodgulch.map", "wizard.map"];
    if !names.iter().all(|name| maps.join(name).exists()) {
        return Ok(()); // (no game data here)
    }
    let folder = Path::new(env!("CARGO_TARGET_TMPDIR")).join("allocations_devgulch");
    let _ = std::fs::remove_dir_all(&folder);
    std::fs::create_dir_all(&folder)?;
    for name in names {
        std::os::unix::fs::symlink(maps.join(name).canonicalize()?, folder.join(name))?;
    }
    let cache = opence_import::testing::cache_folder();
    let geometry = LevelGeometry::load(
        folder.join("devgulch.map").to_string_lossy().into_owned(),
        cache.to_string_lossy().into_owned(),
        0,
    )?;
    let engine = Engine::new();
    engine.start_level(geometry, 1)?;
    engine.spawn_player(0)?;
    run(&engine, "station ai")?;
    // (the station's set placed and its first second played: what is made
    // once is not the frame's)
    for _ in 0..60 {
        engine.advance(1.0 / 60.0)?;
    }
    let (mut count, mut bytes, mut most) = (0.0, 0.0, 0.0f64);
    const FRAMES: u32 = 120;
    for _ in 0..FRAMES {
        engine.advance(1.0 / 60.0)?;
        let (frame, frame_bytes) = counted();
        count += frame;
        bytes += frame_bytes;
        most = most.max(frame);
    }
    let (mean, mean_bytes) = (count / f64::from(FRAMES), bytes / f64::from(FRAMES));
    println!("devgulch ai: {mean:.1} allocations a frame ({mean_bytes:.0} bytes), at most {most}");
    assert!(
        mean <= BUDGET,
        "{mean:.1} allocations a frame, over {BUDGET}"
    );
    Ok(())
}
