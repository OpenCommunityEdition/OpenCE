//! The player: a biped moved by Halo's player physics over the level's
//! collision (opence_physics), and the camera that looks out of it or
//! follows it.
//!
//! The physics world is not `Sync`, so it lives beside the bevy world, in
//! [`crate::Simulation`]; each tick moves the biped first, and a system then
//! puts its object and the camera where the step left them. Its weapons
//! fire after the move, from where it then looks, and their projectiles
//! fly through the same physics world.

use bevy_ecs::prelude::*;
use glam::{Quat, Vec2, Vec3};
use opence_content::{Biped, Level, ObjectKind, TagRef, Weapon};
use opence_physics::{
    Hit, Layers, LevelMesh, MovementInput, Mover, MoverSettings, PhysicsError, Struck,
    World as PhysicsWorld, WorldSettings,
};
use thiserror::Error;

use crate::armory::{self, Armory, Carried, Inventory, WeaponItem};
use crate::clock;
use crate::damage;
use crate::debris::Ejection;
use crate::hash::WorldHasher;
use crate::input::{Buttons, TickInput};
use crate::objects::{self, Object, ObjectHandle, ObjectSpawn};
use crate::projectile::{self, Contact, Detonation, Flew, Flight, Impact};
use crate::random::RandomSeed;
use crate::units::Radians;
use crate::weapon::{secondary, WeaponControls, WeaponState};
use crate::{FreeCamera, Transform};

/// How far the player's melee reaches from its eyes, in world units.
pub const MELEE_RANGE: f32 = 0.8;
/// The rays across and up the melee's fan.
const MELEE_RAYS: u32 = 5;
/// How far the fan's outer rays lean from the aim (as the tangent).
const MELEE_SPREAD: f32 = 0.1;
/// The ticks after a melee before the next, and before it fires (the
/// port's: the game waits on the melee animation).
pub const MELEE_RECOVERY_TICKS: u32 = clock::TICKS_PER_SECOND;

/// Where the player sees from.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum View {
    /// Out of the biped's eyes; its own body is not drawn.
    #[default]
    FirstPerson,
    /// Behind and above it, stopped short of walls.
    ThirdPerson,
}

impl View {
    /// The other one.
    pub fn toggled(self) -> View {
        match self {
            View::FirstPerson => View::ThirdPerson,
            View::ThirdPerson => View::FirstPerson,
        }
    }
}

/// How far behind its eyes the third-person camera follows, in world
/// units.
pub const FOLLOW_DISTANCE: f32 = 1.2;
/// How far above them it follows.
pub const FOLLOW_HEIGHT: f32 = 0.25;
/// How far short of a wall the third-person camera stops.
const FOLLOW_MARGIN: f32 = 0.05;

