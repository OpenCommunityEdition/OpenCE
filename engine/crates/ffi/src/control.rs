//! The control socket: a Unix socket, the user's alone, over which a tool
//! or a test runs the shell's commands, asks for the world's state, has a
//! frame drawn to a PNG and follows the log. Each line sent is a JSON
//! request (`opence_dev::control`); each reply is a JSON line.
//!
//! A connection's thread holds the engine weakly, so the socket never keeps
//! it alive. What only the app can do (loading a map, drawing a frame,
//! quitting) is queued for it: it takes the actions with
//! [`Engine::take_control_actions`] and the frames with
//! [`Engine::control_frame_requests`], and answers each frame with
//! [`Engine::finish_frame`].

use std::io::{BufRead, BufReader, Read, Write};
use std::os::unix::net::UnixStream;
use std::path::{Path, PathBuf};
use std::sync::atomic::{AtomicBool, AtomicU64, AtomicUsize, Ordering};
use std::sync::{Arc, Condvar, Mutex, MutexGuard, PoisonError, Weak};
use std::time::{Duration, Instant};

use opence_dev::control::{
    bind_private, default_socket_path, parse_request, refusal, reply, RateLimit, Request,
    RequestError, MAX_CONNECTIONS, MAX_REQUEST,
};
use opence_dev::logbook::{self, Level};
use rustc_hash::{FxHashMap, FxHashSet};
use serde_json::{json, Map, Value};

use crate::devtools::{ShellAction, ShellOutput};
use crate::{Engine, EngineError};

/// The longest a frame request waits for the app to draw it.
pub const FRAME_TIMEOUT: Duration = Duration::from_secs(30);

/// The smallest and largest frame, in pixels on a side.
pub const FRAME_SIZES: std::ops::RangeInclusive<u32> = 16..=8192;

/// How often a script looks whether the app has served its line.
const SERVE_POLL: Duration = Duration::from_millis(5);

/// How often `subscribe log` looks for new lines.
const LOG_POLL: Duration = Duration::from_millis(50);

/// How long a reply may wait for a reader that does not read.
const WRITE_TIMEOUT: Duration = Duration::from_secs(5);

/// A frame the app is to draw and write as a PNG.
#[derive(Clone, Debug, PartialEq, Eq, uniffi::Record)]
pub struct FrameRequest {
    /// The number to answer it with.
    pub id: u64,
    /// The PNG to write: an absolute path.
    pub path: String,
    /// Its width, in pixels.
    pub width: u32,
    /// Its height, in pixels.
    pub height: u32,
}

/// A line of the engine's log.
#[derive(Clone, Debug, PartialEq, Eq, uniffi::Record)]
pub struct LogEntry {
    /// Its number.
    pub sequence: u64,
    /// Its level: error, warn, info, debug or trace.
    pub level: String,
    /// What wrote it.
    pub target: String,
    /// Its text.
    pub text: String,
}

impl From<logbook::LogLine> for LogEntry {
    fn from(line: logbook::LogLine) -> Self {
        Self {
            sequence: line.sequence,
            level: line.level.as_str().to_lowercase(),
            target: line.target,
            text: line.text,
        }
    }
}

/// Adds a line to the engine's log (the app's own lines): `level` is
/// error, warn, info, debug or trace (info for anything else).
#[uniffi::export]
pub fn engine_log(level: String, target: String, text: String) {
    let level = level.parse().unwrap_or(Level::Info);
    logbook::record(level, &target, &text);
}

/// The log's lines after `sequence` (0 for all that are kept), at most
/// `max`.
#[uniffi::export]
pub fn log_since(sequence: u64, max: u32) -> Vec<LogEntry> {
    logbook::since(sequence, max as usize)
        .into_iter()
        .map(LogEntry::from)
        .collect()
}

/// What the socket shares with its threads and the app.
#[derive(Default)]
pub(crate) struct Control {
    stopped: AtomicBool,
    started: AtomicBool,
    connections: AtomicUsize,
    path: Mutex<Option<PathBuf>>,
    actions: Mutex<Vec<ShellAction>>,
    frames: Mutex<Vec<FrameRequest>>,
    finished: Mutex<FxHashMap<u64, Option<String>>>,
    /// The frames the shell queued, whose answers nobody waits for.
    unclaimed: Mutex<FxHashSet<u64>>,
    drawn: Condvar,
    next_frame: AtomicU64,
    /// The times the app has served the queues (taken the actions, acted
    /// on them, then taken the frames).
    served: AtomicU64,
}

fn lock<T>(mutex: &Mutex<T>) -> MutexGuard<'_, T> {
    mutex.lock().unwrap_or_else(PoisonError::into_inner)
}

