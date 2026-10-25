//! A tick of a10's mission with every encounter placed: the AI's actors,
//! the simulation and the scripts (`cargo bench -p opence_campaign`;
//! nothing without assets/maps).

use std::sync::Arc;

use divan::Bencher;
use opence_campaign::{AiReference, Mission};
use opence_import::testing::stock_level_unheld;
use opence_sim::input::TickInput;
use opence_sim::objects::Difficulty;
use opence_sim::Simulation;

fn main() {
    divan::main();
}

/// a10's mission at legendary with every encounter placed, if it is here.
fn mission() -> Option<Mission> {
    let level = Arc::new(stock_level_unheld("a10").ok()??);
    let encounters = level.scenario.campaign.encounters.len();
    let mut sim = Simulation::empty();
    sim.spawn_player(&level, 0).ok()?;
    let mut mission = Mission::new(sim, level, Difficulty::Legendary);
    for encounter in 0..encounters {
        mission.place_ai(AiReference::Encounter(u16::try_from(encounter).ok()?));
    }
    Some(mission)
}

#[divan::bench(sample_count = 20)]
fn a_missions_tick(bencher: Bencher) {
    let Some(mut mission) = mission() else {
        return;
    };
    bencher.bench_local(|| mission.step(TickInput::default()));
}
