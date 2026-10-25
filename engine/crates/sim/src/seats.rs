//! Vehicles and their seats: each vehicle in the world moves by Halo's
//! mass points ([`crate::vehicle`]) a game tick (of 30 a second) every
//! other tick, and the player enters the nearest seat it can with the
//! action button, as `unit_find_nearby_seat` (units.c) chooses it, and
//! leaves it by its entrance. A driver drives it where it looks; a gunner
//! fires its gun; a passenger whose seat allows weapons fires its own. Its
//! seats power up while ridden and down once left (`unit_update_seats`),
//! and a vehicle destroyed throws its riders out and takes no more.

use bevy_ecs::prelude::*;
use glam::{Quat, Vec3};
use opence_content::{
    AnimationNode, BspWater, Level, MaterialPhysics, Model, ObjectKind, Physics, TagRef, Vehicle,
    VehicleAnimations, VehicleKind, VehicleSeat, Weapon,
};
use std::sync::Arc;

use opence_physics::{Layers, LevelMesh, SurfaceMaterials};

use crate::animation::{
    base_orientations, model_orientations, node_matrices, standing_animation, NodeMatrix,
    Orientation, MAX_NODES,
};
use crate::armory::Armory;
use crate::clock;
use crate::damage::Vitality;
use crate::hash::WorldHasher;
use crate::input::TickInput;
use crate::objects::{Object, ObjectHandle, ObjectTable};
use crate::player::{self, Arms, Player, SeatCamera, Seated};
use crate::random::RandomSeed;
use crate::vehicle::{self, VehicleControls, VehicleLevel, VehicleState};
use crate::vehicle_animation::{self, VehicleMotion, SUSPENSIONS};
use crate::weapon::WeaponState;
use crate::Transform;

/// How far a unit's centre may be from a seat's entrance or its marker
/// and still enter it (units.c's).
pub const ENTRY_RADIUS: f32 = 1.0;
/// The seat animation a unit enters with (`_unit_seat_animation_seat_enter`).
const SEAT_ENTER: usize = 7;
/// How much nearer a seat that is not a driver's must be to win over a
/// driver's (units.c's).
const DRIVER_PREFERENCE: f32 = 1.5;
/// The marker a vehicle's gun fires from.
const MUZZLE: &str = "primary trigger";
/// The engine's ticks to a game tick.
const TICKS_PER_GAME_TICK: u64 = (clock::TICKS_PER_SECOND / 30) as u64;

/// A seat of a kind of vehicle, from the vehicle's origin at its default
/// pose.
#[derive(Clone, Debug, PartialEq)]
struct SeatPlace {
    /// Its flags ([`VehicleSeat::DRIVER`] and the rest).
    flags: u32,
    /// Where its rider sits.
    marker: Vec3,
    /// Where its rider enters from and leaves to: the seat's marker moved
    /// by the first frame of the rider's animation of entering it
    /// (`unit_get_seat_entrance_point`).
    entrance: Vec3,
    /// The player's animation graph has the seat's label, so it can sit
    /// in it.
    fits: bool,
    /// The seat's label: the rider's seat in its animation graph.
    label: String,
    /// Where its camera follows from: its camera marker, or the vehicle's
    /// origin for a seat without one (`unit_get_camera_position`).
    camera: Vec3,
    /// Its camera track's control points, or the globals' for a seat
    /// without one.
    track: Arc<[[f32; 3]]>,
}

/// A kind of vehicle the level has.
#[derive(Clone, Debug, PartialEq)]
struct RideKind {
    tag: TagRef,
    vehicle: Vehicle,
    physics: Physics,
    seats: Vec<SeatPlace>,
    /// Its gun: the armory's weapon, its definition, and its muzzle from
    /// its origin.
    turret: Option<(usize, Weapon, Vec3)>,
    /// Its graph's vehicle animations: its overlays and suspensions.
    animations: Option<VehicleAnimations>,
    /// Its hover and jet thrusters, from its origin.
    thrusters: Thrusters,
}

/// A vehicle in the world.
#[derive(Clone, Debug, PartialEq)]
pub struct Ride {
    /// Its object.
    pub handle: ObjectHandle,
    kind: usize,
    /// Its movement.
    pub state: VehicleState,
    /// Its driver's and its gunner's seats' power, from 0 to 1.
    pub power: [f32; 2],
    /// Its gun, if it has one.
    pub turret: Option<WeaponState>,
    /// Its origin and rotation after the game tick before the last, which
    /// the tick between two game ticks is halfway from.
    previous: (Vec3, Quat),
    /// Where its object was put this tick.
    pub pose: Transform,
    /// Each suspension's travel, 0 (extended) to 255 (compressed).
    pub suspension: [u8; SUSPENSIONS],
}

/// What the current structure BSP's surfaces are made of, for what
/// strikes them (projectiles' responses).
#[derive(Resource, Clone, Debug, Default)]
pub struct LevelSurfaces(pub Option<Arc<SurfaceMaterials>>);

impl LevelSurfaces {
    /// The material type of the surface at `point` (-1: none).
    pub fn at(&self, point: Vec3) -> i16 {
        self.0.as_ref().map_or(-1, |surfaces| surfaces.at(point))
    }
}

/// The level's vehicles, and the vehicles in the world.
#[derive(Clone, Debug, Default, PartialEq)]
pub struct Rides {
    kinds: Vec<RideKind>,
    rides: Vec<Ride>,
    /// The player's unit's centre, from its origin: its bounding offset
    /// from its standing pelvis.
    centre: Vec3,
    // (the current structure BSP's water, and its vehicles' floor and
    // ceiling)
    water: Option<BspWater>,
    floor: f32,
    ceiling: f32,
    // (what the current structure BSP's surfaces are made of, and the
    // globals' materials' physics)
    surfaces: Option<Arc<SurfaceMaterials>>,
    materials: Vec<MaterialPhysics>,
    // (each player last seen seated: the vehicle and the tick)
    left: Vec<(ObjectHandle, ObjectHandle, u64)>,
    // (the globals' minimum_vehicle_flipping_angle)
    flipping_angle: f32,
    // (the mass points that slipped in the last tick)
    slips: Vec<VehicleSlip>,
    // (the thrusters' blasts on the ground in the last tick, and the
    // local random seed that spreads them)
    thrusts: Vec<VehicleThrust>,
    thrust_seed: RandomSeed,
}

