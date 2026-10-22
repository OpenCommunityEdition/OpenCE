//! The developer shell's commands over the running world: loading maps and
//! structures, spawning and moving things, the player's cheats, time, and
//! looking at what the world holds. A line the registry does not know is
//! HaloScript, run with the mission's scripts when a mission plays.

use std::path::{Path, PathBuf};

use opence_content::{Group, ObjectKind, TagRef};
use opence_dev::draw::{Color, DebugDraw, Layer};
use opence_dev::{ArgKind, Args, CommandError, Data, Output, Param, Registry, VarKind, VarValue};
use opence_sim::damage::{Invulnerable, Vitality};
use opence_sim::objects::{ObjectHandle, ObjectName, ObjectSpawn};
use opence_sim::{Quat, Transform, Vec3};
use serde_json::json;

use crate::{campaign, run_ticks, transform, Running, State, Wardrobe};

/// What the app must do after a line: what the engine cannot do alone.
#[derive(Clone, Debug, Default, PartialEq, uniffi::Enum)]
pub enum ShellAction {
    /// Nothing.
    #[default]
    None,
    /// Load the map at `path` and start it at structure `bsp`.
    LoadMap {
        /// The map file.
        path: String,
        /// Its structure BSP.
        bsp: u32,
    },
    /// Draw the level's structure `bsp` (the world already moved to it).
    ReloadBsp {
        /// The structure BSP.
        bsp: u32,
    },
    /// Quit.
    Quit,
}

/// What a line gave back.
#[derive(Clone, Debug, Default, PartialEq, uniffi::Record)]
pub struct ShellOutput {
    /// Whether it ran without an error.
    pub ok: bool,
    /// Its text, or the error's.
    pub text: String,
    /// Its data, as JSON (`null` for none).
    pub json: String,
    /// What the app must do.
    pub action: ShellAction,
}

/// A variable of the shell, for the app's settings and drawing.
#[derive(Clone, Debug, PartialEq, uniffi::Record)]
pub struct ShellVariable {
    /// Its name.
    pub name: String,
    /// Its value, as text.
    pub value: String,
    /// Its default, as text.
    pub default_value: String,
    /// What it does.
    pub help: String,
}

/// The most objects one `spawn` makes.
pub const MAX_SPAWN: i64 = 64;

/// The most ticks one `step` runs (ten seconds).
pub const MAX_STEP: i64 = 600;

/// The most expressions the watch line shows.
pub const MAX_WATCHES: usize = 8;

/// The longest watched expression.
pub const MAX_WATCH_LENGTH: usize = 200;

/// A watched expression's value, for the watch line.
#[derive(Clone, Debug, PartialEq, uniffi::Record)]
pub struct WatchValue {
    /// The expression.
    pub expression: String,
    /// Its value, or why it has none.
    pub value: String,
    /// Whether it has a value.
    pub ok: bool,
}

/// The fastest `time` runs the world.
pub const MAX_TIME_SCALE: f64 = 16.0;

/// The shell's state that is the engine's, not the world's.
#[derive(Debug)]
pub(crate) struct Shell {
    /// The world is paused: frames run no ticks.
    pub paused: bool,
    /// How fast the world runs, against real time.
    pub time_scale: f64,
    /// Where the maps are, for `map`.
    pub maps_folder: Option<PathBuf>,
    /// What the app must do after the line.
    pub action: ShellAction,
    /// The watch line's expressions: variables or HaloScript.
    pub watches: Vec<String>,
    /// The shapes the commands and systems draw.
    pub draw: DebugDraw,
    /// The world layers' shapes, drawn again at each publish.
    pub scene: DebugDraw,
    /// The structure BSP the world plays in (without a mission).
    pub bsp: usize,
    /// Where the trace being taken is written.
    pub trace: Option<PathBuf>,
    /// The script running in the window (`script`), and how the last ended.
    pub script: Option<crate::script::Session>,
    pub script_status: String,
}

impl Default for Shell {
    fn default() -> Self {
        Self {
            paused: false,
            time_scale: 1.0,
            maps_folder: None,
            action: ShellAction::None,
            watches: Vec::new(),
            draw: DebugDraw::new(),
            scene: DebugDraw::new(),
            bsp: 0,
            trace: None,
            script: None,
            script_status: String::new(),
        }
    }
}

// (the drawing switches the app reads)
const VARIABLES: [(&str, &str, bool, bool); 4] = [
    (
        "render.wireframe",
        "Draws the level and objects as lines.",
        false,
        false,
    ),
    (
        "render.lightmaps",
        "Draws the level with its lightmaps (off: lit flat).",
        true,
        true,
    ),
    ("render.objects", "Draws the objects.", true, false),
    ("render.hud", "Draws the HUD.", true, true),
];

// (the debug drawing's layers: `debug.draw.<layer>`)
const LAYERS: [(Layer, &str); 8] = [
    (
        Layer::General,
        "Draws the shapes the commands and systems draw.",
    ),
    (
        Layer::Collision,
        "Draws the level's collision near the camera.",
    ),
    (Layer::Bodies, "Draws the objects' bounding spheres."),
    (Layer::Ai, "Draws the AI's firing positions and paths."),
    (Layer::Triggers, "Draws the trigger volumes."),
    (Layer::Markers, "Draws the objects' axes."),
    (Layer::Lights, "Draws the lights."),
    (Layer::Sounds, "Draws the sounds."),
];

