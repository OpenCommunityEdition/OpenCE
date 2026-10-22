//! The `opence` tool against an engine's control socket (the engine here,
//! with a thread standing in for the app that draws frames).

use std::io::{BufRead, BufReader, Write};
use std::path::{Path, PathBuf};
use std::process::{Command, Stdio};
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Arc;
use std::time::Duration;

use opence_cli::{parse_size, Connection, Error};
use opence_ffi::Engine;

type Result = std::result::Result<(), Box<dyn std::error::Error>>;

struct Running {
    engine: Arc<Engine>,
    socket: PathBuf,
}

impl Running {
    fn start(name: &str) -> std::result::Result<Self, Box<dyn std::error::Error>> {
        let socket = std::env::temp_dir()
            .join(format!("ocl{}{name}", std::process::id()))
            .join("c.sock");
        let engine = Engine::new();
        Arc::clone(&engine).start_control(Some(socket.to_string_lossy().into()))?;
        engine.start_empty_world();
        Ok(Self { engine, socket })
    }

    fn opence(&self, arguments: &[&str]) -> std::io::Result<std::process::Output> {
        Command::new(env!("CARGO_BIN_EXE_opence"))
            .arg("--socket")
            .arg(&self.socket)
            .args(arguments)
            .output()
    }
}

impl Drop for Running {
    fn drop(&mut self) {
        self.engine.stop_control();
        if let Some(folder) = self.socket.parent() {
            let _ = std::fs::remove_dir_all(folder);
        }
    }
}

fn text(output: &std::process::Output) -> String {
    String::from_utf8_lossy(&output.stdout).into_owned()
}

#[test]
fn run_and_state() -> Result {
    let engine = Running::start("a")?;
    let stepped = engine.opence(&["run", "step", "4"])?;
    assert!(stepped.status.success(), "{stepped:?}");
    let json = engine.opence(&["run", "--json", "step 2"])?;
    let reply: serde_json::Value = serde_json::from_str(text(&json).trim())?;
    assert_eq!(reply["data"], 6);
    let state = engine.opence(&["state"])?;
    let state: serde_json::Value = serde_json::from_str(&text(&state))?;
    assert_eq!(state["tick"], 6);
    assert!(state.get("ok").is_none());

    let refused = engine.opence(&["run", "time", "99"])?;
    assert_eq!(refused.status.code(), Some(1));
    assert!(String::from_utf8_lossy(&refused.stderr).contains("99"));
    for wrong in [
        &[][..],
        &["run"][..],
        &["teleport_everything"][..],
        &["frame"][..],
        &["frame", "a.png", "--size", "big"][..],
        &["watch", "--backlog", "many"][..],
        &["script"][..],
    ] {
        assert_eq!(engine.opence(wrong)?.status.code(), Some(2), "{wrong:?}");
    }
    let nowhere = Command::new(env!("CARGO_BIN_EXE_opence"))
        .args(["--socket", "/nonexistent/c.sock", "state"])
        .output()?;
    assert_eq!(nowhere.status.code(), Some(2));
    assert!(String::from_utf8_lossy(&nowhere.stderr).contains("--headless"));
    assert_eq!(parse_size("640x360")?, (640, 360));
    assert!(matches!(parse_size("640"), Err(Error::Usage(_))));
    Ok(())
}

#[test]
fn scripts_stop_at_their_first_failure() -> Result {
    let engine = Running::start("b")?;
    let folder = engine.socket.parent().ok_or("no folder")?.to_path_buf();
    let good = folder.join("good.ocs");
    std::fs::write(&good, "# steps\nstep 2\n\nstep 3\n")?;
    let ran = engine.opence(&["script", &good.to_string_lossy()])?;
    assert!(ran.status.success(), "{ran:?}");
    assert_eq!(text(&ran).lines().count(), 2);
    let bad = folder.join("bad.ocs");
    std::fs::write(&bad, "step 1\ntime 99\nstep 50\n")?;
    let failed = engine.opence(&["script", &bad.to_string_lossy()])?;
    assert_eq!(failed.status.code(), Some(1));
    assert!(text(&failed).contains("FAILED"));
    assert!(String::from_utf8_lossy(&failed.stderr).contains("line 2"));
    let mut connection = Connection::connect(&engine.socket)?;
    let state = connection.ask(&serde_json::json!({"request": "state"}))?;
    assert_eq!(state["tick"], 6);

    // (lines read from a pipe: every one run, refusals counted)
    let mut shell = Command::new(env!("CARGO_BIN_EXE_opence"))
        .arg("--socket")
        .arg(&engine.socket)
        .arg("shell")
        .stdin(Stdio::piped())
        .stdout(Stdio::piped())
        .spawn()?;
    shell
        .stdin
        .take()
        .ok_or("no stdin")?
        .write_all(b"step 1\n# nothing\ntime 99\nstep 1\n")?;
    let output = shell.wait_with_output()?;
    assert_eq!(output.status.code(), Some(1));
    assert!(text(&output).contains("error:"));
    assert_eq!(
        connection.ask(&serde_json::json!({"request": "state"}))?["tick"],
        8
    );
    let completed = opence_cli::complete(&mut connection, "nocl")?;
    assert_eq!(completed, ["noclip"]);
    Ok(())
}

