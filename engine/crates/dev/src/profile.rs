//! Profiling marks: `profile_scope!("name")` times the rest of its block
//! into the profiler, which keeps the last frame's scopes (for `stat
//! frame`) and, while a trace is taken, every scope of its frames, written
//! as a Chrome trace (JSON `traceEvents`, which Perfetto and
//! `chrome://tracing` read).
//!
//! The profiler is off until something asks for it: a scope then costs one
//! atomic load, and records and allocates nothing.

use std::cell::Cell;
use std::collections::BTreeMap;
use std::fmt::Write as _;
use std::sync::atomic::{AtomicBool, AtomicU64, Ordering};
use std::sync::{Mutex, MutexGuard, OnceLock};
use std::time::Instant;

use serde_json::{json, Value};

/// The most scopes kept of one frame (the rest are counted, not kept).
pub const MAX_FRAME_SPANS: usize = 16 * 1024;

/// The most scopes one trace keeps.
pub const MAX_TRACE_SPANS: usize = 1 << 20;

/// The most frames one trace takes.
pub const MAX_TRACE_FRAMES: u32 = 30 * 60 * 10;

/// A scope that was timed.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Span {
    /// What the scope was.
    pub name: String,
    /// The thread it ran on (the profiler's numbering, from 1).
    pub thread: u64,
    /// How many scopes it was inside of on its thread.
    pub depth: u32,
    /// When it began, in nanoseconds since the profiler was made.
    pub start: u64,
    /// How long it took, in nanoseconds.
    pub duration: u64,
}

impl Span {
    fn end(&self) -> u64 {
        self.start + self.duration
    }
}

/// A scope's line in a frame's summary.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Line {
    /// The scope.
    pub name: String,
    /// How many scopes it is inside of.
    pub depth: u32,
    /// How many times it ran in the frame.
    pub count: u32,
    /// Its time, in nanoseconds.
    pub total: u64,
    /// Its time less its scopes' (directly inside it).
    pub own: u64,
}

#[derive(Debug, Default)]
struct Inner {
    frame: Vec<Span>,
    dropped: usize,
    last: Vec<Span>,
    last_dropped: usize,
    frames: u64,
    trace: Option<Trace>,
}

#[derive(Debug)]
struct Trace {
    spans: Vec<Span>,
    /// The frames left to take.
    left: u32,
    full: bool,
}

/// The profiler: one for the process ([`profiler`]).
#[derive(Debug)]
pub struct Profiler {
    /// Frames are measured (`stat frame`).
    measuring: AtomicBool,
    /// A trace is taken, or frames measured: scopes are recorded.
    on: AtomicBool,
    epoch: Instant,
    threads: AtomicU64,
    inner: Mutex<Inner>,
}

thread_local! {
    static THREAD: Cell<u64> = const { Cell::new(0) };
    static DEPTH: Cell<u32> = const { Cell::new(0) };
    /// The scopes the app opened on this thread ([`Profiler::begin`]).
    static OPEN: std::cell::RefCell<Vec<(String, u32, u64)>> =
        const { std::cell::RefCell::new(Vec::new()) };
}

/// The process's profiler.
pub fn profiler() -> &'static Profiler {
    static PROFILER: OnceLock<Profiler> = OnceLock::new();
    PROFILER.get_or_init(Profiler::new)
}

/// Times the rest of the block as a scope `name` (an expression giving
/// `&str`, read only when the profiler is on). A name written as a literal
/// is also a `tracing` span, for the JSON log and Tracy; one made at run
/// time is only the profiler's, as a span's name must be fixed.
#[macro_export]
macro_rules! profile_scope {
    ($name:literal) => {
        let _profile_scope = $crate::profile::profiler().scope(|| $name);
        let _profile_span = $crate::tracing::trace_span!($name).entered();
    };
    ($name:expr) => {
        let _profile_scope = $crate::profile::profiler().scope(|| $name);
    };
}

/// A scope's guard: it records the scope when dropped.
#[must_use = "a scope is timed until its guard is dropped"]
pub struct Scope<'a> {
    open: Option<(&'a Profiler, String, u32, u64)>,
}

impl Drop for Scope<'_> {
    fn drop(&mut self) {
        if let Some((profiler, name, depth, start)) = self.open.take() {
            DEPTH.with(|cell| cell.set(depth));
            profiler.record(name, depth, start);
        }
    }
}