/// The prefix of the debug drawing's variables.
const DRAW_PREFIX: &str = "debug.draw.";

/// Turns a layer on or off when its variable changes.
fn draw_layer(state: &mut State, name: &str, value: &VarValue) {
    let Some(layer) = name.strip_prefix(DRAW_PREFIX).and_then(Layer::named) else {
        return;
    };
    let on = matches!(value, VarValue::Bool(true));
    state.shell.draw.set_layer(layer, on);
    state.shell.scene.set_layer(layer, on);
}

/// The registry with the engine's commands and variables.
pub(crate) fn registry() -> Registry<State> {
    let mut registry = Registry::new();
    registry.command(
        "map",
        "Loads a map from the maps folder (or a path) at a structure BSP.",
        &[
            Param::required("name", ArgKind::String),
            Param::optional("bsp", ArgKind::Int),
        ],
        map,
    );
    registry.command(
        "bsp",
        "Moves the world to the level's structure BSP.",
        &[Param::required("index", ArgKind::Int)],
        bsp,
    );
    registry.command(
        "spawn",
        "Makes objects of a tag in front of the camera.",
        &[
            Param::required("tag", ArgKind::String),
            Param::optional("count", ArgKind::Int),
        ],
        spawn,
    );
    registry.command(
        "teleport",
        "Moves the player (or the camera) to a point or beside an object.",
        &[
            Param::required("x_or_object", ArgKind::String),
            Param::optional("y", ArgKind::Float),
            Param::optional("z", ArgKind::Float),
        ],
        teleport,
    );
    registry.command(
        "noclip",
        "Flies the player through walls.",
        &[Param::optional("on", ArgKind::Bool)],
        noclip,
    );
    registry.command(
        "god",
        "Makes the player take no damage.",
        &[Param::optional("on", ArgKind::Bool)],
        god,
    );
    registry.command(
        "give",
        "Puts a weapon of the level's in the player's hands.",
        &[Param::required("weapon", ArgKind::Rest)],
        give,
    );
    registry.command(
        "enter",
        "Sits the player in a vehicle's seat.",
        &[
            Param::required("vehicle", ArgKind::Int),
            Param::optional("seat", ArgKind::Int),
        ],
        enter,
    );
    registry.command(
        "kill",
        "Kills an object, every unit but the player (all), or the player.",
        &[Param::optional("object", ArgKind::String)],
        kill,
    );
    registry.command(
        "time",
        "Runs the world faster or slower than real time.",
        &[Param::required("scale", ArgKind::Float)],
        time,
    );
    registry.command(
        "pause",
        "Stops the world's ticks (or starts them again).",
        &[Param::optional("on", ArgKind::Bool)],
        pause,
    );
    registry.command(
        "step",
        "Runs ticks of a paused world.",
        &[Param::optional("ticks", ArgKind::Int)],
        step,
    );
    registry.command(
        "objects",
        "Lists the objects whose tags have the filter in them.",
        &[Param::optional("filter", ArgKind::String)],
        objects,
    );
    registry.command(
        "inspect",
        "Shows an object's kind, tag, place and health.",
        &[Param::required("object", ArgKind::Int)],
        inspect,
    );
    registry.command(
        "tags",
        "Lists the level's object tags whose paths have the filter in them.",
        &[Param::optional("filter", ArgKind::String)],
        tags,
    );
    registry.command(
        "log",
        "Sets which lines the log keeps (render=debug,warn), or shows its last lines.",
        &[Param::optional("filter", ArgKind::String)],
        log,
    );
    registry.command(
        "watch",
        "Shows a variable's or a HaloScript expression's value on the watch line (or lists them).",
        &[Param::optional("expression", ArgKind::Rest)],
        watch,
    );
    registry.command(
        "unwatch",
        "Takes an expression off the watch line (or all of them).",
        &[Param::optional("expression", ArgKind::Rest)],
        unwatch,
    );
    registry.command("clear", "Clears the shell's scrollback.", &[], clear);
    registry.command("quit", "Quits.", &[], quit);
    for (name, help, default, saved) in VARIABLES {
        registry.variable(name, help, VarKind::Bool, VarValue::Bool(default), saved);
    }
    for (layer, help) in LAYERS {
        let name = format!("{DRAW_PREFIX}{}", layer.name());
        let on = DebugDraw::new().is_on(layer);
        registry.variable(&name, help, VarKind::Bool, VarValue::Bool(on), false);
    }
    registry.listen(draw_layer);
    registry.command(
        "debug_line",
        "Draws a line from x y z to x y z, for some ticks (one second).",
        &[
            Param::required("x0", ArgKind::Float),
            Param::required("y0", ArgKind::Float),
            Param::required("z0", ArgKind::Float),
            Param::required("x1", ArgKind::Float),
            Param::required("y1", ArgKind::Float),
            Param::required("z1", ArgKind::Float),
            Param::optional("ticks", ArgKind::Int),
        ],
        debug_line,
    );
    registry.command(
        "debug_box",
        "Draws a box from x y z to x y z, for some ticks (one second).",
        &[
            Param::required("x0", ArgKind::Float),
            Param::required("y0", ArgKind::Float),
            Param::required("z0", ArgKind::Float),
            Param::required("x1", ArgKind::Float),
            Param::required("y1", ArgKind::Float),
            Param::required("z1", ArgKind::Float),
            Param::optional("ticks", ArgKind::Int),
        ],
        debug_box,
    );
    registry.command(
        "debug_sphere",
        "Draws a sphere at x y z of a radius, for some ticks (one second).",
        &[
            Param::required("x", ArgKind::Float),
            Param::required("y", ArgKind::Float),
            Param::required("z", ArgKind::Float),
            Param::required("radius", ArgKind::Float),
            Param::optional("ticks", ArgKind::Int),
        ],
        debug_sphere,
    );
    registry.command(
        "debug_text",
        "Draws text at x y z, for a second.",
        &[
            Param::required("x", ArgKind::Float),
            Param::required("y", ArgKind::Float),
            Param::required("z", ArgKind::Float),
            Param::required("text", ArgKind::Rest),
        ],
        debug_text,
    );
    registry.command(
        "debug_clear",
        "Removes the shapes the commands and systems drew.",
        &[],
        debug_clear,
    );
    registry.command(
        "stat",
        "Measures each frame's scopes and shows the last frame's; `stat frame off` stops.",
        &[
            Param::required("what", ArgKind::Enum(&["frame"])),
            Param::optional("switch", ArgKind::Bool),
        ],
        stat,
    );
    registry.command(
        "trace",
        "Writes the next frames' scopes as a Chrome trace (Perfetto reads it); `trace stop` writes it now.",
        &[
            Param::required("file", ArgKind::String),
            Param::optional("frames", ArgKind::Int),
        ],
        trace,
    );
    registry.command(
        "script",
        "Runs a test script (.ocs) in the window, its frames in <name>-frames; `stop` stops it.",
        &[Param::optional("file", ArgKind::Rest)],
        script,
    );
    registry.fallback(halo_script);
    registry
}

