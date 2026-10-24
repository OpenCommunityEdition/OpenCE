//! Driving and flying: Halo's vehicle physics, a port of its mass points
//! (physics/physics.c and units/vehicles.c of the decompilation) rather than
//! Jolt's vehicle constraints, so that the Warthog, the Scorpion, the Ghost
//! and the Banshee handle as they did. Each mass point is a sphere that the
//! level holds up, rubs and (for a hovering one) pushes away from; the
//! vehicle's controller sets what its powered mass points push with.
//!
//! A vehicle steps at the game's 30 Hz, on every other engine tick, and
//! everything here is in the tags' units: world units a game tick and world
//! units a game tick a tick. Water holds up, drags and lifts the mass
//! points under it (a boat floats, and rights itself); the ground's
//! materials do not scale its friction, and the level is the only thing a
//! vehicle runs into.

use glam::{Mat3, Quat, Vec2, Vec3};
use opence_content::{
    BspWater, FrictionType, MaterialPhysics, Physics, PoweredMassPoint, Vehicle, VehicleKind,
};
use opence_physics::{Layers, PhysicsError, SurfaceMaterials, World};

/// The game's ticks a second, which a vehicle steps at.
pub const GAME_TICKS_PER_SECOND: u32 = 30;

/// Gravity, in world units a game tick a tick (the game's globals').
pub const GRAVITY: f32 = 0.0035651792;

/// The ticks a vehicle that has come to rest takes to stop.
const STOP_TICKS: u8 = 15;

/// A mass point's flags, as it left the last tick.
const POINT_AT_REST: u8 = 1 << 0;
const POINT_ON_GROUND: u8 = 1 << 1;
const POINT_IN_WATER: u8 = 1 << 3;
const POINT_ANTIGRAVING: u8 = 1 << 4;

/// Below these, squared, a mass point or a vehicle is at rest.
const REST_SPEED_SQUARED: f32 = 0.0011111111;
const REST_SPIN_SQUARED: f32 = 0.0027415568;
const REST_ACCELERATION_SQUARED: f32 = 3.0864197e-7;
const REST_ANGULAR_ACCELERATION_SQUARED: f32 = 3.0461742e-6;

/// How deep two mass points meet before their push is their weight
/// (physics.c's global_physics_collision_depth).
const COLLISION_DEPTH: f32 = 0.2;

/// The passes a step takes to back out of the level.
const PENETRATION_PASSES: usize = 4;

/// The game's epsilon (real_math.h's).
const EPSILON: f32 = 0.0001;

/// The game ticks a vehicle is turned back over for, at most.
const UPENDING_TICKS: u8 = 30;

/// A vehicle whose up axis stands this high is back over.
const UPENDED: f32 = 0.9;

/// The spin, a game tick, a vehicle is turned back over with, before its
/// tag's limits scale it.
const UPENDING_TORQUE: f32 = 0.3;

/// A vehicle whose up axis stands lower than this is flipped
/// (`vehicle_is_flipped`).
const FLIPPED: f32 = 0.2;

/// Where a player turning a vehicle back over stands, from it: the side
/// they push it over from, or the end it stands on.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Approach {
    /// From its right, as its forward axis lies (the game's 1).
    Right,
    /// From its left (2).
    Left,
    /// It stands on its tail (3).
    NoseUp,
    /// It stands on its nose (4).
    NoseDown,
}

impl Approach {
    /// Where a player at `player` stands to a vehicle at `vehicle` facing
    /// `forward`.
    pub fn of(player: Vec3, vehicle: Vec3, forward: Vec3) -> Approach {
        if libm::fabsf(forward.z) > core::f32::consts::FRAC_1_SQRT_2 {
            return if forward.z < 0.0 {
                Approach::NoseDown
            } else {
                Approach::NoseUp
            };
        }
        let across = Vec3::Z.cross(vehicle - player);
        if across.dot(forward) > 0.0 {
            Approach::Left
        } else {
            Approach::Right
        }
    }
}

/// A vehicle being turned back over: who pushed it from where, and for
/// how many game ticks.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Upending {
    /// Where they pushed it from.
    pub approach: Approach,
    /// The game ticks it has been turned for.
    pub ticks: u8,
}

/// A vehicle's state between game ticks (its object's and its
/// _vehicle_datum's).
#[derive(Clone, Debug, PartialEq)]
pub struct VehicleState {
    /// Its centre of mass.
    pub position: Vec3,
    /// Its velocity.
    pub velocity: Vec3,
    /// Its forward axis.
    pub forward: Vec3,
    /// Its up axis.
    pub up: Vec3,
    /// Its spin, about its centre of mass.
    pub angular_velocity: Vec3,
    /// Its speed, as its controller wants it (forward positive).
    pub speed: f32,
    /// Its slide, as its controller wants it (left positive).
    pub slide: f32,
    /// Its wheels' turn, or a tank's turn as a speed.
    pub turn: f32,
    /// How far its wheels have rolled, round their circumference.
    pub wheel: f32,
    /// How far a tank's left and right treads have rolled.
    pub treads: [f32; 2],
    /// How much a scout hovers, 0 to 1.
    pub hover: f32,
    /// A fighter's thrust.
    pub thrust: f32,
    /// The ticks it has been off the ground.
    pub airborne_ticks: u8,
    /// The ticks it has been on the ground.
    pub on_ground_ticks: u8,
    /// The ticks it has left to stop.
    pub stop_time: u8,
    /// Whether it is at rest (its physics asleep).
    pub at_rest: bool,
    /// Whether any of its mass points is on the ground.
    pub on_ground: bool,
    /// Whether any of its mass points is in water (Halo's on media).
    pub in_water: bool,
    /// Whether all of its mass points are under water (wholly under
    /// media).
    pub under_water: bool,
    /// Whether it is braking.
    pub brake: bool,
    /// Each mass point's flags, as it left the last tick.
    pub points: Vec<u8>,
    /// The push of the vehicles it met, for its next step.
    pub collision_force: Vec3,
    /// Their turn, about its centre of mass, for its next step.
    pub collision_torque: Vec3,
    /// Whether a player is turning it back over.
    pub upending: Option<Upending>,
}

impl VehicleState {
    /// A vehicle placed with its origin at `origin`, facing `forward` with
    /// `up` up, and still.
    pub fn placed(physics: &Physics, origin: Vec3, forward: Vec3, up: Vec3) -> VehicleState {
        let frame = frame(forward, up);
        VehicleState {
            position: origin + frame * Vec3::from_array(physics.center_of_mass),
            velocity: Vec3::ZERO,
            forward,
            up,
            angular_velocity: Vec3::ZERO,
            speed: 0.0,
            slide: 0.0,
            turn: 0.0,
            wheel: 0.0,
            treads: [0.0; 2],
            hover: 0.0,
            thrust: 0.0,
            airborne_ticks: 0,
            on_ground_ticks: 0,
            stop_time: 0,
            at_rest: false,
            on_ground: false,
            in_water: false,
            under_water: false,
            brake: false,
            points: vec![0; physics.mass_points.len()],
            collision_force: Vec3::ZERO,
            collision_torque: Vec3::ZERO,
            upending: None,
        }
    }

    /// Wakes its physics: something pushed it, or what it rested on moved.
    pub fn wake(&mut self) {
        self.at_rest = false;
    }

    /// Where its origin is (its model's), from its centre of mass.
    pub fn origin(&self, physics: &Physics) -> Vec3 {
        self.position - frame(self.forward, self.up) * Vec3::from_array(physics.center_of_mass)
    }

    /// Its rotation, from the world's axes.
    pub fn rotation(&self) -> Quat {
        rotation_of(frame(self.forward, self.up))
    }

    /// Whether it lies over (`vehicle_is_flipped`).
    pub fn is_flipped(&self) -> bool {
        self.up.z < FLIPPED
    }

    /// Whether it stands high enough to be entered: its up axis is above
    /// level by more than the globals' `minimum_vehicle_flipping_angle`
    /// from upright (else a player turns it back over).
    pub fn enterable(&self, minimum_flipping_angle: f32) -> bool {
        let flipping_angle = core::f32::consts::FRAC_PI_2 - minimum_flipping_angle;
        self.up.z > libm::cosf(flipping_angle)
    }

    /// Starts turning it back over, pushed by a player at `player`; false
    /// if it is being turned already.
    pub fn start_upending(&mut self, physics: &Physics, player: Vec3) -> bool {
        if self.upending.is_some() {
            return false;
        }
        self.upending = Some(Upending {
            approach: Approach::of(player, self.origin(physics), self.forward),
            ticks: 0,
        });
        self.wake();
        true
    }
}

/// A game tick of turning a vehicle back over (the unit's upending): it
/// spins about its long axis, pushed from a side, or about its cross axis,
/// stood on an end, until it is up, or for at most a second. `limits` are
/// its tag's slowest and fastest flipping spins.
fn upend(state: &mut VehicleState, kind: VehicleKind, limits: [f32; 2]) {
    let Some(mut upending) = state.upending else {
        return;
    };
    if upending.ticks >= UPENDING_TICKS || state.up.z > UPENDED {
        state.upending = None;
        return;
    }
    let (forward, up) = (state.forward, state.up);
    let torque = match upending.approach {
        Approach::Left | Approach::NoseDown => UPENDING_TORQUE,
        Approach::Right | Approach::NoseUp => -UPENDING_TORQUE,
    };
    let axis = match upending.approach {
        Approach::NoseUp | Approach::NoseDown => forward.cross(up),
        Approach::Right | Approach::Left => forward + forward.cross(up) * -forward.z,
    };
    // (the further over it lies, the harder it is turned, within the
    // tag's limits; written so that limits the wrong way round do not
    // panic)
    let roll = (-2.0 * up.z).max(limits[0]).min(limits[1]);
    state.wake();
    state.angular_velocity = axis * (torque * roll);
    match kind {
        VehicleKind::AlienFighter => state.velocity.z = state.velocity.z.min(-0.01),
        VehicleKind::HumanTank => state.velocity = forward * forward.dot(state.velocity),
        _ => {}
    }
    upending.ticks += 1;
    state.upending = Some(upending);
}

