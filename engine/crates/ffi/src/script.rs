//! The test driver over the engine (`opence_dev::script`): `.ocs` scripts
//! run headless as fast as the world ticks ([`Engine::run_script`]), over
//! the control socket with the app drawing their frames, or in the window
//! by the shell's `script` command, at the window's pace.
//!
//! A script's values are the shell's variables, the world's state
//! (`tick`, `paused`, `playing`, `level`, `mission`, `objects`,
//! `player.x`, `player.y`, `player.z`, `player.alive`) and HaloScript.

use std::path::{Path, PathBuf};
use std::time::{Duration, Instant};

use opence_dev::logbook::{self, Level};
use opence_dev::script::{Failure, Progress, Ran, ScriptHost, ScriptRunner};
use opence_dev::{Data, Registry};
use opence_sim::damage::Vitality;
use serde_json::{json, Map, Value};

use crate::control::{state_fields, Control, FRAME_TIMEOUT};
use crate::devtools::{self, ShellAction};
use crate::{publish, run_ticks, Engine, State};

/// The most ticks one script runs (four hours).
pub const MAX_SCRIPT_TICKS: u64 = 4 * 60 * 60 * 30;

/// A script's frames' size.
pub const SCRIPT_FRAME: (u32, u32) = (1280, 720);

/// How long a script over the socket waits for a world to run in.
const STALL: Duration = Duration::from_secs(30);

/// How often it looks for one.
const STALL_POLL: Duration = Duration::from_millis(10);

/// What a script that ran headless did.
#[derive(Clone, Debug, PartialEq, uniffi::Record)]
pub struct ScriptReport {
    /// Every line passed.
    pub ok: bool,
    /// The lines that ran.
    pub lines: u32,
    /// The ticks the world ran.
    pub ticks: u64,
    /// The frames' names, in order (headless, none is drawn).
    pub frames: Vec<String>,
    /// Why it stopped, if a line failed.
    pub failure: Option<ScriptFailure>,
    /// The report as text.
    pub text: String,
}

/// The line a script stopped at.
#[derive(Clone, Debug, PartialEq, uniffi::Record)]
pub struct ScriptFailure {
    /// Its number, from 1 (0 for the whole script).
    pub line: u32,
    /// Its text.
    pub text: String,
    /// The expression that did not hold, if it was one.
    pub expression: Option<String>,
    /// Its value, if it had one.
    pub value: Option<String>,
    /// Why.
    pub reason: String,
    /// The world's state then, as JSON.
    pub state: String,
}

impl From<&Failure> for ScriptFailure {
    fn from(failure: &Failure) -> Self {
        Self {
            line: u32::try_from(failure.line).unwrap_or(u32::MAX),
            text: failure.text.clone(),
            expression: failure.expression.clone(),
            value: failure.value.clone(),
            reason: failure.reason.clone(),
            state: failure.state.to_string(),
        }
    }
}

/// Where a script runs.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
enum Mode {
    /// Without the app: lines that need it fail; frames are only named.
    Headless,
    /// Over the control socket: the app acts on lines and draws frames
    /// between polls.
    Socket,
    /// In the window, polled each frame.
    Window,
}

/// What a script's host keeps between polls.
#[derive(Debug)]
struct Io {
    mode: Mode,
    /// Where its frames are written.
    folder: Option<PathBuf>,
    /// A line queued an action for the app (over the socket: to wait for).
    queued: bool,
    /// Over the socket: whether the app acted on it.
    settled: Option<Result<(), String>>,
    /// Over the socket: the frame to draw after the poll, then its result.
    wanted: Option<String>,
    drawn: Option<Result<(), String>>,
    /// In the window: the frame queued, and when.
    pending: Option<(u64, Instant)>,
}

/// A script and its host's state.
#[derive(Debug)]
pub(crate) struct Session {
    runner: ScriptRunner,
    io: Io,
    /// The file it was read from, in the window.
    path: Option<PathBuf>,
}

