//! The developer shell's commands, each changing a headless world as it
//! says: over an empty world, and over bloodgulch (those pass without
//! assets/maps/bloodgulch.map).

use std::path::Path;
use std::sync::Arc;

use opence_ffi::devtools::{ShellAction, ShellOutput};
use opence_ffi::{Engine, LevelGeometry};

type Result = std::result::Result<(), Box<dyn std::error::Error>>;

fn maps() -> std::path::PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../assets/maps")
}

fn bloodgulch() -> Option<Arc<LevelGeometry>> {
    let map = maps().join("bloodgulch.map");
    if !map.exists() {
        return None; // (no game data here)
    }
    let cache = Path::new(env!("CARGO_TARGET_TMPDIR")).join("devtools_cache");
    std::fs::create_dir_all(&cache).ok()?;
    LevelGeometry::load(
        map.to_string_lossy().into_owned(),
        cache.to_string_lossy().into_owned(),
        0,
    )
    .ok()
}

fn run(engine: &Engine, line: &str) -> std::result::Result<ShellOutput, String> {
    let output = engine.run_command(line.into());
    if output.ok {
        Ok(output)
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

fn json(output: &ShellOutput) -> std::result::Result<serde_json::Value, serde_json::Error> {
    serde_json::from_str(&output.json)
}

/// The ticks ten frames of a thirtieth of a second run.
fn ten_frames(engine: &Engine) -> std::result::Result<u32, opence_ffi::EngineError> {
    let mut ticks = 0;
    for _ in 0..10 {
        ticks += engine.advance(1.0 / 30.0)?;
    }
    Ok(ticks)
}

#[test]
fn time_pause_and_step() -> Result {
    let engine = Engine::new();
    refused(&engine, "step")?; // (no world)
    engine.start_empty_world();
    assert_eq!(ten_frames(&engine)?, 20);
    run(&engine, "time 2")?;
    assert_eq!(ten_frames(&engine)?, 40);
    run(&engine, "time 0.5")?;
    assert_eq!(ten_frames(&engine)?, 10);
    refused(&engine, "time 99")?;
    refused(&engine, "time -1")?;
    run(&engine, "time 1; pause")?;
    assert!(engine.paused());
    assert_eq!(ten_frames(&engine)?, 0);
    let stepped = run(&engine, "step 10")?;
    assert_eq!(json(&stepped)?, 20 + 40 + 10 + 10);
    run(&engine, "step")?;
    refused(&engine, "step 0")?;
    refused(&engine, "step 100000")?;
    run(&engine, "pause off")?;
    assert!(!engine.paused());
    assert_eq!(ten_frames(&engine)?, 20);
    Ok(())
}

#[test]
fn the_app_does_what_the_engine_cannot() -> Result {
    let engine = Engine::new();
    assert_eq!(run(&engine, "quit")?.action, ShellAction::Quit);
    refused(&engine, "map nowhere_at_all")?;
    let folder = std::env::temp_dir().join(format!("opence_devtools_{}", std::process::id()));
    std::fs::create_dir_all(&folder)?;
    std::fs::write(folder.join("test.map"), b"not really")?;
    engine.set_maps_folder(folder.to_string_lossy().into_owned());
    let loading = run(&engine, "map test 1")?;
    let path = folder.join("test.map").to_string_lossy().into_owned();
    assert_eq!(loading.action, ShellAction::LoadMap { path, bsp: 1 });
    // (the action is the line's only)
    assert_eq!(run(&engine, "help map")?.action, ShellAction::None);
    refused(&engine, "map test -1")?;
    // (the test map is made, so it needs no file)
    let made = run(&engine, "map devmap")?;
    let path = folder.join("devmap.map").to_string_lossy().into_owned();
    assert_eq!(made.action, ShellAction::LoadMap { path, bsp: 0 });
    std::fs::remove_dir_all(&folder)?;
    Ok(())
}

#[test]
fn the_screen_passes_can_be_switched_off_and_shown_alone() -> Result {
    let engine = Engine::new();
    for name in [
        "render.gtao",
        "render.ssr",
        "render.contact_shadows",
        "render.sun_shadows",
        "render.lights",
    ] {
        assert!(engine.shell_flag(name.into()), "{name} is on by default");
        run(&engine, &format!("{name} off"))?;
        assert!(!engine.shell_flag(name.into()), "{name} stayed on");
        assert_eq!(
            engine.saved_shell_variables().get(name).map(String::as_str),
            Some("false"),
            "{name} is kept between runs"
        );
    }
    assert_eq!(engine.shell_value("render.view".into()), "frame");
    for view in ["depth", "normals", "ao", "contact", "ssr", "frame"] {
        run(&engine, &format!("render.view {view}"))?;
        assert_eq!(engine.shell_value("render.view".into()), view);
    }
    refused(&engine, "render.view albedo")?;
    assert_eq!(engine.shell_value("render.view".into()), "frame");
    assert_eq!(engine.shell_value("no.such.variable".into()), "");
    // (the view is not kept between runs)
    run(&engine, "render.view ao")?;
    assert!(!engine.saved_shell_variables().contains_key("render.view"));
    Ok(())
}

#[test]
fn variables_completion_and_bindings() -> Result {
    let engine = Engine::new();
    assert!(!engine.shell_flag("render.wireframe".into()));
    run(&engine, "render.wireframe on")?;
    assert!(engine.shell_flag("render.wireframe".into()));
    engine.set_shell_variable("render.lightmaps".into(), "off".into())?;
    assert!(!engine.shell_flag("render.lightmaps".into()));
    assert!(engine
        .set_shell_variable("render.lightmaps".into(), "maybe".into())
        .is_err());
    let saved = engine.saved_shell_variables();
    assert_eq!(
        saved.get("render.lightmaps").map(String::as_str),
        Some("false")
    );
    // (wireframe is not kept between runs)
    assert!(!saved.contains_key("render.wireframe"));
    let other = Engine::new();
    assert!(other.load_shell_variables(saved).is_empty());
    assert!(!other.shell_flag("render.lightmaps".into()));
    let variables = engine.shell_variables();
    let wireframe = variables
        .iter()
        .find(|variable| variable.name == "render.wireframe")
        .ok_or("no render.wireframe")?;
    assert_eq!(
        (wireframe.value.as_str(), wireframe.default_value.as_str()),
        ("true", "false")
    );
    assert_eq!(engine.complete_command("nocl".into()), ["noclip"]);
    assert_eq!(engine.complete_command("ren".into()).len(), 15);
    run(&engine, "bind f5 pause")?;
    engine.start_empty_world();
    assert!(engine.press_key("f5".into()).ok);
    assert!(engine.paused());
    assert!(engine.press_key("f6".into()).ok);
    let help = run(&engine, "help")?;
    for command in [
        "map", "bsp", "spawn", "teleport", "noclip", "god", "give", "enter", "kill", "time",
        "pause", "step", "objects", "inspect", "tags", "vars", "set", "bind", "exec", "help",
        "log", "quit",
    ] {
        assert!(
            help.text.lines().any(|line| line.starts_with(command)),
            "{command}"
        );
    }
    Ok(())
}

#[test]
fn without_a_level_or_a_player() -> Result {
    let engine = Engine::new();
    engine.start_empty_world();
    for line in [
        "spawn weapons\\pistol\\pistol",
        "bsp 0",
        "tags",
        "noclip",
        "god",
        "give pistol",
        "kill",
        "inspect 0",
        "enter 0",
        "teleport 3",
    ] {
        refused(&engine, line)?;
    }
    // (no mission: HaloScript has nothing to run it)
    let unknown = refused(&engine, "(ai_place x)")?;
    assert!(unknown.contains("ai_place"), "{unknown}");
    // (the free camera moves instead of the player)
    run(&engine, "teleport 1 2 3")?;
    assert_eq!(json(&run(&engine, "objects")?)?, serde_json::json!([]));
    Ok(())
}

#[test]
fn over_bloodgulch() -> Result {
    let Some(level) = bloodgulch() else {
        return Ok(());
    };
    let engine = Engine::new();
    engine.start_level(level, 2)?;
    engine.spawn_player(0)?;

    let tags = json(&run(&engine, "tags warthog")?)?;
    assert!(tags
        .as_array()
        .is_some_and(|tags| tags.iter().any(|tag| tag["kind"] == "vehicle")));
    let before = json(&run(&engine, "objects")?)?
        .as_array()
        .map_or(0, Vec::len);

    // spawn: in front of the camera, dressed
    let vehicle = tags
        .as_array()
        .and_then(|tags| tags.iter().find(|tag| tag["kind"] == "vehicle"))
        .and_then(|tag| tag["path"].as_str())
        .ok_or("no vehicle")?
        .replace('\\', "/");
    let made = json(&run(&engine, &format!("spawn {vehicle} 2"))?)?;
    let made: Vec<i64> = made
        .as_array()
        .ok_or("no ids")?
        .iter()
        .filter_map(serde_json::Value::as_i64)
        .collect();
    assert_eq!(made.len(), 2);
    let after = json(&run(&engine, "objects")?)?;
    assert_eq!(after.as_array().map_or(0, Vec::len), before + 2);
    let warthog = made[0];
    let inspected = json(&run(&engine, &format!("inspect {warthog}"))?)?;
    assert_eq!(inspected["kind"], "vehicle");
    refused(&engine, "spawn nothing_like_it")?;
    refused(&engine, "spawn warthog 1000")?;
    refused(&engine, "inspect 99999")?;

    // teleport: to a point, and beside an object
    let to = json(&run(&engine, "teleport 30 -70 2")?)?;
    assert_eq!(to, serde_json::json!([30.0, -70.0, 2.0]));
    run(&engine, &format!("teleport {warthog}"))?;
    refused(&engine, "teleport x 1 2")?;

    // god: no damage
    run(&engine, "god on")?;
    let player = json(&run(&engine, "objects cyborg")?)?;
    let player_id = player[0]["id"].as_i64().ok_or("no player")?;
    assert_eq!(
        json(&run(&engine, &format!("inspect {player_id}"))?)?["invulnerable"],
        true
    );
    run(&engine, "god")?;
    assert!(json(&run(&engine, &format!("inspect {player_id}"))?)?
        .get("invulnerable")
        .is_none());

    // noclip: flies up through the ground with the jump button held
    run(&engine, "noclip on")?;
    let start = json(&run(&engine, &format!("inspect {player_id}"))?)?["position"][2]
        .as_f64()
        .ok_or("no height")?;
    engine.set_input(opence_ffi::InputState {
        buttons: opence_ffi::input_buttons().jump,
        ..Default::default()
    });
    run(&engine, "step 30")?;
    let risen = json(&run(&engine, &format!("inspect {player_id}"))?)?["position"][2]
        .as_f64()
        .ok_or("no height")?;
    assert!(risen > start + 1.0, "{start} to {risen}");
    engine.set_input(opence_ffi::InputState::default());
    run(&engine, "noclip off")?;

    // give: a weapon of the level's
    let weapon = json(&run(&engine, "tags")?)?
        .as_array()
        .and_then(|tags| {
            tags.iter()
                .find(|tag| {
                    tag["kind"] == "weapon"
                        && tag["path"]
                            .as_str()
                            .is_some_and(|path| path.starts_with("weapons"))
                })
                .cloned()
        })
        .and_then(|tag| tag["path"].as_str().map(|path| path.replace('\\', "/")))
        .ok_or("no weapon")?;
    let given = run(&engine, &format!("give {weapon}"))?;
    let name = weapon.rsplit('/').next().unwrap_or(&weapon);
    assert!(given.text.contains(name), "{}", given.text);
    refused(&engine, "give nothing_like_it")?;

    // enter: the driver's seat, and noclip refused there
    run(&engine, &format!("enter {warthog}"))?;
    refused(&engine, "noclip on")?;
    refused(&engine, &format!("enter {warthog} 99"))?;

    // kill: an object, then every unit but the player, then the player
    let killed = json(&run(&engine, &format!("kill {}", made[1]))?)?;
    assert_eq!(killed, serde_json::json!([made[1]]));
    assert_eq!(
        json(&run(&engine, &format!("inspect {}", made[1]))?)?["dead"],
        true
    );
    run(&engine, "kill all")?;
    assert_eq!(
        json(&run(&engine, &format!("inspect {player_id}"))?)?["dead"],
        false
    );
    run(&engine, "kill")?;
    assert_eq!(
        json(&run(&engine, &format!("inspect {player_id}"))?)?["dead"],
        true
    );

    // bsp: the only one, and one it does not have
    assert_eq!(
        run(&engine, "bsp 0")?.action,
        ShellAction::ReloadBsp { bsp: 0 }
    );
    refused(&engine, "bsp 5")?;
    Ok(())
}

#[test]
fn halo_script_in_a_mission() -> Result {
    let Some(level) = bloodgulch() else {
        return Ok(());
    };
    let engine = Engine::new();
    engine.start_campaign(level, 1)?;
    let sum = run(&engine, "(+ 1 2)")?;
    assert_eq!(sum.text, "3");
    let unknown = refused(&engine, "(not_a_function_at_all)")?;
    assert!(unknown.contains("not_a_function_at_all"), "{unknown}");
    // (a command still runs in a mission)
    run(&engine, "god on")?;
    Ok(())
}

#[test]
fn the_watch_line_and_clear() -> Result {
    let engine = Engine::new();
    engine.start_empty_world();
    assert!(engine.watch_values().is_empty());
    assert_eq!(run(&engine, "watch")?.text, "nothing is watched");
    run(&engine, "watch render.wireframe")?;
    // (watching it twice keeps one)
    run(&engine, "watch render.wireframe")?;
    run(&engine, "watch noclip")?;
    let values = engine.watch_values();
    assert_eq!(values.len(), 2);
    assert_eq!(values[0].expression, "render.wireframe");
    assert_eq!((values[0].ok, values[0].value.as_str()), (true, "false"));
    run(&engine, "set render.wireframe true")?;
    assert_eq!(engine.watch_values()[0].value, "true");
    // a command is never run by the line
    assert_eq!(
        (values[1].ok, values[1].value.as_str()),
        (false, "a command, not a value")
    );
    // (HaloScript without a mission says why)
    run(&engine, "watch (+ 1 2)")?;
    assert!(!engine.watch_values()[2].ok);
    assert_eq!(
        json(&run(&engine, "watch")?)?.as_array().map(Vec::len),
        Some(3)
    );

    run(&engine, "unwatch noclip")?;
    assert!(refused(&engine, "unwatch noclip")?.contains("not watched"));
    assert_eq!(engine.watch_values().len(), 2);
    assert!(refused(&engine, &format!("watch {}", "x".repeat(201)))?.contains("at most"));
    for index in 0..6 {
        run(&engine, &format!("watch (+ {index} 1)"))?;
    }
    assert!(refused(&engine, "watch (+ 9 9)")?.contains("at most 8"));
    assert_eq!(run(&engine, "unwatch")?.text, "unwatched 8");
    assert!(engine.watch_values().is_empty());

    let cleared = run(&engine, "clear")?;
    assert_eq!(json(&cleared)?["clear"], true);
    assert!(engine
        .complete_command("unw".into())
        .contains(&"unwatch".to_owned()));
    Ok(())
}

#[test]
fn the_watch_line_runs_halo_script_in_a_mission() -> Result {
    let Some(level) = bloodgulch() else {
        return Ok(());
    };
    let engine = Engine::new();
    engine.start_campaign(level, 1)?;
    run(&engine, "watch (+ 1 2)")?;
    let values = engine.watch_values();
    assert_eq!((values[0].ok, values[0].value.as_str()), (true, "3"));
    Ok(())
}

/// The lines the newest frame holds (read as the app reads it).
#[allow(unsafe_code)]
fn debug_lines(engine: &Engine) -> u32 {
    let buffers = engine.snapshot_buffers() as *const opence_ffi::snapshot::SnapshotBuffers;
    // SAFETY: the engine is alive and this test is its only reader.
    unsafe { (*opence_ffi::snapshot::opence_snapshot_acquire(buffers)).debug_line_count }
}

#[test]
fn the_world_layers_over_bloodgulch() -> Result {
    let Some(level) = bloodgulch() else {
        return Ok(());
    };
    let engine = Engine::new();
    engine.start_level(level, 2)?;
    engine.spawn_player(0)?;
    engine.step()?;
    assert_eq!(debug_lines(&engine), 0);
    let mut seen = 0;
    for layer in ["collision", "markers", "bodies"] {
        run(&engine, &format!("set debug.draw.{layer} true"))?;
        engine.step()?;
        let lines = debug_lines(&engine);
        assert!(lines > seen, "{layer}: {lines} lines");
        seen = lines;
    }
    for layer in ["collision", "markers", "bodies"] {
        run(&engine, &format!("set debug.draw.{layer} false"))?;
    }
    engine.step()?;
    assert_eq!(debug_lines(&engine), 0);
    Ok(())
}

#[test]
fn scripts_run_headless() -> Result {
    let engine = Engine::new();
    // (no world: a wait cannot run)
    let report = engine.run_script("wait 2\n".into());
    assert!(!report.ok);
    assert!(report.text.contains("no world"), "{}", report.text);

    engine.start_empty_world();
    let report = engine.run_script(
        "# a test\nstep 2\nwait 10\nexpect tick == 12\nwait_until tick >= 40\nframe start\nexpect playing\n"
            .into(),
    );
    assert!(report.ok, "{}", report.text);
    assert_eq!(report.frames, ["start"]);
    assert_eq!(report.lines, 6);

    // a failing expect: its line, its value, and the world's state
    let report = engine.run_script("step 1\nexpect tick == 3\nstep 5\n".into());
    assert!(!report.ok);
    let failure = report.failure.ok_or("no failure")?;
    assert_eq!(failure.line, 2);
    assert_eq!(failure.expression.as_deref(), Some("tick == 3"));
    assert!(failure.value.is_some());
    let state: serde_json::Value = serde_json::from_str(&failure.state)?;
    assert_eq!(state["playing"], true);

    // a timeout, a line that needs the app, a script in a script, a bad line
    let timed = engine.run_script("wait_until tick < 0 5\n".into());
    assert!(!timed.ok && timed.text.contains("line 1"), "{}", timed.text);
    assert!(!engine.run_script("quit\n".into()).ok);
    assert!(!engine.run_script("script other.ocs\n".into()).ok);
    let unread = engine.run_script("wait forever\n".into());
    assert_eq!(unread.failure.map(|failure| failure.line), Some(1));
    // (values: variables, the world's (step paused it), HaloScript; a command is not one)
    assert!(
        engine
            .run_script("expect paused == true\nexpect objects >= 0\n".into())
            .ok
    );
    assert!(
        engine
            .run_script("expect render.wireframe == false\n".into())
            .ok
    );
    assert!(!engine.run_script("expect step == 1\n".into()).ok);
    assert!(!engine.run_script("expect player.alive\n".into()).ok);
    Ok(())
}

#[test]
fn scripts_run_in_the_window() -> Result {
    let engine = Engine::new();
    engine.start_empty_world();
    let folder = std::env::temp_dir().join(format!("opence_scripts_{}", std::process::id()));
    std::fs::create_dir_all(&folder)?;
    let file = folder.join("walk.ocs");
    run(&engine, "pause")?;
    std::fs::write(&file, "wait 3\nexpect tick == 3\n")?;
    refused(&engine, "script nowhere.ocs")?;
    assert!(run(&engine, "script")?.text.contains("no script"));
    run(&engine, &format!("script {}", file.display()))?;
    assert!(run(&engine, "script")?.text.contains("at line"));
    // (each frame runs a line; paused, the waits tick the world)
    for _ in 0..10 {
        engine.advance(1.0 / 30.0)?;
    }
    let status = run(&engine, "script")?.text;
    assert!(status.contains("passed"), "{status}");

    std::fs::write(&file, "wait 1000\n")?;
    run(&engine, &format!("script {}", file.display()))?;
    assert_eq!(run(&engine, "script stop")?.text, "the script stopped");
    assert_eq!(run(&engine, "script stop")?.text, "no script runs");
    std::fs::write(&file, "expect tick == -1\n")?;
    run(&engine, &format!("script {}", file.display()))?;
    engine.advance(1.0 / 30.0)?;
    assert!(run(&engine, "script")?.text.contains("line 1"));
    std::fs::remove_dir_all(&folder)?;
    Ok(())
}

#[test]
fn scripts_over_bloodgulch() -> Result {
    let Some(level) = bloodgulch() else {
        return Ok(());
    };
    let engine = Engine::new();
    engine.start_level(level, 2)?;
    engine.spawn_player(0)?;
    let report = engine.run_script(
        "expect level\nexpect player.alive\nexpect objects > 0\nwait 30\nexpect player.z > -1000\n"
            .into(),
    );
    assert!(report.ok, "{}", report.text);
    Ok(())
}