/// Why there is no player.
#[derive(Debug, Error)]
pub enum PlayerError {
    /// The scenario has no player start of that number.
    #[error("the scenario has no player start {0}")]
    NoStart(usize),
    /// The start's structure BSP is not in the level.
    #[error("the level has no structure bsp {0}")]
    NoBsp(usize),
    /// Neither the campaign's nor multiplayer's player unit is a biped in
    /// the level.
    #[error("the level has no player biped")]
    NoBiped,
    /// The world holds no more objects.
    #[error("the world holds no more objects")]
    TableFull,
    /// The physics refused the level or the biped.
    #[error(transparent)]
    Physics(#[from] PhysicsError),
}

/// The player: its biped's movement and where it looks.
pub struct Player {
    physics: PhysicsWorld,
    mover: Mover,
    /// The facing: the turn to the left from +X.
    yaw: f32,
    /// The look up from level.
    pitch: f32,
    view: View,
    /// Its biped's object.
    handle: ObjectHandle,
    /// Its biped's tag.
    unit: TagRef,
    /// The level's weapons.
    armory: Armory,
    /// The weapons it carries.
    inventory: Inventory,
    /// Its projectiles in flight.
    flights: Vec<Flight>,
    /// The buttons held the tick before, for presses.
    held: Buttons,
    /// The world's gravity, in world units per second per second.
    gravity: f32,
    /// The ticks before it can melee again.
    melee_ticks: u32,
    /// The speed of this tick's landing into the ground, if it landed.
    landed: Option<f32>,
    /// The speed its grenades leave its hand, in world units per second.
    grenade_velocity: f32,
    /// Where they leave it, from its eyes (the globals').
    grenade_origin: Vec3,
}

/// What a tick of the player's weapons came to.
#[derive(Clone, Debug, Default, PartialEq)]
pub(crate) struct Fired {
    /// Where its projectiles hit.
    pub impacts: Vec<Impact>,
    /// Where its grenades detonated.
    pub detonations: Vec<Detonation>,
    /// The effects its shots made, which throw out casings.
    pub ejections: Vec<Ejection>,
}

/// What the tick's systems need of the player: where its object goes and
/// where the camera goes.
#[derive(Resource, Clone, Copy, Debug)]
pub(crate) struct PlayerPose {
    pub handle: ObjectHandle,
    pub body: Transform,
    pub camera: Transform,
}

/// The player's biped tag in `level`: the campaign's player unit, else
/// multiplayer's, whichever the level has as a biped.
fn player_unit(level: &Level) -> Option<(TagRef, Biped)> {
    [&level.globals.player_unit, &level.globals.multiplayer_unit]
        .into_iter()
        .flatten()
        .find_map(|unit| {
            let biped = level.object(unit)?.biped?;
            Some((unit.clone(), biped))
        })
}

impl Player {
    /// A player standing at `level`'s player start `start`, over the
    /// collision of the start's structure BSP; `spawn` makes its biped's
    /// object.
    pub(crate) fn new(
        level: &Level,
        start: usize,
        spawn: impl FnOnce(TagRef, Transform) -> Option<ObjectHandle>,
    ) -> Result<Player, PlayerError> {
        let place = level
            .scenario
            .player_starts
            .get(start)
            .ok_or(PlayerError::NoStart(start))?;
        let bsp = usize::from(place.structure_bsp.unwrap_or(0));
        let structure = level
            .structure_bsps
            .get(bsp)
            .ok_or(PlayerError::NoBsp(bsp))?;
        let (unit, biped) = player_unit(level).ok_or(PlayerError::NoBiped)?;
        let settings = WorldSettings::default();
        let mut physics = PhysicsWorld::new(settings)?;
        physics.set_level(&LevelMesh::from_collision(&structure.collision))?;
        let mover = Mover::new(
            &mut physics,
            MoverSettings {
                biped,
                player: level.globals.player,
                gravity: -settings.gravity.z,
            },
            Vec3::from_array(place.position),
        )?;
        let yaw = libm::remainderf(place.facing, std::f32::consts::TAU);
        let body = body_transform(mover.position, yaw);
        let handle = spawn(unit.clone(), body).ok_or(PlayerError::TableFull)?;
        let armory = Armory::new(level);
        let inventory = Inventory::starting(level, &armory);
        Ok(Player {
            physics,
            mover,
            yaw,
            pitch: 0.0,
            view: View::default(),
            handle,
            unit,
            armory,
            inventory,
            flights: Vec::new(),
            held: Buttons::default(),
            gravity: -settings.gravity.z,
            melee_ticks: 0,
            landed: None,
            grenade_velocity: biped.grenade_velocity,
            grenade_origin: Vec3::from_array(level.globals.player.grenade_origin),
        })
    }

    /// Its biped's object.
    pub fn handle(&self) -> ObjectHandle {
        self.handle
    }

    /// The physics world the biped moves in, which the ragdolls lie in.
    pub(crate) fn physics_mut(&mut self) -> &mut PhysicsWorld {
        &mut self.physics
    }

    /// Its biped's tag.
    pub fn unit(&self) -> &TagRef {
        &self.unit
    }

    /// Its movement: where its feet are, its velocity, its ground.
    pub fn mover(&self) -> &Mover {
        &self.mover
    }

    /// The level's weapons.
    pub fn armory(&self) -> &Armory {
        &self.armory
    }

    /// The weapons it carries.
    pub fn inventory(&self) -> &Inventory {
        &self.inventory
    }