impl Session {
    fn new(runner: ScriptRunner, mode: Mode, folder: Option<PathBuf>) -> Self {
        Self {
            runner,
            io: Io {
                mode,
                folder,
                queued: false,
                settled: None,
                wanted: None,
                drawn: None,
                pending: None,
            },
            path: None,
        }
    }

    fn poll(
        &mut self,
        registry: &mut Registry<State>,
        state: &mut State,
        control: &Control,
    ) -> Progress {
        let mut host = Host {
            registry,
            state,
            control,
            io: &mut self.io,
        };
        self.runner.poll(&mut host)
    }

    fn abort(
        &mut self,
        registry: &mut Registry<State>,
        state: &mut State,
        control: &Control,
        reason: String,
    ) -> Progress {
        let mut host = Host {
            registry,
            state,
            control,
            io: &mut self.io,
        };
        self.runner.abort(&mut host, reason)
    }
}

struct Host<'a> {
    registry: &'a mut Registry<State>,
    state: &'a mut State,
    control: &'a Control,
    io: &'a mut Io,
}

impl ScriptHost for Host<'_> {
    fn run(&mut self, line: &str) -> Result<Ran, String> {
        if line.split_whitespace().next() == Some("script") {
            return Err("a script cannot run a script".into());
        }
        let output = devtools::run(self.registry, self.state, line);
        if !output.ok {
            return Err(output.text);
        }
        if output.action == ShellAction::None {
            return Ok(Ran {
                text: output.text,
                settle: false,
            });
        }
        if self.io.mode == Mode::Headless {
            return Err(format!(
                "`{line}` needs the app: run the script over the control socket (opence script)"
            ));
        }
        self.control.queue_action(output.action);
        self.io.queued = true;
        Ok(Ran {
            text: output.text,
            settle: true,
        })
    }

    fn value(&mut self, expression: &str) -> Result<String, String> {
        value(self.registry, self.state, expression)
    }

    fn frame(&mut self, name: &str) -> Result<bool, String> {
        match self.io.mode {
            Mode::Headless => Ok(true),
            Mode::Socket => {
                if let Some(drawn) = self.io.drawn.take() {
                    return drawn.map(|()| true);
                }
                self.frame_path(name)?;
                self.io.wanted = Some(name.to_owned());
                Ok(false)
            }
            Mode::Window => {
                if let Some((id, since)) = self.io.pending {
                    return match self.control.take_finished(id) {
                        Some(error) => {
                            self.io.pending = None;
                            error.map_or(Ok(true), Err)
                        }
                        None if since.elapsed() > FRAME_TIMEOUT => {
                            self.io.pending = None;
                            self.control.forget_frame(id);
                            Err(format!(
                                "no frame was drawn in {} seconds",
                                FRAME_TIMEOUT.as_secs()
                            ))
                        }
                        None => Ok(false),
                    };
                }
                let path = self.frame_path(name)?;
                let (width, height) = SCRIPT_FRAME;
                let id = self.control.queue_frame(path, width, height);
                self.io.pending = Some((id, Instant::now()));
                Ok(false)
            }
        }
    }

    fn settled(&mut self) -> Result<bool, String> {
        match self.io.mode {
            // (the app acts on the queue before the next frame's poll)
            Mode::Headless | Mode::Window => Ok(true),
            Mode::Socket => match self.io.settled.take() {
                Some(settled) => settled.map(|()| true),
                None => Ok(false),
            },
        }
    }

    fn tick(&mut self) -> u64 {
        tick(self.state)
    }

    fn state(&mut self) -> Data {
        Value::Object(state_fields(self.state))
    }
}

