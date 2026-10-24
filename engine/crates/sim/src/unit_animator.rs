//! The bipeds' animation in play (`biped_update`, `unit_update_animation`):
//! each biped with a [`UnitAnimator`] takes the state it wants from how it
//! is driven (its throttle, the air, its landing, its weapon), and its
//! state machine ([`UnitAnimation`]) plays its base animation, which its
//! [`AnimationPlayer`] poses. Its aiming and looking screens, action,
//! overlay action and soft ping go over the base as
//! [`crate::animation_blend::unit_pose`] lays them.

use std::sync::Arc;

use bevy_ecs::prelude::*;

use crate::animation::AnimationPlayer;
use crate::animation_blend::{aiming_screen_shown, Aim, UnitLayers};
use crate::damage::Vitality;
use crate::objects::{ObjectHandle, ObjectTable};
use crate::random::RandomSeed;
use crate::unit_states::{
    biped_desired_state, BipedTick, Mood, SeatChoice, UnitAnimation, UnitEvent, UnitState, UnitTick,
};
use crate::{ObjectId, Transform};

/// A biped's animation state machine, as it plays in the world.
#[derive(Component, Clone, Debug, Default, PartialEq)]
pub struct UnitAnimator {
    /// Its state, seat and layers.
    pub animation: UnitAnimation,
    /// The ticks it has been in the air.
    pub airborne_ticks: u8,
    /// The label of the weapon its seat was chosen for.
    pub weapon_label: Option<String>,
    /// The label of the vehicle's seat it rides in.
    pub seat_label: Option<String>,
    /// Where it aims, in the world; none for straight ahead.
    pub aiming_vector: Option<[f32; 3]>,
}

/// How a biped is driven in a tick: by a player or an actor.
#[derive(Clone, Debug, PartialEq)]
pub struct UnitDrive {
    /// The biped.
    pub handle: ObjectHandle,
    /// Its throttle: forward, left, up.
    pub throttle: [f32; 3],
    /// It is in the air.
    pub airborne: bool,
    /// The landing it recovers from: 0 soft, 1 hard.
    pub landing: Option<u8>,
    /// The label of the weapon in its hands (`unarmed` for none).
    pub weapon_label: Option<String>,
    /// It rides a vehicle.
    pub attached: bool,
    /// The label of the vehicle's seat it rides in (its seat in its
    /// graph).
    pub seat_label: Option<String>,
    /// It crouches.
    pub crouching: bool,
    /// Where it aims, in the world.
    pub aiming_vector: [f32; 3],
}

/// How each driven biped is driven this tick.
#[derive(Resource, Clone, Debug, Default)]
pub struct UnitDrives(pub Vec<UnitDrive>);

