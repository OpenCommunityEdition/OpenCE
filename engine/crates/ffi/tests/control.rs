//! The control socket over a real Unix socket: commands, the world's state,
//! actions and frames queued for the app (a thread here stands in for it),
//! the log, and refusals of what a connection may not send.

use std::io::{BufRead, BufReader, Write};
use std::os::unix::fs::PermissionsExt;
use std::os::unix::net::UnixStream;
use std::path::PathBuf;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Arc;
use std::time::Duration;

use opence_ffi::devtools::ShellAction;
use opence_ffi::Engine;
use serde_json::{json, Value};

type Result = std::result::Result<(), Box<dyn std::error::Error>>;

fn socket_path(name: &str) -> PathBuf {
    // (short: a socket's path is limited to about a hundred bytes)
    std::env::temp_dir()
        .join(format!("oce{}{name}", std::process::id()))
        .join("c.sock")
}

struct Client {
    reader: BufReader<UnixStream>,
    writer: UnixStream,
}

impl Client {
    fn connect(path: &std::path::Path) -> std::io::Result<Self> {
        let writer = UnixStream::connect(path)?;
        writer.set_read_timeout(Some(Duration::from_secs(40)))?;
        Ok(Self {
            reader: BufReader::new(writer.try_clone()?),
            writer,
        })
    }

    fn send(&mut self, line: &str) -> std::result::Result<Value, Box<dyn std::error::Error>> {
        writeln!(self.writer, "{line}")?;
        self.read()
    }

    fn read(&mut self) -> std::result::Result<Value, Box<dyn std::error::Error>> {
        let mut answer = String::new();
        self.reader.read_line(&mut answer)?;
        Ok(serde_json::from_str(&answer)?)
    }
}