impl Default for Profiler {
    fn default() -> Self {
        Self::new()
    }
}

impl Profiler {
    /// A profiler that is off (tests make their own; the engine uses
    /// [`profiler`]).
    pub fn new() -> Self {
        Self {
            measuring: AtomicBool::new(false),
            on: AtomicBool::new(false),
            epoch: Instant::now(),
            threads: AtomicU64::new(0),
            inner: Mutex::new(Inner::default()),
        }
    }

    fn lock(&self) -> MutexGuard<'_, Inner> {
        self.inner
            .lock()
            .unwrap_or_else(std::sync::PoisonError::into_inner)
    }

    fn now(&self) -> u64 {
        u64::try_from(self.epoch.elapsed().as_nanos()).unwrap_or(u64::MAX)
    }

    fn thread(&self) -> u64 {
        THREAD.with(|cell| {
            if cell.get() == 0 {
                cell.set(self.threads.fetch_add(1, Ordering::Relaxed) + 1);
            }
            cell.get()
        })
    }

    /// Whether scopes are recorded.
    pub fn on(&self) -> bool {
        self.on.load(Ordering::Relaxed)
    }

    /// Whether frames are measured (`stat frame`).
    pub fn measuring(&self) -> bool {
        self.measuring.load(Ordering::Relaxed)
    }

    /// Measures frames, or stops.
    pub fn measure(&self, measure: bool) {
        self.measuring.store(measure, Ordering::Relaxed);
        let mut inner = self.lock();
        if !measure {
            inner.last.clear();
            inner.last_dropped = 0;
        }
        self.switch(&inner);
    }

    fn switch(&self, inner: &Inner) {
        self.on
            .store(self.measuring() || inner.trace.is_some(), Ordering::Relaxed);
    }

    /// Opens a scope `name` (made only if the profiler is on).
    pub fn scope<S: AsRef<str>>(&self, name: impl FnOnce() -> S) -> Scope<'_> {
        if !self.on() {
            return Scope { open: None };
        }
        let depth = DEPTH.with(|cell| {
            let depth = cell.get();
            cell.set(depth + 1);
            depth
        });
        Scope {
            open: Some((self, name().as_ref().to_owned(), depth, self.now())),
        }
    }

    /// Opens a scope `name` on this thread that [`Profiler::end`] closes
    /// (for the app, across the FFI). Returns false if the profiler is off.
    pub fn begin(&self, name: &str) -> bool {
        if !self.on() {
            return false;
        }
        let depth = DEPTH.with(|cell| {
            let depth = cell.get();
            cell.set(depth + 1);
            depth
        });
        let start = self.now();
        OPEN.with(|open| open.borrow_mut().push((name.to_owned(), depth, start)));
        true
    }

    /// Closes the last scope [`Profiler::begin`] opened on this thread, if
    /// there is one.
    pub fn end(&self) {
        if let Some((name, depth, start)) = OPEN.with(|open| open.borrow_mut().pop()) {
            DEPTH.with(|cell| cell.set(depth));
            self.record(name, depth, start);
        }
    }

    fn record(&self, name: String, depth: u32, start: u64) {
        let span = Span {
            name,
            thread: self.thread(),
            depth,
            start,
            duration: self.now().saturating_sub(start),
        };
        let mut inner = self.lock();
        if let Some(trace) = &mut inner.trace {
            if trace.spans.len() < MAX_TRACE_SPANS {
                trace.spans.push(span.clone());
            } else {
                trace.full = true;
            }
        }
        if self.measuring() {
            if inner.frame.len() < MAX_FRAME_SPANS {
                inner.frame.push(span);
            } else {
                inner.dropped += 1;
            }
        }
    }

    /// Ends a frame: its scopes become the last frame's. Returns the trace
    /// that the frame finished, if one did.
    pub fn frame_end(&self) -> Option<Vec<Span>> {
        if !self.on() {
            return None;
        }
        let mut inner = self.lock();
        inner.frames += 1;
        let frame = std::mem::take(&mut inner.frame);
        inner.last = frame;
        inner.last_dropped = std::mem::take(&mut inner.dropped);
        let finished = match &mut inner.trace {
            Some(trace) => {
                trace.left = trace.left.saturating_sub(1);
                trace.left == 0 || trace.full
            }
            None => false,
        };
        let spans = if finished {
            inner.trace.take().map(|trace| trace.spans)
        } else {
            None
        };
        self.switch(&inner);
        spans
    }

    /// Takes a trace of the next `frames` frames.
    pub fn start_trace(&self, frames: u32) {
        let mut inner = self.lock();
        inner.trace = Some(Trace {
            spans: Vec::new(),
            left: frames.clamp(1, MAX_TRACE_FRAMES),
            full: false,
        });
        self.switch(&inner);
    }

    /// Stops the trace, giving its scopes so far, if one was taken.
    pub fn stop_trace(&self) -> Option<Vec<Span>> {
        let mut inner = self.lock();
        let spans = inner.trace.take().map(|trace| trace.spans);
        self.switch(&inner);
        spans
    }

    /// Whether a trace is taken.
    pub fn tracing(&self) -> bool {
        self.lock().trace.is_some()
    }

    /// The last frame's scopes, in the order they began.
    pub fn last_frame(&self) -> Vec<Span> {
        let mut spans = self.lock().last.clone();
        spans.sort_by_key(|span| (span.thread, span.start, u32::MAX - span.depth));
        spans
    }

    /// The last frame's summary ([`summary`]).
    pub fn last_summary(&self) -> Vec<Line> {
        summary(&self.last_frame())
    }

    /// The last frame as text, for `stat frame`.
    pub fn report(&self) -> String {
        let (dropped, frames) = {
            let inner = self.lock();
            (inner.last_dropped, inner.frames)
        };
        let lines = self.last_summary();
        if lines.is_empty() {
            return if frames == 0 {
                "no frame has been measured yet".into()
            } else {
                "the last frame ran no scopes".into()
            };
        }
        let mut out = String::from("scope                           count     ms   own ms\n");
        for line in &lines {
            let indent = "  ".repeat(line.depth.min(12) as usize);
            let _ = writeln!(
                out,
                "{:<32}{:>5} {:>6.2} {:>8.2}",
                format!("{indent}{}", line.name),
                line.count,
                milliseconds(line.total),
                milliseconds(line.own)
            );
        }
        if dropped > 0 {
            let _ = writeln!(out, "({dropped} more scopes were not kept)");
        }
        out.pop();
        out
    }
}

