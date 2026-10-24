//! The developer state the dev panel, the shell, the control socket and the
//! launcher's Developer tab share: the shell's variables, with the lighting
//! controls (`tune.*`) among them. The controls are tuned per map and kept
//! in a file of the map's in the tuning folder (`<map>.toml`), read again
//! as each map loads; the rest are the shell's own. Each change of a
//! variable counts once, so the app reads the state again only when it
//! changed: untouched, it costs nothing and the frame is the stock one.

use std::collections::BTreeMap;
use std::path::{Path, PathBuf};

use opence_dev::{ArgKind, Args, CommandError, Output, Param, Registry, VarKind, VarValue};
use serde::{Deserialize, Serialize};
use serde_json::json;

use crate::State;

/// The prefix of the lighting controls' variables.
pub const TUNE_PREFIX: &str = "tune.";

/// The lighting controls: name, help, default (the stock frame), and the
/// range the panel's slider and a tuning file are held to.
pub const TUNING: [(&str, &str, f64, f64, f64); 11] = [
    (
        "tune.exposure",
        "Brightens (or darkens) the frame by stops before the tone map.",
        0.0,
        -4.0,
        4.0,
    ),
    ("tune.sun", "Scales the sun's light.", 1.0, 0.0, 4.0),
    ("tune.sun_red", "Scales the red of the sun's colour.", 1.0, 0.0, 2.0),
    (
        "tune.sun_green",
        "Scales the green of the sun's colour.",
        1.0,
        0.0,
        2.0,
    ),
    ("tune.sun_blue", "Scales the blue of the sun's colour.", 1.0, 0.0, 2.0),
    (
        "tune.bounce",
        "Scales the baked light that bounced off the level.",
        1.0,
        0.0,
        4.0,
    ),
    (
        "tune.probes",
        "Scales the light the probes give the objects.",
        1.0,
        0.0,
        4.0,
    ),
    (
        "tune.ao",
        "Scales how much the ambient occlusion darkens.",
        1.0,
        0.0,
        2.0,
    ),
    (
        "tune.emissive",
        "Scales the light the shaders give off themselves.",
        1.0,
        0.0,
        4.0,
    ),
    ("tune.bloom", "Scales the bloom's strength.", 1.0, 0.0, 4.0),
    (
        "tune.tone_curve",
        "The tone curve's contrast (1 the stock curve).",
        1.0,
        0.5,
        2.0,
    ),
];

/// The variable that shows the dev panel.
pub const PANEL: &str = "dev.panel";

/// A lighting control as the panel shows it.
#[derive(Clone, Debug, PartialEq, uniffi::Record)]
pub struct TuneControl {
    /// Its variable's name.
    pub name: String,
    /// What it does.
    pub help: String,
    /// Its value now.
    pub value: f64,
    /// Its value in the stock frame.
    pub default_value: f64,
    /// The least it may be.
    pub minimum: f64,
    /// The most it may be.
    pub maximum: f64,
}

/// What `tune` asked to be done with the registry once the line ran.
#[derive(Clone, Copy, Debug, PartialEq)]
pub(crate) enum TuneRequest {
    Save,
    Load,
    Reset,
}

/// Where the tuning of the map playing is kept.
#[derive(Clone, Debug, Default)]
pub(crate) struct Tuning {
    /// The tuning folder and the map's name.
    pub place: Option<(PathBuf, String)>,
    /// What `tune` asked for.
    pub request: Option<TuneRequest>,
    /// The changes of the shell's variables so far.
    pub generation: u64,
}

/// A tuning file: the controls not at their defaults.
#[derive(Debug, Default, Deserialize, Serialize)]
struct TuningFile {
    #[serde(default)]
    tune: BTreeMap<String, f64>,
}

/// Adds the controls, the panel's switch, the `tune` command and the
/// counting of changes to the shell.
pub(crate) fn register(registry: &mut Registry<State>) {
    for (name, help, default, _, _) in TUNING {
        registry.variable(name, help, VarKind::Float, VarValue::Float(default), false);
    }
    registry.variable(
        PANEL,
        "Shows the dev panel.",
        VarKind::Bool,
        VarValue::Bool(false),
        false,
    );
    registry.command(
        "tune",
        "Saves the lighting controls as the map's tuning, loads it again, or resets them; alone, shows them.",
        &[Param::optional(
            "what",
            ArgKind::Enum(&["save", "load", "reset"]),
        )],
        tune,
    );
    registry.listen(changed);
}

fn changed(state: &mut State, _: &str, _: &VarValue) {
    state.shell.tuning.generation = state.shell.tuning.generation.wrapping_add(1);
}

fn tune(state: &mut State, args: &Args) -> Result<Output, CommandError> {
    let request = match args.string_or("what", "")? {
        "save" => TuneRequest::Save,
        "load" => TuneRequest::Load,
        "reset" => TuneRequest::Reset,
        _ => return Ok(Output::none()),
    };
    if request != TuneRequest::Reset && state.shell.tuning.place.is_none() {
        return Err(CommandError::Failed("no map's tuning is open".into()));
    }
    state.shell.tuning.request = Some(request);
    Ok(Output::none())
}

