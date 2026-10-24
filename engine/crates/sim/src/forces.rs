//! The pushes damage gives what it strikes (damage.c's
//! `object_damage_aftermath`): each damage effect's instantaneous
//! acceleration, along the way the damage came and lifted a little, times
//! the struck object's acceleration scale. An area's push is not lessened
//! by distance: whatever its damage reaches is pushed in full.
//!
//! Accelerations are in world units a game tick, as items' and vehicles'
//! velocities are.

use bevy_ecs::prelude::*;
use glam::Vec3;
use opence_content::{DamageEffect, TagRef};

use crate::animation::NodeMatrices;
use crate::damage::{area_scale, bounding_centre, reaches, DamageTable, Vitality};
use crate::item_physics::Lying;
use crate::objects::{Object, ObjectHandle, ObjectTable, Team};
use crate::player::Player;
use crate::projectile::{Detonation, Impact};
use crate::random::RandomSeed;
use crate::seats::Rides;
use crate::vehicle::GAME_TICKS_PER_SECOND;
use crate::Transform;

/// How much a push is lifted: what is added to its direction's height
/// before it is made a direction again.
const LIFT: f32 = 0.45;

/// A push damage gave an object.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Push {
    /// The object pushed.
    pub object: ObjectHandle,
    /// Its acceleration, in world units a game tick.
    pub acceleration: Vec3,
    /// The damage detonates explosives and more than half of it arrived:
    /// a vehicle is pushed twice as hard.
    pub detonates: bool,
}

/// The acceleration an effect's `instantaneous` acceleration gives an
/// object of acceleration `scale`, damaged from along `direction`: the
/// direction lifted, by a game tick's share of a second.
pub fn acceleration(direction: Vec3, instantaneous: f32, scale: f32) -> Vec3 {
    let direction = direction.normalize_or_zero();
    let lifted = Vec3::new(direction.x, direction.y, direction.z + LIFT).normalize_or_zero();
    lifted * (instantaneous * scale / GAME_TICKS_PER_SECOND as f32)
}

/// What `object` is (its definition's tag), if the world has it.
fn tag_of(world: &World, object: ObjectHandle) -> Option<TagRef> {
    let entity = world.resource::<ObjectTable>().entity(object)?;
    world.get::<Object>(entity).map(|object| object.tag.clone())
}

/// The pushes of each impact's damage on the object it struck, along the
/// way the projectile flew.
pub fn impact_pushes(world: &World, impacts: &[Impact]) -> Vec<Push> {
    let Some(table) = world.get_resource::<DamageTable>() else {
        return Vec::new();
    };
    impacts
        .iter()
        .filter_map(|impact| {
            let object = impact.object?;
            let effect = table.effects.get(impact.damage.as_ref()?)?;
            let pushable = table.pushable.get(&tag_of(world, object)?)?;
            push(object, effect, impact.direction, pushable.scale, false)
        })
        .collect()
}

/// A push of `effect`'s, if it pushes at all.
fn push(
    object: ObjectHandle,
    effect: &DamageEffect,
    direction: Vec3,
    scale: f32,
    detonates: bool,
) -> Option<Push> {
    if effect.instantaneous_acceleration <= f32::EPSILON {
        return None;
    }
    let acceleration = acceleration(direction, effect.instantaneous_acceleration, scale);
    (acceleration != Vec3::ZERO).then_some(Push {
        object,
        acceleration,
        detonates,
    })
}

