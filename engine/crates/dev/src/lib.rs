//! The developer's tools: a registry of commands and variables that every
//! part of the engine adds to, run the same way from the shell overlay,
//! the control socket and the test driver's scripts.
//!
//! A line is one or more commands split by `;`. Each is, in turn:
//! - one of the registry's own (`set`, `reset`, `vars`, `help`, `bind`,
//!   `unbind`, `exec`);
//! - a command a system registered, with its arguments parsed to their
//!   kinds;
//! - a variable's name, alone to show it or with a value to set it;
//! - anything else, or anything in parentheses, is HaloScript, given to
//!   the fall-through (the running mission's scripts).

pub mod control;
pub mod draw;
pub mod logbook;
pub mod profile;
pub mod script;
mod words;

use std::collections::BTreeMap;
use std::fmt;

pub use serde_json::Value as Data;
/// (for [`profile_scope!`]'s spans)
pub use tracing;
pub use words::{parse_words, split_commands};

/// The files `exec` may run inside one another.
pub const MAX_EXEC_DEPTH: u8 = 8;

/// What went wrong with a command.
#[derive(Clone, Debug, PartialEq, thiserror::Error)]
pub enum CommandError {
    /// No command or variable of this name, and no fall-through.
    #[error("no command or variable `{0}`")]
    Unknown(String),
    /// An argument that is not of its parameter's kind.
    #[error("{command}: {param} must be {expected}, not `{given}`")]
    BadArgument {
        /// The command.
        command: String,
        /// The parameter.
        param: String,
        /// What it takes.
        expected: String,
        /// What was given.
        given: String,
    },
    /// A required argument not given.
    #[error("{command}: {param} is missing")]
    MissingArgument {
        /// The command.
        command: String,
        /// The parameter.
        param: String,
    },
    /// More arguments than the command takes.
    #[error("{command} takes at most {most} arguments, not {given}")]
    TooManyArguments {
        /// The command.
        command: String,
        /// The arguments given.
        given: usize,
        /// The most it takes.
        most: usize,
    },
    /// A value that is not of its variable's kind.
    #[error("{variable} must be {expected}, not `{given}`")]
    BadValue {
        /// The variable.
        variable: String,
        /// What it takes.
        expected: String,
        /// What was given.
        given: String,
    },
    /// A line that cannot be split into words.
    #[error("{0}")]
    Syntax(String),
    /// A command that ran and failed.
    #[error("{0}")]
    Failed(String),
}

/// What a command gives back: text for the shell, and the same as data
/// for the control socket.
#[derive(Clone, Debug, Default, PartialEq)]
pub struct Output {
    /// Its text (empty for none).
    pub text: String,
    /// Its data (null for none).
    pub data: Data,
}

impl Output {
    /// Nothing.
    pub fn none() -> Self {
        Self::default()
    }

    /// Text, and the same text as its data.
    pub fn text(text: impl Into<String>) -> Self {
        let text = text.into();
        Self {
            data: Data::String(text.clone()),
            text,
        }
    }

    /// Text and data.
    pub fn with_data(text: impl Into<String>, data: Data) -> Self {
        Self {
            text: text.into(),
            data,
        }
    }
}

/// The kind of a command's argument.
#[derive(Clone, Copy, Debug, PartialEq)]
pub enum ArgKind {
    /// `true`, `false`, `on`, `off`, `yes`, `no`, `1` or `0`.
    Bool,
    /// An integer.
    Int,
    /// A finite number.
    Float,
    /// A word.
    String,
    /// One of these words.
    Enum(&'static [&'static str]),
    /// The rest of the line's words, joined by spaces (last only).
    Rest,
}

impl ArgKind {
    /// A word as this kind (none if it is not one).
    pub fn parse(self, word: &str) -> Option<Arg> {
        match self {
            Self::Bool => parse_bool(word).map(Arg::Bool),
            Self::Int => word.parse().ok().map(Arg::Int),
            Self::Float => word
                .parse::<f64>()
                .ok()
                .filter(|v| v.is_finite())
                .map(Arg::Float),
            Self::String | Self::Rest => Some(Arg::String(word.to_owned())),
            Self::Enum(values) => values.contains(&word).then(|| Arg::String(word.to_owned())),
        }
    }

