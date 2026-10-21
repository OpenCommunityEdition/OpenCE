//! Vehicles: their own part of their definitions, their seats, and the
//! physics (`phys`) they move by, a set of mass points that each feel the
//! ground, the water and the air. Each value is as the tag holds it and
//! the game works with it: speeds in world units a tick and accelerations
//! in world units a tick a tick, of the game's 30 ticks a second (the
//! controllers turn them into the engine's), and turns as the game's
//! controllers take them.

use rkyv::{Archive, Deserialize, Serialize};

use crate::TagRef;

/// How a vehicle moves, as its tag's type says.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash, Archive, Serialize, Deserialize)]
pub enum VehicleKind {
    /// Treads (the Scorpion).
    HumanTank,
    /// Wheels that steer (the Warthog).
    HumanJeep,
    /// A boat.
    HumanBoat,
    /// A dropship (the Pelican).
    HumanPlane,
    /// A hovering scout (the Ghost).
    AlienScout,
    /// A flyer (the Banshee).
    AlienFighter,
    /// A turret that stays where it is.
    Turret,
}

impl VehicleKind {
    /// Every kind, in the order of the tag's type field.
    pub const ALL: [VehicleKind; 7] = [
        VehicleKind::HumanTank,
        VehicleKind::HumanJeep,
        VehicleKind::HumanBoat,
        VehicleKind::HumanPlane,
        VehicleKind::AlienScout,
        VehicleKind::AlienFighter,
        VehicleKind::Turret,
    ];
}

/// A vehicle's own part of its definition.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct Vehicle {
    /// Its flags ([`Vehicle::CAUSES_COLLISION_DAMAGE`] and the rest).
    pub flags: u32,
    /// How it moves.
    pub kind: VehicleKind,
    /// Its fastest forward.
    pub maximum_forward_speed: f32,
    /// Its fastest backward.
    pub maximum_reverse_speed: f32,
    /// How fast it gains speed.
    pub speed_acceleration: f32,
    /// How fast it loses speed.
    pub speed_deceleration: f32,
    /// Its sharpest turn to the left.
    pub maximum_left_turn: f32,
    /// Its sharpest turn to the right (negative).
    pub maximum_right_turn: f32,
    /// Its wheels' circumference.
    pub wheel_circumference: f32,
    /// How fast it turns (a fighter's).
    pub turn_rate: f32,
    /// The speed past which it is drawn blurred.
    pub blur_speed: f32,
    /// What its model's A, B, C and D functions follow.
    pub function_modes: [i16; 4],
    /// Its fastest slide to the left.
    pub maximum_left_slide: f32,
    /// Its fastest slide to the right.
    pub maximum_right_slide: f32,
    /// How fast it gains a slide.
    pub slide_acceleration: f32,
    /// How fast it loses a slide.
    pub slide_deceleration: f32,
    /// The slowest spin that flips it.
    pub minimum_flipping_angular_velocity: f32,
    /// The fastest spin it flips with.
    pub maximum_flipping_angular_velocity: f32,
    /// The yaw its fixed gun points at.
    pub fixed_gun_yaw: f32,
    /// The pitch its fixed gun points at.
    pub fixed_gun_pitch: f32,
    /// The sound of its suspension.
    pub suspension_sound: Option<TagRef>,
    /// The sound of its crashes.
    pub crash_sound: Option<TagRef>,
    /// What its wheels and treads leave on the ground.
    pub material_effects: Option<TagRef>,
    /// The effect it plays.
    pub effect: Option<TagRef>,
    /// Its seats, in the tag's order.
    pub seats: Vec<VehicleSeat>,
}

