//! HaloScript: a scenario's scripts run as Halo runs them
//! (`source/hs/hs_runtime.c` as the reference).
//!
//! Each startup, dormant and continuous script is a thread. A thread runs
//! each tick it is due, until it sleeps or its script ends; its expression
//! is evaluated on a stack of frames of its own, so a thread that sleeps in
//! the middle of an expression carries on there when it wakes. The special
//! forms (`begin`, `if`, `set`, `sleep`, `sleep_until` and the rest) are
//! the runtime's own; every other function is a native a system of the
//! engine registers, with its types, in a [`Registry`]. A function or an
//! engine's global the scenario names but no system registered is reported
//! by [`Runtime::missing`], and returns its type's default.

mod value;

use std::collections::{BTreeMap, BTreeSet};

use opence_content::{Constant, NodeKind, ScriptKind, Scripts, ValueType};

pub use value::{cast, Value};

/// The game ticks a second (the simulation's).
pub const TICKS_PER_SECOND: u32 = 30;

/// The frames a thread's stack may hold (a static script calling itself
/// without end runs out of them, and its thread is ended).
pub const MAX_FRAMES: usize = 256;

/// The steps a thread may take in one tick before it is ended (a script
/// does not loop within a tick: only `sleep_until` waits, and it sleeps).
pub const MAX_STEPS: u32 = 1 << 20;

/// What the scripts ask of the game that the runtime itself needs: the
/// casts between objects, their names and lists, and the console.
pub trait World {
    /// The object a scenario's object name stands for (`-1` for none).
    fn object_of_name(&mut self, _name: i32) -> i32 {
        -1
    }

    /// A new list of these objects (`-1` for none).
    fn object_list_of(&mut self, _objects: &[i32]) -> i32 {
        -1
    }

    /// A new list of an encounter's actors' units (`-1` for none).
    fn object_list_of_ai(&mut self, _ai: i32) -> i32 {
        -1
    }

    /// Writes a line to the console (`inspect`).
    fn print(&mut self, _text: &str) {}
}

/// A function a system gives the scripts: it takes its arguments, each
/// already of its parameter's type, and returns a value of its type.
pub type Native<W> = fn(&mut W, &[Value]) -> Value;

struct Function<W> {
    returns: ValueType,
    parameters: Vec<ValueType>,
    evaluate: Native<W>,
}

struct External {
    name: String,
    value_type: ValueType,
    value: Value,
}

/// The functions and globals the engine's systems give the scripts.
pub struct Registry<W> {
    functions: BTreeMap<String, Function<W>>,
    globals: BTreeMap<String, (ValueType, Value)>,
}

impl<W> Default for Registry<W> {
    fn default() -> Self {
        Self {
            functions: BTreeMap::new(),
            globals: BTreeMap::new(),
        }
    }
}

impl<W> Registry<W> {
    /// No functions and no globals.
    pub fn new() -> Self {
        Self::default()
    }

    /// Gives the scripts a function, by its name, its type and its
    /// parameters' types.
    pub fn function(
        &mut self,
        name: &str,
        returns: ValueType,
        parameters: &[ValueType],
        evaluate: Native<W>,
    ) {
        self.functions.insert(
            name.to_owned(),
            Function {
                returns,
                parameters: parameters.to_vec(),
                evaluate,
            },
        );
    }

    /// Gives the scripts one of the engine's globals, and what it starts as.
    pub fn global(&mut self, name: &str, value_type: ValueType, value: Value) {
        self.globals.insert(name.to_owned(), (value_type, value));
    }

    /// Whether a function of this name is given.
    pub fn has_function(&self, name: &str) -> bool {
        self.functions.contains_key(name)
    }
}

/// Something the scenario's scripts name that the engine does not give
/// them.
#[derive(Clone, Debug, PartialEq, Eq, PartialOrd, Ord)]
pub enum Missing {
    /// A function no system registered.
    Function(String),
    /// An engine's global no system registered.
    Global(String),
    /// A call whose arguments are not the function's (its name).
    Arguments(String),
}

