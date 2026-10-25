//! The Pillar of Autumn's own scripts run, with no functions given them:
//! every function they call is reported, and the threads run on without
//! them (skipped where there is no game data).

use std::error::Error;

use opence_import::{import_scripts, Cache};
use opence_script::{Missing, Registry, Runtime, World, TICKS_PER_SECOND};

struct Nothing;

impl World for Nothing {}

#[test]
fn the_pillar_of_autumns_scripts_run_without_their_functions() -> Result<(), Box<dyn Error>> {
    let path = opence_import::testing::maps_folder().join("a10.map");
    let Ok(file) = std::fs::read(&path) else {
        return Ok(()); // (no game data here)
    };
    let scripts = import_scripts(&Cache::open(file)?)?;
    let mut world = Nothing;
    let mut runtime = Runtime::new(scripts, Registry::new(), &mut world);
    for _ in 0..60 * TICKS_PER_SECOND {
        runtime.tick(&mut world);
    }
    let functions = runtime
        .missing()
        .iter()
        .filter(|m| matches!(m, Missing::Function(_)))
        .count();
    assert!(functions > 50, "{functions} functions missing");
    assert!(!runtime
        .missing()
        .contains(&Missing::Function("begin".into())));
    assert!(!runtime
        .missing()
        .iter()
        .any(|m| matches!(m, Missing::Arguments(_))));
    assert_eq!(runtime.time(), 60 * TICKS_PER_SECOND);
    Ok(())
}