/// What its driver and gunner do with a vehicle this tick.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct VehicleControls {
    /// The driver's throttle: forward, and left.
    pub throttle: Vec2,
    /// Where the driver looks.
    pub facing: Vec3,
    /// The driver holds jump (a brake).
    pub jump: bool,
    /// The driver holds crouch.
    pub crouch: bool,
    /// Its driver's and gunner's seats' power, 0 to 1.
    pub seat_power: [f32; 2],
    /// Its driver is an AI's (a fighter does not aim its fixed gun for one).
    pub ai: bool,
}

impl Default for VehicleControls {
    fn default() -> Self {
        VehicleControls {
            throttle: Vec2::ZERO,
            facing: Vec3::X,
            jump: false,
            crouch: false,
            seat_power: [0.0; 2],
            ai: false,
        }
    }
}

/// The level a vehicle moves in.
#[derive(Clone, Copy)]
pub struct VehicleLevel<'a> {
    /// Its physics world.
    pub world: &'a World,
    /// The height flyers are pushed up from below (0: none).
    pub floor: f32,
    /// The height flyers are pushed down from above (0: none).
    pub ceiling: f32,
    /// Its water (none: it has none).
    pub water: Option<&'a BspWater>,
    /// What its surfaces are made of (none: all of no material).
    pub surfaces: Option<&'a SurfaceMaterials>,
    /// The globals' materials' physics, by material type.
    pub materials: &'a [MaterialPhysics],
}

impl VehicleLevel<'_> {
    /// How deep under water `point` is (as `scenario_location_water_depth`
    /// counts it: positive under the surface).
    fn water_depth(&self, point: Vec3) -> f32 {
        self.water
            .map_or(f32::MIN, |water| water.depth(point.to_array()))
    }

    /// The physics of the material of the surface at `point`: none for no
    /// material, or one the globals do not have (the game's default
    /// material, whose scales are all 0).
    fn material(&self, point: Vec3) -> Option<&MaterialPhysics> {
        let kind = self.surfaces?.at(point);
        self.materials.get(usize::try_from(kind).ok()?)
    }
}

/// What the ground does to a mass point on it: the vehicle's own terms,
/// each scaled by its material's where that scale is above 0 (the
/// friction's only for a vehicle of 7500 or less mass).
#[derive(Clone, Copy, Debug, PartialEq)]
struct Ground {
    friction: f32,
    normal_k1: f32,
    normal_k0: f32,
    depth: f32,
    damp_fraction: f32,
}

impl Ground {
    /// The heaviest vehicle whose friction a material scales.
    const FRICTION_MASS_MOST: f32 = 7500.0;

    fn of(physics: &Physics, material: Option<&MaterialPhysics>) -> Ground {
        let mut ground = Ground {
            friction: physics.ground_friction,
            normal_k1: physics.ground_normal_k1,
            normal_k0: physics.ground_normal_k0,
            depth: physics.ground_depth,
            damp_fraction: physics.ground_damp_fraction,
        };
        let Some(material) = material else {
            return ground;
        };
        let scale = |value: &mut f32, by: f32| {
            if by > 0.0 {
                *value *= by;
            }
        };
        if physics.mass <= Ground::FRICTION_MASS_MOST {
            scale(&mut ground.friction, material.ground_friction_scale);
        }
        scale(&mut ground.normal_k1, material.ground_normal_k1_scale);
        scale(&mut ground.normal_k0, material.ground_normal_k0_scale);
        scale(&mut ground.depth, material.ground_depth_scale);
        scale(
            &mut ground.damp_fraction,
            material.ground_damp_fraction_scale,
        );
        ground
    }
}

/// What a controller sets each powered mass point to push with (in the C,
/// left on the stack and set as the controller goes; here zeroed each
/// tick).
#[derive(Clone, Copy, Debug, Default, PartialEq)]
struct Powered {
    ground_friction_velocity: f32,
    water_friction_velocity: f32,
    water_lift: f32,
    air_friction_velocity: f32,
    air_lift: f32,
    thrust_fraction: f32,
    antigrav_fraction: f32,
    rotation: Quat,
}

/// A mass point as the tick found it.
#[derive(Clone, Copy, Debug)]
struct Point {
    /// Where it is.
    position: Vec3,
}

/// Its mass points where it is: each one's centre and radius.
pub(crate) fn spheres<'a>(
    state: &'a VehicleState,
    physics: &'a Physics,
) -> impl Iterator<Item = (Vec3, f32)> + 'a {
    let frame = frame(state.forward, state.up);
    let com = Vec3::from_array(physics.center_of_mass);
    physics.mass_points.iter().map(move |point| {
        (
            state.position + frame * (Vec3::from_array(point.position) - com),
            point.radius,
        )
    })
}

/// How far its mass points reach from its centre of mass.
fn reach(physics: &Physics) -> f32 {
    let com = Vec3::from_array(physics.center_of_mass);
    physics
        .mass_points
        .iter()
        .map(|point| (Vec3::from_array(point.position) - com).length() + point.radius)
        .fold(0.0, f32::max)
}

/// Two vehicles meeting (physics_compute_vehicle_collision): each two of
/// their mass points that overlap push apart by half their overlap, as
/// hard as their weight over the collision depth, at the point between
/// them. The pushes are kept for each one's next step, and both are woken.
/// Whether they met.
pub fn collide(
    (first, first_physics): (&mut VehicleState, &Physics),
    (second, second_physics): (&mut VehicleState, &Physics),
) -> bool {
    let apart = first.position.distance(second.position);
    // (too far apart to meet, or nowhere)
    if apart.is_nan() || apart > reach(first_physics) + reach(second_physics) {
        return false;
    }
    // (a map's mass is finite but may be negative: no push, not a NaN)
    let mass_scale = libm::sqrtf((first_physics.mass * second_physics.mass).max(0.0));
    let (mut force, mut torque) = ([Vec3::ZERO; 2], [Vec3::ZERO; 2]);
    let mut met = false;
    for (point0, radius0) in spheres(first, first_physics) {
        for (point1, radius1) in spheres(second, second_physics) {
            let radius = radius0 + radius1;
            let offset = point1 - point0;
            let distance = offset.length();
            if !(distance < radius && distance > 0.0) {
                continue;
            }
            let direction = offset / distance;
            let penetration = (radius - distance) * 0.5;
            let magnitude = 2.0 * mass_scale * GRAVITY / COLLISION_DEPTH * penetration;
            let at = point0 + direction * (radius0 - penetration);
            let pushes = [-direction * magnitude, direction * magnitude];
            for (side, centre) in [first.position, second.position].into_iter().enumerate() {
                force[side] += pushes[side];
                torque[side] += (at - centre).cross(pushes[side]);
            }
            met = true;
        }
    }
    if met {
        for (side, state) in [first, second].into_iter().enumerate() {
            state.collision_force += force[side];
            state.collision_torque += torque[side];
            state.wake();
        }
    }
    met
}