/// When a thread runs next.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Sleep {
    /// On the first tick at or after this time.
    Until(u32),
    /// When it is woken (a dormant script, or `(sleep -1)`).
    Dormant,
    /// Never: its script has ended.
    Done,
}

impl Sleep {
    fn due(self, time: u32) -> bool {
        matches!(self, Self::Until(at) if at <= time)
    }
}

#[derive(Clone, Copy, Debug, PartialEq)]
enum Arithmetic {
    Plus,
    Minus,
    Times,
    Divide,
    Min,
    Max,
}

#[derive(Clone, Copy, Debug, PartialEq)]
enum Comparison {
    Equal,
    NotEqual,
    Greater,
    Less,
    GreaterOrEqual,
    LessOrEqual,
}

#[derive(Clone, Copy, Debug, PartialEq)]
enum Slot {
    Scenario(usize),
    Engine(usize),
    // (a global no system gave: setting it does nothing)
    Nowhere,
}

#[derive(Clone, Copy, Debug, PartialEq)]
enum Form {
    Begin,
    BeginRandom,
    If,
    Set(Slot),
    And,
    Or,
    Arithmetic(Arithmetic),
    Comparison(Comparison),
    Sleep,
    SleepUntil,
    Wake(Option<usize>),
    Inspect,
    ObjectToUnit,
}

impl Form {
    /// The type `hs_return` casts its value from.
    fn returns(self) -> ValueType {
        match self {
            Self::Begin | Self::BeginRandom | Self::If | Self::Set(_) => ValueType::Passthrough,
            Self::And | Self::Or | Self::Comparison(_) => ValueType::Boolean,
            Self::Arithmetic(_) => ValueType::Real,
            Self::Sleep | Self::SleepUntil | Self::Wake(_) | Self::Inspect => ValueType::Void,
            Self::ObjectToUnit => ValueType::Unit,
        }
    }
}

#[derive(Clone, Debug, PartialEq)]
enum Resolved {
    Constant(Value, ValueType),
    Global(Slot),
    Script(usize),
    Form(Form, Vec<u32>),
    Native(usize, Vec<u32>),
    // (a function that is not there, or not called as it should be: its
    // type's default)
    Missing,
}

#[derive(Debug, Default)]
struct Frame {
    node: u32,
    step: usize,
    values: Vec<Value>,
    order: Vec<u32>,
    start: u32,
}

impl Frame {
    fn of(node: u32) -> Self {
        Self {
            node,
            ..Self::default()
        }
    }
}

struct Thread {
    script: Option<usize>,
    stack: Vec<Frame>,
    sleep: Sleep,
    // (when another thread made it sleep: what it was sleeping until, for
    // `wake` to give back)
    previous: Option<Sleep>,
    result: Value,
}

enum Step {
    Push(u32),
    Return(Value),
    // (the frame stays, and the thread sleeps)
    Wait,
}

/// The scenario's scripts running.
pub struct Runtime<W> {
    scripts: Scripts,
    resolved: Vec<Resolved>,
    functions: Vec<Function<W>>,
    globals: Vec<Value>,
    externals: Vec<External>,
    threads: Vec<Thread>,
    time: u32,
    missing: BTreeSet<Missing>,
    // (the scripts whose threads ran away or overflowed their stacks)
    faults: Vec<String>,
    random: u32,
}

