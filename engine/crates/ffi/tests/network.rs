//! Two engines on this machine: one hosts Slayer on bloodgulch, the other
//! finds it, joins it by its address and plays; the host sees both
//! players. Passes without assets/maps/bloodgulch.map.

use std::path::Path;
use std::sync::Arc;

use opence_ffi::network::{GameKind, NetworkStatus, ServerSearch};
use opence_ffi::{Engine, LevelGeometry};

type Result = std::result::Result<(), Box<dyn std::error::Error>>;

fn bloodgulch() -> Option<Arc<LevelGeometry>> {
    let map = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../assets/maps/bloodgulch.map");
    if !map.exists() {
        return None; // (no game data here)
    }
    let cache = Path::new(env!("CARGO_TARGET_TMPDIR")).join("network_cache");
    std::fs::create_dir_all(&cache).ok()?;
    LevelGeometry::load(
        map.to_string_lossy().into_owned(),
        cache.to_string_lossy().into_owned(),
        0,
    )
    .ok()
}

#[test]
fn an_engine_finds_joins_and_plays_anothers_game() -> Result {
    let Some(level) = bloodgulch() else {
        return Ok(());
    };
    let host = Engine::new();
    let port = host.host_game(level.clone(), GameKind::Slayer, 0, "Gulch".into(), 8)?;
    assert!(port != 0);
    assert_eq!(
        host.network_status(),
        NetworkStatus::Hosting { port, players: 1 }
    );

    let search = ServerSearch::asking(vec![format!("127.0.0.1:{port}")])?;
    let mut found = Vec::new();
    for _ in 0..120 {
        host.step()?;
        found = search.servers();
        if !found.is_empty() {
            break;
        }
        std::thread::yield_now();
    }
    let [server] = found.as_slice() else {
        return Err(format!("found {found:?}").into());
    };
    assert_eq!(
        (server.name.as_str(), server.level.as_str()),
        ("Gulch", "bloodgulch")
    );
    assert_eq!((server.players, server.capacity), (1, 8));

    let client = Engine::new();
    client.join_game(level, server.address.clone(), "Spartan".into())?;
    assert_eq!(client.network_status(), NetworkStatus::Waiting);
    for _ in 0..120 {
        client.step()?;
        host.step()?;
    }
    assert_eq!(
        client.network_status(),
        NetworkStatus::Playing { player: 1 }
    );
    assert_eq!(
        host.network_status(),
        NetworkStatus::Hosting { port, players: 2 }
    );
    assert!(client.playing());
    client.leave_game();
    assert_eq!(client.network_status(), NetworkStatus::Alone);
    Ok(())
}

#[test]
fn a_bad_address_is_refused() {
    assert!(ServerSearch::asking(vec!["127.0.0.1:notaport".into()]).is_err());
}