    /// What it takes, in words.
    pub fn expected(self) -> String {
        match self {
            Self::Bool => "true or false".into(),
            Self::Int => "an integer".into(),
            Self::Float => "a number".into(),
            Self::String | Self::Rest => "a word".into(),
            Self::Enum(values) => format!("one of {}", values.join(", ")),
        }
    }

    /// The words it may be, for completion (none for any word).
    fn choices(self) -> &'static [&'static str] {
        match self {
            Self::Bool => &["false", "true"],
            Self::Enum(values) => values,
            _ => &[],
        }
    }
}

fn parse_bool(word: &str) -> Option<bool> {
    match word.to_ascii_lowercase().as_str() {
        "true" | "on" | "yes" | "1" => Some(true),
        "false" | "off" | "no" | "0" => Some(false),
        _ => None,
    }
}

/// A command's argument, parsed.
#[derive(Clone, Debug, PartialEq)]
pub enum Arg {
    /// A boolean.
    Bool(bool),
    /// An integer.
    Int(i64),
    /// A number.
    Float(f64),
    /// A word.
    String(String),
}

/// A command's parameter.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Param {
    /// Its name.
    pub name: &'static str,
    /// Its kind.
    pub kind: ArgKind,
    /// Whether it may be left out (only after the required ones).
    pub optional: bool,
}

impl Param {
    /// A parameter that must be given.
    pub const fn required(name: &'static str, kind: ArgKind) -> Self {
        Self {
            name,
            kind,
            optional: false,
        }
    }

    /// A parameter that may be left out.
    pub const fn optional(name: &'static str, kind: ArgKind) -> Self {
        Self {
            name,
            kind,
            optional: true,
        }
    }
}

/// A command's arguments, by their parameters' names.
#[derive(Clone, Debug, PartialEq)]
pub struct Args {
    command: String,
    values: Vec<(&'static str, Option<Arg>)>,
}

impl Args {
    fn value(&self, name: &str) -> Option<&Arg> {
        self.values
            .iter()
            .find(|(n, _)| *n == name)
            .and_then(|(_, v)| v.as_ref())
    }

    fn missing(&self, name: &str) -> CommandError {
        CommandError::MissingArgument {
            command: self.command.clone(),
            param: name.to_owned(),
        }
    }

    /// Whether an argument was given.
    pub fn has(&self, name: &str) -> bool {
        self.value(name).is_some()
    }

    /// A word argument.
    pub fn string(&self, name: &str) -> Result<&str, CommandError> {
        match self.value(name) {
            Some(Arg::String(value)) => Ok(value),
            _ => Err(self.missing(name)),
        }
    }

    /// A word argument, or this if it was left out.
    pub fn string_or<'a>(&'a self, name: &str, default: &'a str) -> Result<&'a str, CommandError> {
        if self.has(name) {
            self.string(name)
        } else {
            Ok(default)
        }
    }

    /// An integer argument.
    pub fn int(&self, name: &str) -> Result<i64, CommandError> {
        match self.value(name) {
            Some(Arg::Int(value)) => Ok(*value),
            _ => Err(self.missing(name)),
        }
    }

    /// An integer argument, or this if it was left out.
    pub fn int_or(&self, name: &str, default: i64) -> Result<i64, CommandError> {
        if self.has(name) {
            self.int(name)
        } else {
            Ok(default)
        }
    }

    /// A number argument (an integer one too).
    pub fn float(&self, name: &str) -> Result<f64, CommandError> {
        match self.value(name) {
            Some(Arg::Float(value)) => Ok(*value),
            // (exact for every integer a person types)
            #[allow(clippy::cast_precision_loss)]
            Some(Arg::Int(value)) => Ok(*value as f64),
            _ => Err(self.missing(name)),
        }
    }

    /// A number argument, or this if it was left out.
    pub fn float_or(&self, name: &str, default: f64) -> Result<f64, CommandError> {
        if self.has(name) {
            self.float(name)
        } else {
            Ok(default)
        }
    }