impl<W: World> Runtime<W> {
    /// The scripts, with the functions and globals the engine gives them,
    /// their globals given their first values, and a thread for each script
    /// that runs at the top: none has run yet.
    pub fn new(scripts: Scripts, registry: Registry<W>, world: &mut W) -> Self {
        let mut missing = BTreeSet::new();
        let mut names = BTreeMap::new();
        let functions: Vec<Function<W>> = registry
            .functions
            .into_iter()
            .enumerate()
            .map(|(index, (name, function))| {
                names.insert(name, index);
                function
            })
            .collect();
        let mut externals = Vec::new();
        let mut external_names = BTreeMap::new();
        for (name, (value_type, value)) in registry.globals {
            external_names.insert(name.clone(), externals.len());
            externals.push(External {
                name,
                value_type,
                value,
            });
        }

        let resolved = (0..scripts.nodes.len())
            .map(|node| {
                resolve(
                    &scripts,
                    node,
                    &names,
                    &functions,
                    &external_names,
                    &mut missing,
                )
            })
            .collect();
        let threads = scripts
            .scripts
            .iter()
            .enumerate()
            .filter_map(|(index, script)| {
                let sleep = match script.kind {
                    ScriptKind::Startup | ScriptKind::Continuous => Sleep::Until(0),
                    ScriptKind::Dormant => Sleep::Dormant,
                    ScriptKind::Static | ScriptKind::Stub => return None,
                };
                Some(Thread {
                    script: Some(index),
                    stack: Vec::new(),
                    sleep,
                    previous: None,
                    result: Value::Void,
                })
            })
            .collect();
        let globals = scripts
            .globals
            .iter()
            .map(|global| Value::default_of(global.value_type))
            .collect();
        let mut runtime = Self {
            scripts,
            resolved,
            functions,
            globals,
            externals,
            threads,
            time: 0,
            missing,
            faults: Vec::new(),
            random: 0x2545_F491,
        };
        // (in order: a global's first value may be an earlier one's)
        for index in 0..runtime.scripts.globals.len() {
            let global = &runtime.scripts.globals[index];
            if let Some(root) = global.initial {
                let value_type = global.value_type;
                let value = runtime.evaluate(root, world);
                let from = runtime.node_type(root);
                runtime.globals[index] = cast(world, value, from, value_type);
            }
        }
        runtime
    }

    /// Runs every thread that is due this tick, in the scenarios' order,
    /// then moves the time on a tick.
    pub fn tick(&mut self, world: &mut W) {
        for thread in 0..self.threads.len() {
            if self.threads[thread].sleep.due(self.time) {
                self.run(thread, world);
            }
        }
        self.time = self.time.wrapping_add(1);
    }

    /// The ticks run.
    pub fn time(&self) -> u32 {
        self.time
    }

    /// What the scripts name that the engine does not give them.
    pub fn missing(&self) -> &BTreeSet<Missing> {
        &self.missing
    }

    /// The scripts whose threads were ended for running away or for
    /// overflowing their stacks, in the order they were.
    pub fn faults(&self) -> &[String] {
        &self.faults
    }

    /// The conditions the threads asleep in a `sleep_until` wait on: each
    /// condition's node.
    pub fn waiting(&self) -> Vec<u32> {
        self.threads
            .iter()
            .filter(|t| t.sleep != Sleep::Done)
            .flat_map(|t| &t.stack)
            .filter_map(|frame| match self.resolved.get(frame.node as usize) {
                Some(Resolved::Form(Form::SleepUntil, arguments)) => arguments.first().copied(),
                _ => None,
            })
            .collect()
    }

    /// A scenario's global's value, by its name.
    pub fn global(&self, name: &str) -> Option<&Value> {
        let index = self.scripts.globals.iter().position(|g| g.name == name)?;
        self.globals.get(index)
    }

    /// One of the engine's globals' value, by its name.
    pub fn engine_global(&self, name: &str) -> Option<&Value> {
        self.externals
            .iter()
            .find(|e| e.name == name)
            .map(|e| &e.value)
    }

    /// Sets one of the engine's globals.
    pub fn set_engine_global(&mut self, name: &str, value: Value) {
        if let Some(external) = self.externals.iter_mut().find(|e| e.name == name) {
            external.value = value;
        }
    }

    /// When a script's thread runs next (none for a static script).
    pub fn sleep_of(&self, script: &str) -> Option<Sleep> {
        self.threads
            .iter()
            .find(|t| {
                t.script
                    .and_then(|s| self.scripts.scripts.get(s))
                    .is_some_and(|s| s.name == script)
            })
            .map(|t| t.sleep)
    }

    /// Wakes a script's thread, as `wake` does: it runs on its next turn.
    pub fn wake(&mut self, script: &str) {
        if let Some(index) = self.scripts.scripts.iter().position(|s| s.name == script) {
            self.wake_script(index);
        }
    }