    /// The tag of the weapon in its hands, and its state.
    pub fn weapon(&self) -> Option<(&TagRef, &WeaponState)> {
        let carried = self.inventory.current()?;
        Some((&self.armory.get(carried.armory)?.tag, &carried.state))
    }

    /// Its projectiles in flight.
    pub fn flights(&self) -> &[Flight] {
        &self.flights
    }

    /// Where it sees from.
    pub fn view(&self) -> View {
        self.view
    }

    pub(crate) fn set_view(&mut self, view: View) {
        self.view = view;
    }

    /// Its facing and look.
    pub fn look(&self) -> (Radians, Radians) {
        (Radians(self.yaw), Radians(self.pitch))
    }

    /// One tick of `input`: the mouse's turn, then the move.
    pub(crate) fn step(&mut self, input: &TickInput) -> PlayerPose {
        let max = FreeCamera::MAX_PITCH.0;
        self.yaw = libm::remainderf(self.yaw + input.yaw.0, std::f32::consts::TAU);
        self.pitch = (self.pitch + input.pitch.0).clamp(-max, max);
        let axis = |value: i8| f32::from(value) / 127.0;
        // (forward and left; no faster on a diagonal)
        let throttle = Vec2::new(axis(input.forward), -axis(input.right));
        let throttle = if throttle.length_squared() > 1.0 {
            throttle.normalize()
        } else {
            throttle
        };
        let movement = MovementInput {
            throttle,
            yaw: self.yaw,
            jump: input.buttons.contains(Buttons::JUMP),
            crouch: input.buttons.contains(Buttons::CROUCH),
        };
        // (the physics refuses only numbers that are not finite, which the
        // input never holds: a refused step leaves the biped where it was)
        let airborne = self.mover.airborne;
        let _ = self.mover.step(&mut self.physics, &movement);
        self.landed = (airborne && !self.mover.airborne).then_some(self.mover.landing_velocity);
        self.pose()
    }

    /// Where its object and the camera are now.
    pub(crate) fn pose(&self) -> PlayerPose {
        PlayerPose {
            handle: self.handle,
            body: body_transform(self.mover.position, self.yaw),
            camera: self.camera(),
        }
    }

    /// The camera: at its eyes, or following behind them.
    fn camera(&self) -> Transform {
        let rotation = look_rotation(self.yaw, self.pitch);
        let eyes = self.mover.position + Vec3::Z * self.mover.camera_height();
        let position = match self.view {
            View::FirstPerson => eyes,
            View::ThirdPerson => {
                let back =
                    rotation * Vec3::new(-FOLLOW_DISTANCE, 0.0, 0.0) + Vec3::Z * FOLLOW_HEIGHT;
                let reach = match self.physics.cast_ray(eyes, back, Layers::LEVEL) {
                    Ok(Some(hit)) => (hit.fraction - FOLLOW_MARGIN / back.length()).max(0.0),
                    _ => 1.0,
                };
                eyes + back * reach
            }
        };
        Transform {
            position,
            rotation,
            scale: 1.0,
        }
    }

    /// The speed it landed at this tick, into the ground, if it landed.
    pub fn landed(&self) -> Option<f32> {
        self.landed
    }

    /// Its eyes, and where they look.
    fn aim(&self) -> (Vec3, Quat) {
        (
            self.mover.position + Vec3::Z * self.mover.camera_height(),
            look_rotation(self.yaw, self.pitch),
        )
    }

    /// Gives it `count` grenades of `kind` (0 frag, 1 plasma), up to the
    /// most it carries.
    pub(crate) fn give_grenades(&mut self, kind: usize, count: i16) {
        self.inventory.give_grenades(&self.armory, kind, count);
    }

    /// Whether nothing of the level stands between `from` and `to`.
    pub(crate) fn level_clear(&self, from: Vec3, to: Vec3) -> bool {
        let path = to - from;
        path.length_squared() == 0.0
            || !matches!(
                self.physics.cast_ray(from, path, Layers::LEVEL),
                Ok(Some(_))
            )
    }