impl Control {
    fn stopped(&self) -> bool {
        self.stopped.load(Ordering::Acquire)
    }

    /// Stops listening: the socket's file goes, and the accepting thread
    /// is woken to see it.
    pub(crate) fn stop(&self) {
        if self.stopped.swap(true, Ordering::AcqRel) {
            return;
        }
        if let Some(path) = lock(&self.path).take() {
            let _ = UnixStream::connect(&path);
            let _ = std::fs::remove_file(&path);
        }
        self.drawn.notify_all();
    }

    /// Queues an action for the app.
    pub(crate) fn queue_action(&self, action: ShellAction) {
        lock(&self.actions).push(action);
    }

    /// Queues a frame for the app to draw; its id.
    pub(crate) fn queue_frame(&self, path: String, width: u32, height: u32) -> u64 {
        let id = self.next_frame.fetch_add(1, Ordering::Relaxed) + 1;
        lock(&self.frames).push(FrameRequest {
            id,
            path,
            width,
            height,
        });
        id
    }

    /// Queues a frame no one waits for (the shell's `frame`): its answer
    /// goes to the log.
    pub(crate) fn queue_unclaimed_frame(&self, path: String, width: u32, height: u32) -> u64 {
        let id = self.queue_frame(path, width, height);
        lock(&self.unclaimed).insert(id);
        id
    }

    /// A queued frame's answer, once the app has given it (`Some(error)`
    /// if it could not draw it).
    pub(crate) fn take_finished(&self, id: u64) -> Option<Option<String>> {
        lock(&self.finished).remove(&id)
    }

    /// Forgets a queued frame (the app must not draw it later).
    pub(crate) fn forget_frame(&self, id: u64) {
        lock(&self.frames).retain(|frame| frame.id != id);
        lock(&self.finished).remove(&id);
    }

    /// Waits until the app has taken the queued actions and acted on them.
    pub(crate) fn wait_served(&self) -> Result<(), String> {
        let deadline = Instant::now() + FRAME_TIMEOUT;
        let late = || {
            format!(
                "the app did not act on the line in {} seconds",
                FRAME_TIMEOUT.as_secs()
            )
        };
        while !lock(&self.actions).is_empty() {
            if Instant::now() >= deadline || self.stopped() {
                return Err(late());
            }
            std::thread::sleep(SERVE_POLL);
        }
        // (taken; acted on once the app next takes the frames)
        let taken = self.served.load(Ordering::Acquire);
        while self.served.load(Ordering::Acquire) == taken {
            if Instant::now() >= deadline || self.stopped() {
                return Err(late());
            }
            std::thread::sleep(SERVE_POLL);
        }
        Ok(())
    }

    /// Queues a frame and waits for the app to draw it.
    pub(crate) fn draw(&self, path: String, width: u32, height: u32) -> Result<(), String> {
        let id = self.queue_frame(path, width, height);
        let deadline = Instant::now() + FRAME_TIMEOUT;
        let mut finished = lock(&self.finished);
        loop {
            if let Some(result) = finished.remove(&id) {
                return result.map_or(Ok(()), Err);
            }
            let now = Instant::now();
            if now >= deadline || self.stopped() {
                drop(finished);
                // (not drawn: the app must not draw it later)
                lock(&self.frames).retain(|frame| frame.id != id);
                return Err(format!(
                    "no frame was drawn in {} seconds",
                    FRAME_TIMEOUT.as_secs()
                ));
            }
            finished = self
                .drawn
                .wait_timeout(finished, deadline - now)
                .unwrap_or_else(PoisonError::into_inner)
                .0;
        }
    }
}

#[uniffi::export]
impl Engine {
    /// Listens for requests at `path` (else `OPENCE_CONTROL_SOCKET`, else
    /// `opence/control.sock` in the user's runtime folder); returns the
    /// path. Once an engine: a second call is refused.
    pub fn start_control(self: Arc<Self>, path: Option<String>) -> Result<String, EngineError> {
        let control = Arc::clone(&self.control);
        if control.started.swap(true, Ordering::AcqRel) {
            return Err(EngineError::Load {
                message: "the control socket is already listening".into(),
            });
        }
        let path = path.map_or_else(default_socket_path, PathBuf::from);
        let listener = bind_private(&path).map_err(|error| {
            control.started.store(false, Ordering::Release);
            EngineError::Load {
                message: format!("the control socket at {}: {error}", path.display()),
            }
        })?;
        *lock(&control.path) = Some(path.clone());
        let engine = Arc::downgrade(&self);
        std::thread::Builder::new()
            .name("control".into())
            .spawn(move || accept(listener, engine, control))
            .map_err(|error| EngineError::Load {
                message: error.to_string(),
            })?;
        logbook::record(
            Level::Info,
            "control",
            &format!("listening at {}", path.display()),
        );
        Ok(path.to_string_lossy().into_owned())
    }

