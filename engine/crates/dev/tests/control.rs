//! The control socket's shared rules: requests as JSON lines, refused when
//! too long, too deep or too frequent; the socket made for the user alone;
//! and the log book the shell and `subscribe log` read.

use std::error::Error;
use std::os::unix::fs::PermissionsExt;
use std::time::{Duration, Instant};

use opence_dev::control::{
    bind_private, default_socket_path, parse_request, refusal, reply, RateLimit, Request,
    RequestError, MAX_DEPTH, MAX_REQUEST, REQUEST_BURST, SOCKET_NAME,
};
use opence_dev::logbook::{self, Filter, Level};

#[test]
fn requests_are_json_lines() -> Result<(), Box<dyn Error>> {
    assert_eq!(
        parse_request("\"objects biped\"")?,
        Request::Command("objects biped".into())
    );
    assert_eq!(
        parse_request(r#"{"command": "spawn warthog"}"#)?,
        Request::Command("spawn warthog".into())
    );
    let Request::Structured(name, fields) =
        parse_request(r#"{"request": "frame", "path": "/tmp/a.png", "width": 64}"#)?
    else {
        return Err("not structured".into());
    };
    assert_eq!(name, "frame");
    assert_eq!(fields["width"], 64);
    assert!(!fields.contains_key("request"));
    for line in [
        "",
        "nonsense",
        "[1, 2]",
        "3",
        r#"{"other": 1}"#,
        r#"{"request": 4}"#,
    ] {
        assert!(
            matches!(parse_request(line), Err(RequestError::NotJson(_))),
            "{line}"
        );
    }
    Ok(())
}

#[test]
fn oversized_and_deep_requests_are_refused() -> Result<(), Box<dyn Error>> {
    let long = format!("\"{}\"", "x".repeat(MAX_REQUEST));
    assert_eq!(parse_request(&long), Err(RequestError::TooLong));
    let deep = format!(
        r#"{{"request": "state", "x": {}1{}}}"#,
        "[".repeat(MAX_DEPTH),
        "]".repeat(MAX_DEPTH)
    );
    assert_eq!(parse_request(&deep), Err(RequestError::TooDeep));
    // (brackets in a string are not nesting; an escaped quote stays in it)
    let quoted = format!(r#"{{"command": "say \"{}\""}}"#, "[".repeat(MAX_DEPTH * 2));
    assert!(matches!(parse_request(&quoted)?, Request::Command(_)));
    let fine = format!(
        r#"{{"request": "state", "x": {}1{}}}"#,
        "[".repeat(MAX_DEPTH - 1),
        "]".repeat(MAX_DEPTH - 1)
    );
    assert!(matches!(parse_request(&fine)?, Request::Structured(..)));
    Ok(())
}

#[test]
fn too_frequent_requests_wait_for_the_bucket() {
    let mut rate = RateLimit::default();
    let start = Instant::now();
    let allowed = (0..1000).filter(|_| rate.allow(start)).count();
    assert_eq!(allowed, REQUEST_BURST as usize);
    assert!(!rate.allow(start));
    // (a tenth of a second later, a tenth of the rate is back)
    let later = start + Duration::from_millis(100);
    let refilled = (0..1000).filter(|_| rate.allow(later)).count();
    assert_eq!(refilled, 20);
    let mut slow = RateLimit::new(1.0, 1.0);
    assert!(slow.allow(start));
    assert!(!slow.allow(start + Duration::from_millis(500)));
    assert!(slow.allow(start + Duration::from_millis(1600)));
}

#[test]
fn replies_say_whether_they_succeeded() -> Result<(), Box<dyn Error>> {
    let mut fields = serde_json::Map::new();
    fields.insert("text".into(), "3".into());
    let ok: serde_json::Value = serde_json::from_str(&reply(true, fields))?;
    assert_eq!(ok, serde_json::json!({"ok": true, "text": "3"}));
    let refused: serde_json::Value = serde_json::from_str(&refusal("no"))?;
    assert_eq!(refused, serde_json::json!({"ok": false, "error": "no"}));
    assert_eq!(
        RequestError::TooFrequent.to_string(),
        "too many requests: at most 200 a second"
    );
    Ok(())
}

#[test]
fn the_socket_is_the_users_alone() -> Result<(), Box<dyn Error>> {
    assert!(default_socket_path().ends_with(SOCKET_NAME));
    let root = std::env::temp_dir().join(format!("opence_control_{}", std::process::id()));
    let _ = std::fs::remove_dir_all(&root);
    let path = root.join("private").join(SOCKET_NAME);
    let listener = bind_private(&path)?;
    let folder_mode = std::fs::metadata(root.join("private"))?
        .permissions()
        .mode();
    let socket_mode = std::fs::metadata(&path)?.permissions().mode();
    assert_eq!(folder_mode & 0o777, 0o700);
    assert_eq!(socket_mode & 0o777, 0o600);
    drop(listener);
    // (a stale socket is replaced)
    let _again = bind_private(&path)?;

    // (a folder others can enter, a file that is not a socket, no folder)
    let open = root.join("open");
    std::fs::create_dir_all(&open)?;
    std::fs::set_permissions(&open, std::fs::Permissions::from_mode(0o755))?;
    let error = bind_private(&open.join(SOCKET_NAME)).err().ok_or("bound")?;
    assert_eq!(error.kind(), std::io::ErrorKind::PermissionDenied);
    let file = root.join("private").join("file");
    std::fs::write(&file, b"")?;
    let error = bind_private(&file).err().ok_or("bound")?;
    assert_eq!(error.kind(), std::io::ErrorKind::AlreadyExists);
    assert!(bind_private(std::path::Path::new(SOCKET_NAME)).is_err());
    std::fs::remove_dir_all(&root)?;
    Ok(())
}

#[test]
fn filters_choose_by_target_and_level() -> Result<(), Box<dyn Error>> {
    let filter = Filter::parse("render=debug, render::shadows=error ,warn")?;
    assert!(filter.allows(Level::Debug, "render::level"));
    assert!(!filter.allows(Level::Warn, "render::shadows"));
    assert!(filter.allows(Level::Warn, "sim"));
    assert!(!filter.allows(Level::Info, "sim"));
    assert_eq!(
        filter.to_string(),
        "render=debug,render::shadows=error,warn"
    );
    assert_eq!(Filter::default().to_string(), "info");
    assert!(Filter::parse("render=loud").is_err());
    assert!(Filter::parse("=debug").is_err());
    assert!(Filter::parse("chatty").is_err());
    Ok(())
}

#[test]
fn the_log_book_keeps_numbered_lines() -> Result<(), Box<dyn Error>> {
    // (one test, as the book is the process's)
    let start = logbook::next_sequence() - 1;
    assert!(logbook::install());
    assert!(!logbook::install());
    tracing::info!(target: "sim", "a tick");
    tracing::debug!(target: "sim", "not kept at info");
    let swift = logbook::record(Level::Warn, "swift", "from the app").ok_or("not kept")?;
    let lines = logbook::since(start, 10);
    assert_eq!(
        lines
            .iter()
            .map(|line| (line.target.as_str(), line.text.as_str()))
            .collect::<Vec<_>>(),
        [("sim", "a tick"), ("swift", "from the app")]
    );
    assert_eq!(lines[1].sequence, swift);
    assert_eq!(lines[1].level, Level::Warn);
    assert!(logbook::since(swift, 10).is_empty());

    logbook::set_filter(Filter::parse("sim=trace,error")?);
    assert_eq!(logbook::filter().to_string(), "sim=trace,error");
    assert!(logbook::record(Level::Trace, "sim", "kept").is_some());
    assert!(logbook::record(Level::Warn, "swift", "dropped").is_none());
    logbook::set_filter(Filter::default());

    let long = "é".repeat(logbook::MAX_LINE);
    let cut = logbook::record(Level::Info, "x", &long).ok_or("not kept")?;
    let kept = logbook::since(cut - 1, 1);
    assert!(kept[0].text.len() <= logbook::MAX_LINE);
    for index in 0..logbook::CAPACITY + 5 {
        logbook::record(Level::Info, "flood", &index.to_string());
    }
    let all = logbook::since(0, usize::MAX);
    assert_eq!(all.len(), logbook::CAPACITY);
    assert_eq!(all[0].text, "5");
    assert_eq!(logbook::since(0, 3).len(), 3);
    Ok(())
}