impl Vehicle {
    /// Its physics wakes while it has speed.
    pub const SPEED_WAKES_PHYSICS: u32 = 1 << 0;
    /// Its physics wakes while it turns.
    pub const TURN_WAKES_PHYSICS: u32 = 1 << 1;
    /// Its physics wakes while its driver's seat has power.
    pub const DRIVER_POWER_WAKES_PHYSICS: u32 = 1 << 2;
    /// Its physics wakes while its gunner's seat has power.
    pub const GUNNER_POWER_WAKES_PHYSICS: u32 = 1 << 3;
    /// A throttle against its speed brakes it.
    pub const CONTROL_OPPOSITE_SPEED_SETS_BRAKE: u32 = 1 << 4;
    /// Its physics wakes while it slides.
    pub const SLIDE_WAKES_PHYSICS: u32 = 1 << 5;
    /// What it runs into is damaged.
    pub const CAUSES_COLLISION_DAMAGE: u32 = 1 << 7;
}

/// A seat of a vehicle (the unit part's seats block).
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct VehicleSeat {
    /// Its flags ([`VehicleSeat::DRIVER`] and the rest).
    pub flags: u32,
    /// Its animation label (the animation graph's seat of that name).
    pub label: String,
    /// The marker its rider sits at.
    pub marker: String,
    /// How much its rider's push moves the vehicle.
    pub acceleration_scale: [f32; 3],
    /// How fast its rider turns.
    pub yaw_rate: f32,
    /// How fast its rider looks up and down.
    pub pitch_rate: f32,
    /// The marker its camera is at.
    pub camera_marker: String,
    /// The marker its camera is at under water.
    pub camera_submerged_marker: String,
    /// The pitch its camera levels to.
    pub pitch_auto_level: f32,
    /// Its camera's lowest pitch.
    pub pitch_range_minimum: f32,
    /// Its camera's highest pitch.
    pub pitch_range_maximum: f32,
    /// The furthest its rider turns left of the vehicle (0: no limit).
    pub yaw_minimum: f32,
    /// The furthest its rider turns right of the vehicle (0: no limit).
    pub yaw_maximum: f32,
    /// The character that rides it from the start.
    pub built_in_actor: Option<TagRef>,
}

impl VehicleSeat {
    /// Its rider is not drawn.
    pub const INVISIBLE: u32 = 1 << 0;
    /// Its rider drives.
    pub const DRIVER: u32 = 1 << 2;
    /// Its rider fires the vehicle's weapon.
    pub const GUNNER: u32 = 1 << 3;
    /// Its camera is behind the vehicle.
    pub const THIRD_PERSON_CAMERA: u32 = 1 << 4;
    /// Its camera is behind the vehicle as its rider gets in.
    pub const THIRD_PERSON_ON_ENTER: u32 = 1 << 6;
    /// Its camera is its rider's.
    pub const FIRST_PERSON_CAMERA: u32 = 1 << 7;
    /// Those who do not fight may ride in it.
    pub const ALLOWS_NONCOMBATANTS: u32 = 1 << 10;
}

/// A physics (`phys`) tag: the mass points an object moves by, and how
/// the ground, the water and the air hold them back.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct Physics {
    /// Its tag.
    pub tag: TagRef,
    /// The radius its mass is spread over.
    pub radius: f32,
    /// How hard it is to turn, as a scale.
    pub moment_scale: f32,
    /// Its mass.
    pub mass: f32,
    /// Its centre of mass, from its origin.
    pub center_of_mass: [f32; 3],
    /// Its density.
    pub density: f32,
    /// How much gravity pulls it, as a scale.
    pub gravity_scale: f32,
    /// The ground's friction on it.
    pub ground_friction: f32,
    /// How far into the ground it sinks.
    pub ground_depth: f32,
    /// How much of its speed into the ground the ground takes.
    pub ground_damp_fraction: f32,
    /// The steepest ground its mass points stand on (a normal's z).
    pub ground_normal_k1: f32,
    /// The steepest ground they touch at all.
    pub ground_normal_k0: f32,
    /// The water's friction on it.
    pub water_friction: f32,
    /// How deep it floats.
    pub water_depth: f32,
    /// The water's density.
    pub water_density: f32,
    /// The air's friction on it.
    pub air_friction: f32,
    /// Its moment about its x axis.
    pub xx_moment: f32,
    /// Its moment about its y axis.
    pub yy_moment: f32,
    /// Its moment about its z axis.
    pub zz_moment: f32,
    /// Its inertial matrix and its inverse (worked out by the tools that
    /// built the map), each three rows, if it has them.
    pub inertial_matrix: Option<[[[f32; 3]; 3]; 2]>,
    /// The mass points its controllers push.
    pub powered_mass_points: Vec<PoweredMassPoint>,
    /// Its mass points.
    pub mass_points: Vec<MassPoint>,
}

