//! The developer state: each control set one way is read the same every
//! other (the app's calls, the shell, the control socket), each change is
//! counted once, and the lighting controls are saved and loaded per map.

use std::io::{BufRead, BufReader, Write};
use std::os::unix::net::UnixStream;
use std::path::PathBuf;
use std::sync::Arc;
use std::time::Duration;

use opence_ffi::Engine;
use serde_json::Value;

type Result = std::result::Result<(), Box<dyn std::error::Error>>;

fn folder(name: &str) -> PathBuf {
    std::env::temp_dir().join(format!("oce-tune-{}-{name}", std::process::id()))
}

fn value(engine: &Engine, name: &str) -> f64 {
    engine
        .tune_controls()
        .into_iter()
        .find(|control| control.name == name)
        .map_or(f64::NAN, |control| control.value)
}

#[test]
fn every_control_round_trips() -> Result {
    let engine = Engine::new();
    let controls = engine.tune_controls();
    assert_eq!(controls.len(), 11);
    for control in &controls {
        assert_eq!(control.value, control.default_value, "{}", control.name);
        assert!(control.minimum <= control.default_value && control.default_value <= control.maximum);
        // (each is a shell variable too: the Developer tab and `vars` list it)
        assert!(engine.shell_variables().iter().any(|v| v.name == control.name));
    }
    assert!(!engine.shell_flag("dev.panel".into()));

    // (the app's call, read by the shell)
    engine.set_shell_variable("tune.sun".into(), "2.5".into())?;
    assert_eq!(engine.shell_value("tune.sun".into()), "2.5");
    // (the shell's, read by the app)
    assert!(engine.run_command("set tune.bloom 0.5".into()).ok);
    assert_eq!(value(&engine, "tune.bloom"), 0.5);
    assert!(engine.run_command("tune.ao 1.5".into()).ok);
    assert_eq!(value(&engine, "tune.ao"), 1.5);
    assert!(engine.run_command("dev.panel true".into()).ok);
    assert!(engine.shell_flag("dev.panel".into()));

    // (`tune` lists them, as data for the socket)
    let listed = engine.run_command("tune".into());
    assert!(listed.ok, "{}", listed.text);
    let data: Value = serde_json::from_str(&listed.json)?;
    assert_eq!(data["tune.sun"], 2.5);
    assert_eq!(data["tune.exposure"], 0.0);
    assert!(listed.text.contains("tune.bloom = 0.5"), "{}", listed.text);

    // (reset restores the defaults, and needs no map)
    assert!(engine.run_command("tune reset".into()).ok);
    assert!(engine.tune_controls().iter().all(|c| c.value == c.default_value));
    assert!(!engine.run_command("set tune.sun lots".into()).ok);
    assert!(!engine.run_command("tune save".into()).ok);
    Ok(())
}

#[test]
fn each_change_is_counted_once() -> Result {
    let engine = Engine::new();
    let start = engine.dev_generation();
    // (nothing changed: the app reads nothing again)
    assert_eq!(engine.dev_generation(), start);
    engine.set_shell_variable("tune.exposure".into(), "1".into())?;
    assert_eq!(engine.dev_generation(), start + 1);
    // (set to what it is: no change)
    engine.set_shell_variable("tune.exposure".into(), "1".into())?;
    assert_eq!(engine.dev_generation(), start + 1);
    assert!(engine.run_command("render.ssr false".into()).ok);
    assert_eq!(engine.dev_generation(), start + 2);
    assert!(engine.run_command("help".into()).ok);
    assert_eq!(engine.dev_generation(), start + 2);
    engine.load_shell_variables([("render.bloom".to_owned(), "false".to_owned())].into());
    assert!(engine.dev_generation() > start + 2);
    Ok(())
}

