//! The scripts' sounds on *The Pillar of Autumn* and *Halo*: every sound the
//! scripts start or wait on has its length, and a wait on one lasts what
//! the original's `scripted_sound_time` counts, its longest permutation in
//! game ticks; the music the scripts start loops until they stop it
//! (skipped where there is no game data).

use std::error::Error;
use std::sync::Arc;

use opence_campaign::Mission;
use opence_content::{Constant, Level, NodeKind, TagRef};
use opence_import::testing::stock_level;
use opence_script::{Value, TICKS_PER_SECOND as GAME_TICKS_PER_SECOND};
use opence_sim::clock::TICKS_PER_SECOND;
use opence_sim::input::TickInput;
use opence_sim::objects::Difficulty;
use opence_sim::Simulation;

const MAPS: [&str; 2] = ["a10", "a30"];
/// The simulation's ticks in a game tick (the game's 30 a second).
const SIM_TICKS_PER_GAME_TICK: u32 = TICKS_PER_SECOND / GAME_TICKS_PER_SECOND;

fn level(map: &str) -> Result<Option<Arc<Level>>, Box<dyn Error>> {
    let Some(stock) = stock_level(map)? else {
        return Ok(None); // (no game data here)
    };
    Ok(Some(stock))
}

/// The sounds the scripts call `function` with, as its first argument.
fn sounds_given(level: &Level, function: &str) -> Vec<TagRef> {
    let scripts = &level.scenario.scripts;
    let mut found: Vec<TagRef> = Vec::new();
    for node in &scripts.nodes {
        let NodeKind::Call(call) = &node.kind else {
            continue;
        };
        if call.function != function {
            continue;
        }
        let first = call
            .arguments
            .first()
            .and_then(|&at| scripts.nodes.get(at as usize));
        if let Some(NodeKind::Constant(_, Constant::Tag(Some(tag)))) = first.map(|n| &n.kind) {
            if !found.contains(tag) {
                found.push(tag.clone());
            }
        }
    }
    found
}

/// The length the original waits a sound out for, in game ticks (its
/// longest permutation's milliseconds, as `scripted_sound_time` counts).
fn ticks_of(level: &Level, tag: &TagRef) -> Option<u64> {
    let length = level
        .scenario
        .campaign
        .sound_lengths
        .iter()
        .find(|sound| sound.path.eq_ignore_ascii_case(&tag.path))?;
    Some(u64::from(length.milliseconds) * u64::from(GAME_TICKS_PER_SECOND) / 1000)
}

/// Runs a game tick of `mission`.
fn game_tick(mission: &mut Mission) {
    for _ in 0..SIM_TICKS_PER_GAME_TICK {
        mission.step(TickInput::default());
    }
}

fn time_left(mission: &mut Mission, tag: &TagRef) -> Option<Value> {
    mission.call("sound_impulse_time", &[Value::Tag(Some(tag.clone()))])
}

#[test]
fn every_sound_the_scripts_start_or_wait_on_has_its_length() -> Result<(), Box<dyn Error>> {
    for map in MAPS {
        let Some(level) = level(map)? else {
            continue;
        };
        let started = sounds_given(&level, "sound_impulse_start");
        assert!(!started.is_empty(), "{map}: the scripts start no sounds");
        for tag in started
            .iter()
            .chain(sounds_given(&level, "sound_impulse_time").iter())
        {
            let ticks = ticks_of(&level, tag);
            assert!(
                ticks.is_some_and(|t| t > 0),
                "{map}: {} has no length",
                tag.path
            );
        }
    }
    Ok(())
}

#[test]
fn a_wait_on_a_sound_lasts_its_longest_permutation() -> Result<(), Box<dyn Error>> {
    for map in MAPS {
        let Some(level) = level(map)? else {
            continue;
        };
        let mut mission = Mission::new(Simulation::empty(), level.clone(), Difficulty::Normal);
        // (the longest and the shortest of the sounds the scripts wait on)
        let mut waited = sounds_given(&level, "sound_impulse_time");
        waited.sort_by_key(|tag| ticks_of(&level, tag));
        let ends = [waited.first(), waited.last()];
        for tag in ends.into_iter().flatten() {
            let Some(ticks) = ticks_of(&level, tag) else {
                continue;
            };
            mission.call(
                "sound_impulse_start",
                &[
                    Value::Tag(Some(tag.clone())),
                    Value::Index(-1),
                    Value::Real(1.0),
                ],
            );
            let left = i32::try_from(ticks)?;
            assert_eq!(
                time_left(&mut mission, tag),
                Some(Value::Long(left)),
                "{map}"
            );
            for _ in 1..ticks {
                game_tick(&mut mission);
            }
            assert_eq!(time_left(&mut mission, tag), Some(Value::Long(1)), "{map}");
            game_tick(&mut mission);
            assert_eq!(time_left(&mut mission, tag), Some(Value::Long(0)), "{map}");
            let orders = mission.take_sounds();
            assert!(orders.started.iter().any(|s| s.tag == *tag), "{map}");
        }
    }
    Ok(())
}

#[test]
fn the_music_loops_until_the_scripts_stop_it() -> Result<(), Box<dyn Error>> {
    for map in MAPS {
        let Some(level) = level(map)? else {
            continue;
        };
        let mut mission = Mission::new(Simulation::empty(), level.clone(), Difficulty::Normal);
        let music = sounds_given(&level, "sound_looping_start");
        let Some(tag) = music.first() else {
            panic!("{map}: the scripts start no music");
        };
        let given = Value::Tag(Some(tag.clone()));
        mission.call(
            "sound_looping_start",
            &[given.clone(), Value::Index(-1), Value::Real(1.0)],
        );
        for _ in 0..TICKS_PER_SECOND * 60 {
            mission.step(TickInput::default());
        }
        let playing: Vec<_> = mission
            .loops()
            .map(|(_, l)| (l.tag.clone(), l.alternate))
            .collect();
        assert_eq!(playing, vec![(tag.clone(), false)], "{map}");
        mission.call(
            "sound_looping_set_alternate",
            &[given.clone(), Value::Boolean(true)],
        );
        assert!(mission.loops().all(|(_, l)| l.alternate), "{map}");
        mission.call("sound_looping_stop", &[given]);
        assert_eq!(mission.loops().count(), 0, "{map}");
    }
    Ok(())
}
