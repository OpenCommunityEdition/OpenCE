//! The engine's `tracing` spans and events: the profiler's scopes are
//! spans that nest, the JSON log writes one object a line that parses with
//! the spans an event was in, and events reach the log book.

use std::error::Error;
use std::io;
use std::sync::{Arc, Mutex, PoisonError};

use opence_dev::logbook::{self, Level};
use opence_dev::profile_scope;
use serde_json::Value;
use tracing_subscriber::fmt::MakeWriter;
use tracing_subscriber::layer::SubscriberExt;

/// A writer into a buffer the test reads afterwards.
#[derive(Clone, Default)]
struct Buffer(Arc<Mutex<Vec<u8>>>);

impl Buffer {
    fn lines(&self) -> Result<Vec<Value>, serde_json::Error> {
        let bytes = self.0.lock().unwrap_or_else(PoisonError::into_inner);
        String::from_utf8_lossy(&bytes)
            .lines()
            .map(serde_json::from_str)
            .collect()
    }
}

impl io::Write for Buffer {
    fn write(&mut self, bytes: &[u8]) -> io::Result<usize> {
        self.0
            .lock()
            .unwrap_or_else(PoisonError::into_inner)
            .extend_from_slice(bytes);
        Ok(bytes.len())
    }

    fn flush(&mut self) -> io::Result<()> {
        Ok(())
    }
}

impl<'a> MakeWriter<'a> for Buffer {
    type Writer = Buffer;

    fn make_writer(&'a self) -> Self::Writer {
        self.clone()
    }
}

fn names(spans: &Value) -> Vec<&str> {
    spans
        .as_array()
        .map(|spans| {
            spans
                .iter()
                .filter_map(|span| span["name"].as_str())
                .collect()
        })
        .unwrap_or_default()
}

#[test]
fn scopes_nest_as_spans_and_the_json_log_parses() -> Result<(), Box<dyn Error>> {
    let buffer = Buffer::default();
    let subscriber = tracing_subscriber::registry().with(logbook::json_layer(buffer.clone()));
    tracing::subscriber::with_default(subscriber, || {
        profile_scope!("advance");
        {
            profile_scope!("tick");
            tracing::info!(target: "sim", ticks = 2, "a tick");
        }
        tracing::warn!(target: "render", "after the tick");
    });
    let lines = buffer.lines()?;
    assert_eq!(lines.len(), 2, "{lines:?}");
    assert_eq!(lines[0]["target"], "sim");
    assert_eq!(lines[0]["level"], "INFO");
    assert_eq!(lines[0]["fields"]["message"], "a tick");
    assert_eq!(lines[0]["fields"]["ticks"], 2);
    assert_eq!(names(&lines[0]["spans"]), ["advance", "tick"]);
    assert_eq!(lines[1]["level"], "WARN");
    assert_eq!(names(&lines[1]["spans"]), ["advance"]);
    Ok(())
}

#[test]
fn a_scope_named_at_run_time_is_timed_but_not_a_span() -> Result<(), Box<dyn Error>> {
    let buffer = Buffer::default();
    let subscriber = tracing_subscriber::registry().with(logbook::json_layer(buffer.clone()));
    let name = String::from("from the app");
    tracing::subscriber::with_default(subscriber, || {
        profile_scope!(name.as_str());
        tracing::info!("inside");
    });
    let lines = buffer.lines()?;
    assert_eq!(lines.len(), 1);
    assert!(names(&lines[0]["spans"]).is_empty());
    Ok(())
}

#[test]
fn events_reach_the_log_book_through_its_layer() -> Result<(), Box<dyn Error>> {
    let subscriber = tracing_subscriber::registry().with(logbook::layer());
    let start = logbook::next_sequence() - 1;
    tracing::subscriber::with_default(subscriber, || {
        profile_scope!("frame");
        tracing::warn!(target: "render::shadows", cascade = 3, "too far");
        tracing::trace!(target: "render", "not kept at info");
    });
    let lines: Vec<_> = logbook::since(start, 100)
        .into_iter()
        .filter(|line| line.target.starts_with("render"))
        .collect();
    assert_eq!(lines.len(), 1, "{lines:?}");
    assert_eq!(lines[0].level, Level::Warn);
    assert_eq!(lines[0].text, "too far cascade=3");
    Ok(())
}
