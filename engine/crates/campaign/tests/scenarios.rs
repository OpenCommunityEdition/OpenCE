//! Each campaign scenario's scripts run over its mission for a minute and
//! find every function and global they call (skipped where there is no
//! game data).

use std::error::Error;
use std::sync::Arc;

use opence_campaign::{Mission, MissionScripts};
use opence_import::testing::stock_level_unheld;
use opence_script::TICKS_PER_SECOND;
use opence_sim::input::TickInput;
use opence_sim::objects::Difficulty;
use opence_sim::Simulation;

const MAPS: [&str; 10] = [
    "a10", "a30", "a50", "b30", "b40", "c10", "c20", "c40", "d20", "d40",
];

#[test]
fn every_campaign_scenarios_scripts_find_their_functions() -> Result<(), Box<dyn Error>> {
    for map in MAPS {
        let Some(stock) = stock_level_unheld(map)? else {
            continue; // (no game data here)
        };
        let level = Arc::new(stock);
        let mut sim = Simulation::empty();
        sim.spawn_player(&level, 0)?;
        let mission = Mission::new(sim, level, Difficulty::Normal);
        let mut scripts = MissionScripts::new(mission);
        for _ in 0..60 * opence_sim::clock::TICKS_PER_SECOND {
            scripts.tick(TickInput::default());
        }
        assert!(
            scripts.runtime().missing().is_empty(),
            "{map}: {:?}",
            scripts.runtime().missing()
        );
        assert_eq!(scripts.runtime().time(), 60 * TICKS_PER_SECOND, "{map}");
    }
    Ok(())
}