    /// Evaluates an expression to its end, on a thread of its own that may
    /// not sleep (a global's first value, or the console's); one that tries
    /// to gives its type's default.
    pub fn evaluate(&mut self, root: u32, world: &mut W) -> Value {
        let thread = self.threads.len();
        self.threads.push(Thread {
            script: None,
            stack: vec![Frame::of(root)],
            sleep: Sleep::Until(0),
            previous: None,
            result: Value::Void,
        });
        let mut steps = 0;
        while !self.threads[thread].stack.is_empty() && steps < MAX_STEPS {
            steps += 1;
            if !self.step(thread, world) {
                break;
            }
            if self.threads[thread].sleep != Sleep::Until(0) {
                break;
            }
        }
        let finished = self.threads[thread].stack.is_empty();
        let result = self.threads.pop().map(|t| t.result);
        match result {
            Some(value) if finished => value,
            _ => Value::default_of(self.node_type(root)),
        }
    }

    fn node_type(&self, node: u32) -> ValueType {
        self.scripts
            .nodes
            .get(node as usize)
            .map_or(ValueType::Void, |n| n.value_type)
    }

    fn wake_script(&mut self, script: usize) {
        let Some(thread) = self.threads.iter_mut().find(|t| t.script == Some(script)) else {
            return;
        };
        if thread.sleep != Sleep::Done {
            thread.sleep = thread.previous.take().unwrap_or(Sleep::Until(0));
        }
    }

    /// Runs a thread until it sleeps or its script ends (`hs_thread_main`).
    fn run(&mut self, thread: usize, world: &mut W) {
        let Some(script) = self.threads[thread].script else {
            return;
        };
        self.threads[thread].sleep = Sleep::Until(0);
        if self.threads[thread].stack.is_empty() {
            let Some(root) = self.scripts.scripts.get(script).map(|s| s.root) else {
                self.threads[thread].sleep = Sleep::Done;
                return;
            };
            self.threads[thread].stack.push(Frame::of(root));
        }
        let mut steps = 0;
        while !self.threads[thread].stack.is_empty() && self.threads[thread].sleep.due(self.time) {
            steps += 1;
            if steps > MAX_STEPS || !self.step(thread, world) {
                // (a script that runs away, or a stack that overflows, ends
                // its thread for good)
                if let Some(script) = self.scripts.scripts.get(script) {
                    self.faults.push(script.name.clone());
                }
                self.threads[thread].stack.clear();
                self.threads[thread].sleep = Sleep::Done;
                return;
            }
        }
        if self.threads[thread].stack.is_empty() {
            let kind = self.scripts.scripts.get(script).map(|s| s.kind);
            if matches!(kind, Some(ScriptKind::Startup | ScriptKind::Dormant)) {
                self.threads[thread].sleep = Sleep::Done;
            }
            // (a continuous script starts again on its next turn)
        }
    }

