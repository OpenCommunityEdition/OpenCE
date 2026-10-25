//! The engine's log, kept for the shell and the control socket: the
//! `tracing` events from Rust (and the `log` records of the dependencies,
//! through tracing-log) and the lines Swift adds, the last [`CAPACITY`] of
//! them, each numbered so that a reader can ask for what came after the
//! last it saw. A filter (`render=debug,warn`) chooses what is kept.
//!
//! [`install`] makes the book the process's subscriber, with a JSON log of
//! every span and event in a file when `OPENCE_LOG_JSON` names one, and
//! with the `tracy` feature, Tracy's.

use std::collections::VecDeque;
use std::fmt::Write as _;
use std::sync::{Mutex, MutexGuard, OnceLock, PoisonError};

use tracing::field::{Field, Visit};
use tracing::subscriber::Interest;
use tracing::{Event, Metadata, Subscriber};
use tracing_subscriber::fmt::MakeWriter;
use tracing_subscriber::layer::{Context, Filter as LayerFilter, Layer, SubscriberExt};
use tracing_subscriber::registry::LookupSpan;
use tracing_subscriber::util::SubscriberInitExt;

/// A line's level, from the most to the least severe.
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub enum Level {
    /// Something failed.
    Error,
    /// Something is wrong but went on.
    Warn,
    /// What happened.
    Info,
    /// More of what happened.
    Debug,
    /// Everything.
    Trace,
}

impl Level {
    /// Its name, upper case (`INFO`).
    pub fn as_str(self) -> &'static str {
        match self {
            Self::Error => "ERROR",
            Self::Warn => "WARN",
            Self::Info => "INFO",
            Self::Debug => "DEBUG",
            Self::Trace => "TRACE",
        }
    }
}

impl From<tracing::Level> for Level {
    fn from(level: tracing::Level) -> Self {
        match level {
            tracing::Level::ERROR => Self::Error,
            tracing::Level::WARN => Self::Warn,
            tracing::Level::INFO => Self::Info,
            tracing::Level::DEBUG => Self::Debug,
            _ => Self::Trace,
        }
    }
}

impl std::str::FromStr for Level {
    type Err = String;

    fn from_str(name: &str) -> Result<Self, Self::Err> {
        match parse_threshold(name)? {
            Some(level) => Ok(level),
            None => Err("`off` is not a line's level".into()),
        }
    }
}

/// The least severe level kept, or none (`off`).
type Threshold = Option<Level>;

fn parse_threshold(name: &str) -> Result<Threshold, String> {
    Ok(Some(match name.to_ascii_lowercase().as_str() {
        "off" => return Ok(None),
        "error" => Level::Error,
        "warn" => Level::Warn,
        "info" => Level::Info,
        "debug" => Level::Debug,
        "trace" => Level::Trace,
        _ => {
            return Err(format!(
                "`{name}` is not a level (off, error, warn, info, debug, trace)"
            ))
        }
    }))
}

fn threshold_name(threshold: Threshold) -> String {
    threshold.map_or_else(|| "off".into(), |level| level.as_str().to_lowercase())
}

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
    default: Threshold,
    targets: Vec<(String, Threshold)>,
}

impl Default for Filter {
    fn default() -> Self {
        Self {
            default: Some(Level::Info),
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
            let level = parse_threshold(level)?;
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
        chosen.is_some_and(|chosen| level <= chosen)
    }
}

impl std::fmt::Display for Filter {
    fn fmt(&self, formatter: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        for (target, level) in &self.targets {
            write!(formatter, "{target}={},", threshold_name(*level))?;
        }
        write!(formatter, "{}", threshold_name(self.default))
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

/// An event's text: its message, then its other fields as `name=value`.
#[derive(Default)]
struct Text(String);

impl Text {
    fn separate(&mut self) {
        if !self.0.is_empty() {
            self.0.push(' ');
        }
    }
}

impl Visit for Text {
    fn record_str(&mut self, field: &Field, value: &str) {
        self.separate();
        if field.name() == "message" {
            self.0.push_str(value);
        } else {
            let _ = write!(self.0, "{}={value}", field.name());
        }
    }

    fn record_debug(&mut self, field: &Field, value: &dyn std::fmt::Debug) {
        self.separate();
        if field.name() == "message" {
            let _ = write!(self.0, "{value:?}");
        } else {
            let _ = write!(self.0, "{}={value:?}", field.name());
        }
    }
}

/// The layer that writes events into the book.
struct BookLayer;

impl<S: Subscriber> Layer<S> for BookLayer {
    fn on_event(&self, event: &Event<'_>, _context: Context<'_, S>) {
        let mut text = Text::default();
        event.record(&mut text);
        let metadata = event.metadata();
        record((*metadata.level()).into(), metadata.target(), &text.0);
    }
}

/// The book's filter: events its [`Filter`] keeps, and no spans (so that
/// with nothing else listening a span costs nothing).
struct BookFilter;

impl<S> LayerFilter<S> for BookFilter {
    fn enabled(&self, metadata: &Metadata<'_>, _context: &Context<'_, S>) -> bool {
        metadata.is_event()
            && book()
                .filter
                .allows((*metadata.level()).into(), metadata.target())
    }

    fn callsite_enabled(&self, metadata: &'static Metadata<'static>) -> Interest {
        if metadata.is_event() {
            Interest::sometimes()
        } else {
            Interest::never()
        }
    }
}

/// The layer that keeps events in the book, as its filter chooses.
pub fn layer<S>() -> impl Layer<S>
where
    S: Subscriber + for<'a> LookupSpan<'a>,
{
    BookLayer.with_filter(BookFilter)
}

/// A layer that writes every span's close and every event to `writer` as
/// JSON, an object a line, each event with the spans it was in.
pub fn json_layer<S, W>(writer: W) -> impl Layer<S>
where
    S: Subscriber + for<'a> LookupSpan<'a>,
    W: for<'w> MakeWriter<'w> + Send + Sync + 'static,
{
    tracing_subscriber::fmt::layer()
        .json()
        .with_current_span(false)
        .with_span_list(true)
        .with_writer(writer)
}

/// Makes the book the process's `tracing` subscriber (once; false if
/// another was set first), with the JSON log when `OPENCE_LOG_JSON` names
/// a file it can make, and Tracy's layer with the `tracy` feature.
pub fn install() -> bool {
    let json = std::env::var_os("OPENCE_LOG_JSON")
        .and_then(|path| std::fs::File::create(path).ok())
        .map(|file| json_layer(Mutex::new(file)));
    let subscriber = tracing_subscriber::registry().with(layer()).with(json);
    #[cfg(feature = "tracy")]
    let subscriber = subscriber.with(tracing_tracy::TracyLayer::default());
    subscriber.try_init().is_ok()
}
