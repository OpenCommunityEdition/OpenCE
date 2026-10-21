//! Text typed at the console, made into nodes the runtime evaluates, as
//! Halo's console compiles a line (`source/hs/hs_compile.c` as the
//! reference): each argument is parsed as the type its function takes,
//! names are looked up in the world, and a bare line is a call.

use std::collections::BTreeMap;

use opence_content::{Call, Constant, GlobalRef, Named, NodeKind, ScriptNode, Scripts, ValueType};

use crate::{External, Function, World};

/// The characters a console line may have.
pub const MAX_TEXT: usize = 4096;

/// The depth of parentheses a console line may have.
pub const MAX_DEPTH: usize = 64;

/// What is wrong with a console line.
#[derive(Clone, Debug, PartialEq, thiserror::Error)]
pub enum ConsoleError {
    /// A line that is not an expression.
    #[error("{0}")]
    Syntax(String),
    /// A function that is neither the runtime's, the engine's, nor one of
    /// the scenario's static scripts.
    #[error("no function `{0}`")]
    UnknownFunction(String),
    /// A function called with the wrong number of arguments.
    #[error("{function} takes {expected} arguments, not {given}")]
    Arguments {
        /// The function.
        function: String,
        /// The number it takes (the nearest, for a form taking a range).
        expected: usize,
        /// The number given.
        given: usize,
    },
    /// A name that is not of what its type names.
    #[error("no {value_type:?} named `{name}`")]
    UnknownName {
        /// The type.
        value_type: ValueType,
        /// The name.
        name: String,
    },
    /// A word that is not a value of its type.
    #[error("`{given}` is not a {value_type:?}")]
    NotAValue {
        /// The type.
        value_type: ValueType,
        /// The word.
        given: String,
    },
    /// A global that is neither the scenario's nor the engine's.
    #[error("no global `{0}`")]
    UnknownGlobal(String),
}

/// A line parsed: a word, or a list in parentheses.
#[derive(Clone, Debug, PartialEq)]
enum Expr {
    Word { text: String, quoted: bool },
    List(Vec<Expr>),
}

fn syntax(text: &str) -> ConsoleError {
    ConsoleError::Syntax(text.to_owned())
}

/// A line's expression: one in parentheses, or the words of a bare line
/// as a call (a lone word that names no function is itself).
fn parse(text: &str, is_function: impl Fn(&str) -> bool) -> Result<Expr, ConsoleError> {
    if text.len() > MAX_TEXT {
        return Err(syntax("the line is too long"));
    }
    let mut chars = text.chars().peekable();
    let mut items = Vec::new();
    loop {
        while chars.next_if(|c| c.is_whitespace()).is_some() {}
        if chars.peek().is_none() {
            break;
        }
        items.push(parse_one(&mut chars, 0)?);
    }
    let bare = !text.trim_start().starts_with('(');
    match items.len() {
        0 => Err(syntax("nothing to evaluate")),
        1 if !bare => Ok(items.remove(0)),
        _ if !bare => Err(syntax("more than one expression")),
        1 => match &items[0] {
            Expr::Word {
                text,
                quoted: false,
            } if is_function(text) => Ok(Expr::List(items)),
            _ => Ok(items.remove(0)),
        },
        _ => Ok(Expr::List(items)),
    }
}

fn parse_one(
    chars: &mut std::iter::Peekable<std::str::Chars<'_>>,
    depth: usize,
) -> Result<Expr, ConsoleError> {
    match chars.next() {
        Some('(') => {
            if depth >= MAX_DEPTH {
                return Err(syntax("the parentheses are too deep"));
            }
            let mut items = Vec::new();
            loop {
                while chars.next_if(|c| c.is_whitespace()).is_some() {}
                match chars.peek() {
                    None => return Err(syntax("a parenthesis is not closed")),
                    Some(')') => {
                        chars.next();
                        break;
                    }
                    Some(_) => items.push(parse_one(chars, depth + 1)?),
                }
            }
            if items.is_empty() {
                return Err(syntax("`()` calls nothing"));
            }
            Ok(Expr::List(items))
        }
        Some(')') => Err(syntax("a parenthesis closes nothing")),
        Some('"') => {
            let mut text = String::new();
            loop {
                match chars.next() {
                    None => return Err(syntax("a quote is not closed")),
                    Some('"') => break,
                    Some(c) => text.push(c),
                }
            }
            Ok(Expr::Word { text, quoted: true })
        }
        Some(c) => {
            let mut text = String::from(c);
            while let Some(c) =
                chars.next_if(|c| !c.is_whitespace() && !matches!(c, '(' | ')' | '"'))
            {
                text.push(c);
            }
            Ok(Expr::Word {
                text,
                quoted: false,
            })
        }
        None => Err(syntax("nothing to evaluate")),
    }
}