impl Host<'_> {
    /// The PNG for the frame `name` (its folder made), if a level runs.
    fn frame_path(&self, name: &str) -> Result<String, String> {
        let level = self
            .state
            .running
            .as_ref()
            .is_some_and(|running| running.level.is_some());
        if !level {
            return Err("no level is loaded to draw".into());
        }
        let folder = self
            .io
            .folder
            .as_deref()
            .ok_or("frames need a folder to be written to")?;
        std::fs::create_dir_all(folder)
            .map_err(|error| format!("the frames' folder {}: {error}", folder.display()))?;
        Ok(folder
            .join(format!("{name}.png"))
            .to_string_lossy()
            .into_owned())
    }
}

fn tick(state: &State) -> u64 {
    state
        .running
        .as_ref()
        .map_or(0, |running| running.simulation.tick().0)
}

/// A value of a script's expression: a variable, the world's state, else
/// HaloScript (a command is not run).
pub(crate) fn value(
    registry: &Registry<State>,
    state: &mut State,
    expression: &str,
) -> Result<String, String> {
    if let Some(value) = registry.get(expression) {
        return Ok(value.to_string());
    }
    if let Some(value) = probe(state, expression) {
        return Ok(value);
    }
    let first = expression.split_whitespace().next().unwrap_or("");
    if registry.has_command(first) {
        return Err(format!("`{first}` is a command, not a value"));
    }
    devtools::halo_script(state, expression)
        .map(|output| output.text)
        .map_err(|error| error.to_string())
}

/// The world's state's values.
fn probe(state: &mut State, name: &str) -> Option<String> {
    let paused = state.shell.paused;
    let running = state.running.as_mut();
    let value = match name {
        "tick" => running
            .map_or(0, |running| running.simulation.tick().0)
            .to_string(),
        "paused" => paused.to_string(),
        "playing" => running.is_some().to_string(),
        "level" => running
            .is_some_and(|running| running.level.is_some())
            .to_string(),
        "mission" => running
            .is_some_and(|running| running.campaign.is_some())
            .to_string(),
        "objects" => running
            .map_or(0, |running| running.simulation.objects().len())
            .to_string(),
        "player.x" | "player.y" | "player.z" | "player.alive" => {
            let running = running?;
            let handle = running.simulation.player()?.handle();
            if name == "player.alive" {
                let vitality = running.simulation.component::<Vitality>(handle);
                return Some(vitality.is_none_or(|vitality| !vitality.dead).to_string());
            }
            let at = running.simulation.transform(handle)?.position;
            let axis = match name {
                "player.x" => at.x,
                "player.y" => at.y,
                _ => at.z,
            };
            axis.to_string()
        }
        _ => return None,
    };
    Some(value)
}

/// A finished script's results, for a reply: each line's number, whether
/// it passed and its text (or error).
fn results(runner: &ScriptRunner) -> Value {
    Value::Array(
        runner
            .results()
            .iter()
            .map(|result| {
                let mut fields = Map::new();
                fields.insert("line".into(), json!(result.line));
                fields.insert("ok".into(), Value::Bool(result.ok));
                let key = if result.ok { "text" } else { "error" };
                fields.insert(key.into(), Value::String(result.text.clone()));
                Value::Object(fields)
            })
            .collect(),
    )
}

/// A script that cannot be read, as a failure at its line.
fn unreadable(state: &State, source: &str, line: usize, reason: String) -> Failure {
    Failure {
        line,
        text: source
            .lines()
            .nth(line.saturating_sub(1))
            .unwrap_or("")
            .trim()
            .to_owned(),
        expression: None,
        value: None,
        reason,
        state: Value::Object(state_fields(state)),
    }
}