    /// Stops listening and removes the socket.
    pub fn stop_control(&self) {
        self.control.stop();
    }

    /// What the socket's commands asked of the app, in order, since it last
    /// asked.
    pub fn take_control_actions(&self) -> Vec<ShellAction> {
        std::mem::take(&mut *lock(&self.control.actions))
    }

    /// The frames the socket asked for since the app last asked: each to be
    /// drawn and answered with [`Engine::finish_frame`].
    pub fn control_frame_requests(&self) -> Vec<FrameRequest> {
        self.control.served.fetch_add(1, Ordering::AcqRel);
        std::mem::take(&mut *lock(&self.control.frames))
    }

    /// Answers a frame request: `error` if it could not be drawn.
    pub fn finish_frame(&self, id: u64, error: Option<String>) {
        if lock(&self.control.unclaimed).remove(&id) {
            match error {
                Some(error) => logbook::record(Level::Warn, "frame", &error),
                None => logbook::record(Level::Info, "frame", &format!("frame {id} drawn")),
            };
            return;
        }
        lock(&self.control.finished).insert(id, error);
        self.control.drawn.notify_all();
    }
}

/// Counts a connection while it is served.
struct Served(Arc<Control>);

impl Drop for Served {
    fn drop(&mut self) {
        self.0.connections.fetch_sub(1, Ordering::AcqRel);
    }
}

fn accept(listener: std::os::unix::net::UnixListener, engine: Weak<Engine>, control: Arc<Control>) {
    for stream in listener.incoming() {
        if control.stopped() || engine.strong_count() == 0 {
            break;
        }
        let Ok(mut stream) = stream else { continue };
        if control.connections.fetch_add(1, Ordering::AcqRel) >= MAX_CONNECTIONS {
            control.connections.fetch_sub(1, Ordering::AcqRel);
            let _ = writeln!(
                stream,
                "{}",
                refusal(&format!("at most {MAX_CONNECTIONS} connections at once"))
            );
            continue;
        }
        let served = Served(Arc::clone(&control));
        let engine = engine.clone();
        let spawned = std::thread::Builder::new()
            .name("control connection".into())
            .spawn(move || {
                let served = served;
                serve(&engine, &served.0, stream);
            });
        if spawned.is_err() {
            break;
        }
    }
}

fn serve(engine: &Weak<Engine>, control: &Control, stream: UnixStream) {
    let _ = stream.set_write_timeout(Some(WRITE_TIMEOUT));
    let Ok(reading) = stream.try_clone() else {
        return;
    };
    let mut reader = BufReader::new(reading);
    let mut writer = stream;
    let mut rate = RateLimit::default();
    loop {
        let mut line = String::new();
        let limit = (MAX_REQUEST + 2) as u64;
        match (&mut reader).take(limit).read_line(&mut line) {
            Ok(0) => return,
            Ok(_) => {}
            Err(_) => {
                let _ = writeln!(writer, "{}", refusal("the request is not UTF-8 text"));
                return;
            }
        }
        let line = line.trim_end_matches(['\n', '\r']);
        if line.len() > MAX_REQUEST {
            // (the rest of the line cannot be told from a request)
            let _ = writeln!(writer, "{}", refusal(&RequestError::TooLong.to_string()));
            return;
        }
        if line.trim().is_empty() {
            continue;
        }
        if control.stopped() {
            return;
        }
        let answer = if !rate.allow(Instant::now()) {
            refusal(&RequestError::TooFrequent.to_string())
        } else {
            match parse_request(line) {
                Err(error) => refusal(&error.to_string()),
                Ok(Request::Structured(name, fields)) if name == "subscribe" => {
                    subscribe(engine, control, &mut writer, &fields);
                    return;
                }
                Ok(request) => {
                    let Some(engine) = engine.upgrade() else {
                        return;
                    };
                    answer(&engine, control, request)
                }
            }
        };
        if writeln!(writer, "{answer}").is_err() {
            return;
        }
    }
}

/// An action as JSON: `null`, `"quit"`, `{"load_map": ...}` or
/// `{"reload_bsp": n}`.
fn action_json(action: &ShellAction) -> Value {
    match action {
        ShellAction::None => Value::Null,
        ShellAction::LoadMap { path, bsp } => json!({ "load_map": { "path": path, "bsp": bsp } }),
        ShellAction::ReloadBsp { bsp } => json!({ "reload_bsp": bsp }),
        ShellAction::Quit => json!("quit"),
    }
}

