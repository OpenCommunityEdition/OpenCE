//! The first two missions, *The Pillar of Autumn* and *Halo*, played from
//! their start to their end by a pilot that does what each waiting script
//! waits for: it walks the player into the trigger volumes the scripts
//! test, presses each button and looks each way, and kills the enemies
//! the scripts wait to see dead. The scripts must win the mission, with
//! every function they call given and no thread ended for running away
//! (skipped where there is no game data).

use std::error::Error;
use std::path::Path;
use std::sync::Arc;

use opence_campaign::MissionScripts;
use opence_content::{Constant, Level, NodeKind, Scripts, TriggerVolume};
use opence_import::{import_level, Cache};
use opence_script::Value;
use opence_sim::clock::TICKS_PER_SECOND;
use opence_sim::input::{Buttons, TickInput};
use opence_sim::objects::Difficulty;
use opence_sim::units::Radians;
use opence_sim::Vec3;

/// The longest a mission may take the pilot.
const LONGEST: u32 = 90 * 60 * TICKS_PER_SECOND;
/// The functions that test a trigger volume, its index their first
/// argument.
const VOLUME_TESTS: [&str; 3] = [
    "volume_test_objects",
    "volume_test_objects_all",
    "volume_test_object",
];

fn level(map: &str) -> Result<Option<Arc<Level>>, Box<dyn Error>> {
    let path = Path::new(env!("CARGO_MANIFEST_DIR"))
        .join("../../../assets/maps")
        .join(format!("{map}.map"));
    let Ok(file) = std::fs::read(&path) else {
        return Ok(None); // (no game data here)
    };
    Ok(Some(Arc::new(import_level(&Cache::open(file)?)?)))
}

/// The trigger volumes a condition tests.
fn volumes(scripts: &Scripts, node: u32, found: &mut Vec<usize>) {
    let Some(NodeKind::Call(call)) = scripts.nodes.get(node as usize).map(|n| &n.kind) else {
        return;
    };
    if VOLUME_TESTS.contains(&call.function.as_str()) {
        if let Some(NodeKind::Constant(_, Constant::Named(named))) = call
            .arguments
            .first()
            .and_then(|&a| scripts.nodes.get(a as usize))
            .map(|n| &n.kind)
        {
            if let Ok(index) = usize::try_from(named.index) {
                found.push(index);
            }
        }
    }
    for &argument in &call.arguments {
        volumes(scripts, argument, found);
    }
}

/// A trigger volume's middle.
fn middle(volume: &TriggerVolume) -> Vec3 {
    if !volume.oriented {
        let [x0, x1, y0] = volume.position;
        let [y1, z0, z1] = volume.extents;
        return Vec3::new((x0 + x1) / 2.0, (y0 + y1) / 2.0, (z0 + z1) / 2.0);
    }
    let [f, u] = [Vec3::from(volume.forward), Vec3::from(volume.up)];
    let left = u.cross(f);
    let [a, b, c] = volume.extents;
    Vec3::from(volume.position) + f * (a / 2.0) + left * (b / 2.0) + u * (c / 2.0)
}

/// What the pilot does on `tick`: every button in turn, and a look that
/// sweeps round and up and down.
fn input(tick: u32) -> TickInput {
    let buttons = [
        Buttons::ACTION,
        Buttons::FIRE,
        Buttons::GRENADE,
        Buttons::JUMP,
        Buttons::default(),
    ];
    let sweep = (tick / 15) % 8;
    TickInput {
        buttons: buttons[(tick % 5) as usize],
        forward: 127,
        yaw: Radians(if sweep < 4 { 0.08 } else { -0.08 }),
        pitch: Radians(match sweep % 4 {
            0 => 0.05,
            2 => -0.05,
            _ => 0.0,
        }),
        ..TickInput::default()
    }
}

/// Plays the mission with the pilot until it is won or `LONGEST` passes.
fn pilot(level: Arc<Level>) -> Result<MissionScripts, Box<dyn Error>> {
    let scripts = &level.scenario.scripts;
    let trigger_volumes = &level.scenario.campaign.trigger_volumes;
    let mut mission = MissionScripts::start(Arc::clone(&level), Difficulty::Hard)?;
    let mut turn = 0;
    let centre = mission.mission().sim().rides().centre();
    for tick in 0..LONGEST {
        mission
            .runtime_mut()
            .set_engine_global("cheat_deathless_player", Value::Boolean(true));
        if tick % TICKS_PER_SECOND == 0 {
            let mut targets = Vec::new();
            for condition in mission.runtime().waiting() {
                volumes(scripts, condition, &mut targets);
            }
            targets.dedup();
            if !targets.is_empty() {
                turn += 1;
                let target = targets[turn % targets.len()];
                if let Some(volume) = trigger_volumes.get(target) {
                    mission
                        .mission_mut()
                        .sim_mut()
                        .teleport_player(0, middle(volume) - centre);
                }
            }
            let enemies: Vec<_> = mission
                .mission()
                .actors()
                .iter()
                .filter(|a| a.team > 2)
                .map(|a| a.unit)
                .collect();
            for unit in enemies {
                mission.mission_mut().sim_mut().kill(unit);
            }
        }
        mission.tick(input(tick));
        if mission.mission().flow().won {
            break;
        }
    }
    Ok(mission)
}

fn plays_to_its_end(map: &str) -> Result<(), Box<dyn Error>> {
    let Some(level) = level(map)? else {
        return Ok(());
    };
    let mission = pilot(level)?;
    let runtime = mission.runtime();
    assert!(
        runtime.missing().is_empty(),
        "{map}: {:?}",
        runtime.missing()
    );
    assert!(runtime.faults().is_empty(), "{map}: {:?}", runtime.faults());
    assert!(
        mission.mission().flow().won,
        "{map} was not won in {} s",
        runtime.time() / opence_script::TICKS_PER_SECOND
    );
    Ok(())
}

#[test]
fn the_pillar_of_autumn_plays_to_its_end() -> Result<(), Box<dyn Error>> {
    plays_to_its_end("a10")
}

#[test]
fn halo_plays_to_its_end() -> Result<(), Box<dyn Error>> {
    plays_to_its_end("a30")
}