/// Runs a line, and what the app must do after it.
pub(crate) fn run(registry: &mut Registry<State>, state: &mut State, line: &str) -> ShellOutput {
    state.shell.action = ShellAction::None;
    let result = registry.run(state, line);
    let action = std::mem::take(&mut state.shell.action);
    match result {
        Ok(output) => ShellOutput {
            ok: true,
            text: output.text,
            json: output.data.to_string(),
            action,
        },
        Err(error) => ShellOutput {
            ok: false,
            text: error.to_string(),
            json: json!({ "error": error.to_string() }).to_string(),
            action,
        },
    }
}

/// The watched expressions' values: a variable's, or HaloScript's run
/// against the mission (a command is not run).
pub(crate) fn watch_values(registry: &Registry<State>, state: &mut State) -> Vec<WatchValue> {
    let watches = state.shell.watches.clone();
    watches
        .into_iter()
        .map(|expression| {
            let first = expression.split_whitespace().next().unwrap_or("");
            let (ok, value) = if let Some(value) = registry.get(&expression) {
                (true, value.to_string())
            } else if registry.has_command(first) {
                (false, "a command, not a value".to_owned())
            } else {
                match halo_script(state, &expression) {
                    Ok(output) => (true, output.text),
                    Err(error) => (false, error.to_string()),
                }
            };
            WatchValue {
                expression,
                value,
                ok,
            }
        })
        .collect()
}

/// The registry's variables, as text.
pub(crate) fn variables(registry: &Registry<State>) -> Vec<ShellVariable> {
    registry
        .variable_names()
        .filter_map(|name| {
            let value = registry.get(name)?;
            let (help, default) = registry.describe(name)?;
            Some(ShellVariable {
                name: name.to_owned(),
                value: value.to_string(),
                default_value: default.to_string(),
                help: help.to_owned(),
            })
        })
        .collect()
}

fn failed(text: impl Into<String>) -> CommandError {
    CommandError::Failed(text.into())
}

fn running(state: &mut State) -> Result<&mut Running, CommandError> {
    state
        .running
        .as_mut()
        .ok_or_else(|| failed("no world is running"))
}

fn level(running: &Running) -> Result<std::sync::Arc<opence_content::Level>, CommandError> {
    running
        .level
        .clone()
        .ok_or_else(|| failed("the world has no level"))
}

/// An object by the id `objects` shows (its slot).
fn object_of(running: &mut Running, id: i64) -> Result<ObjectHandle, CommandError> {
    running
        .simulation
        .objects()
        .into_iter()
        .find(|handle| i64::from(handle.index) == id)
        .ok_or_else(|| failed(format!("no object {id}")))
}

fn position(transform: &Transform) -> Data {
    json!([
        transform.position.x,
        transform.position.y,
        transform.position.z
    ])
}

fn kind_name(kind: ObjectKind) -> String {
    format!("{kind:?}").to_lowercase()
}

/// The map file a name means: a path as it is, else the maps folder's
/// `<name>.map`.
fn map_path(maps_folder: Option<&Path>, name: &str) -> PathBuf {
    let given = Path::new(name);
    if given.components().count() > 1 || given.is_absolute() {
        return given.to_path_buf();
    }
    let file = if given.extension().is_some() {
        name.to_owned()
    } else {
        format!("{name}.map")
    };
    maps_folder.map_or_else(|| PathBuf::from(&file), |folder| folder.join(&file))
}