    /// Takes one step of the thread's top frame: it calls for a child's
    /// value, returns its own, or sleeps. False when the stack overflows.
    fn step(&mut self, thread: usize, world: &mut W) -> bool {
        let Some(mut frame) = self.threads[thread].stack.pop() else {
            return true;
        };
        let node = frame.node;
        let value_type = self.node_type(node);
        let resolved = self
            .resolved
            .get(node as usize)
            .cloned()
            .unwrap_or(Resolved::Missing);
        let step = match resolved {
            Resolved::Constant(value, from) => Step::Return(cast(world, value, from, value_type)),
            Resolved::Global(slot) => {
                let (value, from) = self.read(slot);
                Step::Return(cast(world, value, from, value_type))
            }
            Resolved::Missing => Step::Return(Value::default_of(value_type)),
            Resolved::Script(script) => match frame.values.pop() {
                None if frame.step == 0 => {
                    frame.step = 1;
                    match self.scripts.scripts.get(script) {
                        Some(s) => Step::Push(s.root),
                        None => Step::Return(Value::default_of(value_type)),
                    }
                }
                value => {
                    let from = self
                        .scripts
                        .scripts
                        .get(script)
                        .map_or(ValueType::Void, |s| s.return_type);
                    let value = value.unwrap_or(Value::Void);
                    Step::Return(cast(world, value, from, value_type))
                }
            },
            Resolved::Native(function, arguments) => {
                if let Some(&next) = arguments.get(frame.values.len()) {
                    Step::Push(next)
                } else {
                    let Some(function) = self.functions.get(function) else {
                        return true;
                    };
                    let values: Vec<Value> = frame
                        .values
                        .drain(..)
                        .zip(&arguments)
                        .zip(&function.parameters)
                        .map(|((value, &argument), &parameter)| {
                            let from = self
                                .scripts
                                .nodes
                                .get(argument as usize)
                                .map_or(parameter, |n| n.value_type);
                            cast(world, value, from, parameter)
                        })
                        .collect();
                    let value = (function.evaluate)(world, &values);
                    Step::Return(cast(world, value, function.returns, value_type))
                }
            }
            Resolved::Form(form, arguments) => {
                match self.form(thread, form, &arguments, &mut frame, world) {
                    Step::Return(value) => {
                        Step::Return(cast(world, value, form.returns(), value_type))
                    }
                    other => other,
                }
            }
        };
        let stack = &mut self.threads[thread].stack;
        match step {
            Step::Push(child) => {
                stack.push(frame);
                if stack.len() >= MAX_FRAMES {
                    return false;
                }
                stack.push(Frame::of(child));
            }
            Step::Return(value) => match stack.last_mut() {
                Some(parent) => parent.values.push(value),
                None => self.threads[thread].result = value,
            },
            Step::Wait => stack.push(frame),
        }
        true
    }

    fn read(&self, slot: Slot) -> (Value, ValueType) {
        match slot {
            Slot::Scenario(index) => (
                self.globals.get(index).cloned().unwrap_or(Value::Void),
                self.scripts
                    .globals
                    .get(index)
                    .map_or(ValueType::Void, |g| g.value_type),
            ),
            Slot::Engine(index) => self
                .externals
                .get(index)
                .map_or((Value::Void, ValueType::Void), |e| {
                    (e.value.clone(), e.value_type)
                }),
            // (cast from void, it is the type's default)
            Slot::Nowhere => (Value::Void, ValueType::Void),
        }
    }

    fn next_random(&mut self) -> u32 {
        // (xorshift: the same order on every machine)
        let mut x = self.random;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        self.random = x;
        x
    }

