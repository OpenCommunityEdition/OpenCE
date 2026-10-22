//! The engine's log, kept for the shell and the control socket: the `log`
//! crate's records from Rust and the lines Swift adds, the last
//! [`CAPACITY`] of them, each numbered so that a reader can ask for what
//! came after the last it saw. A filter (`render=debug,warn`) chooses what
//! is kept.

use std::collections::VecDeque;
use std::sync::{Mutex, MutexGuard, OnceLock, PoisonError};

pub use log::Level;

/// The lines kept.
pub const CAPACITY: usize = 2000;

/// The longest line kept, in bytes (a longer one is cut).
pub const MAX_LINE: usize = 4096;

/// A line of the log.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct LogLine {
    /// Its number: one more than the line before's.
    pub sequence: u64,
    /// Its level.
    pub level: Level,
    /// What wrote it: a module path, or `swift`, `render` and the like.
    pub target: String,
    /// Its text.
    pub text: String,
}

/// Which lines are kept: the level for targets named (the longest
/// matching prefix wins), and the level for the rest.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Filter {
    default: log::LevelFilter,
    targets: Vec<(String, log::LevelFilter)>,
}

impl Default for Filter {
    fn default() -> Self {
        Self {
            default: log::LevelFilter::Info,
            targets: Vec::new(),
        }
    }
}

impl Filter {
    /// Reads `render=debug,sim=warn,info`: targets and their levels, and a
    /// bare level for the rest. Levels are off, error, warn, info, debug
    /// and trace.
    pub fn parse(spec: &str) -> Result<Self, String> {
        let mut filter = Self::default();
        for part in spec
            .split(',')
            .map(str::trim)
            .filter(|part| !part.is_empty())
        {
            let (target, level) = match part.split_once('=') {
                Some((target, level)) => (Some(target.trim()), level.trim()),
                None => (None, part),
            };
            let level: log::LevelFilter = level.parse().map_err(|_| {
                format!("`{level}` is not a level (off, error, warn, info, debug, trace)")
            })?;
            match target {
                Some("") => return Err(format!("`{part}` names no target")),
                Some(target) => filter.targets.push((target.to_owned(), level)),
                None => filter.default = level,
            }
        }
        Ok(filter)
    }

    /// Whether a line of `level` from `target` is kept.
    pub fn allows(&self, level: Level, target: &str) -> bool {
        let chosen = self
            .targets
            .iter()
            .filter(|(prefix, _)| target.starts_with(prefix.as_str()))
            .max_by_key(|(prefix, _)| prefix.len())
            .map_or(self.default, |(_, level)| *level);
        level <= chosen
    }
}

impl std::fmt::Display for Filter {
    fn fmt(&self, formatter: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        for (target, level) in &self.targets {
            write!(formatter, "{target}={},", level.as_str().to_lowercase())?;
        }
        write!(formatter, "{}", self.default.as_str().to_lowercase())
    }
}

struct Book {
    lines: VecDeque<LogLine>,
    next: u64,
    filter: Filter,
}

fn book() -> MutexGuard<'static, Book> {
    static BOOK: OnceLock<Mutex<Book>> = OnceLock::new();
    BOOK.get_or_init(|| {
        Mutex::new(Book {
            lines: VecDeque::with_capacity(CAPACITY),
            next: 1,
            filter: Filter::default(),
        })
    })
    .lock()
    .unwrap_or_else(PoisonError::into_inner)
}

/// Adds a line, if the filter keeps it; returns its number, or `None`.
pub fn record(level: Level, target: &str, text: &str) -> Option<u64> {
    let mut book = book();
    if !book.filter.allows(level, target) {
        return None;
    }
    let mut text = text.to_owned();
    if text.len() > MAX_LINE {
        let mut end = MAX_LINE;
        while !text.is_char_boundary(end) {
            end -= 1;
        }
        text.truncate(end);
    }
    let sequence = book.next;
    book.next += 1;
    if book.lines.len() == CAPACITY {
        book.lines.pop_front();
    }
    book.lines.push_back(LogLine {
        sequence,
        level,
        target: target.to_owned(),
        text,
    });
    Some(sequence)
}

/// The lines after `sequence` (0 for all that are kept), at most `max`.
pub fn since(sequence: u64, max: usize) -> Vec<LogLine> {
    let book = book();
    book.lines
        .iter()
        .filter(|line| line.sequence > sequence)
        .take(max)
        .cloned()
        .collect()
}

/// The number the next line will have.
pub fn next_sequence() -> u64 {
    book().next
}

/// Sets which lines are kept.
pub fn set_filter(filter: Filter) {
    book().filter = filter;
}

/// The filter in use.
pub fn filter() -> Filter {
    book().filter.clone()
}

struct Logger;

impl log::Log for Logger {
    fn enabled(&self, metadata: &log::Metadata<'_>) -> bool {
        book().filter.allows(metadata.level(), metadata.target())
    }

    fn log(&self, entry: &log::Record<'_>) {
        record(entry.level(), entry.target(), &entry.args().to_string());
    }

    fn flush(&self) {}
}

/// Makes the log book the `log` crate's logger (once; false if another
/// logger was set first).
pub fn install() -> bool {
    static LOGGER: Logger = Logger;
    let installed = log::set_logger(&LOGGER).is_ok();
    if installed {
        log::set_max_level(log::LevelFilter::Trace);
    }
    installed
}