/// Runs a script over the control socket: its lines run with the engine's
/// locks held, its waits tick the world (paused while it runs, so that
/// only the script runs it), and the app acts on its lines and draws its
/// frames in `folder` between polls. The reply's `results` are its lines'.
pub(crate) fn over_socket(
    engine: &Engine,
    source: &str,
    folder: Option<PathBuf>,
) -> (bool, Map<String, Value>) {
    let runner = match ScriptRunner::parse(source) {
        Ok(runner) => runner,
        Err(error) => {
            let failure = unreadable(&engine.lock(), source, error.line, error.reason);
            let mut fields = Map::new();
            fields.insert("results".into(), json!([]));
            fields.insert("error".into(), Value::String(failure.to_string()));
            fields.insert("failure".into(), failure.data());
            return (false, fields);
        }
    };
    let mut session = Session::new(runner, Mode::Socket, folder);
    let was_paused = engine.lock().shell.paused;
    let mut ticks = 0_u64;
    let mut stalled: Option<Instant> = None;
    let progress = loop {
        let progress = {
            let mut registry = engine.lock_registry();
            let mut state = engine.lock();
            state.shell.paused = true;
            let progress = session.poll(&mut registry, &mut state, &engine.control);
            publish(&mut state, &engine.snapshots, 1.0);
            progress
        };
        if progress != Progress::Waiting {
            break progress;
        }
        if std::mem::take(&mut session.io.queued) {
            session.io.settled = Some(engine.control.wait_served());
            continue;
        }
        if let Some(name) = session.io.wanted.take() {
            let folder = session.io.folder.clone().unwrap_or_default();
            let path = folder.join(format!("{name}.png"));
            let (width, height) = SCRIPT_FRAME;
            session.io.drawn = Some(engine.control.draw(
                path.to_string_lossy().into_owned(),
                width,
                height,
            ));
            continue;
        }
        let mut registry = engine.lock_registry();
        let mut state = engine.lock();
        let abort = if state.running.is_none() {
            let since = *stalled.get_or_insert_with(Instant::now);
            (since.elapsed() > STALL).then(|| {
                format!(
                    "no world ran for the script to wait in for {} seconds",
                    STALL.as_secs()
                )
            })
        } else if ticks >= MAX_SCRIPT_TICKS {
            Some(format!("a script runs at most {MAX_SCRIPT_TICKS} ticks"))
        } else {
            stalled = None;
            ticks += 1;
            run_ticks(&mut state, 1)
                .err()
                .map(|error| error.to_string())
        };
        if let Some(reason) = abort {
            break session.abort(&mut registry, &mut state, &engine.control, reason);
        }
        if state.running.is_none() {
            drop(state);
            drop(registry);
            std::thread::sleep(STALL_POLL);
        }
    };
    engine.lock().shell.paused = was_paused;
    let mut fields = Map::new();
    fields.insert("results".into(), results(&session.runner));
    fields.insert("ticks".into(), json!(ticks));
    fields.insert("frames".into(), json!(session.runner.frames()));
    match progress {
        Progress::Failed(failure) => {
            fields.insert("error".into(), Value::String(failure.to_string()));
            fields.insert("failure".into(), failure.data());
            (false, fields)
        }
        _ => (true, fields),
    }
}

/// The shell's `script` command: runs the script at `argument` in the
/// window, `stop` stops it, nothing tells where it stands.
pub(crate) fn command(state: &mut State, argument: &str) -> Result<String, String> {
    match argument {
        "" => Ok(match &state.shell.script {
            Some(session) => format!(
                "{} runs, at line {}",
                session
                    .path
                    .as_deref()
                    .map_or_else(String::new, |path| path.display().to_string()),
                session.runner.current().map_or(0, |line| line.number)
            ),
            None if state.shell.script_status.is_empty() => "no script runs".into(),
            None => state.shell.script_status.clone(),
        }),
        "stop" => Ok(match state.shell.script.take() {
            Some(_) => "the script stopped".into(),
            None => "no script runs".into(),
        }),
        path => {
            let path = PathBuf::from(path);
            let source = read(&path)?;
            let runner = ScriptRunner::parse(&source).map_err(|error| error.to_string())?;
            let stem = path
                .file_stem()
                .map_or_else(|| "script".into(), |stem| stem.to_string_lossy());
            let folder = path
                .parent()
                .unwrap_or_else(|| Path::new("."))
                .join(format!("{stem}-frames"));
            let mut session = Session::new(runner, Mode::Window, Some(folder));
            session.path = Some(path.clone());
            state.shell.script = Some(session);
            state.shell.script_status.clear();
            Ok(format!("{} runs", path.display()))
        }
    }
}