fn map(state: &mut State, args: &Args) -> Result<Output, CommandError> {
    let name = args.string("name")?;
    let bsp = args.int_or("bsp", 0)?;
    let bsp = u32::try_from(bsp).map_err(|_| failed(format!("no structure BSP {bsp}")))?;
    let path = map_path(state.shell.maps_folder.as_deref(), name);
    if !path.is_file() {
        return Err(failed(format!("no map at {}", path.display())));
    }
    let path = path.to_string_lossy().into_owned();
    state.shell.action = ShellAction::LoadMap {
        path: path.clone(),
        bsp,
    };
    Ok(Output::with_data(
        format!("loading {path}"),
        json!({ "map": path, "bsp": bsp }),
    ))
}

fn bsp(state: &mut State, args: &Args) -> Result<Output, CommandError> {
    let index = args.int("index")?;
    let running = running(state)?;
    let level = level(running)?;
    let bsp = usize::try_from(index)
        .ok()
        .filter(|&bsp| bsp < level.structure_bsps.len())
        .ok_or_else(|| {
            failed(format!(
                "no structure BSP {index}: the level has {}",
                level.structure_bsps.len()
            ))
        })?;
    running
        .simulation
        .switch_bsp(&level, bsp)
        .map_err(|error| failed(error.to_string()))?;
    state.shell.bsp = bsp;
    let bsp = u32::try_from(bsp).unwrap_or(0);
    state.shell.action = ShellAction::ReloadBsp { bsp };
    Ok(Output::with_data(
        format!("structure BSP {bsp}"),
        json!({ "bsp": bsp }),
    ))
}

/// The level's object definition a path means: the exact one, else the
/// only one whose path has it in it.
fn definition<'a>(
    level: &'a opence_content::Level,
    path: &str,
    kind: Option<ObjectKind>,
) -> Result<&'a opence_content::ObjectDefinition, CommandError> {
    let path = path.replace('/', "\\");
    let fits =
        |object: &&opence_content::ObjectDefinition| kind.is_none_or(|kind| object.kind == kind);
    if let Some(exact) = level
        .objects
        .iter()
        .filter(fits)
        .find(|object| object.tag.path == path)
    {
        return Ok(exact);
    }
    let found: Vec<_> = level
        .objects
        .iter()
        .filter(fits)
        .filter(|object| object.tag.path.contains(&path))
        .collect();
    match found.as_slice() {
        [one] => Ok(one),
        [] => Err(failed(format!("the level has no tag `{path}`"))),
        many => Err(failed(format!(
            "`{path}` could be {}",
            many.iter()
                .map(|object| object.tag.path.as_str())
                .collect::<Vec<_>>()
                .join(", ")
        ))),
    }
}

/// Where the camera looks from, and the way it looks.
fn camera_ray(running: &Running) -> (Vec3, Vec3) {
    let camera = campaign::camera(running);
    (
        camera.transform.position,
        camera.transform.rotation * Vec3::X,
    )
}

fn spawn(state: &mut State, args: &Args) -> Result<Output, CommandError> {
    let path = args.string("tag")?.to_owned();
    let count = args.int_or("count", 1)?;
    if !(1..=MAX_SPAWN).contains(&count) {
        return Err(failed(format!(
            "spawn makes 1 to {MAX_SPAWN} objects, not {count}"
        )));
    }
    let running = running(state)?;
    let level = level(running)?;
    let definition = definition(&level, &path, None)?;
    let (eye, forward) = camera_ray(running);
    let flat = Vec3::new(forward.x, forward.y, 0.0).normalize_or(Vec3::X);
    let side = Vec3::new(-flat.y, flat.x, 0.0);
    let spacing = (definition.bounding_radius * 2.0).max(0.5);
    let ahead = (definition.bounding_radius + 1.5).max(2.0);
    let facing = Quat::from_rotation_z(libm::atan2f(-flat.y, -flat.x));
    let mut made = Vec::new();
    for at in 0..count {
        // (in a row across the view, the first in the middle)
        let offset = (at as f32 - (count - 1) as f32 * 0.5) * spacing;
        let spawn = ObjectSpawn {
            kind: definition.kind,
            tag: definition.tag.clone(),
            transform: Transform {
                position: eye + flat * ahead + side * offset,
                rotation: facing,
                scale: 1.0,
            },
            name: None,
            permutation: None,
            team: None,
        };
        let handle = running
            .simulation
            .spawn(spawn)
            .map_err(|_| failed("the world holds no more objects"))?;
        Wardrobe::default().dress(&mut running.simulation, &level, handle);
        made.push(handle.index);
    }
    Ok(Output::with_data(
        format!(
            "made {} {} ({})",
            made.len(),
            definition.tag.path,
            made.iter()
                .map(u32::to_string)
                .collect::<Vec<_>>()
                .join(" ")
        ),
        json!(made),
    ))
}