    /// A special form's step.
    fn form(
        &mut self,
        thread: usize,
        form: Form,
        arguments: &[u32],
        frame: &mut Frame,
        world: &mut W,
    ) -> Step {
        let evaluated = frame.values.len();
        let next = arguments.get(evaluated).copied();
        match form {
            Form::Begin => match next {
                Some(child) => Step::Push(child),
                None => Step::Return(frame.values.pop().unwrap_or(Value::Void)),
            },
            Form::BeginRandom => {
                if frame.step == 0 {
                    frame.step = 1;
                    frame.order = arguments.to_vec();
                    // (a shuffle: each expression once, in a random order)
                    for i in (1..frame.order.len()).rev() {
                        let j = self.next_random() as usize % (i + 1);
                        frame.order.swap(i, j);
                    }
                }
                match frame.order.get(evaluated) {
                    Some(&child) => Step::Push(child),
                    None => Step::Return(frame.values.pop().unwrap_or(Value::Void)),
                }
            }
            // (step 0 asks for the condition, step 1 for a branch; step 2
            // returns the branch's value)
            Form::If => match frame.step {
                0 => {
                    frame.step = 1;
                    Step::Push(arguments[0])
                }
                1 => {
                    frame.step = 2;
                    let branch = if frame.values.pop().is_some_and(|v| v.as_boolean()) {
                        arguments.get(1)
                    } else {
                        arguments.get(2)
                    };
                    match branch {
                        Some(&branch) => Step::Push(branch),
                        None => Step::Return(Value::default_of(self.node_type(frame.node))),
                    }
                }
                _ => Step::Return(frame.values.pop().unwrap_or(Value::Void)),
            },
            Form::Set(slot) => match evaluated {
                0 => Step::Push(arguments[1]),
                _ => {
                    let value = frame.values.pop().unwrap_or(Value::Void);
                    let from = self.node_type(arguments[1]);
                    let (_, to) = self.read(slot);
                    let stored = cast(world, value.clone(), from, to);
                    match slot {
                        Slot::Scenario(index) => {
                            if let Some(global) = self.globals.get_mut(index) {
                                *global = stored;
                            }
                        }
                        Slot::Engine(index) => {
                            if let Some(external) = self.externals.get_mut(index) {
                                external.value = stored;
                            }
                        }
                        Slot::Nowhere => {}
                    }
                    Step::Return(value)
                }
            },
            Form::And | Form::Or => {
                let deciding = form == Form::Or;
                match frame.values.last() {
                    Some(value) if value.as_boolean() == deciding => {
                        Step::Return(Value::Boolean(deciding))
                    }
                    _ => match next {
                        Some(child) => Step::Push(child),
                        None => Step::Return(Value::Boolean(!deciding)),
                    },
                }
            }
            Form::Arithmetic(operation) => match next {
                Some(child) => Step::Push(child),
                None => {
                    let mut values = frame.values.iter().map(Value::as_real);
                    let first = values.next().unwrap_or(0.0);
                    Step::Return(Value::Real(values.fold(first, |a, b| match operation {
                        Arithmetic::Plus => a + b,
                        Arithmetic::Minus => a - b,
                        Arithmetic::Times => a * b,
                        Arithmetic::Divide => a / b,
                        Arithmetic::Min => a.min(b),
                        Arithmetic::Max => a.max(b),
                    })))
                }
            },
            Form::Comparison(comparison) => match next {
                Some(child) => Step::Push(child),
                None => {
                    let (a, b) = (&frame.values[0], &frame.values[1]);
                    let numbers = self.node_type(arguments[0]).is_number();
                    let (x, y) = (a.as_real(), b.as_real());
                    Step::Return(Value::Boolean(match comparison {
                        Comparison::Equal if numbers => x == y,
                        Comparison::NotEqual if numbers => x != y,
                        Comparison::Equal => a == b,
                        Comparison::NotEqual => a != b,
                        Comparison::Greater => x > y,
                        Comparison::Less => x < y,
                        Comparison::GreaterOrEqual => x >= y,
                        Comparison::LessOrEqual => x <= y,
                    }))
                }
            },
            Form::Sleep => match next {
                Some(child) => Step::Push(child),
                None => {
                    self.sleep(thread, &frame.values);
                    Step::Return(Value::Void)
                }
            },
            Form::SleepUntil => self.sleep_until(thread, arguments, frame, world),
            Form::Wake(script) => {
                if let Some(script) = script {
                    self.wake_script(script);
                }
                Step::Return(Value::Void)
            }
            Form::Inspect => match next {
                Some(child) => Step::Push(child),
                None => {
                    let text = match &frame.values[0] {
                        Value::Void => String::new(),
                        Value::Boolean(v) => v.to_string(),
                        Value::Real(v) => format!("{v:.6}"),
                        Value::Short(v) => v.to_string(),
                        Value::Long(v) | Value::Index(v) => v.to_string(),
                        Value::String(v) => v.clone(),
                        Value::Tag(tag) => tag.as_ref().map_or("none".into(), |t| t.path.clone()),
                    };
                    world.print(&text);
                    Step::Return(Value::Void)
                }
            },
            Form::ObjectToUnit => match next {
                Some(child) => Step::Push(child),
                None => Step::Return(frame.values.pop().unwrap_or(Value::Index(-1))),
            },
        }
    }

    /// `(sleep <ticks> [<script>])`: none for 0 ticks, until woken for
    /// fewer, and otherwise this thread or the script's for that long.
    fn sleep(&mut self, thread: usize, values: &[Value]) {
        let ticks = values.first().map_or(0, Value::as_index);
        if ticks == 0 {
            return;
        }
        let until = match u32::try_from(ticks) {
            Ok(ticks) => Sleep::Until(self.time.wrapping_add(ticks)),
            Err(_) => Sleep::Dormant,
        };
        let target = match values.get(1).map(Value::as_index) {
            None => thread,
            Some(script) => {
                let script = usize::try_from(script).ok();
                match self
                    .threads
                    .iter()
                    .position(|t| t.script.is_some() && t.script == script)
                {
                    Some(target) => target,
                    None => return,
                }
            }
        };
        let sleeper = &mut self.threads[target];
        if sleeper.sleep == Sleep::Done {
            return;
        }
        if target != thread && sleeper.previous.is_none() {
            sleeper.previous = Some(sleeper.sleep);
        }
        sleeper.sleep = until;
    }