/// The pushes of each detonation's area damage: on every pushable object
/// its damage reaches (as [`crate::damage::apply_detonations`] finds
/// them), away from its point. `clear` says whether nothing of the level
/// stands between two points.
pub fn detonation_pushes(
    world: &mut World,
    detonations: &[Detonation],
    mut clear: impl FnMut(Vec3, Vec3) -> bool,
) -> Vec<Push> {
    let Some(table) = world.get_resource::<DamageTable>() else {
        return Vec::new();
    };
    if table.pushable.is_empty() || detonations.iter().all(|d| d.effect.is_none()) {
        return Vec::new();
    }
    let posed: Vec<(ObjectHandle, TagRef, Transform, Option<_>, Option<u16>)> = world
        .query::<(
            &ObjectHandle,
            &Object,
            &Transform,
            Option<&NodeMatrices>,
            Option<&Team>,
        )>()
        .iter(world)
        .map(|(handle, object, transform, nodes, team)| {
            let root = nodes.and_then(|nodes| nodes.current.first()).cloned();
            (
                *handle,
                object.tag.clone(),
                *transform,
                root,
                team.map(|team| team.0),
            )
        })
        .collect();
    let Some(table) = world.get_resource::<DamageTable>() else {
        return Vec::new();
    };
    // (each pushable object's centre and team, in the order of their
    // handles)
    let mut objects: Vec<(ObjectHandle, f32, Vec3, Option<u16>)> = posed
        .iter()
        .filter_map(|(handle, tag, transform, root, team)| {
            let pushable = table.pushable.get(tag)?;
            let centre = match table.objects.get(tag) {
                Some(damageable) => bounding_centre(damageable, transform, root.as_ref()),
                None => {
                    transform.position + transform.rotation * (pushable.centre * transform.scale)
                }
            };
            Some((*handle, pushable.scale, centre, *team))
        })
        .collect();
    objects.sort_by_key(|(handle, ..)| *handle);
    let team_of = |handle: ObjectHandle| {
        objects
            .iter()
            .find(|(object, ..)| *object == handle)
            .and_then(|(.., team)| *team)
            .or_else(|| {
                posed
                    .iter()
                    .find(|(object, ..)| *object == handle)
                    .and_then(|(.., team)| *team)
            })
    };
    let mut pushes = Vec::new();
    for detonation in detonations {
        let Some(damage) = detonation
            .effect
            .as_ref()
            .and_then(|effect| table.explosions.get(effect))
        else {
            continue;
        };
        let owner = detonation.owner;
        let owner_team = team_of(owner);
        for effect in damage {
            let Some(definition) = table.effects.get(effect) else {
                continue;
            };
            for &(object, scale, centre, team) in &objects {
                if definition.damage_flags & DamageEffect::DOES_NOT_HURT_OWNER != 0
                    && object == owner
                {
                    continue;
                }
                let friendly = object != owner && owner_team.is_some() && team == owner_team;
                if definition.damage_flags & DamageEffect::DOES_NOT_HURT_FRIENDS != 0 && friendly {
                    continue;
                }
                let reached = area_scale(definition, detonation.point.distance(centre));
                if reached <= 0.0
                    || !reaches(
                        detonation.point,
                        centre,
                        definition.area_of_effect_core_radius,
                        &mut clear,
                    )
                {
                    continue;
                }
                // (straight up from beneath it, as the game's centre is)
                let away = centre - detonation.point;
                let direction = if away.length_squared() > f32::EPSILON {
                    away
                } else {
                    Vec3::Z
                };
                let detonates = reached > 0.5
                    && definition.damage_flags & DamageEffect::DETONATES_EXPLOSIVES != 0;
                pushes.extend(push(object, definition, direction, scale, detonates));
            }
        }
    }
    pushes
}

