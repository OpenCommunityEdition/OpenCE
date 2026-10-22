//! `opence`: the developer shell of a running engine (the app with
//! `--headless`, or with `--control` beside its window), over its control
//! socket.
//!
//! ```text
//! opence [--socket PATH] run [--json] LINE...   a shell line
//! opence [--socket PATH] state                  the world's state, as JSON
//! opence [--socket PATH] frame PATH [--size WxH] the view, drawn into a PNG
//! opence [--socket PATH] watch [--backlog N]    the log, as it is written
//! opence [--socket PATH] script FILE            a script's lines, stopping at a failure
//! opence [--socket PATH] shell                  lines typed (completed by the engine), or read
//! ```
//!
//! It exits 0 when everything succeeded, 1 when the engine refused
//! something, and 2 when the engine could not be reached or the arguments
//! were wrong.

use std::io::{self, BufReader, IsTerminal, Write};
use std::path::PathBuf;
use std::process::ExitCode;

use opence_cli::{
    complete, frame, parse_size, run, script, shell_lines, state, watch, Connection, Error,
};
use rustyline::completion::{Completer, Pair};
use rustyline::error::ReadlineError;
use rustyline::{Context, Editor, Helper, Highlighter, Hinter, Validator};

const USAGE: &str = "usage: opence [--socket PATH] (run [--json] LINE... | state | frame PATH [--size WxH] | watch [--backlog N] | script FILE | shell)";

fn main() -> ExitCode {
    let arguments: Vec<String> = std::env::args().skip(1).collect();
    match start(&arguments) {
        Ok(()) => ExitCode::SUCCESS,
        Err(Error::Refused(reason)) => {
            eprintln!("opence: {reason}");
            ExitCode::from(1)
        }
        Err(Error::Usage(text)) => {
            eprintln!("opence: {text}\n{USAGE}");
            ExitCode::from(2)
        }
        Err(error) => {
            eprintln!("opence: {error}");
            ExitCode::from(2)
        }
    }
}

fn start(arguments: &[String]) -> Result<(), Error> {
    let mut arguments = arguments.to_vec();
    let mut socket = opence_dev::control::default_socket_path();
    if let Some(index) = arguments.iter().position(|word| word == "--socket") {
        let path = arguments
            .get(index + 1)
            .ok_or_else(|| Error::Usage("--socket needs a path".into()))?;
        socket = PathBuf::from(path);
        arguments.drain(index..=index + 1);
    }
    let (subcommand, rest) = arguments
        .split_first()
        .ok_or_else(|| Error::Usage("no subcommand".into()))?;
    let mut out = io::stdout().lock();
    match subcommand.as_str() {
        "run" => {
            let as_json = rest.first().is_some_and(|word| word == "--json");
            let words = if as_json { &rest[1..] } else { rest };
            if words.is_empty() {
                return Err(Error::Usage("run needs a line".into()));
            }
            run(
                &mut Connection::connect(&socket)?,
                &words.join(" "),
                as_json,
                &mut out,
            )
        }
        "state" => state(&mut Connection::connect(&socket)?, &mut out),
        "frame" => {
            let path = rest
                .first()
                .ok_or_else(|| Error::Usage("frame needs a PNG's path".into()))?;
            let size = match rest.get(1).map(String::as_str) {
                None => None,
                Some("--size") => Some(parse_size(
                    rest.get(2)
                        .ok_or_else(|| Error::Usage("--size needs WxH".into()))?,
                )?),
                Some(other) => return Err(Error::Usage(format!("frame does not take {other}"))),
            };
            frame(
                &mut Connection::connect(&socket)?,
                path.as_ref(),
                size,
                &mut out,
            )
            .map(|_| ())
        }
        "watch" => {
            let backlog = match rest {
                [] => 0,
                [flag, count] if flag == "--backlog" => count
                    .parse()
                    .map_err(|_| Error::Usage(format!("--backlog is a count, not {count}")))?,
                _ => return Err(Error::Usage("watch takes only --backlog N".into())),
            };
            watch(&mut Connection::connect(&socket)?, backlog, None, &mut out)
        }
        "script" => {
            let file = rest
                .first()
                .ok_or_else(|| Error::Usage("script needs a file".into()))?;
            let text = std::fs::read_to_string(file)?;
            script(&mut Connection::connect(&socket)?, &text, &mut out)
        }
        "shell" => {
            let mut connection = Connection::connect(&socket)?;
            if io::stdin().is_terminal() {
                drop(out);
                interactive(connection)
            } else {
                let refused = shell_lines(&mut connection, BufReader::new(io::stdin()), &mut out)?;
                match refused {
                    0 => Ok(()),
                    count => Err(Error::Refused(format!("{count} lines were refused"))),
                }
            }
        }
        other => Err(Error::Usage(format!("no subcommand {other}"))),
    }
}

/// The interactive shell: lines edited and remembered (in
/// `~/.opence_history`), Tab completing from the engine's registry.
fn interactive(connection: Connection) -> Result<(), Error> {
    let readline = |error: ReadlineError| Error::Io(io::Error::other(error.to_string()));
    let mut editor: Editor<Engine, rustyline::history::FileHistory> =
        Editor::new().map_err(readline)?;
    editor.set_helper(Some(Engine {
        connection: std::cell::RefCell::new(connection),
    }));
    let history = std::env::var_os("HOME").map(|home| PathBuf::from(home).join(".opence_history"));
    if let Some(history) = &history {
        let _ = editor.load_history(history);
    }
    loop {
        let line = match editor.readline("] ") {
            Ok(line) => line,
            Err(ReadlineError::Interrupted | ReadlineError::Eof) => break,
            Err(error) => return Err(readline(error)),
        };
        let line = line.trim();
        if line.is_empty() {
            continue;
        }
        let _ = editor.add_history_entry(line);
        let Some(helper) = editor.helper() else { break };
        let mut connection = helper.connection.borrow_mut();
        let mut out = io::stdout().lock();
        match run(&mut connection, line, false, &mut out) {
            Ok(()) => {}
            Err(Error::Refused(reason)) => writeln!(out, "error: {reason}")?,
            Err(error) => return Err(error),
        }
        if line == "quit" {
            break;
        }
    }
    if let Some(history) = &history {
        let _ = editor.save_history(history);
    }
    Ok(())
}

/// Completion from the engine.
#[derive(Helper, Highlighter, Hinter, Validator)]
struct Engine {
    connection: std::cell::RefCell<Connection>,
}

impl Completer for Engine {
    type Candidate = Pair;

    fn complete(
        &self,
        line: &str,
        position: usize,
        _context: &Context<'_>,
    ) -> rustyline::Result<(usize, Vec<Pair>)> {
        let before = &line[..position];
        let start = before.rfind([' ', ';']).map_or(0, |space| space + 1);
        let words = complete(&mut self.connection.borrow_mut(), before).unwrap_or_default();
        let pairs = words
            .into_iter()
            .map(|word| Pair {
                display: word.clone(),
                replacement: word,
            })
            .collect();
        Ok((start, pairs))
    }
}
