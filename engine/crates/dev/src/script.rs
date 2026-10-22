//! Command scripts (`.ocs`): the test driver's lists of commands, with
//! four lines of their own. `wait <ticks>` lets the world run;
//! `wait_until <expression> [timeout]` lets it run until the expression
//! holds (failing after `timeout` ticks); `expect <expression>` fails the
//! script unless it holds; `frame <name>` takes a frame. Every other line
//! is a command line, run as the shell runs it. Blank lines and `#`
//! comments are skipped.
//!
//! An expression is a value, or a value compared with another by `==`,
//! `!=`, `<`, `<=`, `>` or `>=`. The host gives values (a variable, the
//! world's state, HaloScript); a number, `true`, `false` or quoted text on
//! the right is taken as it is written. A value holds when it is `true`,
//! `on`, `yes` or a number other than zero.
//!
//! The runner does not keep time: its host runs ticks between polls, so
//! that one script runs headless as fast as the world can tick, or in the
//! window at the window's pace.

use std::fmt;

use serde_json::json;

use crate::words::parse_words;
use crate::Data;

/// The longest script, in bytes.
pub const MAX_SCRIPT_BYTES: usize = 1 << 20;

/// The most ticks one `wait` or `wait_until` may wait (an hour).
pub const MAX_WAIT: u64 = 30 * 60 * 60;

/// How long `wait_until` waits without a timeout (ten seconds).
pub const DEFAULT_TIMEOUT: u64 = 300;

/// The longest frame name.
pub const MAX_FRAME_NAME: usize = 64;

/// What a script's line does.
#[derive(Clone, Debug, PartialEq, Eq)]
pub enum Directive {
    /// A command line.
    Run(String),
    /// Lets the world run this many ticks.
    Wait(u64),
    /// Lets the world run until the expression holds.
    WaitUntil {
        /// The expression.
        expression: String,
        /// The most ticks it waits.
        timeout: u64,
    },
    /// Fails the script unless the expression holds.
    Expect(String),
    /// Takes a frame with this name.
    Frame(String),
}

/// A line of a script.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct ScriptLine {
    /// Its number in the file, from 1.
    pub number: usize,
    /// Its text, trimmed.
    pub text: String,
    /// What it does.
    pub directive: Directive,
}

/// A script that cannot be read: its line, and why.
#[derive(Clone, Debug, PartialEq, Eq, thiserror::Error)]
#[error("line {line}: {reason}")]
pub struct ParseError {
    /// The line's number, from 1 (0 for the whole script).
    pub line: usize,
    /// Why.
    pub reason: String,
}

/// Reads a script.
///
/// # Errors
///
/// A line of the four directives that is not written as it must be, or a
/// script longer than [`MAX_SCRIPT_BYTES`].
pub fn parse(source: &str) -> Result<Vec<ScriptLine>, ParseError> {
    if source.len() > MAX_SCRIPT_BYTES {
        return Err(ParseError {
            line: 0,
            reason: format!("a script is at most {MAX_SCRIPT_BYTES} bytes"),
        });
    }
    let mut lines = Vec::new();
    for (index, line) in source.lines().enumerate() {
        let text = line.trim();
        if text.is_empty() || text.starts_with('#') {
            continue;
        }
        let number = index + 1;
        let error = |reason: String| ParseError {
            line: number,
            reason,
        };
        let (word, rest) = text
            .split_once(char::is_whitespace)
            .map_or((text, ""), |(word, rest)| (word, rest.trim()));
        let directive = match word {
            "wait" => Directive::Wait(ticks(rest).map_err(error)?),
            "wait_until" => {
                let (expression, timeout) = split_timeout(rest).map_err(error)?;
                if expression.is_empty() {
                    return Err(error("wait_until needs an expression".into()));
                }
                Directive::WaitUntil {
                    expression,
                    timeout,
                }
            }
            "expect" => {
                if rest.is_empty() {
                    return Err(error("expect needs an expression".into()));
                }
                Directive::Expect(rest.to_owned())
            }
            "frame" => Directive::Frame(frame_name(rest).map_err(error)?.to_owned()),
            _ => Directive::Run(text.to_owned()),
        };
        lines.push(ScriptLine {
            number,
            text: text.to_owned(),
            directive,
        });
    }
    Ok(lines)
}