    /// One tick of its weapons, after its move: its projectiles fly on,
    /// it picks up and changes weapons, throws a grenade, and the one in
    /// its hands fires. Where projectiles hit and grenades detonated is
    /// returned.
    pub(crate) fn step_weapons(&mut self, input: &TickInput, world: &mut World) -> Fired {
        let buttons = input.buttons;
        let held = std::mem::replace(&mut self.held, buttons);
        let mut fired = self.fly(world);
        self.pick_up(world, buttons.pressed(held, Buttons::ACTION));
        if buttons.pressed(held, Buttons::SWAP) {
            self.inventory.swap(&self.armory);
        }
        if buttons.pressed(held, Buttons::SWITCH_GRENADE) {
            self.inventory.switch_grenade();
        }
        let (eyes, aim) = self.aim();
        self.melee_ticks = self.melee_ticks.saturating_sub(1);
        if buttons.pressed(held, Buttons::MELEE) && self.melee_ticks == 0 {
            if let Some(impact) = self.melee(world, eyes, aim) {
                fired.impacts.push(impact);
            }
        }
        if buttons.pressed(held, Buttons::GRENADE) && self.melee_ticks == 0 {
            self.throw_grenade(world, eyes, aim);
        }
        // (no firing while the melee recovers)
        let fire = buttons.contains(Buttons::FIRE) && self.melee_ticks == 0;
        let reload = buttons.pressed(held, Buttons::RELOAD);
        let Some(carried) = self.inventory.current_mut() else {
            return fired;
        };
        let Some(definition) = self.armory.get(carried.armory) else {
            return fired;
        };
        let weapon = &definition.weapon;
        let controls = WeaponControls {
            // (one button: the second trigger only when it follows the first)
            triggers: [
                fire,
                fire && weapon.secondary_trigger_mode == secondary::SLAVED_TO_PRIMARY,
            ],
            reload,
        };
        let shots = carried.state.step(weapon, controls);
        for shot in shots {
            if let Some(effect) = weapon
                .triggers
                .get(shot.trigger)
                .and_then(|trigger| trigger.firing_effect.clone())
            {
                fired
                    .ejections
                    .push(ejection(effect, eyes, aim, self.mover.velocity));
            }
            let (Some(trigger), Some(Some((tag, projectile)))) = (
                weapon.triggers.get(shot.trigger),
                definition.projectiles.get(shot.trigger),
            ) else {
                continue;
            };
            let origin = eyes + aim * Vec3::from_array(trigger.first_person_offset);
            let directions = {
                let mut seed = world.get_resource_or_init::<RandomSeed>();
                projectile::shot_directions(trigger, shot.error, aim, &mut seed)
            };
            for direction in directions {
                let transform = Transform {
                    position: origin,
                    rotation: Quat::from_rotation_arc(Vec3::X, direction),
                    scale: 1.0,
                };
                let Ok(handle) = objects::spawn(
                    world,
                    ObjectSpawn {
                        kind: ObjectKind::Projectile,
                        tag: tag.clone(),
                        transform,
                        name: None,
                        permutation: None,
                        team: None,
                    },
                ) else {
                    break; // (the world holds no more objects)
                };
                self.flights.push(Flight::new(
                    handle,
                    tag.clone(),
                    self.handle,
                    projectile,
                    origin,
                    direction,
                    self.gravity,
                ));
            }
        }
        fired
    }