/// What a line's names are looked up in.
pub(crate) struct Names<'a, W> {
    pub scripts: &'a Scripts,
    pub functions: &'a [Function<W>],
    pub function_names: &'a BTreeMap<String, usize>,
    pub externals: &'a [External],
    pub external_names: &'a BTreeMap<String, usize>,
}

/// A line made into nodes, to go after the scenario's: the nodes, the
/// root's index among them last.
pub(crate) fn compile<W: World>(
    text: &str,
    names: &Names<'_, W>,
    world: &mut W,
) -> Result<Vec<ScriptNode>, ConsoleError> {
    let expr = parse(text, |word| {
        names.function_names.contains_key(word)
            || crate::SPECIAL_FORMS.contains(&word)
            || names.scripts.scripts.iter().any(|s| s.name == word)
    })?;
    let mut compiler = Compiler {
        names,
        world,
        base: names.scripts.nodes.len(),
        nodes: Vec::new(),
    };
    compiler.expression(&expr, ValueType::Passthrough)?;
    Ok(compiler.nodes)
}

struct Compiler<'a, 'w, W> {
    names: &'a Names<'a, W>,
    world: &'w mut W,
    base: usize,
    nodes: Vec<ScriptNode>,
}

/// The type a node is taken as: what its parent takes, or its own.
fn taken(expected: ValueType, own: ValueType) -> ValueType {
    if expected == ValueType::Passthrough {
        own
    } else {
        expected
    }
}

/// The name type of an object type (a unit's name for a unit).
fn name_of(object: ValueType) -> ValueType {
    let offset = ValueType::ObjectName as usize - ValueType::Object as usize;
    ValueType::ALL
        .get(object as usize + offset)
        .copied()
        .unwrap_or(ValueType::ObjectName)
}

impl<W: World> Compiler<'_, '_, W> {
    fn push(
        &mut self,
        value_type: ValueType,
        kind: NodeKind,
    ) -> Result<(u32, ValueType), ConsoleError> {
        let index = u32::try_from(self.base + self.nodes.len())
            .map_err(|_| syntax("the scripts have too many nodes"))?;
        self.nodes.push(ScriptNode { value_type, kind });
        Ok((index, value_type))
    }

    fn call(
        &mut self,
        function: &str,
        expected: ValueType,
        own: ValueType,
        arguments: Vec<u32>,
    ) -> Result<(u32, ValueType), ConsoleError> {
        let call = Call {
            function: function.to_owned(),
            arguments,
        };
        self.push(taken(expected, own), NodeKind::Call(call))
    }

    /// An expression, as `expected` takes it (passthrough: as its own
    /// type): its node and the type it is taken as.
    fn expression(
        &mut self,
        expr: &Expr,
        expected: ValueType,
    ) -> Result<(u32, ValueType), ConsoleError> {
        match expr {
            Expr::Word { text, quoted } => self.word(text, *quoted, expected),
            Expr::List(items) => {
                let Some((
                    Expr::Word {
                        text: name,
                        quoted: false,
                    },
                    arguments,
                )) = items.split_first()
                else {
                    return Err(syntax("a call starts with a function's name"));
                };
                self.list(name, arguments, expected)
            }
        }
    }

    fn arguments(
        &mut self,
        arguments: &[Expr],
        types: &[ValueType],
    ) -> Result<Vec<u32>, ConsoleError> {
        arguments
            .iter()
            .zip(types)
            .map(|(argument, &value_type)| Ok(self.expression(argument, value_type)?.0))
            .collect()
    }

    fn count(function: &str, given: usize, least: usize, most: usize) -> Result<(), ConsoleError> {
        if given < least || given > most {
            return Err(ConsoleError::Arguments {
                function: function.to_owned(),
                expected: if given < least { least } else { most },
                given,
            });
        }
        Ok(())
    }