fn teleport(state: &mut State, args: &Args) -> Result<Output, CommandError> {
    let first = args.string("x_or_object")?.to_owned();
    let running = running(state)?;
    let to = if args.has("y") {
        let x = first
            .parse::<f32>()
            .ok()
            .filter(|x| x.is_finite())
            .ok_or_else(|| failed(format!("teleport: x must be a number, not `{first}`")))?;
        let y = args.float("y")? as f32;
        let z = args.float_or("z", 0.0)? as f32;
        Vec3::new(x, y, z)
    } else {
        let id = first
            .parse::<i64>()
            .map_err(|_| failed(format!("teleport: `{first}` is not an object or a point")))?;
        let handle = object_of(running, id)?;
        let at = running
            .simulation
            .transform(handle)
            .ok_or_else(|| failed(format!("no object {id}")))?;
        let radius = running
            .simulation
            .object(handle)
            .and_then(|object| {
                running
                    .level
                    .as_ref()
                    .and_then(|level| level.object(&object.tag))
                    .map(|definition| definition.bounding_radius)
            })
            .unwrap_or(0.5);
        // (beside it, a little above, not inside it)
        at.position + Vec3::new(0.0, radius + 1.0, 0.5)
    };
    let moved = if running.simulation.player().is_some() {
        running.simulation.teleport_player(0, to)
    } else {
        let mut free = opence_sim::FreeCamera {
            position: to,
            ..Default::default()
        };
        let (yaw, pitch) = camera_angles(running);
        free.yaw = yaw;
        free.pitch = pitch;
        running.simulation.place_free_camera(free);
        true
    };
    if !moved {
        return Err(failed("the player could not be moved there"));
    }
    running.previous_camera = transform(&campaign::camera(running).transform);
    Ok(Output::with_data(
        format!("at {} {} {}", to.x, to.y, to.z),
        json!([to.x, to.y, to.z]),
    ))
}

/// The camera's yaw and pitch, kept when it is moved.
fn camera_angles(running: &Running) -> (opence_sim::units::Radians, opence_sim::units::Radians) {
    let (_, forward) = camera_ray(running);
    (
        opence_sim::units::Radians(libm::atan2f(forward.y, forward.x)),
        opence_sim::units::Radians(libm::asinf(forward.z.clamp(-1.0, 1.0))),
    )
}

fn on_or_toggle(args: &Args, now: bool) -> Result<bool, CommandError> {
    if args.has("on") {
        args.bool("on")
    } else {
        Ok(!now)
    }
}

fn noclip(state: &mut State, args: &Args) -> Result<Output, CommandError> {
    let running = running(state)?;
    let player = running
        .simulation
        .player()
        .ok_or_else(|| failed("there is no player"))?;
    let on = on_or_toggle(args, player.noclip())?;
    if !running.simulation.set_noclip(on) {
        return Err(failed("a seated player cannot fly"));
    }
    Ok(Output::with_data(
        format!("noclip {}", if on { "on" } else { "off" }),
        Data::Bool(on),
    ))
}

fn god(state: &mut State, args: &Args) -> Result<Output, CommandError> {
    let running = running(state)?;
    let handle = running
        .simulation
        .player()
        .map(opence_sim::player::Player::handle)
        .ok_or_else(|| failed("there is no player"))?;
    let now = running
        .simulation
        .component::<Invulnerable>(handle)
        .is_some();
    let on = on_or_toggle(args, now)?;
    running.simulation.set_invulnerable(handle, on);
    Ok(Output::with_data(
        format!("god {}", if on { "on" } else { "off" }),
        Data::Bool(on),
    ))
}

fn give(state: &mut State, args: &Args) -> Result<Output, CommandError> {
    let path = args.string("weapon")?.to_owned();
    let running = running(state)?;
    let level = level(running)?;
    if running.simulation.player().is_none() {
        return Err(failed("there is no player"));
    }
    let tag = definition(&level, &path, Some(ObjectKind::Weapon))?
        .tag
        .clone();
    if !running.simulation.give_weapon(&tag) {
        return Err(failed(format!("the player cannot carry {}", tag.path)));
    }
    Ok(Output::with_data(
        format!("gave {}", tag.path),
        Data::String(tag.path),
    ))
}

fn enter(state: &mut State, args: &Args) -> Result<Output, CommandError> {
    let id = args.int("vehicle")?;
    let seat = args.int_or("seat", 0)?;
    let running = running(state)?;
    let handle = object_of(running, id)?;
    let seat = usize::try_from(seat).map_err(|_| failed(format!("no seat {seat}")))?;
    if !running.simulation.enter_vehicle(handle, seat) {
        return Err(failed(format!(
            "the player cannot sit in seat {seat} of object {id}"
        )));
    }
    Ok(Output::with_data(
        format!("in seat {seat} of {id}"),
        json!({ "vehicle": id, "seat": seat }),
    ))
}

fn kill(state: &mut State, args: &Args) -> Result<Output, CommandError> {
    let which = args.string_or("object", "player")?.to_owned();
    let running = running(state)?;
    let player = running
        .simulation
        .player()
        .map(opence_sim::player::Player::handle);
    let killed: Vec<ObjectHandle> = match which.as_str() {
        "player" => vec![player.ok_or_else(|| failed("there is no player"))?],
        "all" => running
            .simulation
            .objects()
            .into_iter()
            .filter(|&handle| Some(handle) != player)
            .filter(|&handle| {
                running.simulation.object(handle).is_some_and(|object| {
                    matches!(object.kind, ObjectKind::Biped | ObjectKind::Vehicle)
                })
            })
            .collect(),
        id => {
            let id = id
                .parse::<i64>()
                .map_err(|_| failed(format!("kill: `{id}` is not an object, all or player")))?;
            vec![object_of(running, id)?]
        }
    };
    for &handle in &killed {
        running.simulation.kill(handle);
    }
    let ids: Vec<u32> = killed.iter().map(|handle| handle.index).collect();
    Ok(Output::with_data(
        format!("killed {}", ids.len()),
        json!(ids),
    ))
}