    /// Throws a grenade of the kind it has ready, if it has one and the
    /// weapon in its hands allows it: from its eyes, along its aim.
    fn throw_grenade(&mut self, world: &mut World, eyes: Vec3, aim: Quat) {
        let prevented = self
            .inventory
            .current()
            .and_then(|carried| self.armory.get(carried.armory))
            .is_some_and(|definition| {
                definition.weapon.flags & Weapon::PREVENTS_GRENADE_THROWING != 0
            });
        let kind = self.inventory.grenade;
        let count = self.inventory.grenades.get(kind).copied().unwrap_or(0);
        if prevented || count <= 0 {
            return;
        }
        let Some((tag, projectile)) = self
            .armory
            .grenade(kind)
            .and_then(|grenade| grenade.projectile.clone())
        else {
            return;
        };
        // (the globals' origin: forward, then across and above the aim as
        // the game takes them, from the world's up)
        let forward = aim * Vec3::X;
        let across = Vec3::Z.cross(forward).try_normalize().unwrap_or(Vec3::Z);
        let above = forward.cross(across).try_normalize().unwrap_or(Vec3::Z);
        let [ahead, first, second] = self.grenade_origin.to_array();
        let origin = eyes + forward * ahead + across * first + above * second;
        let Ok(handle) = objects::spawn(
            world,
            ObjectSpawn {
                kind: ObjectKind::Projectile,
                tag: tag.clone(),
                transform: Transform {
                    position: origin,
                    rotation: aim,
                    scale: 1.0,
                },
                name: None,
                permutation: None,
                team: None,
            },
        ) else {
            return; // (the world holds no more objects)
        };
        let mut seed = world.get_resource_or_init::<RandomSeed>();
        self.flights.push(Flight::thrown(
            handle,
            tag,
            self.handle,
            &projectile,
            origin,
            forward * self.grenade_velocity,
            self.gravity,
            &mut seed,
        ));
        if let Some(carried) = self.inventory.grenades.get_mut(kind) {
            *carried -= 1;
        }
    }

    /// A tick of its projectiles' flight: those that hit something, are
    /// spent or detonate are removed, with their objects; those stuck to
    /// something follow it.
    fn fly(&mut self, world: &mut World) -> Fired {
        let mut fired = Fired::default();
        let physics = &self.physics;
        for flight in &mut self.flights {
            if let Some(parent) = flight.attachment().and_then(|attached| attached.parent) {
                flight.follow(frame(world, parent));
            }
        }
        let targets = damage::targets(world, self.handle);
        let frames: Vec<Option<Transform>> = targets
            .iter()
            .map(|target| frame(world, target.handle))
            .collect();
        let mut poses = Vec::with_capacity(self.flights.len());
        self.flights.retain_mut(|flight| {
            // (the nearer of the level and the bipeds)
            let flew = flight.step(|from, path| {
                let level = physics.cast_ray(from, path, Layers::LEVEL).ok().flatten();
                match damage::first_target(&targets, from, path) {
                    Some((target, fraction, normal))
                        if level.as_ref().is_none_or(|hit| fraction < hit.fraction) =>
                    {
                        Some(Contact {
                            hit: Hit {
                                fraction,
                                point: from + path * fraction,
                                normal,
                                struck: Struck::Other,
                            },
                            object: Some(target.handle),
                            frame: targets
                                .iter()
                                .position(|other| other.handle == target.handle)
                                .and_then(|index| frames.get(index).copied().flatten()),
                        })
                    }
                    _ => level.map(|hit| Contact {
                        hit,
                        object: None,
                        frame: None,
                    }),
                }
            });
            match flew {
                Flew::On => {
                    poses.push((flight.handle, flight.transform()));
                    true
                }
                Flew::Hit(impact) => {
                    fired.impacts.push(impact);
                    objects::remove(world, flight.handle);
                    false
                }
                Flew::Detonated(detonation) => {
                    fired.detonations.push(detonation);
                    objects::remove(world, flight.handle);
                    false
                }
                Flew::Spent => {
                    objects::remove(world, flight.handle);
                    false
                }
            }
        });
        world.insert_resource(ProjectilePoses(poses));
        fired
    }