fn ticks(text: &str) -> Result<u64, String> {
    match text.parse::<u64>() {
        Ok(ticks) if (1..=MAX_WAIT).contains(&ticks) => Ok(ticks),
        _ => Err(format!("wait takes 1 to {MAX_WAIT} ticks, not `{text}`")),
    }
}

/// An expression and its timeout: the last word, when there are more and
/// it is a whole number outside parentheses.
fn split_timeout(text: &str) -> Result<(String, u64), String> {
    if let Some((expression, last)) = text.rsplit_once(char::is_whitespace) {
        let expression = expression.trim();
        if last.bytes().all(|byte| byte.is_ascii_digit()) && !ends_compared(expression) {
            let timeout = ticks(last).map_err(|_| {
                format!("wait_until's timeout is 1 to {MAX_WAIT} ticks, not `{last}`")
            })?;
            return Ok((expression.to_owned(), timeout));
        }
    }
    Ok((text.to_owned(), DEFAULT_TIMEOUT))
}

/// Whether `text` ends with a comparison (so that the number after it is
/// what it compares with, not a timeout).
fn ends_compared(text: &str) -> bool {
    text.rsplit(char::is_whitespace)
        .next()
        .is_some_and(|last| Comparison::parse(last).is_some())
}

/// A frame's name: letters, digits, `_` and `-`.
///
/// # Errors
///
/// Anything else, or a name longer than [`MAX_FRAME_NAME`].
pub fn frame_name(text: &str) -> Result<&str, String> {
    let allowed = |c: char| c.is_ascii_alphanumeric() || c == '_' || c == '-';
    if text.is_empty() || text.len() > MAX_FRAME_NAME || !text.chars().all(allowed) {
        return Err(format!(
            "a frame's name is 1 to {MAX_FRAME_NAME} letters, digits, _ and -, not `{text}`"
        ));
    }
    Ok(text)
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
enum Comparison {
    Equal,
    NotEqual,
    Less,
    LessOrEqual,
    Greater,
    GreaterOrEqual,
}

impl Comparison {
    fn parse(word: &str) -> Option<Self> {
        Some(match word {
            "==" => Self::Equal,
            "!=" => Self::NotEqual,
            "<" => Self::Less,
            "<=" => Self::LessOrEqual,
            ">" => Self::Greater,
            ">=" => Self::GreaterOrEqual,
            _ => return None,
        })
    }
}

/// Splits `expression` at a comparison outside parentheses and quotes:
/// the left, the comparison and the right.
fn split_comparison(expression: &str) -> Option<(&str, Comparison, &str)> {
    let mut depth = 0_i32;
    let mut quoted = false;
    let mut start = None;
    let bytes = expression.as_bytes();
    for (index, &byte) in bytes.iter().enumerate() {
        match byte {
            b'"' => quoted = !quoted,
            b'(' if !quoted => depth += 1,
            b')' if !quoted => depth -= 1,
            _ => {}
        }
        let space = byte.is_ascii_whitespace();
        if quoted || depth != 0 {
            start = None;
            continue;
        }
        match (start, space) {
            (None, false) => start = Some(index),
            (Some(from), true) => {
                if let Some(comparison) = expression.get(from..index).and_then(Comparison::parse) {
                    let left = expression.get(..from)?.trim();
                    let right = expression.get(index..)?.trim();
                    if !left.is_empty() && !right.is_empty() {
                        return Some((left, comparison, right));
                    }
                }
                start = None;
            }
            _ => {}
        }
    }
    None
}

/// Whether a value holds.
#[must_use]
pub fn holds(value: &str) -> Option<bool> {
    match value.trim() {
        "true" | "on" | "yes" => Some(true),
        "false" | "off" | "no" | "none" | "" => Some(false),
        other => other.parse::<f64>().ok().map(|number| number != 0.0),
    }
}

/// A literal on a comparison's right: a number, `true` or `false`, or
/// quoted text.
fn literal(text: &str) -> Option<String> {
    if text.parse::<f64>().is_ok() || text == "true" || text == "false" {
        return Some(text.to_owned());
    }
    if text.len() >= 2 && text.starts_with('"') && text.ends_with('"') {
        return parse_words(text).ok()?.into_iter().next();
    }
    None
}

/// How close two numbers must be to be equal.
const EQUAL_WITHIN: f64 = 1e-4;

fn compare(left: &str, comparison: Comparison, right: &str) -> Result<bool, String> {
    let numbers = (left.trim().parse::<f64>(), right.trim().parse::<f64>());
    if let (Ok(a), Ok(b)) = numbers {
        let equal = (a - b).abs() <= EQUAL_WITHIN * a.abs().max(b.abs()).max(1.0);
        return Ok(match comparison {
            Comparison::Equal => equal,
            Comparison::NotEqual => !equal,
            Comparison::Less => a < b && !equal,
            Comparison::LessOrEqual => a < b || equal,
            Comparison::Greater => a > b && !equal,
            Comparison::GreaterOrEqual => a > b || equal,
        });
    }
    match comparison {
        Comparison::Equal => Ok(left == right),
        Comparison::NotEqual => Ok(left != right),
        _ => Err(format!("`{left}` and `{right}` are not both numbers")),
    }
}

/// What a script's host does for it: runs its lines, gives its values,
/// takes its frames and tells the world's tick.
pub trait ScriptHost {
    /// Runs a command line; its text, or why it failed.
    ///
    /// # Errors
    ///
    /// The command's error.
    fn run(&mut self, line: &str) -> Result<Ran, String>;

    /// A value's text.
    ///
    /// # Errors
    ///
    /// Why it has none.
    fn value(&mut self, expression: &str) -> Result<String, String>;

    /// Takes the frame `name`: true once it is taken (it is asked again,
    /// at the next poll, until then).
    ///
    /// # Errors
    ///
    /// Why it was not.
    fn frame(&mut self, name: &str) -> Result<bool, String>;

    /// Whether the app has acted on the last line that needed it (asked
    /// at each poll until it has).
    ///
    /// # Errors
    ///
    /// Why it did not.
    fn settled(&mut self) -> Result<bool, String> {
        Ok(true)
    }

    /// The world's tick.
    fn tick(&mut self) -> u64;

    /// The world's state, for a failure's report.
    fn state(&mut self) -> Data;
}

/// What a command line gave back.
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct Ran {
    /// Its text.
    pub text: String,
    /// The app must act on it before the next line (the runner waits a
    /// poll).
    pub settle: bool,
}

/// A line that ran, for the report.
#[derive(Clone, Debug, PartialEq)]
pub struct LineResult {
    /// Its number.
    pub line: usize,
    /// Whether it passed.
    pub ok: bool,
    /// What it gave back, or why it failed.
    pub text: String,
}

/// Why a script stopped before its end.
#[derive(Clone, Debug, PartialEq)]
pub struct Failure {
    /// The line's number.
    pub line: usize,
    /// The line.
    pub text: String,
    /// The expression that did not hold, if it was one.
    pub expression: Option<String>,
    /// Its value, if it had one.
    pub value: Option<String>,
    /// Why.
    pub reason: String,
    /// The world's state when it failed.
    pub state: Data,
}

impl Failure {
    /// The failure as JSON.
    #[must_use]
    pub fn data(&self) -> Data {
        json!({
            "line": self.line,
            "text": self.text,
            "expression": self.expression,
            "value": self.value,
            "reason": self.reason,
            "state": self.state,
        })
    }
}

impl fmt::Display for Failure {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "line {}: {}: {}", self.line, self.text, self.reason)?;
        if let Some(value) = &self.value {
            write!(f, " (its value was {value})")?;
        }
        write!(f, "; state {}", self.state)
    }
}