/// One game tick of a vehicle: its controls taken, and its physics
/// stepped unless it is at rest.
pub fn game_tick(
    state: &mut VehicleState,
    vehicle: &Vehicle,
    physics: &Physics,
    controls: &VehicleControls,
    level: VehicleLevel<'_>,
) -> Result<(), PhysicsError> {
    state.points.resize(physics.mass_points.len(), 0);
    let throttle = controls.throttle;
    state.brake = controls.jump
        || (vehicle.flags & Vehicle::CONTROL_OPPOSITE_SPEED_SETS_BRAKE != 0
            && ((throttle.x > 0.0 && state.speed < 0.0)
                || (throttle.x < 0.0 && state.speed > 0.0)));
    let left = state.up.cross(state.forward);
    let steering = libm::atan2f(
        left.dot(controls.facing),
        controls.facing.dot(state.forward),
    );

    let speed = SpeedParameters {
        positive: vehicle.maximum_forward_speed,
        negative: vehicle.maximum_reverse_speed,
        acceleration: vehicle.speed_acceleration,
        deceleration: vehicle.speed_deceleration,
    };
    if state.brake {
        speed_update_seek(&mut state.speed, speed, 0.0, 1.0);
    } else {
        speed_update_seek(&mut state.speed, speed, throttle.x, 1.0);
        let slide = SpeedParameters {
            positive: vehicle.maximum_left_slide,
            negative: vehicle.maximum_right_slide,
            acceleration: vehicle.slide_acceleration,
            deceleration: vehicle.slide_deceleration,
        };
        speed_update_seek(&mut state.slide, slide, throttle.y, 1.0);
    }
    if vehicle.kind == VehicleKind::HumanTank {
        if state.speed == 0.0 {
            speed_update_seek(&mut state.turn, speed, 0.0, 1.0);
        } else {
            let target = (steering * 0.63661975).clamp(-1.0, 1.0) * vehicle.maximum_forward_speed;
            speed_update_seek(&mut state.turn, speed, target, 2.0);
        }
    } else {
        let desired = if state.speed < 0.0 {
            -steering
        } else {
            steering
        };
        let desired = desired.clamp(
            vehicle.maximum_right_turn.to_radians(),
            vehicle.maximum_left_turn.to_radians(),
        );
        // (the limits as the tag holds them, in degrees, which the game
        // compares radians with)
        let limits = [vehicle.maximum_left_turn, vehicle.maximum_right_turn];
        position_update_seek(
            &mut state.turn,
            limits,
            false,
            desired,
            vehicle.turn_rate.to_radians() / GAME_TICKS_PER_SECOND as f32,
        );
    }

    let wakes = (vehicle.flags & Vehicle::SPEED_WAKES_PHYSICS != 0 && state.speed != 0.0)
        || (vehicle.flags & Vehicle::TURN_WAKES_PHYSICS != 0 && state.turn != 0.0)
        || (vehicle.flags & Vehicle::DRIVER_POWER_WAKES_PHYSICS != 0
            && controls.seat_power[0] != 0.0)
        || (vehicle.flags & Vehicle::GUNNER_POWER_WAKES_PHYSICS != 0
            && controls.seat_power[1] != 0.0)
        || (vehicle.flags & Vehicle::SLIDE_WAKES_PHYSICS != 0 && state.slide != 0.0);
    if wakes {
        state.wake();
    }
    upend(
        state,
        vehicle.kind,
        [
            vehicle.minimum_flipping_angular_velocity,
            vehicle.maximum_flipping_angular_velocity,
        ],
    );

    if !state.at_rest {
        match vehicle.kind {
            VehicleKind::HumanJeep => jeep(state, vehicle, physics, level)?,
            VehicleKind::HumanTank => tank(state, vehicle, physics, level)?,
            VehicleKind::HumanBoat => boat(state, physics, level)?,
            VehicleKind::AlienScout => scout(state, vehicle, physics, controls, steering, level)?,
            VehicleKind::AlienFighter => fighter(state, vehicle, physics, controls, level)?,
            // (a plane and a turret are left to the mass points alone)
            VehicleKind::HumanPlane | VehicleKind::Turret => {
                let mut powered = [Powered::identity(); 32];
                physics_update(state, physics, &mut powered, Vec3::ZERO, Vec3::ZERO, level)?;
            }
        }
        compute_airborne_ticks(state, physics);
        if state.at_rest {
            state.stop_time = STOP_TICKS;
        }
        if matches!(
            vehicle.kind,
            VehicleKind::HumanPlane | VehicleKind::AlienFighter
        ) {
            let power = controls.seat_power[0];
            let z = state.position.z;
            if level.floor != 0.0 && z < level.floor {
                state.velocity.z +=
                    ((level.floor - z) * 0.015625 - state.velocity.z * 0.0625) * power;
            }
            if level.ceiling != 0.0 && z > level.ceiling {
                state.velocity.z -=
                    ((z - level.ceiling) * 0.015625 + state.velocity.z * 0.0625) * power;
            }
        }
    } else if state.stop_time > 0 {
        slowly_stop(state);
    }
    Ok(())
}

impl Powered {
    fn identity() -> Powered {
        Powered {
            rotation: Quat::IDENTITY,
            ..Powered::default()
        }
    }
}

/// A vehicle's frame: its forward, left and up axes as columns.
fn frame(forward: Vec3, up: Vec3) -> Mat3 {
    Mat3::from_cols(forward, up.cross(forward), up)
}

/// A rotation's quaternion, from its matrix (with libm's square root).
fn rotation_of(m: Mat3) -> Quat {
    let (x, y, z) = (m.x_axis, m.y_axis, m.z_axis);
    let trace = x.x + y.y + z.z;
    let q = if trace > 0.0 {
        let s = 0.5 / libm::sqrtf(trace + 1.0);
        Quat::from_xyzw((y.z - z.y) * s, (z.x - x.z) * s, (x.y - y.x) * s, 0.25 / s)
    } else if x.x > y.y && x.x > z.z {
        let s = 2.0 * libm::sqrtf(1.0 + x.x - y.y - z.z);
        Quat::from_xyzw(0.25 * s, (y.x + x.y) / s, (z.x + x.z) / s, (y.z - z.y) / s)
    } else if y.y > z.z {
        let s = 2.0 * libm::sqrtf(1.0 + y.y - x.x - z.z);
        Quat::from_xyzw((y.x + x.y) / s, 0.25 * s, (z.y + y.z) / s, (z.x - x.z) / s)
    } else {
        let s = 2.0 * libm::sqrtf(1.0 + z.z - x.x - y.y);
        Quat::from_xyzw((z.x + x.z) / s, (z.y + y.z) / s, 0.25 * s, (x.y - y.x) / s)
    };
    q.normalize()
}

/// `v` turned about the unit `axis` by `angle`.
fn rotate(v: Vec3, axis: Vec3, angle: f32) -> Vec3 {
    let (sin, cos) = (libm::sinf(angle), libm::cosf(angle));
    v * cos + axis.cross(v) * sin + axis * (axis.dot(v) * (1.0 - cos))
}

/// A vector's length and its direction (zero and unchanged below the
/// game's epsilon: normalize3d).
fn normalize(v: Vec3) -> (f32, Vec3) {
    let length = libm::sqrtf(v.dot(v));
    if length.abs() < EPSILON {
        (0.0, v)
    } else {
        (length, v / length)
    }
}

/// `v` shortened to `length` if it is longer (limit3d).
fn limit(v: Vec3, length: f32) -> Vec3 {
    let squared = v.dot(v);
    if squared > length * length {
        v * (length / libm::sqrtf(squared))
    } else {
        v
    }
}

/// -1, 0 or 1, as `v` is.
fn sign(v: f32) -> f32 {
    if v > 0.0 {
        1.0
    } else if v < 0.0 {
        -1.0
    } else {
        0.0
    }
}

/// How far `v` is from `begin` to `end`, 0 to 1, either way round.
fn pin_fraction(v: f32, begin: f32, end: f32) -> f32 {
    if begin < end {
        if v <= begin {
            0.0
        } else if v >= end {
            1.0
        } else {
            (v - begin) / (end - begin)
        }
    } else if v <= end {
        1.0
    } else if v >= begin {
        0.0
    } else {
        (begin - v) / (begin - end)
    }
}

/// A speed's limits and how fast it changes (physics_variables.c's).
#[derive(Clone, Copy, Debug, PartialEq)]
struct SpeedParameters {
    positive: f32,
    negative: f32,
    acceleration: f32,
    deceleration: f32,
}

fn speed_update(speed: &mut f32, parameters: SpeedParameters, delta: f32) {
    let magnitude = delta.abs();
    let acceleration = magnitude * parameters.acceleration;
    let deceleration = magnitude * parameters.deceleration;
    if delta > 0.0 {
        if *speed <= -deceleration {
            *speed += deceleration;
        } else if *speed >= 0.0 {
            *speed += acceleration;
        } else {
            *speed = (*speed / deceleration + 1.0) * acceleration;
        }
        *speed = speed.min(magnitude * parameters.positive);
    } else if delta < 0.0 {
        if *speed >= deceleration {
            *speed -= deceleration;
        } else if *speed <= 0.0 {
            *speed -= acceleration;
        } else {
            *speed = (*speed / deceleration - 1.0) * acceleration;
        }
        *speed = (-magnitude * parameters.negative).max(*speed);
    }
}

/// A speed moved toward `target`, stopping there.
fn speed_update_seek(speed: &mut f32, parameters: SpeedParameters, target: f32, delta: f32) {
    if *speed > target {
        speed_update(speed, parameters, -delta);
        if *speed <= target {
            *speed = target;
        }
    } else if *speed < target {
        speed_update(speed, parameters, delta);
        if *speed >= target {
            *speed = target;
        }
    }
}

fn position_update(position: &mut f32, limits: [f32; 2], wrap: bool, delta: f32) {
    *position += delta;
    if *position < limits[1] {
        if wrap {
            *position += limits[0] - limits[1];
        } else {
            *position = limits[1];
        }
    } else if *position > limits[0] {
        if wrap {
            *position -= limits[0] - limits[1];
        } else {
            *position = limits[0];
        }
    }
}

fn seek_direction(limits: [f32; 2], position: f32, wrap: bool, target: f32) -> f32 {
    let difference = target - position;
    let direction = sign(difference);
    if wrap && difference.abs() > (limits[0] - limits[1]) * 0.5 {
        -direction
    } else {
        direction
    }
}

/// A position moved toward `target` by `delta`, stopping there.
fn position_update_seek(position: &mut f32, limits: [f32; 2], wrap: bool, target: f32, delta: f32) {
    let direction = seek_direction(limits, *position, wrap, target);
    if direction != 0.0 {
        position_update(position, limits, wrap, direction * delta);
        if seek_direction(limits, *position, wrap, target) == direction {
            return;
        }
    }
    *position = target;
}

/// A value rolled round `circumference`, kept positive.
fn roll(value: f32, by: f32, circumference: f32) -> f32 {
    if circumference.is_nan() || circumference == 0.0 {
        return value;
    }
    let rolled = libm::fmodf(value + by, circumference);
    if rolled < 0.0 {
        rolled + circumference
    } else {
        rolled
    }
}