fn time(state: &mut State, args: &Args) -> Result<Output, CommandError> {
    let scale = args.float("scale")?;
    if !(0.0..=MAX_TIME_SCALE).contains(&scale) {
        return Err(failed(format!(
            "time runs 0 to {MAX_TIME_SCALE} times as fast, not {scale}"
        )));
    }
    state.shell.time_scale = scale;
    Ok(Output::with_data(
        format!("time {scale}"),
        Data::from(scale),
    ))
}

fn pause(state: &mut State, args: &Args) -> Result<Output, CommandError> {
    let on = on_or_toggle(args, state.shell.paused)?;
    state.shell.paused = on;
    Ok(Output::with_data(
        if on { "paused" } else { "running" },
        Data::Bool(on),
    ))
}

fn step(state: &mut State, args: &Args) -> Result<Output, CommandError> {
    let ticks = args.int_or("ticks", 1)?;
    if !(1..=MAX_STEP).contains(&ticks) {
        return Err(failed(format!(
            "step runs 1 to {MAX_STEP} ticks, not {ticks}"
        )));
    }
    running(state)?;
    state.shell.paused = true;
    run_ticks(state, u32::try_from(ticks).unwrap_or(1))
        .map_err(|error| failed(error.to_string()))?;
    let tick = state
        .running
        .as_ref()
        .map_or(0, |running| running.simulation.tick().0);
    Ok(Output::with_data(format!("tick {tick}"), Data::from(tick)))
}

/// An object's row: its id, kind, tag and place.
fn row(running: &Running, handle: ObjectHandle) -> Option<(String, Data)> {
    let object = running.simulation.object(handle)?;
    let at = running.simulation.transform(handle)?;
    let text = format!(
        "{:>4} {:<10} {} ({:.2} {:.2} {:.2})",
        handle.index,
        kind_name(object.kind),
        object.tag.path,
        at.position.x,
        at.position.y,
        at.position.z
    );
    let data = json!({
        "id": handle.index,
        "kind": kind_name(object.kind),
        "tag": object.tag.path,
        "position": position(&at),
    });
    Some((text, data))
}

fn objects(state: &mut State, args: &Args) -> Result<Output, CommandError> {
    let filter = args.string_or("filter", "")?.replace('/', "\\");
    let running = running(state)?;
    let rows: Vec<(String, Data)> = running
        .simulation
        .objects()
        .into_iter()
        .filter_map(|handle| row(running, handle))
        .filter(|(_, data)| {
            data["tag"]
                .as_str()
                .is_some_and(|tag| tag.contains(&filter))
                || data["kind"] == filter.as_str()
        })
        .collect();
    let text = rows
        .iter()
        .map(|(text, _)| text.as_str())
        .collect::<Vec<_>>()
        .join("\n");
    Ok(Output::with_data(
        text,
        Data::Array(rows.into_iter().map(|(_, data)| data).collect()),
    ))
}

fn inspect(state: &mut State, args: &Args) -> Result<Output, CommandError> {
    let id = args.int("object")?;
    let running = running(state)?;
    let handle = object_of(running, id)?;
    let (text, mut data) = row(running, handle).ok_or_else(|| failed(format!("no object {id}")))?;
    let mut lines = vec![text];
    if let Some(name) = running.simulation.component::<ObjectName>(handle) {
        lines.push(format!("  name {}", name.0));
        data["name"] = Data::String(name.0);
    }
    if let Some(vitality) = running.simulation.component::<Vitality>(handle) {
        lines.push(format!(
            "  body {:.2} shield {:.2}{}",
            vitality.body,
            vitality.shield,
            if vitality.dead { " dead" } else { "" }
        ));
        data["body"] = json!(vitality.body);
        data["shield"] = json!(vitality.shield);
        data["dead"] = Data::Bool(vitality.dead);
    }
    if running
        .simulation
        .component::<Invulnerable>(handle)
        .is_some()
    {
        lines.push("  invulnerable".into());
        data["invulnerable"] = Data::Bool(true);
    }
    if let Some(at) = running.simulation.transform(handle) {
        let rotation = at.rotation;
        data["rotation"] = json!([rotation.x, rotation.y, rotation.z, rotation.w]);
    }
    Ok(Output::with_data(lines.join("\n"), data))
}

fn tags(state: &mut State, args: &Args) -> Result<Output, CommandError> {
    let filter = args.string_or("filter", "")?.replace('/', "\\");
    let running = running(state)?;
    let level = level(running)?;
    let found: Vec<(&TagRef, ObjectKind)> = level
        .objects
        .iter()
        .filter(|object| object.tag.path.contains(&filter))
        .map(|object| (&object.tag, object.kind))
        .collect();
    let text = found
        .iter()
        .map(|(tag, kind)| format!("{:<10} {}", kind_name(*kind), tag.path))
        .collect::<Vec<_>>()
        .join("\n");
    let data = found
        .iter()
        .map(|(tag, kind)| {
            json!({ "kind": kind_name(*kind), "group": group_name(tag.group), "path": tag.path })
        })
        .collect();
    Ok(Output::with_data(text, Data::Array(data)))
}