/// Where a script stands after a poll.
#[derive(Clone, Debug, PartialEq)]
pub enum Progress {
    /// It waits for the world (or the app): poll again after a tick.
    Waiting,
    /// Every line passed.
    Finished,
    /// A line failed; the script stopped there.
    Failed(Failure),
}

#[derive(Clone, Debug, PartialEq, Eq)]
enum Wait {
    Ticks {
        until: u64,
    },
    Condition {
        deadline: u64,
    },
    Frame,
    /// A line's action, which the app takes between polls; its text.
    Settle {
        text: String,
    },
}

/// Runs a script's lines in turn, as far as it can without the world
/// running, at each poll.
#[derive(Clone, Debug)]
pub struct ScriptRunner {
    lines: Vec<ScriptLine>,
    next: usize,
    waiting: Option<Wait>,
    results: Vec<LineResult>,
    frames: Vec<String>,
    done: Option<Progress>,
}

impl ScriptRunner {
    /// A runner of `lines`, at the first.
    #[must_use]
    pub fn new(lines: Vec<ScriptLine>) -> Self {
        Self {
            lines,
            next: 0,
            waiting: None,
            results: Vec::new(),
            frames: Vec::new(),
            done: None,
        }
    }

    /// A runner of `source`.
    ///
    /// # Errors
    ///
    /// The script cannot be read.
    pub fn parse(source: &str) -> Result<Self, ParseError> {
        parse(source).map(Self::new)
    }