fn jeep(
    state: &mut VehicleState,
    vehicle: &Vehicle,
    physics: &Physics,
    level: VehicleLevel<'_>,
) -> Result<(), PhysicsError> {
    state.wheel = roll(state.wheel, state.speed, vehicle.wheel_circumference);
    let mut powered = [Powered::identity(); 32];
    if physics.powered_mass_points.len() == 2 {
        // (the front wheels turn one way and the back the other)
        powered[0].ground_friction_velocity = state.speed;
        powered[0].rotation = Quat::from_xyzw(
            0.0,
            0.0,
            libm::sinf(state.turn * 0.5),
            libm::cosf(state.turn * 0.5),
        );
        powered[1].ground_friction_velocity = state.speed;
        powered[1].rotation = Quat::from_xyzw(
            0.0,
            0.0,
            libm::sinf(-state.turn * 0.5),
            libm::cosf(-state.turn * 0.5),
        );
    }
    physics_update(state, physics, &mut powered, Vec3::ZERO, Vec3::ZERO, level)
}

fn tank(
    state: &mut VehicleState,
    vehicle: &Vehicle,
    physics: &Physics,
    level: VehicleLevel<'_>,
) -> Result<(), PhysicsError> {
    let left = state.speed - state.turn;
    let right = state.speed + state.turn;
    state.treads[0] = roll(state.treads[0], left, vehicle.wheel_circumference);
    state.treads[1] = roll(state.treads[1], right, vehicle.wheel_circumference);
    let mut powered = [Powered::identity(); 32];
    if physics.powered_mass_points.len() == 2 {
        powered[0].ground_friction_velocity = left;
        powered[1].ground_friction_velocity = right;
    }
    physics_update(state, physics, &mut powered, Vec3::ZERO, Vec3::ZERO, level)
}

/// The fastest a boat rights itself, in radians a game tick a tick
/// (`VEHICLE_ANGULAR_ACCELERATION`).
const BOAT_RIGHTING: f32 = 0.013962634;

fn boat(
    state: &mut VehicleState,
    physics: &Physics,
    level: VehicleLevel<'_>,
) -> Result<(), PhysicsError> {
    let mut powered = [Powered::identity(); 32];
    if physics.powered_mass_points.len() != 3 {
        return physics_update(state, physics, &mut powered, Vec3::ZERO, Vec3::ZERO, level);
    }
    // (the rudder turns less the faster it goes)
    let speed = libm::sqrtf(state.velocity.dot(state.velocity)) * 2.5;
    let angle = (1.0 - speed.min(1.0)) * (state.turn * 0.5);
    powered[0].water_friction_velocity = state.speed;
    powered[0].water_lift = 0.003;
    powered[0].rotation = Quat::from_xyzw(0.0, 0.0, libm::sinf(angle), libm::cosf(angle));
    powered[1].water_lift = 0.003;
    powered[2].water_lift = 0.005;

    // (rolled back upright about its forward axis, leant into its slide)
    let (forward, up) = (state.forward, state.up);
    let (length, upright) = normalize(forward * -forward.z + Vec3::Z);
    let torque = if length != 0.0 {
        let cross = up.cross(forward);
        let spin = forward.cross(state.velocity).dot(Vec3::Z) * 2.0 * std::f32::consts::PI;
        let upright = rotate(upright, forward, spin);
        let mut angle = angle_between(upright, up);
        if cross.dot(upright) > 0.0 {
            angle = -angle;
        }
        let dot = state.angular_velocity.dot(forward);
        let wanted = libm::sqrtf(angle.abs() * 2.0 * BOAT_RIGHTING) * sign(angle) - dot;
        forward * (wanted.clamp(-BOAT_RIGHTING, BOAT_RIGHTING) * physics.xx_moment)
    } else {
        Vec3::ZERO
    };
    physics_update(state, physics, &mut powered, Vec3::ZERO, torque, level)
}

/// The angle between two vectors (`angle_between_vectors3d`, through the
/// cosine of twice the angle).
fn angle_between(a: Vec3, b: Vec3) -> f32 {
    let squares = a.dot(a) * b.dot(b);
    if squares == 0.0 {
        return 0.0;
    }
    let ab = a.dot(b);
    let c = (2.0 * ab * ab / squares - 1.0).clamp(-1.0, 1.0);
    let angle = libm::acosf(c) * 0.5;
    if ab < 0.0 {
        std::f32::consts::PI - angle
    } else {
        angle
    }
}

fn scout(
    state: &mut VehicleState,
    vehicle: &Vehicle,
    physics: &Physics,
    controls: &VehicleControls,
    steering: f32,
    level: VehicleLevel<'_>,
) -> Result<(), PhysicsError> {
    let antigrav = controls.seat_power[0];
    let mut powered = [Powered::identity(); 32];
    for point in powered.iter_mut() {
        point.antigrav_fraction = antigrav;
    }
    let throttle = controls.throttle;
    let hover = state.hover;
    let (forward, up) = (state.forward, state.up);
    let mut force = Vec3::ZERO;
    let mut torque = Vec3::ZERO;
    // (out of the water, and not upside down)
    if level.water_depth(state.position) < 0.5 && up.z > -0.2 {
        let frame = frame(forward, up);
        if hover > 0.0 {
            let local = frame.transpose() * state.velocity;
            let fastest = vehicle.maximum_forward_speed * if state.brake { 0.8 } else { 1.0 };
            let mut most = vehicle.speed_acceleration;
            let acceleration = Vec3::new(
                fastest * throttle.x - local.x,
                fastest * throttle.y - local.y,
                0.0,
            );
            if state.on_ground_ticks > 0 && steering.abs() > std::f32::consts::FRAC_PI_4 {
                most *= 1.0 - (f32::from(state.on_ground_ticks) * 0.05).min(0.98);
            }
            let acceleration = frame * limit(acceleration, most);
            force += acceleration * (physics.mass * hover);

            let current = up.dot(state.angular_velocity);
            let mut desired = libm::sqrtf(steering.abs() * 0.0069813174) * sign(steering);
            if desired.abs() > EPSILON && steering / desired < 2.0 {
                desired = steering * 0.5;
            }
            let error = (desired - current).clamp(-0.0034906587, 0.0034906587);
            torque += up * (error * physics.zz_moment * hover);
        }
        if hover < 1.0 {
            let left = up.cross(forward);
            let flat_forward = Vec2::new(forward.x, forward.y).normalize_or_zero();
            let flat_left = Vec2::new(left.x, left.y).normalize_or_zero();
            let (pitch, roll);
            if up.z > 0.0 {
                let flat_up = Vec2::new(up.x, up.y);
                let flat_spin = Vec2::new(state.angular_velocity.x, state.angular_velocity.y);
                let align = Vec2::new(flat_up.dot(flat_forward), flat_up.dot(flat_left));
                let rate = Vec2::new(flat_spin.dot(flat_left), -flat_spin.dot(flat_forward));
                let level = -align - rate * 15.0;
                let a = level.x.abs() * sign(throttle.x * level.x);
                let b = level.y.abs() * sign(throttle.y * level.y);
                let control = Vec2::new(
                    throttle.x * (a + 1.0).clamp(0.3, 2.5),
                    throttle.y * (b + 1.0).clamp(0.3, 2.5),
                ) * 0.0015514038;
                let levelling = (1.0 - up.z) * 0.0038785094;
                pitch = levelling * level.x + control.x;
                roll = levelling * level.y + control.y;
            } else {
                pitch = throttle.x * 0.0015514038;
                roll = throttle.y * 0.0015514038;
            }
            torque += (left * (physics.yy_moment * pitch) + forward * (-physics.xx_moment * roll))
                * (1.0 - hover);
        }
        if state.brake {
            let slowing = if vehicle.maximum_forward_speed > 0.0 {
                (forward.dot(state.velocity) / vehicle.maximum_forward_speed).clamp(0.0, 1.0)
            } else {
                0.0
            };
            let left = up.cross(forward);
            if slowing > 0.0 {
                torque += left * (physics.yy_moment * slowing * hover * -0.005817764);
                force += Vec3::Z * (physics.mass * slowing * hover * 0.004);
            }
            if state.airborne_ticks > 0 {
                let (length, axis) = normalize(left.cross(Vec3::Z));
                if length > 0.0 {
                    let fade = (1.0 - f32::from(state.airborne_ticks) / 30.0).clamp(0.0, 1.0);
                    force += axis * ((1.0 - hover) * physics.mass * fade * 0.002)
                        + Vec3::Z * ((1.0 - hover) * physics.mass * fade * 0.001);
                }
            }
        }
        force *= antigrav;
        torque *= antigrav;
    }
    physics_update(state, physics, &mut powered, force, torque, level)?;

    let maximum = state.up.z.max(0.4);
    let (mut held, mut pushed) = (0usize, 0usize);
    for (point, flags) in physics.mass_points.iter().zip(&state.points) {
        if point.powered_mass_point.is_some() {
            pushed += 1;
            if flags & POINT_ANTIGRAVING != 0 {
                held += 1;
            }
        }
    }
    let ratio = if pushed > 0 {
        held as f32 / pushed as f32
    } else {
        0.0
    };
    let target = (ratio * maximum).clamp(0.0, 1.0);
    state.hover = target.clamp(state.hover - 0.1, state.hover + 0.1);
    Ok(())
}