    /// A boolean argument.
    pub fn bool(&self, name: &str) -> Result<bool, CommandError> {
        match self.value(name) {
            Some(Arg::Bool(value)) => Ok(*value),
            _ => Err(self.missing(name)),
        }
    }

    /// A boolean argument, or this if it was left out.
    pub fn bool_or(&self, name: &str, default: bool) -> Result<bool, CommandError> {
        if self.has(name) {
            self.bool(name)
        } else {
            Ok(default)
        }
    }
}

/// The kind of a variable.
#[derive(Clone, Copy, Debug, PartialEq)]
pub enum VarKind {
    /// A boolean.
    Bool,
    /// An integer.
    Int,
    /// A finite number.
    Float,
    /// Any text.
    String,
    /// One of these words (held as a string).
    Enum(&'static [&'static str]),
}

impl VarKind {
    /// Text as a value of this kind (none if it is not one).
    pub fn parse(self, text: &str) -> Option<VarValue> {
        match self {
            Self::Bool => parse_bool(text).map(VarValue::Bool),
            Self::Int => text.parse().ok().map(VarValue::Int),
            Self::Float => text
                .parse::<f64>()
                .ok()
                .filter(|v| v.is_finite())
                .map(VarValue::Float),
            Self::String => Some(VarValue::String(text.to_owned())),
            Self::Enum(values) => values
                .contains(&text)
                .then(|| VarValue::String(text.to_owned())),
        }
    }

    fn arg(self) -> ArgKind {
        match self {
            Self::Bool => ArgKind::Bool,
            Self::Int => ArgKind::Int,
            Self::Float => ArgKind::Float,
            Self::String => ArgKind::Rest,
            Self::Enum(values) => ArgKind::Enum(values),
        }
    }
}

/// A variable's value.
#[derive(Clone, Debug, PartialEq)]
pub enum VarValue {
    /// A boolean.
    Bool(bool),
    /// An integer.
    Int(i64),
    /// A number.
    Float(f64),
    /// Text, or one of an enum's words.
    String(String),
}

impl VarValue {
    /// It as data.
    pub fn data(&self) -> Data {
        match self {
            Self::Bool(value) => Data::Bool(*value),
            Self::Int(value) => Data::from(*value),
            Self::Float(value) => Data::from(*value),
            Self::String(value) => Data::String(value.clone()),
        }
    }
}

impl fmt::Display for VarValue {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Bool(value) => write!(f, "{value}"),
            Self::Int(value) => write!(f, "{value}"),
            Self::Float(value) => write!(f, "{value}"),
            Self::String(value) => write!(f, "{value}"),
        }
    }
}

/// A command a system registered: it takes the engine and its arguments.
pub type Handler<C> = fn(&mut C, &Args) -> Result<Output, CommandError>;

/// What runs a line that is not a command or a variable (HaloScript).
pub type Fallback<C> = fn(&mut C, &str) -> Result<Output, CommandError>;

/// What hears a variable's change: its name and new value.
pub type Listener<C> = fn(&mut C, &str, &VarValue);

struct Command<C> {
    help: &'static str,
    params: Vec<Param>,
    handler: Handler<C>,
}

struct Variable {
    help: &'static str,
    kind: VarKind,
    default: VarValue,
    value: VarValue,
    saved: bool,
}

// (the registry's own commands, their usage and help)
const BUILT_INS: [(&str, &str, &str); 7] = [
    (
        "bind",
        "<key> <command>",
        "Runs a command when a key is pressed.",
    ),
    (
        "exec",
        "<file>",
        "Runs a file's commands, a line at a time.",
    ),
    (
        "help",
        "[name]",
        "Shows the commands, or a command's or variable's help.",
    ),
    (
        "reset",
        "<variable>",
        "Sets a variable back to its default.",
    ),
    ("set", "<variable> <value>", "Sets a variable."),
    ("unbind", "<key>", "Takes a key's command away."),
    (
        "vars",
        "[filter]",
        "Shows the variables whose names have the filter in them.",
    ),
];