/// Steps each biped's state machine by a tick, in the order of the
/// objects' ids (so that the seed is drawn from in the same order on every
/// machine), and has its player play the base animation it chose. A biped
/// playing an impulse (its player resumes another animation after) is left
/// to it, and one its graph has no seat or state for plays on as it did.
pub(crate) fn animate_units(
    mut seed: ResMut<RandomSeed>,
    drives: Option<Res<UnitDrives>>,
    table: Option<Res<ObjectTable>>,
    order: Query<(Entity, &ObjectId), With<UnitAnimator>>,
    mut units: Query<(&mut UnitAnimator, &mut AnimationPlayer, Option<&Vitality>)>,
) {
    let mut entities: Vec<(Entity, ObjectId)> =
        order.iter().map(|(entity, id)| (entity, *id)).collect();
    entities.sort_by_key(|(_, id)| *id);
    let drives: Vec<(Entity, &UnitDrive)> = match (&drives, &table) {
        (Some(drives), Some(table)) => drives
            .0
            .iter()
            .filter_map(|drive| Some((table.entity(drive.handle)?, drive)))
            .collect(),
        _ => Vec::new(),
    };
    for (entity, _) in entities {
        let Ok((mut animator, mut player, vitality)) = units.get_mut(entity) else {
            continue;
        };
        if player.resumes() {
            continue;
        }
        // (one no player or actor drives stands, unarmed)
        let drive = drives
            .iter()
            .find(|(driven, _)| *driven == entity)
            .map(|(_, drive)| *drive);
        let throttle = drive.map_or([0.0; 3], |drive| drive.throttle);
        let airborne = drive.is_some_and(|drive| drive.airborne);
        let landing = drive.and_then(|drive| drive.landing);
        let attached = drive.is_some_and(|drive| drive.attached);
        let crouching = drive.is_some_and(|drive| drive.crouching);
        let seat_label = drive.and_then(|drive| drive.seat_label.as_deref());
        let weapon_label = drive.map_or(Some("unarmed"), |drive| drive.weapon_label.as_deref());
        let graph = Arc::clone(&player.graph);
        let dead = vitality.is_some_and(|vitality| vitality.dead);
        let animator = &mut *animator;
        // (players and actors that fight stand in combat:
        // `_unit_animation_state_in_combat`, players.c)
        animator.animation.mood = Mood::InCombat;
        animator.airborne_ticks = if airborne {
            animator.airborne_ticks.saturating_add(1)
        } else {
            0
        };
        let mut events = Vec::new();
        // (into a vehicle's seat: the seat's own, entering it; out of it:
        // its base seat at once, as the port's rider is already out)
        if animator.seat_label.as_deref() != seat_label && !dead {
            match seat_label {
                Some(label) => {
                    animator.animation.enter_seat(
                        &graph,
                        label,
                        weapon_label,
                        &mut || seed.next_real(),
                        &mut events,
                    );
                }
                None => {
                    animator.animation.base_seat = None;
                    animator.animation.state = None;
                }
            }
            animator.seat_label = seat_label.map(String::from);
        }
        // (a new weapon in its hands: its seat again, for its class)
        if animator.weapon_label.as_deref() != weapon_label && !attached && !dead {
            if let Some(seat) = animator.animation.base_seat {
                animator.animation.change_seat(&graph, seat, weapon_label);
            }
            animator.weapon_label = weapon_label.map(String::from);
        }
        // (a rider sits still in its seat)
        let desired = if attached && !dead {
            UnitState::Idle
        } else {
            biped_desired_state(&BipedTick {
                state: animator.animation.state,
                dead,
                limp: false,
                airborne,
                airborne_ticks: animator.airborne_ticks,
                no_dying_airborne: false,
                flying: false,
                landing,
                throttle,
                stunned: false,
                turn: None,
            })
        };
        let tick = UnitTick {
            desired: Some(desired),
            seat: SeatChoice {
                crouching,
                ..SeatChoice::default()
            },
            weapon_label,
            attached,
            dead,
            destroyed_after_dying: false,
            at_rest: false,
            biped: true,
            airborne,
            no_dying_airborne: false,
        };
        animator
            .animation
            .update(&graph, &tick, &mut || seed.next_real(), &mut events);
        animator.aiming_vector = drive.map(|drive| drive.aiming_vector);
        let interpolate = events.iter().find_map(|event| match event {
            UnitEvent::Interpolate(frames) => Some(*frames),
            _ => None,
        });
        if let Some(base) = animator.animation.base {
            player.drive(base, interpolate);
        }
    }
}