    /// The lines that ran, in order (a wait's once it ended).
    #[must_use]
    pub fn results(&self) -> &[LineResult] {
        &self.results
    }

    /// The frames taken, in order.
    #[must_use]
    pub fn frames(&self) -> &[String] {
        &self.frames
    }

    /// The line it is at, if it has not finished.
    #[must_use]
    pub fn current(&self) -> Option<&ScriptLine> {
        self.lines.get(self.next)
    }

    /// Whether it has finished or failed.
    #[must_use]
    pub fn is_done(&self) -> bool {
        self.done.is_some()
    }

    /// Stops the script at the line it is at, for `reason` (its host
    /// cannot go on: no world, too many ticks).
    pub fn abort(&mut self, host: &mut impl ScriptHost, reason: String) -> Progress {
        if let Some(done) = &self.done {
            return done.clone();
        }
        let line = self.lines.get(self.next).cloned().unwrap_or(ScriptLine {
            number: 0,
            text: String::new(),
            directive: Directive::Run(String::new()),
        });
        let progress = Progress::Failed(*self.failed(host, &line, None, None, reason));
        self.done = Some(progress.clone());
        progress
    }

    /// Runs the lines that need no ticks; stops at a wait, the end or a
    /// failure.
    pub fn poll(&mut self, host: &mut impl ScriptHost) -> Progress {
        if let Some(done) = &self.done {
            return done.clone();
        }
        let progress = self.run(host);
        if progress != Progress::Waiting {
            self.done = Some(progress.clone());
        }
        progress
    }

    fn run(&mut self, host: &mut impl ScriptHost) -> Progress {
        loop {
            let Some(line) = self.lines.get(self.next).cloned() else {
                return Progress::Finished;
            };
            if let Some(wait) = self.waiting.clone() {
                match self.resume(host, &line, &wait) {
                    Ok(true) => {}
                    Ok(false) => return Progress::Waiting,
                    Err(failure) => return Progress::Failed(*failure),
                }
                self.waiting = None;
                let text = match wait {
                    Wait::Settle { text } => text,
                    _ => String::new(),
                };
                self.passed(&line, text);
                continue;
            }
            if let Err(failure) = self.start(host, &line) {
                return Progress::Failed(*failure);
            }
            if matches!(self.waiting, Some(Wait::Settle { .. })) {
                // (the app acts on the line before the next poll)
                return Progress::Waiting;
            }
        }
    }