fn read(path: &Path) -> Result<String, String> {
    let size = std::fs::metadata(path)
        .map_err(|error| format!("{}: {error}", path.display()))?
        .len();
    if size > opence_dev::script::MAX_SCRIPT_BYTES as u64 {
        return Err(format!(
            "{}: a script is at most {} bytes",
            path.display(),
            opence_dev::script::MAX_SCRIPT_BYTES
        ));
    }
    std::fs::read_to_string(path).map_err(|error| format!("{}: {error}", path.display()))
}

/// Polls the window's script, once a frame: while the world is paused it
/// runs a tick after it for the script's waits. A script that ends is logged.
pub(crate) fn poll_window(registry: &mut Registry<State>, state: &mut State, control: &Control) {
    let Some(mut session) = state.shell.script.take() else {
        return;
    };
    let name = session
        .path
        .as_deref()
        .map_or_else(|| "the script".into(), |path| path.display().to_string());
    match session.poll(registry, state, control) {
        Progress::Waiting => {
            // (the world is paused: the script's wait runs it)
            if state.shell.paused && state.running.is_some() {
                let _ = run_ticks(state, 1);
            }
            state.shell.script = Some(session);
        }
        Progress::Finished => {
            let text = format!(
                "{name}: its {} lines passed",
                session.runner.results().len()
            );
            logbook::record(Level::Info, "script", &text);
            state.shell.script_status = text;
        }
        Progress::Failed(failure) => {
            let text = format!("{name}: {failure}");
            logbook::record(Level::Error, "script", &text);
            state.shell.script_status = text;
        }
    }
}

#[uniffi::export]
impl Engine {
    /// Runs a script headless, as fast as the world ticks: its waits run
    /// the world (there must be one), its frames are only named, and a
    /// line that needs the app (a map to load) fails. Publishes the world
    /// as it left it.
    pub fn run_script(&self, source: String) -> ScriptReport {
        let mut registry = self.lock_registry();
        let mut state = self.lock();
        let runner = match ScriptRunner::parse(&source) {
            Ok(runner) => runner,
            Err(error) => {
                let failure = unreadable(&state, &source, error.line, error.reason);
                return ScriptReport {
                    ok: false,
                    lines: 0,
                    ticks: 0,
                    frames: Vec::new(),
                    failure: Some(ScriptFailure::from(&failure)),
                    text: failure.to_string(),
                };
            }
        };
        let mut session = Session::new(runner, Mode::Headless, None);
        let mut ticks = 0_u64;
        let progress = loop {
            match session.poll(&mut registry, &mut state, &self.control) {
                Progress::Waiting => {}
                done => break done,
            }
            let reason = if state.running.is_none() {
                Some("no world runs for the script to wait in".to_owned())
            } else if ticks >= MAX_SCRIPT_TICKS {
                Some(format!("a script runs at most {MAX_SCRIPT_TICKS} ticks"))
            } else {
                ticks += 1;
                run_ticks(&mut state, 1)
                    .err()
                    .map(|error| error.to_string())
            };
            if let Some(reason) = reason {
                break session.abort(&mut registry, &mut state, &self.control, reason);
            }
        };
        publish(&mut state, &self.snapshots, 1.0);
        let lines = u32::try_from(session.runner.results().len()).unwrap_or(u32::MAX);
        let frames = session.runner.frames().to_vec();
        match progress {
            Progress::Failed(failure) => ScriptReport {
                ok: false,
                lines,
                ticks,
                frames,
                failure: Some(ScriptFailure::from(&failure)),
                text: failure.to_string(),
            },
            _ => ScriptReport {
                ok: true,
                lines,
                ticks,
                frames,
                failure: None,
                text: format!("{lines} lines passed in {ticks} ticks"),
            },
        }
    }
}