/// Runs a line, queueing its action for the app.
fn command(engine: &Engine, control: &Control, line: &str) -> (bool, Map<String, Value>) {
    let ShellOutput {
        ok,
        text,
        json,
        action,
    } = engine.run_command(line.to_owned());
    let mut fields = Map::new();
    fields.insert(
        "data".into(),
        serde_json::from_str(&json).unwrap_or(Value::Null),
    );
    fields.insert("action".into(), action_json(&action));
    if ok {
        fields.insert("text".into(), Value::String(text));
    } else {
        fields.insert("error".into(), Value::String(text));
    }
    if action != ShellAction::None {
        lock(&control.actions).push(action);
    }
    (ok, fields)
}

fn answer(engine: &Engine, control: &Control, request: Request) -> String {
    let (name, fields) = match request {
        Request::Command(line) => {
            let (ok, fields) = command(engine, control, &line);
            return reply(ok, fields);
        }
        Request::Structured(name, fields) => (name, fields),
    };
    let answered = match name.as_str() {
        "state" => Ok(engine.control_state()),
        "objects" => objects(engine, control, &fields),
        "complete" => text(&fields, "line").map(|line| {
            let mut found = Map::new();
            found.insert(
                "completions".into(),
                json!(engine.complete_command(line.to_owned())),
            );
            found
        }),
        "script" => return script(engine, &fields),
        "log" => log(&fields),
        "frame" | "screenshot" => frame(engine, control, &fields),
        other => Err(format!(
            "no request `{other}` (state, objects, complete, script, log, frame, \
             screenshot, subscribe)"
        )),
    };
    match answered {
        Ok(fields) => reply(true, fields),
        Err(error) => refusal(&error),
    }
}

fn text<'a>(fields: &'a Map<String, Value>, name: &str) -> Result<&'a str, String> {
    fields
        .get(name)
        .and_then(Value::as_str)
        .ok_or_else(|| format!("the request needs `{name}`, as text"))
}

fn number(fields: &Map<String, Value>, name: &str, default: u64) -> Result<u64, String> {
    match fields.get(name) {
        None => Ok(default),
        Some(value) => value
            .as_u64()
            .ok_or_else(|| format!("`{name}` must be a whole number")),
    }
}

fn objects(
    engine: &Engine,
    control: &Control,
    fields: &Map<String, Value>,
) -> Result<Map<String, Value>, String> {
    let filter = match fields.get("filter") {
        None => "",
        Some(_) => text(fields, "filter")?,
    };
    let allowed = |c: char| c.is_ascii_alphanumeric() || "_./\\-".contains(c);
    if filter.len() > 256 || !filter.chars().all(allowed) {
        return Err("a filter is a tag path or a kind: letters, digits and _ . / -".into());
    }
    let (ok, mut found) = command(
        engine,
        control,
        &format!("objects {}", filter.replace('\\', "/")),
    );
    if !ok {
        return Err(found
            .remove("error")
            .and_then(|error| error.as_str().map(str::to_owned))
            .unwrap_or_default());
    }
    let mut objects = Map::new();
    objects.insert(
        "objects".into(),
        found.remove("data").unwrap_or(Value::Null),
    );
    Ok(objects)
}

/// Runs a script (`opence_dev::script`), its frames written in `folder`
/// if it is given: its reply's `results` are its lines', in turn.
fn script(engine: &Engine, fields: &Map<String, Value>) -> String {
    let source = match text(fields, "text") {
        Ok(source) => source,
        Err(error) => return refusal(&error),
    };
    let folder = match fields.get("folder") {
        None => None,
        Some(_) => match text(fields, "folder") {
            Ok(folder) => Some(std::path::PathBuf::from(folder)),
            Err(error) => return refusal(&error),
        },
    };
    let (ok, fields) = crate::script::over_socket(engine, source, folder);
    reply(ok, fields)
}

fn log_json(entry: LogEntry) -> Value {
    json!({
        "sequence": entry.sequence,
        "level": entry.level,
        "target": entry.target,
        "text": entry.text,
    })
}

fn log(fields: &Map<String, Value>) -> Result<Map<String, Value>, String> {
    let since = number(fields, "since", 0)?;
    let max = number(fields, "max", 500)?.min(logbook::CAPACITY as u64);
    let mut found = Map::new();
    found.insert(
        "lines".into(),
        Value::Array(
            log_since(since, max as u32)
                .into_iter()
                .map(log_json)
                .collect(),
        ),
    );
    found.insert("next".into(), json!(logbook::next_sequence()));
    Ok(found)
}