fn fighter(
    state: &mut VehicleState,
    vehicle: &Vehicle,
    physics: &Physics,
    controls: &VehicleControls,
    level: VehicleLevel<'_>,
) -> Result<(), PhysicsError> {
    let power = controls.seat_power[0];
    let fastest = vehicle.maximum_forward_speed;

    // (toward its speed along where it faces)
    let desired_velocity = state.forward * state.speed;
    let throttle = if state.speed > 0.0 && fastest > 0.0 {
        state.speed / fastest
    } else if state.speed < 0.0 && vehicle.maximum_reverse_speed > 0.0 {
        -state.speed / vehicle.maximum_reverse_speed
    } else {
        0.0
    };
    let mut most = throttle * vehicle.speed_acceleration;
    let least = throttle * vehicle.speed_deceleration;
    let mut acceleration = desired_velocity - state.velocity;
    acceleration.z += GRAVITY;
    let dot = desired_velocity.dot(acceleration);
    let wanted = desired_velocity.dot(desired_velocity);
    if dot > EPSILON && wanted > 0.0 {
        most = (most - least) * ((dot * dot / acceleration.dot(acceleration)) / wanted) + least;
    } else {
        most = least;
    }
    let force = limit(acceleration, most) * (physics.mass * power);

    // (and turned to where its driver looks)
    let mut desired_forward = controls.facing;
    let (length, mut desired_up) = normalize(Vec3::Z - controls.facing * controls.facing.z);
    if length == 0.0 {
        desired_up = Vec3::X;
    }
    if !controls.ai {
        let (sin, cos) = (
            libm::sinf(vehicle.fixed_gun_pitch),
            libm::cosf(vehicle.fixed_gun_pitch),
        );
        let (f, u) = (desired_forward, desired_up);
        desired_forward = f * cos + u * sin;
        desired_up = u * cos - f * sin;
    }
    // (banked into the turn: Halo's yaw_vectors turns the up about the
    // forward here, its arguments the other way round)
    if fastest > 0.0 {
        let cross = desired_forward.x * state.velocity.y - desired_forward.y * state.velocity.x;
        let bank = cross / fastest * vehicle.maximum_left_turn;
        let (sin, cos) = (libm::sinf(bank), libm::cosf(bank));
        desired_up = desired_up * cos + desired_forward.cross(desired_up) * sin;
    }
    let desired = frame(desired_forward, desired_up);
    let current = frame(state.forward, state.up);
    let (axis, angle) = axis_angle(desired * current.transpose());
    let desired_spin = axis * (angle * vehicle.turn_rate / std::f32::consts::PI);
    let moment = (physics.xx_moment + physics.yy_moment + physics.zz_moment) / 3.0;
    let torque = (desired_spin - state.angular_velocity) * (moment * power);

    if vehicle.turn_rate > 0.0 {
        let spin =
            libm::sqrtf(state.angular_velocity.dot(state.angular_velocity)) / vehicle.turn_rate;
        let change = if spin > state.thrust {
            let most = ((1.0 - state.thrust) * (1.0 - state.thrust) * 0.2).clamp(0.01, 0.05);
            (spin - state.thrust).min(most)
        } else {
            let most = -(state.thrust * state.thrust * 0.05).max(0.005);
            (spin - state.thrust).max(most)
        };
        state.thrust += change;
    }

    let mut powered = [Powered::identity(); 32];
    for point in powered.iter_mut().take(2) {
        point.antigrav_fraction = power;
    }
    physics_update(state, physics, &mut powered, force, torque, level)
}

/// A rotation's axis and angle, the angle at most a half turn.
fn axis_angle(m: Mat3) -> (Vec3, f32) {
    let q = rotation_of(m);
    let (w, v) = (q.w.clamp(-1.0, 1.0), Vec3::new(q.x, q.y, q.z));
    let mut angle = 2.0 * libm::acosf(w);
    let (length, mut axis) = normalize(v);
    if length == 0.0 {
        return (Vec3::Z, 0.0);
    }
    if angle > std::f32::consts::PI {
        axis = -axis;
        angle = 2.0 * std::f32::consts::PI - angle;
    }
    (axis, angle)
}

fn compute_airborne_ticks(state: &mut VehicleState, physics: &Physics) {
    state.airborne_ticks = state.airborne_ticks.saturating_add(1);
    for (point, flags) in physics.mass_points.iter().zip(&state.points) {
        if flags & POINT_ON_GROUND != 0 {
            state.airborne_ticks = 0;
            state.on_ground_ticks = state.on_ground_ticks.saturating_add(1);
            return;
        }
        if point.powered_mass_point.is_some() && flags & POINT_ANTIGRAVING != 0 {
            state.airborne_ticks = 0;
        }
    }
    state.on_ground_ticks = 0;
}

fn slowly_stop(state: &mut VehicleState) {
    state.stop_time -= 1;
    state.velocity *= 0.835;
    state.angular_velocity *= 0.835;
    state.position += state.velocity;
    let (angle, axis) = normalize(state.angular_velocity);
    if angle > 0.0 {
        state.forward = rotate(state.forward, axis, angle);
        state.up = rotate(state.up, axis, angle);
    }
    if state.stop_time == 0 {
        state.velocity = Vec3::ZERO;
        state.angular_velocity = Vec3::ZERO;
    }
}

/// Forward and up turned by a spin, kept square.
fn spun(forward: Vec3, up: Vec3, spin: Vec3) -> (Vec3, Vec3) {
    let (angle, axis) = normalize(spin);
    if angle == 0.0 {
        return (forward, up);
    }
    let (_, forward) = normalize(rotate(forward, axis, angle));
    let up = rotate(up, axis, angle);
    let (_, up) = normalize(up - forward * up.dot(forward));
    (forward, up)
}

/// The friction along or across a mass point's axis, scaled by type.
fn friction_evaluate(
    friction: Vec3,
    kind: FrictionType,
    parallel: f32,
    perpendicular: f32,
    forward: Vec3,
    up: Vec3,
) -> Vec3 {
    let axis = match kind {
        FrictionType::Point => return friction,
        FrictionType::Forward => forward,
        FrictionType::Left => up.cross(forward),
        FrictionType::Up => up,
    };
    let along = axis * friction.dot(axis);
    along * parallel + (friction - along) * perpendicular
}

/// The forces on each mass point, summed: gravity, the ground and the air,
/// and what the powered ones push with.
fn compute_new(
    state: &mut VehicleState,
    physics: &Physics,
    powered: &[Powered; 32],
    level: VehicleLevel<'_>,
    points: &mut Vec<Point>,
) -> Result<(Vec3, Vec3), PhysicsError> {
    let world = level.world;
    let frame = frame(state.forward, state.up);
    let com = Vec3::from_array(physics.center_of_mass);
    // (gravity on the whole, at its centre of mass)
    let mut total_force = Vec3::new(0.0, 0.0, -physics.mass * physics.gravity_scale * GRAVITY);
    let mut total_torque = Vec3::ZERO;
    let mut submerged = 0usize;
    points.clear();
    for (index, point) in physics.mass_points.iter().enumerate() {
        let pushed = point.powered_mass_point.and_then(|index| {
            Some((
                physics.powered_mass_points.get(usize::from(index))?,
                powered.get(usize::from(index))?,
            ))
        });
        let local = match pushed {
            Some((_, powered)) => Mat3::from_quat(powered.rotation),
            None => Mat3::IDENTITY,
        };
        let forward = frame * (local * Vec3::from_array(point.forward));
        let up = frame * (local * Vec3::from_array(point.up));
        let position = state.position + frame * (Vec3::from_array(point.position) - com);
        let radius = position - state.position;
        let velocity = state.angular_velocity.cross(radius) + state.velocity;
        let mass = point.mass;
        let mut flags = 0u8;

        // (the ground under it, or the depths of hell)
        let (normal, depth, material) =
            match world.sphere_contact(position, point.radius, Layers::LEVEL)? {
                Some(contact) => {
                    // (the material under it, on the plane it meets)
                    let under = position - contact.normal * (point.radius - contact.depth);
                    (contact.normal, contact.depth, level.material(under))
                }
                None => (Vec3::Z, point.radius - (position.z + 256.0), None),
            };
        let mut normal_force = Vec3::ZERO;
        let mut ground_friction = Vec3::ZERO;
        if depth > 0.0 {
            let ground = Ground::of(physics, material);
            let into = velocity.dot(normal);
            let spring = if ground.depth > 0.0 {
                GRAVITY / ground.depth * depth
            } else {
                0.0
            };
            normal_force = normal * (physics.mass * (spring - into * ground.damp_fraction));
            let sliding = velocity - normal * into;
            let scale = -mass * ground.friction;
            let mut friction = sliding * scale;
            if let Some((definition, powered)) = pushed {
                if definition.flags & PoweredMassPoint::GROUND_FRICTION != 0
                    && powered.ground_friction_velocity != 0.0
                {
                    let footing = pin_fraction(normal.z, ground.normal_k0, ground.normal_k1);
                    let upright = up.dot(normal).clamp(0.0, 1.0);
                    let weight = upright * upright * footing * footing * scale;
                    let mut drive = forward * -powered.ground_friction_velocity;
                    drive -= normal * drive.dot(normal);
                    friction += drive * weight;
                }
            }
            ground_friction = friction_evaluate(
                friction,
                point.friction_type,
                point.friction_parallel_scale,
                point.friction_perpendicular_scale,
                forward,
                up,
            );
            flags |= POINT_ON_GROUND;
        }

        let mut push = Vec3::ZERO;
        let mut water_pressure = Vec3::ZERO;
        let mut water_friction = Vec3::ZERO;
        let water_depth = level.water_depth(position);
        if water_depth > 0.0 {
            let fraction = if water_depth < physics.water_depth {
                water_depth / physics.water_depth
            } else {
                1.0
            };
            if point.density > 0.0 && physics.water_depth > 0.0 {
                water_pressure.z = mass / point.density
                    * physics.water_density
                    * fraction
                    * (physics.gravity_scale * GRAVITY);
            }
            let water_scale = -mass * physics.water_friction;
            let mut friction = velocity * water_scale;
            if let Some((definition, powered)) = pushed {
                if definition.flags & PoweredMassPoint::WATER_FRICTION != 0
                    && powered.water_friction_velocity != 0.0
                {
                    friction = (velocity - forward * powered.water_friction_velocity) * water_scale;
                }
            }
            water_friction = friction_evaluate(
                friction,
                point.friction_type,
                point.friction_parallel_scale,
                point.friction_perpendicular_scale,
                forward,
                up,
            );
            if let Some((definition, powered)) = pushed {
                if definition.flags & PoweredMassPoint::WATER_LIFT != 0 && powered.water_lift != 0.0
                {
                    push += up
                        * (forward.dot(velocity).abs()
                            * powered.water_lift
                            * physics.mass
                            * fraction);
                }
            }
            flags |= POINT_IN_WATER;
            submerged += 1;
        }

        let air_scale = -mass * physics.air_friction;
        let mut air = velocity * air_scale;
        if let Some((definition, powered)) = pushed {
            if definition.flags & PoweredMassPoint::AIR_FRICTION != 0
                && powered.air_friction_velocity != 0.0
            {
                air = (velocity - forward * powered.air_friction_velocity) * air_scale;
            }
        }
        let air_friction = friction_evaluate(
            air,
            point.friction_type,
            point.friction_parallel_scale,
            point.friction_perpendicular_scale,
            forward,
            up,
        );
        if velocity.dot(velocity) < REST_SPEED_SQUARED {
            flags |= POINT_AT_REST;
        }
        if let Some((definition, powered)) = pushed {
            if definition.flags & PoweredMassPoint::AIR_LIFT != 0 {
                push += up * (forward.dot(velocity).abs() * physics.mass * powered.air_lift);
            }
            if definition.flags & PoweredMassPoint::THRUST != 0 {
                push += forward * (powered.thrust_fraction * physics.mass);
            }
            if definition.flags & PoweredMassPoint::ANTIGRAV != 0
                && definition.antigrav_height > 0.0
            {
                let probe = point.radius + definition.antigrav_height;
                if let Some(hit) =
                    world.cast_ray(position, Vec3::new(0.0, 0.0, -probe), Layers::LEVEL)?
                {
                    let height = probe * hit.fraction - point.radius;
                    let upright = pin_fraction(
                        up.z,
                        definition.antigrav_normal_k0,
                        definition.antigrav_normal_k1,
                    );
                    let lift = if height > 0.0 {
                        1.0 - height / definition.antigrav_height
                    } else {
                        1.0
                    };
                    let magnitude = (lift * lift * GRAVITY
                        - hit.normal.dot(velocity) * definition.antigrav_damp_fraction)
                        * powered.antigrav_fraction
                        * definition.antigrav_strength
                        * physics.mass
                        * upright;
                    push += hit.normal * magnitude;
                    flags |= POINT_ANTIGRAVING;
                }
            }
        }

        let force =
            normal_force + ground_friction + water_pressure + water_friction + air_friction + push;
        total_force += force;
        total_torque += radius.cross(force);
        if let Some(slot) = state.points.get_mut(index) {
            *slot = flags;
        }
        points.push(Point { position });
    }
    state.in_water = submerged > 0;
    state.under_water = submerged > 0 && submerged == physics.mass_points.len();
    Ok((total_force, total_torque))
}