/// A mass point its vehicle's controller pushes.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct PoweredMassPoint {
    /// Its name.
    pub name: String,
    /// Its flags ([`PoweredMassPoint::GROUND_FRICTION`] and the rest).
    pub flags: u32,
    /// The push that holds it above the ground.
    pub antigrav_strength: f32,
    /// How far from the ground the push is felt.
    pub antigrav_offset: f32,
    /// How high the push holds it.
    pub antigrav_height: f32,
    /// How quickly the push dies away.
    pub antigrav_damp_fraction: f32,
    /// How much the push lifts it at its normal.
    pub antigrav_normal_k1: f32,
    /// How much the push lifts it at all.
    pub antigrav_normal_k0: f32,
}

impl PoweredMassPoint {
    /// The ground holds it back.
    pub const GROUND_FRICTION: u32 = 1 << 0;
    /// The water holds it back.
    pub const WATER_FRICTION: u32 = 1 << 1;
    /// The air holds it back.
    pub const AIR_FRICTION: u32 = 1 << 2;
    /// It lifts on the water.
    pub const WATER_LIFT: u32 = 1 << 3;
    /// It lifts in the air.
    pub const AIR_LIFT: u32 = 1 << 4;
    /// It thrusts.
    pub const THRUST: u32 = 1 << 5;
    /// It is held above the ground.
    pub const ANTIGRAV: u32 = 1 << 6;
}

/// How a mass point slides on the ground.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash, Archive, Serialize, Deserialize)]
pub enum FrictionType {
    /// The same every way.
    Point,
    /// Along its forward axis or across it.
    Forward,
    /// Along its left axis or across it.
    Left,
    /// Along its up axis or across it.
    Up,
}

impl FrictionType {
    /// Every type, in the order of the tag's field.
    pub const ALL: [FrictionType; 4] = [
        FrictionType::Point,
        FrictionType::Forward,
        FrictionType::Left,
        FrictionType::Up,
    ];
}

/// A mass point: a sphere of an object's mass, at a place on it.
#[derive(Clone, Debug, PartialEq, Archive, Serialize, Deserialize)]
pub struct MassPoint {
    /// Its name.
    pub name: String,
    /// The powered mass point that pushes it.
    pub powered_mass_point: Option<u16>,
    /// The model node it moves with.
    pub model_node: Option<u16>,
    /// Its flags ([`MassPoint::METALLIC`]).
    pub flags: u32,
    /// Its share of the mass.
    pub relative_mass: f32,
    /// Its mass (worked out by the tools that built the map).
    pub mass: f32,
    /// Its density, as a share.
    pub relative_density: f32,
    /// Its density (worked out by the tools that built the map).
    pub density: f32,
    /// Where it is, from its object's origin.
    pub position: [f32; 3],
    /// Its forward axis.
    pub forward: [f32; 3],
    /// Its up axis.
    pub up: [f32; 3],
    /// How it slides on the ground.
    pub friction_type: FrictionType,
    /// The ground's friction on it along its axis, as a scale.
    pub friction_parallel_scale: f32,
    /// The ground's friction on it across its axis, as a scale.
    pub friction_perpendicular_scale: f32,
    /// Its radius.
    pub radius: f32,
}

impl MassPoint {
    /// It is metal (its contacts spark).
    pub const METALLIC: u32 = 1 << 0;
}