/// The commands and variables.
pub struct Registry<C> {
    commands: BTreeMap<String, Command<C>>,
    variables: BTreeMap<String, Variable>,
    listeners: Vec<Listener<C>>,
    fallback: Option<Fallback<C>>,
    bindings: BTreeMap<String, String>,
    depth: u8,
}

impl<C> Default for Registry<C> {
    fn default() -> Self {
        Self {
            commands: BTreeMap::new(),
            variables: BTreeMap::new(),
            listeners: Vec::new(),
            fallback: None,
            bindings: BTreeMap::new(),
            depth: 0,
        }
    }
}

impl<C> Registry<C> {
    /// Only the registry's own commands.
    pub fn new() -> Self {
        Self::default()
    }

    /// Adds a command (the registry's own commands are taken first, so one
    /// of their names is never this).
    pub fn command(
        &mut self,
        name: &str,
        help: &'static str,
        params: &[Param],
        handler: Handler<C>,
    ) {
        debug_assert!(!BUILT_INS.iter().any(|(n, _, _)| *n == name));
        self.commands.insert(
            name.to_owned(),
            Command {
                help,
                params: params.to_vec(),
                handler,
            },
        );
    }

    /// Adds a variable at its default; a saved one is kept in the
    /// settings by [`Registry::saved`].
    pub fn variable(
        &mut self,
        name: &str,
        help: &'static str,
        kind: VarKind,
        default: VarValue,
        saved: bool,
    ) {
        self.variables.insert(
            name.to_owned(),
            Variable {
                help,
                kind,
                value: default.clone(),
                default,
                saved,
            },
        );
    }

    /// Adds a listener to every variable's changes.
    pub fn listen(&mut self, listener: Listener<C>) {
        self.listeners.push(listener);
    }

    /// Sets what runs a line that is not a command or a variable.
    pub fn fallback(&mut self, fallback: Fallback<C>) {
        self.fallback = Some(fallback);
    }

    /// Whether a command of this name is registered (the registry's own
    /// among them).
    pub fn has_command(&self, name: &str) -> bool {
        self.commands.contains_key(name) || BUILT_INS.iter().any(|(n, _, _)| *n == name)
    }

    /// The commands' names, in order.
    pub fn command_names(&self) -> Vec<&str> {
        let mut names: Vec<&str> = BUILT_INS.iter().map(|(n, _, _)| *n).collect();
        names.extend(self.commands.keys().map(String::as_str));
        names.sort_unstable();
        names
    }

    /// The variables' names, in order.
    pub fn variable_names(&self) -> impl Iterator<Item = &str> {
        self.variables.keys().map(String::as_str)
    }

    /// A variable's value.
    pub fn get(&self, name: &str) -> Option<&VarValue> {
        self.variables.get(name).map(|v| &v.value)
    }

    /// A boolean variable's value (false if there is none).
    pub fn get_bool(&self, name: &str) -> bool {
        matches!(self.get(name), Some(VarValue::Bool(true)))
    }

    /// A number variable's value (an integer one too; this if there is
    /// none).
    pub fn get_float(&self, name: &str, otherwise: f64) -> f64 {
        match self.get(name) {
            Some(VarValue::Float(value)) => *value,
            #[allow(clippy::cast_precision_loss)]
            Some(VarValue::Int(value)) => *value as f64,
            _ => otherwise,
        }
    }

    /// Sets a variable, telling the listeners if it changed.
    pub fn set(
        &mut self,
        context: &mut C,
        name: &str,
        value: VarValue,
    ) -> Result<(), CommandError> {
        let variable = self
            .variables
            .get_mut(name)
            .ok_or_else(|| CommandError::Unknown(name.to_owned()))?;
        let text = value.to_string();
        if variable.kind.parse(&text).as_ref() != Some(&value) {
            return Err(CommandError::BadValue {
                variable: name.to_owned(),
                expected: variable.kind.arg().expected(),
                given: text,
            });
        }
        if variable.value == value {
            return Ok(());
        }
        variable.value = value.clone();
        for listener in self.listeners.clone() {
            listener(context, name, &value);
        }
        Ok(())
    }