    fn list(
        &mut self,
        name: &str,
        arguments: &[Expr],
        expected: ValueType,
    ) -> Result<(u32, ValueType), ConsoleError> {
        let given = arguments.len();
        let all = |value_type| vec![value_type; given];
        match name {
            "begin" | "begin_random" => {
                Self::count(name, given, 1, usize::MAX)?;
                let mut nodes = Vec::with_capacity(given);
                let mut last = ValueType::Void;
                for (index, argument) in arguments.iter().enumerate() {
                    let (node, value_type) = if index + 1 == given {
                        self.expression(argument, expected)?
                    } else {
                        self.expression(argument, ValueType::Void)?
                    };
                    nodes.push(node);
                    last = value_type;
                }
                self.call(name, expected, last, nodes)
            }
            "if" => {
                Self::count(name, given, 2, 3)?;
                let (condition, _) = self.expression(&arguments[0], ValueType::Boolean)?;
                let (then, own) = self.expression(&arguments[1], expected)?;
                let mut nodes = vec![condition, then];
                if let Some(otherwise) = arguments.get(2) {
                    nodes.push(self.expression(otherwise, own)?.0);
                }
                self.call(name, expected, own, nodes)
            }
            "set" => {
                Self::count(name, given, 2, 2)?;
                let Expr::Word {
                    text,
                    quoted: false,
                } = &arguments[0]
                else {
                    return Err(syntax("set takes a global's name"));
                };
                let (global, value_type) = self
                    .global(text)
                    .ok_or_else(|| ConsoleError::UnknownGlobal(text.clone()))?;
                let (target, _) = self.push(value_type, NodeKind::Global(global))?;
                let (value, _) = self.expression(&arguments[1], value_type)?;
                self.call(name, expected, value_type, vec![target, value])
            }
            "and" | "or" => {
                Self::count(name, given, 1, usize::MAX)?;
                let nodes = self.arguments(arguments, &all(ValueType::Boolean))?;
                self.call(name, expected, ValueType::Boolean, nodes)
            }
            "+" | "-" | "*" | "/" | "min" | "max" => {
                Self::count(name, given, 1, usize::MAX)?;
                let nodes = self.arguments(arguments, &all(ValueType::Real))?;
                self.call(name, expected, ValueType::Real, nodes)
            }
            "=" | "!=" | ">" | "<" | ">=" | "<=" => {
                Self::count(name, given, 2, 2)?;
                // (the second is taken as the first's type: a name compared
                // with what a function returns)
                let (first, own) = self.expression(&arguments[0], ValueType::Passthrough)?;
                let (second, _) = self.expression(&arguments[1], own)?;
                self.call(name, expected, ValueType::Boolean, vec![first, second])
            }
            "sleep" => {
                Self::count(name, given, 1, 2)?;
                let nodes = self.arguments(arguments, &[ValueType::Short, ValueType::Script])?;
                self.call(name, expected, ValueType::Void, nodes)
            }
            "sleep_until" => {
                Self::count(name, given, 1, 3)?;
                let types = [ValueType::Boolean, ValueType::Short, ValueType::Short];
                let nodes = self.arguments(arguments, &types)?;
                self.call(name, expected, ValueType::Void, nodes)
            }
            "wake" => {
                Self::count(name, given, 1, 1)?;
                let nodes = self.arguments(arguments, &[ValueType::Script])?;
                self.call(name, expected, ValueType::Void, nodes)
            }
            "inspect" => {
                Self::count(name, given, 1, 1)?;
                let nodes = self.arguments(arguments, &[ValueType::Passthrough])?;
                self.call(name, expected, ValueType::Void, nodes)
            }
            "object_to_unit" | "unit" => {
                Self::count(name, given, 1, 1)?;
                let nodes = self.arguments(arguments, &[ValueType::Object])?;
                self.call(name, expected, ValueType::Unit, nodes)
            }
            _ => {
                if let Some(&index) = self.names.function_names.get(name) {
                    let function = &self.names.functions[index];
                    let (returns, parameters) = (function.returns, function.parameters.clone());
                    Self::count(name, given, parameters.len(), parameters.len())?;
                    let nodes = self.arguments(arguments, &parameters)?;
                    return self.call(name, expected, returns, nodes);
                }
                let script = self
                    .names
                    .scripts
                    .scripts
                    .iter()
                    .position(|s| s.name == name);
                if let Some(script) = script {
                    Self::count(name, given, 0, 0)?;
                    let returns = self.names.scripts.scripts[script].return_type;
                    let script = u32::try_from(script).map_err(|_| syntax("too many scripts"))?;
                    return self.push(taken(expected, returns), NodeKind::ScriptCall(script));
                }
                Err(ConsoleError::UnknownFunction(name.to_owned()))
            }
        }
    }