/// Gives each push to what it pushes: an item lying about is woken and
/// turned (`item_accelerate`), a vehicle woken and spun
/// (`vehicle_accelerate`), and a living player's biped thrown off its feet
/// (`biped_accelerate`). The dead are the ragdolls' to throw.
pub(crate) fn apply(world: &mut World, rides: &mut Rides, players: &mut [Player], pushes: &[Push]) {
    if pushes.is_empty() {
        return;
    }
    let mut seed = world.remove_resource::<RandomSeed>().unwrap_or_default();
    for push in pushes {
        if rides.push(push.object, push.acceleration, push.detonates) {
            continue;
        }
        if let Some(player) = players
            .iter_mut()
            .find(|player| player.handle() == push.object)
        {
            let dead = world
                .resource::<ObjectTable>()
                .entity(push.object)
                .and_then(|entity| world.get::<Vitality>(entity))
                .is_some_and(|vitality| vitality.dead);
            if !dead {
                player.push(push.acceleration);
            }
            continue;
        }
        let Some(entity) = world.resource::<ObjectTable>().entity(push.object) else {
            continue;
        };
        if let Some(mut lying) = world.get_mut::<Lying>(entity) {
            lying.motion.accelerate(push.acceleration, &mut seed);
        }
    }
    world.insert_resource(seed);
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::damage::Pushable;
    use crate::objects::{spawn, ObjectSpawn};
    use opence_content::{FallingDamage, Group, Item, ObjectKind};
    use opence_physics::Struck;
    use std::collections::BTreeMap;

    fn tag(group: &[u8; 4], path: &str) -> TagRef {
        TagRef {
            group: Group(*group),
            path: path.into(),
        }
    }

    /// A blast reaching 2.5 that pushes by 3 world units a second, a
    /// tick's push 0.1, sparing its owner's friends.
    fn blast() -> DamageEffect {
        DamageEffect {
            tag: tag(b"jpt!", "blast"),
            radius: [0.5, 2.5],
            cutoff_scale: 0.0,
            flags: 0,
            side_effect: 0,
            category: 0,
            damage_flags: DamageEffect::DOES_NOT_HURT_FRIENDS | DamageEffect::DETONATES_EXPLOSIVES,
            area_of_effect_core_radius: 0.0,
            damage_minimum: 0.0,
            damage: [40.0, 40.0],
            vehicle_passthrough_penalty: 0.0,
            active_camouflage_damage: 0.0,
            stun: 0.0,
            maximum_stun: 0.0,
            stun_time: 0.0,
            instantaneous_acceleration: 3.0,
            material_modifiers: vec![1.0; DamageEffect::MATERIAL_TYPES],
        }
    }

    /// A world of the blast and crates (`eqip`) it pushes at half scale,
    /// their centres half a unit up; a crate at each `x` along X, of each
    /// team.
    fn crates(at: &[(f32, Option<u16>)]) -> (World, Vec<ObjectHandle>) {
        let mut simulation = crate::Simulation::empty();
        let mut world = std::mem::take(&mut simulation.world);
        world.insert_resource(DamageTable {
            objects: BTreeMap::new(),
            effects: BTreeMap::from([(tag(b"jpt!", "blast"), blast())]),
            explosions: BTreeMap::from([(tag(b"effe", "explosion"), vec![tag(b"jpt!", "blast")])]),
            multiplayer: true,
            falling: FallingDamage::default(),
            pushable: BTreeMap::from([(
                tag(b"eqip", "crate"),
                Pushable {
                    scale: 0.5,
                    centre: Vec3::new(0.0, 0.0, 0.5),
                },
            )]),
        });
        let handles = at
            .iter()
            .map(|&(x, team)| {
                let spawned = spawn(
                    &mut world,
                    ObjectSpawn {
                        kind: ObjectKind::Equipment,
                        tag: tag(b"eqip", "crate"),
                        transform: Transform {
                            position: Vec3::new(x, 0.0, 0.0),
                            ..Transform::default()
                        },
                        name: None,
                        permutation: None,
                        team,
                    },
                );
                let Ok(handle) = spawned else {
                    panic!("no room");
                };
                handle
            })
            .collect();
        (world, handles)
    }

    fn detonation(owner: ObjectHandle, x: f32) -> Detonation {
        Detonation {
            projectile: tag(b"proj", "grenade"),
            owner,
            point: Vec3::new(x, 0.0, 0.5),
            effect: Some(tag(b"effe", "explosion")),
            attached: None,
            attached_damage: None,
        }
    }

    #[test]
    fn a_blast_pushes_in_full_what_it_reaches_and_away_from_it() {
        let (mut world, crates) = crates(&[(1.0, None), (-2.0, None), (4.0, None)]);
        let nobody = ObjectHandle {
            index: 999,
            salt: 1,
        };
        let pushes = detonation_pushes(&mut world, &[detonation(nobody, 0.0)], |_, _| true);
        assert_eq!(pushes.len(), 2, "{pushes:?}");
        // (the near one more than half reached: it detonates; the far one
        // pushed as hard, but not)
        let near = pushes[0];
        assert_eq!(near.object, crates[0]);
        assert!(near.detonates);
        let away = Vec3::new(1.0, 0.0, LIFT).normalize() * 0.05;
        assert!(near.acceleration.distance(away) < 1e-5, "{near:?}");
        let far = pushes[1];
        assert_eq!(far.object, crates[1]);
        assert!(!far.detonates);
        assert!((far.acceleration.length() - 0.05).abs() < 1e-5);
        assert!(far.acceleration.x < 0.0 && far.acceleration.z > 0.0);
        // (behind a wall: nothing)
        let walled = detonation_pushes(&mut world, &[detonation(nobody, 0.0)], |_, _| false);
        assert!(walled.is_empty());
    }

    #[test]
    fn a_blast_spares_its_owner_s_friends_but_not_its_owner() {
        let (mut world, crates) = crates(&[(1.0, Some(1)), (-1.0, Some(1)), (0.0, Some(2))]);
        let pushes = detonation_pushes(&mut world, &[detonation(crates[0], 0.0)], |_, _| true);
        let pushed: Vec<ObjectHandle> = pushes.iter().map(|push| push.object).collect();
        assert_eq!(pushed, vec![crates[0], crates[2]]);
        // (straight up from beneath the crate it went off in)
        let up = pushes[1].acceleration;
        assert!(up.distance(Vec3::Z * 0.05) < 1e-5, "{up}");
    }

    #[test]
    fn an_impact_pushes_along_the_way_its_projectile_flew() {
        let (world, crates) = crates(&[(1.0, None)]);
        let impact = |object, damage| Impact {
            projectile: tag(b"proj", "bullet"),
            owner: ObjectHandle { index: 9, salt: 1 },
            damage,
            point: Vec3::new(1.0, 0.0, 0.5),
            normal: -Vec3::Y,
            direction: Vec3::Y * 20.0,
            struck: Struck::Other,
            object,
        };
        let pushes = impact_pushes(
            &world,
            &[
                impact(Some(crates[0]), Some(tag(b"jpt!", "blast"))),
                impact(None, Some(tag(b"jpt!", "blast"))),
                impact(Some(crates[0]), None),
            ],
        );
        assert_eq!(pushes.len(), 1);
        let along = Vec3::new(0.0, 1.0, LIFT).normalize() * 0.05;
        assert!(pushes[0].acceleration.distance(along) < 1e-5);
    }

    #[test]
    fn a_push_wakes_an_item_lying_about() {
        let (mut world, crates) = crates(&[(1.0, None)]);
        let Some(entity) = world.resource::<ObjectTable>().entity(crates[0]) else {
            panic!("no entity");
        };
        let mut lying = Lying::new(Item::default(), &Transform::default(), Vec3::ZERO);
        lying.motion.at_rest = true;
        world.entity_mut(entity).insert(lying);
        let push = Push {
            object: crates[0],
            acceleration: Vec3::new(0.1, 0.0, 0.05),
            detonates: false,
        };
        apply(&mut world, &mut Rides::default(), &mut [], &[push]);
        let Some(lying) = world.get::<Lying>(entity) else {
            panic!("not lying");
        };
        assert!(!lying.motion.at_rest);
        assert_eq!(lying.motion.velocity, push.acceleration);
        assert!(world.get_resource::<RandomSeed>().is_some());
    }

    #[test]
    fn a_push_is_lifted_and_a_game_tick_of_the_acceleration() {
        let push = acceleration(Vec3::X, 30.0, 1.0);
        let lifted = Vec3::new(1.0, 0.0, LIFT).normalize();
        assert!(push.distance(lifted) < 1e-5);
        // (scaled by the object's acceleration scale)
        let half = acceleration(Vec3::X * 7.0, 30.0, 0.5);
        assert!(half.distance(lifted * 0.5) < 1e-5);
        assert_eq!(acceleration(Vec3::ZERO, 30.0, 1.0), Vec3::Z);
    }
}