fn group_name(group: Group) -> String {
    String::from_utf8_lossy(&group.0).trim_end().to_owned()
}

/// The lines `log` shows.
const LOG_LINES: usize = 20;

fn log(_: &mut State, args: &Args) -> Result<Output, CommandError> {
    use opence_dev::logbook;
    if args.has("filter") {
        let filter = logbook::Filter::parse(args.string("filter")?).map_err(failed)?;
        let text = format!("the log keeps {filter}");
        let data = Data::String(filter.to_string());
        logbook::set_filter(filter);
        return Ok(Output::with_data(text, data));
    }
    let next = logbook::next_sequence();
    let lines = logbook::since(next.saturating_sub(LOG_LINES as u64 + 1), LOG_LINES);
    let mut text = vec![format!("the log keeps {}", logbook::filter())];
    text.extend(lines.iter().map(|line| {
        format!(
            "{:>6} {:<5} {}: {}",
            line.sequence,
            line.level.as_str().to_lowercase(),
            line.target,
            line.text
        )
    }));
    let data = lines
        .iter()
        .map(|line| json!({ "sequence": line.sequence, "target": line.target, "text": line.text }))
        .collect();
    Ok(Output::with_data(text.join("\n"), Data::Array(data)))
}

fn watch(state: &mut State, args: &Args) -> Result<Output, CommandError> {
    let watches = &mut state.shell.watches;
    if !args.has("expression") {
        let data = watches.iter().cloned().map(Data::String).collect();
        let text = if watches.is_empty() {
            "nothing is watched".to_owned()
        } else {
            watches.join("\n")
        };
        return Ok(Output::with_data(text, Data::Array(data)));
    }
    let expression = args.string("expression")?.trim().to_owned();
    if expression.len() > MAX_WATCH_LENGTH {
        return Err(failed(format!(
            "a watched expression is at most {MAX_WATCH_LENGTH} bytes"
        )));
    }
    if !watches.contains(&expression) {
        if watches.len() >= MAX_WATCHES {
            return Err(failed(format!("at most {MAX_WATCHES} are watched")));
        }
        watches.push(expression.clone());
    }
    Ok(Output::with_data(
        format!("watching {expression}"),
        Data::String(expression),
    ))
}

fn unwatch(state: &mut State, args: &Args) -> Result<Output, CommandError> {
    let watches = &mut state.shell.watches;
    if !args.has("expression") {
        let count = watches.len();
        watches.clear();
        return Ok(Output::with_data(
            format!("unwatched {count}"),
            Data::from(count),
        ));
    }
    let expression = args.string("expression")?.trim();
    let before = watches.len();
    watches.retain(|watched| watched != expression);
    if watches.len() == before {
        return Err(failed(format!("{expression} is not watched")));
    }
    Ok(Output::text(format!("unwatched {expression}")))
}

/// (the app clears its scrollback when the data says so)
fn clear(_: &mut State, _: &Args) -> Result<Output, CommandError> {
    Ok(Output::with_data("", json!({ "clear": true })))
}

fn quit(state: &mut State, _: &Args) -> Result<Output, CommandError> {
    state.shell.action = ShellAction::Quit;
    Ok(Output::text("quitting"))
}

/// The ticks a debug shape lasts unless told (a second).
const DEBUG_TICKS: i64 = 30;

/// The most ticks a debug shape lasts (ten minutes).
const MAX_DEBUG_TICKS: i64 = 18_000;

fn point(args: &Args, names: [&str; 3]) -> Result<[f32; 3], CommandError> {
    let mut point = [0.0; 3];
    for (at, name) in point.iter_mut().zip(names) {
        #[allow(clippy::cast_possible_truncation)]
        let value = args.float(name)? as f32;
        if !value.is_finite() {
            return Err(failed(format!("{name} is not a number")));
        }
        *at = value;
    }
    Ok(point)
}

fn debug_ticks(args: &Args) -> Result<u32, CommandError> {
    let ticks = args.int_or("ticks", DEBUG_TICKS)?;
    u32::try_from(ticks.clamp(1, MAX_DEBUG_TICKS)).map_err(|_| failed("too many ticks"))
}

fn drawn(state: &State, what: &str) -> Output {
    Output::with_data(
        format!("{what} drawn"),
        json!({ "lines": state.shell.draw.lines().len(), "labels": state.shell.draw.labels().len() }),
    )
}

fn debug_line(state: &mut State, args: &Args) -> Result<Output, CommandError> {
    let from = point(args, ["x0", "y0", "z0"])?;
    let to = point(args, ["x1", "y1", "z1"])?;
    let ticks = debug_ticks(args)?;
    state
        .shell
        .draw
        .line(Layer::General, from, to, Color::WHITE, ticks);
    Ok(drawn(state, "line"))
}

fn debug_box(state: &mut State, args: &Args) -> Result<Output, CommandError> {
    let from = point(args, ["x0", "y0", "z0"])?;
    let to = point(args, ["x1", "y1", "z1"])?;
    let min = [from[0].min(to[0]), from[1].min(to[1]), from[2].min(to[2])];
    let max = [from[0].max(to[0]), from[1].max(to[1]), from[2].max(to[2])];
    let ticks = debug_ticks(args)?;
    state
        .shell
        .draw
        .bounds(Layer::General, min, max, Color::WHITE, ticks);
    Ok(drawn(state, "box"))
}

