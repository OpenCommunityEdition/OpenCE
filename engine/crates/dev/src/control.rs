//! The control socket's side of things that both ends share: where the
//! socket is, how big and how deep a request may be, how often requests may
//! come, and the private folder the socket is made in. The engine answers
//! the requests (the ffi crate); the `opence` tool sends them.

use std::fs;
use std::io;
use std::os::unix::fs::{FileTypeExt, PermissionsExt};
use std::os::unix::net::UnixListener;
use std::path::{Path, PathBuf};
use std::time::Instant;

use serde_json::{Map, Value};

/// The longest request line, in bytes.
pub const MAX_REQUEST: usize = 64 * 1024;

/// The deepest a request's JSON may nest.
pub const MAX_DEPTH: usize = 16;

/// Requests a second a connection may send, on average.
pub const REQUESTS_PER_SECOND: f64 = 200.0;

/// Requests a connection may send at once before the rate holds it back.
pub const REQUEST_BURST: f64 = 400.0;

/// Connections served at once.
pub const MAX_CONNECTIONS: usize = 8;

/// The socket's name inside its folder.
pub const SOCKET_NAME: &str = "control.sock";

/// The socket's path: `OPENCE_CONTROL_SOCKET` if it is set, else
/// `control.sock` in `opence` in the user's runtime folder
/// (`XDG_RUNTIME_DIR`, else `TMPDIR`, which macOS makes for each user).
pub fn default_socket_path() -> PathBuf {
    if let Some(path) = std::env::var_os("OPENCE_CONTROL_SOCKET") {
        return PathBuf::from(path);
    }
    let runtime = std::env::var_os("XDG_RUNTIME_DIR")
        .or_else(|| std::env::var_os("TMPDIR"))
        .map_or_else(std::env::temp_dir, PathBuf::from);
    runtime.join("opence").join(SOCKET_NAME)
}

/// Listens at `path`, made so that only the user can reach it: its folder
/// is made readable by the user alone (an existing folder that others can
/// read or enter is refused), a stale socket there is removed, and the
/// socket is made readable and writable by the user alone.
pub fn bind_private(path: &Path) -> io::Result<UnixListener> {
    let folder = path
        .parent()
        .filter(|folder| !folder.as_os_str().is_empty())
        .ok_or_else(|| io::Error::new(io::ErrorKind::InvalidInput, "the socket needs a folder"))?;
    if !folder.exists() {
        fs::create_dir_all(folder)?;
        fs::set_permissions(folder, fs::Permissions::from_mode(0o700))?;
    }
    let mode = fs::metadata(folder)?.permissions().mode();
    if mode & 0o077 != 0 {
        return Err(io::Error::new(
            io::ErrorKind::PermissionDenied,
            format!(
                "{} can be reached by others (mode {:o})",
                folder.display(),
                mode & 0o777
            ),
        ));
    }
    match fs::symlink_metadata(path) {
        Ok(existing) if existing.file_type().is_socket() => fs::remove_file(path)?,
        Ok(_) => {
            return Err(io::Error::new(
                io::ErrorKind::AlreadyExists,
                format!("{} is there and is not a socket", path.display()),
            ))
        }
        Err(_) => {}
    }
    let listener = UnixListener::bind(path)?;
    fs::set_permissions(path, fs::Permissions::from_mode(0o600))?;
    Ok(listener)
}

/// Why a request was refused before it was answered.
#[derive(Debug, Clone, PartialEq, Eq, thiserror::Error)]
pub enum RequestError {
    /// Longer than [`MAX_REQUEST`].
    #[error("the request is longer than {MAX_REQUEST} bytes")]
    TooLong,
    /// Nested deeper than [`MAX_DEPTH`].
    #[error("the request nests deeper than {MAX_DEPTH}")]
    TooDeep,
    /// Not JSON, or not an object or a string.
    #[error("the request is not a JSON object or string: {0}")]
    NotJson(String),
    /// Sent faster than the rate allows.
    #[error("too many requests: at most {REQUESTS_PER_SECOND} a second")]
    TooFrequent,
}