/// A frame request's checks: an absolute `.png` path in a folder that is
/// there, and a size in [`FRAME_SIZES`].
pub(crate) fn frame_target(fields: &Map<String, Value>) -> Result<(String, u32, u32), String> {
    let path = text(fields, "path")?;
    let file = Path::new(path);
    let png = file
        .extension()
        .is_some_and(|extension| extension.eq_ignore_ascii_case("png"));
    if !file.is_absolute() || !png || path.contains('\0') || path.len() > 4096 {
        return Err("a frame's `path` is an absolute path ending .png".into());
    }
    if !file.parent().is_some_and(Path::is_dir) {
        return Err(format!("no folder for {path}"));
    }
    let size = |name: &str, default: u64| -> Result<u32, String> {
        let value = number(fields, name, default)?;
        u32::try_from(value)
            .ok()
            .filter(|value| FRAME_SIZES.contains(value))
            .ok_or_else(|| {
                format!(
                    "`{name}` must be from {} to {}",
                    FRAME_SIZES.start(),
                    FRAME_SIZES.end()
                )
            })
    };
    Ok((path.to_owned(), size("width", 1280)?, size("height", 720)?))
}

fn frame(
    engine: &Engine,
    control: &Control,
    fields: &Map<String, Value>,
) -> Result<Map<String, Value>, String> {
    let (path, width, height) = frame_target(fields)?;
    if !engine.has_level() {
        return Err("no level is loaded to draw".into());
    }
    control.draw(path.clone(), width, height)?;
    let mut drawn = Map::new();
    drawn.insert("path".into(), Value::String(path));
    drawn.insert("width".into(), json!(width));
    drawn.insert("height".into(), json!(height));
    Ok(drawn)
}

/// Streams the log's new lines as `{"log": line}` until the reader goes,
/// the engine goes or the socket stops.
fn subscribe(
    engine: &Weak<Engine>,
    control: &Control,
    writer: &mut UnixStream,
    fields: &Map<String, Value>,
) {
    let topic = fields.get("topic").and_then(Value::as_str).unwrap_or("log");
    if topic != "log" {
        let _ = writeln!(writer, "{}", refusal(&format!("no topic `{topic}` (log)")));
        return;
    }
    let mut seen = match number(fields, "since", logbook::next_sequence().saturating_sub(1)) {
        Ok(since) => since,
        Err(error) => {
            let _ = writeln!(writer, "{}", refusal(&error));
            return;
        }
    };
    let mut subscribed = Map::new();
    subscribed.insert("subscribed".into(), json!("log"));
    if writeln!(writer, "{}", reply(true, subscribed)).is_err() {
        return;
    }
    while !control.stopped() && engine.strong_count() > 0 {
        for entry in log_since(seen, 256) {
            seen = entry.sequence;
            if writeln!(writer, "{}", json!({ "log": log_json(entry) })).is_err() {
                return;
            }
        }
        std::thread::sleep(LOG_POLL);
    }
}

impl Engine {
    /// Whether a level is running, to draw.
    fn has_level(&self) -> bool {
        self.lock()
            .running
            .as_ref()
            .is_some_and(|running| running.level.is_some())
    }

    /// The world's state for the socket's `state`.
    fn control_state(&self) -> Map<String, Value> {
        state_fields(&self.lock())
    }
}

/// The world's state: whether it plays, its level and mission, its time
/// and the player's place.
pub(crate) fn state_fields(state: &crate::State) -> Map<String, Value> {
    let running = state.running.as_ref();
    let player = running.and_then(|running| {
        let handle = running.simulation.player()?.handle();
        let at = running.simulation.transform(handle)?;
        Some(json!([at.position.x, at.position.y, at.position.z]))
    });
    let mut fields = Map::new();
    fields.insert("playing".into(), Value::Bool(running.is_some()));
    fields.insert(
        "level".into(),
        Value::Bool(running.is_some_and(|running| running.level.is_some())),
    );
    fields.insert(
        "mission".into(),
        Value::Bool(running.is_some_and(|running| running.campaign.is_some())),
    );
    fields.insert("paused".into(), Value::Bool(state.shell.paused));
    fields.insert("time_scale".into(), json!(state.shell.time_scale));
    fields.insert(
        "tick".into(),
        json!(running.map_or(0, |running| running.simulation.tick().0)),
    );
    fields.insert("player".into(), player.unwrap_or(Value::Null));
    fields
}
