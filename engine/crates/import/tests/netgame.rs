//! bloodgulch's multiplayer data: a flag stand for each team, Slayer's and
//! CTF's player starts, and netgame equipment whose items are imported.
//! Passes without assets/maps/bloodgulch.map.

use std::error::Error;
use std::path::Path;

use opence_content::{GameType, Level, NetgameFlagKind};
use opence_import::{import_level, Cache};

fn bloodgulch() -> Result<Option<Level>, Box<dyn Error>> {
    let path = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../assets/maps/bloodgulch.map");
    let Ok(file) = std::fs::read(&path) else {
        return Ok(None); // (no game data here)
    };
    Ok(Some(import_level(&Cache::open(file)?)?))
}

#[test]
fn bloodgulch_has_its_flags_starts_and_equipment() -> Result<(), Box<dyn Error>> {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let scenario = &level.scenario;
    let mut stands: Vec<u16> = scenario
        .netgame_flags
        .iter()
        .filter(|flag| flag.kind == NetgameFlagKind::CtfFlag)
        .map(|flag| flag.team)
        .collect();
    stands.sort_unstable();
    assert_eq!(stands, [0, 1]);

    for game in [GameType::Slayer, GameType::CaptureTheFlag] {
        let starts = scenario
            .player_starts
            .iter()
            .filter(|start| start.game_types.matches(game))
            .count();
        assert!(starts >= 2, "{game:?} has {starts} starts");
    }

    let slayer: Vec<_> = scenario
        .netgame_equipment
        .iter()
        .filter(|equipment| equipment.game_types.matches(GameType::Slayer))
        .collect();
    assert!(slayer.len() >= 10, "{} items for Slayer", slayer.len());
    for equipment in &slayer {
        assert!(!equipment.items.is_empty());
        for choice in &equipment.items {
            assert!(
                level.objects.iter().any(|object| object.tag == choice.item),
                "{} is not imported",
                choice.item.path
            );
        }
    }
    Ok(())
}
