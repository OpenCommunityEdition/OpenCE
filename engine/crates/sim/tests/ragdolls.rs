//! Ragdolls on bloodgulch: a cyborg killed by a scripted grenade becomes a
//! ragdoll that follows its pose, then goes loose and is still there a
//! minute later; the corpse cap removes the oldest; strong blasts tear its
//! limbs off. Passes without assets/maps/bloodgulch.map.

use std::error::Error;

use opence_content::{Level, ObjectKind};
use opence_import::testing::stock_level;
use opence_sim::damage::Vitality;
use opence_sim::input::{Buttons, TickInput};
use opence_sim::objects::{ObjectHandle, ObjectSpawn};
use opence_sim::projectile::Detonation;
use opence_sim::ragdoll::DRIVE_TICKS;
use opence_sim::units::Radians;
use opence_sim::{clock, Quat, Simulation, Transform, Vec3};

fn bloodgulch() -> Result<Option<Level>, Box<dyn Error>> {
    let Some(stock) = stock_level("bloodgulch")? else {
        return Ok(None); // (no game data here)
    };
    Ok(Some(Level::clone(&stock)))
}

/// The player settled at the first start, looking across open ground, and
/// the detonation of a frag it threw there (a grenade's explosion to
/// script).
fn settled(level: &Level) -> Result<(Simulation, Detonation), Box<dyn Error>> {
    let mut simulation = Simulation::empty();
    simulation.spawn_player(level, 0)?;
    simulation.step(TickInput {
        yaw: Radians(std::f32::consts::PI),
        ..TickInput::default()
    });
    for _ in 0..90 {
        simulation.step(TickInput::default());
    }
    assert!(simulation.give_grenades(0, 1));
    simulation.step(TickInput {
        buttons: Buttons::GRENADE,
        ..TickInput::default()
    });
    for _ in 0..8 * clock::TICKS_PER_SECOND {
        simulation.step(TickInput::default());
        if let Some(detonation) = simulation.detonations().first() {
            let detonation = detonation.clone();
            return Ok((simulation, detonation));
        }
    }
    Err("the frag never detonated".into())
}

/// A cyborg `distance` in front of the player, `across` to its left.
fn cyborg(
    simulation: &mut Simulation,
    level: &Level,
    distance: f32,
    across: f32,
) -> Result<ObjectHandle, Box<dyn Error>> {
    let (feet, yaw) = {
        let player = simulation.player().ok_or("no player")?;
        (player.mover().position, player.look().0 .0)
    };
    let turn = Quat::from_rotation_z(yaw);
    let unit = level
        .globals
        .multiplayer_unit
        .clone()
        .ok_or("no multiplayer unit")?;
    Ok(simulation.spawn(ObjectSpawn {
        kind: ObjectKind::Biped,
        tag: unit,
        transform: Transform {
            position: feet + turn * Vec3::new(distance, across, 0.0),
            rotation: turn,
            scale: 1.0,
        },
        name: None,
        permutation: None,
        team: None,
    })?)
}

fn position(simulation: &Simulation, object: ObjectHandle) -> Result<Vec3, Box<dyn Error>> {
    Ok(simulation
        .component::<Transform>(object)
        .ok_or("no transform")?
        .position)
}

/// Detonates `grenade` at `point` and runs a tick.
fn blast(simulation: &mut Simulation, grenade: &Detonation, point: Vec3) {
    simulation.detonate(Detonation {
        point,
        attached: None,
        attached_damage: None,
        ..grenade.clone()
    });
    simulation.step(TickInput::default());
}

/// Scripted grenades at `object`'s feet until it dies (at most five).
fn kill(
    simulation: &mut Simulation,
    grenade: &Detonation,
    object: ObjectHandle,
) -> Result<(), Box<dyn Error>> {
    for _ in 0..5 {
        let feet = position(simulation, object)?;
        blast(simulation, grenade, feet + Vec3::new(0.3, 0.0, 0.1));
        let dead = simulation
            .component::<Vitality>(object)
            .is_some_and(|vitality| vitality.dead);
        if dead {
            return Ok(());
        }
    }
    Err("the grenades never killed it".into())
}