    /// `(sleep_until <condition> [<ticks> [<timeout>]])`: tests the
    /// condition every so many ticks (30 unless given), until it is true or
    /// the time out has passed. As Halo's does, it sleeps once before it
    /// first tests.
    fn sleep_until(
        &mut self,
        thread: usize,
        arguments: &[u32],
        frame: &mut Frame,
        world: &mut W,
    ) -> Step {
        // (values: the period, then the timeout, then each test's result)
        if frame.step == 0 {
            frame.start = self.time;
            frame.step = 1;
            if let Some(&period) = arguments.get(1) {
                return Step::Push(period);
            }
            frame.values.push(Value::Short(30));
        }
        if frame.step == 1 {
            frame.step = 2;
            if let Some(&timeout) = arguments.get(2) {
                return Step::Push(timeout);
            }
            frame.values.push(Value::Long(-1));
        }
        let period = frame.values[0].as_index().max(1);
        let timeout = frame.values[1].as_index();
        let condition = frame.values.get(2).is_some_and(Value::as_boolean);
        let start = i64::from(frame.start);
        let now = i64::from(self.time);
        if condition || (timeout >= 0 && now >= start + i64::from(timeout)) {
            return Step::Return(Value::Void);
        }
        frame.values.truncate(2);
        let mut until = now + i64::from(period);
        if timeout >= 0 {
            until = until.min(start + i64::from(timeout));
        }
        self.threads[thread].sleep = Sleep::Until(u32::try_from(until).unwrap_or(u32::MAX));
        // (a constant's or a global's value is taken now; a call's when the
        // thread wakes)
        let condition_node = arguments[0];
        match self.resolved.get(condition_node as usize) {
            Some(Resolved::Constant(..) | Resolved::Global(_)) => {
                let value = self.value_now(condition_node, world);
                frame.values.push(value);
                Step::Wait
            }
            _ => Step::Push(condition_node),
        }
    }

    /// A constant's or a global's value, as its node takes it.
    fn value_now(&self, node: u32, world: &mut W) -> Value {
        let value_type = self.node_type(node);
        match self.resolved.get(node as usize) {
            Some(Resolved::Constant(value, from)) => cast(world, value.clone(), *from, value_type),
            Some(Resolved::Global(slot)) => {
                let (value, from) = self.read(*slot);
                cast(world, value, from, value_type)
            }
            _ => Value::default_of(value_type),
        }
    }
}

/// The functions the runtime evaluates itself.
pub const SPECIAL_FORMS: [&str; 24] = [
    "begin",
    "begin_random",
    "if",
    "set",
    "and",
    "or",
    "+",
    "-",
    "*",
    "/",
    "min",
    "max",
    "=",
    "!=",
    ">",
    "<",
    ">=",
    "<=",
    "sleep",
    "sleep_until",
    "wake",
    "inspect",
    "object_to_unit",
    "unit",
];