#[test]
fn tuning_is_saved_and_loaded_per_map() -> Result {
    let folder = folder("maps");
    let _ = std::fs::remove_dir_all(&folder);
    let place = folder.to_string_lossy().into_owned();
    let engine = Engine::new();

    assert_eq!(engine.set_tuning(place.clone(), "devgulch".into())?, Vec::<String>::new());
    engine.set_shell_variable("tune.sun".into(), "1.75".into())?;
    let saved = engine.run_command("tune save".into());
    assert!(saved.ok, "{}", saved.text);
    let text = std::fs::read_to_string(folder.join("devgulch.toml"))?;
    // (only what differs from the stock frame)
    assert!(text.contains("sun = 1.75"), "{text}");
    assert!(!text.contains("bloom"), "{text}");

    // (another map starts at the defaults, and keeps its own)
    engine.set_tuning(place.clone(), "devmap".into())?;
    assert_eq!(value(&engine, "tune.sun"), 1.0);
    engine.set_shell_variable("tune.bounce".into(), "3".into())?;
    engine.save_tuning()?;
    engine.set_tuning(place.clone(), "devgulch".into())?;
    assert_eq!(value(&engine, "tune.sun"), 1.75);
    assert_eq!(value(&engine, "tune.bounce"), 1.0);

    // (`tune load` drops what was not saved)
    engine.set_shell_variable("tune.sun".into(), "0.5".into())?;
    assert!(engine.run_command("tune load".into()).ok);
    assert_eq!(value(&engine, "tune.sun"), 1.75);

    // (saved at the defaults, the file goes)
    assert!(engine.run_command("tune reset".into()).ok);
    engine.save_tuning()?;
    assert!(!folder.join("devgulch.toml").exists());

    // (a file is untrusted: an unknown name or a value out of range is refused)
    std::fs::write(
        folder.join("devmap.toml"),
        "[tune]\nsun = 2.0\nbloom = 99.0\nwarp = 1.0\n",
    )?;
    let mut refused = engine.set_tuning(place.clone(), "devmap".into())?;
    refused.sort();
    assert_eq!(refused, ["tune.bloom", "tune.warp"]);
    assert_eq!(value(&engine, "tune.sun"), 2.0);
    assert_eq!(value(&engine, "tune.bloom"), 1.0);
    std::fs::write(folder.join("devmap.toml"), "not toml [")?;
    assert!(engine.set_tuning(place, "devmap".into()).is_err());
    let _ = std::fs::remove_dir_all(&folder);
    Ok(())
}

#[test]
fn the_socket_sets_and_reads_the_controls() -> Result {
    let path = std::env::temp_dir()
        .join(format!("oct{}", std::process::id()))
        .join("c.sock");
    let engine = Engine::new();
    Arc::clone(&engine).start_control(Some(path.to_string_lossy().into()))?;
    let stream = UnixStream::connect(&path)?;
    stream.set_read_timeout(Some(Duration::from_secs(40)))?;
    let mut reader = BufReader::new(stream.try_clone()?);
    let mut writer = stream;
    let mut send = |line: &str| -> std::result::Result<Value, Box<dyn std::error::Error>> {
        writeln!(writer, "{line}")?;
        let mut answer = String::new();
        reader.read_line(&mut answer)?;
        Ok(serde_json::from_str(&answer)?)
    };

    assert_eq!(send(r#""set tune.emissive 2""#)?["ok"], true);
    assert_eq!(value(&engine, "tune.emissive"), 2.0);
    engine.set_shell_variable("tune.tone_curve".into(), "1.25".into())?;
    let listed = send(r#""tune""#)?;
    assert_eq!(listed["ok"], true);
    assert_eq!(listed["data"]["tune.tone_curve"], 1.25);
    assert_eq!(listed["data"]["tune.emissive"], 2.0);
    assert_eq!(send(r#""tune reset""#)?["ok"], true);
    assert_eq!(value(&engine, "tune.emissive"), 1.0);
    engine.stop_control();
    Ok(())
}
