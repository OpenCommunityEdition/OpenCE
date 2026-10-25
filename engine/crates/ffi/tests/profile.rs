//! The profiler's commands, `stat frame` and `trace`, over an empty world
//! (in their own test binary: the profiler is one for the process).

use std::path::Path;

use opence_ffi::Engine;

type Result = std::result::Result<(), Box<dyn std::error::Error>>;

fn run(engine: &Engine, line: &str) -> std::result::Result<String, String> {
    let output = engine.run_command(line.into());
    if output.ok {
        Ok(output.text)
    } else {
        Err(format!("`{line}`: {}", output.text))
    }
}

fn refused(engine: &Engine, line: &str) -> std::result::Result<String, String> {
    let output = engine.run_command(line.into());
    if output.ok {
        Err(format!("`{line}` was not refused: {}", output.text))
    } else {
        Ok(output.text)
    }
}

fn frames(engine: &Engine, count: u32) -> std::result::Result<(), opence_ffi::EngineError> {
    for _ in 0..count {
        engine.advance(1.0 / 30.0)?;
    }
    Ok(())
}

/// The names of a trace file's scopes.
fn traced(path: &Path) -> std::result::Result<Vec<String>, Box<dyn std::error::Error>> {
    let trace: serde_json::Value = serde_json::from_str(&std::fs::read_to_string(path)?)?;
    let events = trace["traceEvents"].as_array().ok_or("no traceEvents")?;
    Ok(events
        .iter()
        .filter_map(|event| event["name"].as_str().map(str::to_owned))
        .collect())
}

#[test]
fn frames_are_measured_and_traced() -> Result {
    let engine = Engine::new();
    engine.start_empty_world();

    // stat frame: on, then the last frame's scopes, then off
    assert!(run(&engine, "stat frame")?.contains("measured from the next"));
    frames(&engine, 3)?;
    let shown = engine.run_command("stat frame".into());
    assert!(shown.ok, "{}", shown.text);
    assert!(shown.text.contains("advance"), "{}", shown.text);
    let data: serde_json::Value = serde_json::from_str(&shown.json)?;
    let names: Vec<&str> = data
        .as_array()
        .ok_or("no lines")?
        .iter()
        .filter_map(|line| line["name"].as_str())
        .collect();
    for name in ["advance", "ticks", "tick", "publish"] {
        assert!(names.contains(&name), "{name} in {names:?}");
    }
    assert_eq!(run(&engine, "stat frame off")?, "frames are not measured");

    // trace: its file, its frames
    let folder = Path::new(env!("CARGO_TARGET_TMPDIR")).join("profile_trace");
    std::fs::create_dir_all(&folder)?;
    let file = folder.join("frames.json");
    let _ = std::fs::remove_file(&file);
    assert!(refused(&engine, "trace frames.txt")?.contains(".json"));
    assert!(refused(&engine, &format!("trace {} 0", file.display()))?.contains("1 to"));
    assert!(refused(&engine, "trace stop")?.contains("no trace"));
    run(&engine, &format!("trace {} 2", file.display()))?;
    // (the app's own scopes go into it too)
    assert!(engine.profile_begin("render".into()));
    engine.profile_end();
    frames(&engine, 3)?;
    let names = traced(&file)?;
    for name in ["advance", "tick", "render"] {
        assert!(names.iter().any(|traced| traced == name), "{name}");
    }

    // trace stop writes what it has
    std::fs::remove_file(&file)?;
    run(&engine, &format!("trace {} 100", file.display()))?;
    frames(&engine, 2)?;
    assert!(run(&engine, "trace stop")?.contains("traced into"));
    assert!(traced(&file)?.iter().any(|name| name == "advance"));
    // (nothing measured now: a scope is not taken)
    assert!(!engine.profile_begin("render".into()));

    // the dev panel's timings: measured, the last frame's scopes, then none
    assert!(engine.frame_scopes().is_empty());
    engine.measure_frames(true);
    frames(&engine, 2)?;
    let scopes = engine.frame_scopes();
    let advance = scopes
        .iter()
        .find(|scope| scope.name == "advance")
        .ok_or("no advance")?;
    assert_eq!(advance.depth, 0);
    assert!(
        advance.milliseconds >= advance.own_milliseconds,
        "{advance:?}"
    );
    assert!(scopes
        .iter()
        .any(|scope| scope.name == "tick" && scope.depth > 0));
    engine.measure_frames(false);
    assert!(engine.frame_scopes().is_empty());
    Ok(())
}