    /// Starts `line`: a line that ends at once passes, else it waits.
    fn start(&mut self, host: &mut impl ScriptHost, line: &ScriptLine) -> Result<(), Box<Failure>> {
        match &line.directive {
            Directive::Run(command) => match host.run(command) {
                Ok(ran) => {
                    if ran.settle {
                        self.waiting = Some(Wait::Settle { text: ran.text });
                    } else {
                        self.passed(line, ran.text);
                    }
                    Ok(())
                }
                Err(reason) => Err(self.failed(host, line, None, None, reason)),
            },
            Directive::Wait(ticks) => {
                self.waiting = Some(Wait::Ticks {
                    until: host.tick().saturating_add(*ticks),
                });
                Ok(())
            }
            Directive::WaitUntil { timeout, .. } => {
                // (it is resumed at once: one that holds already passes)
                self.waiting = Some(Wait::Condition {
                    deadline: host.tick().saturating_add(*timeout),
                });
                Ok(())
            }
            Directive::Expect(expression) => match evaluate(host, expression) {
                Ok((true, value)) => {
                    self.passed(line, value);
                    Ok(())
                }
                Ok((false, value)) => Err(self.failed(
                    host,
                    line,
                    Some(expression),
                    Some(value),
                    "the expression does not hold".into(),
                )),
                Err((value, reason)) => {
                    Err(self.failed(host, line, Some(expression), value, reason))
                }
            },
            Directive::Frame(_) => {
                self.waiting = Some(Wait::Frame);
                Ok(())
            }
        }
    }

    /// Whether `line`'s wait has ended.
    fn resume(
        &mut self,
        host: &mut impl ScriptHost,
        line: &ScriptLine,
        wait: &Wait,
    ) -> Result<bool, Box<Failure>> {
        match (wait, &line.directive) {
            (Wait::Ticks { until }, _) => Ok(host.tick() >= *until),
            (
                Wait::Condition { deadline },
                Directive::WaitUntil {
                    expression,
                    timeout,
                },
            ) => match evaluate(host, expression) {
                Ok((true, _)) => Ok(true),
                Ok((false, value)) if host.tick() >= *deadline => Err(self.failed(
                    host,
                    line,
                    Some(expression),
                    Some(value),
                    format!("the expression did not hold in {timeout} ticks"),
                )),
                Ok((false, _)) => Ok(false),
                Err((value, reason)) => {
                    Err(self.failed(host, line, Some(expression), value, reason))
                }
            },
            (Wait::Frame, Directive::Frame(name)) => match host.frame(name) {
                Ok(true) => {
                    self.frames.push(name.clone());
                    Ok(true)
                }
                Ok(false) => Ok(false),
                Err(reason) => Err(self.failed(host, line, None, None, reason)),
            },
            (Wait::Settle { .. }, _) => match host.settled() {
                Ok(settled) => Ok(settled),
                Err(reason) => Err(self.failed(host, line, None, None, reason)),
            },
            _ => Ok(true),
        }
    }

    fn passed(&mut self, line: &ScriptLine, text: String) {
        self.results.push(LineResult {
            line: line.number,
            ok: true,
            text,
        });
        self.next += 1;
    }

    fn failed(
        &mut self,
        host: &mut impl ScriptHost,
        line: &ScriptLine,
        expression: Option<&str>,
        value: Option<String>,
        reason: String,
    ) -> Box<Failure> {
        self.results.push(LineResult {
            line: line.number,
            ok: false,
            text: reason.clone(),
        });
        Box::new(Failure {
            line: line.number,
            text: line.text.clone(),
            expression: expression.map(str::to_owned),
            value,
            reason,
            state: host.state(),
        })
    }
}