#[allow(clippy::cast_precision_loss)]
fn milliseconds(nanoseconds: u64) -> f64 {
    nanoseconds as f64 / 1e6
}

/// Sums `spans` (each thread's in the order they began) by their place:
/// a scope's line is the same scope under the same parents, in the order
/// first seen. A line's `own` time is its total less its children's.
pub fn summary(spans: &[Span]) -> Vec<Line> {
    let mut ordered: Vec<&Span> = spans.iter().collect();
    ordered.sort_by_key(|span| (span.thread, span.start, u32::MAX - span.depth));
    let mut lines: Vec<Line> = Vec::new();
    let mut places: BTreeMap<(u64, Vec<String>), usize> = BTreeMap::new();
    // (each thread's open scopes: their end, their line, their path)
    let mut open: Vec<(u64, usize, Vec<String>)> = Vec::new();
    let mut thread = 0;
    for span in ordered {
        if span.thread != thread {
            open.clear();
            thread = span.thread;
        }
        // (a scope inside fewer scopes than are open, or one that ends
        // after an open one, is not inside it: scopes of no length that
        // start together are told apart by their depth)
        while open.len() > span.depth as usize
            || open.last().is_some_and(|(end, ..)| *end < span.end())
        {
            open.pop();
        }
        let mut path = open
            .last()
            .map(|(_, _, path)| path.clone())
            .unwrap_or_default();
        if let Some((_, parent, _)) = open.last() {
            if let Some(parent) = lines.get_mut(*parent) {
                parent.own = parent.own.saturating_sub(span.duration);
            }
        }
        path.push(span.name.clone());
        let depth = u32::try_from(path.len() - 1).unwrap_or(u32::MAX);
        let index = *places
            .entry((span.thread, path.clone()))
            .or_insert_with(|| {
                lines.push(Line {
                    name: span.name.clone(),
                    depth,
                    count: 0,
                    total: 0,
                    own: 0,
                });
                lines.len() - 1
            });
        if let Some(line) = lines.get_mut(index) {
            line.count += 1;
            line.total += span.duration;
            line.own += span.duration;
        }
        open.push((span.end(), index, path));
    }
    // (a line's children were seen after it: list them under it)
    order(&lines)
}