    /// Sets a variable from text, read as its kind.
    pub fn set_text(
        &mut self,
        context: &mut C,
        name: &str,
        text: &str,
    ) -> Result<(), CommandError> {
        let variable = self
            .variables
            .get(name)
            .ok_or_else(|| CommandError::Unknown(name.to_owned()))?;
        let value = variable
            .kind
            .parse(text)
            .ok_or_else(|| CommandError::BadValue {
                variable: name.to_owned(),
                expected: variable.kind.arg().expected(),
                given: text.to_owned(),
            })?;
        self.set(context, name, value)
    }

    /// A variable's help and its default.
    pub fn describe(&self, name: &str) -> Option<(&'static str, &VarValue)> {
        self.variables.get(name).map(|v| (v.help, &v.default))
    }

    /// The saved variables that are not at their default, as text.
    pub fn saved(&self) -> BTreeMap<String, String> {
        self.variables
            .iter()
            .filter(|(_, v)| v.saved && v.value != v.default)
            .map(|(name, v)| (name.clone(), v.value.to_string()))
            .collect()
    }

    /// Sets the saved variables from the settings (the listeners are not
    /// told); gives back the names that are not saved variables, or whose
    /// values are not of their kind.
    pub fn load_saved(&mut self, saved: &BTreeMap<String, String>) -> Vec<String> {
        let mut refused = Vec::new();
        for (name, text) in saved {
            match self.variables.get_mut(name) {
                Some(variable) if variable.saved => match variable.kind.parse(text) {
                    Some(value) => variable.value = value,
                    None => refused.push(name.clone()),
                },
                _ => refused.push(name.clone()),
            }
        }
        refused
    }

    /// A key's command.
    pub fn binding(&self, key: &str) -> Option<&str> {
        self.bindings.get(key).map(String::as_str)
    }

    /// Every key's command.
    pub fn bindings(&self) -> &BTreeMap<String, String> {
        &self.bindings
    }

    /// Runs a key's command (nothing for a key with none).
    pub fn press(&mut self, context: &mut C, key: &str) -> Result<Output, CommandError> {
        match self.bindings.get(key).cloned() {
            Some(line) => self.run(context, &line),
            None => Ok(Output::none()),
        }
    }

    /// Runs a line's commands in turn, stopping at the first that fails;
    /// their outputs are joined.
    pub fn run(&mut self, context: &mut C, line: &str) -> Result<Output, CommandError> {
        let mut outputs = Vec::new();
        for command in split_commands(line) {
            outputs.push(self.run_one(context, command)?);
        }
        Ok(match outputs.len() {
            0 => Output::none(),
            1 => outputs.remove(0),
            _ => Output {
                text: outputs
                    .iter()
                    .map(|o| o.text.as_str())
                    .filter(|t| !t.is_empty())
                    .collect::<Vec<_>>()
                    .join("\n"),
                data: Data::Array(outputs.into_iter().map(|o| o.data).collect()),
            },
        })
    }