    /// Its melee: the nearest biped within reach of its eyes along `aim`,
    /// before the level, struck with its weapon's melee damage.
    fn melee(&mut self, world: &mut World, eyes: Vec3, aim: Quat) -> Option<Impact> {
        let carried = self.inventory.current()?;
        let definition = self.armory.get(carried.armory)?;
        let weapon = &definition.weapon;
        if weapon.flags & Weapon::PREVENTS_MELEE_ATTACK != 0 {
            return None;
        }
        self.melee_ticks = MELEE_RECOVERY_TICKS;
        let damage = weapon.melee_damage.clone()?;
        let targets = damage::targets(world, self.handle);
        // (a fan of rays about the aim, the nearest hit taken)
        let mut nearest: Option<(f32, damage::Target, Vec3, Vec3)> = None;
        for step in 0..MELEE_RAYS * MELEE_RAYS {
            let across =
                |index: u32| (index as f32 / (MELEE_RAYS - 1) as f32 - 0.5) * 2.0 * MELEE_SPREAD;
            let local = Vec3::new(1.0, across(step % MELEE_RAYS), across(step / MELEE_RAYS));
            let path = aim * local.normalize() * MELEE_RANGE;
            let reach = match self.physics.cast_ray(eyes, path, Layers::LEVEL) {
                Ok(Some(hit)) => hit.fraction,
                _ => 1.0,
            };
            if let Some((target, fraction, normal)) = damage::first_target(&targets, eyes, path) {
                if fraction < reach && nearest.is_none_or(|(best, ..)| fraction < best) {
                    nearest = Some((fraction, target, path, normal));
                }
            }
        }
        let (fraction, target, path, normal) = nearest?;
        Some(Impact {
            projectile: definition.tag.clone(),
            owner: self.handle,
            damage: Some(damage),
            point: eyes + path * fraction,
            normal,
            direction: path.normalize_or_zero(),
            struck: Struck::Other,
            object: Some(target.handle),
        })
    }

    /// Takes the rounds of weapons it walks over that it carries, and
    /// with `action`, picks up the nearest other, dropping the one in its
    /// hands if its hands are full.
    fn pick_up(&mut self, world: &mut World, action: bool) {
        let feet = self.mover.position;
        let mut near: Vec<(f32, ObjectHandle, usize, Option<WeaponState>)> = world
            .query::<(&ObjectHandle, &Transform, &Object, Option<&WeaponItem>)>()
            .iter(world)
            .filter(|(_, _, object, _)| object.kind == ObjectKind::Weapon)
            .filter_map(|(handle, transform, object, item)| {
                let distance = transform.position.distance(feet);
                let index = self.armory.find(&object.tag)?;
                (distance <= armory::PICKUP_RADIUS)
                    .then(|| (distance, *handle, index, item.map(|item| item.0.clone())))
            })
            .collect();
        // (nearest first; at the same distance, by slot)
        near.sort_by(|a, b| a.0.total_cmp(&b.0).then(a.1.cmp(&b.1)));
        for (_, handle, index, item) in &near {
            let Some(carried) = self
                .inventory
                .weapons
                .iter_mut()
                .find(|carried| carried.armory == *index)
            else {
                continue;
            };
            let Some(definition) = self.armory.get(*index) else {
                continue;
            };
            let mut item = match item.clone() {
                Some(item) => item,
                None => match WeaponState::new(&definition.weapon) {
                    Ok(item) => item,
                    Err(_) => continue,
                },
            };
            if armory::take_rounds(&mut carried.state, &mut item, &definition.weapon) > 0 {
                if let Some(entity) = world
                    .get_resource::<objects::ObjectTable>()
                    .and_then(|table| table.entity(*handle))
                {
                    world.entity_mut(entity).insert(WeaponItem(item));
                }
            }
        }
        if !action {
            return;
        }
        let Some((_, handle, index, item)) = near
            .into_iter()
            .find(|(_, _, index, _)| !self.inventory.carries(*index))
        else {
            return;
        };
        let Some(definition) = self.armory.get(index) else {
            return;
        };
        let state = match item {
            Some(item) => item,
            None => match WeaponState::new(&definition.weapon) {
                Ok(state) => state,
                Err(_) => return,
            },
        };
        objects::remove(world, handle);
        let dropped = self.inventory.take(
            Carried {
                armory: index,
                state,
            },
            &self.armory,
        );
        if let Some(dropped) = dropped {
            self.drop_weapon(world, dropped);
        }
    }