/// A tick of the mass points: their forces summed with the controller's,
/// the vehicle moved and spun by them, and backed out of the level.
fn physics_update(
    state: &mut VehicleState,
    physics: &Physics,
    powered: &mut [Powered; 32],
    magic_force: Vec3,
    magic_torque: Vec3,
    level: VehicleLevel<'_>,
) -> Result<(), PhysicsError> {
    let world = level.world;
    let mut points = Vec::with_capacity(physics.mass_points.len());
    let (force, torque) = compute_new(state, physics, powered, level, &mut points)?;
    // (and the push of the vehicles it met, spent)
    let force = force + magic_force + std::mem::take(&mut state.collision_force);
    let torque = torque + magic_torque + std::mem::take(&mut state.collision_torque);

    let acceleration = if physics.mass > 0.0 {
        force / physics.mass
    } else {
        Vec3::ZERO
    };
    let mut velocity = state.velocity + acceleration;
    let mut position = state.position + velocity;
    let frame = frame(state.forward, state.up);
    let angular_acceleration = match physics.inertial_matrix {
        Some(matrices) => {
            let inverse = Mat3::from_cols_array_2d(&matrices[1]);
            frame * inverse * frame.transpose() * torque
        }
        None => Vec3::ZERO,
    };
    let mut spin = state.angular_velocity + angular_acceleration;
    let (mut forward, mut up) = spun(state.forward, state.up, spin);

    // (backed out of the level: each mass point's sweep from where it was,
    // the earliest hit taken, up to four times)
    let com = Vec3::from_array(physics.center_of_mass);
    let mut committed = false;
    for _ in 0..PENETRATION_PASSES {
        let moved = self::frame(forward, up);
        let mut earliest: Option<(f32, Vec3, Vec3)> = None;
        for (point, before) in physics.mass_points.iter().zip(&points) {
            let after = position + moved * (Vec3::from_array(point.position) - com);
            let delta = after - before.position;
            if delta.dot(delta) <= 0.0 {
                continue;
            }
            if let Some(hit) = world.cast_ray(before.position, delta, Layers::LEVEL)? {
                if earliest.is_none_or(|(fraction, _, _)| hit.fraction < fraction) {
                    earliest = Some((hit.fraction, hit.normal, delta));
                }
            }
        }
        let Some((fraction, normal, delta)) = earliest else {
            committed = true;
            break;
        };
        let along = delta.dot(normal);
        let margin = if along != 0.0 {
            0.0078125 / along.abs()
        } else {
            0.03125
        };
        let fraction = (fraction - margin).max(0.0);
        let into = normal.dot(velocity);
        if into < 0.0 {
            velocity += normal * ((fraction - 1.0) * into);
            position = state.position + velocity;
        }
        spin *= fraction;
        (forward, up) = spun(state.forward, state.up, spin);
    }
    if committed {
        state.position = position;
        state.forward = forward;
        state.up = up;
    }
    state.velocity = velocity;
    state.angular_velocity = spin;

    let mut resting = 0usize;
    let mut grounded = 0usize;
    for flags in &state.points {
        if flags & POINT_AT_REST != 0 {
            resting += 1;
        }
        if flags & POINT_ON_GROUND != 0 {
            grounded += 1;
        }
    }
    state.on_ground = grounded > 0;
    state.at_rest = resting == state.points.len()
        && grounded >= 3
        && velocity.dot(velocity) <= REST_SPEED_SQUARED
        && spin.dot(spin) <= REST_SPIN_SQUARED
        && acceleration.dot(acceleration) <= REST_ACCELERATION_SQUARED
        && angular_acceleration.dot(angular_acceleration) <= REST_ANGULAR_ACCELERATION_SQUARED;
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    /// A body on its side or back, `up` up, facing `forward`.
    fn lying(forward: Vec3, up: Vec3) -> VehicleState {
        let mut state = VehicleState::placed(&floater(1, 1.0), Vec3::ZERO, forward, up);
        state.at_rest = true;
        state
    }

    #[test]
    fn the_approach_is_the_side_pushed_from_or_the_end_stood_on() {
        let forward = Vec3::X;
        // (a player on its right, the world's -y as it faces +x)
        assert_eq!(
            Approach::of(Vec3::new(0.0, -2.0, 0.0), Vec3::ZERO, forward),
            Approach::Right
        );
        assert_eq!(
            Approach::of(Vec3::new(0.0, 2.0, 0.0), Vec3::ZERO, forward),
            Approach::Left
        );
        let tail = Vec3::new(0.1, 0.0, 0.99).normalize();
        assert_eq!(Approach::of(Vec3::X, Vec3::ZERO, tail), Approach::NoseUp);
        assert_eq!(Approach::of(Vec3::X, Vec3::ZERO, -tail), Approach::NoseDown);
    }

    #[test]
    fn a_vehicle_is_entered_above_the_flipping_angle_and_flipped_below_it() {
        let upright = lying(Vec3::X, Vec3::Z);
        assert!(upright.enterable(0.0) && !upright.is_flipped());
        let side = lying(Vec3::X, Vec3::new(0.0, 0.8, 0.6).normalize());
        // (at 45 degrees it must stand higher than sin 45)
        assert!(side.enterable(0.5));
        assert!(!side.enterable(core::f32::consts::FRAC_PI_4));
        let back = lying(Vec3::X, -Vec3::Z);
        assert!(!back.enterable(0.0) && back.is_flipped());
    }

    #[test]
    fn upending_spins_it_over_for_a_second_at_most() {
        let mut state = lying(Vec3::X, -Vec3::Z);
        assert!(state.start_upending(&floater(1, 1.0), Vec3::new(0.0, -2.0, 0.0)));
        assert!(!state.start_upending(&floater(1, 1.0), Vec3::ZERO));
        assert!(!state.at_rest);
        // (on its back it rolls at the fastest the tag allows, about its
        // forward axis, the way the push from its right turns it)
        upend(&mut state, VehicleKind::HumanJeep, [0.1, 0.4]);
        assert!(state.angular_velocity.distance(Vec3::X * -0.3 * 0.4) < 1e-6);
        // (limits the wrong way round give the fastest, without a panic)
        upend(&mut state, VehicleKind::HumanJeep, [0.4, 0.1]);
        assert!(state.angular_velocity.distance(Vec3::X * -0.3 * 0.1) < 1e-6);
        for _ in 2..UPENDING_TICKS {
            upend(&mut state, VehicleKind::HumanJeep, [0.1, 0.4]);
        }
        assert!(state.upending.is_some());
        upend(&mut state, VehicleKind::HumanJeep, [0.1, 0.4]);
        assert_eq!(state.upending, None);
    }

    #[test]
    fn upending_stops_once_it_is_up_and_holds_a_tank_to_its_tracks() {
        let mut state = lying(Vec3::X, -Vec3::Z);
        state.velocity = Vec3::new(0.2, 0.3, 0.0);
        assert!(state.start_upending(&floater(1, 1.0), Vec3::ZERO));
        upend(&mut state, VehicleKind::HumanTank, [0.0, 1.0]);
        assert!(state.velocity.distance(Vec3::new(0.2, 0.0, 0.0)) < 1e-6);
        let mut fighter = lying(Vec3::X, -Vec3::Z);
        fighter.velocity.z = 0.5;
        assert!(fighter.start_upending(&floater(1, 1.0), Vec3::ZERO));
        upend(&mut fighter, VehicleKind::AlienFighter, [0.0, 1.0]);
        assert!((fighter.velocity.z + 0.01).abs() < 1e-6);
        state.up = Vec3::Z;
        upend(&mut state, VehicleKind::HumanTank, [0.0, 1.0]);
        assert_eq!(state.upending, None);
    }

    const WARTHOG: SpeedParameters = SpeedParameters {
        positive: 0.255,
        negative: 0.1,
        acceleration: 0.00275,
        deceleration: 0.011,
    };

    #[test]
    fn a_speed_gains_its_acceleration_to_its_fastest() {
        let mut speed = 0.0;
        speed_update_seek(&mut speed, WARTHOG, 1.0, 1.0);
        assert!((speed - 0.00275).abs() < 1e-7);
        for _ in 0..200 {
            speed_update_seek(&mut speed, WARTHOG, 1.0, 1.0);
        }
        assert_eq!(speed, 0.255);
    }

    #[test]
    fn a_speed_loses_its_deceleration_and_stops_at_its_target() {
        let mut speed = 0.1;
        speed_update_seek(&mut speed, WARTHOG, 0.0, 1.0);
        assert!((speed - 0.089).abs() < 1e-7);
        for _ in 0..20 {
            speed_update_seek(&mut speed, WARTHOG, 0.0, 1.0);
        }
        assert_eq!(speed, 0.0);
        for _ in 0..200 {
            speed_update_seek(&mut speed, WARTHOG, -1.0, 1.0);
        }
        assert_eq!(speed, -0.1);
    }

    #[test]
    fn a_position_seeks_its_target_and_wraps() {
        let mut turn = 0.0;
        position_update_seek(&mut turn, [30.0, -30.0], false, 0.5, 0.1);
        assert!((turn - 0.1).abs() < 1e-7);
        position_update_seek(&mut turn, [30.0, -30.0], false, 0.15, 0.1);
        assert_eq!(turn, 0.15);
        let mut angle = 170.0;
        position_update_seek(&mut angle, [180.0, -180.0], true, -170.0, 15.0);
        assert_eq!(angle, -175.0);
    }

    #[test]
    fn pin_fraction_runs_either_way() {
        assert_eq!(pin_fraction(0.0, 0.25, 0.5), 0.0);
        assert_eq!(pin_fraction(1.0, 0.25, 0.5), 1.0);
        assert!((pin_fraction(0.375, 0.25, 0.5) - 0.5).abs() < 1e-6);
        assert_eq!(pin_fraction(0.0, 0.5, 0.25), 1.0);
        assert_eq!(pin_fraction(1.0, 0.5, 0.25), 0.0);
        assert!((pin_fraction(0.375, 0.5, 0.25) - 0.5).abs() < 1e-6);
    }

    #[test]
    fn friction_scales_along_and_across_its_axis() {
        let friction = Vec3::new(1.0, 2.0, 0.0);
        let scaled = friction_evaluate(friction, FrictionType::Forward, 0.5, 2.0, Vec3::X, Vec3::Z);
        assert!((scaled - Vec3::new(0.5, 4.0, 0.0)).length() < 1e-6);
        let left = friction_evaluate(friction, FrictionType::Left, 0.5, 2.0, Vec3::X, Vec3::Z);
        assert!((left - Vec3::new(2.0, 1.0, 0.0)).length() < 1e-6);
        assert_eq!(
            friction_evaluate(friction, FrictionType::Point, 0.5, 2.0, Vec3::X, Vec3::Z),
            friction
        );
    }

    #[test]
    fn a_rotation_matrix_and_its_quaternion_agree() {
        let q = Quat::from_axis_angle(Vec3::new(1.0, 2.0, 3.0).normalize(), 1.2);
        let back = rotation_of(Mat3::from_quat(q));
        assert!(back.dot(q).abs() > 0.99999);
        let (axis, angle) = axis_angle(Mat3::from_quat(q));
        assert!((angle - 1.2).abs() < 1e-4);
        assert!((axis - Vec3::new(1.0, 2.0, 3.0).normalize()).length() < 1e-4);
    }

    #[test]
    fn a_wheel_rolls_round_its_circumference() {
        assert!((roll(1.0, 0.5, 1.2) - 0.3).abs() < 1e-6);
        assert!((roll(0.1, -0.3, 1.2) - 1.0).abs() < 1e-6);
        assert_eq!(roll(0.5, 0.25, 0.0), 0.5);
    }

    #[test]
    fn the_angle_between_two_vectors_runs_to_a_half_turn() {
        use std::f32::consts::{FRAC_PI_2, PI};
        assert!(angle_between(Vec3::X, Vec3::X).abs() < 1e-3);
        assert!((angle_between(Vec3::X, Vec3::Y * 2.0) - FRAC_PI_2).abs() < 1e-3);
        assert!((angle_between(Vec3::X, -Vec3::X) - PI).abs() < 1e-3);
        assert_eq!(angle_between(Vec3::ZERO, Vec3::X), 0.0);
    }

    /// Water whose surface is at z = 0 everywhere: one leaf, one cluster.
    fn sea() -> BspWater {
        use opence_content::level::Bsp3dNode;
        use opence_content::{ClusterWater, CollisionBsp};
        BspWater {
            collision: CollisionBsp {
                bsp3d_nodes: vec![Bsp3dNode {
                    plane: 0,
                    children: [i32::MIN, i32::MIN],
                }],
                planes: vec![[0.0, 0.0, 1.0, 0.0]],
                ..CollisionBsp::default()
            },
            leaf_clusters: vec![Some(0)],
            cluster_water: vec![Some(ClusterWater {
                plane: Some([0.0, 0.0, 1.0, 0.0]),
                plane_distance: 0.0,
            })],
        }
    }

    /// A body of `points` mass points (each `density`, powered by the
    /// powered mass point of its index), spread across its left axis.
    fn floater(points: usize, density: f32) -> Physics {
        use opence_content::{Group, MassPoint, TagRef};
        let powered = PoweredMassPoint {
            name: String::new(),
            flags: PoweredMassPoint::WATER_FRICTION | PoweredMassPoint::WATER_LIFT,
            antigrav_strength: 0.0,
            antigrav_offset: 0.0,
            antigrav_height: 0.0,
            antigrav_damp_fraction: 0.0,
            antigrav_normal_k1: 0.0,
            antigrav_normal_k0: 0.0,
        };
        let mass = 1.0 / points as f32;
        Physics {
            tag: TagRef {
                group: Group(*b"phys"),
                path: String::new(),
            },
            radius: 1.0,
            moment_scale: 1.0,
            mass: 1.0,
            center_of_mass: [0.0; 3],
            density,
            gravity_scale: 1.0,
            ground_friction: 0.2,
            ground_depth: 0.15,
            ground_damp_fraction: 0.05,
            ground_normal_k1: 0.7,
            ground_normal_k0: 0.5,
            water_friction: 0.05,
            water_depth: 0.25,
            water_density: 1.0,
            air_friction: 0.001,
            xx_moment: 1.0,
            yy_moment: 1.0,
            zz_moment: 1.0,
            inertial_matrix: Some([[[1.0, 0.0, 0.0], [0.0, 1.0, 0.0], [0.0, 0.0, 1.0]]; 2]),
            powered_mass_points: vec![powered; points],
            mass_points: (0..points)
                .map(|index| MassPoint {
                    name: String::new(),
                    powered_mass_point: u16::try_from(index).ok(),
                    model_node: None,
                    flags: 0,
                    relative_mass: mass,
                    mass,
                    relative_density: density,
                    density,
                    position: [0.0, index as f32 - (points as f32 - 1.0) * 0.5, 0.0],
                    forward: [1.0, 0.0, 0.0],
                    up: [0.0, 0.0, 1.0],
                    friction_type: FrictionType::Point,
                    friction_parallel_scale: 1.0,
                    friction_perpendicular_scale: 1.0,
                    radius: 0.1,
                })
                .collect(),
        }
    }

    #[test]
    fn what_is_lighter_than_water_floats_and_what_is_heavier_sinks() -> Result<(), PhysicsError> {
        let world = World::new(opence_physics::WorldSettings::default())?;
        let water = sea();
        let level = VehicleLevel {
            world: &world,
            floor: 0.0,
            ceiling: 0.0,
            water: Some(&water),
            surfaces: None,
            materials: &[],
        };
        let light = floater(1, 0.5);
        let mut state = VehicleState::placed(&light, Vec3::new(0.0, 0.0, -2.0), Vec3::X, Vec3::Z);
        for _ in 0..600 {
            let mut powered = [Powered::identity(); 32];
            physics_update(
                &mut state,
                &light,
                &mut powered,
                Vec3::ZERO,
                Vec3::ZERO,
                level,
            )?;
        }
        // (held where the water's pressure meets its weight: half its
        // floating depth under)
        assert!(state.in_water && state.under_water);
        assert!(
            (state.position.z + 0.125).abs() < 0.05,
            "{}",
            state.position.z
        );
        assert!(state.points.iter().all(|flags| flags & POINT_IN_WATER != 0));

        let heavy = floater(1, 2.0);
        let mut state = VehicleState::placed(&heavy, Vec3::new(0.0, 0.0, -2.0), Vec3::X, Vec3::Z);
        for _ in 0..60 {
            let mut powered = [Powered::identity(); 32];
            physics_update(
                &mut state,
                &heavy,
                &mut powered,
                Vec3::ZERO,
                Vec3::ZERO,
                level,
            )?;
        }
        assert!(state.position.z < -2.0, "{}", state.position.z);

        // (and out of water nothing holds it up)
        let dry = VehicleLevel {
            water: None,
            ..level
        };
        let mut state = VehicleState::placed(&light, Vec3::new(0.0, 0.0, -2.0), Vec3::X, Vec3::Z);
        let mut powered = [Powered::identity(); 32];
        physics_update(
            &mut state,
            &light,
            &mut powered,
            Vec3::ZERO,
            Vec3::ZERO,
            dry,
        )?;
        assert!(!state.in_water && state.velocity.z < 0.0);
        Ok(())
    }

    #[test]
    fn a_boat_rights_itself_and_its_rudder_turns_it() -> Result<(), PhysicsError> {
        let world = World::new(opence_physics::WorldSettings::default())?;
        let water = sea();
        let level = VehicleLevel {
            world: &world,
            floor: 0.0,
            ceiling: 0.0,
            water: Some(&water),
            surfaces: None,
            materials: &[],
        };
        let boat_physics = floater(3, 0.5);
        // (rolled most of a quarter turn onto its side)
        let rolled = Vec3::new(0.0, -libm::sinf(1.2), libm::cosf(1.2));
        let mut state = VehicleState::placed(&boat_physics, Vec3::ZERO, Vec3::X, rolled);
        for _ in 0..300 {
            boat(&mut state, &boat_physics, level)?;
        }
        assert!(state.up.z > 0.98, "{}", state.up);

        // (driven ahead with the rudder over, it turns)
        state.speed = 0.1;
        state.turn = 0.5;
        let start = state.forward;
        for _ in 0..120 {
            boat(&mut state, &boat_physics, level)?;
        }
        assert!(
            state.velocity.dot(state.forward) > 0.0,
            "{}",
            state.velocity
        );
        assert!(state.forward.dot(start) < 0.99, "{}", state.forward);

        // (and a boat without its three powered mass points only floats)
        let raft = floater(2, 0.5);
        let mut state = VehicleState::placed(&raft, Vec3::ZERO, Vec3::X, Vec3::Z);
        state.speed = 0.1;
        for _ in 0..30 {
            boat(&mut state, &raft, level)?;
        }
        assert!(state.velocity.x.abs() < 1e-3, "{}", state.velocity);
        Ok(())
    }

    #[test]
    fn a_material_scales_the_ground_s_terms() {
        let mut physics = floater(1, 0.5);
        assert_eq!(
            Ground::of(&physics, None),
            Ground {
                friction: 0.2,
                normal_k1: 0.7,
                normal_k0: 0.5,
                depth: 0.15,
                damp_fraction: 0.05,
            }
        );
        let ice = MaterialPhysics {
            ground_friction_scale: 0.5,
            ground_normal_k1_scale: 2.0,
            ground_normal_k0_scale: -1.0,
            ground_depth_scale: 0.0,
            ground_damp_fraction_scale: 3.0,
        };
        let scaled = Ground::of(&physics, Some(&ice));
        assert!((scaled.friction - 0.1).abs() < 1e-6);
        assert!((scaled.normal_k1 - 1.4).abs() < 1e-6);
        // (a scale of 0 or less leaves the vehicle's own)
        assert_eq!(scaled.normal_k0, 0.5);
        assert_eq!(scaled.depth, 0.15);
        assert!((scaled.damp_fraction - 0.15).abs() < 1e-6);
        // (a vehicle heavier than 7500 keeps its friction)
        physics.mass = 8000.0;
        let heavy = Ground::of(&physics, Some(&ice));
        assert_eq!(heavy.friction, 0.2);
        assert!((heavy.normal_k1 - 1.4).abs() < 1e-6);
    }

    #[test]
    fn a_slide_slows_by_what_the_ground_is_made_of() -> Result<(), PhysicsError> {
        use opence_physics::{LevelMesh, WorldSettings};

        let mut world = World::new(WorldSettings::default())?;
        let mesh = LevelMesh {
            vertices: vec![
                [-50.0, -50.0, 0.0],
                [50.0, -50.0, 0.0],
                [50.0, 50.0, 0.0],
                [-50.0, 50.0, 0.0],
            ],
            triangles: vec![[0, 1, 2], [0, 2, 3]],
            materials: vec![Some(0), Some(0)],
        };
        world.set_level(&mesh)?;
        // (collision material 0 is of material type 1, whose friction is
        // four times)
        let surfaces = SurfaceMaterials::new(&mesh, &[1]);
        let rough = [
            MaterialPhysics::default(),
            MaterialPhysics {
                ground_friction_scale: 4.0,
                ..MaterialPhysics::default()
            },
        ];
        let slide = |physics: &Physics, materials: &[MaterialPhysics]| {
            let level = VehicleLevel {
                world: &world,
                floor: 0.0,
                ceiling: 0.0,
                water: None,
                surfaces: Some(&surfaces),
                materials,
            };
            // (resting on the ground at its depth, sliding along x)
            let mut state =
                VehicleState::placed(physics, Vec3::new(0.0, 0.0, 0.1), Vec3::X, Vec3::Z);
            state.velocity = Vec3::new(0.1, 0.0, 0.0);
            for _ in 0..5 {
                let mut powered = [Powered::identity(); 32];
                physics_update(
                    &mut state,
                    physics,
                    &mut powered,
                    Vec3::ZERO,
                    Vec3::ZERO,
                    level,
                )?;
            }
            Ok::<_, PhysicsError>(state)
        };
        let mut physics = floater(1, 0.5);
        physics.mass_points[0].radius = 0.25;
        let plain = slide(&physics, &rough[..1])?;
        let roughened = slide(&physics, &rough)?;
        assert!(plain.on_ground && roughened.on_ground);
        assert!(plain.velocity.x > 0.0, "{}", plain.velocity);
        assert!(
            roughened.velocity.x < plain.velocity.x * 0.5,
            "{} against {}",
            roughened.velocity,
            plain.velocity
        );
        // (a vehicle heavier than 7500 slides as on any ground)
        physics.mass = 8000.0;
        let heavy = slide(&physics, &rough[..1])?;
        let heavy_rough = slide(&physics, &rough)?;
        assert_eq!(heavy.velocity, heavy_rough.velocity);
        Ok(())
    }

    #[test]
    fn two_vehicles_that_meet_push_apart_and_wake() -> Result<(), PhysicsError> {
        let world = World::new(opence_physics::WorldSettings::default())?;
        let physics = floater(1, 0.5);
        let mut first = VehicleState::placed(&physics, Vec3::ZERO, Vec3::X, Vec3::Z);
        let mut second =
            VehicleState::placed(&physics, Vec3::new(0.15, 0.0, 0.0), Vec3::X, Vec3::Z);
        first.at_rest = true;
        second.at_rest = true;
        assert!(collide((&mut first, &physics), (&mut second, &physics)));
        assert!(!first.at_rest && !second.at_rest);
        // (radii 0.1 and 0.1, 0.15 apart: half of 0.05 deep, pushed along x)
        let expected = 2.0 * GRAVITY / COLLISION_DEPTH * 0.025;
        assert!((first.collision_force.x + expected).abs() < 1e-6);
        assert!((second.collision_force.x - expected).abs() < 1e-6);
        assert_eq!(first.collision_force + second.collision_force, Vec3::ZERO);
        // (spent on the next step, which moves them apart)
        let level = VehicleLevel {
            world: &world,
            floor: 0.0,
            ceiling: 0.0,
            water: None,
            surfaces: None,
            materials: &[],
        };
        let mut powered = [Powered::identity(); 32];
        physics_update(
            &mut first,
            &physics,
            &mut powered,
            Vec3::ZERO,
            Vec3::ZERO,
            level,
        )?;
        assert_eq!(first.collision_force, Vec3::ZERO);
        assert!(first.velocity.x < 0.0);
        // (far apart, they do not meet)
        let mut far = VehicleState::placed(&physics, Vec3::new(5.0, 0.0, 0.0), Vec3::X, Vec3::Z);
        far.at_rest = true;
        assert!(!collide((&mut first, &physics), (&mut far, &physics)));
        assert!(far.at_rest);
        // (a map's negative mass pushes neither, rather than making a NaN)
        let mut weightless = physics.clone();
        weightless.mass = -1.0;
        let mut third = VehicleState::placed(&physics, Vec3::ZERO, Vec3::X, Vec3::Z);
        let mut fourth =
            VehicleState::placed(&weightless, Vec3::new(0.15, 0.0, 0.0), Vec3::X, Vec3::Z);
        assert!(collide((&mut third, &physics), (&mut fourth, &weightless)));
        assert_eq!(third.collision_force, Vec3::ZERO);
        assert_eq!(fourth.collision_force, Vec3::ZERO);
        Ok(())
    }
}