#[test]
fn a_cyborg_killed_by_a_grenade_becomes_a_ragdoll_that_stays() -> Result<(), Box<dyn Error>> {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let (mut simulation, grenade) = settled(&level)?;
    let cyborg = cyborg(&mut simulation, &level, 4.0, 0.0)?;
    simulation.step(TickInput::default());
    let standing = position(&simulation, cyborg)?;
    assert!(simulation.ragdolls().of(cyborg).is_none());
    kill(&mut simulation, &grenade, cyborg)?;

    let ragdoll = simulation.ragdolls().of(cyborg).ok_or("no ragdoll")?;
    assert!(ragdoll.body().parts.len() >= 10, "{:?}", ragdoll.body());
    assert!(!ragdoll.limp);
    for _ in 0..DRIVE_TICKS {
        simulation.step(TickInput::default());
    }
    let ragdoll = simulation.ragdolls().of(cyborg).ok_or("no ragdoll")?;
    assert!(ragdoll.limp);

    // (a minute on, it lies where it fell)
    for _ in 0..60 * clock::TICKS_PER_SECOND {
        simulation.step(TickInput::default());
    }
    let ragdoll = simulation.ragdolls().of(cyborg).ok_or("the corpse went")?;
    let lying = position(&simulation, cyborg)?;
    assert!(lying.is_finite(), "{lying}");
    assert!(lying.distance(standing) < 20.0, "{standing} {lying}");
    assert!((lying.z - standing.z).abs() < 2.0, "{standing} {lying}");
    let pelvis = ragdoll.poses()[0].position;
    let head = ragdoll
        .poses()
        .iter()
        .map(|pose| pose.position.z)
        .fold(f32::MIN, f32::max);
    // (lying down: nothing much above its pelvis)
    assert!(head - pelvis.z < 0.5, "{pelvis} {head}");
    let nodes = simulation
        .component::<opence_sim::animation::NodeMatrices>(cyborg)
        .ok_or("no nodes")?;
    assert_eq!(nodes.current[0].position, pelvis.to_array());
    Ok(())
}

#[test]
fn the_corpse_cap_removes_the_oldest() -> Result<(), Box<dyn Error>> {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let (mut simulation, grenade) = settled(&level)?;
    simulation.set_corpses(2);
    let cyborgs = [
        cyborg(&mut simulation, &level, 4.0, -6.0)?,
        cyborg(&mut simulation, &level, 4.0, 0.0)?,
        cyborg(&mut simulation, &level, 4.0, 6.0)?,
    ];
    simulation.step(TickInput::default());
    for &cyborg in &cyborgs {
        kill(&mut simulation, &grenade, cyborg)?;
    }
    let ragdolls: Vec<ObjectHandle> = simulation
        .ragdolls()
        .active()
        .iter()
        .map(|ragdoll| ragdoll.object)
        .collect();
    assert_eq!(ragdolls, cyborgs[1..]);
    Ok(())
}

#[test]
fn strong_blasts_tear_limbs_off() -> Result<(), Box<dyn Error>> {
    let Some(level) = bloodgulch()? else {
        return Ok(());
    };
    let (mut simulation, grenade) = settled(&level)?;
    let cyborg = cyborg(&mut simulation, &level, 4.0, 0.0)?;
    simulation.step(TickInput::default());
    kill(&mut simulation, &grenade, cyborg)?;
    // (blasts against a joint that tears)
    for _ in 0..6 {
        let ragdoll = simulation.ragdolls().of(cyborg).ok_or("no ragdoll")?;
        if ragdoll.torn != 0 {
            break;
        }
        let part = ragdoll
            .body()
            .parts
            .iter()
            .position(|part| part.break_damage > 0.0)
            .ok_or("no joint tears")?;
        let point = ragdoll.poses()[part].position;
        blast(&mut simulation, &grenade, point);
    }
    let torn = simulation.ragdolls().of(cyborg).ok_or("no ragdoll")?.torn;
    assert_ne!(torn, 0);

    // (none, with the tearing off)
    let (mut simulation, grenade) = settled(&level)?;
    simulation.set_dismemberment(false);
    let cyborg = self::cyborg(&mut simulation, &level, 4.0, 0.0)?;
    simulation.step(TickInput::default());
    kill(&mut simulation, &grenade, cyborg)?;
    for _ in 0..6 {
        let ragdoll = simulation.ragdolls().of(cyborg).ok_or("no ragdoll")?;
        let point = ragdoll.poses()[ragdoll.poses().len() / 2].position;
        blast(&mut simulation, &grenade, point);
    }
    assert_eq!(
        simulation.ragdolls().of(cyborg).ok_or("no ragdoll")?.torn,
        0
    );
    Ok(())
}