/// Whether `expression` holds, and the value it had (the left's, for a
/// comparison); or the value, if any, and why it could not be told.
fn evaluate(
    host: &mut impl ScriptHost,
    expression: &str,
) -> Result<(bool, String), (Option<String>, String)> {
    if let Some((left, comparison, right)) = split_comparison(expression) {
        let a = host.value(left).map_err(|reason| (None, reason))?;
        let b = match literal(right) {
            Some(literal) => literal,
            None => host
                .value(right)
                .map_err(|reason| (Some(a.clone()), reason))?,
        };
        let holds = compare(&a, comparison, &b).map_err(|reason| (Some(a.clone()), reason))?;
        return Ok((holds, a));
    }
    let value = host.value(expression).map_err(|reason| (None, reason))?;
    match holds(&value) {
        Some(holds) => Ok((holds, value)),
        None => Err((
            Some(value.clone()),
            format!("`{value}` is not true or false"),
        )),
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::collections::BTreeMap;

    /// A world of named values whose tick the test runs.
    #[derive(Default)]
    struct World {
        tick: u64,
        values: BTreeMap<String, String>,
        ran: Vec<String>,
        frames: Vec<String>,
        /// Polls a frame takes before it is taken.
        frame_delay: u32,
        settle: bool,
        /// What `settled` answers, in turn (then true).
        settled: Vec<Result<bool, String>>,
    }

    impl ScriptHost for World {
        fn run(&mut self, line: &str) -> Result<Ran, String> {
            if line.starts_with("bad") {
                return Err(format!("no command {line}"));
            }
            if let Some(rest) = line.strip_prefix("set ") {
                let (name, value) = rest.split_once(' ').ok_or("set what")?;
                self.values.insert(name.into(), value.into());
            }
            self.ran.push(line.into());
            Ok(Ran {
                text: format!("ran {line}"),
                settle: self.settle && line.starts_with("map"),
            })
        }

        fn value(&mut self, expression: &str) -> Result<String, String> {
            if expression == "tick" {
                return Ok(self.tick.to_string());
            }
            self.values
                .get(expression)
                .cloned()
                .ok_or_else(|| format!("no value {expression}"))
        }

        fn frame(&mut self, name: &str) -> Result<bool, String> {
            if name == "broken" {
                return Err("no level to draw".into());
            }
            if self.frame_delay > 0 {
                self.frame_delay -= 1;
                return Ok(false);
            }
            self.frames.push(name.into());
            Ok(true)
        }

        fn settled(&mut self) -> Result<bool, String> {
            if self.settled.is_empty() {
                Ok(true)
            } else {
                self.settled.remove(0)
            }
        }

        fn tick(&mut self) -> u64 {
            self.tick
        }

        fn state(&mut self) -> Data {
            json!({ "tick": self.tick })
        }
    }

    /// Polls until the script ends, ticking between polls; the ticks run.
    fn run(runner: &mut ScriptRunner, world: &mut World) -> (Progress, u64) {
        let start = world.tick;
        for _ in 0..100_000 {
            match runner.poll(world) {
                Progress::Waiting => world.tick += 1,
                done => return (done, world.tick - start),
            }
        }
        (Progress::Waiting, world.tick - start)
    }

    fn runner(source: &str) -> ScriptRunner {
        match ScriptRunner::parse(source) {
            Ok(runner) => runner,
            Err(error) => panic!("{error}"),
        }
    }

    #[test]
    fn the_four_directives_are_read() {
        let lines = parse(
            "# a test\n\nwait 30\nwait_until (> (ai_living_count x) 0) 90\n\
             wait_until tick >= 5\nexpect noclip\nframe start-1\nspawn warthog\n",
        );
        let directives: Vec<Directive> = match lines {
            Ok(lines) => lines.into_iter().map(|line| line.directive).collect(),
            Err(error) => panic!("{error}"),
        };
        assert_eq!(
            directives,
            vec![
                Directive::Wait(30),
                Directive::WaitUntil {
                    expression: "(> (ai_living_count x) 0)".into(),
                    timeout: 90
                },
                // (the 5 is what tick is compared with, not a timeout)
                Directive::WaitUntil {
                    expression: "tick >= 5".into(),
                    timeout: DEFAULT_TIMEOUT
                },
                Directive::Expect("noclip".into()),
                Directive::Frame("start-1".into()),
                Directive::Run("spawn warthog".into()),
            ]
        );
        assert_eq!(parse("\n\nwait 5").map(|lines| lines[0].number), Ok(3));
    }

    #[test]
    fn badly_written_directives_name_their_line() {
        for (source, line) in [
            ("wait", 1),
            ("step\nwait -3", 2),
            ("wait 0", 1),
            ("wait 999999999", 1),
            ("wait_until", 1),
            ("wait_until x 0", 1),
            ("expect", 1),
            ("frame ../escape", 1),
            ("frame", 1),
        ] {
            assert_eq!(
                parse(source).map_err(|error| error.line),
                Err(line),
                "{source}"
            );
        }
        let long = "#".repeat(MAX_SCRIPT_BYTES + 1);
        assert_eq!(parse(&long).map_err(|error| error.line), Err(0));
    }

    #[test]
    fn wait_lets_the_world_run() {
        let mut world = World::default();
        let mut script = runner("step\nwait 30\nstep\nwait 2");
        let (progress, ticks) = run(&mut script, &mut world);
        assert_eq!(progress, Progress::Finished);
        assert_eq!(ticks, 32);
        assert_eq!(world.ran, ["step", "step"]);
        assert_eq!(script.results().len(), 4);
        assert!(script.results().iter().all(|result| result.ok));
        assert!(script.is_done());
        assert_eq!(script.poll(&mut world), Progress::Finished);
    }

    #[test]
    fn wait_until_runs_until_the_expression_holds() {
        let mut world = World::default();
        let mut script = runner("wait_until tick >= 12\nexpect tick == 12\nwait_until tick > 3");
        let (progress, ticks) = run(&mut script, &mut world);
        assert_eq!(progress, Progress::Finished);
        // (one that holds already passes without a tick)
        assert_eq!(ticks, 12);
    }

    #[test]
    fn wait_until_fails_at_its_timeout() {
        let mut world = World::default();
        world.values.insert("door_open".into(), "false".into());
        let mut script = runner("step\nwait_until door_open 40\nstep");
        let (progress, ticks) = run(&mut script, &mut world);
        let Progress::Failed(failure) = progress else {
            panic!("{progress:?}");
        };
        assert_eq!(ticks, 40);
        assert_eq!(failure.line, 2);
        assert_eq!(failure.expression.as_deref(), Some("door_open"));
        assert_eq!(failure.value.as_deref(), Some("false"));
        assert!(failure.reason.contains("40 ticks"), "{}", failure.reason);
        assert_eq!(failure.state, json!({ "tick": 40 }));
        assert_eq!(world.ran, ["step"]);
    }

    #[test]
    fn a_failed_expect_reports_its_line_expression_and_state() {
        let mut world = World::default();
        world.values.insert("health".into(), "0.25".into());
        let mut script = runner("wait 3\n\n# the player is hurt\nexpect health > 0.5\nstep");
        let (progress, _) = run(&mut script, &mut world);
        let Progress::Failed(failure) = progress else {
            panic!("{progress:?}");
        };
        assert_eq!(failure.line, 4);
        assert_eq!(failure.text, "expect health > 0.5");
        assert_eq!(failure.expression.as_deref(), Some("health > 0.5"));
        assert_eq!(failure.value.as_deref(), Some("0.25"));
        assert_eq!(failure.state, json!({ "tick": 3 }));
        let shown = failure.to_string();
        assert!(shown.contains("line 4") && shown.contains("0.25") && shown.contains("tick"));
        assert_eq!(failure.data()["line"], 4);
        assert!(world.ran.is_empty(), "nothing after the failure runs");
        assert_eq!(script.results().last().map(|result| result.ok), Some(false));
    }

    #[test]
    fn expressions_compare_and_hold() {
        let mut world = World::default();
        world.values.insert("name".into(), "warthog".into());
        world.values.insert("speed".into(), "2.00001".into());
        world.values.insert("two".into(), "2".into());
        world.values.insert("on".into(), "on".into());
        world.values.insert("word".into(), "maybe".into());
        for (expression, expected) in [
            ("name == \"warthog\"", Ok(true)),
            ("name != warthog_alias", Err(())), // (an unknown value on the right)
            ("speed == 2", Ok(true)),
            ("speed == two", Ok(true)),
            ("speed <= 2", Ok(true)),
            ("speed < 2", Ok(false)),
            ("speed >= 2", Ok(true)),
            ("speed > 1", Ok(true)),
            ("speed != 3", Ok(true)),
            ("on", Ok(true)),
            ("two", Ok(true)),
            ("name < 3", Err(())),
            ("word", Err(())),
            ("missing", Err(())),
            ("missing == 3", Err(())),
        ] {
            let got = evaluate(&mut world, expression)
                .map(|(holds, _)| holds)
                .map_err(|_| ());
            assert_eq!(got, expected, "{expression}");
        }
        assert_eq!(holds("none"), Some(false));
        assert_eq!(holds("0.0"), Some(false));
        assert_eq!(holds("-1"), Some(true));
        // (a comparison inside HaloScript's parentheses is not split)
        assert_eq!(split_comparison("(= a b)"), None);
        assert_eq!(split_comparison("\"a == b\""), None);
        assert!(split_comparison("(f x) == 3").is_some());
    }

    #[test]
    fn a_failed_command_stops_the_script() {
        let mut world = World::default();
        let mut script = runner("step\nbad command\nstep");
        let (progress, _) = run(&mut script, &mut world);
        let Progress::Failed(failure) = progress else {
            panic!("{progress:?}");
        };
        assert_eq!(failure.line, 2);
        assert_eq!(failure.expression, None);
        assert!(failure.reason.contains("no command"));
        assert_eq!(world.ran, ["step"]);
    }

    #[test]
    fn frames_are_taken_when_the_host_can() {
        let mut world = World {
            frame_delay: 2,
            ..World::default()
        };
        let mut script = runner("frame first\nframe second");
        let (progress, ticks) = run(&mut script, &mut world);
        assert_eq!(progress, Progress::Finished);
        assert_eq!(ticks, 2, "the first frame waited two polls");
        assert_eq!(script.frames(), ["first", "second"]);
        assert_eq!(world.frames, ["first", "second"]);

        let mut script = runner("frame broken\nstep");
        let (progress, _) = run(&mut script, &mut world);
        assert!(
            matches!(&progress, Progress::Failed(failure) if failure.reason.contains("no level")),
            "{progress:?}"
        );
    }

    #[test]
    fn a_line_the_app_acts_on_waits_a_poll() {
        let mut world = World {
            settle: true,
            ..World::default()
        };
        let mut script = runner("map bloodgulch\nstep");
        assert_eq!(script.poll(&mut world), Progress::Waiting);
        assert_eq!(world.ran, ["map bloodgulch"]);
        assert_eq!(script.current().map(|line| line.number), Some(1));
        assert_eq!(script.poll(&mut world), Progress::Finished);
        assert_eq!(world.ran, ["map bloodgulch", "step"]);
        assert_eq!(script.results().len(), 2);
    }

    #[test]
    fn a_line_waits_until_the_app_has_acted_on_it() {
        let mut world = World {
            settle: true,
            settled: vec![Ok(false), Ok(false), Err("the app went".into())],
            ..World::default()
        };
        let mut script = runner("map a\nstep");
        assert_eq!(script.poll(&mut world), Progress::Waiting);
        assert_eq!(script.poll(&mut world), Progress::Waiting);
        assert_eq!(script.poll(&mut world), Progress::Waiting);
        let progress = script.poll(&mut world);
        assert!(
            matches!(&progress, Progress::Failed(failure) if failure.line == 1),
            "{progress:?}"
        );
        assert_eq!(world.ran, ["map a"]);
    }

    #[test]
    fn a_script_is_aborted_at_its_line() {
        let mut world = World::default();
        let mut script = runner("step\nwait 10\nstep");
        assert_eq!(script.poll(&mut world), Progress::Waiting);
        let progress = script.abort(&mut world, "no world".into());
        let Progress::Failed(failure) = &progress else {
            panic!("{progress:?}");
        };
        assert_eq!((failure.line, failure.reason.as_str()), (2, "no world"));
        assert_eq!(script.poll(&mut world), progress);
        assert_eq!(script.abort(&mut world, "again".into()), progress);

        let mut finished = runner("step");
        assert_eq!(finished.poll(&mut world), Progress::Finished);
        assert_eq!(
            finished.abort(&mut world, "late".into()),
            Progress::Finished
        );
        let mut empty = runner("");
        assert!(matches!(empty.abort(&mut world, "x".into()), Progress::Failed(f) if f.line == 0));
    }
}