/// Where a vehicle's thruster's blast met the ground in a tick, and the
/// vehicle's effect it plays there.
#[derive(Clone, Debug, PartialEq)]
pub struct VehicleThrust {
    /// The vehicle.
    pub vehicle: ObjectHandle,
    /// Its effect.
    pub effect: TagRef,
    /// The effect's locations by name: a point and the way it faces.
    pub locations: Vec<(&'static str, Vec3, Vec3)>,
    /// Its scale, 0 to 1: how near and how hard the blast is.
    pub scale: f32,
}

/// A vehicle's mass point slipping on the ground in a tick, and the
/// material effects it plays.
#[derive(Clone, Debug, PartialEq)]
pub struct VehicleSlip {
    /// The vehicle.
    pub vehicle: ObjectHandle,
    /// Its material effects.
    pub material_effects: TagRef,
    /// Where and how hard it slipped.
    pub slip: vehicle::Slip,
}

/// A player a vehicle ran into hard enough to hurt.
#[derive(Clone, Copy, Debug, PartialEq)]
pub(crate) struct Strike {
    /// The player's biped.
    pub victim: ObjectHandle,
    /// Who is to blame: the vehicle's driver, else the vehicle.
    pub owner: ObjectHandle,
}

/// A player enters a vehicle only while both are stiller than the root of
/// this, in world units (and radians) a game tick.
const ENTRY_STILLNESS: f32 = 0.01;

/// The game ticks a vehicle does not hurt the player who left it.
const LEFT_GRACE: u64 = 90;

/// A vehicle faster than this (world units a game tick) hurts whom it
/// runs into...
const STRIKE_SPEED: f32 = 0.066_666_67;

/// ...as does one whose velocity differs from theirs by more than the
/// root of this.
const STRIKE_RELATIVE: f32 = 0.001_111_111;

/// How much the push of a vehicle running into a player is lifted.
const STRIKE_LIFT: f32 = 0.8;

/// The least speed a vehicle pushes a player it runs into at.
const STRIKE_LEAST_SPEED: f32 = 0.1;

/// Where the vehicles' objects go this tick.
#[derive(Resource, Clone, Debug, Default)]
pub(crate) struct RidePoses(pub Vec<(ObjectHandle, Transform)>);

/// What the vehicles' overlays are posed by this tick.
#[derive(Resource, Clone, Debug, Default)]
pub(crate) struct VehicleMotions(pub Vec<(ObjectHandle, VehicleMotion)>);

/// Puts the vehicles' objects where their movement left them, before the
/// animations pose their nodes from there.
pub(crate) fn move_rides(
    poses: Option<Res<RidePoses>>,
    table: Res<ObjectTable>,
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

impl Rides {
    /// The vehicles' mass points that slipped in the last tick.
    pub fn slips(&self) -> &[VehicleSlip] {
        &self.slips
    }

    /// Where the vehicles' thrusters' blasts met the ground in the last
    /// tick.
    pub fn thrusts(&self) -> &[VehicleThrust] {
        &self.thrusts
    }

    /// Each vehicle that moved this game tick meets those before it and
    /// those at rest (physics.c's unit collisions, after each vehicle's
    /// update), for their next steps.
    fn collide(&mut self, stepped: &[bool]) {
        for index in 0..self.rides.len() {
            if !stepped.get(index).copied().unwrap_or(false) {
                continue;
            }
            for other in 0..self.rides.len() {
                if other == index || !(other < index || self.rides[other].state.at_rest) {
                    continue;
                }
                let (low, high) = (index.min(other), index.max(other));
                let (before, after) = self.rides.split_at_mut(high);
                let (Some(low_ride), Some(high_ride)) = (before.get_mut(low), after.first_mut())
                else {
                    continue;
                };
                let (Some(low_kind), Some(high_kind)) = (
                    self.kinds.get(low_ride.kind),
                    self.kinds.get(high_ride.kind),
                ) else {
                    continue;
                };
                // (the one that moved first, as the game has it)
                let (mover, met) = if index == low {
                    (
                        (&mut low_ride.state, &low_kind.physics),
                        (&mut high_ride.state, &high_kind.physics),
                    )
                } else {
                    (
                        (&mut high_ride.state, &high_kind.physics),
                        (&mut low_ride.state, &low_kind.physics),
                    )
                };
                vehicle::collide(mover, met);
            }
        }
    }

    /// Gives the vehicle of `handle` a push of `acceleration` (world units
    /// a game tick; twice that if it `detonates`), as `vehicle_accelerate`
    /// does: it speeds, spins about its up, and wakes. False if it is no
    /// vehicle of the world's.
    pub(crate) fn push(
        &mut self,
        handle: ObjectHandle,
        acceleration: Vec3,
        detonates: bool,
    ) -> bool {
        let Some(ride) = self.rides.iter_mut().find(|ride| ride.handle == handle) else {
            return false;
        };
        if !acceleration.is_finite() {
            return true;
        }
        let acceleration = if detonates {
            acceleration * 2.0
        } else {
            acceleration
        };
        ride.state.velocity += acceleration;
        ride.state.angular_velocity += ride.state.up.cross(acceleration) * std::f32::consts::PI;
        ride.state.wake();
        true
    }

    /// Each moving vehicle running into a player on foot this game tick
    /// (`physics_compute_biped_collision`): the player is pushed up and
    /// away, at least as fast as the vehicle goes, and moved out as far
    /// again; the players it hurts. A vehicle's mass points stand for its
    /// collision model, which the level does not keep, and a player is
    /// struck where its pill's lower sphere meets one of them.
    pub(crate) fn strike(&mut self, world: &World, players: &mut [Player]) -> Vec<Strike> {
        let tick = world.resource::<crate::Tick>().0;
        for player in players.iter() {
            let Some(seated) = player.seated() else {
                continue;
            };
            self.left.retain(|(rider, ..)| *rider != player.handle());
            self.left.push((player.handle(), seated.vehicle, tick));
        }
        let grace = LEFT_GRACE * TICKS_PER_GAME_TICK;
        self.left
            .retain(|(.., seen)| tick.saturating_sub(*seen) <= grace);
        if !tick.is_multiple_of(TICKS_PER_GAME_TICK) {
            return Vec::new();
        }
        let mut strikes = Vec::new();
        for ride in &self.rides {
            let Some(kind) = self.kinds.get(ride.kind) else {
                continue;
            };
            if ride.state.at_rest {
                continue;
            }
            let state = &ride.state;
            let speed = state.velocity.length();
            let driver = players
                .iter()
                .find(|player| {
                    player.seated().is_some_and(|seated| {
                        seated.vehicle == ride.handle && seated.flags & VehicleSeat::DRIVER != 0
                    })
                })
                .map(Player::handle);
            for player in players.iter_mut() {
                if player.seated().is_some() || player.noclip() || destroyed(world, player.handle())
                {
                    continue;
                }
                let mover = player.mover();
                let radius = mover.radius();
                let base = mover.position + Vec3::Z * radius;
                let touching = vehicle::spheres(state, &kind.physics).any(|(centre, reach)| {
                    centre.distance_squared(base) < (reach + radius) * (reach + radius)
                });
                if !touching {
                    continue;
                }
                let away = (mover.position + self.centre - state.position).normalize_or_zero();
                let lifted = Vec3::new(away.x, away.y, away.z + STRIKE_LIFT).normalize_or_zero();
                let push = (lifted * speed.max(STRIKE_LEAST_SPEED) + state.velocity) * 0.5;
                let theirs = mover.velocity / vehicle::GAME_TICKS_PER_SECOND as f32;
                let left = self.left.iter().any(|(rider, vehicle, _)| {
                    *rider == player.handle() && *vehicle == ride.handle
                });
                let hurts = !left
                    && (speed > STRIKE_SPEED
                        || state.velocity.distance_squared(theirs) > STRIKE_RELATIVE);
                // (out of the way first, as the game fixes its pill there;
                // left where it is if it cannot stand there)
                player.shove(push * 2.0);
                player.push(push);
                if hurts {
                    strikes.push(Strike {
                        victim: player.handle(),
                        owner: driver.unwrap_or(ride.handle),
                    });
                }
            }
        }
        strikes
    }

    /// The player's unit's centre, from its origin, as its body is turned
    /// (Halo's bounding sphere's centre).
    pub fn centre(&self) -> Vec3 {
        self.centre
    }

    /// Moves the vehicles into `level`'s structure BSP `bsp`: its water,
    /// and the floor and ceiling its flyers keep between.
    pub fn set_bsp(&mut self, level: &Level, bsp: usize) {
        let structure = level.structure_bsps.get(bsp);
        self.water = structure.map(|structure| structure.water());
        self.floor = structure.map_or(0.0, |structure| structure.vehicle_floor);
        self.ceiling = structure.map_or(0.0, |structure| structure.vehicle_ceiling);
        self.surfaces = structure.map(|structure| {
            let types: Vec<i16> = structure
                .collision_materials
                .iter()
                .map(|material| material.material_type)
                .collect();
            Arc::new(SurfaceMaterials::new(
                &LevelMesh::from_collision(&structure.collision),
                &types,
            ))
        });
        self.materials = level.globals.materials.clone();
    }

    /// What the current structure BSP's surfaces are made of, shared.
    pub fn surfaces(&self) -> LevelSurfaces {
        LevelSurfaces(self.surfaces.clone())
    }

    /// `level`'s vehicles, for a player whose unit is `unit` and whose
    /// weapons are `armory`'s.
    pub fn for_level(level: &Level, unit: &TagRef, armory: &Armory) -> Rides {
        let player = level.object(unit);
        let model = |tag: Option<&TagRef>| {
            tag.and_then(|tag| level.model_index(tag))
                .and_then(|index| level.models.get(index))
        };
        let player_model = model(player.and_then(|player| player.model.as_ref()));
        let graph = player
            .and_then(|player| player.animation_graph.as_ref())
            .and_then(|tag| level.animation_graph(tag));
        // (an animation's root node at its first frame, posing the
        // player's model)
        let root_of = |index: u16| {
            let animation = graph?.animations.get(usize::from(index))?;
            let count = player_model
                .map_or(0, |model| model.nodes.len())
                .max(usize::from(animation.node_count))
                .clamp(1, MAX_NODES);
            let mut nodes = vec![Orientation::REST; count];
            base_orientations(player_model, animation, 0, &mut nodes);
            nodes.first().copied()
        };
        // (Halo's bounding sphere is the bounding offset from the first
        // node, the pelvis, not from the feet: the standing pelvis here)
        let offset = player.map_or([0.0; 3], |player| player.bounding_offset);
        let centre = graph
            .and_then(|graph| root_of(standing_animation(graph, true)))
            .map_or(offset, |root| {
                NodeMatrix::from_orientation(&root).transform_point(offset)
            });
        let kinds = level
            .objects
            .iter()
            .filter(|object| object.kind == ObjectKind::Vehicle)
            .filter_map(|object| {
                let vehicle = object.vehicle.clone()?;
                let physics = level
                    .physics
                    .iter()
                    .find(|physics| Some(&physics.tag) == object.physics.as_ref())?
                    .clone();
                let render = model(object.model.as_ref());
                let matrices = render.map(default_pose).unwrap_or_default();
                let marker =
                    |name: &str| render.and_then(|render| marker_matrix(render, &matrices, name));
                let seats: Vec<SeatPlace> = vehicle
                    .seats
                    .iter()
                    .map(|seat| {
                        let matrix = marker(&seat.marker).unwrap_or(NodeMatrix::IDENTITY);
                        let unit_seat = graph.and_then(|graph| {
                            graph
                                .unit_seats
                                .iter()
                                .find(|unit_seat| unit_seat.label.eq_ignore_ascii_case(&seat.label))
                        });
                        // (the root node of the entering animation's first
                        // frame, in the seat marker's frame)
                        let root = unit_seat
                            .and_then(|unit_seat| {
                                unit_seat.animations.get(SEAT_ENTER).copied().flatten()
                            })
                            .and_then(root_of)
                            .map_or([0.0; 3], |root| root.translation);
                        let track = if seat.camera_track.is_empty() {
                            &level.globals.camera_track
                        } else {
                            &seat.camera_track
                        };
                        SeatPlace {
                            flags: seat.flags,
                            marker: Vec3::from_array(matrix.position),
                            entrance: Vec3::from_array(matrix.transform_point(root)),
                            fits: !seat.label.is_empty() && unit_seat.is_some(),
                            label: seat.label.clone(),
                            camera: Some(seat.camera_marker.as_str())
                                .filter(|name| !name.is_empty())
                                .and_then(marker)
                                .map_or(Vec3::ZERO, |matrix| Vec3::from_array(matrix.position)),
                            track: track.as_slice().into(),
                        }
                    })
                    .collect();
                let turret = vehicle
                    .initial_weapons
                    .first()
                    .and_then(|tag| armory.find(tag))
                    .and_then(|index| Some((index, armory.get(index)?.weapon.clone())))
                    .map(|(index, weapon)| {
                        let gunner = vehicle
                            .seats
                            .iter()
                            .zip(&seats)
                            .find(|(seat, _)| seat.flags & VehicleSeat::GUNNER != 0)
                            .map_or(Vec3::ZERO, |(_, place)| place.marker);
                        let muzzle = marker(MUZZLE)
                            .map_or(gunner, |matrix| Vec3::from_array(matrix.position));
                        (index, weapon, muzzle)
                    });
                let animations = object
                    .animation_graph
                    .as_ref()
                    .and_then(|tag| level.animation_graph(tag))
                    .and_then(|graph| graph.vehicle.clone());
                let thrusters = render.map_or_else(Thrusters::default, |render| Thrusters {
                    hover: marker_matrices(render, &matrices, "hover thrusters"),
                    jet: marker_matrices(render, &matrices, "jet thrusters"),
                });
                Some(RideKind {
                    tag: object.tag.clone(),
                    vehicle,
                    physics,
                    seats,
                    turret,
                    animations,
                    thrusters,
                })
            })
            .collect();
        Rides {
            kinds,
            rides: Vec::new(),
            centre: Vec3::from_array(centre),
            flipping_angle: level.globals.player.minimum_vehicle_flipping_angle,
            ..Rides::default()
        }
    }

    /// The vehicles in the world, in the order of their handles.
    pub fn rides(&self) -> &[Ride] {
        &self.rides
    }

    /// The vehicle of `handle`, if it is in the world.
    pub fn ride(&self, handle: ObjectHandle) -> Option<&Ride> {
        self.rides.iter().find(|ride| ride.handle == handle)
    }

    /// Where the rider of seat `seat` of `handle` enters from and leaves
    /// to, in the world.
    pub fn entrance(&self, handle: ObjectHandle, seat: usize) -> Option<Vec3> {
        let ride = self.ride(handle)?;
        let place = self.kinds.get(ride.kind)?.seats.get(seat)?;
        Some(at(&ride.pose, place.entrance))
    }

    /// The label of seat `seat` of `handle`: its rider's seat in the
    /// rider's animation graph.
    pub fn seat_label(&self, handle: ObjectHandle, seat: usize) -> Option<&str> {
        let ride = self.ride(handle)?;
        let place = self.kinds.get(ride.kind)?.seats.get(seat)?;
        Some(place.label.as_str()).filter(|label| !label.is_empty())
    }

    /// Where the rider of seat `seat` of `handle` sits, in the world.
    pub fn seat(&self, handle: ObjectHandle, seat: usize) -> Option<Vec3> {
        let ride = self.ride(handle)?;
        let place = self.kinds.get(ride.kind)?.seats.get(seat)?;
        Some(at(&ride.pose, place.marker))
    }

    /// Takes on the vehicles new to the world, where their objects were
    /// put, and lets go of those it no longer has.
    pub(crate) fn adopt(&mut self, world: &mut World) {
        let objects: Vec<(ObjectHandle, TagRef, Transform)> = world
            .query::<(&ObjectHandle, &Object, &Transform)>()
            .iter(world)
            .filter(|(_, object, _)| object.kind == ObjectKind::Vehicle)
            .map(|(handle, object, transform)| (*handle, object.tag.clone(), *transform))
            .collect();
        self.rides
            .retain(|ride| objects.iter().any(|(handle, ..)| *handle == ride.handle));
        for (handle, tag, transform) in objects {
            if self.ride(handle).is_some() {
                continue;
            }
            let Some(kind) = self.kinds.iter().position(|kind| kind.tag == tag) else {
                continue;
            };
            let definition = &self.kinds[kind];
            let state = VehicleState::placed(
                &definition.physics,
                transform.position,
                transform.rotation * Vec3::X,
                transform.rotation * Vec3::Z,
            );
            let turret = definition
                .turret
                .as_ref()
                .and_then(|(_, weapon, _)| WeaponState::new(weapon).ok());
            self.rides.push(Ride {
                handle,
                kind,
                previous: (transform.position, transform.rotation),
                state,
                power: [0.0; 2],
                turret,
                pose: transform,
                suspension: [0; SUSPENSIONS],
            });
        }
        self.rides.sort_by_key(|ride| ride.handle);
    }

    /// The seat nearest a unit centred at `centre` it can enter, as
    /// `unit_find_nearby_seat` chooses: within reach of its entrance or
    /// its marker, of a vehicle not destroyed, the player's graph having
    /// its label, not `taken`, and needing no driver unless the vehicle
    /// has one; a driver's seat is kept over another unless that one is
    /// half as near again.
    fn nearby_seat(&self, world: &World, centre: Vec3, taken: &[Seated]) -> Option<(usize, usize)> {
        let mut best: Option<(usize, usize, f32, bool)> = None;
        for (index, ride) in self.rides.iter().enumerate() {
            if destroyed(world, ride.handle) {
                continue;
            }
            let Some(kind) = self.kinds.get(ride.kind) else {
                continue;
            };
            let aboard: Vec<&Seated> = taken
                .iter()
                .filter(|seated| seated.vehicle == ride.handle)
                .collect();
            let driven = aboard
                .iter()
                .any(|seated| seated.flags & VehicleSeat::DRIVER != 0);
            for (seat, place) in kind.seats.iter().enumerate() {
                if !place.fits
                    || (place.flags & VehicleSeat::REQUIRES_DRIVER != 0 && !driven)
                    || aboard.iter().any(|seated| seated.seat == seat)
                {
                    continue;
                }
                let distance = at(&ride.pose, place.entrance)
                    .distance(centre)
                    .min(at(&ride.pose, place.marker).distance(centre));
                if distance >= ENTRY_RADIUS {
                    continue;
                }
                let driver = place.flags & VehicleSeat::DRIVER != 0;
                let scale = match best {
                    Some((.., true)) if !driver => DRIVER_PREFERENCE,
                    _ => 1.0,
                };
                if best.is_none_or(|(.., nearest, _)| distance * scale < nearest) {
                    best = Some((index, seat, distance, driver));
                }
            }
        }
        best.map(|(index, seat, ..)| (index, seat))
    }

    /// With the action button pressed, the player leaves its seat by its
    /// entrance, or enters the nearest it can; whether it did either.
    pub(crate) fn act(
        &mut self,
        world: &World,
        player: &mut Player,
        taken: &[Seated],
        pressed: bool,
    ) -> bool {
        if !pressed {
            return false;
        }
        if let Some(seated) = player.seated() {
            let exit = self
                .entrance(seated.vehicle, seated.seat)
                .unwrap_or(player.mover().position);
            player.stand(exit);
            return true;
        }
        let centre = player.mover().position + player.pose().body.rotation * self.centre;
        let Some((index, seat)) = self.nearby_seat(world, centre, taken) else {
            return false;
        };
        let flipping_angle = self.flipping_angle;
        let Some(ride) = self.rides.get_mut(index) else {
            return false;
        };
        let Some(kind) = self.kinds.get(ride.kind) else {
            return false;
        };
        let Some(place) = kind.seats.get(seat) else {
            return false;
        };
        if !ride.state.enterable(flipping_angle) {
            // (one lying over is turned back over, unless someone drives
            // it)
            let driven = taken.iter().any(|seated| {
                seated.vehicle == ride.handle && seated.flags & VehicleSeat::DRIVER != 0
            });
            return !driven
                && ride
                    .state
                    .start_upending(&kind.physics, player.mover().position);
        }
        // (and one is entered only by a player standing still, while it
        // is still; the game's measures the player's fall too, but the
        // mover leaves the ground walking down a slope, so only its speed
        // across the ground counts here)
        let speed =
            player.mover().velocity.truncate().extend(0.0) / vehicle::GAME_TICKS_PER_SECOND as f32;
        if speed.length_squared() >= ENTRY_STILLNESS
            || ride.state.angular_velocity.length_squared() >= ENTRY_STILLNESS
        {
            return false;
        }
        player.sit(
            Seated {
                vehicle: ride.handle,
                seat,
                flags: place.flags,
            },
            Transform {
                position: at(&ride.pose, place.marker),
                ..ride.pose
            },
            seat_camera(&ride.pose, place),
        );
        true
    }

    /// Sits `player` in seat `seat` of the vehicle `handle` at once, as
    /// the shell's `enter` does, wherever it stands; false for an object
    /// that is not a ride, a destroyed one, or a seat it does not have.
    pub(crate) fn seat_player(
        &self,
        world: &World,
        player: &mut Player,
        handle: ObjectHandle,
        seat: usize,
    ) -> bool {
        let Some(ride) = self.rides.iter().find(|ride| ride.handle == handle) else {
            return false;
        };
        let place = self
            .kinds
            .get(ride.kind)
            .and_then(|kind| kind.seats.get(seat));
        let Some(place) = place.filter(|_| !destroyed(world, handle)) else {
            return false;
        };
        player.set_noclip(false);
        player.sit(
            Seated {
                vehicle: handle,
                seat,
                flags: place.flags,
            },
            Transform {
                position: at(&ride.pose, place.marker),
                ..ride.pose
            },
            seat_camera(&ride.pose, place),
        );
        true
    }

    /// One tick of the vehicles: on a game tick, each powers its seats and
    /// moves, driven by the player if it drives it; between, each is put
    /// halfway from its last game tick to this. The player is put in its
    /// seat, or thrown out of a vehicle destroyed or gone.
    pub(crate) fn step(&mut self, world: &mut World, players: &mut [Player], inputs: &[TickInput]) {
        let tick = world.resource::<crate::Tick>().0;
        let game_tick = tick.is_multiple_of(TICKS_PER_GAME_TICK);
        let riders: Vec<(Option<Seated>, Quat)> = players
            .iter()
            .map(|player| (player.seated(), player.aim().1))
            .collect();
        let Some(guard) = players.first().map(Player::physics) else {
            return;
        };
        let physics = &*guard;
        self.slips.clear();
        self.thrusts.clear();
        let mut poses = Vec::with_capacity(self.rides.len());
        let mut motions = Vec::with_capacity(self.rides.len());
        let mut stepped = vec![false; self.rides.len()];
        for (index, ride) in self.rides.iter_mut().enumerate() {
            let Some(kind) = self.kinds.get(ride.kind) else {
                continue;
            };
            let dead = destroyed(world, ride.handle);
            let aboard = |seat: u32, not: u32| {
                riders.iter().enumerate().find(|(_, (seated, _))| {
                    seated.is_some_and(|seated| {
                        seated.vehicle == ride.handle
                            && seated.flags & seat != 0
                            && seated.flags & not == 0
                    })
                })
            };
            let driving = aboard(VehicleSeat::DRIVER, 0).filter(|_| !dead);
            let gunning = aboard(VehicleSeat::GUNNER, VehicleSeat::DRIVER).filter(|_| !dead);
            let ridden = !dead
                && riders
                    .iter()
                    .any(|(seated, _)| seated.is_some_and(|seated| seated.vehicle == ride.handle));
            if game_tick {
                ride.previous = (ride.state.origin(&kind.physics), ride.state.rotation());
                let driver = driving.is_some();
                let gunner = gunning.is_some();
                for (seat, (power, timing)) in ride
                    .power
                    .iter_mut()
                    .zip(&kind.vehicle.powered_seats)
                    .enumerate()
                {
                    let on = [driver, gunner][seat];
                    *power = if on {
                        (*power + 1.0 / (timing.powerup_time * 30.0)).min(1.0)
                    } else {
                        (*power - 1.0 / (timing.powerdown_time * 30.0)).max(0.0)
                    };
                    if !power.is_finite() {
                        *power = if on { 1.0 } else { 0.0 };
                    }
                }
                let driven = driving.and_then(|(index, (_, aim))| Some((inputs.get(index)?, aim)));
                let controls = match driven {
                    Some((input, aim)) => VehicleControls {
                        throttle: player::throttle(input),
                        facing: *aim * Vec3::X,
                        jump: input.buttons.contains(crate::input::Buttons::JUMP),
                        crouch: input.buttons.contains(crate::input::Buttons::CROUCH),
                        seat_power: ride.power,
                        ai: false,
                    },
                    None => VehicleControls {
                        facing: ride.state.forward,
                        seat_power: ride.power,
                        ..VehicleControls::default()
                    },
                };
                if ridden || !ride.state.at_rest || ride.state.upending.is_some() {
                    stepped[index] = true;
                    let level = VehicleLevel {
                        world: physics,
                        floor: self.floor,
                        ceiling: self.ceiling,
                        water: self.water.as_ref(),
                        surfaces: self.surfaces.as_deref(),
                        materials: &self.materials,
                    };
                    // (a step the physics refuses leaves it where it was)
                    let before = ride.state.clone();
                    if vehicle::game_tick(
                        &mut ride.state,
                        &kind.vehicle,
                        &kind.physics,
                        &controls,
                        level,
                    )
                    .is_err()
                    {
                        ride.state = before;
                    }
                    let slips = std::mem::take(&mut ride.state.slips);
                    if let Some(effects) = &kind.vehicle.material_effects {
                        self.slips.extend(slips.into_iter().map(|slip| VehicleSlip {
                            vehicle: ride.handle,
                            material_effects: effects.clone(),
                            slip,
                        }));
                    }
                    if let Some(effect) = &kind.vehicle.effect {
                        let blasts = kind.thrusters.blasts(
                            kind.vehicle.kind,
                            (ride.state.origin(&kind.physics), ride.state.rotation()),
                            (ride.state.hover, ride.state.thrust),
                            ride.power[0],
                            &mut self.thrust_seed,
                            |start, vector| {
                                let hit = physics.cast_ray(start, vector, Layers::LEVEL).ok()??;
                                Some((hit.fraction, hit.point, hit.normal))
                            },
                        );
                        self.thrusts
                            .extend(blasts.into_iter().map(|(locations, scale)| VehicleThrust {
                                vehicle: ride.handle,
                                effect: effect.clone(),
                                locations,
                                scale,
                            }));
                    }
                }
                if let Some(animations) = &kind.animations {
                    // (the ground below each wheel, a cast the physics
                    // refuses finding none)
                    vehicle_animation::update_suspension(
                        animations,
                        &kind.physics,
                        ride.state.origin(&kind.physics),
                        ride.state.forward,
                        ride.state.up,
                        &mut ride.suspension,
                        |start, vector| match physics.cast_ray(start, vector, Layers::LEVEL) {
                            Ok(Some(hit)) => hit.fraction,
                            _ => 1.0,
                        },
                    );
                }
            }
            motions.push((
                ride.handle,
                VehicleMotion {
                    turn: ride.state.turn,
                    speed: ride.state.speed,
                    wheel: ride.state.wheel,
                    velocity: ride.state.velocity,
                    forward: ride.state.forward,
                    up: ride.state.up,
                    suspension: ride.suspension,
                    maximum_forward_speed: kind.vehicle.maximum_forward_speed,
                    maximum_reverse_speed: kind.vehicle.maximum_reverse_speed,
                    wheel_circumference: kind.vehicle.wheel_circumference,
                },
            ));
            let current = (ride.state.origin(&kind.physics), ride.state.rotation());
            let (position, rotation) = if game_tick {
                (
                    ride.previous.0.lerp(current.0, 0.5),
                    ride.previous.1.slerp(current.1, 0.5),
                )
            } else {
                current
            };
            ride.pose = Transform {
                position,
                rotation,
                scale: ride.pose.scale,
            };
            poses.push((ride.handle, ride.pose));
        }
        if game_tick {
            self.collide(&stepped);
        }
        world.insert_resource(RidePoses(poses));
        world.insert_resource(VehicleMotions(motions));
        // (each player's seat moves its biped in the world it may share)
        drop(guard);
        for player in players.iter_mut() {
            let Some(seated) = player.seated() else {
                continue;
            };
            match self.ride(seated.vehicle) {
                Some(ride)
                    if !destroyed(world, ride.handle) && !destroyed(world, player.handle()) =>
                {
                    let place = self
                        .kinds
                        .get(ride.kind)
                        .and_then(|kind| kind.seats.get(seated.seat));
                    let position =
                        place.map_or(ride.pose.position, |place| at(&ride.pose, place.marker));
                    let camera = place.and_then(|place| seat_camera(&ride.pose, place));
                    player.place_in_seat(
                        Transform {
                            position,
                            ..ride.pose
                        },
                        camera,
                    );
                }
                _ => {
                    // (thrown out by its entrance, or where it sat)
                    let exit = self
                        .entrance(seated.vehicle, seated.seat)
                        .unwrap_or(player.mover().position);
                    player.stand(exit);
                }
            }
        }
    }

    /// What the player fights with in its seat: its own weapons on foot or
    /// in a seat that allows them (picking up only if `pick_up`), its
    /// vehicle's gun in a gunner's seat, else nothing.
    pub(crate) fn arms(&mut self, seated: Option<Seated>, pick_up: bool) -> Arms<'_> {
        let Some(seated) = seated else {
            return Arms::OnFoot { action: pick_up };
        };
        if seated.flags & VehicleSeat::GUNNER != 0 {
            if let Some(ride) = self
                .rides
                .iter_mut()
                .find(|ride| ride.handle == seated.vehicle)
            {
                let turret = self
                    .kinds
                    .get(ride.kind)
                    .and_then(|kind| kind.turret.as_ref())
                    .map(|(index, _, muzzle)| (*index, *muzzle));
                if let (Some((index, muzzle)), Some(state)) = (turret, ride.turret.as_mut()) {
                    return Arms::Mounted {
                        armory: index,
                        state,
                        muzzle: at(&ride.pose, muzzle),
                    };
                }
            }
        }
        if seated.flags & VehicleSeat::ALLOWS_WEAPONS != 0 {
            return Arms::Own;
        }
        Arms::Unarmed
    }

    /// Its state, for the world's hash.
    pub(crate) fn hash(&self, hasher: &mut WorldHasher) {
        hasher.u64(self.rides.len() as u64);
        for ride in &self.rides {
            hasher.u64(u64::from(ride.handle.index));
            hasher.u64(u64::from(ride.handle.salt));
            let state = &ride.state;
            for value in state
                .position
                .to_array()
                .into_iter()
                .chain(state.velocity.to_array())
                .chain(state.forward.to_array())
                .chain(state.up.to_array())
                .chain(state.angular_velocity.to_array())
                .chain(state.collision_force.to_array())
                .chain(state.collision_torque.to_array())
                .chain([state.speed, state.slide, state.turn])
                .chain(ride.power)
            {
                hasher.f32(value);
            }
            hasher.u64(u64::from(state.at_rest));
            match &ride.turret {
                Some(turret) => {
                    hasher.u64(1);
                    turret.hash(hasher);
                }
                None => hasher.u64(0),
            }
        }
    }
}

/// Where `local`, from an object's origin, is in the world.
fn at(transform: &Transform, local: Vec3) -> Vec3 {
    transform.position + transform.rotation * (local * transform.scale)
}

/// Whether `handle`'s object is destroyed.
fn destroyed(world: &World, handle: ObjectHandle) -> bool {
    world
        .get_resource::<ObjectTable>()
        .and_then(|table| table.entity(handle))
        .and_then(|entity| world.get::<Vitality>(entity))
        .is_some_and(|vitality| vitality.dead)
}

/// A model's nodes at its default pose, from its origin.
pub fn default_pose(model: &Model) -> Vec<NodeMatrix> {
    let links: Vec<AnimationNode> = model
        .nodes
        .iter()
        .map(|node| AnimationNode {
            name: node.name.clone(),
            parent: node.parent,
            first_child: node.first_child,
            next_sibling: node.next_sibling,
        })
        .collect();
    let mut orientations = vec![Orientation::REST; links.len()];
    model_orientations(model, &mut orientations);
    node_matrices(&links, &orientations, &NodeMatrix::IDENTITY)
}

/// The camera behind the vehicle at `pose` that the rider of `place`
/// sees from, if the seat has one (`director_desired_perspective`'s
/// third-person camera seat).
fn seat_camera(pose: &Transform, place: &SeatPlace) -> Option<SeatCamera> {
    (place.flags & VehicleSeat::THIRD_PERSON_CAMERA != 0).then(|| SeatCamera {
        marker: at(pose, place.camera),
        track: Arc::clone(&place.track),
    })
}

/// The matrices of every instance of `model`'s marker `name`, from its
/// origin, with its nodes at `matrices`.
fn marker_matrices(model: &Model, matrices: &[NodeMatrix], name: &str) -> Vec<NodeMatrix> {
    model
        .markers
        .iter()
        .filter(|marker| marker.name.eq_ignore_ascii_case(name))
        .flat_map(|marker| &marker.instances)
        .filter_map(|instance| {
            let node = matrices.get(usize::from(instance.node))?;
            Some(node.multiply(&NodeMatrix::from_orientation(&Orientation {
                rotation: instance.rotation,
                translation: instance.translation,
                scale: 1.0,
            })))
        })
        .collect()
}

/// A vehicle's thrusters (its model's hover and jet thrusters' markers),
/// from its origin.
#[derive(Clone, Debug, Default, PartialEq)]
struct Thrusters {
    hover: Vec<NodeMatrix>,
    jet: Vec<NodeMatrix>,
}

/// An effect's locations by name: a point and the way it faces.
type Locations = Vec<(&'static str, Vec3, Vec3)>;

impl Thrusters {
    /// Where the blasts of a vehicle of `kind` at `pose`, its hover and
    /// thrust at `lift` and its driver's seat at `power`, meet what `cast` finds (the fraction along the cast, the
    /// point and the normal): a scout's or a fighter's hover thrusters
    /// (`create_ghost_effect`), a plane's hover and jet thrusters
    /// (`create_pelican_effect`). Each is the effect's locations and its
    /// scale.
    fn blasts(
        &self,
        kind: VehicleKind,
        (origin, rotation): (Vec3, Quat),
        (hover_lift, thrust_lift): (f32, f32),
        power: f32,
        seed: &mut RandomSeed,
        mut cast: impl FnMut(Vec3, Vec3) -> Option<(f32, Vec3, Vec3)>,
    ) -> Vec<(Locations, f32)> {
        let plane = match kind {
            VehicleKind::AlienScout | VehicleKind::AlienFighter => false,
            VehicleKind::HumanPlane => true,
            _ => return Vec::new(),
        };
        if !plane && power <= 0.0 {
            return Vec::new();
        }
        // (fifteen hover thrusters at most; a plane's jets fill to sixteen)
        let hover = self.hover.len().min(15);
        let jets = if plane { 16 - hover } else { 0 };
        let mut blasts = Vec::new();
        for (index, marker) in self.hover[..hover]
            .iter()
            .chain(self.jet.iter().take(jets))
            .enumerate()
        {
            let position = origin + rotation * Vec3::from_array(marker.position);
            let forward = rotation * Vec3::from_array(marker.forward);
            // (the ghost's cone is fifteen, in radians, as the original has
            // it: any way at all; the pelican's a twelfth of a half turn)
            let outer = if plane {
                std::f32::consts::PI / 12.0
            } else {
                15.0
            };
            let direction = spread(forward, outer, seed);
            let vector = if plane {
                let lift = if index < hover {
                    hover_lift
                } else {
                    thrust_lift
                };
                direction * (lift * 6.0 + 2.0)
            } else {
                direction
            };
            let Some((fraction, point, normal)) = cast(position, vector) else {
                continue;
            };
            let reflected = direction - normal * (2.0 * direction.dot(normal));
            let mut locations = vec![
                ("incident", point, -direction),
                ("normal", point, normal),
                ("reflected", point, reflected),
            ];
            let scale = if plane {
                1.0 - fraction
            } else {
                locations.push(("midpoint", (position + point) * 0.5, reflected));
                (-forward.z * (1.0 - fraction) * power).clamp(0.0, 1.0)
            };
            if plane || scale > 0.0 {
                blasts.push((locations, scale));
            }
        }
        blasts
    }
}

/// `axis` turned at random by up to `outer` radians, about a way across
/// it at random (`seed_random_vector_in_cone3d`, its inner angle 0).
fn spread(axis: Vec3, outer: f32, seed: &mut RandomSeed) -> Vec3 {
    let z = 2.0 * seed.next_real() - 1.0;
    let around = std::f32::consts::TAU * seed.next_real();
    let flat = libm::sqrtf((1.0_f32 - z * z).max(0.0));
    let random = Vec3::new(flat * libm::cosf(around), flat * libm::sinf(around), z);
    let across = axis.cross(random).normalize_or_zero();
    if across == Vec3::ZERO {
        return axis;
    }
    Quat::from_axis_angle(across, outer * seed.next_real()) * axis
}

/// The matrix of `model`'s marker `name` (its first instance), from its
/// origin, with its nodes at `matrices`.
pub fn marker_matrix(model: &Model, matrices: &[NodeMatrix], name: &str) -> Option<NodeMatrix> {
    let marker = model
        .markers
        .iter()
        .find(|marker| marker.name.eq_ignore_ascii_case(name))?;
    let instance = marker.instances.first()?;
    let node = matrices.get(usize::from(instance.node))?;
    Some(node.multiply(&NodeMatrix::from_orientation(&Orientation {
        rotation: instance.rotation,
        translation: instance.translation,
        scale: 1.0,
    })))
}

#[cfg(test)]
mod tests {
    use super::*;

    /// A thruster `height` above the origin, blowing down.
    fn down(height: f32) -> NodeMatrix {
        NodeMatrix {
            forward: [0.0, 0.0, -1.0],
            left: [0.0, 1.0, 0.0],
            up: [1.0, 0.0, 0.0],
            position: [0.0, 0.0, height],
            scale: 1.0,
        }
    }

    /// Flat ground at z = 0: the fraction along the cast, the point and the
    /// normal where `vector` from `start` meets it.
    fn ground(start: Vec3, vector: Vec3) -> Option<(f32, Vec3, Vec3)> {
        if vector.z >= 0.0 {
            return None;
        }
        let fraction = start.z / -vector.z;
        (fraction <= 1.0).then(|| (fraction, start + vector * fraction, Vec3::Z))
    }

    #[test]
    fn a_scouts_hover_thrusters_blast_the_ground_only_while_it_is_powered() {
        let thrusters = Thrusters {
            hover: vec![down(0.5); 20],
            jet: Vec::new(),
        };
        let pose = (Vec3::ZERO, Quat::IDENTITY);
        let mut seed = RandomSeed::default();
        let mut casts = 0;
        let mut blasts = Vec::new();
        for _ in 0..100 {
            blasts.extend(thrusters.blasts(
                VehicleKind::AlienScout,
                pose,
                (0.0, 0.0),
                1.0,
                &mut seed,
                |start, vector| {
                    casts += 1;
                    // (a unit cast, as the original's)
                    assert!((vector.length() - 1.0).abs() < 1e-4);
                    ground(start, vector)
                },
            ));
        }
        // (fifteen thrusters at most; the cone is fifteen radians, so some
        // blow up and miss)
        assert_eq!(casts, 1500);
        assert!(
            !blasts.is_empty() && blasts.len() < 1500,
            "{}",
            blasts.len()
        );
        for (locations, scale) in &blasts {
            let names: Vec<&str> = locations.iter().map(|(name, _, _)| *name).collect();
            assert_eq!(names, ["incident", "normal", "reflected", "midpoint"]);
            assert!(*scale > 0.0 && *scale <= 1.0, "{scale}");
            assert_eq!(locations[1].2, Vec3::Z);
            assert!(locations[0].1.z.abs() < 1e-5, "{:?}", locations[0].1);
        }
        let unpowered = thrusters.blasts(
            VehicleKind::AlienScout,
            pose,
            (0.0, 0.0),
            0.0,
            &mut seed,
            ground,
        );
        assert!(unpowered.is_empty());
        let wheeled = thrusters.blasts(
            VehicleKind::HumanJeep,
            pose,
            (0.0, 0.0),
            1.0,
            &mut seed,
            ground,
        );
        assert!(wheeled.is_empty());
    }

    #[test]
    fn a_planes_hover_and_jet_thrusters_blast_the_ground_by_their_lift() {
        let thrusters = Thrusters {
            hover: vec![down(1.0)],
            jet: vec![down(1.0)],
        };
        let mut seed = RandomSeed::default();
        let mut lengths = Vec::new();
        let blasts = thrusters.blasts(
            VehicleKind::HumanPlane,
            (Vec3::ZERO, Quat::IDENTITY),
            (0.5, 0.0),
            0.0,
            &mut seed,
            |start, vector| {
                lengths.push(vector.length());
                ground(start, vector)
            },
        );
        // (hover 0.5 gives 5 units, thrust 0 gives 2; unpowered seats do not
        // stop a plane's)
        assert!((lengths[0] - 5.0).abs() < 1e-4 && (lengths[1] - 2.0).abs() < 1e-4);
        assert_eq!(blasts.len(), 2);
        for (locations, scale) in &blasts {
            assert_eq!(locations.len(), 3);
            assert!(*scale > 0.0 && *scale < 1.0, "{scale}");
        }
        // (a unit up, within a twelfth of a half turn of straight down)
        assert!(blasts[0].1 > 1.0 - 1.0 / 5.0 / 0.96 && blasts[0].1 <= 1.0 - 1.0 / 5.0);
    }
}