    fn run_one(&mut self, context: &mut C, command: &str) -> Result<Output, CommandError> {
        if command.starts_with('(') {
            return self.fall_through(context, command);
        }
        let words = parse_words(command)?;
        let Some(name) = words.first() else {
            return Ok(Output::none());
        };
        let rest = &words[1..];
        match name.as_str() {
            "set" => {
                let variable = rest.first().ok_or_else(|| missing("set", "variable"))?;
                if rest.len() < 2 {
                    return Err(missing("set", "value"));
                }
                self.set_text(context, variable, &rest[1..].join(" "))?;
                Ok(Output::none())
            }
            "reset" => {
                let name = rest.first().ok_or_else(|| missing("reset", "variable"))?;
                let default = self
                    .variables
                    .get(name)
                    .map(|v| v.default.clone())
                    .ok_or_else(|| CommandError::Unknown(name.clone()))?;
                self.set(context, name, default)?;
                Ok(Output::none())
            }
            "vars" => Ok(self.list(rest.first().map_or("", String::as_str))),
            "help" => self.help(rest.first().map(String::as_str)),
            "bind" => {
                let key = rest.first().ok_or_else(|| missing("bind", "key"))?;
                if rest.len() < 2 {
                    return Err(missing("bind", "command"));
                }
                self.bindings.insert(key.clone(), rest[1..].join(" "));
                Ok(Output::none())
            }
            "unbind" => {
                let key = rest.first().ok_or_else(|| missing("unbind", "key"))?;
                self.bindings.remove(key);
                Ok(Output::none())
            }
            "exec" => {
                let path = rest.first().ok_or_else(|| missing("exec", "file"))?;
                self.exec(context, path)
            }
            _ => {
                if let Some(command_entry) = self.commands.get(name) {
                    let args = parse_args(name, &command_entry.params, rest)?;
                    let handler = command_entry.handler;
                    return handler(context, &args);
                }
                if self.variables.contains_key(name) {
                    if rest.is_empty() {
                        return Ok(self.show(name));
                    }
                    self.set_text(context, name, &rest.join(" "))?;
                    return Ok(Output::none());
                }
                self.fall_through(context, command)
            }
        }
    }

    fn fall_through(&mut self, context: &mut C, command: &str) -> Result<Output, CommandError> {
        match self.fallback {
            Some(fallback) => fallback(context, command),
            None => Err(CommandError::Unknown(
                command
                    .split_whitespace()
                    .next()
                    .unwrap_or(command)
                    .to_owned(),
            )),
        }
    }

    fn exec(&mut self, context: &mut C, path: &str) -> Result<Output, CommandError> {
        if self.depth >= MAX_EXEC_DEPTH {
            return Err(CommandError::Failed(format!(
                "exec: more than {MAX_EXEC_DEPTH} files inside one another"
            )));
        }
        let text = std::fs::read_to_string(path)
            .map_err(|error| CommandError::Failed(format!("exec: {path}: {error}")))?;
        self.depth += 1;
        let mut lines = Vec::new();
        let mut result = Ok(());
        for (number, line) in text.lines().enumerate() {
            let line = line.trim();
            if line.is_empty() || line.starts_with('#') {
                continue;
            }
            match self.run(context, line) {
                Ok(output) if !output.text.is_empty() => lines.push(output.text),
                Ok(_) => {}
                Err(error) => {
                    result = Err(CommandError::Failed(format!(
                        "{path}:{}: {error}",
                        number + 1
                    )));
                    break;
                }
            }
        }
        self.depth -= 1;
        result.map(|()| Output::text(lines.join("\n")))
    }

    fn show(&self, name: &str) -> Output {
        match self.variables.get(name) {
            Some(v) => Output::with_data(
                format!("{name} = {} (default {})", v.value, v.default),
                v.value.data(),
            ),
            None => Output::none(),
        }
    }

    fn list(&self, filter: &str) -> Output {
        let mut text = Vec::new();
        let mut data = serde_json::Map::new();
        for (name, v) in self.variables.iter().filter(|(n, _)| n.contains(filter)) {
            text.push(format!("{name} = {}", v.value));
            data.insert(name.clone(), v.value.data());
        }
        Output::with_data(text.join("\n"), Data::Object(data))
    }

    fn help(&self, name: Option<&str>) -> Result<Output, CommandError> {
        let Some(name) = name else {
            let lines: Vec<String> = self
                .command_names()
                .into_iter()
                .map(|name| format!("{name:<12} {}", self.help_of(name).unwrap_or_default().1))
                .collect();
            return Ok(Output::text(lines.join("\n")));
        };
        if let Some((usage, help)) = self.help_of(name) {
            return Ok(Output::text(format!("{usage}\n  {help}")));
        }
        if let Some(v) = self.variables.get(name) {
            let kind = match v.kind {
                VarKind::Bool => "true|false".to_owned(),
                VarKind::Int => "integer".to_owned(),
                VarKind::Float => "number".to_owned(),
                VarKind::String => "text".to_owned(),
                VarKind::Enum(values) => values.join("|"),
            };
            return Ok(Output::text(format!(
                "{name} <{kind}> = {} (default {})\n  {}",
                v.value, v.default, v.help
            )));
        }
        Err(CommandError::Unknown(name.to_owned()))
    }