/// The lines with each one's children after it.
fn order(lines: &[Line]) -> Vec<Line> {
    // (lines are made in the order first seen, a parent before its child;
    // a stable walk by depth keeps them so)
    let mut out: Vec<Line> = Vec::with_capacity(lines.len());
    let mut stack: Vec<usize> = Vec::new();
    let mut children: Vec<Vec<usize>> = vec![Vec::new(); lines.len()];
    let mut roots = Vec::new();
    for (index, line) in lines.iter().enumerate() {
        while stack
            .last()
            .and_then(|top| lines.get(*top))
            .is_some_and(|top| top.depth >= line.depth)
        {
            stack.pop();
        }
        match stack.last() {
            Some(parent) => {
                if let Some(list) = children.get_mut(*parent) {
                    list.push(index);
                }
            }
            None => roots.push(index),
        }
        stack.push(index);
    }
    fn walk(index: usize, lines: &[Line], children: &[Vec<usize>], out: &mut Vec<Line>) {
        if let Some(line) = lines.get(index) {
            out.push(line.clone());
        }
        for child in children.get(index).into_iter().flatten() {
            walk(*child, lines, children, out);
        }
    }
    for root in roots {
        walk(root, lines, &children, &mut out);
    }
    out
}

/// `spans` as a Chrome trace: complete events (`ph` `X`) in microseconds.
pub fn chrome_trace(spans: &[Span]) -> Value {
    #[allow(clippy::cast_precision_loss)]
    let micro = |nanoseconds: u64| nanoseconds as f64 / 1e3;
    let events: Vec<Value> = spans
        .iter()
        .map(|span| {
            json!({
                "name": span.name,
                "cat": "opence",
                "ph": "X",
                "ts": micro(span.start),
                "dur": micro(span.duration),
                "pid": 1,
                "tid": span.thread,
            })
        })
        .collect();
    json!({"traceEvents": events, "displayTimeUnit": "ms"})
}

#[cfg(test)]
mod tests {
    use super::*;

    fn span(name: &str, depth: u32, start: u64, duration: u64) -> Span {
        Span {
            name: name.into(),
            thread: 1,
            depth,
            start,
            duration,
        }
    }

    fn busy(nanoseconds: u64) {
        let until = Instant::now() + std::time::Duration::from_nanos(nanoseconds);
        while Instant::now() < until {
            std::hint::spin_loop();
        }
    }

    #[test]
    fn nested_scopes_add_up() {
        let spans = [
            span("frame", 0, 0, 100),
            span("tick", 1, 10, 30),
            span("physics", 2, 15, 10),
            span("tick", 1, 50, 20),
            span("publish", 1, 80, 15),
        ];
        let lines = summary(&spans);
        let names: Vec<(&str, u32, u32, u64, u64)> = lines
            .iter()
            .map(|line| {
                (
                    line.name.as_str(),
                    line.depth,
                    line.count,
                    line.total,
                    line.own,
                )
            })
            .collect();
        assert_eq!(
            names,
            [
                ("frame", 0, 1, 100, 35),
                ("tick", 1, 2, 50, 40),
                ("physics", 2, 1, 10, 10),
                ("publish", 1, 1, 15, 15),
            ]
        );
        // (a line's own time and its children's make its total)
        let children: u64 = lines
            .iter()
            .filter(|line| line.depth == 1)
            .map(|line| line.total)
            .sum();
        assert_eq!(lines[0].own + children, lines[0].total);
    }