/// A request, as its JSON line gives it: a command line (a bare JSON
/// string, or `{"command": "..."}`) or a structured request
/// (`{"request": "state"}` and the like, with its other fields).
#[derive(Debug, Clone, PartialEq)]
pub enum Request {
    /// A line of the shell.
    Command(String),
    /// A structured request: its name and its fields.
    Structured(String, Map<String, Value>),
}

/// Reads a request line, checking its size and nesting before parsing it.
pub fn parse_request(line: &str) -> Result<Request, RequestError> {
    if line.len() > MAX_REQUEST {
        return Err(RequestError::TooLong);
    }
    if depth(line) > MAX_DEPTH {
        return Err(RequestError::TooDeep);
    }
    let value: Value =
        serde_json::from_str(line).map_err(|error| RequestError::NotJson(error.to_string()))?;
    match value {
        Value::String(command) => Ok(Request::Command(command)),
        Value::Object(mut fields) => {
            if let Some(Value::String(command)) = fields.remove("command") {
                return Ok(Request::Command(command));
            }
            match fields.remove("request") {
                Some(Value::String(name)) => Ok(Request::Structured(name, fields)),
                _ => Err(RequestError::NotJson(
                    "an object needs \"command\" or \"request\"".into(),
                )),
            }
        }
        _ => Err(RequestError::NotJson("not an object or a string".into())),
    }
}

/// How deep `text`'s brackets nest, outside its strings.
fn depth(text: &str) -> usize {
    let (mut deepest, mut level) = (0usize, 0usize);
    let (mut quoted, mut escaped) = (false, false);
    for byte in text.bytes() {
        if quoted {
            match (escaped, byte) {
                (true, _) => escaped = false,
                (false, b'\\') => escaped = true,
                (false, b'"') => quoted = false,
                _ => {}
            }
            continue;
        }
        match byte {
            b'"' => quoted = true,
            b'[' | b'{' => {
                level += 1;
                deepest = deepest.max(level);
            }
            b']' | b'}' => level = level.saturating_sub(1),
            _ => {}
        }
    }
    deepest
}

/// A reply line: `{"ok": true, ...fields}` or `{"ok": false, "error": ...}`.
pub fn reply(ok: bool, mut fields: Map<String, Value>) -> String {
    fields.insert("ok".into(), Value::Bool(ok));
    Value::Object(fields).to_string()
}

/// A refusal's reply line.
pub fn refusal(error: &str) -> String {
    let mut fields = Map::new();
    fields.insert("error".into(), Value::String(error.to_owned()));
    reply(false, fields)
}

/// A connection's rate: a bucket of [`REQUEST_BURST`] requests, refilled at
/// [`REQUESTS_PER_SECOND`].
#[derive(Debug, Clone)]
pub struct RateLimit {
    tokens: f64,
    rate: f64,
    burst: f64,
    last: Option<Instant>,
}

impl Default for RateLimit {
    fn default() -> Self {
        Self::new(REQUESTS_PER_SECOND, REQUEST_BURST)
    }
}

impl RateLimit {
    /// A bucket of `burst`, refilled at `rate` a second.
    pub fn new(rate: f64, burst: f64) -> Self {
        Self {
            tokens: burst,
            rate,
            burst,
            last: None,
        }
    }

    /// Takes a request at `now`: false if the bucket is empty.
    pub fn allow(&mut self, now: Instant) -> bool {
        if let Some(last) = self.last {
            let seconds = now.saturating_duration_since(last).as_secs_f64();
            self.tokens = (self.tokens + seconds * self.rate).min(self.burst);
        }
        self.last = Some(now);
        if self.tokens >= 1.0 {
            self.tokens -= 1.0;
            true
        } else {
            false
        }
    }
}