fn debug_sphere(state: &mut State, args: &Args) -> Result<Output, CommandError> {
    let center = point(args, ["x", "y", "z"])?;
    #[allow(clippy::cast_possible_truncation)]
    let radius = args.float("radius")? as f32;
    if !(radius.is_finite() && radius > 0.0) {
        return Err(failed("the radius must be above 0"));
    }
    let ticks = debug_ticks(args)?;
    state
        .shell
        .draw
        .sphere(Layer::General, center, radius, Color::WHITE, ticks);
    Ok(drawn(state, "sphere"))
}

fn debug_text(state: &mut State, args: &Args) -> Result<Output, CommandError> {
    let at = point(args, ["x", "y", "z"])?;
    let text = args.string("text")?;
    state.shell.draw.text(
        Layer::General,
        at,
        text,
        Color::WHITE,
        u32::try_from(DEBUG_TICKS).unwrap_or(1),
    );
    Ok(drawn(state, "text"))
}

fn debug_clear(state: &mut State, _: &Args) -> Result<Output, CommandError> {
    state.shell.draw.clear();
    Ok(drawn(state, "nothing"))
}

/// HaloScript, run by the mission's scripts.
pub(crate) fn halo_script(state: &mut State, text: &str) -> Result<Output, CommandError> {
    let running = running(state)?;
    let Running {
        campaign,
        simulation,
        ..
    } = running;
    let campaign = campaign.as_mut().ok_or_else(|| {
        let word = text.split_whitespace().next().unwrap_or(text);
        CommandError::Unknown(word.to_owned())
    })?;
    let (value, value_type) = campaign
        .evaluate(simulation, text)
        .map_err(|error| failed(error.to_string()))?;
    let shown = match &value {
        opence_script::Value::Void => String::new(),
        opence_script::Value::Boolean(value) => value.to_string(),
        opence_script::Value::Real(value) => value.to_string(),
        opence_script::Value::Short(value) => value.to_string(),
        opence_script::Value::Long(value) | opence_script::Value::Index(value) => value.to_string(),
        opence_script::Value::String(value) => value.clone(),
        opence_script::Value::Tag(tag) => tag
            .as_ref()
            .map_or_else(|| "none".to_owned(), |tag| tag.path.clone()),
    };
    Ok(Output::with_data(
        shown.clone(),
        json!({ "value": shown, "type": format!("{value_type:?}") }),
    ))
}

fn script(state: &mut State, args: &Args) -> Result<Output, CommandError> {
    let argument = args.string_or("file", "")?;
    crate::script::command(state, argument.trim())
        .map(Output::text)
        .map_err(CommandError::Failed)
}

fn stat(_: &mut State, args: &Args) -> Result<Output, CommandError> {
    let profiler = opence_dev::profile::profiler();
    if !args.bool_or("switch", true)? {
        profiler.measure(false);
        return Ok(Output::text("frames are not measured"));
    }
    if !profiler.measuring() {
        profiler.measure(true);
        return Ok(Output::text(
            "frames are measured from the next: `stat frame` shows the last",
        ));
    }
    let lines: Vec<Data> = profiler
        .last_summary()
        .iter()
        .map(|line| {
            json!({
                "name": line.name,
                "depth": line.depth,
                "count": line.count,
                "total": line.total,
                "own": line.own,
            })
        })
        .collect();
    Ok(Output::with_data(profiler.report(), Data::Array(lines)))
}

fn trace(state: &mut State, args: &Args) -> Result<Output, CommandError> {
    let profiler = opence_dev::profile::profiler();
    let file = args.string("file")?;
    if file == "stop" {
        let Some(spans) = profiler.stop_trace() else {
            return Err(CommandError::Failed("no trace is taken".into()));
        };
        let path = state.shell.trace.take().unwrap_or_default();
        return write_trace(&path, &spans).map(Output::text);
    }
    let frames = args.int_or("frames", 300)?;
    let max = i64::from(opence_dev::profile::MAX_TRACE_FRAMES);
    if !(1..=max).contains(&frames) {
        return Err(CommandError::Failed(format!(
            "a trace takes 1 to {max} frames, not {frames}"
        )));
    }
    let path = PathBuf::from(file);
    if path.extension().is_none_or(|extension| extension != "json") {
        return Err(CommandError::Failed("a trace is written as .json".into()));
    }
    let frames = u32::try_from(frames).unwrap_or(1);
    profiler.start_trace(frames);
    let text = format!(
        "the next {frames} frames are traced into {}",
        path.display()
    );
    state.shell.trace = Some(path);
    Ok(Output::text(text))
}

/// Writes a trace's scopes to `path`.
pub(crate) fn write_trace(
    path: &Path,
    spans: &[opence_dev::profile::Span],
) -> Result<String, CommandError> {
    let trace = opence_dev::profile::chrome_trace(spans);
    std::fs::write(path, trace.to_string())
        .map_err(|error| CommandError::Failed(format!("{}: {error}", path.display())))?;
    Ok(format!(
        "{} scopes were traced into {}",
        spans.len(),
        path.display()
    ))
}