/// Does what `tune` asked, now that the registry is free; the line's
/// output, or its error. `tune` alone lists the controls.
pub(crate) fn finish(
    registry: &mut Registry<State>,
    state: &mut State,
    line: &str,
    output: Result<Output, CommandError>,
) -> Result<Output, CommandError> {
    let request = state.shell.tuning.request.take();
    let output = output?;
    match request {
        None if line.trim() == "tune" => {
            let controls = controls(registry);
            let text = controls
                .iter()
                .map(|control| format!("{} = {}", control.name, control.value))
                .collect::<Vec<_>>()
                .join("\n");
            let data: serde_json::Map<String, serde_json::Value> = controls
                .iter()
                .map(|control| (control.name.clone(), json!(control.value)))
                .collect();
            Ok(Output::with_data(text, data.into()))
        }
        None => Ok(output),
        Some(TuneRequest::Reset) => {
            reset(registry, state);
            Ok(Output::text("the lighting controls are at their defaults"))
        }
        Some(TuneRequest::Save) => {
            let path = save(registry, state).map_err(CommandError::Failed)?;
            Ok(Output::text(format!("saved {}", path.display())))
        }
        Some(TuneRequest::Load) => {
            let refused = load(registry, state).map_err(CommandError::Failed)?;
            Ok(Output::text(if refused.is_empty() {
                "loaded the map's tuning".to_owned()
            } else {
                format!("loaded the map's tuning; refused {}", refused.join(", "))
            }))
        }
    }
}

/// The controls as they are now.
pub(crate) fn controls(registry: &Registry<State>) -> Vec<TuneControl> {
    TUNING
        .iter()
        .map(|&(name, help, default, minimum, maximum)| TuneControl {
            name: name.to_owned(),
            help: help.to_owned(),
            value: registry.get_float(name, default),
            default_value: default,
            minimum,
            maximum,
        })
        .collect()
}

/// Sets every control back to its default.
pub(crate) fn reset(registry: &mut Registry<State>, state: &mut State) {
    for (name, _, default, _, _) in TUNING {
        let _ = registry.set(state, name, VarValue::Float(default));
    }
}

/// A map's tuning file.
fn path(folder: &Path, map: &str) -> PathBuf {
    // (a map's name, not a path: only its last part is kept)
    let name = Path::new(map)
        .file_stem()
        .map_or_else(|| "map".into(), |stem| stem.to_string_lossy().into_owned());
    folder.join(format!("{name}.toml"))
}

/// Writes the controls not at their defaults to the map's file (removed
/// when every one is); its path.
pub(crate) fn save(registry: &Registry<State>, state: &State) -> Result<PathBuf, String> {
    let (folder, map) = state
        .shell
        .tuning
        .place
        .as_ref()
        .ok_or("no map's tuning is open")?;
    let file = TuningFile {
        tune: controls(registry)
            .into_iter()
            .filter(|control| control.value != control.default_value)
            .map(|control| (control.name[TUNE_PREFIX.len()..].to_owned(), control.value))
            .collect(),
    };
    let path = path(folder, map);
    if file.tune.is_empty() {
        match std::fs::remove_file(&path) {
            Ok(()) => {}
            Err(error) if error.kind() == std::io::ErrorKind::NotFound => {}
            Err(error) => return Err(format!("{}: {error}", path.display())),
        }
        return Ok(path);
    }
    let text = toml::to_string(&file).map_err(|error| error.to_string())?;
    std::fs::create_dir_all(folder).map_err(|error| format!("{}: {error}", folder.display()))?;
    std::fs::write(&path, text).map_err(|error| format!("{}: {error}", path.display()))?;
    Ok(path)
}

/// Sets the controls from the map's file, the rest to their defaults; the
/// names refused (not a control, or out of its range).
pub(crate) fn load(registry: &mut Registry<State>, state: &mut State) -> Result<Vec<String>, String> {
    reset(registry, state);
    let (folder, map) = state
        .shell
        .tuning
        .place
        .clone()
        .ok_or("no map's tuning is open")?;
    let path = path(&folder, &map);
    let text = match std::fs::read_to_string(&path) {
        Ok(text) => text,
        Err(error) if error.kind() == std::io::ErrorKind::NotFound => return Ok(Vec::new()),
        Err(error) => return Err(format!("{}: {error}", path.display())),
    };
    let file: TuningFile =
        toml::from_str(&text).map_err(|error| format!("{}: {error}", path.display()))?;
    let mut refused = Vec::new();
    for (key, value) in file.tune {
        let name = format!("{TUNE_PREFIX}{key}");
        match TUNING.iter().find(|control| control.0 == name) {
            Some(&(_, _, _, minimum, maximum))
                if value.is_finite() && (minimum..=maximum).contains(&value) =>
            {
                let _ = registry.set(state, &name, VarValue::Float(value));
            }
            _ => refused.push(name),
        }
    }
    Ok(refused)
}