/// What a node is to the runtime: its value, its global, its script, its
/// special form or its native, with what the scenario names that the
/// engine does not have recorded.
fn resolve<W>(
    scripts: &Scripts,
    node: usize,
    names: &BTreeMap<String, usize>,
    functions: &[Function<W>],
    externals: &BTreeMap<String, usize>,
    missing: &mut BTreeSet<Missing>,
) -> Resolved {
    let Some(this) = scripts.nodes.get(node) else {
        return Resolved::Missing;
    };
    match &this.kind {
        NodeKind::Constant(written, constant) => {
            Resolved::Constant(Value::of_constant(constant), *written)
        }
        NodeKind::Global(global) => Resolved::Global(slot(scripts, global, externals, missing)),
        NodeKind::ScriptCall(script) => {
            let script = *script as usize;
            if script < scripts.scripts.len() {
                Resolved::Script(script)
            } else {
                Resolved::Missing
            }
        }
        NodeKind::Call(call) => {
            let arguments = call.arguments.clone();
            if arguments.iter().any(|&a| a as usize >= scripts.nodes.len()) {
                missing.insert(Missing::Arguments(call.function.clone()));
                return Resolved::Missing;
            }
            let count = arguments.len();
            let form = match call.function.as_str() {
                "begin" => Some(Form::Begin),
                "begin_random" => Some(Form::BeginRandom),
                "if" if (2..=3).contains(&count) => Some(Form::If),
                "set" if count == 2 => match &scripts.nodes[arguments[0] as usize].kind {
                    NodeKind::Global(global) => {
                        Some(Form::Set(slot(scripts, global, externals, missing)))
                    }
                    _ => None,
                },
                "and" if count >= 1 => Some(Form::And),
                "or" if count >= 1 => Some(Form::Or),
                "+" if count >= 1 => Some(Form::Arithmetic(Arithmetic::Plus)),
                "-" if count >= 1 => Some(Form::Arithmetic(Arithmetic::Minus)),
                "*" if count >= 1 => Some(Form::Arithmetic(Arithmetic::Times)),
                "/" if count >= 1 => Some(Form::Arithmetic(Arithmetic::Divide)),
                "min" if count >= 1 => Some(Form::Arithmetic(Arithmetic::Min)),
                "max" if count >= 1 => Some(Form::Arithmetic(Arithmetic::Max)),
                "=" if count == 2 => Some(Form::Comparison(Comparison::Equal)),
                "!=" if count == 2 => Some(Form::Comparison(Comparison::NotEqual)),
                ">" if count == 2 => Some(Form::Comparison(Comparison::Greater)),
                "<" if count == 2 => Some(Form::Comparison(Comparison::Less)),
                ">=" if count == 2 => Some(Form::Comparison(Comparison::GreaterOrEqual)),
                "<=" if count == 2 => Some(Form::Comparison(Comparison::LessOrEqual)),
                "sleep" if (1..=2).contains(&count) => Some(Form::Sleep),
                "sleep_until" if (1..=3).contains(&count) => Some(Form::SleepUntil),
                "wake" if count == 1 => match &scripts.nodes[arguments[0] as usize].kind {
                    NodeKind::Constant(_, Constant::Named(named)) => Some(Form::Wake(
                        usize::try_from(named.index)
                            .ok()
                            .filter(|&s| s < scripts.scripts.len()),
                    )),
                    _ => None,
                },
                "inspect" if count == 1 => Some(Form::Inspect),
                "object_to_unit" | "unit" if count == 1 => Some(Form::ObjectToUnit),
                _ => None,
            };
            if let Some(form) = form {
                return Resolved::Form(form, arguments);
            }
            if SPECIAL_FORMS.contains(&call.function.as_str()) {
                missing.insert(Missing::Arguments(call.function.clone()));
                return Resolved::Missing;
            }
            match names.get(&call.function) {
                Some(&index) if functions[index].parameters.len() == count => {
                    Resolved::Native(index, arguments)
                }
                Some(_) => {
                    missing.insert(Missing::Arguments(call.function.clone()));
                    Resolved::Missing
                }
                None => {
                    missing.insert(Missing::Function(call.function.clone()));
                    Resolved::Missing
                }
            }
        }
    }
}

fn slot(
    scripts: &Scripts,
    global: &opence_content::GlobalRef,
    externals: &BTreeMap<String, usize>,
    missing: &mut BTreeSet<Missing>,
) -> Slot {
    if global.external {
        match externals.get(&global.name) {
            Some(&index) => Slot::Engine(index),
            None => {
                missing.insert(Missing::Global(global.name.clone()));
                Slot::Nowhere
            }
        }
    } else if usize::from(global.index) < scripts.globals.len() {
        Slot::Scenario(usize::from(global.index))
    } else {
        Slot::Nowhere
    }
}