#[test]
fn commands_state_and_actions() -> Result {
    let path = socket_path("a");
    let engine = Engine::new();
    let listening = Arc::clone(&engine).start_control(Some(path.to_string_lossy().into()))?;
    assert_eq!(PathBuf::from(listening), path);
    assert!(Arc::clone(&engine).start_control(None).is_err());
    let mode = std::fs::metadata(&path)?.permissions().mode();
    assert_eq!(mode & 0o777, 0o600);

    let mut client = Client::connect(&path)?;
    let state = client.send(r#"{"request": "state"}"#)?;
    assert_eq!(state["ok"], true);
    assert_eq!(state["playing"], false);
    engine.start_empty_world();
    let ran = client.send(r#""step 3""#)?;
    assert_eq!(ran["ok"], true);
    assert_eq!(ran["data"], 3);
    let state = client.send(r#"{"request": "state"}"#)?;
    assert_eq!(
        (&state["playing"], &state["paused"], &state["tick"]),
        (&json!(true), &json!(true), &json!(3))
    );
    let refused = client.send(r#"{"command": "time 99"}"#)?;
    assert_eq!(refused["ok"], false);
    assert!(refused["error"].as_str().is_some_and(|e| e.contains("99")));

    // (the app's actions wait for it)
    let quit = client.send(r#""quit""#)?;
    assert_eq!(quit["action"], "quit");
    assert_eq!(engine.take_control_actions(), [ShellAction::Quit]);
    assert!(engine.take_control_actions().is_empty());

    let objects = client.send(r#"{"request": "objects", "filter": "biped"}"#)?;
    assert_eq!(objects["objects"], json!([]));
    let bad = client.send(r#"{"request": "objects", "filter": "a; quit"}"#)?;
    assert_eq!(bad["ok"], false);
    assert!(engine.take_control_actions().is_empty());

    let completed = client.send(r#"{"request": "complete", "line": "nocl"}"#)?;
    assert_eq!(completed["completions"], json!(["noclip"]));

    let script = client.send(
        &json!({"request": "script", "text": "# a test\nstep 2\n\ntime 99\nstep 5"}).to_string(),
    )?;
    assert_eq!(script["ok"], false);
    let results = script["results"].as_array().ok_or("no results")?;
    assert_eq!(results.len(), 2);
    assert_eq!(
        (&results[1]["line"], &results[1]["ok"]),
        (&json!(4), &json!(false))
    );

    let unknown = client.send(r#"{"request": "teleport_everything"}"#)?;
    assert_eq!(unknown["ok"], false);

    engine.stop_control();
    assert!(!path.exists());
    assert!(UnixStream::connect(&path).is_err());
    if let Some(folder) = path.parent() {
        std::fs::remove_dir_all(folder)?;
    }
    Ok(())
}

#[test]
fn frames_are_drawn_by_the_app() -> Result {
    let path = socket_path("b");
    let engine = Engine::new();
    Arc::clone(&engine).start_control(Some(path.to_string_lossy().into()))?;
    let mut client = Client::connect(&path)?;
    let folder = path.parent().ok_or("no folder")?.to_path_buf();
    let png = folder.join("frame.png");
    let request = json!({"request": "frame", "path": png, "width": 64, "height": 32}).to_string();

    // (no level: nothing to draw)
    engine.start_empty_world();
    assert_eq!(client.send(&request)?["ok"], false);
    for bad in [
        json!({"request": "frame", "path": "frame.png"}),
        json!({"request": "frame", "path": "/tmp/frame.jpg"}),
        json!({"request": "frame", "path": "/no/such/folder/frame.png"}),
        json!({"request": "frame", "path": png, "width": 4}),
        json!({"request": "screenshot", "path": png, "height": 100000}),
    ] {
        let answer = client.send(&bad.to_string())?;
        assert_eq!(answer["ok"], false, "{bad}");
    }
    assert!(engine.control_frame_requests().is_empty());

    // (the app, drawing each frame asked for)
    let Some(level) = bloodgulch() else {
        return Ok(());
    };
    engine.start_level(level, 2)?;
    let done = Arc::new(AtomicBool::new(false));
    let app = {
        let engine = Arc::clone(&engine);
        let done = Arc::clone(&done);
        std::thread::spawn(move || {
            while !done.load(Ordering::Acquire) {
                for frame in engine.control_frame_requests() {
                    let written =
                        std::fs::write(&frame.path, [frame.width as u8, frame.height as u8]);
                    engine.finish_frame(frame.id, written.err().map(|error| error.to_string()));
                }
                std::thread::sleep(Duration::from_millis(5));
            }
        })
    };
    let drawn = client.send(&request)?;
    assert_eq!(drawn["ok"], true, "{drawn}");
    assert_eq!(std::fs::read(&png)?, [64, 32]);
    done.store(true, Ordering::Release);
    app.join().map_err(|_| "the app's thread panicked")?;
    drop(client);
    drop(engine);
    // (the engine's end stops the socket)
    assert!(!path.exists());
    std::fs::remove_dir_all(&folder)?;
    Ok(())
}

#[test]
fn the_shells_frame_is_queued_for_the_app() -> Result {
    let folder = std::path::Path::new(env!("CARGO_TARGET_TMPDIR")).join("shell_frame");
    std::fs::create_dir_all(&folder)?;
    let png = folder.join("shot.png");
    let engine = Engine::new();
    let line = format!("frame {} 64 32", png.display());

    // refused: no level, a bad name, a bad size, no folder
    engine.start_empty_world();
    assert!(!engine.run_command(line.clone()).ok);
    for bad in [
        format!("frame {}", folder.join("shot.jpg").display()),
        format!("frame {} 4 4", png.display()),
        "frame /no/such/folder/shot.png".to_owned(),
    ] {
        assert!(!engine.run_command(bad.clone()).ok, "{bad}");
    }
    assert!(engine.control_frame_requests().is_empty());

    let Some(level) = bloodgulch() else {
        return Ok(());
    };
    engine.start_level(level, 2)?;
    let output = engine.run_command(line);
    assert!(output.ok, "{}", output.text);
    let frames = engine.control_frame_requests();
    let frame = frames.first().ok_or("no frame queued")?;
    assert_eq!(
        (frame.path.as_str(), frame.width, frame.height),
        (png.to_str().ok_or("path")?, 64, 32)
    );
    // (its answer goes to the log; a relative path is the working folder's)
    engine.finish_frame(frame.id, Some("drawn badly".into()));
    assert!(engine.run_command("frame shot.png".into()).ok);
    let relative = engine.control_frame_requests();
    let cwd = std::env::current_dir()?.join("shot.png");
    assert_eq!(
        relative.first().map(|frame| frame.path.clone()),
        Some(cwd.to_string_lossy().into_owned())
    );
    std::fs::remove_dir_all(&folder)?;
    Ok(())
}

fn bloodgulch() -> Option<Arc<opence_ffi::LevelGeometry>> {
    let maps = std::path::Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../assets/maps");
    let map = maps.join("bloodgulch.map");
    if !map.exists() {
        return None;
    }
    let cache = std::path::Path::new(env!("CARGO_TARGET_TMPDIR")).join("control_cache");
    std::fs::create_dir_all(&cache).ok()?;
    opence_ffi::LevelGeometry::load(
        map.to_string_lossy().into_owned(),
        cache.to_string_lossy().into_owned(),
        0,
    )
    .ok()
}

#[test]
fn the_log_and_its_subscribers() -> Result {
    let path = socket_path("c");
    let engine = Engine::new();
    Arc::clone(&engine).start_control(Some(path.to_string_lossy().into()))?;
    let mut follower = Client::connect(&path)?;
    let subscribed = follower.send(r#"{"request": "subscribe", "topic": "log"}"#)?;
    assert_eq!(subscribed["subscribed"], "log");
    opence_ffi::control::engine_log("warn".into(), "swift".into(), "from the app".into());
    let line = loop {
        let line = follower.read()?;
        if line["log"]["target"] == "swift" {
            break line;
        }
    };
    assert_eq!(line["log"]["text"], "from the app");
    assert_eq!(line["log"]["level"], "warn");

    let mut client = Client::connect(&path)?;
    let lines = client.send(r#"{"request": "log", "max": 5000}"#)?;
    assert!(lines["lines"]
        .as_array()
        .is_some_and(|lines| lines.iter().any(|line| line["text"] == "from the app")));
    assert_eq!(
        client.send(r#""log sim=debug,warn""#)?["data"],
        "sim=debug,warn"
    );
    let shown = client.send(r#""log""#)?;
    assert!(shown["text"]
        .as_str()
        .is_some_and(|text| text.starts_with("the log keeps sim=debug,warn")));
    assert_eq!(client.send(r#""log sim=loud""#)?["ok"], false);
    assert_eq!(client.send(r#""log info""#)?["ok"], true);
    let entries = opence_ffi::control::log_since(0, 10_000);
    assert!(entries.iter().any(|entry| entry.text == "from the app"));
    let wrong = Client::connect(&path)?.send(r#"{"request": "subscribe", "topic": "frames"}"#)?;
    assert_eq!(wrong["ok"], false);
    engine.stop_control();
    if let Some(folder) = path.parent() {
        std::fs::remove_dir_all(folder)?;
    }
    Ok(())
}

#[test]
fn what_a_connection_may_not_send() -> Result {
    let path = socket_path("d");
    let engine = Engine::new();
    Arc::clone(&engine).start_control(Some(path.to_string_lossy().into()))?;
    let mut client = Client::connect(&path)?;
    assert_eq!(client.send("not json")?["ok"], false);
    let deep = format!("{}1{}", "[".repeat(40), "]".repeat(40));
    assert_eq!(client.send(&deep)?["ok"], false);
    // (blank lines are skipped, and the connection still answers)
    writeln!(client.writer)?;
    assert_eq!(client.send(r#"{"request": "state"}"#)?["ok"], true);

    // (too fast: refused until the bucket fills)
    let mut refused = 0;
    let burst: String = (0..600).map(|_| "{\"request\": \"state\"}\n").collect();
    client.writer.write_all(burst.as_bytes())?;
    for _ in 0..600 {
        if client.read()?["ok"] == false {
            refused += 1;
        }
    }
    assert!(refused >= 150, "{refused}");

    // (too long: refused, and the connection closed)
    let mut long = Client::connect(&path)?;
    let huge = format!(
        "\"{}\"\n",
        "x".repeat(opence_dev::control::MAX_REQUEST + 10)
    );
    long.writer.write_all(huge.as_bytes())?;
    let answer = long.read()?;
    assert_eq!(answer["ok"], false);
    let mut rest = String::new();
    assert_eq!(long.reader.read_line(&mut rest).unwrap_or(0), 0);

    // (not UTF-8: refused, and closed)
    let mut binary = Client::connect(&path)?;
    binary.writer.write_all(b"\"\xff\xfe\"\n")?;
    assert_eq!(binary.read()?["ok"], false);

    // (at most eight at once)
    drop(client);
    drop(long);
    drop(binary);
    std::thread::sleep(Duration::from_millis(100));
    let mut held = Vec::new();
    for _ in 0..opence_dev::control::MAX_CONNECTIONS {
        held.push(Client::connect(&path)?);
    }
    std::thread::sleep(Duration::from_millis(100));
    let mut ninth = Client::connect(&path)?;
    assert_eq!(ninth.read()?["ok"], false);
    drop(held);
    engine.stop_control();
    if let Some(folder) = path.parent() {
        std::fs::remove_dir_all(folder)?;
    }
    Ok(())
}