    #[test]
    fn scopes_are_timed_into_the_frame() {
        let profiler = Profiler::new();
        {
            let _off = profiler.scope(|| "unseen");
        }
        assert!(profiler.frame_end().is_none());
        assert!(profiler.last_frame().is_empty());
        assert_eq!(profiler.report(), "no frame has been measured yet");

        profiler.measure(true);
        assert!(profiler.on() && profiler.measuring());
        {
            let _outer = profiler.scope(|| "outer");
            busy(200_000);
            {
                let _inner = profiler.scope(|| "inner");
                busy(200_000);
            }
            let _second = profiler.scope(|| String::from("second"));
        }
        assert!(profiler.begin("app"));
        assert!(profiler.begin("app inner"));
        profiler.end();
        profiler.end();
        profiler.end(); // (nothing open)
        profiler.frame_end();
        let spans = profiler.last_frame();
        let names: Vec<(&str, u32)> = spans
            .iter()
            .map(|span| (span.name.as_str(), span.depth))
            .collect();
        assert_eq!(
            names,
            [
                ("outer", 0),
                ("inner", 1),
                ("second", 1),
                ("app", 0),
                ("app inner", 1)
            ]
        );
        let [outer, inner, ..] = spans.as_slice() else {
            panic!("no spans");
        };
        assert!(outer.duration >= inner.duration + 200_000);
        assert!(inner.start >= outer.start && inner.end() <= outer.end());
        let report = profiler.report();
        assert!(
            report.contains("outer") && report.contains("  inner"),
            "{report}"
        );

        profiler.frame_end();
        assert_eq!(profiler.report(), "the last frame ran no scopes");
        profiler.measure(false);
        assert!(!profiler.on());
        assert!(!profiler.begin("off"));
    }

    #[test]
    fn a_trace_takes_its_frames() {
        let profiler = Profiler::new();
        profiler.start_trace(2);
        assert!(profiler.tracing() && profiler.on() && !profiler.measuring());
        for frame in 0..2 {
            {
                let _tick = profiler.scope(|| format!("frame {frame}"));
            }
            let finished = profiler.frame_end();
            if frame == 0 {
                assert!(finished.is_none());
            } else {
                let spans = finished.unwrap_or_default();
                assert_eq!(spans.len(), 2);
                let trace = chrome_trace(&spans);
                let text = trace.to_string();
                let read: Value = serde_json::from_str(&text).unwrap_or_default();
                let events = read["traceEvents"].as_array().cloned().unwrap_or_default();
                assert_eq!(events.len(), 2);
                assert_eq!(events[0]["ph"], "X");
                assert_eq!(events[1]["name"], "frame 1");
                assert!(events[0]["ts"].as_f64().is_some() && events[0]["dur"].as_f64().is_some());
            }
        }
        assert!(!profiler.tracing() && !profiler.on());
        profiler.start_trace(100);
        {
            let _one = profiler.scope(|| "one");
        }
        assert_eq!(profiler.stop_trace().map(|spans| spans.len()), Some(1));
        assert!(profiler.stop_trace().is_none());
    }

    #[test]
    fn threads_are_numbered_apart() {
        let profiler = std::sync::Arc::new(Profiler::new());
        profiler.measure(true);
        {
            let _main = profiler.scope(|| "main");
        }
        let other = std::sync::Arc::clone(&profiler);
        let joined = std::thread::spawn(move || {
            let _other = other.scope(|| "other");
        })
        .join();
        assert!(joined.is_ok());
        profiler.frame_end();
        let spans = profiler.last_frame();
        assert_eq!(spans.len(), 2);
        assert_ne!(spans[0].thread, spans[1].thread);
        // (each thread's scopes are their own roots)
        assert!(profiler.last_summary().iter().all(|line| line.depth == 0));
    }

    #[test]
    fn a_frame_keeps_at_most_its_limit() {
        let profiler = Profiler::new();
        profiler.measure(true);
        for _ in 0..MAX_FRAME_SPANS + 3 {
            let _many = profiler.scope(|| "many");
        }
        profiler.frame_end();
        assert_eq!(profiler.last_frame().len(), MAX_FRAME_SPANS);
        assert!(profiler.report().contains("(3 more scopes were not kept)"));
        assert_eq!(profiler.last_summary()[0].count as usize, MAX_FRAME_SPANS);
    }

    #[test]
    fn the_macro_times_its_block() {
        let profiler = profiler();
        profiler.measure(true);
        {
            crate::profile_scope!("macro");
        }
        profiler.frame_end();
        assert!(profiler
            .last_frame()
            .iter()
            .any(|span| span.name == "macro"));
        profiler.measure(false);
    }
}
