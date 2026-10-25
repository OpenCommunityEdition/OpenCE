//! The player on bloodgulch: spawned at the scenario's start, moved by the
//! tick's input, seen from its eyes or from behind. Passes without
//! assets/maps/bloodgulch.map.

use std::error::Error;

use opence_content::Level;
use opence_import::testing::stock_level;
use opence_sim::input::{Buttons, TickInput};
use opence_sim::player::{View, FOLLOW_DISTANCE};
use opence_sim::units::Radians;
use opence_sim::{Simulation, Vec3};

fn bloodgulch() -> Result<Option<Level>, Box<dyn Error>> {
    let Some(stock) = stock_level("bloodgulch")? else {
        return Ok(None); // (no game data here)
    };
    Ok(Some(Level::clone(&stock)))
}

fn run(simulation: &mut Simulation, input: TickInput, ticks: usize) {
    for _ in 0..ticks {
        simulation.step(input);
    }
}

/// The camera is at the biped's eyes: its feet and its camera height.
fn assert_at_the_eyes(simulation: &Simulation) {
    let Some(player) = simulation.player() else {
        panic!("no player");
    };
    let mover = player.mover();
    let eyes = mover.position + Vec3::Z * mover.camera_height();
    let camera = simulation.camera().transform.position;
    assert!((camera - eyes).length() < 1e-5, "{camera} against {eyes}");
}

#[test]
fn the_player_starts_at_the_scenarios_start() -> Result<(), Box<dyn Error>> {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let start = &level.scenario.player_starts[0];
    let mut simulation = Simulation::empty();
    let handle = simulation.spawn_player(&level, 0)?;
    // (settled onto the ground beneath the start)
    run(&mut simulation, TickInput::default(), 60);
    let body = simulation.transform(handle).ok_or("no biped")?;
    let at = Vec3::from_array(start.position);
    assert!(
        (body.position.truncate() - at.truncate()).length() < 0.01,
        "{}",
        body.position
    );
    assert!((body.position.z - at.z).abs() < 0.5, "{}", body.position);
    let player = simulation.player().ok_or("no player")?;
    assert!(!player.mover().airborne);
    assert!((player.look().0 .0 - start.facing).abs() < 1e-5);
    assert_at_the_eyes(&simulation);
    assert!(simulation.spawn_player(&level, usize::MAX).is_err());
    Ok(())
}

#[test]
fn the_first_person_camera_follows_the_biped() -> Result<(), Box<dyn Error>> {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let mut simulation = Simulation::empty();
    let handle = simulation.spawn_player(&level, 0)?;
    run(&mut simulation, TickInput::default(), 30);
    let from = simulation.transform(handle).ok_or("no biped")?.position;
    // a quarter turn to the left, then a second's run
    simulation.step(TickInput {
        yaw: Radians(std::f32::consts::FRAC_PI_2),
        ..TickInput::default()
    });
    let facing = simulation.player().ok_or("no player")?.look().0 .0;
    let forward = TickInput {
        forward: 127,
        ..TickInput::default()
    };
    for _ in 0..60 {
        simulation.step(forward);
        assert_at_the_eyes(&simulation);
    }
    let moved = simulation.transform(handle).ok_or("no biped")?.position - from;
    let along = Vec3::new(libm::cosf(facing), libm::sinf(facing), 0.0);
    let speed = level.globals.player.run_forward;
    assert!(moved.dot(along) > 0.7 * speed, "{moved} along {along}");
    assert!(
        moved.truncate().perp_dot(along.truncate()).abs() < 0.05,
        "{moved}"
    );
    // (the camera looks the biped's way)
    let look = simulation.camera().transform.rotation * Vec3::X;
    assert!((look - along).length() < 1e-4, "{look}");
    // its own body is not drawn from its eyes
    let mut drawn = Vec::new();
    simulation.visit_drawn(|object| drawn.push(object.id.0));
    assert!(!drawn.contains(&handle.index));
    // the jump leaves the ground; the crouch lowers the eyes
    simulation.step(TickInput {
        buttons: Buttons::JUMP,
        ..TickInput::default()
    });
    run(&mut simulation, TickInput::default(), 3);
    assert!(simulation.player().ok_or("no player")?.mover().airborne);
    run(&mut simulation, TickInput::default(), 90);
    let standing = simulation.camera().transform.position.z;
    run(
        &mut simulation,
        TickInput {
            buttons: Buttons::CROUCH,
            ..TickInput::default()
        },
        30,
    );
    assert!(simulation.camera().transform.position.z < standing - 0.1);
    assert_at_the_eyes(&simulation);
    Ok(())
}

#[test]
fn the_third_person_camera_follows_from_behind() -> Result<(), Box<dyn Error>> {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let mut simulation = Simulation::empty();
    let handle = simulation.spawn_player(&level, 0)?;
    run(&mut simulation, TickInput::default(), 30);
    assert!(simulation.set_player_view(View::ThirdPerson));
    let player = simulation.player().ok_or("no player")?;
    let mover = player.mover();
    let eyes = mover.position + Vec3::Z * mover.camera_height();
    let camera = simulation.camera().transform;
    let behind = eyes - camera.position;
    let forward = camera.rotation * Vec3::X;
    assert!(behind.dot(forward) > 0.0, "{behind}");
    assert!(behind.length() <= FOLLOW_DISTANCE + 0.3, "{behind}");
    let mut drawn = Vec::new();
    simulation.visit_drawn(|object| drawn.push(object.id.0));
    assert!(drawn.contains(&handle.index));
    // looking down from behind, the camera stays above the ground it
    // would otherwise sink into
    simulation.step(TickInput {
        pitch: Radians(-1.4),
        ..TickInput::default()
    });
    let low = simulation.camera().transform.position;
    let ground = simulation.player().ok_or("no player")?.mover().position;
    assert!(low.z > ground.z, "{low} over {ground}");
    assert!(simulation.set_player_view(View::FirstPerson));
    assert_at_the_eyes(&simulation);
    Ok(())
}

#[test]
fn a_players_replay_gives_the_same_hash() -> Result<(), Box<dyn Error>> {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let inputs: Vec<TickInput> = (0..600)
        .map(|tick| TickInput {
            buttons: if tick % 97 == 0 {
                Buttons::JUMP
            } else {
                Buttons::default()
            },
            forward: if tick % 200 < 150 { 127 } else { -127 },
            right: if tick % 300 < 100 { 127 } else { 0 },
            up: 0,
            yaw: Radians(0.01 * libm::sinf(tick as f32 * 0.05)),
            pitch: Radians(0.002),
        })
        .collect();
    let hash = |inputs: &[TickInput]| -> Result<u64, Box<dyn Error>> {
        let mut simulation = Simulation::empty();
        simulation.spawn_player(&level, 0)?;
        for &input in inputs {
            simulation.step(input);
        }
        Ok(simulation.world_hash())
    };
    let first = hash(&inputs)?;
    assert_eq!(hash(&inputs)?, first);
    let mut changed = inputs.clone();
    changed[300].forward = 0;
    assert_ne!(hash(&changed)?, first);
    Ok(())
}