    /// Puts a weapon down at its feet, keeping its rounds.
    fn drop_weapon(&self, world: &mut World, dropped: Carried) {
        let Some(definition) = self.armory.get(dropped.armory) else {
            return;
        };
        let transform = Transform {
            position: self.mover.position,
            rotation: Quat::from_rotation_z(self.yaw),
            scale: 1.0,
        };
        let spawned = objects::spawn(
            world,
            ObjectSpawn {
                kind: ObjectKind::Weapon,
                tag: definition.tag.clone(),
                transform,
                name: None,
                permutation: None,
                team: None,
            },
        );
        if let Ok(handle) = spawned {
            if let Some(entity) = world
                .get_resource::<objects::ObjectTable>()
                .and_then(|table| table.entity(handle))
            {
                world.entity_mut(entity).insert(WeaponItem(dropped.state));
            }
        }
    }

    /// Its state, for the world's hash.
    pub(crate) fn hash(&self, hasher: &mut WorldHasher) {
        let mover = &self.mover;
        for value in mover
            .position
            .to_array()
            .into_iter()
            .chain(mover.velocity.to_array())
            .chain([mover.crouch, self.yaw, self.pitch])
        {
            hasher.f32(value);
        }
        hasher.u64(u64::from(mover.airborne));
        hasher.u64(match self.view {
            View::FirstPerson => 0,
            View::ThirdPerson => 1,
        });
        hasher.u64(u64::from(self.held.0));
        hasher.u64(u64::from(self.melee_ticks));
        self.inventory.hash(hasher);
        hasher.u64(self.flights.len() as u64);
        for flight in &self.flights {
            flight.hash(hasher);
        }
    }
}

/// An object's transform, if it is in the world.
fn frame(world: &World, handle: ObjectHandle) -> Option<Transform> {
    let entity = world
        .get_resource::<crate::objects::ObjectTable>()?
        .entity(handle)?;
    world.get::<Transform>(entity).copied()
}

/// The biped's object: its feet, turned to its facing.
fn body_transform(position: Vec3, yaw: f32) -> Transform {
    Transform {
        position,
        rotation: Quat::from_rotation_z(yaw),
        scale: 1.0,
    }
}

/// A look's orientation: a yaw about Z, then a pitch about the camera's Y
/// (the free camera's).
fn look_rotation(yaw: f32, pitch: f32) -> Quat {
    Quat::from_rotation_z(yaw) * Quat::from_rotation_y(-pitch)
}

/// Where the player's projectiles' objects go this tick.
#[derive(Resource, Clone, Debug, Default)]
pub(crate) struct ProjectilePoses(pub Vec<(ObjectHandle, Transform)>);

/// Puts the player's projectiles' objects where their flight left them.
pub(crate) fn move_projectiles(
    poses: Option<Res<ProjectilePoses>>,
    table: Res<crate::objects::ObjectTable>,
    mut transforms: Query<&mut Transform>,
) {
    let Some(poses) = poses else { return };
    for (handle, pose) in &poses.0 {
        if let Some(mut transform) = table
            .entity(*handle)
            .and_then(|entity| transforms.get_mut(entity).ok())
        {
            *transform = *pose;
        }
    }
}

/// Puts the player's object where its step left it, before the animations
/// pose its nodes from there.
pub(crate) fn move_player(
    pose: Option<Res<PlayerPose>>,
    table: Res<crate::objects::ObjectTable>,
    mut transforms: Query<&mut Transform>,
) {
    let Some(pose) = pose else { return };
    let Some(entity) = table.entity(pose.handle) else {
        return;
    };
    if let Ok(mut transform) = transforms.get_mut(entity) {
        *transform = pose.body;
    }
}

/// Where a shot's effect is made, from the eyes and the aim: by the
/// weapon's ejection port, to the right of and below the view, facing
/// right and up, as the first-person weapons' markers are (the sim has no
/// first-person markers to put it at).
fn ejection(effect: TagRef, eyes: Vec3, aim: Quat, velocity: Vec3) -> Ejection {
    Ejection {
        effect,
        position: eyes + aim * Vec3::new(0.2, -0.08, -0.08),
        rotation: aim
            * Quat::from_rotation_z(-std::f32::consts::FRAC_PI_2)
            * Quat::from_rotation_y(-0.5),
        velocity,
    }
}
