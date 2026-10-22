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
    std::fs::remove_dir_all(&folder)?;
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
    assert_eq!(engine.complete_command("ren".into()).len(), 4);
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
    assert_eq!(json(&run(&engine, "watch")?)?.as_array().map(Vec::len), Some(3));

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