    /// A global by its name: the scenario's first, then the engine's.
    fn global(&self, name: &str) -> Option<(GlobalRef, ValueType)> {
        if let Some(index) = self
            .names
            .scripts
            .globals
            .iter()
            .position(|g| g.name == name)
        {
            let value_type = self.names.scripts.globals[index].value_type;
            let index = u16::try_from(index).ok()?;
            return Some((
                GlobalRef {
                    name: name.to_owned(),
                    external: false,
                    index,
                },
                value_type,
            ));
        }
        let &index = self.names.external_names.get(name)?;
        let value_type = self.names.externals.get(index)?.value_type;
        Some((
            GlobalRef {
                name: name.to_owned(),
                external: true,
                index: u16::try_from(index).ok()?,
            },
            value_type,
        ))
    }

    fn word(
        &mut self,
        text: &str,
        quoted: bool,
        expected: ValueType,
    ) -> Result<(u32, ValueType), ConsoleError> {
        if !quoted {
            if let Some((global, value_type)) = self.global(text) {
                return self.push(taken(expected, value_type), NodeKind::Global(global));
            }
        }
        let not_a_value = || ConsoleError::NotAValue {
            value_type: expected,
            given: text.to_owned(),
        };
        let unknown = |value_type| ConsoleError::UnknownName {
            value_type,
            name: text.to_owned(),
        };
        let (written, constant) = match expected {
            ValueType::Passthrough | ValueType::Void => {
                if quoted {
                    (ValueType::String, Constant::String(text.to_owned()))
                } else if let Some(value) = real(text) {
                    (ValueType::Real, Constant::Real(value))
                } else if let Some(value) = boolean(text) {
                    // (a number first: `1` with nothing to say otherwise is one)
                    (ValueType::Boolean, Constant::Boolean(value))
                } else {
                    return Err(ConsoleError::UnknownGlobal(text.to_owned()));
                }
            }
            ValueType::Boolean => (
                expected,
                Constant::Boolean(boolean(text).ok_or_else(not_a_value)?),
            ),
            ValueType::Real => (
                expected,
                Constant::Real(real(text).ok_or_else(not_a_value)?),
            ),
            ValueType::Short => (
                expected,
                Constant::Short(text.parse().map_err(|_| not_a_value())?),
            ),
            ValueType::Long => (
                expected,
                Constant::Long(text.parse().map_err(|_| not_a_value())?),
            ),
            ValueType::String => (expected, Constant::String(text.to_owned())),
            ValueType::Script => {
                let index = self
                    .names
                    .scripts
                    .scripts
                    .iter()
                    .position(|s| s.name == text)
                    .and_then(|i| i32::try_from(i).ok())
                    .ok_or_else(|| unknown(expected))?;
                (expected, named(text, index))
            }
            t if t.is_tag() => {
                let tag = if text == "none" {
                    None
                } else {
                    Some(self.world.tag_of(t, text).ok_or_else(|| unknown(t))?)
                };
                (t, Constant::Tag(tag))
            }
            t if t.is_object() || t == ValueType::ObjectList => {
                let name_type = if t == ValueType::ObjectList {
                    ValueType::ObjectName
                } else {
                    name_of(t)
                };
                if text == "none" {
                    (name_type, named(text, -1))
                } else {
                    let index = self.world.name_index(name_type, text);
                    if index >= 0 {
                        (name_type, named(text, index))
                    } else if t == ValueType::ObjectList
                        && self.world.name_index(ValueType::Ai, text) >= 0
                    {
                        let index = self.world.name_index(ValueType::Ai, text);
                        (ValueType::Ai, named(text, index))
                    } else {
                        return Err(unknown(name_type));
                    }
                }
            }
            t => {
                let index = self.world.name_index(t, text);
                if index < 0 && text != "none" {
                    return Err(unknown(t));
                }
                (t, named(text, index))
            }
        };
        self.push(
            taken(expected, written),
            NodeKind::Constant(written, constant),
        )
    }
}

fn named(name: &str, index: i32) -> Constant {
    Constant::Named(Named {
        name: name.to_owned(),
        index,
    })
}

fn boolean(text: &str) -> Option<bool> {
    match text {
        "true" | "on" | "1" => Some(true),
        "false" | "off" | "0" => Some(false),
        _ => None,
    }
}

fn real(text: &str) -> Option<f32> {
    text.parse::<f32>().ok().filter(|v| v.is_finite())
}