    fn help_of(&self, name: &str) -> Option<(String, &'static str)> {
        if let Some((_, usage, help)) = BUILT_INS.iter().find(|(n, _, _)| *n == name) {
            return Some((format!("{name} {usage}"), help));
        }
        let command = self.commands.get(name)?;
        let mut usage = name.to_owned();
        for param in &command.params {
            let shown = if param.kind == ArgKind::Rest {
                format!("{}...", param.name)
            } else {
                param.name.to_owned()
            };
            if param.optional {
                usage.push_str(&format!(" [{shown}]"));
            } else {
                usage.push_str(&format!(" <{shown}>"));
            }
        }
        Some((usage, command.help))
    }

    /// The lines a line may be completed to, in order: the last word
    /// completed as a command, a variable, or one of an argument's words.
    pub fn complete(&self, line: &str) -> Vec<String> {
        let start = line
            .rfind(|c: char| c.is_whitespace() || c == ';')
            .map_or(0, |at| at + 1);
        let (head, word) = line.split_at(start);
        let command = head.rsplit(';').next().unwrap_or(head);
        let before: Vec<&str> = command.split_whitespace().collect();
        let candidates: Vec<String> = match before.as_slice() {
            [] => self
                .command_names()
                .into_iter()
                .map(str::to_owned)
                .chain(self.variables.keys().cloned())
                .collect(),
            ["set" | "reset"] => self.variables.keys().cloned().collect(),
            ["help"] => self
                .command_names()
                .into_iter()
                .map(str::to_owned)
                .chain(self.variables.keys().cloned())
                .collect(),
            ["set", variable] => self.variable_choices(variable),
            [variable] if self.variables.contains_key(*variable) => self.variable_choices(variable),
            [name, args @ ..] => self
                .commands
                .get(*name)
                .and_then(|c| c.params.get(args.len()))
                .map(|p| p.kind.choices().iter().map(|c| (*c).to_owned()).collect())
                .unwrap_or_default(),
        };
        let mut lines: Vec<String> = candidates
            .into_iter()
            .filter(|c| c.starts_with(word))
            .map(|c| format!("{head}{c}"))
            .collect();
        lines.sort();
        lines.dedup();
        lines
    }

    fn variable_choices(&self, name: &str) -> Vec<String> {
        self.variables
            .get(name)
            .map(|v| {
                v.kind
                    .arg()
                    .choices()
                    .iter()
                    .map(|c| (*c).to_owned())
                    .collect()
            })
            .unwrap_or_default()
    }
}

fn missing(command: &str, param: &str) -> CommandError {
    CommandError::MissingArgument {
        command: command.to_owned(),
        param: param.to_owned(),
    }
}

fn parse_args(command: &str, params: &[Param], words: &[String]) -> Result<Args, CommandError> {
    let mut values = Vec::with_capacity(params.len());
    let mut at = 0;
    for param in params {
        let value = if param.kind == ArgKind::Rest {
            let rest = words.get(at..).unwrap_or_default();
            at = words.len();
            (!rest.is_empty()).then(|| Arg::String(rest.join(" ")))
        } else if let Some(word) = words.get(at) {
            at += 1;
            Some(
                param
                    .kind
                    .parse(word)
                    .ok_or_else(|| CommandError::BadArgument {
                        command: command.to_owned(),
                        param: param.name.to_owned(),
                        expected: param.kind.expected(),
                        given: word.clone(),
                    })?,
            )
        } else {
            None
        };
        if value.is_none() && !param.optional {
            return Err(missing(command, param.name));
        }
        values.push((param.name, value));
    }
    if at < words.len() {
        return Err(CommandError::TooManyArguments {
            command: command.to_owned(),
            given: words.len(),
            most: params.len(),
        });
    }
    Ok(Args {
        command: command.to_owned(),
        values,
    })
}
