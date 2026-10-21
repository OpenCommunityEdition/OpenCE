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
    AnimationNode, Level, Model, ObjectKind, Physics, TagRef, Vehicle, VehicleSeat, Weapon,
};

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
use crate::player::{self, Arms, Player, Seated};
use crate::vehicle::{self, VehicleControls, VehicleLevel, VehicleState};
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
}

/// The level's vehicles, and the vehicles in the world.
#[derive(Clone, Debug, Default, PartialEq)]
pub struct Rides {
    kinds: Vec<RideKind>,
    rides: Vec<Ride>,
    /// The player's unit's centre, from its origin: its bounding offset
    /// from its standing pelvis.
    centre: Vec3,
}

/// Where the vehicles' objects go this tick.
#[derive(Resource, Clone, Debug, Default)]
pub(crate) struct RidePoses(pub Vec<(ObjectHandle, Transform)>);

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
                        SeatPlace {
                            flags: seat.flags,
                            marker: Vec3::from_array(matrix.position),
                            entrance: Vec3::from_array(matrix.transform_point(root)),
                            fits: !seat.label.is_empty() && unit_seat.is_some(),
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
                Some(RideKind {
                    tag: object.tag.clone(),
                    vehicle,
                    physics,
                    seats,
                    turret,
                })
            })
            .collect();
        Rides {
            kinds,
            rides: Vec::new(),
            centre: Vec3::from_array(centre),
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
        let ride = &self.rides[index];
        let Some(place) = self
            .kinds
            .get(ride.kind)
            .and_then(|kind| kind.seats.get(seat))
        else {
            return false;
        };
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
        let mut poses = Vec::with_capacity(self.rides.len());
        for ride in &mut self.rides {
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
                if ridden || !ride.state.at_rest {
                    let level = VehicleLevel {
                        world: physics,
                        floor: 0.0,
                        ceiling: 0.0,
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
                }
            }
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
        world.insert_resource(RidePoses(poses));
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
                    let position = self
                        .seat(seated.vehicle, seated.seat)
                        .unwrap_or(ride.pose.position);
                    player.place_in_seat(Transform {
                        position,
                        ..ride.pose
                    });
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
fn default_pose(model: &Model) -> Vec<NodeMatrix> {
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

/// The matrix of `model`'s marker `name` (its first instance), from its
/// origin, with its nodes at `matrices`.
fn marker_matrix(model: &Model, matrices: &[NodeMatrix], name: &str) -> Option<NodeMatrix> {
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