/// The layers a biped's state machine lays over its base animation: its
/// action, overlay action and soft ping, and its aiming and looking
/// screens toward where it aims, unless its state hides them.
pub(crate) fn unit_layers(animator: &UnitAnimator, transform: &Transform) -> UnitLayers {
    let animation = &animator.animation;
    let state = animation
        .state
        .map_or(-1, |state| i16::try_from(state.index()).unwrap_or(i16::MAX));
    let forward = (transform.rotation * glam::Vec3::X).to_array();
    let up = (transform.rotation * glam::Vec3::Z).to_array();
    let aim = aiming_screen_shown(animation.action.is_some(), state).then(|| {
        let vector = animator.aiming_vector.unwrap_or(forward);
        Aim {
            aiming_screen: animation.aiming_screen,
            looking_screen: animation.looking_screen,
            forward,
            up,
            aiming_vector: vector,
            looking_vector: vector,
            looks: true,
        }
    });
    UnitLayers {
        action: animation.action,
        overlay_action: animation.overlay_action,
        soft_ping: animation.soft_ping,
        seat: animation.seat,
        aim,
        ..UnitLayers::default()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::unit_states::SEAT_STAND;
    use crate::Simulation;
    use opence_content::{
        AnimationGraph, Group, ObjectKind, TagRef, UnitSeat, WeaponClass, WeaponType,
    };

    /// The animations of [`graph`]: 0 idle, 1 move front, 2 airborne, 3
    /// land soft, 4 land hard, 5 the crouching idle, 6 the rifle's idle,
    /// 7 entering a driver's seat, 8 sitting in it.
    fn graph() -> Arc<AnimationGraph> {
        let animations = (0..9)
            .map(|_| {
                let mut animation = crate::animation::tests::animation_named("a", 2);
                animation.frame_count = 5;
                animation
            })
            .collect();
        let mut graph = crate::animation::tests::graph_of(animations);
        let mut stand = vec![None; 51];
        stand[0] = Some(0);
        stand[8] = Some(1);
        stand[16] = Some(2);
        stand[17] = Some(3);
        stand[18] = Some(4);
        let mut crouch = stand.clone();
        crouch[0] = Some(5);
        let mut rifle = stand.clone();
        rifle[0] = Some(6);
        let class = |label: &str, kind: &str, animations: Vec<Option<u16>>| WeaponClass {
            label: label.into(),
            animations,
            weapon_types: vec![WeaponType {
                label: kind.into(),
                animations: vec![],
            }],
            ..WeaponClass::default()
        };
        let seat = |label: &str, unarmed: Vec<Option<u16>>| UnitSeat {
            label: label.into(),
            weapon_classes: vec![
                class("unarmed", "", unarmed),
                class("rifle", "ar", rifle.clone()),
            ],
            ..UnitSeat::default()
        };
        let mut driving = vec![None; 51];
        driving[0] = Some(8);
        let mut entering = vec![None; 30];
        entering[7] = Some(7);
        graph.unit_seats = vec![
            seat("stand", stand),
            seat("crouch", crouch),
            UnitSeat {
                label: "driver".into(),
                animations: entering,
                weapon_classes: vec![class("unarmed", "", driving)],
                ..UnitSeat::default()
            },
        ];
        Arc::new(graph)
    }

    fn biped(simulation: &mut Simulation, x: f32) -> Option<ObjectHandle> {
        let handle = simulation
            .spawn(crate::objects::ObjectSpawn {
                kind: ObjectKind::Biped,
                tag: TagRef {
                    group: Group(*b"bipd"),
                    path: "test".into(),
                },
                transform: Transform {
                    position: glam::Vec3::new(x, 0.0, 0.0),
                    ..Default::default()
                },
                name: None,
                permutation: None,
                team: None,
            })
            .ok()?;
        simulation
            .animate_unit(handle, AnimationPlayer::new(graph(), None, 0))
            .then_some(handle)
    }

    fn drive(handle: ObjectHandle) -> UnitDrive {
        UnitDrive {
            handle,
            throttle: [0.0; 3],
            airborne: false,
            landing: None,
            weapon_label: Some("unarmed".into()),
            attached: false,
            seat_label: None,
            crouching: false,
            aiming_vector: [1.0, 0.0, 0.0],
        }
    }

    /// Runs `ticks` ticks of the animator, each driven by `drives`.
    fn run(simulation: &mut Simulation, ticks: usize, drives: &dyn Fn(usize) -> Vec<UnitDrive>) {
        let mut schedule = Schedule::default();
        schedule.add_systems(animate_units);
        for tick in 0..ticks {
            simulation.world.insert_resource(UnitDrives(drives(tick)));
            schedule.run(&mut simulation.world);
        }
    }

    fn playing(simulation: &Simulation, handle: ObjectHandle) -> Option<(Option<UnitState>, u16)> {
        let animator = simulation.component::<UnitAnimator>(handle)?;
        let player = simulation.component::<AnimationPlayer>(handle)?;
        Some((animator.animation.state, player.state.animation))
    }

    #[test]
    fn an_undriven_biped_stands_idle_in_combat() -> Result<(), Box<dyn std::error::Error>> {
        let mut simulation = Simulation::empty();
        let handle = biped(&mut simulation, 0.0).ok_or("no biped")?;
        run(&mut simulation, 2, &|_| Vec::new());
        assert_eq!(
            playing(&simulation, handle),
            Some((Some(UnitState::Idle), 0))
        );
        let animator = simulation
            .component::<UnitAnimator>(handle)
            .ok_or("no animator")?;
        assert_eq!(animator.animation.base_seat, Some(SEAT_STAND));
        Ok(())
    }

    #[test]
    fn throttle_air_and_landings_play_their_animations() -> Result<(), Box<dyn std::error::Error>> {
        let mut simulation = Simulation::empty();
        let handle = biped(&mut simulation, 0.0).ok_or("no biped")?;
        run(&mut simulation, 2, &|_| {
            vec![UnitDrive {
                throttle: [1.0, 0.0, 0.0],
                ..drive(handle)
            }]
        });
        assert_eq!(
            playing(&simulation, handle),
            Some((Some(UnitState::MoveFront), 1))
        );
        // (three ticks in the air are a jump's start: on the fourth, the
        // air)
        run(&mut simulation, 3, &|_| {
            vec![UnitDrive {
                airborne: true,
                ..drive(handle)
            }]
        });
        assert_ne!(
            playing(&simulation, handle).map(|(state, _)| state),
            Some(Some(UnitState::Airborne))
        );
        run(&mut simulation, 1, &|_| {
            vec![UnitDrive {
                airborne: true,
                ..drive(handle)
            }]
        });
        assert_eq!(
            playing(&simulation, handle),
            Some((Some(UnitState::Airborne), 2))
        );
        run(&mut simulation, 1, &|_| {
            vec![UnitDrive {
                landing: Some(0),
                ..drive(handle)
            }]
        });
        assert_eq!(
            playing(&simulation, handle),
            Some((Some(UnitState::LandSoft), 3))
        );
        Ok(())
    }

    #[test]
    fn crouching_and_a_weapon_choose_the_seat_and_class() -> Result<(), Box<dyn std::error::Error>>
    {
        let mut simulation = Simulation::empty();
        let handle = biped(&mut simulation, 0.0).ok_or("no biped")?;
        run(&mut simulation, 2, &|_| {
            vec![UnitDrive {
                crouching: true,
                ..drive(handle)
            }]
        });
        assert_eq!(
            playing(&simulation, handle),
            Some((Some(UnitState::Idle), 5))
        );
        run(&mut simulation, 2, &|_| {
            vec![UnitDrive {
                weapon_label: Some("ar".into()),
                ..drive(handle)
            }]
        });
        assert_eq!(
            playing(&simulation, handle),
            Some((Some(UnitState::Idle), 6))
        );
        Ok(())
    }

    #[test]
    fn a_rider_enters_sits_and_gets_out() -> Result<(), Box<dyn std::error::Error>> {
        let mut simulation = Simulation::empty();
        let handle = biped(&mut simulation, 0.0).ok_or("no biped")?;
        run(&mut simulation, 2, &|_| vec![drive(handle)]);
        let riding = UnitDrive {
            attached: true,
            seat_label: Some("driver".into()),
            ..drive(handle)
        };
        run(&mut simulation, 1, &|_| vec![riding.clone()]);
        assert_eq!(
            playing(&simulation, handle),
            Some((Some(UnitState::EnteringSeat), 7))
        );
        // (the entering animation's five frames, then the seat's idle)
        run(&mut simulation, 6, &|_| vec![riding.clone()]);
        assert_eq!(
            playing(&simulation, handle),
            Some((Some(UnitState::Idle), 8))
        );
        // (moving the vehicle does not walk its rider)
        run(&mut simulation, 2, &|_| {
            vec![UnitDrive {
                throttle: [1.0, 0.0, 0.0],
                ..riding.clone()
            }]
        });
        assert_eq!(
            playing(&simulation, handle),
            Some((Some(UnitState::Idle), 8))
        );
        run(&mut simulation, 2, &|_| vec![drive(handle)]);
        assert_eq!(
            playing(&simulation, handle),
            Some((Some(UnitState::Idle), 0))
        );
        let animator = simulation
            .component::<UnitAnimator>(handle)
            .ok_or("no animator")?;
        assert_eq!(animator.animation.base_seat, Some(SEAT_STAND));
        Ok(())
    }

    #[test]
    fn a_stands_animation_is_left_alone() -> Result<(), Box<dyn std::error::Error>> {
        let mut simulation = Simulation::empty();
        let handle = biped(&mut simulation, 0.0).ok_or("no biped")?;
        // (a station's stand plays the animation it was given)
        if !simulation.animate(handle, AnimationPlayer::new(graph(), None, 4)) {
            return Err("not animated".into());
        }
        assert!(simulation.component::<UnitAnimator>(handle).is_none());
        run(&mut simulation, 2, &|_| {
            vec![UnitDrive {
                throttle: [1.0, 0.0, 0.0],
                ..drive(handle)
            }]
        });
        let player = simulation
            .component::<AnimationPlayer>(handle)
            .ok_or("no player")?;
        assert_eq!(player.state.animation, 4);
        Ok(())
    }

    #[test]
    fn the_same_drive_animates_the_same() -> Result<(), Box<dyn std::error::Error>> {
        let states = || -> Option<Vec<(Option<UnitState>, u16)>> {
            let mut simulation = Simulation::empty();
            let handles: Vec<ObjectHandle> = (0..3)
                .map(|index| biped(&mut simulation, index as f32))
                .collect::<Option<_>>()?;
            let mut seen = Vec::new();
            for tick in 0..40 {
                let drives: Vec<UnitDrive> = handles
                    .iter()
                    .enumerate()
                    .map(|(index, handle)| UnitDrive {
                        throttle: [if (tick + index) % 7 < 3 { 1.0 } else { 0.0 }, 0.0, 0.0],
                        airborne: (tick + index) % 11 > 6,
                        ..drive(*handle)
                    })
                    .collect();
                run(&mut simulation, 1, &|_| drives.clone());
                for handle in &handles {
                    seen.push(playing(&simulation, *handle)?);
                }
            }
            Some(seen)
        };
        let first = states().ok_or("no states")?;
        assert_eq!(Some(first), states());
        Ok(())
    }
}