#[test]
fn watch_follows_the_log() -> Result {
    let engine = Running::start("c")?;
    opence_ffi::control::engine_log("info".into(), "swift".into(), "before".into());
    let mut watching = Command::new(env!("CARGO_BIN_EXE_opence"))
        .arg("--socket")
        .arg(&engine.socket)
        .args(["watch", "--backlog", "1"])
        .stdout(Stdio::piped())
        .spawn()?;
    let mut lines = BufReader::new(watching.stdout.take().ok_or("no stdout")?).lines();
    let first = lines.next().ok_or("no backlog")??;
    assert!(first.ends_with("swift: before"), "{first}");
    // (until the subscription is made, a line may come before it)
    let mut found = false;
    for _ in 0..50 {
        opence_ffi::control::engine_log("warn".into(), "swift".into(), "after".into());
        std::thread::sleep(Duration::from_millis(20));
        if let Some(Ok(line)) = lines.next() {
            assert!(line.contains("warn swift: after"), "{line}");
            found = true;
            break;
        }
    }
    watching.kill()?;
    watching.wait()?;
    assert!(found);

    let mut connection = Connection::connect(&engine.socket)?;
    let mut out = Vec::new();
    opence_cli::watch(&mut connection, 0, Some(0), &mut out)?;
    assert!(out.is_empty());
    Ok(())
}

#[test]
fn frames_are_written_by_the_app() -> Result {
    let engine = Running::start("d")?;
    let folder = engine.socket.parent().ok_or("no folder")?.to_path_buf();
    let png = folder.join("view.png");
    let path = png.to_string_lossy().into_owned();
    // (no level: refused)
    assert_eq!(engine.opence(&["frame", &path])?.status.code(), Some(1));
    let Some(level) = bloodgulch() else {
        return Ok(());
    };
    engine.engine.start_level(level, 1)?;
    let done = Arc::new(AtomicBool::new(false));
    let app = {
        let engine = Arc::clone(&engine.engine);
        let done = Arc::clone(&done);
        std::thread::spawn(move || {
            while !done.load(Ordering::Acquire) {
                for frame in engine.control_frame_requests() {
                    let size = format!("{}x{}", frame.width, frame.height);
                    let written = std::fs::write(&frame.path, size);
                    engine.finish_frame(frame.id, written.err().map(|error| error.to_string()));
                }
                std::thread::sleep(Duration::from_millis(5));
            }
        })
    };
    let drawn = engine.opence(&["frame", &path, "--size", "200x100"])?;
    done.store(true, Ordering::Release);
    app.join().map_err(|_| "the app's thread panicked")?;
    assert!(drawn.status.success(), "{drawn:?}");
    assert_eq!(text(&drawn).trim(), path);
    assert_eq!(std::fs::read_to_string(&png)?, "200x100");
    Ok(())
}

fn bloodgulch() -> Option<Arc<opence_ffi::LevelGeometry>> {
    let map = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../assets/maps/bloodgulch.map");
    if !map.exists() {
        return None;
    }
    let cache = Path::new(env!("CARGO_TARGET_TMPDIR")).join("cli_cache");
    std::fs::create_dir_all(&cache).ok()?;
    opence_ffi::LevelGeometry::load(
        map.to_string_lossy().into_owned(),
        cache.to_string_lossy().into_owned(),
        0,
    )
    .ok()
}
